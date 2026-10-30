/*
    src/internals.cpp: internal libnanobind data structures

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include "nb_internals.h"
#include <thread>

#if defined(NB_FREE_THREADED)
#  if defined(_WIN32)
#    include <windows.h>
#  else
#    include <pthread.h>
#  endif
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

extern PyObject *nb_func_getattro(PyObject *, PyObject *);
extern PyObject *nb_func_get_doc(PyObject *, void *);
extern PyObject *nb_func_get_nb_signature(PyObject *, void *);
extern PyObject *nb_bound_method_getattro(PyObject *, PyObject *);
extern PyObject *nb_bound_method_get_doc(PyObject *, void *);
extern int nb_func_traverse(PyObject *, visitproc, void *);
extern int nb_func_clear(PyObject *);
extern void nb_func_dealloc(PyObject *);
extern int nb_bound_method_traverse(PyObject *, visitproc, void *);
extern int nb_bound_method_clear(PyObject *);
extern void nb_bound_method_dealloc(PyObject *);
extern PyObject *nb_method_descr_get(PyObject *, PyObject *, PyObject *);


static PyType_Slot nb_meta_slots[] = {
    { Py_tp_base, nullptr },
    { 0, nullptr }
};

static PyType_Spec nb_meta_spec = {
    /* .name = */ "nanobind.nb_meta",
    /* .basicsize = */ 0,
    /* .itemsize = */ 0,
    /* .flags = */ Py_TPFLAGS_DEFAULT |
                   Py_TPFLAGS_IMMUTABLETYPE,
    /* .slots = */ nb_meta_slots
};

static PyMemberDef nb_func_members[] = {
    { "__vectorcalloffset__", Py_T_PYSSIZET,
      (Py_ssize_t) offsetof(nb_func, vectorcall), Py_READONLY, nullptr },
    { nullptr, 0, 0, 0, nullptr }
};

static PyGetSetDef nb_func_getset[] = {
    { "__doc__", nb_func_get_doc, nullptr, nullptr, nullptr },
    { "__nb_signature__", nb_func_get_nb_signature, nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr, nullptr, nullptr }
};

static PyType_Slot nb_func_slots[] = {
    { Py_tp_members, (void *) nb_func_members },
    { Py_tp_getset, (void *) nb_func_getset },
    { Py_tp_getattro, (void *) nb_func_getattro },
    { Py_tp_traverse, (void *) nb_func_traverse },
    { Py_tp_clear, (void *) nb_func_clear },
    { Py_tp_dealloc, (void *) nb_func_dealloc },
    { Py_tp_new, (void *) PyType_GenericNew },
    { Py_tp_call, (void *) PyVectorcall_Call },
    { 0, nullptr }
};

static PyType_Spec nb_func_spec = {
    /* .name = */ "nanobind.nb_func",
    /* .basicsize = */ (int) sizeof(nb_func),
    /* .itemsize = */ (int) sizeof(func_data),
    /* .flags = */ Py_TPFLAGS_DEFAULT |
                   Py_TPFLAGS_HAVE_GC |
                   Py_TPFLAGS_HAVE_VECTORCALL |
                   Py_TPFLAGS_IMMUTABLETYPE,
    /* .slots = */ nb_func_slots
};

static PyType_Slot nb_method_slots[] = {
    { Py_tp_members, (void *) nb_func_members },
    { Py_tp_getset, (void *) nb_func_getset },
    { Py_tp_getattro, (void *) nb_func_getattro },
    { Py_tp_traverse, (void *) nb_func_traverse },
    { Py_tp_clear, (void *) nb_func_clear },
    { Py_tp_dealloc, (void *) nb_func_dealloc },
    { Py_tp_descr_get, (void *) nb_method_descr_get },
    { Py_tp_new, (void *) PyType_GenericNew },
    { Py_tp_call, (void *) PyVectorcall_Call },
    { 0, nullptr }
};

static PyType_Spec nb_method_spec = {
    /*.name = */ "nanobind.nb_method",
    /*.basicsize = */ (int) sizeof(nb_func),
    /*.itemsize = */ (int) sizeof(func_data),
    /*.flags = */ Py_TPFLAGS_DEFAULT |
                  Py_TPFLAGS_HAVE_GC |
                  Py_TPFLAGS_METHOD_DESCRIPTOR |
                  Py_TPFLAGS_HAVE_VECTORCALL |
                  Py_TPFLAGS_IMMUTABLETYPE,
    /*.slots = */ nb_method_slots
};

static PyMemberDef nb_bound_method_members[] = {
    { "__vectorcalloffset__", Py_T_PYSSIZET,
      (Py_ssize_t) offsetof(nb_bound_method, vectorcall), Py_READONLY, nullptr },
    { "__func__", Py_T_OBJECT_EX,
      (Py_ssize_t) offsetof(nb_bound_method, func), Py_READONLY, nullptr },
    { "__self__", Py_T_OBJECT_EX,
      (Py_ssize_t) offsetof(nb_bound_method, self), Py_READONLY, nullptr },
    { nullptr, 0, 0, 0, nullptr }
};

static PyGetSetDef nb_bound_method_getset[] = {
    { "__doc__", nb_bound_method_get_doc, nullptr, nullptr, nullptr },
    { nullptr, nullptr, nullptr, nullptr, nullptr }
};

static PyType_Slot nb_bound_method_slots[] = {
    { Py_tp_members, (void *) nb_bound_method_members },
    { Py_tp_getset, (void *) nb_bound_method_getset },
    { Py_tp_getattro, (void *) nb_bound_method_getattro },
    { Py_tp_traverse, (void *) nb_bound_method_traverse },
    { Py_tp_clear, (void *) nb_bound_method_clear },
    { Py_tp_dealloc, (void *) nb_bound_method_dealloc },
    { Py_tp_call, (void *) PyVectorcall_Call },
    { 0, nullptr }
};

static PyType_Spec nb_bound_method_spec = {
    /* .name = */ "nanobind.nb_bound_method",
    /* .basicsize = */ (int) sizeof(nb_bound_method),
    /* .itemsize = */ 0,
    /* .flags = */ Py_TPFLAGS_DEFAULT |
                   Py_TPFLAGS_HAVE_GC |
                   Py_TPFLAGS_HAVE_VECTORCALL |
                   Py_TPFLAGS_IMMUTABLETYPE,
    /* .slots = */ nb_bound_method_slots
};

void default_exception_translator(const std::exception_ptr &p, void *) {
    try {
        std::rethrow_exception(p);
    } catch (const std::bad_alloc &e) {
        PyErr_SetString(PyExc_MemoryError, e.what());
    } catch (const std::domain_error &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::invalid_argument &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::length_error &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::out_of_range &e) {
        PyErr_SetString(PyExc_IndexError, e.what());
    } catch (const std::range_error &e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::overflow_error &e) {
        PyErr_SetString(PyExc_OverflowError, e.what());
    } catch (const std::exception &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    }
}

// Backend state instances created by this binary
static std::vector<nb_internals *> internals_created;
#if defined(NB_FREE_THREADED)
static PyMutex internals_created_mutex { };
#endif

#if defined(Py_LIMITED_API)
// Interpreter constants declared in nb_internals.h
freefunc PyType_Type_tp_free = nullptr;
initproc PyType_Type_tp_init = nullptr;
inquiry PyType_Type_tp_clear = nullptr;
destructor PyType_Type_tp_dealloc = nullptr;
setattrofunc PyType_Type_tp_setattro = nullptr;
descrgetfunc PyProperty_Type_tp_descr_get = nullptr;
descrsetfunc PyProperty_Type_tp_descr_set = nullptr;
ptrdiff_t nb_type_data_offset = 0;
#endif

#if defined(NB_FREE_THREADED)
NB_THREAD_LOCAL nb_thread_state *nb_thread_state_tls = nullptr;

// Reclaims a thread's state when it exits (the cleanup-key callback).
static void nb_thread_state_destroy(void *p) noexcept {
    nb_thread_state *ts = (nb_thread_state *) p;
    if (!ts)
        return;

    // Reclaim this thread's instance pools if the runtime is still alive.
    // Both steps need a thread state, so a shutdown that beats this callback
    // leaves the pools to the operating system.
    if (ts->pools) {
        if (cleanup_guard guard{}) {
            for (uint32_t i = 0; i < ts->pools_size; ++i)
                nb_pool_drain(&ts->pools[i], /* can_free = */ true);
            PyMem_Free(ts->pools);
        }
    }

    if (nb_thread_state_tls == ts)
        nb_thread_state_tls = nullptr;
    delete ts;
}

// Slow path for nb_thread_state_get(): fetch the state associated with the
// given domain, allocating it with a cleanup callback if needed
nb_thread_state *nb_thread_state_alloc(nb_internals *p) noexcept {
#if defined(_WIN32)
    DWORD key = p->thread_state_key;
    nb_thread_state *ts = (nb_thread_state *) FlsGetValue(key);
    if (!ts) {
        ts = new nb_thread_state();
        ts->internals = p;
        check(FlsSetValue(key, ts), "nanobind: FlsSetValue() failed!");
    }
#else
    pthread_key_t key = p->thread_state_key;
    nb_thread_state *ts = (nb_thread_state *) pthread_getspecific(key);
    if (!ts) {
        ts = new nb_thread_state();
        ts->internals = p;
        check(pthread_setspecific(key, ts) == 0,
              "nanobind: pthread_setspecific() failed!");
    }
#endif
    nb_thread_state_tls = ts;
    return ts;
}
#endif


static const char* interned_c_strs[pyobj_name::string_count] {
    #define NB_INTERNED_ENTRY(name) #name,
    NB_INTERNED_STRINGS(NB_INTERNED_ENTRY)
    #undef NB_INTERNED_ENTRY
};

static void new_constant(nb_internals *p, int index, PyObject *o) {
    p->pyobjects[index] = o;
    new_object(p, o);
}

/// Populate the PyObject cache of the given backend state
static void init_pyobjects(nb_internals *p) {
    if (p->pyobjects[0])
        return;

    NB_NOUNROLL
    for (int i = 0; i < pyobj_name::string_count; ++i)
        new_constant(p, i, PyUnicode_InternFromString(interned_c_strs[i]));

    new_constant(p, pyobj_name::interned_max_version_tpl,
                 PyTuple_Pack(1, NB_INTERNED(p, max_version)));

    PyObject *one = PyLong_FromLong(1), *zero = PyLong_FromLong(0);
    new_constant(p, pyobj_name::interned_dl_cpu_tpl, PyTuple_Pack(2, one, zero));
    Py_DECREF(zero);
    Py_DECREF(one);

    PyObject *major = PyLong_FromLong(dlpack::major_version),
             *minor = PyLong_FromLong(dlpack::minor_version);
    new_constant(p, pyobj_name::interned_dl_version_tpl, PyTuple_Pack(2, major, minor));
    Py_DECREF(minor);
    Py_DECREF(major);

#if defined(Py_LIMITED_API) || defined(PYPY_VERSION)
    // Upper bound of the 'uint64_t' range, see load_int_exact() in common.cpp
    PyObject *u64_max = PyLong_FromUnsignedLongLong(~(unsigned long long) 0),
             *u64_one = PyLong_FromLong(1);
    new_constant(p, pyobj_name::interned_u64_limit, PyNumber_Add(u64_max, u64_one));
    Py_DECREF(u64_one);
    Py_DECREF(u64_max);
#endif
}

#if defined(Py_LIMITED_API)
/// Fill this binary's interpreter constants: cached PyType_Type and
/// PyProperty_Type slots plus the offset of the type_data record within a
/// type object (identical for every nb_internals of one interpreter)
static void init_limited_api_constants(PyTypeObject *nb_meta, PyObject *mod) {
    if (nb_type_data_offset)
        return;

    PyType_Type_tp_free = (freefunc) PyType_GetSlot(&PyType_Type, Py_tp_free);
    PyType_Type_tp_init = (initproc) PyType_GetSlot(&PyType_Type, Py_tp_init);
    PyType_Type_tp_clear = (inquiry) PyType_GetSlot(&PyType_Type, Py_tp_clear);
    PyType_Type_tp_dealloc =
        (destructor) PyType_GetSlot(&PyType_Type, Py_tp_dealloc);
    PyType_Type_tp_setattro =
        (setattrofunc) PyType_GetSlot(&PyType_Type, Py_tp_setattro);
    PyProperty_Type_tp_descr_get =
        (descrgetfunc) PyType_GetSlot(&PyProperty_Type, Py_tp_descr_get);
    PyProperty_Type_tp_descr_set =
        (descrsetfunc) PyType_GetSlot(&PyProperty_Type, Py_tp_descr_set);

    PyType_Slot dummy_slots[] = {
        { Py_tp_base, &PyType_Type },
        { 0, nullptr }
    };

    PyType_Spec dummy_spec = {
        /* .name = */ "nanobind.dummy",
        /* .basicsize = */ - (int) sizeof(void*),
        /* .itemsize = */ 0,
        /* .flags = */ Py_TPFLAGS_DEFAULT,
        /* .slots = */ dummy_slots
    };

    PyObject *dummy = PyType_FromMetaclass(nb_meta, mod, &dummy_spec, nullptr);
    nb_type_data_offset =
        ((uint8_t *) PyObject_GetTypeData(dummy, nb_meta) - (uint8_t *) dummy);
    Py_DECREF(dummy);
}
#endif

/// Create lifeline + internal types if needed
static void init_internals(nb_internals *p) {
    if (p->lifeline) {
#if defined(Py_LIMITED_API)
        init_limited_api_constants(Py_TYPE((PyObject *) p->nb_type),
                                   p->nb_module);
#endif
        return;
    }

    p->lifeline = PyList_New(0);
    check(p->lifeline, "nanobind::detail::nb_module_init(): "
                        "could not create lifeline list!");

    str nb_name("nanobind");
    p->nb_module = PyModule_NewObject(nb_name.ptr());
    new_object(p, p->nb_module);

    // Construct nanobind's meta-meta class
    nb_meta_slots[0].pfunc = (PyObject *) &PyType_Type;
    PyTypeObject *nb_meta = new_type(p, &nb_meta_spec);

    p->nb_func         = new_type(p, &nb_func_spec);
    p->nb_method       = new_type(p, &nb_method_spec);
    p->nb_bound_method = new_type(p, &nb_bound_method_spec);

    check(p->nb_module && nb_meta && p->nb_func &&
              p->nb_method && p->nb_bound_method,
          "nanobind::detail::nb_module_init(): initialization failed!");

#if defined(Py_LIMITED_API)
    init_limited_api_constants(nb_meta, p->nb_module);
#endif

    // Create the single metaclass shared by all bound types. This may
    // access 'nb_type_data_offset' defined just above.
    p->nb_type = nb_type_create_metaclass(p, nb_meta);
    check(p->nb_type, "nanobind::detail::nb_module_init(): "
                      "nb_type metaclass creation failed!");
}

PyObject *import_cached(nb_internals *p, import_cache *c) noexcept {
    // Resolve outside of the lock since the import may need to re-enter nanobind.
    PyObject *mod = PyImport_ImportModule(c->module);
    if (!mod)
        return nullptr;
    PyObject *o = PyObject_GetAttrString(mod, c->attr);
    Py_DECREF(mod);
    if (!o)
        return nullptr;

    lock_internals guard(p);

    if (PyObject *cur = c->load()) { // lost the race
        Py_DECREF(o);
        return cur;
    }

    if (!p->lifeline) {
        Py_DECREF(o);
        PyErr_SetString(PyExc_RuntimeError,
                        "nanobind::detail::import_cached(): the nanobind "
                        "internals are being torn down!");
        return nullptr;
    }

    if (PyList_Append(p->lifeline, o)) {
        Py_DECREF(o);
        return nullptr;
    }
    Py_DECREF(o); // the lifeline now owns the only reference

    p->import_slots.push_back(c);
    c->store(o);
    return o;
}

/// Does the entry describe the current content of 'str'? Check the
/// address and contents.
static bool name_cache_match(const nb_internals::name_cache_entry &e,
                             const char *str, size_t bound) {
    if (bound)
        return e.len < bound && str[e.len] == '\0' &&
               memcmp(e.utf8, str, e.len) == 0;

    for (size_t i = 0; i < e.len; ++i)
        if (e.utf8[i] != str[i])
            return false;
    return str[e.len] == '\0';
}

/// Miss path of cached_string() below; 'slot' and 'vacancy' carry over what
/// the fast path already determined about the key's two-slot set
NB_NOINLINE static PyObject *cached_string_slow(nb_internals *p,
                                                const char *str, size_t bound,
                                                bool *owned, size_t slot,
                                                bool vacancy) noexcept {
    uintptr_t key = (uintptr_t) str;

    // On FT threads, interned strings are immortal. To avoid leaks with
    // runtime-generated values, we only intern strings that are about to
    // enter the cache. This is restricted to literals.
    if (bound == 0 || !vacancy) {
        PyObject *s = PyUnicode_FromString(str);
        *owned = s != nullptr;
        return s;
    }

    PyObject *s = PyUnicode_InternFromString(str);
    if (NB_UNLIKELY(!s))
        return nullptr;
    *owned = true;

    lock_internals guard(p);

    nb_internals::name_cache_entry *free_slot = nullptr;
    for (size_t i = 0; i < 2; ++i) {
        nb_internals::name_cache_entry &e = p->name_cache[slot ^ i];
        PyObject *v = e.value.load_relaxed();
        if (!v) {
            if (!free_slot)
                free_slot = &e;
        } else if (e.key == key && name_cache_match(e, str, bound)) {
            // Lost a race against an identical fill; interning made the
            // strings equal, so ours only holds a redundant reference
            Py_DECREF(s);
            *owned = false;
            return v;
        }
    }

    if (!free_slot)
        return s; // the set filled up concurrently; the reference is owned

    // Hand the string over to the lifeline, whose lifetime bounds that of
    // the cache entry, or signal to the caller that this was not possible.
    Py_ssize_t utf8_len = 0;
    const char *utf8 = PyUnicode_AsUTF8AndSize(s, &utf8_len);
    if (!utf8 || !p->lifeline || PyList_Append(p->lifeline, s) != 0) {
        PyErr_Clear();
        return s;
    }
    Py_DECREF(s); // the lifeline now owns the only reference

    free_slot->key = key;
    free_slot->len = (size_t) utf8_len;
    free_slot->utf8 = utf8;
    free_slot->value.store_release(s);
    *owned = false;
    return s;
}

/// Fast path of the 'cached_string' slot, inlined into the string-keyed
/// object protocol operations below
static NB_INLINE PyObject *cached_string_fast(nb_internals *p,
                                              const char *str, size_t bound,
                                              bool *owned) noexcept {
    uintptr_t key = (uintptr_t) str;

    // Related string addresses differ mostly in their low bits. Apply a
    // Fibonacci hash-based mixing step to spread this across the table.
    size_t slot = (size_t) ((key * 0x9E3779B97F4A7C15ull) >>
                            (64 - nb_internals::name_cache_bits));
    *owned = false;

    bool vacancy = false;
    // Probe the entry and its neighbor
    for (size_t i = 0; i < 2; ++i) {
        nb_internals::name_cache_entry &e = p->name_cache[slot ^ i];
        PyObject *v = e.value.load_acquire();
        if (NB_LIKELY(v && e.key == key && name_cache_match(e, str, bound)))
            return v;
        vacancy |= !v;
    }

    return cached_string_slow(p, str, bound, owned, slot, vacancy);
}

PyObject *cached_string(nb_internals *p, const char *str, size_t bound,
                        bool *owned) noexcept {
    return cached_string_fast(p, str, bound, owned);
}

/// Python string key of the string-keyed object protocol operations below
/// (interned when it could be cached); raises if the conversion fails
struct py_key {
    PyObject *value;
    bool owned;

    py_key(nb_internals *p, const char *str, size_t bound)
        : value(cached_string_fast(p, str, bound, &owned)) {
        if (NB_UNLIKELY(!value))
            raise_python_error();
    }
    py_key(const py_key &) = delete;
    ~py_key() {
        if (NB_UNLIKELY(owned))
            Py_DECREF(value);
    }
};

PyObject *getattr_str(nb_internals *p, PyObject *obj, const char *str,
                      size_t bound) {
    py_key key(p, str, bound);
    return getattr(obj, key.value);
}

PyObject *getattr_def(PyObject *obj, PyObject *key, PyObject *def) noexcept {
    return getattr(obj, key, def);
}

PyObject *getattr_str_def(nb_internals *p, PyObject *obj, const char *str,
                          size_t bound, PyObject *def) noexcept {
    bool owned;
    PyObject *key = cached_string_fast(p, str, bound, &owned);
    if (NB_UNLIKELY(!key)) {
        PyErr_Clear();
        return Py_XNewRef(def);
    }
    PyObject *res = getattr(obj, key, def);
    if (NB_UNLIKELY(owned))
        Py_DECREF(key);
    return res;
}

void setattr_str(nb_internals *p, PyObject *obj, const char *str,
                 size_t bound, PyObject *value) {
    py_key key(p, str, bound);
    setattr(obj, key.value, value);
}

void delattr_str(nb_internals *p, PyObject *obj, const char *str,
                 size_t bound) {
    py_key key(p, str, bound);
    delattr(obj, key.value);
}

PyObject *dict_getitem_str(nb_internals *p, PyObject *obj, const char *str,
                           size_t bound) {
    py_key key(p, str, bound);
    bool error;
    PyObject *value = dict_getitem_ref(obj, key.value, &error);
    if (NB_UNLIKELY(!value))
        error ? raise_python_error() : raise_key_error(key.value);
    return value;
}

void dict_setitem_str(nb_internals *p, PyObject *obj, const char *str,
                      size_t bound, PyObject *value) {
    py_key key(p, str, bound);
    raise_if_nonzero(PyDict_SetItem(obj, key.value, value));
}

void dict_delitem_str(nb_internals *p, PyObject *obj, const char *str,
                      size_t bound) {
    py_key key(p, str, bound);
    raise_if_nonzero(PyDict_DelItem(obj, key.value));
}

// The three operations below take the dictionary path above when 'obj' is an
// exact dictionary, for which the two are equivalent. Subclasses stay on the
// object protocol, where an overridden '__getitem__' and the '__missing__'
// hook remain visible.

PyObject *getitem_str(nb_internals *p, PyObject *obj, const char *str,
                      size_t bound) {
    if (PyDict_CheckExact(obj))
        return dict_getitem_str(p, obj, str, bound);
    py_key key(p, str, bound);
    return raise_if_null(PyObject_GetItem(obj, key.value));
}

void setitem_str(nb_internals *p, PyObject *obj, const char *str,
                 size_t bound, PyObject *value) {
    if (PyDict_CheckExact(obj))
        return dict_setitem_str(p, obj, str, bound, value);
    py_key key(p, str, bound);
    setitem(obj, key.value, value);
}

void delitem_str(nb_internals *p, PyObject *obj, const char *str,
                 size_t bound) {
    if (PyDict_CheckExact(obj))
        return dict_delitem_str(p, obj, str, bound);
    py_key key(p, str, bound);
    delitem(obj, key.value);
}

bool contains_str(nb_internals *p, PyObject *obj, const char *str,
                  size_t bound) {
    py_key key(p, str, bound);
    int rv;
    if (PyDict_CheckExact(obj))
        rv = PyDict_Contains(obj, key.value);
    else
        rv = PySequence_Contains(obj, key.value);
    if (NB_UNLIKELY(rv == -1))
        raise_python_error();
    return rv == 1;
}

bool hasattr_str(nb_internals *p, PyObject *obj, const char *str,
                 size_t bound) noexcept {
    bool owned;
    PyObject *key = cached_string_fast(p, str, bound, &owned);
    if (NB_UNLIKELY(!key)) {
        PyErr_Clear();
        return false;
    }
    bool rv = PyObject_HasAttr(obj, key) == 1;
    if (NB_UNLIKELY(owned))
        Py_DECREF(key);
    return rv;
}

PyObject *type_lookup_str(nb_internals *p, PyObject *t, const char *str,
                          size_t bound) noexcept {
    bool owned;
    PyObject *key = cached_string_fast(p, str, bound, &owned);
    if (NB_UNLIKELY(!key)) {
        PyErr_Clear();
        return nullptr;
    }
    PyObject *rv = type_lookup(p, t, key);
    if (NB_UNLIKELY(owned))
        Py_DECREF(key);
    return rv;
}

void internals_inc_ref(nb_internals *p) {
    p->shared_ref_count.value++;
}

/// This function clears and internal nanobind types and the lifeline list
/// during interpreter shutdown. It aggressively calls ``tp_clear`` on the
/// types. CPython does not run enough GC passes to collect all reference cycles
/// involving these objects at shutdown. This helps achieve a leak-free
/// shutdown.
static void internals_release_types(nb_internals *p) {
    inquiry clear = NB_TYPE_SLOT(PyType_Type, tp_clear);
    if (!clear || !p->nb_type) {
        Py_CLEAR(p->lifeline);
        return;
    }

    PyObject *types[] = {
        (PyObject *) p->nb_type,
        (PyObject *) Py_TYPE((PyObject *) p->nb_type), // nb_meta
        (PyObject *) p->nb_func,
        (PyObject *) p->nb_method,
        (PyObject *) p->nb_bound_method,
        (PyObject *) p->nb_static_property.load_relaxed(),
        (PyObject *) p->nb_ndarray.load_relaxed()
    };

    // Keep every type alive while the lifeline and the cycles go away
    for (PyObject *t : types)
        Py_XINCREF(t);
    Py_CLEAR(p->lifeline);
    for (PyObject *t : types) {
        if (t)
            clear(t);
    }
    for (PyObject *t : types)
        Py_XDECREF(t);
}

void internals_dec_ref(nb_internals *p) {
    auto value = --p->shared_ref_count.value;
    if (value != 0)
        return;

    internals_release_types(p);

    p->nb_module = nullptr;
    p->nb_type = nullptr;
    p->nb_func = nullptr;
    p->nb_method = nullptr;
    p->nb_bound_method = nullptr;
    p->nb_static_property.store_release(nullptr);
    p->nb_ndarray.store_release(nullptr);
    for (auto &entry : p->ndarray_export)
        entry.store_release(nullptr);

    for (int i = 0; i < pyobj_name::total_count; ++i)
        p->pyobjects[i] = nullptr;

    for (import_cache *c : p->import_slots)
        c->store(nullptr);
    p->import_slots.clear();

    // The cached interned strings die with the lifeline
    for (auto &e : p->name_cache)
        e.value.store_release(nullptr);
}

static void nb_module_free(void *m) {
    // The state holds the domain registered by nb_module_init(); it is
    // still zero when the module was created but never executed.
    void *state = PyModule_GetState((PyObject *) m);
    nb_internals *p = state ? *(nb_internals **) state : nullptr;
    if (p)
        internals_dec_ref(p);
}

/// Dictionary key of the capsule anchoring a foreign module's domain
static const char *nb_anchor_name = "__nanobind_internals__";

static void nb_anchor_free(PyObject *capsule) {
    nb_internals **state =
        (nb_internals **) PyCapsule_GetPointer(capsule, nb_anchor_name);
    if (!state) {
        PyErr_Clear();
        return;
    }
    if (*state)
        internals_dec_ref(*state);
    delete state;
}

/* Modules created by module_new() record their domain in the per-module state.
   Foreign modules registered via nanobind::register_module() have no such
   state, and the equivalent storage is anchored in a capsule owned by the
   module dictionary. */
static nb_internals **nb_module_anchor(PyObject *m) {
    PyObject *dict = PyModule_GetDict(m);
    if (!dict) {
        PyErr_SetString(PyExc_SystemError,
                        "nanobind::detail::nb_module_init(): could not access "
                        "the module dictionary!");
        return nullptr;
    }

    PyObject *key = PyUnicode_FromString(nb_anchor_name);
    if (!key)
        return nullptr;

    PyObject *capsule = dict_getitem_or_default(dict, key, nullptr);
    if (!capsule) {
        nb_internals **state = new nb_internals *(nullptr);
        capsule = PyCapsule_New(state, nb_anchor_name, nb_anchor_free);
        if (!capsule) {
            delete state;
            Py_DECREF(key);
            return nullptr;
        }

        // A concurrent registration of the same module could have won the race
        PyObject *found;
#if PY_VERSION_HEX >= 0x030D0000 && !defined(Py_LIMITED_API) && \
    !defined(PYPY_VERSION)
        found = nullptr;
        int rv = PyDict_SetDefaultRef(dict, key, capsule, &found);
        if (rv < 0)
            Py_CLEAR(found);
#else
        // The GIL prevents interleaving between the recheck and the insertion
        found = dict_getitem_or_default(dict, key, nullptr);
        if (!found && PyDict_SetItem(dict, key, capsule) == 0)
            found = Py_NewRef(capsule);
#endif
        Py_DECREF(capsule);
        capsule = found;
    }

    Py_DECREF(key);
    if (!capsule)
        return nullptr;

    nb_internals **state =
        (nb_internals **) PyCapsule_GetPointer(capsule, nb_anchor_name);
    Py_DECREF(capsule);
    return state;
}

// 'flags' holds the ABI tag, which nothing reads yet
PyObject *module_new(const char *name, const char *doc, void *exec,
                     uint32_t) noexcept {
    PyModuleDef_Slot *s =
        (PyModuleDef_Slot *) PyMem_Calloc(4, sizeof(PyModuleDef_Slot));
    PyModuleDef *d = (PyModuleDef *) PyMem_Calloc(1, sizeof(PyModuleDef));
    if (!s || !d) {
        PyMem_Free(s);
        PyMem_Free(d);
        PyErr_NoMemory();
        return nullptr;
    }

    size_t i = 0;
    s[i++] = { Py_mod_exec, exec };
#if defined(NB_FREE_THREADED)
    s[i++] = { Py_mod_gil, Py_MOD_GIL_NOT_USED };
#endif
#if PY_VERSION_HEX >= 0x030C0000
    s[i++] = { Py_mod_multiple_interpreters,
               Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED };
#endif

    PyModuleDef_Base base = PyModuleDef_HEAD_INIT;
    d->m_base = base;
    d->m_name = name;
    d->m_doc = doc;
    d->m_slots = s;
    // Per-module state recording the domain, filled in by nb_module_init()
    d->m_size = (Py_ssize_t) sizeof(nb_internals *);
    d->m_free = nb_module_free;
    return PyModuleDef_Init(d);
}


static bool is_alive_value = false;
static bool *is_alive_ptr = &is_alive_value;
bool is_alive() noexcept { return *is_alive_ptr; }


#if defined(NB_HAVE_INTERP_VIEW)
PyInterpreterView *nb_interp_view = nullptr;

// nb::gil_scoped_acquire is usable as soon as an application links against
// nanobind, which an embedding application may do before it imports the
// extension that publishes 'nb_interp_view'. Such callers get a throw-away
// view instead.
NB_NOINLINE void *attach_tstate_early() noexcept {
    PyInterpreterView *view = PyInterpreterView_FromMain();
    if (!view)
        return nullptr;
    void *token = PyThreadState_EnsureFromView(view);
    PyInterpreterView_Close(view);
    return token;
}
#endif

void *tstate_ensure() noexcept { return attach_tstate(); }

void tstate_release(void *token) noexcept {
    if (token)
        detach_tstate(token);
}


NB_NOINLINE static void internals_cleanup_one(nb_internals *p) {
    (void) p;
#if !defined(PYPY_VERSION) && !defined(NB_FREE_THREADED)
    // The memory leak checker is unsupported on PyPy, see
    // see https://foss.heptapod.net/pypy/pypy/-/issues/3855.
    //
    // Leak reporting is explicitly disabled on free-threaded builds
    // for now because of the decision to immortalize function and
    // type objects. This may change in the future.

    bool print_leak_warnings = p->print_leak_warnings;

    // Unmap pooled instances to avoid false leaks (can_free=false: no thread state here).
    for (const auto &kv : p->type_c2p_slow) {
        type_data *td = kv.second;
        if (td->flags & (uint32_t) type_flags::pooled)
            nb_pool_drain(&td->pool, /* can_free = */ false);
    }

    size_t inst_leaks = 0, keep_alive_leaks = 0;

    // Shard locking no longer needed, Py_AtExit is single-threaded
    for (size_t i = 0; i < p->shard_count; ++i) {
        nb_shard &s = p->shards[i];
        inst_leaks += s.inst_c2p.size();
        keep_alive_leaks += s.keep_alive.size();
    }

#ifdef _DEBUG
// in debug mode, show all leak records
#define INC_CTR do {} while(0)
#else
// otherwise show just the first 10 or 20
#define INC_CTR ctr++
#endif

    bool leak = inst_leaks > 0 || keep_alive_leaks > 0;

    if (print_leak_warnings && inst_leaks > 0) {
        fprintf(stderr, "nanobind: leaked %zu instances!\n", inst_leaks);

#if !defined(Py_LIMITED_API)
        auto print_leak = [](void* k, PyObject* v) {
            type_data *tp = nb_type_data(Py_TYPE(v));
            fprintf(stderr, " - leaked instance %p of type \"%s\"\n", k, tp->name);
        };

        int ctr = 0;
        for (size_t i = 0; i < p->shard_count && ctr < 20; ++i) {
            for (auto [k, v]: p->shards[i].inst_c2p) {
                if (NB_UNLIKELY(nb_is_seq(v))) {
                    nb_inst_seq* seq = nb_get_seq(v);
                    for(; seq != nullptr && ctr < 20; seq = seq->next) {
                        print_leak(k, seq->inst);
                        INC_CTR;
                    }
                } else {
                    print_leak(k, (PyObject*)v);
                    INC_CTR;
                }
                if (ctr >= 20)
                    break;
            }
        }
        if (ctr >= 20) {
            fprintf(stderr, " - ... skipped remainder\n");
        }
#endif
    }

    if (print_leak_warnings && keep_alive_leaks > 0)
        fprintf(stderr, "nanobind: leaked %zu keep_alive records!\n",
                keep_alive_leaks);

    // Only report function/type leaks if actual nanobind instances were leaked
#if !defined(NB_ABORT_ON_LEAK)
    if (!leak)
        print_leak_warnings = false;
#endif

    if (!p->type_c2p_slow.empty()) {
        if (print_leak_warnings) {
            fprintf(stderr, "nanobind: leaked %zu types!\n",
                    p->type_c2p_slow.size());
            int ctr = 0;
            for (const auto &kv : p->type_c2p_slow) {
                fprintf(stderr, " - leaked type \"%s\"\n", kv.second->name);
                INC_CTR;
                if (ctr == 10) {
                    fprintf(stderr, " - ... skipped remainder\n");
                    break;
                }
            }
        }
        leak = true;
    }

    if (!p->funcs.empty()) {
        if (print_leak_warnings) {
            fprintf(stderr, "nanobind: leaked %zu functions!\n",
                    p->funcs.size());
            int ctr = 0;
            for (auto [f, p2] : p->funcs) {
                fprintf(stderr, " - leaked function \"%s\"\n",
                        nb_func_data(f)->name);
                INC_CTR;
                if (ctr == 10) {
                    fprintf(stderr, " - ... skipped remainder\n");
                    break;
                }
            }
        }
        leak = true;
    }

    if (!leak) {
        nb_translator_seq* t = p->translators.load_relaxed();
        while (t) {
            nb_translator_seq *next = t->next;
            delete t;
            t = next;
        }

#if defined(NB_FREE_THREADED)
        // This code won't run for now but is kept here for a time when
        // immortalization isn't needed anymore.

        delete[] p->shards;
#endif

        delete p;
    } else {
        if (print_leak_warnings) {
            fprintf(stderr, "nanobind: this is likely caused by a reference "
                            "counting issue in the binding code.\n"
                            "See https://nanobind.readthedocs.io/en/latest/refleaks.html");
        }

        #if defined(NB_ABORT_ON_LEAK) && !defined(NB_FREE_THREADED)
            abort(); // Extra-strict behavior for the CI server
        #endif
    }
#endif
}

static void internals_cleanup() {
    is_alive_value = false;
    *is_alive_ptr = false;

    for (nb_internals *p : internals_created)
        internals_cleanup_one(p);
    internals_created.clear();
}

/// Adopt the record referenced by 'capsule' (consumed) and register the
/// module as one of its users
static nb_internals *internals_join(PyObject *capsule,
                                    nb_internals **mod_state) {
    nb_internals *p =
        (nb_internals *) PyCapsule_GetPointer(capsule, "nb_internals");
    Py_DECREF(capsule);
    if (!p)
        return nullptr;

    is_alive_ptr = p->is_alive_ptr;

    init_internals(p);
    init_pyobjects(p);

    // A module that is executed more than once (importlib.reload) counts once
    if (*mod_state != p) {
        *mod_state = p;
        internals_inc_ref(p);
    }
    return p;
}

/// Discard a freshly created record whose publication failed or that lost the
/// race against a concurrent import of another extension of the same domain
static void internals_discard(nb_internals *p) noexcept {
    // Release the record's Python objects via the refcounted teardown path
    internals_inc_ref(p);
    internals_dec_ref(p);

    nb_translator_seq *t = p->translators.load_relaxed();
    while (t) {
        nb_translator_seq *next = t->next;
        delete t;
        t = next;
    }

#if defined(NB_FREE_THREADED)
    delete[] p->shards;
#  if defined(_WIN32)
    FlsFree(p->thread_state_key);
#  else
    pthread_key_delete(p->thread_state_key);
#  endif
#endif
    delete p;
}

static nb_internals *nb_module_init_impl(const char *domain, PyObject *m) {
#if defined(NB_HAVE_INTERP_VIEW)
    // Needed by every later attach_tstate() call, including those of
    // extensions that reuse an already initialized 'nb_internals'
    if (!nb_interp_view) {
        nb_interp_view = PyInterpreterView_FromMain();
        if (!nb_interp_view) {
            PyErr_NoMemory();
            return nullptr;
        }
    }
#endif

    if (!m || !PyModule_Check(m)) {
        PyErr_SetString(PyExc_SystemError,
                        "nanobind::detail::nb_module_init(): expected a "
                        "module object!");
        return nullptr;
    }

    // Storage recording the module's domain. The 'm_free' handler identifies
    // modules created by module_new(), which reserve per-module state for it.
    PyModuleDef *def = PyModule_GetDef(m);
    if (!def)
        PyErr_Clear(); // PyModule_New() creates modules without a definition

    nb_internals **mod_state;
    if (def && def->m_free == nb_module_free) {
        mod_state = (nb_internals **) PyModule_GetState(m);
        if (!mod_state) {
            PyErr_SetString(PyExc_SystemError,
                            "nanobind::detail::nb_module_init(): the module "
                            "object does not carry nanobind state!");
            return nullptr;
        }
    } else {
        mod_state = nb_module_anchor(m);
        if (!mod_state)
            return nullptr;
    }

#if defined(PYPY_VERSION)
    PyObject *dict = PyEval_GetBuiltins();
#else
    PyObject *dict = PyInterpreterState_GetDict(PyInterpreterState_Get());
#endif
    if (!dict) {
        PyErr_SetString(PyExc_SystemError,
                        "nanobind: could not access the internals dictionary!");
        return nullptr;
    }

    // Backend binaries in one process share the state of a domain exactly
    // when their keys match
    PyObject *key = PyUnicode_FromFormat("__nb_internals_%s_%s__",
                                         NB_INTERNALS_KEY, domain);
    if (!key)
        return nullptr;

    PyObject *capsule = dict_getitem_or_default(dict, key, nullptr);
    if (capsule) {
        Py_DECREF(key);
        return internals_join(capsule, mod_state);
    }

    nb_internals *p = new nb_internals();

    size_t shard_count = 1;
#if defined(NB_FREE_THREADED)
    size_t hw_concurrency = std::thread::hardware_concurrency();
    while (shard_count < hw_concurrency)
        shard_count *= 2;
    shard_count *= 2;
    p->shards = new nb_shard[shard_count];
    p->shard_mask = shard_count - 1;

    // Per-domain key for reclaiming nb_thread_state at thread exit
#if defined(_WIN32)
    p->thread_state_key = FlsAlloc((PFLS_CALLBACK_FUNCTION) nb_thread_state_destroy);
    check(p->thread_state_key != FLS_OUT_OF_INDEXES, "nanobind: FlsAlloc() failed!");
#else
    check(pthread_key_create(&p->thread_state_key, nb_thread_state_destroy) == 0,
          "nanobind: pthread_key_create() failed!");
#endif
#endif
    p->shard_count = shard_count;

    init_internals(p);
    init_pyobjects(p);

    p->translators.store_release(
        new nb_translator_seq{ default_exception_translator, nullptr, nullptr });

    is_alive_value = true;
    is_alive_ptr = &is_alive_value;
    p->is_alive_ptr = is_alive_ptr;

#if !defined(PYPY_VERSION)
    // typing.py on CPython introduces spurious reference leaks that upset
    // nanobind's leak checker. The following band-aid installs an 'atexit'
    // handler that clears LRU caches used in typing.py. To be resilient to
    // potential future changes in typing.py, the implementation fails silently
    // if any step goes wrong. For context, see
    // https://github.com/python/cpython/issues/98253 and
    // https://github.com/python/cpython/issues/151728. */

    const char *str =
        "def cleanup():\n"
        "    try:\n"
        "        import sys\n"
        "        fs = getattr(sys.modules.get('typing'), '_cleanups', None)\n"
        "        if fs is not None:\n"
        "            for f in fs:\n"
        "                f()\n"
        "    except:\n"
        "        pass\n"
        "import atexit\n"
        "atexit.register(cleanup)\n"
        "del atexit, cleanup";

    PyObject *code = Py_CompileString(str, "<internal>", Py_file_input);
    if (code) {
        PyObject *globals = PyDict_New();
        if (globals) {
            PyObject *result = PyEval_EvalCode(code, globals, nullptr);
            if (!result)
                PyErr_Clear();
            Py_XDECREF(result);
            Py_DECREF(globals);
        } else {
            PyErr_Clear();
        }
        Py_DECREF(code);
    } else {
        PyErr_Clear();
    }
#endif

    capsule = PyCapsule_New(p, "nb_internals", nullptr);
    if (!capsule) {
        Py_DECREF(key);
        internals_discard(p);
        return nullptr;
    }

    /* Publish the record unless a concurrent import of another extension of
       this domain published one in the meantime: the initialization above
       runs Python code, and free-threaded builds do not serialize the
       execution of distinct modules at all. */
    PyObject *found;
    int rv;
#if PY_VERSION_HEX >= 0x030D0000 && !defined(Py_LIMITED_API) && \
    !defined(PYPY_VERSION)
    found = nullptr;
    rv = PyDict_SetDefaultRef(dict, key, capsule, &found);
    if (rv == 0)
        Py_CLEAR(found); // references our own capsule
#else
    // The GIL prevents interleaving between the recheck and the insertion
    found = dict_getitem_or_default(dict, key, nullptr);
    rv = found ? 1 : PyDict_SetItem(dict, key, capsule);
#endif
    Py_DECREF(capsule);
    Py_DECREF(key);

    if (NB_UNLIKELY(rv != 0)) {
        internals_discard(p);
        if (rv < 0)
            return nullptr;
        return internals_join(found, mod_state); // lost the race, adopt
    }

    // Track the published record for the exit-time sweep
    {
#if defined(NB_FREE_THREADED)
        PyMutex_Lock(&internals_created_mutex);
#endif
        bool need_atexit = internals_created.empty();
        internals_created.push_back(p);
#if defined(NB_FREE_THREADED)
        PyMutex_Unlock(&internals_created_mutex);
#endif
        if (need_atexit && Py_AtExit(internals_cleanup))
            fprintf(stderr,
                    "Warning: could not install the nanobind cleanup handler! This "
                    "is needed to check for reference leaks and release remaining "
                    "resources at interpreter shutdown (e.g., to avoid leaks being "
                    "reported by tools like 'valgrind'). If you are a user of a "
                    "python extension library, you can ignore this warning.");
    }

    *mod_state = p;
    internals_inc_ref(p);
    return p;
}

NB_NOINLINE nb_internals *nb_module_init(const char *domain,
                                         PyObject *m) noexcept {
    // Fill libnanobind's copy of the cache. Extensions fill their own in PyInit_.
    init_singletons();

    try {
        return nb_module_init_impl(domain, m);
    } catch (python_error &e) {
        e.restore();
    } catch (const std::bad_alloc &) {
        PyErr_NoMemory();
    } catch (const std::exception &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    } catch (...) {
        PyErr_SetString(PyExc_RuntimeError,
                        "nanobind: unknown initialization failure!");
    }
    return nullptr;
}

#if defined(NB_COMPACT_ASSERTIONS)
NB_NOINLINE void fail_unspecified() noexcept {
    #if defined(NB_COMPACT_ASSERTION_MESSAGE)
        fail(NB_COMPACT_ASSERTION_MESSAGE);
    #else
        fail("encountered an unrecoverable error condition. Recompile using the"
             " 'Debug' mode to obtain further information about this problem.");
    #endif
}
#endif

// Statically initialized boundary function table
[[maybe_unused]] static const nb_backend_table nb_backend_export = {
    nb_backend_slot_count, NB_BACKEND_ABI_MINOR, { 0 },
#define NB_SLOT(ret, name, args) nanobind::detail::name,
#include <nanobind/nb_backend_slots.h>
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
