/*
    src/trampoline.cpp: support for overriding virtual functions in Python

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/trampoline.h>
#include <atomic>
#include <cstring>
#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/* === Overview ===

   Trampolines forward C++ method calls to Python if the derived Python type
   overrides them. Detecting the presence of an override requires:

   1a. acquiring the GIL on regular builds.
   1b. attaching the thread state in free-threaded builds (a mutex acquire
       and an allocation for threads created via ``std::thread``).
   2. performing a Python attribute lookup.

   This is very expensive. If done naively, step 1 can degrade the
   performance of parallel C++ applications even if no overrides are present.

   The trampoline infrastructure therefore caches whether an override is
   present to pay this cost only once. This cache is stored within the Python
   type object; ``trampoline_enter()`` probes it without locking or Python
   API calls and only falls back to the expensive resolution above on the
   first dispatch of each method. The implementation must also detect when
   the user monkey-patches the type, in which case the cache must be
   invalidated.

   === Implementation ===

   The cache is a hash table mapping a method name to one of two values:

   - an interned Python string object encoding the name (override exists).
     The extension passes it to getattr() to fetch and invoke the override.
   - ``NB_TRAMPOLINE_NO_OVERRIDE`` (override missing)

   A missing entry means that the name is not resolved yet. Each method name
   arrives with a 64-bit FNV-1a hash that the extension computed at compile
   time. The table uses open addressing with linear probing at a load factor
   of at most one half; entries are keyed by the literal's address, and the
   same name compiled into several extension objects is matched by content.

   === Life cycle and invalidation ===

   The table cannot be safely modified or resized since readers may access it
   concurrently without holding a lock. The implementation instead builds a
   complete replacement and publishes it with a release store. All created
   tables are chained per type and freed upon garbage collection of the type.
   No reader can remain at that point because its existence would require
   holding a reference to the type.

   Re-publishing after each insertion and keeping old versions wastes some
   memory, which is acceptable because the number of trampoline slots is
   normally very small.

   Assigning or deleting a type attribute drops the tables of the modified type
   and of all transitive subclasses, whose tables cache resolutions against
   their full MRO. Attribute assignment is very common when binding nanobind
   classes (once per method), and walking the subclass tree each time would add
   significant overhead to imports. The implementation therefore marks types
   whose subtree may contain override tables, using the low bit of
   ``type_data::trampoline_allocs``. The first publication into a type's
   table sets it on the type and every nanobind base in its MRO. The walk
   drops the table of each marked type and recurses into its subclasses, and
   it stops at unmarked types since no table can exist below them.

   === Concurrency ===

   Readers are lock-free and may run on threads without interpreter state. A
   stale reader at worst misses the shortcut and performs a full resolution.
   Writers (publication, marking, and the invalidation walk) hold the GIL
   and, in free-threaded builds, a critical section on the type they modify,
   which serializes them per type.

   The name lookup in trampoline_enter() runs arbitrary Python code and must
   therefore happen outside the critical section. A type modification can slip
   in between the lookup and the publication of its result. The counter
   nb_internals::trampoline_epoch guards against this: the invalidation walk
   increments it before dropping any table, and trampoline_enter() reads it
   before the lookup and compares inside the critical section, repeating the
   lookup if it changed. Marks and subclasses that the walk did not see were
   created after the modification and cannot hold stale entries.
*/

/// Indicates that a trampoline method is not overridden in Python
#define NB_TRAMPOLINE_NO_OVERRIDE ((void *) 1)

/// Slot of a per-type override table
struct trampoline_entry {
    const char *name; ///< Literal identity; nullptr marks a free slot
    void *value;      ///< NB_TRAMPOLINE_NO_OVERRIDE or the interned name
};

/// Header of an immutable per-type override table
struct trampoline_table {
    trampoline_table *next; ///< Chain of the type's retired tables
    uint32_t mask;          ///< Capacity (a power of two) minus one
    uint32_t count;         ///< Number of used entries

    /// Entries follow the header contiguously
    trampoline_entry *entries() const {
        return (trampoline_entry *) (this + 1);
    }

    /// Initial probe position of 'hash'. Fibonacci hashing: multiplication
    /// by 2^64 divided by the golden ratio mixes the hash before the
    /// truncation to the table size.
    uint32_t slot(uint64_t hash) const {
        return (uint32_t) ((hash * 0x9E3779B97F4A7C15ull) >> 32) & mask;
    }
};

static inline std::atomic<trampoline_table *> &table_cell(type_data *td) {
    return *(std::atomic<trampoline_table *> *) &td->trampoline_table_pub;
}

/// type_data::trampoline_allocs packs two things into one word: the chain of
/// table allocations owned by the type, and the subtree mark in the low bit.
/// The accessors below decode it.
static inline std::atomic<uintptr_t> &alloc_word(type_data *td) {
    return *(std::atomic<uintptr_t> *) &td->trampoline_allocs;
}

static inline trampoline_table *alloc_chain(uintptr_t w) {
    return (trampoline_table *) (w & ~(uintptr_t) 1);
}

static inline bool subtree_marked(type_data *td) {
    return (alloc_word(td).load(std::memory_order_relaxed) & 1) != 0;
}

/// Release increment of the modification counter (see the file comment)
static inline void epoch_inc(nb_internals *p) {
#if defined(NB_FREE_THREADED)
    p->trampoline_epoch.value.fetch_add(1, std::memory_order_release);
#else
    p->trampoline_epoch.value++;
#endif
}

/// Set the subtree mark on 'tp' and every nanobind base in its MRO. The
/// invalidation walk skips unmarked subtrees (see big comment above). Runs
/// once per type, before its first override entry is published.
static void trampoline_mark(PyTypeObject *tp) noexcept {
    nb_internals *int_p = nb_type_data(tp)->internals;

    PyObject *mro = PyObject_GetAttrString((PyObject *) tp, "__mro__");
    check(mro && PyTuple_Check(mro),
          "nanobind::detail::trampoline_enter(): could not fetch the MRO!");

    // Bases first, 'tp' last: later publications only check 'tp' to skip
    // this function, so its mark must not become visible before the others
    for (Py_ssize_t i = PyTuple_Size(mro) - 1; i >= 0; --i) {
        PyObject *base = PyTuple_GetItem(mro, i); // borrowed
        if (!base || !PyType_Check(base) ||
            !PyType_IsSubtype(Py_TYPE(base), int_p->nb_type))
            continue;
        type_data *td = nb_type_data((PyTypeObject *) base);
        // Marks are one-way, and a marked type has marked ancestors, so
        // types that are already marked need no further ordering work
        if (subtree_marked(td))
            continue;
        ft_object_guard guard(base);
        alloc_word(td).fetch_or(1, std::memory_order_relaxed);
    }

    Py_DECREF(mro);
}

PyObject *trampoline_new(nb_internals *p, void *ptr) noexcept {
    // GIL is held when the trampoline constructor runs. Lock the
    // associated instance shard in GIL-less Python.
    nb_shard &shard = p->shard(ptr);
    lock_shard lock(shard);

    nb_ptr_map &inst_c2p = shard.inst_c2p;
    nb_ptr_map::iterator it = inst_c2p.find(ptr);
    check(it != inst_c2p.end() && (((uintptr_t) it->second) & 1) == 0,
          "nanobind::detail::trampoline_new(): unique instance not found!");
    return (PyObject *) it->second;
}

/// Lock-free table lookup; null means "not resolved yet"
static void *trampoline_probe(type_data *td, const char *name, uint64_t hash) {
    trampoline_table *t = table_cell(td).load(std::memory_order_acquire);
    if (t) {
        for (uint32_t i = t->slot(hash); t->entries()[i].name;
             i = (i + 1) & t->mask) {
            trampoline_entry &e = t->entries()[i];
            // Entries are keyed by literal address; the same method name
            // compiled into several extension objects is matched by content
            if (e.name == name || strcmp(e.name, name) == 0)
                return e.value;
        }
    }
    return nullptr;
}

/// Determine whether 'tp' overrides the method 'name'. The function looks up
/// the raw MRO entry without descriptor binding and treats one of nanobind's
/// own function types as "not overridden". Returns NB_TRAMPOLINE_NO_OVERRIDE,
/// the interned name (strong reference), or null with '*error' set. GIL is
/// held.
static void *trampoline_resolve(PyTypeObject *tp, const char *name,
                           const char **error) {
    PyObject *key = PyUnicode_InternFromString(name);
    if (!key) {
        PyErr_Clear();
        *error = "could not intern string";
        return nullptr;
    }

    nb_internals *int_p = nb_type_data(tp)->internals;
    PyObject *raw = type_lookup(int_p, (PyObject *) tp, key);

    if (!raw) {
        Py_DECREF(key);
        *error = "lookup failed";
        return nullptr;
    }

    PyTypeObject *raw_tp = Py_TYPE(raw);
    Py_DECREF(raw);

    if (raw_tp == int_p->nb_func || raw_tp == int_p->nb_method ||
        raw_tp == int_p->nb_bound_method) {
        Py_DECREF(key);
        return NB_TRAMPOLINE_NO_OVERRIDE;
    }

    return key;
}

/// Publish a resolved value, consuming its reference. The caller holds the
/// GIL and the type's critical section. Builds and publishes a replacement
/// table holding the previous entries plus (name, value).
static void trampoline_publish(type_data *td, const char *name, uint64_t hash,
                               void *value) {
    if (trampoline_probe(td, name, hash)) {
        // Raced with an identical publication; interning makes the values
        // equal, so ours only holds a redundant reference
        if (value != NB_TRAMPOLINE_NO_OVERRIDE)
            Py_DECREF((PyObject *) value);
        return;
    }

    trampoline_table *cur = table_cell(td).load(std::memory_order_relaxed);
    uint32_t count = (cur ? cur->count : 0) + 1, capacity = 4;
    while (capacity < 2 * count)
        capacity *= 2;

    trampoline_table *t = (trampoline_table *) malloc_check(
        sizeof(trampoline_table) + capacity * sizeof(trampoline_entry));

    // Chain the allocation and set the type's mark bit, whose presence
    // tells the invalidation walk that tables may exist in this subtree
    t->next = alloc_chain(alloc_word(td).load(std::memory_order_relaxed));
    alloc_word(td).store((uintptr_t) t | 1, std::memory_order_relaxed);

    t->mask = capacity - 1;
    t->count = count;
    memset((void *) t->entries(), 0, capacity * sizeof(trampoline_entry));

    auto insert = [t](const char *n, void *v) {
        uint32_t i = t->slot(str_hash(n));
        while (t->entries()[i].name)
            i = (i + 1) & t->mask;
        t->entries()[i] = { n, v };
    };

    insert(name, value);
    if (cur) {
        for (uint32_t i = 0; i <= cur->mask; ++i) {
            trampoline_entry &e = cur->entries()[i];
            if (!e.name)
                continue;
            // Copied entries share ownership of their interned keys
            if (e.value != NB_TRAMPOLINE_NO_OVERRIDE)
                Py_INCREF((PyObject *) e.value);
            insert(e.name, e.value);
        }
    }

    table_cell(td).store(t, std::memory_order_release);
}

static NB_THREAD_LOCAL ticket *current_ticket = nullptr;

void trampoline_enter(PyObject *self, const char *name, uint64_t hash,
                      bool pure, ticket *t) {
    // The type is looked up on every dispatch so that '__class__'
    // reassignment redirects future calls and cannot leave a dangling
    // reference to a collected type behind
    type_data *td = nb_type_data(Py_TYPE(self));
    nb_internals *int_p = td->internals;
    const char *error = nullptr;
    void *state = nullptr;

    void *value = trampoline_probe(td, name, hash);

    if (!value) {
        state = attach_tstate();
        if (!state)
            goto shutdown;

        // The load above may have raced with a '__class__' reassignment
        td = nb_type_data(Py_TYPE(self));

        if (!subtree_marked(td))
            trampoline_mark(Py_TYPE(self));

        for (;;) {
            size_t epoch = int_p->trampoline_epoch.load_acquire();

            value = trampoline_probe(td, name, hash);
            if (value)
                break;

            value = trampoline_resolve(td->type_py, name, &error);
            if (!value)
                goto fail;

            bool done = false;
            // The critical section must end before the thread state is
            // released, hence the block scope.
            {
                ft_object_guard guard((PyObject *) td->type_py);
                if (epoch == int_p->trampoline_epoch.load_relaxed()) {
                    trampoline_publish(td, name, hash, value);
                    done = true;
                }
            }
            if (done)
                break;

            // Raced with a type modification; resolve again
            if (value != NB_TRAMPOLINE_NO_OVERRIDE)
                Py_DECREF((PyObject *) value);
        }
    }

    if (value == NB_TRAMPOLINE_NO_OVERRIDE) {
        if (pure) {
            error = "tried to call a pure virtual function";
            goto fail;
        }
        if (state)
            detach_tstate(state);
        return; // 't->key' stays null; the caller runs the C++ base method
    }

    if (!state) {
        state = attach_tstate();
        if (!state)
            goto shutdown;
    }

    t->state = state;
    t->key = (PyObject *) value;
    t->self = self;
    t->prev = current_ticket;

    if (t->prev && t->prev->self.is(t->self) && t->prev->key.is(t->key)) {
        // The override itself invoked the bound base implementation, which
        // re-dispatched here through the C++ vtable. Deliver the call to
        // the C++ base method instead of recursing endlessly.
        t->self = handle();
        t->key = handle();
        t->prev = nullptr;
        detach_tstate(state);
        if (pure)
            raise("nanobind::detail::trampoline_enter('%s()'): tried to call "
                  "a pure virtual function!", name);
        return;
    }

    current_ticket = t;
    return;

shutdown:
    // The interpreter is finalizing and refuses further thread state
    // attachments. Defer to the C++ base method if the caller has one.
    if (!pure)
        return;
    error = "the Python interpreter is shutting down";

fail:
    if (state)
        detach_tstate(state);
    raise("nanobind::detail::trampoline_enter('%s::%s()'): %s!",
          td->name, name, error);
}

void trampoline_leave(ticket *t) noexcept {
    current_ticket = t->prev;
    detach_tstate(t->state);
}

/// Drop the published tables in the marked part of 'tp's subtree. 'meth'
/// lazily caches the unbound type.__subclasses__ descriptor for the walk.
static void trampoline_invalidate_rec(nb_internals *int_p, PyObject *tp,
                                      PyObject *&meth) noexcept {
    if (!PyType_Check(tp) || !PyType_IsSubtype(Py_TYPE(tp), int_p->nb_type))
        return;

    type_data *td = nb_type_data((PyTypeObject *) tp);
    {
        // Serialize against publishers and markers of this type (see the
        // file comment). An unmarked type has no trampoline instances at
        // or below it, so the entire branch can be skipped.
        ft_object_guard guard(tp);
        if (!subtree_marked(td))
            return;
        table_cell(td).store(nullptr, std::memory_order_release);
    }

#if !defined(Py_LIMITED_API) && !defined(NB_FREE_THREADED) && !defined(PYPY_VERSION)
    // The GIL orders subclass registration against this walk, so an
    // empty tp_subclasses field proves that there is nothing to visit.
    // PyPy does not maintain this field; use __subclasses__ there instead.
    if (!((PyTypeObject *) tp)->tp_subclasses)
        return;
#endif

    if (!meth) {
        // The unbound type.__subclasses__, so that a shadowing definition
        // on a subclass cannot divert the walk
        meth = PyObject_GetAttrString((PyObject *) &PyType_Type,
                                      "__subclasses__");
        if (!meth) {
            PyErr_Clear();
            return;
        }
    }

    PyObject *subs = call_one_arg(meth, tp);
    if (!subs) {
        PyErr_Clear();
        return;
    }

    Py_ssize_t n = PySequence_Length(subs);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject *sub = PySequence_GetItem(subs, i);
        if (!sub)
            break;
        trampoline_invalidate_rec(int_p, sub, meth);
        Py_DECREF(sub);
    }

    PyErr_Clear();
    Py_DECREF(subs);
}

void nb_trampoline_invalidate(PyObject *tp) noexcept {
    nb_internals *int_p = nb_type_data((PyTypeObject *) tp)->internals;
    epoch_inc(int_p);

    PyObject *meth = nullptr;
    trampoline_invalidate_rec(int_p, tp, meth);
    Py_XDECREF(meth);
}

void nb_trampoline_free(type_data *td) noexcept {
    trampoline_table *t =
        alloc_chain(alloc_word(td).load(std::memory_order_relaxed));

    while (t) {
        trampoline_table *next = t->next;
        for (uint32_t i = 0; i <= t->mask; ++i) {
            trampoline_entry &e = t->entries()[i];
            if (e.name && e.value != NB_TRAMPOLINE_NO_OVERRIDE)
                Py_DECREF((PyObject *) e.value);
        }
        free(t);
        t = next;
    }

    td->trampoline_allocs = nullptr;
    td->trampoline_table_pub = nullptr;
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
