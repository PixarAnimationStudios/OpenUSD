/*
    src/nb_type.cpp: libnanobind functionality for binding classes

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include "nb_internals.h"
#include "nb_ft.h"


#if defined(_MSC_VER)
#  pragma warning(disable: 4706) // assignment within conditional expression
#endif

#if !defined(Py_tp_vectorcall)
#  define Py_tp_vectorcall 82
#endif

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

static PyObject **nb_dict_ptr(PyObject *self, PyTypeObject *tp) {
    Py_ssize_t dictoffset = nb_type_data(tp)->dictoffset;
    return dictoffset ? (PyObject **) ((uint8_t *) self + dictoffset) : nullptr;
}

static PyObject **nb_weaklist_ptr(PyObject *self, PyTypeObject *tp) {
    Py_ssize_t weaklistoffset = nb_type_data(tp)->weaklistoffset;
    return weaklistoffset ? (PyObject **) ((uint8_t *) self + weaklistoffset) : nullptr;
}

static PyGetSetDef inst_getset[] = {
    { "__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict, nullptr, nullptr },
    { nullptr, nullptr, nullptr, nullptr, nullptr }
};

static int inst_clear(PyObject *self) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject **dict = nb_dict_ptr(self, tp);
    if (dict)
        Py_CLEAR(*dict);
    return 0;
}

static int inst_traverse(PyObject *self, visitproc visit, void *arg) {
    PyTypeObject *tp = Py_TYPE(self);
    PyObject **dict = nb_dict_ptr(self, tp);
    if (dict)
        Py_VISIT(*dict);
    Py_VISIT(tp);
    return 0;
}

static int inst_init(PyObject *self, PyObject *, PyObject *) {
    const type_data *t = nb_type_data(Py_TYPE(self));
    PyErr_Format(PyExc_TypeError, "%s: no constructor defined!", t->name);
    return -1;
}

/// Function to check (in the fastest manner) if a nb_type is garbage-collected.
static NB_INLINE bool nb_type_has_gc(PyTypeObject *tp, uint32_t flags) {
#if defined(Py_LIMITED_API)
    (void) tp;
    return flags & (uint32_t) type_flags_internal::has_gc;
#else
    (void) flags;
    return PyType_HasFeature(tp, Py_TPFLAGS_HAVE_GC);
#endif
}

static int nb_type_init_py(PyTypeObject *self);

/// 'type_data::internals' doubles as the marker of an initialized record.
/// Free-threaded builds publish it with release/acquire semantics so that a
/// thread observing the marker also observes the rest of the record.
static NB_INLINE nb_internals *internals_load(type_data *t) {
#if defined(NB_FREE_THREADED)
    return ((std::atomic<nb_internals *> *) &t->internals)
        ->load(std::memory_order_acquire);
#else
    return t->internals;
#endif
}

static NB_INLINE void internals_store(type_data *t, nb_internals *p) {
#if defined(NB_FREE_THREADED)
    ((std::atomic<nb_internals *> *) &t->internals)
        ->store(p, std::memory_order_release);
#else
    t->internals = p;
#endif
}

/// Python subclasses of bindings are normally initialized by nb_type_init(),
/// but there are ways to create uninitialized ones (e.g., via type.__new__()).
/// This function detects and fixes it. Returns false upon failure.
static NB_INLINE bool nb_type_ensure(PyTypeObject *tp) {
    return NB_LIKELY(internals_load(nb_type_data(tp)) != nullptr) ||
           nb_type_init_py(tp) == 0;
}

/// Allocate memory for a nb_type instance with internal storage
PyObject *inst_new_int(PyTypeObject *tp, PyObject * /* args */,
                       PyObject * /*kwd */) {
    const type_data *t = nb_type_data(tp);
    if (NB_UNLIKELY(!nb_type_ensure(tp)))
        return nullptr;
    uint32_t flags = t->flags;

    // Instance pool fast path
    if (NB_LIKELY(flags & (uint32_t) type_flags::pooled)) {
        nb_inst_pool *pool = nb_pool_lookup((type_data *) t);
        if (pool && pool->count) {
            nb_inst *self = pool->slots[--pool->count];

            // Resurrect the dead object with a single reference.
            nb_resurrect((PyObject *) self);

            // Overwrite the status word in full. Pooled types are always
            // co-located, internal-storage, non-intrusive.
            nb_inst_state s {};
            s.state = nb_inst_state::state_uninitialized;
            s.direct = 1;
            s.internal = 1;
            s.destruct = 0;
            s.cpp_delete = 0;
            s.intrusive = 0;
            s.pad = 0;
            s.clear_keep_alive = 0;
            s.unused = 0;
            nb_inst_state_write(self, s);

            // Re-enable try_inc_ref for this object.
            nb_enable_try_inc_ref((PyObject *) self);

            // The revived object must hold a reference to its type object
            NB_INCREF_TYPE((PyObject *) tp);

            // Re-track GC instances
            if (NB_UNLIKELY(nb_type_has_gc(tp, flags)))
                PyObject_GC_Track((PyObject *) self);

            return (PyObject *) self;
        }
    }

    bool gc = nb_type_has_gc(tp, flags);

    nb_inst *self;
    if (NB_LIKELY(!gc))
        self = PyObject_New(nb_inst, tp);
    else
        self = (nb_inst *) PyType_GenericAlloc(tp, 0);

    if (NB_LIKELY(self)) {
        uint32_t align = t->align;
        bool intrusive = flags & (uint32_t) type_flags::intrusive_ptr;

        uintptr_t payload = (uintptr_t) (self + 1);

        if (NB_UNLIKELY(align > sizeof(void *)))
            payload = (payload + align - 1) & ~(uintptr_t(align) - 1);

        self->offset = (int32_t) ((intptr_t) payload - (intptr_t) self);

        nb_inst_state s {};
        s.direct = 1;
        s.internal = 1;
        s.state = nb_inst_state::state_uninitialized;
        s.destruct = 0;
        s.cpp_delete = 0;
        s.clear_keep_alive = 0;
        s.intrusive = intrusive;
        s.pad = 0;
        s.unused = 0;
        nb_inst_state_write(self, s);

        // Make the object compatible with nb_try_inc_ref (free-threaded builds only)
        nb_enable_try_inc_ref((PyObject *) self);

        // Update hash table that maps from C++ to Python instance
        nb_shard &shard = t->internals->shard((void *) payload);
        lock_shard guard(shard);
        auto [it, success] = shard.inst_c2p.try_emplace((void *) payload, self);
        check(success, "nanobind::detail::inst_new_int(): unexpected collision!");
    }

    return (PyObject *) self;
}

/// Allocate memory for a nb_type instance with external storage. In contrast to
/// 'inst_new_int()', this does not yet register the instance in the internal
/// data structures. The function 'inst_register()' must be used to do so.
PyObject *inst_new_ext(PyTypeObject *tp, void *value) {
    const type_data *t = nb_type_data(tp);
    uint32_t flags = t->flags;
    bool gc = nb_type_has_gc(tp, flags);

    nb_inst *self;
    if (NB_LIKELY(!gc)) {
        self = (nb_inst *) PyObject_Malloc(sizeof(nb_inst));
        if (!self)
            return PyErr_NoMemory();
        PyObject_Init((PyObject *) self, tp);
    } else {
        self = (nb_inst *) PyType_GenericAlloc(tp, 0);
        if (!self)
            return nullptr;
    }

    // Compute offset to instance value
    // Use uint64_t because subtracting tagged pointers (e.g., with
    // HardwareAddressSanitizer) may overflow, which is undefined behavior for
    // signed integers.
    int32_t offset = (int32_t) ((uintptr_t) value - (uintptr_t) self);

    bool direct = (intptr_t) self + offset == (intptr_t) value;
    if (NB_UNLIKELY(!direct)) {
        // Location is not representable as signed 32 bit offset
        if (!gc) {
            /// Allocate memory for an extra pointer
            nb_inst *self_2 =
                (nb_inst *) PyObject_Realloc(self, sizeof(nb_inst) + sizeof(void *));

            if (NB_UNLIKELY(!self_2)) {
                NB_DECREF_TYPE(tp);
                PyObject_Free(self);
                return PyErr_NoMemory();
            }

            self = self_2;
        }

        *(void **) (self + 1) = value;
        offset = (int32_t) sizeof(nb_inst);
    }

    bool intrusive = flags & (uint32_t) type_flags::intrusive_ptr;

    self->offset = offset;

    nb_inst_state s {};
    s.direct = direct;
    s.internal = 0;
    s.state = nb_inst_state::state_uninitialized;
    s.destruct = 0;
    s.cpp_delete = 0;
    s.clear_keep_alive = 0;
    s.intrusive = intrusive;
    s.pad = 0;
    s.unused = 0;
    nb_inst_state_write(self, s);

    // Make the object compatible with nb_try_inc_ref (free-threaded builds only)
    nb_enable_try_inc_ref((PyObject *) self);

    return (PyObject *) self;
}

/// Register the object constructed by 'inst_new_ext()' in the internal data structures
static void inst_register(nb_internals *p, PyObject *inst,
                          void *value) noexcept {
    nb_shard &shard = p->shard(value);
    lock_shard guard(shard);

    // Update hash table that maps from C++ to Python instance
    auto [it, success] = shard.inst_c2p.try_emplace(value, inst);

    if (NB_UNLIKELY(!success)) {
        void *entry = it->second;

        // Potentially convert the map value into linked list format
        if (!nb_is_seq(entry)) {
            nb_inst_seq *first = (nb_inst_seq *) PyMem_Malloc(sizeof(nb_inst_seq));
            check(first, "nanobind::detail::inst_new_ext(): list element "
                         "allocation failed!");
            first->inst = (PyObject *) entry;
            first->next = nullptr;
            entry = it.value() = nb_mark_seq(first);
        }

        nb_inst_seq *seq = nb_get_seq(entry);
        while (true) {
            // The following should never happen
            check(inst != seq->inst, "nanobind::detail::inst_new_ext(): duplicate instance!");

            if (!seq->next)
                break;
            seq = seq->next;
        }

        nb_inst_seq *next = (nb_inst_seq *) PyMem_Malloc(sizeof(nb_inst_seq));
        check(next,
              "nanobind::detail::inst_new_ext(): list element allocation failed!");

        next->inst = (PyObject *) inst;
        next->next = nullptr;
        seq->next = next;
    }
}


// Return the instance pool associated with type `td` or allocate it.
nb_inst_pool *nb_pool_ensure(type_data *td) noexcept {
#if !defined(NB_FREE_THREADED)
    // In GIL-protected Python, global pool data structure is reachable via `td`
    nb_inst_pool *pool = &td->pool;
#else
    // In FT builds, the pool is per thread and stored in a packed pointer
    // array that may need to be allocated or expanded.
    nb_thread_state *ts = nb_thread_state_get(td->internals);
    uint32_t idx = td->pool_index;

    if (NB_UNLIKELY(idx >= ts->pools_size)) {
        uint32_t old_size = ts->pools_size,
                 new_size = idx + 1;

        nb_inst_pool *np = (nb_inst_pool *) PyMem_Realloc(
            ts->pools, new_size * sizeof(nb_inst_pool));
        check(np, "nb_pool_ensure(): out of memory!");

        memset(np + old_size, 0, (new_size - old_size) * sizeof(nb_inst_pool));
        ts->pools = np;
        ts->pools_size = new_size;
    }

    nb_inst_pool *pool = &ts->pools[idx];
#endif

    // Allocate the pool if it does not yet exist
    if (NB_UNLIKELY(!pool->slots)) {
        pool->capacity = td->pool_capacity;
        pool->count = 0;
        pool->slots = (nb_inst **) PyMem_Malloc(pool->capacity * sizeof(nb_inst *));
        check(pool->slots, "nb_pool_ensure(): out of memory!");
    }

    return pool;
}

// Unmap all parked instances of a pool from 'inst_c2p' and release them. When
// called after interpreter shutdown it is no longer safe to release memory via
// PyMem_Free/PyObject_Free and can_free=false must be specified.
void nb_pool_drain(nb_inst_pool *pool, bool can_free) noexcept {
    if (!pool || !pool->slots)
        return;

    // Check the type-level properties of the pooled instances once
    bool gc = false;
    nb_internals *pi = nullptr;
    if (pool->count) {
        PyTypeObject *tp = Py_TYPE((PyObject *) pool->slots[0]);
        pi = nb_type_data(tp)->internals;
        if (can_free)
            gc = nb_type_has_gc(tp, nb_type_data(tp)->flags);
    }

    for (uint32_t i = 0; i < pool->count; ++i) {
        nb_inst *inst = pool->slots[i];
        void *p = inst_ptr(inst);

        nb_shard &shard = pi->shard(p);
        lock_shard guard(shard);

        // Unmap 'inst' from inst_c2p
        nb_ptr_map &inst_c2p = shard.inst_c2p;
        nb_ptr_map::iterator it = inst_c2p.find(p);
        if (NB_LIKELY(it != inst_c2p.end())) {
            void *entry = it->second;
            if (NB_LIKELY(entry == inst)) {
                inst_c2p.erase_fast(it);
            } else if (nb_is_seq(entry)) {
                nb_inst_seq *seq = nb_get_seq(entry), *pred = nullptr;
                do {
                    if ((nb_inst *) seq->inst == inst) {
                        if (pred)
                            pred->next = seq->next;
                        else if (seq->next)
                            it.value() = nb_mark_seq(seq->next);
                        else
                            inst_c2p.erase_fast(it);
                        if (can_free)
                            PyMem_Free(seq);
                        break;
                    }
                    pred = seq;
                    seq = seq->next;
                } while (seq);
            }
        }

        if (can_free) {
            if (NB_UNLIKELY(gc))
                PyObject_GC_Del(inst);
            else
                PyObject_Free(inst);
        }
    }

    if (can_free)
        PyMem_Free(pool->slots);
    pool->slots = nullptr;
    pool->count = 0;
    pool->capacity = 0;
}

static void inst_dealloc(PyObject *self) {
    PyTypeObject *tp = Py_TYPE(self);
    const type_data *t = nb_type_data(tp);
    nb_inst *inst = (nb_inst *) self;
    uint32_t flags = t->flags;

    bool gc = nb_type_has_gc(tp, flags);

    // For GC types, untrack the instance and clear its dict and weak references
    if (NB_UNLIKELY(gc)) {
        PyObject_GC_UnTrack(self);

        if (flags & (uint32_t) type_flags::has_dynamic_attr) {
            PyObject **dict = nb_dict_ptr(self, tp);
            if (dict)
                Py_CLEAR(*dict);
        }

        // Clear weak references if needed
        if (flags & (uint32_t) type_flags::is_weak_referenceable) {
            PyObject **weaklist = nb_weaklist_ptr(self, tp);
            if (weaklist && *weaklist)
#if defined(PYPY_VERSION)
                Py_CLEAR(*weaklist);
#else
                PyObject_ClearWeakRefs(self);
#endif
        }
    }

    // Fast path: run the C++ destructor, then put the mapped object in the pool
    // for reuse. The guard below admits only "clean" instances:
    //
    //   - 'internal': the payload is co-located and owned by nanobind.
    //   - '!clear_keep_alive': doesn't need more complex teardown below.
    //   - 'state != relinquished': exclude unusual ownership semantics.
    if (NB_LIKELY((flags & (uint32_t) type_flags::pooled) &&
                  inst->state.internal && !inst->state.clear_keep_alive &&
                  inst->state.state != nb_inst_state::state_relinquished)) {
        if (inst->state.destruct && (flags & (uint32_t) type_flags::has_destruct))
            t->destruct(inst_ptr(inst));

        // Look up the pool or create it
        nb_inst_pool *pool = nb_pool_lookup((type_data *) t);
        if (NB_UNLIKELY(!pool || !pool->slots))
            pool = nb_pool_ensure((type_data *) t);

        if (NB_LIKELY(pool->count < pool->capacity)) {
            // There is space in the pool. Stash the object and release its
            // reference to the type object.
            pool->slots[pool->count++] = inst;
            NB_DECREF_TYPE(tp);
            return;
        }

        // The pool is full. Release without rerunning the destructor
        inst->state.destruct = 0;
    }

    void *p = inst_ptr(inst);

    if (inst->state.destruct) {
        check(flags & (uint32_t) type_flags::is_destructible,
              "nanobind::detail::inst_dealloc(\"%s\"): attempted to call "
              "the destructor of a non-destructible type!", t->name);
        if (flags & (uint32_t) type_flags::has_destruct)
            t->destruct(p);
    }

    if (inst->state.cpp_delete) {
        if (NB_LIKELY(t->align <= (uint32_t) __STDCPP_DEFAULT_NEW_ALIGNMENT__))
            operator delete(p);
        else
            operator delete(p, std::align_val_t(t->align));
    }

    nb_weakref_seq *wr_seq = nullptr;

    {
        // Enter critical section of shard
        nb_shard &shard = t->internals->shard(p);
        lock_shard guard(shard);

        if (NB_UNLIKELY(inst->state.clear_keep_alive)) {
            nb_ptr_map &keep_alive = shard.keep_alive;
            nb_ptr_map::iterator it = keep_alive.find(self);
            check(it != keep_alive.end(),
                  "nanobind::detail::inst_dealloc(\"%s\"): inconsistent "
                  "keep_alive information", t->name);

            wr_seq = (nb_weakref_seq *) it->second;
            keep_alive.erase_fast(it);
        }

        // Unmap 'inst' from inst_c2p
        nb_ptr_map &inst_c2p = shard.inst_c2p;
        nb_ptr_map::iterator it = inst_c2p.find(p);

        bool found = false;
        if (NB_LIKELY(it != inst_c2p.end())) {
            void *entry = it->second;
            if (NB_LIKELY(entry == inst)) {
                // Fast path: a direct 'p -> inst' mapping.
                inst_c2p.erase_fast(it);
                found = true;
            } else if (nb_is_seq(entry)) {
                // Multiple instances alias this address. Unlink the right one.
                nb_inst_seq *seq = nb_get_seq(entry), *pred = nullptr;
                do {
                    if ((nb_inst *) seq->inst == inst) {
                        if (pred)
                            pred->next = seq->next;
                        else if (seq->next)
                            it.value() = nb_mark_seq(seq->next);
                        else
                            inst_c2p.erase_fast(it);
                        PyMem_Free(seq);
                        found = true;
                        break;
                    }
                    pred = seq;
                    seq = seq->next;
                } while (seq);
            }
        }

        check(found,
              "nanobind::detail::inst_dealloc(): attempted to remove an unknown "
              "instance (%p) of type \"%s\"!",
              p, nb_type_data(Py_TYPE((PyObject *) inst))->name);
    }

    while (wr_seq) {
        nb_weakref_seq *c = wr_seq;
        wr_seq = c->next;

        if (c->callback)
            c->callback(c->payload);
        else
            Py_DECREF((PyObject *) c->payload);

        PyMem_Free(c);
    }

    if (NB_UNLIKELY(gc))
        PyObject_GC_Del(self);
    else
        PyObject_Free(self);

    // Release the type reference acquired at allocation. On free-threaded
    // builds, nanobind types are immortal but Python subclasses are not.
#if defined(Py_GIL_DISABLED)
    if (flags & (uint32_t) type_flags_internal::is_python_type)
        Py_DECREF(tp);
#else
    Py_DECREF(tp);
#endif
}


type_data *nb_type_c2p(nb_internals *internals_,
                       const std::type_info *type) {
#if defined(NB_FREE_THREADED)
    nb_type_map_fast &type_c2p_fast =
        nb_thread_state_get(internals_)->type_c2p_fast;
#else
    nb_type_map_fast &type_c2p_fast = internals_->type_c2p_fast;
#endif

    nb_type_map_fast::iterator it_fast = type_c2p_fast.find((void *) type);
    if (it_fast != type_c2p_fast.end())
        return (type_data *) it_fast->second;

    lock_internals guard(internals_);
    nb_type_map_slow &type_c2p_slow = internals_->type_c2p_slow;
    nb_type_map_slow::iterator it_slow = type_c2p_slow.find(type);
    if (it_slow != type_c2p_slow.end()) {
        type_data *d = it_slow->second;

#if !defined(NB_FREE_THREADED)
        // Maintain a linked list to clean up 'type_c2p_fast' when the type
        // expires (see nb_type_unregister). In free-threaded mode, we leak
        // these entries until the thread destructs.
        nb_alias_chain *chain =
            (nb_alias_chain *) PyMem_Malloc(sizeof(nb_alias_chain));
        check(chain, "Could not allocate nb_alias_chain entry!");
        chain->next = d->alias_chain;
        chain->value = type;
        d->alias_chain = chain;
#endif

        type_c2p_fast[(void *) type] = d;
        return d;
    }

    return nullptr;
}

void nb_type_unregister(type_data *t) noexcept {
    nb_internals *internals_ = t->internals;
    nb_type_map_slow &type_c2p_slow = internals_->type_c2p_slow;

    lock_internals guard(internals_);
    size_t n_del_slow = type_c2p_slow.erase(t->type);

#if defined(NB_FREE_THREADED)
    // In free-threaded mode, stale type information remains in the
    // 'type_c2p_fast' TLS. This data structure is eventually deallocated
    // when the thread terminates.
    //
    // In principle, this is dangerous because the user could delete a type
    // binding from a module at runtime, causing the associated
    // Python type object to be freed. If a function then attempts to return
    // a value with such a de-registered type, nanobind should raise an
    // exception, which requires knowing that the entry in 'type_c2p_fast'
    // has become invalid in the meantime.
    //
    // Right now, this problem is avoided because we immortalize type objects in
    // ``nb_type_new()`` and ``enum_create()``. However, we may not always
    // want to stick with immortalization, which is just a workaround.
    //
    // In the future, a global version counter modified with acquire/release
    // semantics (see https://github.com/wjakob/nanobind/pull/695#discussion_r1761600010)
    // might prove to be a similarly efficient but more general solution.
    bool fail = n_del_slow != 1;
#else
    nb_type_map_fast &type_c2p_fast = internals_->type_c2p_fast;
    size_t n_del_fast = type_c2p_fast.erase((void *) t->type);

    bool fail = n_del_fast != 1 || n_del_slow != 1;
    if (!fail) {
        nb_alias_chain *cur = t->alias_chain;
        while (cur) {
            nb_alias_chain *next = cur->next;
            n_del_fast = type_c2p_fast.erase((void *) cur->value);
            if (n_del_fast != 1) {
                fail = true;
                break;
            }
            PyMem_Free(cur);
            cur = next;
        }
    }
#endif

    check(!fail,
          "nanobind::detail::nb_type_unregister(\"%s\"): could not "
          "find type!", t->name);
}

static void nb_type_dealloc(PyObject *o) {
    type_data *t = nb_type_data((PyTypeObject *) o);

#if !defined(NB_FREE_THREADED)
    // Drain the per-type instance pool before unregistering. (In free-threaded
    // builds, bound types are immortalized, so this is never reached; their
    // per-thread pools are drained at thread exit instead.)
    if (t->flags & (uint32_t) type_flags::pooled)
        nb_pool_drain(&t->pool, /* can_free = */ true);
#endif

    if (t->type && (t->flags & (uint32_t) type_flags_internal::is_python_type) == 0)
        nb_type_unregister(t);

    if (t->flags & (uint32_t) type_flags_internal::has_implicit_conversions) {
        PyMem_Free(t->implicit.cpp);
        PyMem_Free(t->implicit.py);
    }

    bool initialized = t->name != nullptr;
    nb_internals *p = t->internals;
    free((char *) t->name);
    PyMem_Free(t->supplement);
    nb_trampoline_free(t);

    PyTypeObject *meta = Py_TYPE(o);
    NB_TYPE_SLOT(PyType_Type, tp_dealloc)(o);
    NB_DECREF_TYPE(meta);

    if (initialized)
        internals_dec_ref(p);
}

/// Initialize the type record of a Python subclass of a nanobind type
static NB_NOINLINE int nb_type_init_py(PyTypeObject *self) {
#if defined(Py_LIMITED_API)
    PyObject *bases = (PyObject *) PyType_GetSlot(self, Py_tp_bases);
    PyTypeObject *base = (PyTypeObject *) PyType_GetSlot(self, Py_tp_base);
#else
    PyObject *bases = self->tp_bases;
    PyTypeObject *base = self->tp_base;
#endif

    // Permit Python mixins alongside exactly one nanobind base. A nanobind
    // base's metaclass is an instance of the same meta-metaclass 'nb_meta'.
    PyTypeObject *nb_meta = Py_TYPE((PyObject *) Py_TYPE((PyObject *) self)),
                 *nb_base = nullptr;
    Py_ssize_t n_bases = bases ? NB_TUPLE_GET_SIZE(bases) : 0;
    for (Py_ssize_t i = 0; i < n_bases; ++i) {
        PyTypeObject *b = (PyTypeObject *) NB_TUPLE_GET_ITEM(bases, i);
        if (Py_TYPE((PyObject *) Py_TYPE((PyObject *) b)) != nb_meta)
            continue;
        if (nb_base) {
            PyErr_SetString(PyExc_TypeError,
                            "nanobind: multiple nanobind bases are not supported!");
            return -1;
        }
        nb_base = b;
    }

    if (!nb_base) {
        PyErr_SetString(PyExc_TypeError, "nanobind: the 'nb_type' metaclass "
                                         "requires a nanobind base type!");
        return -1;
    }

    // Python selects 'tp_base', which provides the instance layout
    if (base != nb_base) {
        PyErr_SetString(PyExc_TypeError,
                        "nanobind: instance layout must come from the nanobind base!");
        return -1;
    }

    // The base may itself still await initialization
    if (!nb_type_ensure(base))
        return -1;

    type_data *t_b = nb_type_data(base), *t = nb_type_data(self);
    if (t_b->flags & (uint32_t) type_flags::is_final) {
        PyErr_Format(PyExc_TypeError, "The type '%s' prohibits subclassing!",
                     t_b->name);
        return -1;
    }

    PyObject *name_o = nb_type_name((PyObject *) self);
    char *name = strdup_check(PyUnicode_AsUTF8AndSize(name_o, nullptr));
    Py_DECREF(name_o);

    // Assemble the record locally, it is published as a whole below
    type_data tmp = *t_b;
    tmp.internals = nullptr;
    tmp.name = name;
    tmp.type_py = self;

    // Subclasses resolve their own overrides
    tmp.trampoline_table_pub = nullptr;
    tmp.trampoline_allocs = nullptr;

    tmp.flags |=  (uint32_t) type_flags_internal::is_python_type;
    tmp.flags &= ~(uint32_t) type_flags_internal::has_implicit_conversions;

    // A Python subclass is always a GC heap type
    tmp.flags |= (uint32_t) type_flags_internal::has_gc;

    // Sublclasses do not inherit the pooling feature as a consequence
    tmp.flags &= ~(uint32_t) type_flags::pooled;
    tmp.pool_capacity = 0;
#if defined(NB_FREE_THREADED)
    tmp.pool_index = 0;
#else
    tmp.pool = nb_inst_pool{};
#endif

    tmp.implicit.cpp = nullptr;
    tmp.implicit.py = nullptr;
    tmp.alias_chain = nullptr;
    tmp.supplement = nullptr;

#if defined(Py_LIMITED_API)
    tmp.vectorcall = nullptr;
#else
    self->tp_vectorcall = nullptr;
#endif

    nb_internals *p = t_b->internals;
    lock_internals guard(p);

    // Another thread may have initialized the record in the meantime
    if (t->internals) {
        free(name);
        return 0;
    }

    *t = tmp;
    internals_inc_ref(p);
    internals_store(t, p);

    return 0;
}

/// Called when a C++ type is extended from within Python
static int nb_type_init(PyObject *self, PyObject *args, PyObject *kwds) {
    int rv = NB_TYPE_SLOT(PyType_Type, tp_init)(self, args, kwds);
    if (rv == 0 && !nb_type_ensure((PyTypeObject *) self))
        rv = -1;
    return rv;
}

#if defined(Py_LIMITED_API)
/// Dictionary lookup returning a new reference, does not raise.
static NB_INLINE PyObject *dict_lookup(PyObject *d, PyObject *key) noexcept {
#  if NB_PYTHON_VERSION >= 0x030D0000
    PyObject *value = nullptr;
    PyDict_GetItemRef(d, key, &value);
    return value;
#  else
    return Py_XNewRef(PyDict_GetItem(d, key));
#  endif
}

/// Substitute for _PyType_LookupRef() in limited-API builds: walk the MRO of
/// the type 't' by hand and return the first match as a new reference
static PyObject *type_lookup_portable(nb_internals *p, PyObject *t,
                                      PyObject *key) noexcept {
    PyObject *mro = PyObject_GetAttr(t, NB_INTERNED(p, __mro__)),
             *result = nullptr;

    if (!mro || !PyTuple_Check(mro)) {
        Py_XDECREF(mro);
        PyErr_Clear();
        return nullptr;
    }

    Py_ssize_t n = PyTuple_Size(mro);
    for (Py_ssize_t i = 0; i < n && !result; ++i) {
        PyObject *base = PyTuple_GetItem(mro, i); // borrowed
        if (!base)
            break;

        // Heap types hand out their namespace directly, while static types
        // are only reachable through a read-only proxy
        PyObject *dict = type_dict(base);
        if (NB_LIKELY(dict != nullptr)) {
            result = dict_lookup(dict, key);
        } else {
            dict = PyObject_GetAttr(base, NB_INTERNED(p, __dict__));
            if (!dict)
                break;
            result = PyObject_GetItem(dict, key);
        }

        Py_DECREF(dict);
    }

    PyErr_Clear();
    Py_DECREF(mro);
    return result;
}
#endif

PyObject *type_lookup(nb_internals *p, PyObject *t, PyObject *key) noexcept {
#if !defined(Py_LIMITED_API)
    (void) p;
    PyTypeObject *tp = (PyTypeObject *) t;
#  if PY_VERSION_HEX >= 0x030D0000 && !defined(PYPY_VERSION)
    return _PyType_LookupRef(tp, key);
#  else
    return Py_XNewRef(_PyType_Lookup(tp, key));
#  endif
#else
    return type_lookup_portable(p, t, key);
#endif
}

PyObject *type_dict(PyObject *t) noexcept {
#if !defined(Py_LIMITED_API) && PY_VERSION_HEX >= 0x030C0000 && \
    !defined(PYPY_VERSION)
    return PyType_GetDict((PyTypeObject *) t);
#elif !defined(Py_LIMITED_API)
    return Py_XNewRef(((PyTypeObject *) t)->tp_dict);
#else
    // PyType_GetDict() is not part of the limited API. Heap types store their
    // namespace where 'type' declares the instance dictionary of its
    // instances, which PyObject_GenericGetDict() reaches. Static types keep it
    // elsewhere (in the interpreter state since Python 3.12) and are out of
    // reach.
    if (!(PyType_GetFlags((PyTypeObject *) t) & Py_TPFLAGS_HEAPTYPE))
        return nullptr;

    PyObject *dict = PyObject_GenericGetDict(t, nullptr);
    if (!dict)
        PyErr_Clear();
    return dict;
#endif
}

PyObject *inst_dict(PyObject *o) noexcept {
    // Do some checks before calling PyObject_GenericGetDict() since raising and
    // then clearning exceptions is costly.
#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)
    PyTypeObject *tp = Py_TYPE(o);
    bool has_dict = tp->tp_dictoffset != 0;

#if PY_VERSION_HEX >= 0x030B0000 && PY_VERSION_HEX < 0x030C0000
    // Types with a managed dictionary report the offset -1 from Python 3.12+
    has_dict |= (tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) != 0;
#endif

    if (!has_dict)
        return nullptr;
#endif

    PyObject *dict = PyObject_GenericGetDict(o, nullptr);
    if (NB_UNLIKELY(!dict))
        PyErr_Clear();

    return dict;
}

bool type_freeze(PyObject *t) {
    // While Python 3.14 already has PyType_Freeze(), we ignore it there because
    // of this performance bug: https://github.com/python/cpython/issues/143361
#if NB_PYTHON_VERSION >= 0x030F0000 && !defined(PYPY_VERSION)
    if (PyType_Freeze((PyTypeObject *) t))
        raise_python_error();
    return true;
#else
    (void) t;
    return false;
#endif
}

/// Special case to handle 'Class.property = value' assignments
int nb_type_setattro(PyObject* obj, PyObject* name, PyObject* value) {
    if (NB_UNLIKELY(!nb_type_ensure((PyTypeObject *) obj)))
        return -1;

    nb_internals *int_p = nb_type_data((PyTypeObject *) obj)->internals;

    /* Fetch the raw MRO entry for 'name'. The lookup must not engage the
       descriptor protocol so that a static property is seen as-is below. */
    PyObject *cur = type_lookup(int_p, obj, name);

    if (cur) {
        PyTypeObject *tp = int_p->nb_static_property.load_acquire();
        // For type.static_prop = value and value==NUL L(deletion), call the setter.
        // For type.static_prop = another_static_prop, replace the descriptor.
        if (Py_TYPE(cur) == tp && (!value || Py_TYPE(value) != tp)) {
            int rv = int_p->nb_static_property_descr_set(cur, obj, value);
            Py_DECREF(cur);
            return rv;
        }
        Py_DECREF(cur);

        const char *cname = PyUnicode_AsUTF8AndSize(name, nullptr);
        if (!cname) {
            PyErr_Clear(); // probably a non-string attribute name
        } else if (cname[0] == '@') {
            // Prevent type attributes starting with an `@` sign from being
            // rebound or deleted. This is useful to safely stash owning
            // references. The ``nb::enum_<>`` class, e.g., uses this to ensure
            // indirect ownership of a borrowed reference in the supplemental
            // type data.
            PyErr_Format(PyExc_AttributeError,
                         "internal nanobind attribute '%s' cannot be "
                         "reassigned or deleted.", cname);
            return -1;
        }
    }

    int rv = NB_TYPE_SLOT(PyType_Type, tp_setattro)(obj, name, value);

    // The assignment may have (un)shadowed a bound method; drop the
    // trampoline override tables of this type and its subclasses so that
    // virtual method calls observe the modification.
    if (rv == 0)
        nb_trampoline_invalidate(obj);

    return rv;
}

#if NB_TYPE_FROM_METACLASS_IMPL || NB_TYPE_GET_SLOT_IMPL

struct nb_slot {
#if NB_TYPE_GET_SLOT_IMPL
    uint8_t indirect_1;
    uint8_t indirect_2;
#endif
    uint8_t direct;
};

template <size_t I1, size_t I2, size_t Offset1, size_t Offset2> nb_slot constexpr Ei() {
    // Compile-time check to ensure that indices and alignment match our expectation
    static_assert(I1 == I2 && (Offset1 % sizeof(void *)) == 0 && (Offset2 % sizeof(void *)) == 0,
                  "nb_slot construction: internal error");

#if NB_TYPE_GET_SLOT_IMPL
    size_t o = 0;
    switch (Offset1) {
        case offsetof(PyHeapTypeObject, as_async):    o = offsetof(PyTypeObject, tp_as_async); break;
        case offsetof(PyHeapTypeObject, as_number):   o = offsetof(PyTypeObject, tp_as_number); break;
        case offsetof(PyHeapTypeObject, as_mapping):  o = offsetof(PyTypeObject, tp_as_mapping); break;
        case offsetof(PyHeapTypeObject, as_sequence): o = offsetof(PyTypeObject, tp_as_sequence); break;
        case offsetof(PyHeapTypeObject, as_buffer):   o = offsetof(PyTypeObject, tp_as_buffer); break;
        default: break;
    }

    return {
        (uint8_t) (o / sizeof(void *)),
        (uint8_t) ((Offset2 - Offset1) / sizeof(void *)),
        (uint8_t) (Offset2 / sizeof(void *)),
    };
#else
    return { (uint8_t) (Offset2 / sizeof(void *)) };
#endif
}

// Precomputed mapping from type slot ID to an entry in the data structure
#define E(i1, p1, p2, name)                            \
    Ei<i1, Py_##p2##_##name,                           \
       offsetof(PyHeapTypeObject, p1),                 \
       offsetof(PyHeapTypeObject, p1.p2##_##name)>()

static constexpr nb_slot type_slots[] {
    E(1,  as_buffer, bf, getbuffer),
    E(2,  as_buffer, bf, releasebuffer),
    E(3,  as_mapping, mp, ass_subscript),
    E(4,  as_mapping, mp, length),
    E(5,  as_mapping, mp, subscript),
    E(6,  as_number, nb, absolute),
    E(7,  as_number, nb, add),
    E(8,  as_number, nb, and),
    E(9,  as_number, nb, bool),
    E(10, as_number, nb, divmod),
    E(11, as_number, nb, float),
    E(12, as_number, nb, floor_divide),
    E(13, as_number, nb, index),
    E(14, as_number, nb, inplace_add),
    E(15, as_number, nb, inplace_and),
    E(16, as_number, nb, inplace_floor_divide),
    E(17, as_number, nb, inplace_lshift),
    E(18, as_number, nb, inplace_multiply),
    E(19, as_number, nb, inplace_or),
    E(20, as_number, nb, inplace_power),
    E(21, as_number, nb, inplace_remainder),
    E(22, as_number, nb, inplace_rshift),
    E(23, as_number, nb, inplace_subtract),
    E(24, as_number, nb, inplace_true_divide),
    E(25, as_number, nb, inplace_xor),
    E(26, as_number, nb, int),
    E(27, as_number, nb, invert),
    E(28, as_number, nb, lshift),
    E(29, as_number, nb, multiply),
    E(30, as_number, nb, negative),
    E(31, as_number, nb, or),
    E(32, as_number, nb, positive),
    E(33, as_number, nb, power),
    E(34, as_number, nb, remainder),
    E(35, as_number, nb, rshift),
    E(36, as_number, nb, subtract),
    E(37, as_number, nb, true_divide),
    E(38, as_number, nb, xor),
    E(39, as_sequence, sq, ass_item),
    E(40, as_sequence, sq, concat),
    E(41, as_sequence, sq, contains),
    E(42, as_sequence, sq, inplace_concat),
    E(43, as_sequence, sq, inplace_repeat),
    E(44, as_sequence, sq, item),
    E(45, as_sequence, sq, length),
    E(46, as_sequence, sq, repeat),
    E(47, ht_type, tp, alloc),
    E(48, ht_type, tp, base),
    E(49, ht_type, tp, bases),
    E(50, ht_type, tp, call),
    E(51, ht_type, tp, clear),
    E(52, ht_type, tp, dealloc),
    E(53, ht_type, tp, del),
    E(54, ht_type, tp, descr_get),
    E(55, ht_type, tp, descr_set),
    E(56, ht_type, tp, doc),
    E(57, ht_type, tp, getattr),
    E(58, ht_type, tp, getattro),
    E(59, ht_type, tp, hash),
    E(60, ht_type, tp, init),
    E(61, ht_type, tp, is_gc),
    E(62, ht_type, tp, iter),
    E(63, ht_type, tp, iternext),
    E(64, ht_type, tp, methods),
    E(65, ht_type, tp, new),
    E(66, ht_type, tp, repr),
    E(67, ht_type, tp, richcompare),
    E(68, ht_type, tp, setattr),
    E(69, ht_type, tp, setattro),
    E(70, ht_type, tp, str),
    E(71, ht_type, tp, traverse),
    E(72, ht_type, tp, members),
    E(73, ht_type, tp, getset),
    E(74, ht_type, tp, free),
    E(75, as_number, nb, matrix_multiply),
    E(76, as_number, nb, inplace_matrix_multiply),
    E(77, as_async, am, await),
    E(78, as_async, am, aiter),
    E(79, as_async, am, anext),
    E(80, ht_type, tp, finalize),
#if !defined(PYPY_VERSION)
    E(81, as_async, am, send),
#endif
};

#if NB_TYPE_GET_SLOT_IMPL
void *type_get_slot(PyTypeObject *t, int slot_id) {
    nb_slot slot = type_slots[slot_id - 1];

    if (PyType_HasFeature(t, Py_TPFLAGS_HEAPTYPE)) {
        return ((void **) t)[slot.direct];
    } else {
        if (slot.indirect_1) {
            void **group = ((void ***) t)[slot.indirect_1];
            return group ? group[slot.indirect_2] : nullptr;
        } else {
            return ((void **) t)[slot.indirect_2];
        }
    }
}
#endif

#endif

static PyObject *nb_type_from_metaclass(PyTypeObject *meta, PyObject *mod,
                                        PyType_Spec *spec) {
#if NB_TYPE_FROM_METACLASS_IMPL == 0
    // Life is good, PyType_FromMetaclass() is available
    return PyType_FromMetaclass(meta, mod, spec, nullptr);
#else
    // The fallback code below emulates PyType_FromMetaclass() on Python prior
    // to version 3.12. It requires access to CPython-internal structures, which
    // is why nanobind can only target the stable ABI on version 3.12+.

    const char *name = strrchr(spec->name, '.');
    PyObject *modname_o = nullptr;
    if (name) {
        modname_o = PyUnicode_FromStringAndSize(spec->name, name - spec->name);
        if (!modname_o)
            return nullptr;
        name++;
    } else {
        name = spec->name;
    }

    PyObject *name_o = PyUnicode_InternFromString(name);
    if (!name_o) {
        Py_XDECREF(modname_o);
        return nullptr;
    }

    const char *name_cstr = PyUnicode_AsUTF8AndSize(name_o, nullptr);
    if (!name_cstr) {
        Py_XDECREF(modname_o);
        Py_DECREF(name_o);
        return nullptr;
    }

    PyHeapTypeObject *ht = (PyHeapTypeObject *) PyType_GenericAlloc(meta, 0);
    if (!ht) {
        Py_XDECREF(modname_o);
        Py_DECREF(name_o);
        return nullptr;
    }

    ht->ht_name = name_o;
    ht->ht_qualname = Py_NewRef(name_o);
    ht->ht_module = Py_XNewRef(mod);

    PyTypeObject *tp = &ht->ht_type;
    tp->tp_name = name_cstr;
    tp->tp_basicsize = spec->basicsize;
    tp->tp_itemsize = spec->itemsize;
    tp->tp_flags = spec->flags | Py_TPFLAGS_HEAPTYPE;
    tp->tp_as_async = &ht->as_async;
    tp->tp_as_number = &ht->as_number;
    tp->tp_as_sequence = &ht->as_sequence;
    tp->tp_as_mapping = &ht->as_mapping;
    tp->tp_as_buffer = &ht->as_buffer;

    PyType_Slot *ts = spec->slots;
    bool fail = false;
    while (true) {
        int slot = ts->slot;

        if (slot == 0) {
            break;
        } else if ((size_t) slot * sizeof(nb_slot) <= sizeof(type_slots)) {
            *(((void **) ht) + type_slots[slot - 1].direct) = ts->pfunc;
        } else {
            PyErr_Format(PyExc_RuntimeError,
                         "nb_type_from_metaclass(): unhandled slot %i", slot);
            fail = true;
            break;
        }
        ts++;
    }

    // Bring type object into a safe state (before error handling)
    const PyMemberDef *members = tp->tp_members;
    const char *doc = tp->tp_doc;
    tp->tp_members = nullptr;
    tp->tp_doc = nullptr;
    Py_XINCREF(tp->tp_base);

    if (doc && !fail) {
        size_t size = strlen(doc) + 1;
        /// This code path is only used for Python 3.12, where
        /// PyObject_Malloc is the right allocation routine for tp_doc
        char *target = (char *) PyObject_Malloc(size);
        if (!target) {
            PyErr_NoMemory();
            fail = true;
        } else {
            memcpy(target, doc, size);
            tp->tp_doc = target;
        }
    }

    if (members && !fail) {
        while (members->name) {
            if (members->type == Py_T_PYSSIZET && members->flags == Py_READONLY) {
                if (strcmp(members->name, "__dictoffset__") == 0)
                    tp->tp_dictoffset = members->offset;
                else if (strcmp(members->name, "__weaklistoffset__") == 0)
                    tp->tp_weaklistoffset = members->offset;
                else if (strcmp(members->name, "__vectorcalloffset__") == 0)
                    tp->tp_vectorcall_offset = members->offset;
                else
                    fail = true;
            } else {
                fail = true;
            }

            if (fail) {
                PyErr_Format(
                    PyExc_RuntimeError,
                    "nb_type_from_metaclass(): unhandled tp_members entry!");
                break;
            }

            members++;
        }
    }

    if (!fail) {
        tp->tp_dict = PyDict_New();
        // Can't use the interned '__module__' string here: this code runs
        // during the creation of the internals, before init_pyobjects()
        if (!tp->tp_dict ||
            (modname_o &&
             PyDict_SetItemString(tp->tp_dict, "__module__", modname_o) < 0))
            fail = true;
    }
    Py_XDECREF(modname_o);

#if defined(PYPY_VERSION)
    // PyPy may consider small bound classes to be layout-compatible with
    // 'object' and then select a preceding Python mixin as 'tp_base'. A
    // private slot gives root bound classes a distinct layout. PyPy omits
    // the instance dictionary unless '__dict__' is listed as well.
    if (!fail && (spec->flags & Py_TPFLAGS_BASETYPE) && !tp->tp_base) {
        PyObject *slots = Py_BuildValue("(ss)", "__nb_layout", "__dict__");
        if (!slots ||
            PyDict_SetItemString(tp->tp_dict, "__slots__", slots) < 0)
            fail = true;
        Py_XDECREF(slots);
    }
#endif

    if (fail || PyType_Ready(tp) != 0) {
        Py_DECREF(tp);
        return nullptr;
    }

    return (PyObject *) tp;
#endif
}

extern int nb_type_setattro(PyObject* obj, PyObject* name, PyObject* value);

// Fallback path for nb_type_vectorcall (caller did not provision space)
NB_NOINLINE static PyObject *
nb_type_vectorcall_fixup(nb_func *func, PyObject *self, PyObject *const *args_in,
                         Py_ssize_t nargs, PyObject *kwargs_in,
                         bool is_init) noexcept {
    const size_t buf_size = 5;
    PyObject **args, *buf[buf_size];
    bool alloc = false;

    size_t size = (size_t) nargs + 1;
    if (kwargs_in)
        size += (size_t) NB_TUPLE_GET_SIZE(kwargs_in);

    if (size < buf_size) {
        args = buf;
    } else {
        args = (PyObject **) PyMem_Malloc(size * sizeof(PyObject *));
        if (!args) {
            if (is_init)
                Py_DECREF(self);
            return PyErr_NoMemory();
        }
        alloc = true;
    }

    memcpy(args + 1, args_in, sizeof(PyObject *) * (size - 1));
    args[0] = self;

    size_t call_nargsf = (size_t) (nargs + 1);
    if (is_init)
        call_nargsf |= NB_VECTORCALL_TRUSTED_SELF;

    PyObject *rv =
        func->vectorcall((PyObject *) func, args, call_nargsf, kwargs_in);

    if (NB_UNLIKELY(alloc))
        PyMem_Free(args);

    if (is_init) {
        if (!rv) {
            Py_DECREF(self);
            return nullptr;
        }
#if !NB_IMMORTAL_SINGLETONS
        Py_DECREF(rv);
#endif
        return self;
    }
    return rv;
}

// Implements the vector call protocol directly on type objects to construct
// instances more efficiently.
static PyObject *nb_type_vectorcall(PyObject *self, PyObject *const *args_in,
                                    size_t nargsf,
                                    PyObject *kwargs_in) noexcept {
    PyTypeObject *tp = (PyTypeObject *) self;
    type_data *td = nb_type_data(tp);
    nb_func *func = (nb_func *) td->init;
    bool is_init = (td->flags & (uint32_t) type_flags_internal::has_new) == 0;
    Py_ssize_t nargs = NB_VECTORCALL_NARGS_EXT(nargsf);

    if (NB_UNLIKELY(!func)) {
        PyErr_Format(PyExc_TypeError, "%s: no constructor defined!", td->name);
        return nullptr;
    }

    if (NB_LIKELY(is_init)) {
        self = inst_new_int(tp, nullptr, nullptr);
        if (!self)
            return nullptr;
    } else if (nargs == 0 && !kwargs_in &&
               !(td->flags & (uint32_t) type_flags_internal::has_nullary_new)) {
        // When the bindings define a custom __new__ operator, nanobind always
        // provides a no-argument dummy __new__ constructor to handle unpickling
        // via __setstate__. This is an implementation detail that should not be
        // exposed. Therefore, only allow argument-less calls if there is an
        // actual __new__ overload with a compatible signature. This is
        // detected in nb_func.cpp based on whether any __init__ overload can
        // accept no arguments.

        return func->vectorcall((PyObject *) func, nullptr, 0, nullptr);
    }

    if (NB_UNLIKELY(!(nargsf & PY_VECTORCALL_ARGUMENTS_OFFSET)))
        return nb_type_vectorcall_fixup(func, self, args_in, nargs, kwargs_in,
                                        is_init);

    PyObject **args = (PyObject **) (args_in - 1);
    PyObject *temp = args[0];
    args[0] = self;

    size_t call_nargsf = (size_t) (nargs + 1);
    if (NB_LIKELY(is_init))
        call_nargsf |= NB_VECTORCALL_TRUSTED_SELF;

    PyObject *rv =
        func->vectorcall((PyObject *) func, args, call_nargsf, kwargs_in);

    args[0] = temp;

    if (NB_LIKELY(is_init)) {
        if (!rv) {
            Py_DECREF(self);
            return nullptr;
        }

        // __init__ constructor: 'rv' is None
#if !NB_IMMORTAL_SINGLETONS
        Py_DECREF(rv);
#endif
        return self;
    } else {
        // __new__ constructor
        return rv;
    }
}

PyTypeObject *nb_type_create_metaclass(nb_internals *p,
                                       PyTypeObject *nb_meta) noexcept {
#if PY_VERSION_HEX >= 0x030C0000
    int basicsize = -(int) sizeof(type_data),
        itemsize = 0;
#else
    int basicsize = (int) PyType_Type.tp_basicsize + (int) sizeof(type_data),
        itemsize = (int) PyType_Type.tp_itemsize;
#endif

    PyType_Slot slots[] = {
        { Py_tp_base, &PyType_Type },
        { Py_tp_dealloc, (void *) nb_type_dealloc },
        { Py_tp_setattro, (void *) nb_type_setattro },
        { Py_tp_init, (void *) nb_type_init },
        { 0, nullptr },
        { 0, nullptr },
        { 0, nullptr }
    };

    PyType_Spec spec = {
        /* .name = */ "nanobind.nb_type",
        /* .basicsize = */ basicsize,
        /* .itemsize = */ itemsize,
        /* .flags = */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
        /* .slots = */ slots
    };

#if defined(Py_LIMITED_API)
    PyMemberDef members[] = {
        { "__vectorcalloffset__", Py_T_PYSSIZET, 0, Py_READONLY, nullptr },
        { nullptr, 0, 0, 0, nullptr }
    };

    // Workaround because __vectorcalloffset__ does not support Py_RELATIVE_OFFSET
    members[0].offset = nb_type_data_offset + (Py_ssize_t) offsetof(type_data, vectorcall);

    if (NB_DYNAMIC_VERSION < 0x030E0000) {
        slots[4] = { Py_tp_members, (void *) members };
        slots[5] = { Py_tp_call, PyType_GetSlot(&PyType_Type, Py_tp_call) };
        spec.flags |= Py_TPFLAGS_HAVE_VECTORCALL;
    }
#endif

    PyTypeObject *tp = (PyTypeObject *) nb_type_from_metaclass(
        nb_meta, p->nb_module, &spec);

    if (tp) {
        make_immortal((PyObject *) tp);
        // Root the metaclass in the lifeline so it outlives every bound type.
        new_object(p, (PyObject *) tp);
    }

    return tp;
}

// This helper function extracts the function/class name from a custom signature attribute
NB_NOINLINE char *extract_name(const char *cmd, const char *prefix, const char *s) {
    (void) cmd;

    // Move to the last line
    const char *p = strrchr(s, '\n');
    p = p ? (p + 1) : s;

    // Check that the last line starts with the right prefix
    size_t prefix_len = strlen(prefix);
    check(strncmp(p, prefix, prefix_len) == 0,
          "%s(): last line of custom signature \"%s\" must start with \"%s\"!",
          cmd, s, prefix);
    p += prefix_len;

    // Find the opening parenthesis or bracket
    const char *p2 = strchr(p, '(');
    const char *p3 = strchr(p, '[');
    if (p2 == nullptr)
        p2 = p3;
    else if (p3 != nullptr)
        p2 = p2 < p3 ? p2 : p3;
    check(p2 != nullptr,
          "%s(): last line of custom signature \"%s\" must contain an opening "
          "parenthesis (\"(\") or bracket (\"[\")!", cmd, s);

    // A few sanity checks
    size_t len = strlen(p);
    char last = p[len ? (len - 1) : 0];

    check(last != ':' && last != ' ',
          "%s(): custom signature \"%s\" should not end with \":\" or \" \"!", cmd, s);
    check((p2 == p || (p[0] != ' ' && p2[-1] != ' ')),
          "%s(): custom signature \"%s\" contains leading/trailing space around name!", cmd, s);

    size_t size = (size_t) (p2 - p);
    char *result = (char *) malloc_check(size + 1);
    memcpy(result, p, size);
    result[size] = '\0';

    return result;
}

static PyMethodDef class_getitem_method[] = {
    { "__class_getitem__", Py_GenericAlias, METH_O | METH_CLASS, nullptr },
    { nullptr }
};

/// Called when a C++ type is bound via nb::class_<>
static PyObject *nb_type_new_impl(nb_internals *p, const type_data_init *t) {
    bool has_doc               = t->flags & (uint32_t) type_init_flags::has_doc,
         has_base              = t->flags & (uint32_t) type_init_flags::has_base,
         has_base_py           = t->flags & (uint32_t) type_init_flags::has_base_py,
         has_type_slots        = t->flags & (uint32_t) type_init_flags::has_type_slots,
         has_supplement        = t->flags & (uint32_t) type_init_flags::has_supplement,
         has_dynamic_attr      = t->flags & (uint32_t) type_flags::has_dynamic_attr,
         is_weak_referenceable = t->flags & (uint32_t) type_flags::is_weak_referenceable,
         is_generic            = t->flags & (uint32_t) type_flags::is_generic,
         intrusive_ptr         = t->flags & (uint32_t) type_flags::intrusive_ptr,
         has_shared_from_this  = t->flags & (uint32_t) type_flags::has_shared_from_this,
         has_signature         = t->flags & (uint32_t) type_flags::has_signature;

    const char *t_name = t->name;
    if (has_signature)
        t_name =
            extract_name("nanobind::detail::nb_type_new", "class ", t->name);

    str name = steal<str>(PyUnicode_InternFromString(t_name)),
        qualname = name;
    object modname;
    PyObject *mod = nullptr;

    // Update hash table that maps from std::type_info to Python type
    nb_type_map_slow::iterator it;
    bool success;
    nb_internals *internals_ = p;

    PyObject *existing = nullptr;
    {
        lock_internals guard(internals_);
        // Placeholder, filled at the end. Type registration must not race with type use.
        std::tie(it, success) = internals_->type_c2p_slow.try_emplace(t->type, nullptr);
        if (!success) {
            existing = (PyObject *) it->second->type_py;
            NB_INCREF_TYPE(existing);
        }
    }

    if (!success) {
        // Warn only after releasing the lock: PyErr_WarnFormat can run
        // arbitrary Python code, and the internals mutex is non-reentrant
        if (PyErr_WarnFormat(PyExc_RuntimeWarning, 1,
                             "nanobind: type '%s' was already registered!\n",
                             t_name) != 0)
            warning_failed();
        if (has_signature)
            free((char *) t_name);
        return existing;
    }

    if (t->scope != nullptr) {
        if (PyModule_Check(t->scope)) {
            mod = t->scope;
            modname = str_getattr_def(p, t->scope, "__name__");
        } else {
            modname =
                getattr(t->scope, NB_INTERNED(p, __module__), handle());

            object scope_qualname = str_getattr_def(p, t->scope, "__qualname__");
            if (scope_qualname.is_valid())
                qualname = steal<str>(
                    PyUnicode_FromFormat("%U.%U", scope_qualname.ptr(), name.ptr()));
        }
    }

    if (modname.is_valid())
        name = steal<str>(
            PyUnicode_FromFormat("%U.%U", modname.ptr(), name.ptr()));

    constexpr size_t ptr_size = sizeof(void *);
    // Decode 'size_align' (see type_size_align() in nb_backend.h)
    uint32_t t_align = 1u << (t->size_align & 0x1f),
             t_size = (t->size_align >> 5) * t_align;
    size_t basicsize = sizeof(nb_inst) + t_size;
    if (t_align > ptr_size)
        basicsize += t_align - ptr_size;

    PyObject *base = nullptr;

#if !defined(PYPY_VERSION) // see https://github.com/pypy/pypy/issues/4914
    bool generic_base = false;
#endif
    if (has_base_py) {
        check(!has_base,
              "nanobind::detail::nb_type_new(\"%s\"): multiple base types "
              "specified!", t_name);
        base = (PyObject *) t->base_py;

#if !defined(PYPY_VERSION) // see https://github.com/pypy/pypy/issues/4914
        if (Py_TYPE(base) == &Py_GenericAliasType) {
            base = PyObject_GetAttrString(base, "__origin__");
            check(base != nullptr,
                  "nanobind::detail::nb_type_new(\"%s\"): could not access base of type alias!", t_name);
            Py_DECREF(base);
            generic_base = true;
        }
#endif

        check(nb_type_check(p, base),
              "nanobind::detail::nb_type_new(\"%s\"): base type is not a "
              "nanobind type!", t_name);
    } else if (has_base) {
        lock_internals guard(internals_);
        nb_type_map_slow::iterator it2 = internals_->type_c2p_slow.find(t->base);
        check(it2 != internals_->type_c2p_slow.end(),
                  "nanobind::detail::nb_type_new(\"%s\"): base type \"%s\" not "
                  "known to nanobind!", t_name, type_name(t->base));
        base = (PyObject *) it2->second->type_py;
    }

    type_data *tb = nullptr;
    if (base) {
        // Check if the base type already has dynamic attributes
        tb = nb_type_data((PyTypeObject *) base);
        if (tb->flags & (uint32_t) type_flags::has_dynamic_attr)
            has_dynamic_attr = true;

        if (tb->flags & (uint32_t) type_flags::is_weak_referenceable)
            is_weak_referenceable = true;

        // Handle a corner case (base class larger than derived class)
        // which can arise when extending trampoline base classes

        PyTypeObject *base_2 = (PyTypeObject *) base;
        type_data *tb_2 = tb;

        do {
            size_t base_basicsize = sizeof(nb_inst) + tb_2->size;
            if (tb_2->align > ptr_size)
                base_basicsize += tb_2->align - ptr_size;
            if (base_basicsize > basicsize)
                basicsize = base_basicsize;

#if !defined(Py_LIMITED_API)
            base_2 = base_2->tp_base;
#else
            base_2 = (PyTypeObject *) PyType_GetSlot(base_2, Py_tp_base);
#endif

            if (!base_2 || !nb_type_check(p, (PyObject *) base_2))
                break;

            tb_2 = nb_type_data(base_2);
        } while (true);
    }

    bool base_intrusive_ptr =
        tb && (tb->flags & (uint32_t) type_flags::intrusive_ptr);

    // tp_basicsize must satisfy pointer alignment.
    basicsize = (basicsize + ptr_size - 1) / ptr_size * ptr_size;

    char *name_copy = strdup_check(name.c_str());

    constexpr size_t nb_type_max_slots = 12,
                     nb_extra_slots = 80,
                     nb_total_slots = nb_type_max_slots +
                                      nb_extra_slots + 1;

    PyMemberDef members[3] { };
    PyType_Slot slots[nb_total_slots], *s = slots;
    PyType_Spec spec = {
        /* .name = */ name_copy,
        /* .basicsize = */ (int) basicsize,
        /* .itemsize = */ 0,
        /* .flags = */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        /* .slots = */ slots
    };

    if (base)
        *s++ = { Py_tp_base, (void *) base };

    *s++ = { Py_tp_init, (void *) inst_init };
    *s++ = { Py_tp_new, (void *) inst_new_int };
    *s++ = { Py_tp_dealloc, (void *) inst_dealloc };

    if (has_doc)
        *s++ = { Py_tp_doc, (void *) t->doc };

    vectorcallfunc type_vectorcall = nb_type_vectorcall;

    bool has_traverse = false,
         has_getset = false,
         has_custom_init_or_new = false,
         has_custom_type_vectorcall = false;

    if (has_type_slots) {
        size_t num_avail = nb_extra_slots;
        size_t i = 0;
        while (t->type_slots[i].slot) {
            check(i != num_avail,
                  "nanobind::detail::nb_type_new(\"%s\"): ran out of "
                  "type slots!", t_name);

            const PyType_Slot &ts = t->type_slots[i++];
            int slot_id = ts.slot;

            has_traverse |= slot_id == Py_tp_traverse;
            has_getset |= slot_id == Py_tp_getset;
            has_custom_init_or_new |=
                slot_id == Py_tp_init || slot_id == Py_tp_new;

            if (slot_id == Py_tp_vectorcall) {
                type_vectorcall = (vectorcallfunc) ts.pfunc;
                has_custom_type_vectorcall = true;
                continue;
            }

            *s++ = ts;
        }
    }

    if (has_custom_init_or_new && !has_custom_type_vectorcall)
        type_vectorcall = nullptr;

    Py_ssize_t dictoffset = 0, weaklistoffset = 0;
    int num_members = 0;

    // realign to sizeof(void*) if needed
    if (has_dynamic_attr || is_weak_referenceable)
        basicsize = (basicsize + ptr_size - 1) / ptr_size * ptr_size;

    if (has_dynamic_attr) {
        dictoffset = (Py_ssize_t) basicsize;
        basicsize += ptr_size;

        members[num_members] = PyMemberDef{ "__dictoffset__", Py_T_PYSSIZET,
                                            dictoffset, Py_READONLY, nullptr };
        ++num_members;

        // Install GC traverse and clear routines if not inherited/overridden
        if (!has_traverse) {
            *s++ = { Py_tp_traverse, (void *) inst_traverse };
            *s++ = { Py_tp_clear, (void *) inst_clear };
            has_traverse = true;
        }
        spec.basicsize = (int) basicsize;

        if (!has_getset)
            *s++ = { Py_tp_getset, (void *) inst_getset };
    }

    if (is_weak_referenceable) {
        weaklistoffset = (Py_ssize_t) basicsize;
        basicsize += ptr_size;

        members[num_members] = PyMemberDef{ "__weaklistoffset__", Py_T_PYSSIZET,
                                            weaklistoffset, Py_READONLY, nullptr };
        ++num_members;

        // Install GC traverse and clear routines if not inherited/overridden
        if (!has_traverse) {
            *s++ = { Py_tp_traverse, (void *) inst_traverse };
            *s++ = { Py_tp_clear, (void *) inst_clear };
            has_traverse = true;
        }
        spec.basicsize = (int) basicsize;
    }

    if (num_members > 0)
        *s++ = { Py_tp_members, (void*) members };

    if (is_generic)
        *s++ = { Py_tp_methods, (void*) class_getitem_method };

    if (NB_DYNAMIC_VERSION >= 0x030E0000 && type_vectorcall)
        *s++ = { Py_tp_vectorcall, (void *) type_vectorcall };

    if (has_traverse)
        spec.flags |= Py_TPFLAGS_HAVE_GC;

    *s++ = { 0, nullptr };

    PyObject *result = nb_type_from_metaclass(internals_->nb_type, mod, &spec);
    if (!result) {
        python_error err;
        check(false,
              "nanobind::detail::nb_type_new(\"%s\"): type construction "
              "failed: %s!", t_name, err.what());
    }

    make_immortal(result);
    internals_inc_ref(p);

    type_data *to = nb_type_data((PyTypeObject *) result);

    memset((void *) to, 0, sizeof(type_data));
    to->internals = p;
    to->size = t_size;
    to->flags = NB_ABI_FLAGS(t->flags) & ~(uint32_t) type_init_flags::all_init_flags;
    to->align = t_align;
    to->name = t->name;
    to->type = t->type;
    if (t->flags & (uint32_t) type_flags::has_destruct)
        to->destruct = t->destruct;
    if (t->flags & (uint32_t) type_flags::has_copy)
        to->copy = t->copy;
    if (t->flags & (uint32_t) type_flags::has_move)
        to->move = t->move;
    if (intrusive_ptr)
        to->set_self_py = t->set_self_py;
    if (has_shared_from_this)
        to->keep_shared_from_this_alive = t->keep_shared_from_this_alive;

    if (!intrusive_ptr && base_intrusive_ptr) {
        to->flags |= (uint32_t) type_flags::intrusive_ptr;
        to->set_self_py = tb->set_self_py;
    }

    if (!has_shared_from_this && tb &&
        (tb->flags & (uint32_t) type_flags::has_shared_from_this)) {
        to->flags |= (uint32_t) type_flags::has_shared_from_this;
        to->keep_shared_from_this_alive = tb->keep_shared_from_this_alive;
    }

    if (NB_DYNAMIC_VERSION < 0x030E0000) {
        // On Python 3.14+, use Py_tp_vectorcall to set the type vectorcall
        // slot. Otherwise, assign tp_vectorcall or use a workaround (via
        // tp_vectorcall_offset) for stable ABI builds.

        #if defined(Py_LIMITED_API)
            to->vectorcall = type_vectorcall;
        #else
            ((PyTypeObject *) result)->tp_vectorcall = type_vectorcall;
        #endif
    }

    to->name = name_copy;
    to->type_py = (PyTypeObject *) result;
    to->alias_chain = nullptr;
    to->init = nullptr;

    if (has_supplement) {
        to->supplement = PyMem_Malloc(t->supplement_size);
        check(to->supplement,
              "nanobind::detail::nb_type_new(\"%s\"): supplement allocation "
              "failed!", t_name);
        memset(to->supplement, 0, t->supplement_size);
    } else {
        to->supplement = nullptr;
    }

    if (has_dynamic_attr)
        to->flags |= (uint32_t) type_flags::has_dynamic_attr;
    if (is_weak_referenceable)
        to->flags |= (uint32_t) type_flags::is_weak_referenceable;

    // Always cache dictoffset/weaklistoffset so nb_dict_ptr()/nb_weaklist_ptr()
    // only access dicts/weaklists created by nanobind, not those added by Python
    to->dictoffset = (uint32_t) dictoffset;
    to->weaklistoffset = (uint32_t) weaklistoffset;

    // Cache the type's GC-ness (also covers GC-ness inherited from a base) so
    // the allocation/deallocation hot paths avoid a PyType_GetFlags() call.
    bool have_gc =
        PyType_HasFeature((PyTypeObject *) result, Py_TPFLAGS_HAVE_GC);
    if (have_gc)
        to->flags |= (uint32_t) type_flags_internal::has_gc;

    // Instance pool setup + eligibility check (nb::pooled). Decide once here
    // so the hot paths can trust the 'pooled' flag alone.
    to->pool_capacity = 0;
#if defined(NB_FREE_THREADED)
    to->pool_index = 0;
#else
    to->pool = nb_inst_pool{};
#endif
    if (to->flags & (uint32_t) type_flags::pooled) {
#if defined(PYPY_VERSION)
        // PyPy's cpyext object model is incompatible with park/revive step
        to->flags &= ~(uint32_t) type_flags::pooled;
#else
        if (t->pool_capacity == 0) {
            to->flags &= ~(uint32_t) type_flags::pooled;
        } else {
            bool eligible =
                !(to->flags & (uint32_t) type_flags::intrusive_ptr);
            if (!eligible)
                fail("nanobind: type '%s': instance pooling is incompatible "
                     "with intrusive reference counting!", t_name);

            to->pool_capacity = t->pool_capacity;
#if defined(NB_FREE_THREADED)
            to->pool_index = p->pool_index_counter.fetch_add(
                1, std::memory_order_relaxed);
#endif
        }
#endif
    }

    if (t->scope != nullptr)
        setattr_str(p, t->scope, t_name, 0, result);

    setattr_str(p, result, "__qualname__", sizeof("__qualname__"),
                qualname.ptr());

    if (modname.is_valid())
        setattr(result, NB_INTERNED(p, __module__), modname.ptr());

    {
        lock_internals guard(internals_);
        internals_->type_c2p_slow[t->type] = to;

        #if !defined(NB_FREE_THREADED)
            internals_->type_c2p_fast[(void *) t->type] = to;
        #endif
    }

    if (has_signature) {
        str_setattr(p, result, "__nb_signature__", str(t->name));
        free((char *) t_name);
    }

#if !defined(PYPY_VERSION)
    if (generic_base)
        str_setattr(p, result, "__orig_bases__",
                    steal(raise_if_null(
                        PyTuple_Pack(1, (PyObject *) t->base_py))));
#endif

    return result;
}

PyObject *nb_type_new(nb_internals *p, const type_data_init *t) noexcept {
    try {
        return nb_type_new_impl(p, t);
    } catch (...) {
        fail_exception("nanobind::detail::nb_type_new", t->name);
    }
}


PyObject *call_one_arg(PyObject *fn, PyObject *arg) noexcept {
    PyObject *argv[2] = { nullptr, arg };
    return PyObject_Vectorcall(fn, argv + 1,
                               PY_VECTORCALL_ARGUMENTS_OFFSET + 1, nullptr);
}

/// Encapsulates the implicit conversion part of nb_type_get()
static NB_NOINLINE bool nb_type_get_implicit(PyObject *src,
                                             PyTypeObject *src_type,
                                             const std::type_info *cpp_type_src,
                                             const type_data *dst_type,
                                             nb_internals *internals_,
                                             cleanup_list *cleanup, void **out) noexcept {
    if (dst_type->implicit.cpp && cpp_type_src) {
        const std::type_info **it = dst_type->implicit.cpp;
        const std::type_info *v;

        while ((v = *it++)) {
            if (v == cpp_type_src || *v == *cpp_type_src)
                goto found;
        }

        it = dst_type->implicit.cpp;
        while ((v = *it++)) {
            const type_data *d = nb_type_c2p(internals_, v);
            if (d && PyType_IsSubtype(src_type, d->type_py))
                goto found;
        }
    }

    if (dst_type->implicit.py) {
        bool (**it)(PyTypeObject *, PyObject *, cleanup_list *) noexcept =
            dst_type->implicit.py;
        bool (*v2)(PyTypeObject *, PyObject *, cleanup_list *) noexcept;

        while ((v2 = *it++)) {
            if (v2(dst_type->type_py, src, cleanup))
                goto found;
        }
    }

    return false;

found:
    PyObject *result = call_one_arg((PyObject *) dst_type->type_py, src);

    if (result) {
        cleanup->append(result);
        *out = inst_ptr((nb_inst *) result);
        return true;
    } else {
        PyErr_Clear();

        if (internals_->print_implicit_cast_warnings) {
#if !defined(Py_LIMITED_API)
            const char *name = src_type->tp_name;
#else
            PyObject *name_py = nb_inst_name(src);
            const char *name = PyUnicode_AsUTF8AndSize(name_py, nullptr);
#endif
            // Can't use PyErr_Warn*() if conversion failed due to a stack overflow
            fprintf(stderr,
                    "nanobind: implicit conversion from type '%s' to type '%s' "
                    "failed!\n", name, dst_type->name);

#if defined(Py_LIMITED_API)
            Py_DECREF(name_py);
#endif
        }

        return false;
    }
}

// Issue a warning when an instance is in an found to be in an invalid state
NB_NOINLINE static bool nb_type_get_state_error(uint8_t state,
                                                const char *name) noexcept {
    constexpr const char *errors[4] = {
        /* 0 = uninit  */ "attempted to access an uninitialized instance",
        /* 1 = relinq  */ "attempted to access a relinquished instance",
        /* 2 = ready   */ "attempted to initialize an already-initialized instance",
        /* 3 = invalid */ "instance state has become corrupted",
    };
    if (PyErr_WarnFormat(PyExc_RuntimeWarning, 1, "nanobind: %s of type '%s'!\n",
                         errors[state & 3], name) != 0)
        warning_failed();
    return false;
}

// Attempt to retrieve a pointer to a C++ instance
bool nb_type_get(nb_internals *p, const std::type_info *cpp_type,
                 PyObject *src, uint32_t flags, cleanup_list *cleanup,
                 void **out) noexcept {
    static_assert(cast_flags::construct == nb_inst_state::state_ready,
                  "this function is optimized assuming that "
                  "cast_flags::construct == nb_inst_state::state_ready");

    // Convert None -> nullptr, unless the target binds by value/reference and
    // therefore has no valid mapping for None (then reject -> next overload).
    if (NB_UNLIKELY(src == none_ptr())) {
        if (flags & cast_flags::none_disallowed)
            return false;
        *out = nullptr;
        return true;
    }

    // Trusted 'self' from nb_type_vectorcall: a freshly allocated instance of
    // exactly this type, so skip verification and read the pointer directly.
    if (NB_UNLIKELY(flags & cast_flags::trusted)) {
        *out = inst_ptr((nb_inst *) src);
        return true;
    }

    PyTypeObject *src_type = Py_TYPE(src);
    const std::type_info *cpp_type_src = nullptr;
    const bool src_is_nb_type = nb_type_check(p, (PyObject *) src_type);

    type_data *dst_type = nullptr;
    nb_internals *internals_ = p;

    // If 'src' is a nanobind-bound type
    if (NB_LIKELY(src_is_nb_type)) {
        type_data *t = nb_type_data(src_type);
        cpp_type_src = t->type;

        // Check if the source / destination typeid are an exact match
        bool valid = cpp_type == cpp_type_src || *cpp_type == *cpp_type_src;

        // If not, look up the Python type and check the inheritance chain
        if (NB_UNLIKELY(!valid)) {
            dst_type = nb_type_c2p(internals_, cpp_type);
            if (dst_type)
                valid = PyType_IsSubtype(src_type, dst_type->type_py);
        }

        // Success, return the pointer if the instance is correctly initialized
        if (NB_LIKELY(valid)) {
            nb_inst *inst = (nb_inst *) src;

            // (flags & construct)       state  xor-result  should accept?
            //          [normal] 0  [uninit] 0  0           no
            //          [normal] 0  [relinq] 1  1           no
            //          [normal] 0   [ready] 2  2           yes
            //       [construct] 2  [uninit] 0  2           yes
            //       [construct] 2  [relinq] 1  3           no
            //       [construct] 2   [ready] 2  0           no

            if (NB_UNLIKELY(((flags & cast_flags::construct) ^
                             inst->state.state) != nb_inst_state::state_ready))
                return nb_type_get_state_error(inst->state.state, t->name);

            *out = inst_ptr(inst);

            return true;
        }
    }

    // Try an implicit conversion as last resort (if possible & requested)
    if ((flags & (uint16_t) cast_flags::convert) && cleanup) {
        if (!src_is_nb_type)
            dst_type = nb_type_c2p(internals_, cpp_type);

        if (dst_type &&
            (dst_type->flags & (uint32_t) type_flags_internal::has_implicit_conversions))
            return nb_type_get_implicit(src, src_type, cpp_type_src, dst_type,
                                        internals_, cleanup, out);
    }

    return false;
}

static PyObject *keep_alive_callback(PyObject *self, PyObject *const *args,
                                     Py_ssize_t nargs) {
    check(nargs == 1 && PyWeakref_CheckRefExact(args[0]),
          "nanobind::detail::keep_alive_callback(): invalid input!");
    Py_DECREF(args[0]); // self
    Py_DECREF(self); // patient
    return none_ref();
}

static PyMethodDef keep_alive_callback_def = {
    "keep_alive_callback", (PyCFunction) (void *) keep_alive_callback,
    METH_FASTCALL, nullptr
};

void keep_alive_py(nb_internals *p, PyObject *nurse, PyObject *patient) {
    PyObject *none = none_ptr();
    if (!patient || !nurse || nurse == none || patient == none)
        return;

    if (nb_type_check(p, (PyObject *) Py_TYPE(nurse))) {
#if defined(NB_FREE_THREADED)
        nb_shard &shard = p->shard(inst_ptr((nb_inst *) nurse));
        lock_shard guard(shard);
#else
        nb_shard &shard = p->shards[0];
#endif

        nb_weakref_seq **pp = (nb_weakref_seq **) &shard.keep_alive[nurse];
        do {
            nb_weakref_seq *p = *pp;
            if (!p)
                break;
            else if (p->payload == patient && !p->callback)
                return;
            pp = &p->next;
        } while (true);

        nb_weakref_seq *s =
            (nb_weakref_seq *) PyMem_Malloc(sizeof(nb_weakref_seq));
        check(s, "nanobind::detail::keep_alive_py(): out of memory!");

        s->payload = patient;
        s->callback = nullptr;
        s->next = nullptr;
        *pp = s;

        Py_INCREF(patient);
        ((nb_inst *) nurse)->state.clear_keep_alive = true;
    } else {
        PyObject *callback =
            PyCFunction_New(&keep_alive_callback_def, patient);
        check(callback,
              "nanobind::detail::keep_alive_py(): callback creation failed!");

        PyObject *weakref = PyWeakref_NewRef(nurse, callback);
        if (!weakref) {
            Py_DECREF(callback);
            PyErr_Clear();
            raise("nanobind::detail::keep_alive_py(): could not create a weak "
                  "reference! Likely, the 'nurse' argument you specified is not "
                  "a weak-referenceable type!");
        }

        // Increase patient reference count, leak weak reference
        Py_INCREF(patient);
        Py_DECREF(callback);
    }
}

void keep_alive_ptr(nb_internals *p, PyObject *nurse, void *payload,
                void (*callback)(void *) noexcept) noexcept {
    check(nurse, "nanobind::detail::keep_alive_ptr(): 'nurse' is undefined!");

    if (nb_type_check(p, (PyObject *) Py_TYPE(nurse))) {
#if defined(NB_FREE_THREADED)
        nb_shard &shard = p->shard(inst_ptr((nb_inst *) nurse));
        lock_shard guard(shard);
#else
        nb_shard &shard = p->shards[0];
#endif

        nb_weakref_seq
            **pp = (nb_weakref_seq **) &shard.keep_alive[nurse],
            *s   = (nb_weakref_seq *) PyMem_Malloc(sizeof(nb_weakref_seq));
        check(s, "nanobind::detail::keep_alive_ptr(): out of memory!");

        s->payload = payload;
        s->callback = callback;
        s->next = *pp;
        *pp = s;

        ((nb_inst *) nurse)->state.clear_keep_alive = true;
    } else {
        auto capsule_cleanup = [](PyObject *o) {
            auto callback_2 = (void (*)(void *)) PyCapsule_GetContext(o);
            if (callback_2)
                callback_2(PyCapsule_GetPointer(o, nullptr));
        };

        PyObject *patient = PyCapsule_New(payload, nullptr, capsule_cleanup);
        check(patient, "nanobind::detail::keep_alive_ptr(): capsule creation "
                       "failed!");
        check(PyCapsule_SetContext(patient, (void *) callback) == 0,
              "nanobind::detail::keep_alive_ptr(): could not set context!");

        keep_alive_py(p, nurse, patient);
        Py_DECREF(patient);
    }
}

static PyObject *nb_type_put_common(void *value, type_data *t, rv_policy rvp,
                                    cleanup_list *cleanup,
                                    bool *is_new) noexcept {
    // The reference_internals RVP needs a self pointer, give up if unavailable
    if (rvp == rv_policy::reference_internal && (!cleanup || !cleanup->self()))
        return nullptr;

    const bool intrusive = t->flags & (uint32_t) type_flags::intrusive_ptr;
    if (intrusive)
        rvp = rv_policy::take_ownership;

    const bool create_new = rvp == rv_policy::copy || rvp == rv_policy::move;

    nb_inst *inst;
    if (create_new)
        inst = (nb_inst *) inst_new_int(t->type_py, nullptr, nullptr);
    else
        inst = (nb_inst *) inst_new_ext(t->type_py, value);

    if (!inst)
        return nullptr;

    void *new_value = inst_ptr(inst);
    if (rvp == rv_policy::move) {
        if (t->flags & (uint32_t) type_flags::is_move_constructible) {
            if (t->flags & (uint32_t) type_flags::has_move) {
                try {
                    t->move(new_value, value);
                } catch (...) {
                    Py_DECREF(inst);
                    return nullptr;
                }
            } else {
                memcpy(new_value, value, t->size);
                memset(value, 0, t->size);
            }
        } else {
            check(t->flags & (uint32_t) type_flags::is_copy_constructible,
                  "nanobind::detail::nb_type_put(\"%s\"): attempted to move "
                  "an instance that is neither copy- nor move-constructible!",
                  t->name);

            rvp = rv_policy::copy;
        }
    }

    if (rvp == rv_policy::copy) {
        check(t->flags & (uint32_t) type_flags::is_copy_constructible,
              "nanobind::detail::nb_type_put(\"%s\"): attempted to copy "
              "an instance that is not copy-constructible!", t->name);

        if (t->flags & (uint32_t) type_flags::has_copy) {
            try {
                t->copy(new_value, value);
            } catch (...) {
                Py_DECREF(inst);
                return nullptr;
            }
        } else {
            memcpy(new_value, value, t->size);
        }
    }

    // If we can find an existing C++ shared_ptr for this object, and
    // the instance we're creating just holds a pointer, then take out
    // another C++ shared_ptr that shares ownership with the existing
    // one, and tie its lifetime to the Python object. This is the
    // same thing done by the <nanobind/stl/shared_ptr.h> caster when
    // returning shared_ptr<T> to Python explicitly.
    if ((t->flags & (uint32_t) type_flags::has_shared_from_this) &&
        !create_new && t->keep_shared_from_this_alive((PyObject *) inst))
        rvp = rv_policy::reference;
    else if (is_new)
        *is_new = true;

    inst->state.destruct = rvp != rv_policy::reference && rvp != rv_policy::reference_internal;
    inst->state.cpp_delete = rvp == rv_policy::take_ownership;
    inst->state.state = nb_inst_state::state_ready;

    if (rvp == rv_policy::reference_internal)
        keep_alive_py(t->internals, (PyObject *) inst, cleanup->self());

    if (intrusive)
        t->set_self_py(new_value, (PyObject *) inst);

    if (!create_new)
        inst_register(t->internals, (PyObject *) inst, value);

    return (PyObject *) inst;
}

PyObject *nb_type_put(nb_internals *p, const std::type_info *cpp_type,
                      const std::type_info *cpp_type_p,
                      void *value, rv_policy rvp,
                      cleanup_list *cleanup,
                      bool *is_new) noexcept {
    // Convert nullptr -> None
    if (!value)
        return none_ref();

    // Check if the instance is already registered with nanobind
    nb_internals *internals_ = p;

    // Look up the corresponding Python type
    type_data *td = nullptr,
              *td_p = nullptr;

    auto lookup_type = [cpp_type, cpp_type_p, internals_, &td, &td_p]() -> bool {
        if (!td) {
            type_data *d = nb_type_c2p(internals_, cpp_type);
            if (!d)
                return false;
            td = d;

            if (cpp_type_p && cpp_type_p != cpp_type)
                td_p = nb_type_c2p(internals_, cpp_type_p);
        }

        return true;
    };

    if (rvp != rv_policy::copy) {
        nb_shard &shard = internals_->shard(value);
        lock_shard guard(shard);

        // Check if the instance is already registered with nanobind
        nb_ptr_map &inst_c2p = shard.inst_c2p;
        nb_ptr_map::iterator it = inst_c2p.find(value);

        if (it != inst_c2p.end()) {
            void *entry = it->second;
            nb_inst_seq seq;

            if (NB_UNLIKELY(nb_is_seq(entry))) {
                seq = *nb_get_seq(entry);
            } else {
                seq.inst = (PyObject *) entry;
                seq.next = nullptr;
            }

            while (true) {
                PyTypeObject *tp = Py_TYPE(seq.inst);

                const std::type_info *p = nb_type_data(tp)->type;

                if (p == cpp_type || p == cpp_type_p) {
                    if (nb_try_inc_ref(seq.inst))
                        return seq.inst;
                }

                if (!lookup_type())
                    return nullptr;

                if (PyType_IsSubtype(tp, td->type_py) ||
                    (td_p && PyType_IsSubtype(tp, td_p->type_py))) {
                    if (nb_try_inc_ref(seq.inst))
                        return seq.inst;
                }

                if (seq.next == nullptr)
                    break;

                seq = *seq.next;
            }
        } else if (rvp == rv_policy::none) {
            return nullptr;
        }
    }

    // Look up the corresponding Python type if not already done
    if (!lookup_type())
        return nullptr;

    return nb_type_put_common(value, td_p ? td_p : td, rvp, cleanup, is_new);
}

static void nb_type_put_unique_finalize(PyObject *o,
                                        const std::type_info *cpp_type,
                                        bool cpp_delete, bool is_new) {
    (void) cpp_type;
    check(cpp_delete || !is_new,
          "nanobind::detail::nb_type_put_unique(type='%s', cpp_delete=%i): "
          "ownership status has become corrupted.",
          type_name(cpp_type), cpp_delete);

    nb_inst *inst = (nb_inst *) o;

    if (cpp_delete) {
        check(inst->state.state == (is_new ? nb_inst_state::state_ready
                                     : nb_inst_state::state_relinquished) &&
                  (bool) inst->state.destruct == is_new &&
                  (bool) inst->state.cpp_delete == is_new,
              "nanobind::detail::nb_type_put_unique(type='%s', cpp_delete=%i): "
              "unexpected status flags! (state=%i, destruct=%i, cpp_delete=%i)",
              type_name(cpp_type), cpp_delete, inst->state.state, inst->state.destruct,
              inst->state.cpp_delete);

        inst->state.state = nb_inst_state::state_ready;
        inst->state.destruct = inst->state.cpp_delete = true;
    } else {
        check(inst->state.state == nb_inst_state::state_relinquished,
                  "nanobind::detail::nb_type_put_unique('%s'): ownership "
                  "status has become corrupted.", type_name(cpp_type));

        inst->state.state = nb_inst_state::state_ready;
    }
}

PyObject *nb_type_put_unique(nb_internals *p, const std::type_info *cpp_type,
                             const std::type_info *cpp_type_p,
                             void *value,
                             cleanup_list *cleanup, bool cpp_delete) noexcept {
    rv_policy policy = cpp_delete ? rv_policy::take_ownership_v : rv_policy::none_v;

    bool is_new = false;
    PyObject *o =
        nb_type_put(p, cpp_type, cpp_type_p, value, policy, cleanup, &is_new);

    if (o)
        nb_type_put_unique_finalize(o, cpp_type, cpp_delete, is_new);

    return o;
}

static void warn_relinquish_failed(const char *why, PyObject *o) noexcept {
    PyObject *name = nb_inst_name(o);
    int rc = PyErr_WarnFormat(
        PyExc_RuntimeWarning, 1,
        "nanobind::detail::nb_relinquish_ownership(): could not "
        "transfer ownership of a Python instance of type '%U' to C++. %s",
        name, why);
    if (rc != 0) // user has configured warnings-as-errors
        PyErr_WriteUnraisable(o);
    Py_DECREF(name);
}

bool nb_type_relinquish_ownership(PyObject *o, bool cpp_delete) noexcept {
    nb_inst *inst = (nb_inst *) o;

    // This function is called after nb_type_get() succeeds, so the instance
    // should be ready; but the !ready case is possible if an attempt is made to
    // transfer ownership of the same object to C++ multiple times as part of
    // the same data structure. For example, converting Python (foo, foo) to C++
    // std::pair<std::unique_ptr<T>, std::unique_ptr<T>>.

    if (inst->state.state != nb_inst_state::state_ready) {
        warn_relinquish_failed(
            "The resulting data structure would have multiple "
            "std::unique_ptrs, each thinking that they own the same instance, "
            "which is not allowed.", o);
        return false;
    }

    if (cpp_delete) {
        if (!inst->state.cpp_delete || !inst->state.destruct || inst->state.internal) {
            warn_relinquish_failed(
                "This is only possible when the instance was previously "
                "constructed on the C++ side and is now owned by Python, which "
                "was not the case here. You could change the unique pointer "
                "signature to std::unique_ptr<T, nb::deleter<T>> to work "
                "around this issue.", o);
            return false;
        }

        inst->state.cpp_delete = false;
        inst->state.destruct = false;
    }

    inst->state.state = nb_inst_state::state_relinquished;
    return true;
}

void nb_type_restore_ownership(PyObject *o, bool cpp_delete) noexcept {
    nb_inst *inst = (nb_inst *) o;

    check(inst->state.state == nb_inst_state::state_relinquished,
          "nanobind::detail::nb_type_restore_ownership('%s'): ownership "
          "status has become corrupted.",
          PyUnicode_AsUTF8AndSize(nb_inst_name(o), nullptr));

    inst->state.state = nb_inst_state::state_ready;
    if (cpp_delete) {
        inst->state.cpp_delete = true;
        inst->state.destruct = true;
    }
}

bool nb_type_isinstance(nb_internals *p, PyObject *o,
                        const std::type_info *t) noexcept {
    type_data *d = nb_type_c2p(p, t);
    if (d)
        return PyType_IsSubtype(Py_TYPE(o), d->type_py);
    else
        return false;
}

PyObject *nb_type_lookup(nb_internals *p,
                         const std::type_info *t) noexcept {
    type_data *d = nb_type_c2p(p, t);
    if (d)
        return (PyObject *) d->type_py;
    else
        return nullptr;
}

bool nb_type_check(nb_internals *p, PyObject *t) noexcept {
    return p->nb_type == Py_TYPE(t);
}

size_t nb_type_size(PyObject *t) noexcept {
    return nb_type_data((PyTypeObject *) t)->size;
}

size_t nb_type_align(PyObject *t) noexcept {
    return nb_type_data((PyTypeObject *) t)->align;
}

const std::type_info *nb_type_info(PyObject *t) noexcept {
    return nb_type_data((PyTypeObject *) t)->type;
}

void *nb_type_supplement(PyObject *t) noexcept {
    return nb_type_data((PyTypeObject *) t)->supplement;
}

PyObject *nb_inst_alloc(PyTypeObject *t) {
    PyObject *result = inst_new_int(t, nullptr, nullptr);
    if (!result)
        raise_python_error();
    return result;
}

PyObject *nb_inst_reference(PyTypeObject *t, void *ptr, PyObject *parent) {
    PyObject *result = inst_new_ext(t, ptr);
    if (!result)
        raise_python_error();
    nb_internals *p = nb_type_data(t)->internals;
    nb_inst *nbi = (nb_inst *) result;
    nbi->state.destruct = nbi->state.cpp_delete = false;
    nbi->state.state = nb_inst_state::state_ready;
    if (parent)
        keep_alive_py(p, result, parent);
    inst_register(p, result, ptr);
    return result;
}

PyObject *nb_inst_take_ownership(PyTypeObject *t, void *ptr) {
    PyObject *result = inst_new_ext(t, ptr);
    if (!result)
        raise_python_error();
    nb_inst *nbi = (nb_inst *) result;
    nbi->state.destruct = nbi->state.cpp_delete = true;
    nbi->state.state = nb_inst_state::state_ready;
    inst_register(nb_type_data(t)->internals, result, ptr);
    return result;
}

void *nb_inst_ptr(PyObject *o) noexcept {
    return inst_ptr((nb_inst *) o);
}

void nb_inst_zero(PyObject *o) noexcept {
    nb_inst *nbi = (nb_inst *) o;
    type_data *td = nb_type_data(Py_TYPE(o));
    memset(inst_ptr(nbi), 0, td->size);
    nbi->state.state = nb_inst_state::state_ready;
    nbi->state.destruct = true;
}

PyObject *nb_inst_alloc_zero(PyTypeObject *t) {
    PyObject *result = inst_new_int(t, nullptr, nullptr);
    if (!result)
        raise_python_error();
    nb_inst *nbi = (nb_inst *) result;
    type_data *td = nb_type_data(t);
    memset(inst_ptr(nbi), 0, td->size);
    nbi->state.state = nb_inst_state::state_ready;
    nbi->state.destruct = true;
    return result;
}

void nb_inst_state_write(PyObject *o, uint32_t state) noexcept {
    nb_inst *nbi = (nb_inst *) o;
    bool ready = state & 1, destruct = state & 2;
    nbi->state.state = (uint8_t) (ready ? nb_inst_state::state_ready
                                        : nb_inst_state::state_uninitialized);
    nbi->state.destruct = destruct;
    nbi->state.cpp_delete = destruct && !nbi->state.internal;
}

uint32_t nb_inst_state_read(PyObject *o) noexcept {
    nb_inst *nbi = (nb_inst *) o;
    return (uint32_t) (nbi->state.state == nb_inst_state::state_ready) |
           ((uint32_t) nbi->state.destruct << 1);
}

void nb_inst_destruct(PyObject *o) noexcept {
    nb_inst *nbi = (nb_inst *) o;
    type_data *t = nb_type_data(Py_TYPE(o));

    check(nbi->state.state != nb_inst_state::state_relinquished,
          "nanobind::detail::nb_inst_destruct(\"%s\"): attempted to destroy "
          "an object whose ownership had been transferred away!",
          t->name);

    if (nbi->state.destruct) {
        check(t->flags & (uint32_t) type_flags::is_destructible,
              "nanobind::detail::nb_inst_destruct(\"%s\"): attempted to call "
              "the destructor of a non-destructible type!",
              t->name);
        if (t->flags & (uint32_t) type_flags::has_destruct)
            t->destruct(inst_ptr(nbi));
        nbi->state.destruct = false;
    }

    nbi->state.state = nb_inst_state::state_uninitialized;
}

/* Shared implementation of nb_inst_copy/nb_inst_move. An uninitialized
   'dst' is constructed in place and marked to-be-destructed; a live 'dst'
   gets replace semantics: its contents are destructed first (even if the
   'destruct' flag is unset) and the flag keeps its previous value, so an
   instance referencing foreign storage stays a non-owning view. */
static void inst_copy_move(PyObject *dst, const PyObject *src,
                           bool move) noexcept {
    if (src == dst)
        return;

    nb_inst *nbi = (nb_inst *) dst;
    bool ready = nbi->state.state == nb_inst_state::state_ready,
         destruct = true;

    if (ready) {
        destruct = nbi->state.destruct;
        nbi->state.destruct = true;
        nb_inst_destruct(dst);
    }

    PyTypeObject *tp = Py_TYPE((PyObject *) src);
    type_data *t = nb_type_data(tp);

    check(tp == Py_TYPE(dst) &&
              (t->flags & (uint32_t) (move ? type_flags::is_move_constructible
                                           : type_flags::is_copy_constructible)),
          "nanobind::detail::nb_inst_copy(): invalid arguments!");

    void *src_data = inst_ptr((nb_inst *) src),
         *dst_data = inst_ptr(nbi);

    if (move) {
        if (t->flags & (uint32_t) type_flags::has_move) {
            t->move(dst_data, src_data);
        } else {
            memcpy(dst_data, src_data, t->size);
            memset(src_data, 0, t->size);
        }
    } else {
        if (t->flags & (uint32_t) type_flags::has_copy)
            t->copy(dst_data, src_data);
        else
            memcpy(dst_data, src_data, t->size);
    }

    nbi->state.state = nb_inst_state::state_ready;
    nbi->state.destruct = destruct;
}

void nb_inst_copy(PyObject *dst, const PyObject *src) noexcept {
    inst_copy_move(dst, src, false);
}

void nb_inst_move(PyObject *dst, const PyObject *src) noexcept {
    inst_copy_move(dst, src, true);
}

PyObject *nb_type_name(PyObject *t) noexcept {
    error_scope s;

#if PY_VERSION_HEX >= 0x030B0000
    // PyType_GetName() reads the name slot directly and always yields a 'str'
    PyObject *result = PyType_GetName((PyTypeObject *) t);
    if (NB_UNLIKELY(!result))
        return PyUnicode_FromString("<unknown type>");
#else
    // The '__name__' attribute may be missing or not a string
    PyObject *result = PyObject_GetAttrString(t, "__name__");
    if (NB_UNLIKELY(!result || !PyUnicode_Check(result))) {
        Py_XDECREF(result);
        return PyUnicode_FromString("<unknown type>");
    }
#endif

    if (PyType_HasFeature((PyTypeObject *) t, Py_TPFLAGS_HEAPTYPE)) {
#if NB_PYTHON_VERSION >= 0x030D0000
        PyObject *mod = PyType_GetModuleName((PyTypeObject *) t);
#else
        PyObject *mod = PyObject_GetAttrString(t, "__module__");
#endif
        // Tolerate a missing or non-string '__module__' attribute
        if (NB_LIKELY(mod && PyUnicode_Check(mod))) {
            PyObject *combined = PyUnicode_FromFormat("%U.%U", mod, result);
            Py_DECREF(result);
            result = combined;
        }
        Py_XDECREF(mod);
    }

    return result;
}

PyObject *nb_inst_name(PyObject *o) noexcept {
        return nb_type_name((PyObject *) Py_TYPE(o));
}

bool nb_inst_python_derived(PyObject *o) noexcept {
    return nb_type_data(Py_TYPE(o))->flags &
           (uint32_t) type_flags_internal::is_python_type;
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
