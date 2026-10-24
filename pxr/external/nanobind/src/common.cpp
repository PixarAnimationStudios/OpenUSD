/*
    src/common.cpp: miscellaneous libnanobind functionality

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/nanobind.h>
#include <memory>
#include "nb_internals.h"

#if defined(_MSC_VER)
#  pragma warning(disable: 6255) // _alloca indicates failure by raising a stack overflow exception
#endif

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/* Note that this runs without an attached thread state in some cases (e.g.
   when a trampoline gives up because the interpreter is shutting down), hence
   the use of malloc() over PyMem_Malloc() for oversized messages. */
NB_NOINLINE static builtin_exception
create_exception(exception_type type, const char *fmt, va_list args_) {
    char buf[512];
    va_list args;

    va_copy(args, args_);
    size_t size = (size_t) vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (size < sizeof(buf)) {
        return builtin_exception(type, buf);
    } else {
        std::unique_ptr<char[]> temp(new (std::nothrow) char[size + 1]);
        if (!temp)
            return builtin_exception(type, buf); // Fall back to a truncation

        va_copy(args, args_);
        vsnprintf(temp.get(), size + 1, fmt, args);
        va_end(args);

        return builtin_exception(type, temp.get());
    }
}

void raise_v(exception_type type, const char *fmt, va_list args) {
    throw create_exception(type, fmt, args);
}

void fail(const char *fmt, ...) noexcept {
    va_list args;
    fprintf(stderr, "Critical nanobind error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    abort();
}

/// Called by the 'noexcept' routines that declare bindings: an operation that
/// raises there means that the extension itself is fundamentally broken
void fail_exception(const char *context, const char *name) noexcept {
    try {
        throw;
    } catch (const std::exception &e) {
        fail("%s(\"%s\"): %s", context, name, e.what());
    } catch (...) {
        fail("%s(\"%s\"): an unknown exception occurred!", context, name);
    }
}

// ========================================================================

PyObject *submodule_new(nb_internals *, PyObject *base, const char *name,
                        const char *doc) noexcept {
    const char *base_name, *tmp_str;
    Py_ssize_t tmp_size = 0;
    object tmp, res;

    base_name = PyModule_GetName(base);
    if (!base_name)
        goto fail;

    tmp = steal(PyUnicode_FromFormat("%s.%s", base_name, name));
    if (!tmp.is_valid())
        goto fail;

    tmp_str = PyUnicode_AsUTF8AndSize(tmp.ptr(), &tmp_size);
    if (!tmp_str)
        goto fail;

#if NB_PYTHON_VERSION < 0x030D0000
    res = borrow(PyImport_AddModule(tmp_str));
#else
    res = steal(PyImport_AddModuleRef(tmp_str));
#endif

    if (!res.is_valid())
        goto fail;

    if (doc) {
        tmp = steal(PyUnicode_FromString(doc));
        if (!tmp.is_valid())
            goto fail;
        if (PyObject_SetAttrString(res.ptr(), "__doc__", tmp.ptr()))
            goto fail;
    }

    if (PyModule_AddObjectRef(base, name, res.ptr()))
        goto fail;

    return res.release().ptr();

fail:
    raise_python_error();
}

// ========================================================================

size_t len_hint(nb_internals *p, PyObject *o) noexcept {
    (void) p;
#if !defined(Py_LIMITED_API)
    Py_ssize_t res = PyObject_LengthHint(o, 0);
    if (res < 0) {
        PyErr_Clear();
        res = 0;
    }
    return (size_t) res;
#else
    PyTypeObject *tp = Py_TYPE(o);
    lenfunc l = (lenfunc) type_get_slot(tp, Py_sq_length);
    if (!l)
        l = (lenfunc) type_get_slot(tp, Py_mp_length);

    if (l) {
        Py_ssize_t res = l(o);
        if (res < 0) {
            PyErr_Clear();
            res = 0;
        }
        return (size_t) res;
    }

    try {
        object fn = steal(getattr(o, NB_INTERNED(p, __length_hint__)));
        object hint = obj_call(p, fn);
        uint64_t result = 0;
        if (load_u64(p, hint.ptr(), 0, &result) &&
            result <= (uint64_t) SIZE_MAX)
            return (size_t) result;
    } catch (...) { }
    return 0;
#endif
}

// ========================================================================

PyObject *obj_vectorcall(nb_internals *, PyObject *base,
                         PyObject *const *args, size_t nargsf,
                         uint64_t owned, uint32_t flags) {
    size_t nargs = (size_t) PyVectorcall_NARGS(nargsf);
    PyObject *res = nullptr;
    bool cast_error = !base;

    for (size_t i = 0; i < nargs; ++i)
        cast_error |= !args[i];

    if (!cast_error)
        res = (flags & (uint32_t) call_flags::method
                   ? PyObject_VectorcallMethod
                   : PyObject_Vectorcall)(base, args, nargsf, nullptr);

    // Calls that pass every argument as a borrowed pointer skip this pass
    if (owned || nargs > 64) {
        for (size_t i = 0; i < nargs; ++i) {
            if (i >= 64 || (owned & ((uint64_t) 1 << i)))
                Py_XDECREF(args[i]);
        }
    }

    if (flags & (uint32_t) call_flags::base_owned)
        Py_XDECREF(base);

    if (!res) {
        if (cast_error)
            raise_python_or_cast_error();
        else
            raise_python_error();
    }

    return res;
}

/// Turn the operand of a '**' expansion into a private dict, with the error
/// message that CPython uses for the equivalent Python syntax
static PyObject *kwargs_dict(PyObject *value) {
    PyObject *dict = PyDict_New();
    if (dict && PyDict_Update(dict, value) < 0) {
        Py_CLEAR(dict);
        if (PyErr_ExceptionMatches(PyExc_AttributeError))
            PyErr_SetString(PyExc_TypeError,
                            "argument after ** must be a mapping");
    }
    return dict;
}

PyObject *obj_vectorcall_ex(nb_internals *, PyObject *base, call_arg *args,
                            size_t n, uint32_t flags) {
    PyObject *res = nullptr, *kwnames = nullptr, **stack, **pos, **kw;
    size_t nargs = 0, nkwargs = 0, nkw = 0;
    bool cast_error = !base;

    // Pass 1: turn expansion operands into tuples and private dicts (as CPython
    // does before CALL_FUNCTION_EX) and count. Only the conversions can run
    // Python code, and they cannot change the size of the entries before them.
    for (size_t i = 0; i < n && !cast_error; ++i) {
        call_arg &a = args[i];
        bool star = a.kind == call_arg_kind::args;

        if (!a.value || (a.kind == call_arg_kind::keyword && !a.name)) {
            cast_error = true;
        } else if (a.kind == call_arg_kind::positional) {
            nargs++;
        } else if (a.kind == call_arg_kind::keyword) {
            nkwargs++;
        } else {
            PyObject *tmp = star ? PySequence_Tuple(a.value) : kwargs_dict(a.value);
            if (!tmp)
                goto cleanup;
            Py_DECREF(a.value);
            a.value = tmp;
            if (star)
                nargs += (size_t) NB_TUPLE_GET_SIZE(tmp);
            else
                nkwargs += (size_t) NB_DICT_GET_SIZE(tmp);
        }
    }
    if (cast_error)
        goto cleanup;

    // Pass 2: fill the stack, whose entries borrow from the 'call_arg' array
    // and the containers created above, and 'kwnames'. 'stack[0]' is the
    // writable slot required by PEP 590.
    stack = (PyObject **) alloca((nargs + nkwargs + 1) * sizeof(PyObject *));
    stack[0] = nullptr;
    pos = stack + 1;
    kw = pos + nargs;

    if (nkwargs && !(kwnames = PyTuple_New((Py_ssize_t) nkwargs)))
        goto cleanup;

    for (size_t i = 0; i < n; ++i) {
        call_arg &a = args[i];
        switch (a.kind) {
            case call_arg_kind::positional:
                *pos++ = a.value;
                break;

            case call_arg_kind::keyword: // 'kwnames' takes over the name
                *kw++ = a.value;
                NB_TUPLE_SET_ITEM(kwnames, (Py_ssize_t) nkw++, a.name);
                a.name = nullptr;
                break;

            case call_arg_kind::args:
                for (Py_ssize_t j = 0, l = NB_TUPLE_GET_SIZE(a.value); j < l; ++j)
                    *pos++ = NB_TUPLE_GET_ITEM(a.value, j);
                break;

            case call_arg_kind::kwargs: {
                PyObject *key, *value;
                Py_ssize_t p = 0;
                while (PyDict_Next(a.value, &p, &key, &value)) {
                    if (!PyUnicode_Check(key)) {
                        PyErr_SetString(PyExc_TypeError, "keywords must be strings");
                        goto cleanup;
                    }
                    NB_TUPLE_SET_ITEM(kwnames, (Py_ssize_t) nkw++, Py_NewRef(key));
                    *kw++ = value;
                }
                break;
            }
        }
    }

    res = (flags & (uint32_t) call_flags::method
               ? PyObject_VectorcallMethod
               : PyObject_Vectorcall)(base, stack + 1,
                                      nargs | PY_VECTORCALL_ARGUMENTS_OFFSET,
                                      kwnames);

cleanup:
    Py_XDECREF(kwnames);
    for (size_t i = 0; i < n; ++i) {
        Py_XDECREF(args[i].value);
        Py_XDECREF(args[i].name);
    }
    if (flags & (uint32_t) call_flags::base_owned)
        Py_XDECREF(base);

    if (!res) {
        if (cast_error)
            raise_python_or_cast_error();
        else
            raise_python_error();
    }

    return res;
}


// ========================================================================

template <bool IsTuple>
static PyObject *seq_new_impl(PyObject **items, size_t n) noexcept {
    PyObject *result = IsTuple ? PyTuple_New((Py_ssize_t) n)
                               : PyList_New((Py_ssize_t) n);
    size_t i = 0;

    if (NB_UNLIKELY(!result))
        goto fail;

    for (; i < n; ++i) {
        if (NB_UNLIKELY(!items[i]))
            goto fail;

        if constexpr (IsTuple)
            NB_TUPLE_SET_ITEM(result, (Py_ssize_t) i, items[i]);
        else
            NB_LIST_SET_ITEM(result, (Py_ssize_t) i, items[i]);
    }

    return result;

fail:
    Py_XDECREF(result);
    for (; i < n; ++i)
        Py_XDECREF(items[i]);
    return nullptr;
}

PyObject *tuple_new(PyObject **items, size_t n) noexcept {
    return seq_new_impl<true>(items, n);
}

PyObject *list_new(PyObject **items, size_t n) noexcept {
    return seq_new_impl<false>(items, n);
}


#if defined(Py_LIMITED_API) || defined(PYPY_VERSION)
/// Scratch builder whose entries follow the header in the same allocation
struct seq_scratch {
    size_t n;
    bool is_tuple;
    PyObject **items;
};
#endif

template <bool IsTuple>
static void *seq_alloc_impl(size_t n, PyObject ***items) noexcept {
#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)
    PyObject *result;
    PyObject **storage = nullptr;

    if constexpr (IsTuple) {
        result = PyTuple_New((Py_ssize_t) n);
        // The empty tuple is a singleton, don't share its pointer
        if (result && n)
            storage = ((PyTupleObject *) result)->ob_item;
    } else {
        result = PyList_New((Py_ssize_t) n);
        if (result)
            storage = ((PyListObject *) result)->ob_item;
    }

    *items = storage;
    return result;
#else
    seq_scratch *b = nullptr;

    if (NB_LIKELY(n <= PY_SSIZE_T_MAX / sizeof(PyObject *)))
        b = (seq_scratch *) PyMem_Malloc(sizeof(seq_scratch) +
                                         n * sizeof(PyObject *));

    if (NB_UNLIKELY(!b)) {
        *items = nullptr;
        PyErr_NoMemory();
        return nullptr;
    }

    b->n = n;
    b->is_tuple = IsTuple;
    b->items = (PyObject **) (b + 1);
    memset(b->items, 0, n * sizeof(PyObject *));

    *items = b->items;
    return b;
#endif
}

void *tuple_alloc(size_t n, PyObject ***items) noexcept {
    return seq_alloc_impl<true>(n, items);
}

void *list_alloc(size_t n, PyObject ***items) noexcept {
    return seq_alloc_impl<false>(n, items);
}


PyObject *seq_commit(void *builder, size_t n_valid) noexcept {
#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)
    PyObject *result = (PyObject *) builder;

    if (NB_LIKELY(n_valid == (size_t) Py_SIZE(result)))
        return result;

    Py_DECREF(result); // Releases the entries stored so far
    return nullptr;
#else
    seq_scratch *b = (seq_scratch *) builder;
    PyObject *result = nullptr;

    if (NB_LIKELY(n_valid == b->n))
        result = b->is_tuple ? PyTuple_New((Py_ssize_t) b->n)
                             : PyList_New((Py_ssize_t) b->n);

    if (NB_LIKELY(result)) {
        if (b->is_tuple) {
            for (size_t i = 0; i < b->n; ++i)
                NB_TUPLE_SET_ITEM(result, (Py_ssize_t) i, b->items[i]);
        } else {
            for (size_t i = 0; i < b->n; ++i)
                NB_LIST_SET_ITEM(result, (Py_ssize_t) i, b->items[i]);
        }
    } else {
        for (size_t i = 0; i < b->n; ++i)
            Py_XDECREF(b->items[i]);
    }

    PyMem_Free(b);
    return result;
#endif
}

#if defined(Py_LIMITED_API) || defined(PYPY_VERSION)
/// Capsule destructor of a null-terminated array of strong references
static void array_capsule_free(PyObject *o) noexcept {
    PyObject **ptr = (PyObject **) PyCapsule_GetPointer(o, nullptr);
    for (size_t i = 0; ptr[i] != nullptr; ++i)
        Py_DECREF(ptr[i]);
    PyMem_Free(ptr);
}
#endif


PyObject **seq_get(PyObject *seq, size_t *size_out, PyObject **temp_out) noexcept {
    PyObject *temp = nullptr;
    size_t size = 0;
    PyObject **result = nullptr;

    // This function is used during overload resolution; if anything
    // goes wrong, it fails gracefully without reporting errors. Other
    // overloads will then be tried.

    if (PyUnicode_CheckExact(seq) || PyBytes_CheckExact(seq)) {
        *size_out = 0;
        *temp_out = nullptr;
        return nullptr;
    }

#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)
    if (PyTuple_CheckExact(seq)) {
        size = (size_t) PyTuple_GET_SIZE(seq);
        result = ((PyTupleObject *) seq)->ob_item;
        // Special case for zero-sized lists/tuples. CPython
        // sets ob_item to NULL, which this function incidentally uses to
        // signal an error. Return a nonzero pointer that will, however,
        // still trigger a segfault if dereferenced.
        if (size == 0)
            result = (PyObject **) 1;
#  if !defined(NB_FREE_THREADED) // Require immutable holder in free-threaded mode
    } else if (PyList_CheckExact(seq)) {
        size = (size_t) PyList_GET_SIZE(seq);
        result = ((PyListObject *) seq)->ob_item;
        if (size == 0) // ditto
            result = (PyObject **) 1;
#  endif
    } else if (PySequence_Check(seq)) {
        temp = PySequence_Tuple(seq);

        if (temp)
            result = seq_get(temp, &size, temp_out);
        else
            PyErr_Clear();
    }
#else
    // There isn't a nice way to get a PyObject** in Py_LIMITED_API. This
    // is going to be slow, but hopefully also very future-proof..
    if (PySequence_Check(seq)) {
        Py_ssize_t size_seq = PySequence_Length(seq);

        if (size_seq >= 0) {
            result = (PyObject **) PyMem_Malloc(sizeof(PyObject *) * (size_t) (size_seq + 1));

            if (result) {
                result[size_seq] = nullptr;

                for (Py_ssize_t i = 0; i < size_seq; ++i) {
                    PyObject *o = PySequence_GetItem(seq, i);

                    if (o) {
                        result[i] = o;
                    } else {
                        PyErr_Clear();
                        for (Py_ssize_t j = 0; j < i; ++j)
                            Py_DECREF(result[j]);

                        PyMem_Free(result);
                        result = nullptr;
                        break;
                    }
                }
            }

            if (result) {
                temp = PyCapsule_New(result, nullptr, array_capsule_free);

                if (temp) {
                    size = (size_t) size_seq;
                } else {
                    PyErr_Clear();
                    for (Py_ssize_t i = 0; i < size_seq; ++i)
                        Py_DECREF(result[i]);

                    PyMem_Free(result);
                    result = nullptr;
                }
            }
        } else if (size_seq < 0) {
            PyErr_Clear();
        }
    }
#endif

    *temp_out = temp;
    *size_out = size;
    return result;
}


PyObject **seq_get_with_size(PyObject *seq, size_t size,
                             PyObject **temp_out) noexcept {

    // This function is used during overload resolution; if anything
    // goes wrong, it fails gracefully without reporting errors. Other
    // overloads will then be tried.

    PyObject *temp = nullptr,
             **result = nullptr;

#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)
    if (PyTuple_CheckExact(seq)) {
        if (size == (size_t) PyTuple_GET_SIZE(seq)) {
            result = ((PyTupleObject *) seq)->ob_item;
            // Special case for zero-sized lists/tuples. CPython
            // sets ob_item to NULL, which this function incidentally uses to
            // signal an error. Return a nonzero pointer that will, however,
            // still trigger a segfault if dereferenced.
            if (size == 0)
                result = (PyObject **) 1;
        }
#  if !defined(NB_FREE_THREADED) // Require immutable holder in free-threaded mode
    } else if (PyList_CheckExact(seq)) {
        if (size == (size_t) PyList_GET_SIZE(seq)) {
            result = ((PyListObject *) seq)->ob_item;
            if (size == 0) // ditto
                result = (PyObject **) 1;
        }
#  endif
    } else if (PySequence_Check(seq)) {
        Py_ssize_t size_seq = PySequence_Size(seq);
        if (size_seq != (Py_ssize_t) size) {
            if (size_seq == -1)
                PyErr_Clear();
        } else {
            temp = PySequence_Tuple(seq);
            if (temp)
                result = seq_get_with_size(temp, size, temp_out);
            else
                PyErr_Clear();
        }
    }
#else
    // There isn't a nice way to get a PyObject** in Py_LIMITED_API. This
    // is going to be slow, but hopefully also very future-proof..
    if (PySequence_Check(seq)) {
        Py_ssize_t size_seq = PySequence_Length(seq);

        if (size == (size_t) size_seq) {
            result =
                (PyObject **) PyMem_Malloc(sizeof(PyObject *) * (size + 1));

            if (result) {
                result[size] = nullptr;

                for (Py_ssize_t i = 0; i < size_seq; ++i) {
                    PyObject *o = PySequence_GetItem(seq, i);

                    if (o) {
                        result[i] = o;
                    } else {
                        PyErr_Clear();
                        for (Py_ssize_t j = 0; j < i; ++j)
                            Py_DECREF(result[j]);

                        PyMem_Free(result);
                        result = nullptr;
                        break;
                    }
                }
            }

            if (result) {
                temp = PyCapsule_New(result, nullptr, array_capsule_free);

                if (!temp) {
                    PyErr_Clear();
                    for (Py_ssize_t i = 0; i < size_seq; ++i)
                        Py_DECREF(result[i]);

                    PyMem_Free(result);
                    result = nullptr;
                }
            }
        } else if (size_seq < 0) {
            PyErr_Clear();
        }
    }
#endif

    *temp_out = temp;
    return result;
}

// ========================================================================

/// Snapshot buffer of 'mapping_get()' below. It holds strong references in a
/// tuple, or in a capsule-owned array where the limited API cannot reach one.
struct mapping_snapshot {
    PyObject *temp = nullptr;
    PyObject **items = nullptr;
    size_t index = 0;

    /// Reserve room for 'n' entries. Returns false when out of memory.
    bool alloc(size_t n) noexcept {
#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)
        temp = PyTuple_New((Py_ssize_t) n);
        if (temp)
            items = ((PyTupleObject *) temp)->ob_item;
#else
        if (NB_LIKELY(n < PY_SSIZE_T_MAX / sizeof(PyObject *)))
            items = (PyObject **) PyMem_Malloc(sizeof(PyObject *) * (n + 1));
#endif
        if (NB_UNLIKELY(!items))
            PyErr_Clear();
        return items != nullptr;
    }

    /// Store a strong reference to 'o' in the next entry
    void put(PyObject *o) noexcept { items[index++] = Py_NewRef(o); }

    /// Release the buffer along with everything stored in it
    void release() noexcept {
#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)
        Py_CLEAR(temp);
#else
        for (size_t i = 0; i < index; ++i)
            Py_DECREF(items[i]);
        PyMem_Free(items);
#endif
        items = nullptr;
    }

    /// Hand the buffer to the caller, or return null when that is not possible
    PyObject **commit(PyObject **temp_out) noexcept {
        // A nonzero dummy pointer separates an empty mapping from a failure
        if (!items)
            return (PyObject **) 1;

#if defined(Py_LIMITED_API) || defined(PYPY_VERSION)
        items[index] = nullptr;
        temp = PyCapsule_New(items, nullptr, array_capsule_free);
        if (NB_UNLIKELY(!temp)) {
            PyErr_Clear();
            release();
            return nullptr;
        }
#endif
        *temp_out = temp;
        return items;
    }
};

PyObject **mapping_get(PyObject *o, size_t *size_out, PyObject **temp_out) noexcept {
    // Failures are silent so that overload resolution can try other candidates
    mapping_snapshot s;

    *size_out = 0;
    *temp_out = nullptr;

    if (PyDict_CheckExact(o)) {
        // Direct traversal avoids the per-entry tuples of PyMapping_Items()
        ft_object_guard guard(o);

        size_t size = (size_t) NB_DICT_GET_SIZE(o);
        if (size && !s.alloc(2 * size))
            return nullptr;

        Py_ssize_t pos = 0;
        PyObject *key, *value;

        // The allocation above can run Python code that resizes 'o'
        for (size_t i = 0; i < size && PyDict_Next(o, &pos, &key, &value); ++i) {
            s.put(key);
            s.put(value);
        }
    } else {
        if (!PyMapping_Check(o))
            return nullptr;

        PyObject *items = PyMapping_Items(o);
        if (!items) {
            PyErr_Clear();
            return nullptr;
        }

        // The buffer size derives from 'items', hence this check
        if (!list_check(items)) {
            Py_DECREF(items);
            return nullptr;
        }

        // 'items' is unique to this thread, no locking or ref. counting needed
        size_t size = (size_t) NB_LIST_GET_SIZE(items);

        if (size && !s.alloc(2 * size)) {
            Py_DECREF(items);
            return nullptr;
        }

        for (Py_ssize_t i = 0; i < (Py_ssize_t) size; ++i) {
            PyObject *item = NB_LIST_GET_ITEM(items, i);

            if (!tuple_check(item) || NB_TUPLE_GET_SIZE(item) != 2) {
                Py_DECREF(items);
                s.release();
                return nullptr;
            }

            s.put(NB_TUPLE_GET_ITEM(item, 0));
            s.put(NB_TUPLE_GET_ITEM(item, 1));
        }

        Py_DECREF(items);
    }

    *size_out = s.index / 2;
    return s.commit(temp_out);
}

// ========================================================================

static void property_install_impl(nb_internals *p, PyObject *scope,
                                  const char *name, PyObject *getter,
                                  PyObject *setter, bool is_static) {
    PyTypeObject *tp = is_static ? nb_static_property_tp(p) : &PyProperty_Type;
    PyObject *m = getter ? getter : setter;
    object doc = none();

    PyTypeObject *mt = m ? Py_TYPE(m) : nullptr;
    if (m && (mt == p->nb_func || mt == p->nb_method)) {
        func_data *f = nb_func_data(m);
        if (f->flags & (uint32_t) func_flags::has_doc)
            doc = str(f->doc);
    }

    object prop = obj_call(p, handle((PyObject *) tp),
                           handle(getter ? getter : none_ptr()),
                           handle(setter ? setter : none_ptr()),
                           handle(none_ptr()) /* deleter */, doc);
    str_setattr(p, scope, name, prop);
}

void property_install(nb_internals *p, PyObject *scope, const char *name,
                      PyObject *getter, PyObject *setter,
                      bool is_static) noexcept {
    try {
        property_install_impl(p, scope, name, getter, setter, is_static);
    } catch (...) {
        fail_exception("nanobind::detail::property_install", name);
    }
}

// ========================================================================

bool load_cmplx(nb_internals *p, PyObject *ob, uint32_t flags,
                double *out) noexcept {
    (void) p;
    bool is_complex = PyComplex_CheckExact(ob),
         convert = (flags & cast_flags::convert);
#if !defined(Py_LIMITED_API)
    if (is_complex || convert) {
        Py_complex result = PyComplex_AsCComplex(ob);
        if (result.real != -1.0 || !PyErr_Occurred()) {
            out[0] = result.real; out[1] = result.imag;
            return true;
        } else {
            PyErr_Clear();
        }
    }
#else
#if Py_LIMITED_API < 0x030D0000
    // Before version 3.13, __complex__() was not called by the Stable ABI
    // functions PyComplex_{Real,Imag}AsDouble(), so we do so ourselves.
    if (!is_complex && convert
            && !PyType_IsSubtype(Py_TYPE(ob), &PyComplex_Type)
            && PyObject_HasAttr(ob, NB_INTERNED(p, __complex__))) {
        PyObject* tmp = PyObject_CallFunctionObjArgs(
                (PyObject*) &PyComplex_Type, ob, NULL);
        if (tmp) {
            double re = PyComplex_RealAsDouble(tmp);
            double im = PyComplex_ImagAsDouble(tmp);
            Py_DECREF(tmp);
            if ((re != -1.0 && im != -1.0) || !PyErr_Occurred()) {
                out[0] = re; out[1] = im;
                return true;
            }
        }
        PyErr_Clear();
        return false;
    }
#endif
    if (is_complex || convert) {
        double re = PyComplex_RealAsDouble(ob);
        double im = PyComplex_ImagAsDouble(ob);
        if ((re != -1.0 && im != -1.0) || !PyErr_Occurred()) {
            out[0] = re; out[1] = im;
            return true;
        } else {
            PyErr_Clear();
        }
    }
#endif

    return false;
}

bool load_f64(nb_internals *, PyObject *o, uint32_t flags,
              double *out) noexcept {
    bool is_float = PyFloat_CheckExact(o);

#if !defined(Py_LIMITED_API)
    if (NB_LIKELY(is_float)) {
        *out = PyFloat_AS_DOUBLE(o);
        return true;
    }

    is_float = false;
#endif

    if (is_float || (flags & cast_flags::convert)) {
        double result = PyFloat_AsDouble(o);

        if (result != -1.0 || !PyErr_Occurred()) {
            *out = result;
            return true;
        } else {
            PyErr_Clear();
        }
    }

    return false;
}

bool load_f32(nb_internals *, PyObject *o, uint32_t flags,
              float *out) noexcept {
    bool is_float = PyFloat_CheckExact(o);
    bool convert = flags & cast_flags::convert;

#if !defined(Py_LIMITED_API)
    if (NB_LIKELY(is_float)) {
        double d = PyFloat_AS_DOUBLE(o);
        float result = (float) d;
        if (convert || (double) result == d || d != d) {
            *out = result;
            return true;
        } else {
            return false;
        }
    }

    is_float = false;
#endif

    if (is_float || convert) {
        double d = PyFloat_AsDouble(o);
        if (d != -1.0 || !PyErr_Occurred()) {
            float result = (float) d;
            if (convert || (double) result == d || d != d) {
                *out = result;
                return true;
            }
        } else {
            PyErr_Clear();
        }
    }

    return false;
}

#if !defined(Py_LIMITED_API) && !defined(PYPY_VERSION)

/// Sign and digits of a Python 'int', see Include/cpython/longintrepr.h
struct int_repr {
    const digit *digits; // Little-endian, PyLong_SHIFT bits each
    Py_ssize_t size;     // Number of digits, of which zero has none
    bool negative;
};

NB_INLINE int_repr int_repr_get(PyObject *o) noexcept {
    PyLongObject *l = (PyLongObject *) o;

#if PY_VERSION_HEX < 0x030c0000
    Py_ssize_t size = Py_SIZE(l);
    return { l->ob_digit, size < 0 ? -size : size, size < 0 };
#else
    uintptr_t tag = l->long_value.lv_tag;
    return { l->long_value.ob_digit,
             (Py_ssize_t) (tag >> _PyLong_NON_SIZE_BITS),
             (tag & _PyLong_SIGN_MASK) == 2 };
#endif
}

/// Convert an object that is already known to be an 'int'. Never raises.
template <typename T>
NB_INLINE bool load_int_exact(nb_internals *, PyObject *o, T *out) noexcept {
    constexpr size_t bits = sizeof(T) * 8;

    // Number of digits of the largest value of type 'T'
    constexpr Py_ssize_t size_max =
        (Py_ssize_t) ((bits + PyLong_SHIFT - 1) / PyLong_SHIFT);

    // Largest leading digit that keeps the accumulator below 2**64
    constexpr uint64_t digit_max =
        (~(uint64_t) 0) >> ((size_max - 1) * PyLong_SHIFT);

    // Magnitude of the largest positive and negative value of type 'T'
    constexpr uint64_t max_pos = (~(uint64_t) 0) >> (64 - bits + std::is_signed_v<T>),
                       max_neg = std::is_signed_v<T> ? max_pos + 1 : 0;

    int_repr r = int_repr_get(o);
    uint64_t value;

    if (NB_LIKELY(r.size <= 1)) {
        value = r.size ? r.digits[0] : 0;
    } else {
        if (NB_UNLIKELY(r.size > size_max ||
                        (r.size == size_max && r.digits[size_max - 1] > digit_max)))
            return false;

        value = 0;
        for (Py_ssize_t i = r.size - 1; i >= 0; --i)
            value = (value << PyLong_SHIFT) | r.digits[i];
    }

    // 'max_neg' is zero when 'T' is unsigned, which rejects negative values
    if (NB_UNLIKELY(value > (r.negative ? max_neg : max_pos)))
        return false;

    *out = (T) (r.negative ? (uint64_t) 0 - value : value);
    return true;
}

#else

/// Convert an object that is already known to be an 'int'. Never raises.
template <typename T>
NB_INLINE bool load_int_exact(nb_internals *p, PyObject *o, T *out) noexcept {
    int overflow;
    long long value = PyLong_AsLongLongAndOverflow(o, &overflow);

    if (NB_LIKELY(overflow == 0)) {
        T value_t = (T) value;

        if (NB_UNLIKELY((std::is_unsigned_v<T> && value < 0) ||
                        (sizeof(T) != sizeof(long long) &&
                         value != (long long) value_t)))
            return false;

        *out = value_t;
        return true;
    }

    // Branch to handle the [2**63, 2**64) range (out of bounds of 'long long')
    if constexpr (std::is_unsigned_v<T> && sizeof(T) == sizeof(long long)) {
        if (overflow > 0) {
            // Guard against out of range values using a comparison instead of
            // letting PyLong_AsUnsignedLongLong fail, which would be much more expensive.
            if (PyObject_RichCompareBool(o, NB_INTERNED(p, u64_limit),
                                         Py_LT) == 1) {
                *out = (T) PyLong_AsUnsignedLongLongMask(o);
                return true;
            }
        }
    }

    return false;
}

#endif

template <typename T>
NB_INLINE bool load_int(nb_internals *p, PyObject *o, uint32_t flags,
                        T *out) noexcept {
    if (NB_LIKELY(PyLong_CheckExact(o)))
        return load_int_exact(p, o, out);

    if (!(flags & cast_flags::convert))
        return false;

    // Handle subclasses of 'int' via the _exact() caster
    if (PyLong_Check(o))
        return load_int_exact(p, o, out);

    // Give up if we reach this point and __index__() does not exist
#if !defined(Py_LIMITED_API)
    PyNumberMethods *nm = Py_TYPE(o)->tp_as_number;
    if (!nm || !nm->nb_index)
        return false;
#else
    if (!PyIndex_Check(o))
        return false;
#endif

    PyObject *temp = PyNumber_Index(o);
    if (NB_UNLIKELY(!temp)) {
        PyErr_Clear();
        return false;
    }

    bool result = load_int_exact(p, temp, out);
    Py_DECREF(temp);
    return result;
}

bool load_u8(nb_internals *p, PyObject *o, uint32_t flags,
             uint8_t *out) noexcept {
    return load_int(p, o, flags, out);
}

bool load_i8(nb_internals *p, PyObject *o, uint32_t flags,
             int8_t *out) noexcept {
    return load_int(p, o, flags, out);
}

bool load_u16(nb_internals *p, PyObject *o, uint32_t flags,
             uint16_t *out) noexcept {
    return load_int(p, o, flags, out);
}

bool load_i16(nb_internals *p, PyObject *o, uint32_t flags,
             int16_t *out) noexcept {
    return load_int(p, o, flags, out);
}

bool load_u32(nb_internals *p, PyObject *o, uint32_t flags,
             uint32_t *out) noexcept {
    return load_int(p, o, flags, out);
}

bool load_i32(nb_internals *p, PyObject *o, uint32_t flags,
             int32_t *out) noexcept {
    return load_int(p, o, flags, out);
}

bool load_u64(nb_internals *p, PyObject *o, uint32_t flags,
             uint64_t *out) noexcept {
    return load_int(p, o, flags, out);
}

bool load_i64(nb_internals *p, PyObject *o, uint32_t flags,
             int64_t *out) noexcept {
    return load_int(p, o, flags, out);
}

// ========================================================================

bool gil_check() noexcept {
#if !defined(Py_LIMITED_API)
    return PyGILState_Check() != 0;
#else
    // Not expressible in the limited API; report success
    return true;
#endif
}

// ========================================================================

/* 'abi3t' extensions store the mutex as a single byte because PyMutex is not
   part of their stable ABI. A larger PyMutex would overflow that byte. */
#if defined(NB_FREE_THREADED)
static_assert(sizeof(PyMutex) == 1, "nb::ft_mutex assumes a one-byte PyMutex");
#endif

void ft_mutex_lock(void *m) noexcept {
#if defined(NB_FREE_THREADED)
    PyMutex_Lock((PyMutex *) m);
#else
    (void) m;
#endif
}

void ft_mutex_unlock(void *m) noexcept {
#if defined(NB_FREE_THREADED)
    PyMutex_Unlock((PyMutex *) m);
#else
    (void) m;
#endif
}

// ========================================================================

uint32_t read_flag(nb_internals *p, nb_flag f) noexcept {
    switch (f) {
        case nb_flag::leak_warnings:
            return p->print_leak_warnings;
        case nb_flag::implicit_cast_warnings:
            return p->print_implicit_cast_warnings;
        default:
            fail("nanobind::detail::read_flag(): unknown flag!");
    }
}

void write_flag(nb_internals *p, nb_flag f, uint32_t value) {
    switch (f) {
        case nb_flag::leak_warnings:
            p->print_leak_warnings = value != 0;
            break;
        case nb_flag::implicit_cast_warnings:
            p->print_implicit_cast_warnings = value != 0;
            break;
        default:
            raise("nanobind::detail::write_flag(): unknown flag!");
    }
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
