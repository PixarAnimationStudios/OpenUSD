#include <nanobind/ndarray.h>
#include <atomic>
#include <memory>
#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)

NAMESPACE_BEGIN(dlpack)

/// Indicates the managed_dltensor_versioned is read only.
static constexpr uint64_t flag_bitmask_read_only = 1UL << 0;

struct version {
    uint32_t major;
    uint32_t minor;
};

NAMESPACE_END(dlpack)

// ========================================================================

NAMESPACE_BEGIN(detail)

/// Maximum number of ndarray dimensions (2x NumPy's NPY_MAXDIMS)
static constexpr int32_t max_ndim = 128;

// DLPack version 0, deprecated Feb 2024, obsoleted March 2025
struct managed_dltensor {
    dlpack::dltensor dltensor;
    void *manager_ctx;
    void (*deleter)(managed_dltensor *);
};

// DLPack version 1, pre-release Feb 2024, release Sep 2024
struct managed_dltensor_versioned {
    dlpack::version version;
    void *manager_ctx;
    void (*deleter)(managed_dltensor_versioned *);
    uint64_t flags = 0UL;
    dlpack::dltensor dltensor;
};

static void mt_from_buffer_delete(managed_dltensor_versioned* self) {
    // Don't run the cleanup if the interpreter has been shut down
    if (cleanup_guard guard{}) {
        Py_buffer *buf = (Py_buffer *) self->manager_ctx;
        PyBuffer_Release(buf);
        PyMem_Free(buf);
        PyMem_Free(self);  // This also frees shape and size arrays.
    }
}

// Forward declaration
struct ndarray_handle;

template<typename MT>
static void mt_from_handle_delete(MT* self) {
    // Don't run the cleanup if the interpreter has been shut down
    if (cleanup_guard guard{}) {
        ndarray_handle* th = (ndarray_handle *) self->manager_ctx;
        PyMem_Free(self);
        ndarray_dec_ref(th);
    }
}

template<bool Versioned>
static void capsule_delete(PyObject *capsule) {
    const char* capsule_name;
    if constexpr (Versioned)
        capsule_name = "dltensor_versioned";
    else
        capsule_name = "dltensor";

    using MT = std::conditional_t<Versioned, managed_dltensor_versioned,
                                             managed_dltensor>;
    error_scope scope; // temporarily save any existing errors
    MT* mt = (MT*) PyCapsule_GetPointer(capsule, capsule_name);
    if (mt)
        mt->deleter(mt);
    else
        PyErr_Clear();
}

// Reference-counted wrapper for versioned or unversioned managed tensors
struct ndarray_handle {
    union {
        managed_dltensor           *mt_unversioned;
        managed_dltensor_versioned *mt_versioned;
    };
    std::atomic<size_t> refcount;
    PyObject *owner, *self;
    bool versioned;     // This tags which union member is active.
    bool free_strides;  // True if we added strides to an imported tensor.
    bool call_deleter;  // True if tensor was imported, else PyMem_Free(mt).
    bool ro;            // Whether tensor is read-only.

    dlpack::dltensor &tensor() {
        return versioned ? mt_versioned->dltensor : mt_unversioned->dltensor;
    }

    template <bool Versioned> PyObject *make_capsule() {
        using MT = std::conditional_t<Versioned, managed_dltensor_versioned,
                                                 managed_dltensor>;
        const char *name = Versioned ? "dltensor_versioned" : "dltensor";

        // Reuse nanobind's own managed tensor if its flavor already matches;
        // otherwise allocate a fresh one wrapping a copy of the DLTensor.
        MT *mt = nullptr;
        if (versioned == Versioned) {
            if constexpr (Versioned)
                mt = mt_versioned;
            else
                mt = mt_unversioned;
            if (mt->manager_ctx != this)
                mt = nullptr;
        }

        PyObject *capsule;
        if (mt) {
            capsule = PyCapsule_New(mt, name, capsule_delete<Versioned>);
        } else {
            scoped_pymalloc<MT> fresh;
            if constexpr (Versioned) {
                fresh->version = {dlpack::major_version, dlpack::minor_version};
                fresh->flags = ro ? dlpack::flag_bitmask_read_only : 0;
            }
            fresh->manager_ctx = this;
            fresh->deleter = mt_from_handle_delete<MT>;
            memcpy(&fresh->dltensor, &tensor(), sizeof(dlpack::dltensor));
            capsule = PyCapsule_New(fresh.release(), name,
                                    capsule_delete<Versioned>);
        }
        check(capsule, "Could not make capsule");
        refcount++;
        return capsule;
    }
};

// ========================================================================

static void nb_ndarray_dealloc(PyObject *self) {
    PyTypeObject *tp = Py_TYPE(self);
    ndarray_dec_ref(((nb_ndarray *) self)->th);
    PyObject_Free(self);
    Py_DECREF(tp);
}

static int nb_ndarray_getbuffer(PyObject *self, Py_buffer *view, int flags) {
    // The buffer protocol requires that 'view->obj' be set to NULL whenever
    // the exporter signals failure by returning -1.
    view->obj = nullptr;

    ndarray_handle *th = ((nb_ndarray *) self)->th;
    dlpack::dltensor &t = th->tensor();

    if (t.device.device_type != device::cpu::value) {
        PyErr_SetString(PyExc_BufferError, "Only CPU-allocated ndarrays can be "
                                           "accessed via the buffer protocol!");
        return -1;
    }

    // Honor a writable request: refuse to expose read-only memory as writable.
    if ((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE && th->ro) {
        PyErr_SetString(PyExc_BufferError,
            "Cannot provide writable access to a read-only ndarray!");
        return -1;
    }

    const char *format = nullptr;
    switch ((dlpack::dtype_code) t.dtype.code) {
        case dlpack::dtype_code::Int:
            switch (t.dtype.bits) {
                case 8: format = "b"; break;
                case 16: format = "h"; break;
                case 32: format = "i"; break;
                case 64: format = "q"; break;
            }
            break;

        case dlpack::dtype_code::UInt:
            switch (t.dtype.bits) {
                case 8: format = "B"; break;
                case 16: format = "H"; break;
                case 32: format = "I"; break;
                case 64: format = "Q"; break;
            }
            break;

        case dlpack::dtype_code::Float:
            switch (t.dtype.bits) {
                case 16: format = "e"; break;
                case 32: format = "f"; break;
                case 64: format = "d"; break;
            }
            break;

        case dlpack::dtype_code::Complex:
            switch (t.dtype.bits) {
                case 32: format = "Ze"; break;
                case 64: format = "Zf"; break;
                case 128: format = "Zd"; break;
            }
            break;

        case dlpack::dtype_code::Bool:
            format = "?";
            break;

        default:
            break;
    }

    if (!format || t.dtype.lanes != 1) {
        PyErr_SetString(PyExc_BufferError,
            "Cannot convert DLPack dtype into buffer protocol format!");
        return -1;
    }

    const Py_ssize_t itemsize = t.dtype.bits / 8;
    Py_ssize_t len = itemsize, size = 1;
    for (size_t i = 0; i < (size_t) t.ndim; ++i) {
        len *= (Py_ssize_t) t.shape[i];
        size *= (Py_ssize_t) t.shape[i];
    }

    // When the consumer cannot handle strides, only C-contiguous data may be
    // exported -- otherwise it would interpret 'buf'..'buf + len' as a packed
    // C-contiguous block and silently read the wrong elements. Arrays with one
    // or fewer elements are trivially contiguous.
    if ((flags & PyBUF_STRIDES) != PyBUF_STRIDES && size > 1) {
        bool c_contig = true;
        for (int64_t i = t.ndim - 1, accum = 1; i >= 0; --i) {
            c_contig &= t.shape[i] == 1 || t.strides[i] == accum;
            accum *= t.shape[i];
        }

        if (!c_contig) {
            PyErr_SetString(PyExc_BufferError,
                "Cannot provide a contiguous buffer for a non-C-contiguous "
                "ndarray!");
            return -1;
        }
    }

    scoped_pymalloc<Py_ssize_t> shape_and_strides(2 * (size_t) t.ndim);
    Py_ssize_t* shape = shape_and_strides.get();
    Py_ssize_t* strides = shape + t.ndim;

    for (size_t i = 0; i < (size_t) t.ndim; ++i) {
        shape[i] = (Py_ssize_t) t.shape[i];
        strides[i] = (Py_ssize_t) t.strides[i] * itemsize;
    }

    view->buf = (void *) ((uintptr_t) t.data + t.byte_offset);
    view->obj = Py_NewRef(self);
    view->len = len;
    view->itemsize = itemsize;
    view->readonly = th->ro;
    view->ndim = t.ndim;
    view->format =
        ((flags & PyBUF_FORMAT) == PyBUF_FORMAT) ? (char *) format : nullptr;
    view->shape = ((flags & PyBUF_ND) == PyBUF_ND) ? shape : nullptr;
    view->strides =
        ((flags & PyBUF_STRIDES) == PyBUF_STRIDES) ? strides : nullptr;
    view->suboffsets = nullptr;
    view->internal = shape_and_strides.release();

    return 0;
}

static void nb_ndarray_releasebuffer(PyObject *, Py_buffer *view) {
    PyMem_Free(view->internal);
}

// This function implements __dlpack__() for a nanobind.nb_ndarray.
static PyObject *nb_ndarray_dlpack(PyObject *self, PyObject *const *args,
                                   Py_ssize_t nargs, PyObject *kwnames) {
    if (nargs != 0) {
        PyErr_SetString(PyExc_TypeError,
                "__dlpack__() does not accept positional arguments");
        return nullptr;
    }
    Py_ssize_t nkwargs = (kwnames) ? NB_TUPLE_GET_SIZE(kwnames) : 0;

    // Match a keyword name against an interned reference.
    auto key_is = [](PyObject *key, PyObject *r) -> bool {
        return key == r;
    };

    // Match a keyword name against an interned reference, falling back to a
    // string comparison since kwnames passed via f(**d) are not guaranteed to
    // be identical to the interned objects.
    auto key_equals = [](PyObject *key, PyObject *r) -> bool {
        return key == r || PyObject_RichCompareBool(key, r, Py_EQ) == 1;
    };

    ndarray_handle *th = ((nb_ndarray *) self)->th;
    nb_internals *p = ((nb_ndarray *) self)->internals;
    dlpack::dltensor &t = th->tensor();

    long max_major_version = 0;

    // Return nkwargs on success, -1 on error, else index of unmatched kwarg.
    auto parse_kwargs = [&kwnames, &nkwargs, &args, &t, &max_major_version, p](
                                Py_ssize_t begin, auto compare) -> Py_ssize_t {
        // Extract a 2-tuple of integers; returns false (with no error set)
        // for any other input.
        auto get_int_pair = [](PyObject *value, long *a, long *b) -> bool {
            if (!PyTuple_Check(value) || NB_TUPLE_GET_SIZE(value) != 2)
                return false;
            *a = PyLong_AsLong(NB_TUPLE_GET_ITEM(value, 0));
            *b = PyLong_AsLong(NB_TUPLE_GET_ITEM(value, 1));
            if (PyErr_Occurred()) {
                PyErr_Clear();
                return false;
            }
            return true;
        };

        for (Py_ssize_t i = begin; i < nkwargs; ++i) {
            PyObject* key = NB_TUPLE_GET_ITEM(kwnames, i);
            PyObject* value = args[i];
            long a, b;
            if (compare(key, NB_INTERNED(p, copy))) {
                // The capsule aliases C++-owned storage; a copy cannot be made
                if (value == true_ptr()) {
                    PyErr_SetString(PyExc_BufferError,
                            "__dlpack__(): copy=True is not supported.");
                    return -1;
                }
            } else if (compare(key, NB_INTERNED(p, dl_device))) {
                // Reject requests for a device other than the array's own
                if (value != none_ptr() &&
                        (!get_int_pair(value, &a, &b) ||
                         a != (long) t.device.device_type ||
                         b != (long) t.device.device_id)) {
                    PyErr_SetString(PyExc_BufferError,
                            "__dlpack__(): unsupported dl_device.");
                    return -1;
                }
            } else if (compare(key, NB_INTERNED(p, max_version))) {
                if (value != none_ptr()) {
                    if (!get_int_pair(value, &a, &b)) {
                        PyErr_SetString(PyExc_TypeError,
                                "max_version must be None or tuple[int, int]");
                        return -1;
                    }
                    max_major_version = a;
                }
            } else if (compare(key, NB_INTERNED(p, stream))) {
                // Accepted but ignored: nanobind tracks no producer stream and
                // has no backend dependency, so it cannot synchronize. See docs.
            } else {
                return i;
            }
        }
        return nkwargs;
    };

    Py_ssize_t result = parse_kwargs(0, key_is);
    if (NB_UNLIKELY(result < 0))
        return nullptr;
    if (NB_UNLIKELY(result < nkwargs)) {
        result = parse_kwargs(result, key_equals);
        if (NB_UNLIKELY(result < 0))
            return nullptr;
        if (NB_UNLIKELY(result < nkwargs)) {
            PyErr_Format(PyExc_TypeError,
                    "__dlpack__(): unsupported keyword argument '%S'",
                    NB_TUPLE_GET_ITEM(kwnames, result));
            return nullptr;
        }
    }

    PyObject *capsule;
    if (max_major_version >= (long)dlpack::major_version)
        capsule = th->make_capsule<true>();
    else
        capsule = th->make_capsule<false>();

    return capsule;
}

// This function implements __dlpack_device__() for a nanobind.nb_ndarray.
static PyObject *nb_ndarray_dlpack_device(PyObject *self, PyObject *) {
    ndarray_handle *th = ((nb_ndarray *) self)->th;
    dlpack::dltensor& t = th->tensor();
    PyObject *r;
    if (t.device.device_type == 1 && t.device.device_id == 0) {
        r = Py_NewRef(NB_INTERNED(((nb_ndarray *) self)->internals, dl_cpu_tpl));
    } else {
        r = PyTuple_New(2);
        PyObject *r0 = PyLong_FromLong(t.device.device_type);
        PyObject *r1 = PyLong_FromLong(t.device.device_id);
        if (!r || !r0 || !r1) {
            Py_XDECREF(r);
            Py_XDECREF(r0);
            Py_XDECREF(r1);
            return nullptr;
        }
        NB_TUPLE_SET_ITEM(r, 0, r0);
        NB_TUPLE_SET_ITEM(r, 1, r1);
    }
    return r;
}

static PyMethodDef nb_ndarray_methods[] = {
   { "__dlpack__", (PyCFunction) (void *) nb_ndarray_dlpack,
                   METH_FASTCALL | METH_KEYWORDS, nullptr },
   { "__dlpack_device__", nb_ndarray_dlpack_device, METH_NOARGS, nullptr },
   { nullptr, nullptr, 0, nullptr }
};

static PyTypeObject *nb_ndarray_tp(nb_internals *internals_) noexcept {
    PyTypeObject *tp = internals_->nb_ndarray.load_acquire();

    if (NB_UNLIKELY(!tp)) {
        lock_internals guard(internals_);
        tp = internals_->nb_ndarray.load_relaxed();
        if (tp)
            return tp;

        PyType_Slot slots[] = {
            { Py_tp_dealloc, (void *) nb_ndarray_dealloc },
            { Py_tp_methods, (void *) nb_ndarray_methods },
            { Py_bf_getbuffer, (void *) nb_ndarray_getbuffer },
            { Py_bf_releasebuffer, (void *) nb_ndarray_releasebuffer },
            { 0, nullptr }
        };

        PyType_Spec spec = {
            /* .name = */ "nanobind.nb_ndarray",
            /* .basicsize = */ (int) sizeof(nb_ndarray),
            /* .itemsize = */ 0,
            /* .flags = */ Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
                           Py_TPFLAGS_DISALLOW_INSTANTIATION,
            /* .slots = */ slots
        };

        tp = new_type(internals_, &spec);
        check(tp, "nb_ndarray type creation failed!");

        internals_->nb_ndarray.store_release(tp);
    }

    return tp;
}

// ========================================================================

using mt_unique_ptr_t = std::unique_ptr<managed_dltensor_versioned,
                                        decltype(&mt_from_buffer_delete)>;

static mt_unique_ptr_t make_mt_from_buffer_protocol(PyObject *o, bool ro) {
    mt_unique_ptr_t mt_unique_ptr(nullptr, &mt_from_buffer_delete);
    scoped_pymalloc<Py_buffer> view;
    if (PyObject_GetBuffer(o, view.get(),
                           ro ? PyBUF_RECORDS_RO : PyBUF_RECORDS)) {
        PyErr_Clear();
        return mt_unique_ptr;
    }

    char format_c = 'B';
    const char *format_str = view->format;
    if (format_str)
        format_c = *format_str;

    bool skip_first = format_c == '@' || format_c == '=';

    int32_t num = 1;
    if (*(uint8_t *) &num == 1) {
        if (format_c == '<')
            skip_first = true;
    } else {
        if (format_c == '!' || format_c == '>')
            skip_first = true;
    }

    if (skip_first && format_str)
        format_c = *++format_str;

    bool is_complex = format_str && format_str[0] == 'Z';
    if (is_complex)
        format_c = *++format_str;

    dlpack::dtype dt { };
    bool fail = format_str && format_str[0] != '\0' && format_str[1] != '\0';

    if (!fail) {
        switch (format_c) {
            case 'c':
            case 'b':
            case 'h':
            case 'i':
            case 'l':
            case 'q':
            case 'n': dt.code = (uint8_t) dlpack::dtype_code::Int; break;

            case 'B':
            case 'H':
            case 'I':
            case 'L':
            case 'Q':
            case 'N': dt.code = (uint8_t) dlpack::dtype_code::UInt; break;

            case 'e':
            case 'f':
            case 'd': dt.code = (uint8_t) dlpack::dtype_code::Float; break;

            case '?': dt.code = (uint8_t) dlpack::dtype_code::Bool; break;

            default: fail = true;
        }

        if (is_complex) {
            fail |= dt.code != (uint8_t) dlpack::dtype_code::Float;
            dt.code = (uint8_t) dlpack::dtype_code::Complex;
        }

        dt.lanes = 1;
        dt.bits = (uint8_t) (view->itemsize * 8);
    }

    if (fail) {
        PyBuffer_Release(view.get());
        return mt_unique_ptr;
    }

    int32_t ndim = view->ndim;
    if (ndim < 0 || ndim > max_ndim) {
        PyBuffer_Release(view.get());
        return mt_unique_ptr;
    }

    static_assert(alignof(managed_dltensor_versioned) >= alignof(int64_t));
    scoped_pymalloc<managed_dltensor_versioned> mt(1, 2 * sizeof(int64_t) * (size_t) ndim);
    int64_t* shape = nullptr;
    int64_t* strides = nullptr;
    if (ndim > 0) {
        shape = new ((void*) (mt.get() + 1)) int64_t[2 * (size_t) ndim];
        strides = shape + ndim;
    }

    mt->dltensor.data = view->buf;
    mt->dltensor.device = { device::cpu::value, 0 };
    mt->dltensor.ndim = ndim;
    mt->dltensor.dtype = dt;
    mt->dltensor.shape = shape;
    mt->dltensor.strides = strides;
    mt->dltensor.byte_offset = 0UL;

    const int64_t itemsize = (int64_t) view->itemsize;
    for (int32_t i = 0; i < ndim; ++i) {
        int64_t stride = view->strides[i] / itemsize;
        if (stride * itemsize != view->strides[i]) {
            PyBuffer_Release(view.get());
            return mt_unique_ptr;
        }
        strides[i] = stride;
        shape[i] = (int64_t) view->shape[i];
    }

    mt->version = {dlpack::major_version, dlpack::minor_version};
    mt->manager_ctx = view.release();
    mt->deleter = mt_from_buffer_delete;
    mt->flags = (ro) ? dlpack::flag_bitmask_read_only : 0;

    mt_unique_ptr.reset(mt.release());
    return mt_unique_ptr;
}

// Per-framework import data for the source-detectable frameworks. Indexed by
// the framework `value` from ndarray.h; rows only exist for frameworks that can
// be detected as an incoming object (numpy..cupy), everything else is empty.
struct import_info {
    const char *module_prefix; // __module__ prefix, for detection
    const char *type_name;     // exact Py_TYPE name, for ndarray_check
    const char *to_dlpack_pkg; // legacy to_dlpack() module, or nullptr
};

static constexpr import_info importers[] = {
    /* no_framework */ { nullptr, nullptr, nullptr },
    /* numpy        */ { "numpy", "numpy.ndarray", nullptr },
    /* pytorch      */ { "torch", "torch.Tensor", "torch.utils.dlpack" },
    /* tensorflow   */ { "tensorflow", "tensorflow.Tensor", "tensorflow.experimental.dlpack" },
    /* jax          */ { "jaxlib", "jaxlib._jax.ArrayImpl", "jax.dlpack" },
    /* cupy         */ { "cupy", "cupy.ndarray", nullptr }
};

static constexpr int importer_count = sizeof(importers) / sizeof(importers[0]);

// Detect the source framework from __module__, returning its ndarray.h `value`
// (no_framework::value if unrecognized). Never raises.
static int detect_framework(nb_internals *p, PyTypeObject *tp) noexcept {
    object mod = steal(PyObject_GetAttr((PyObject *) tp,
                                        NB_INTERNED(p, __module__)));
    const char *name =
        mod.is_valid() ? PyUnicode_AsUTF8AndSize(mod.ptr(), nullptr) : nullptr;
    if (!name) {
        PyErr_Clear();
        return no_framework::value;
    }
    for (int i = 1; i < importer_count; ++i) {
        const char *p = importers[i].module_prefix;
        if (strncmp(name, p, strlen(p)) == 0)
            return i;
    }
    return no_framework::value;
}

// Convert to the requested dtype (and order, where the framework supports it).
static object convert_array(nb_internals *p, int framework, PyObject *src,
                            const char *dtype, char order) {
    object converted;
    try {
        object dtype_str = steal(raise_if_null(PyUnicode_FromString(dtype)));
        switch (framework) {
            case numpy::value:
            case cupy::value: {
                object order_str = steal(raise_if_null(
                    PyUnicode_FromStringAndSize(&order, 1)));
                converted =
                    obj_call(p, steal(getattr(src, NB_INTERNED(p, astype))),
                             dtype_str, order_str);
                break;
            }

            case pytorch::value: {
                module_ torch = module_::import_("torch");
                converted = obj_call(p, steal(getattr(src, NB_INTERNED(p, to))),
                                     str_getattr(p, torch, dtype));
                if (order == 'C')
                    converted = obj_call(
                        p, steal(getattr(converted.ptr(),
                                         NB_INTERNED(p, contiguous))));
                break;
            }

            case tensorflow::value: {
                module_ tensorflow = module_::import_("tensorflow");
                converted =
                    obj_call(p,
                             steal(getattr(tensorflow.ptr(),
                                           NB_INTERNED(p, cast))),
                             handle(src), dtype_str);
                break;
            }

            case jax::value:
                converted =
                    obj_call(p, steal(getattr(src, NB_INTERNED(p, astype))),
                             dtype_str);
                break;

            default:
                break;
        }
    } catch (...) {
        converted.reset();
    }
    return converted;
}

// True if `src` supports the buffer protocol. Non-raising.
static bool obj_has_buffer(PyObject *src, PyTypeObject *tp) noexcept {
#if !defined(Py_LIMITED_API)
    (void) src;
    return tp->tp_as_buffer && tp->tp_as_buffer->bf_getbuffer;
#else
    (void) tp;
    return PyObject_CheckBuffer(src);
#endif
}

// Fetch __dlpack__ as an unbound descriptor (callable with self at args[0]), or
// an invalid object if absent. Avoids exception-related costs if possible.
static object dlpack_method(nb_internals *p, PyTypeObject *tp) noexcept {
#if !defined(Py_LIMITED_API) && PY_VERSION_HEX >= 0x030D0000 && \
    !defined(PYPY_VERSION)
    return steal(_PyType_LookupRef(tp, NB_INTERNED(p, __dlpack__)));
#elif !defined(Py_LIMITED_API)
    return borrow(_PyType_Lookup(tp, NB_INTERNED(p, __dlpack__)));
#else
    object descr =
        steal(PyObject_GetAttr((PyObject *) tp, NB_INTERNED(p, __dlpack__)));
    if (!descr.is_valid()) // can raise
        PyErr_Clear();
    return descr;
#endif
}

bool ndarray_check(nb_internals *p, PyObject *o) noexcept {
    PyTypeObject *tp = Py_TYPE(o);
    if (dlpack_method(p, tp).is_valid() || obj_has_buffer(o, tp))
        return true;

    if (tp == &PyCapsule_Type)
        return true;

    PyObject *name = nb_type_name((PyObject *) tp);
    check(name, "Could not obtain type name! (1)");

    const char *tp_name = PyUnicode_AsUTF8AndSize(name, nullptr);
    check(tp_name, "Could not obtain type name! (2)");

    bool result = false;
    for (int i = 1; i < importer_count; ++i) {
        if (strcmp(tp_name, importers[i].type_name) == 0) {
            result = true;
            break;
        }
    }

    Py_DECREF(name);
    return result;
}

// Helper function reports whether `code` represents a complex number.
static NB_INLINE bool dtype_code_is_complex(uint8_t code) {
    return code == (uint8_t) dlpack::dtype_code::Complex ||
           code == (uint8_t) dlpack::dtype_code::Bcomplex;
}

static ndarray_handle *ndarray_import_impl(nb_internals *p, PyObject *src,
                                           const ndarray_config &cfg,
                                           bool convert,
                                           cleanup_list *cleanup) noexcept {
    object capsule;
    mt_unique_ptr_t mt_unique_ptr(nullptr, &mt_from_buffer_delete);

    // Capsule flavor (versioned or not) to probe for first during extraction.
    // Defaults to unversioned: the right guess for an unknown user capsule.
    bool expect_versioned = false;

    PyTypeObject *tp = Py_TYPE(src);
    const bool src_is_pycapsule = tp == &PyCapsule_Type;

    if (src_is_pycapsule) {
        capsule = borrow(src);
    } else {
        // __dlpack__ is by contract a plain method, so call the looked-up
        // descriptor directly (args[0] is self) rather than re-resolving it.
        object dlpack_descr = dlpack_method(p, tp);

        if (dlpack_descr.is_valid()) {
            PyObject* args[] = {src, NB_INTERNED(p, dl_version_tpl)};
            size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;

            // max_version_kw requests a versioned capsule, nullptr the cheaper
            // unversioned one.
            PyObject *max_version_kw = NB_INTERNED(p, max_version_tpl);
            auto dlpack = [&](PyObject *kwnames) {
                return steal(PyObject_Vectorcall(dlpack_descr.ptr(), args,
                                                 nargsf, kwnames));
            };

            // The unversioned path is generally faster to handle for the target
            // framework. Try that first if the user only requested readonly input.
            capsule = dlpack(cfg.ro ? nullptr : max_version_kw);
            expect_versioned = !cfg.ro;

            // Fall back to the other variant on failure: a read-only source
            // refusing unversioned export raises BufferError, and producers
            // predating max_version (array API < v2023) raise TypeError.
            if (!capsule.is_valid() &&
                (PyErr_ExceptionMatches(PyExc_BufferError) ||
                 PyErr_ExceptionMatches(PyExc_TypeError))) {
                PyErr_Clear();
                capsule = dlpack(cfg.ro ? max_version_kw : nullptr);
                expect_versioned = cfg.ro;
            }

            if (!capsule.is_valid())
                PyErr_Clear();
        }

        // Fall back to the buffer protocol, again gated on a non-raising probe.
        if (!capsule.is_valid() && obj_has_buffer(src, tp))
            mt_unique_ptr = make_mt_from_buffer_protocol(src, cfg.ro);

        // Try the function to_dlpack(), already obsolete in array API v2021
        if (!mt_unique_ptr && !capsule.is_valid()) {
            const char *pkg =
                importers[detect_framework(p, tp)].to_dlpack_pkg;
            if (pkg) {
                try {
                    object package = module_::import_(pkg);
                    if (package.is_valid())
                        capsule = obj_call(
                            p, str_getattr(p, package, "to_dlpack"),
                            handle(src));
                } catch (...) {
                    capsule.reset();
                }
            }
            if (!capsule.is_valid())
                return nullptr;
        }
    }

    void* mt;  // can be versioned or unversioned
    bool versioned;
    if (mt_unique_ptr) {
        mt = mt_unique_ptr.get();
        versioned = true;
    } else {
        // Probe the expected capsule name first
        static const char *names[2] = { "dltensor", "dltensor_versioned" };
        versioned = expect_versioned;
        mt = PyCapsule_GetPointer(capsule.ptr(), names[(int) versioned]);
        if (!mt) {
            PyErr_Clear();
            versioned = !versioned;
            mt = PyCapsule_GetPointer(capsule.ptr(), names[(int) versioned]);
            if (!mt) {
                PyErr_Clear();
                return nullptr;
            }
        }
    }

    dlpack::dltensor& t = (versioned)
                              ? ((managed_dltensor_versioned *) mt)->dltensor
                              : ((managed_dltensor *) mt)->dltensor;

    uint64_t flags = (versioned) ? ((managed_dltensor_versioned *) mt)->flags
                                 : 0UL;

    if (t.ndim < 0 || t.ndim > max_ndim)
        return nullptr;

    // Reject a read-only ndarray if a writable one is required, and
    // reject an ndarray not on the required device.
    if ((!cfg.ro && (flags & dlpack::flag_bitmask_read_only))
        || (cfg.device_type != 0 && t.device.device_type != cfg.device_type)) {
        return nullptr;
    }

    // Check if the ndarray satisfies the remaining requirements.
    bool has_dtype = cfg.dtype != dlpack::dtype(),
         has_shape = cfg.ndim != -1,
         has_order = cfg.order != '\0';

    bool pass_dtype = true, pass_shape = true, pass_order = true;

    if (has_dtype)
        pass_dtype = t.dtype == cfg.dtype;

    if (has_shape) {
        pass_shape = t.ndim == cfg.ndim;
        if (pass_shape) {
            for (int32_t i = 0; i < cfg.ndim; ++i) {
                if (cfg.shape[i] != -1 && t.shape[i] != cfg.shape[i]) {
                    pass_shape = false;
                    break;
                }
            }
        }
    }

    // Only the order check below needs the element count, so skip it otherwise.
    int64_t size = 1;
    if (has_order)
        for (int32_t i = 0; i < t.ndim; ++i)
            size *= t.shape[i];

    // Tolerate any strides if the array has 1 or fewer elements
    if (pass_shape && has_order && size > 1) {
        char order = cfg.order;

        bool c_order = order == 'C' || order == 'A',
             f_order = order == 'F' || order == 'A';

        if (!t.strides) {
            // When the provided tensor does not have a valid
            // strides field, it uses the C ordering convention
            if (c_order) {
                pass_order = true;
            } else {
                int nontrivial_dims = 0;
                for (int i = 0; i < t.ndim; ++i)
                    nontrivial_dims += (int) (t.shape[i] > 1);
                pass_order = nontrivial_dims <= 1;
            }
        } else {
            if (c_order) {
                for (int64_t i = t.ndim - 1, accum = 1; i >= 0; --i) {
                    c_order &= t.shape[i] == 1 || t.strides[i] == accum;
                    accum *= t.shape[i];
                }
            }

            if (f_order) {
                for (int64_t i = 0, accum = 1; i < t.ndim; ++i) {
                    f_order &= t.shape[i] == 1 || t.strides[i] == accum;
                    accum *= t.shape[i];
                }
            }

            pass_order = c_order || f_order;
        }
    }

    // Do not convert shape and do not convert complex numbers to non-complex.
    convert &= pass_shape &
               !(dtype_code_is_complex(t.dtype.code) &&
                 has_dtype && !dtype_code_is_complex(cfg.dtype.code));

    // Support implicit conversion of dtype and order.
    if (convert && (!pass_dtype || !pass_order) && !src_is_pycapsule) {
        int fw = detect_framework(p, tp);

        char order = 'K'; // for NumPy. 'K' means 'keep'
        if (cfg.order)
            order = cfg.order;

        dlpack::dtype dt = has_dtype ? cfg.dtype : t.dtype;
        if (dt.lanes != 1)
            return nullptr;

        char dtype[12];
        if (dt.code == (uint8_t) dlpack::dtype_code::Bool) {
            std::strcpy(dtype, "bool");
        } else {
            const char *prefix = nullptr;
            switch (dt.code) {
                case (uint8_t) dlpack::dtype_code::Int:
                    prefix = "int";
                    break;
                case (uint8_t) dlpack::dtype_code::UInt:
                    prefix = "uint";
                    break;
                case (uint8_t) dlpack::dtype_code::Float:
                    prefix = "float";
                    break;
                case (uint8_t) dlpack::dtype_code::Bfloat:
                    prefix = "bfloat";
                    break;
                case (uint8_t) dlpack::dtype_code::Complex:
                    prefix = "complex";
                    break;
                case (uint8_t) dlpack::dtype_code::Bcomplex:
                    prefix = "bcomplex";
                    break;
                default:
                    return nullptr;
            }
            snprintf(dtype, sizeof(dtype), "%s%u", prefix, dt.bits);
        }

        object converted = convert_array(p, fw, src, dtype, order);

        // Potentially try once again, recursively
        if (converted.is_valid()) {
            ndarray_handle *h =
                ndarray_import_impl(p, converted.ptr(), cfg, false, nullptr);
            if (h && cleanup)
                cleanup->append(converted.release().ptr());
            return h;
        }
    }

    if (!pass_dtype || !pass_shape || !pass_order)
        return nullptr;

    // Create a reference-counted wrapper
    scoped_pymalloc<ndarray_handle> result;
    if (versioned)
        result->mt_versioned = (managed_dltensor_versioned *) mt;
    else
        result->mt_unversioned = (managed_dltensor *) mt;

    result->refcount = 0;
    result->owner = nullptr;
    result->versioned = versioned;
    result->call_deleter = true;
    result->ro = cfg.ro;

    if (src_is_pycapsule) {
        result->self = nullptr;
    } else {
        result->self = Py_NewRef(src);
    }

    // If ndim > 0, ensure that the strides member is initialized.
    if (t.strides || t.ndim == 0) {
        result->free_strides = false;
    } else {
        result->free_strides = true;

        scoped_pymalloc<int64_t> strides((size_t) t.ndim);
        int64_t accum = 1;
        for (int32_t i = t.ndim - 1; i >= 0; --i) {
            strides[(size_t) i] = accum;
            accum *= t.shape[i];
        }
        t.strides = strides.release();
    }

    if (capsule.is_valid()) {
        // Neutralize the producer's capsule so its destructor won't free the
        // DLManagedTensor that nanobind now owns. Clearing the destructor is
        // sufficient and is the only step the common __dlpack__() path needs:
        // nanobind holds the sole reference to that capsule and never re-reads
        // its name. A user-supplied raw capsule, by contrast, is still
        // referenced by the caller, so it is additionally renamed to the
        // conventional "used" name to stop a second import from re-consuming it.
        bool fail = PyCapsule_SetDestructor(capsule.ptr(), nullptr);
        if (src_is_pycapsule && !fail) {
            const char* used_name = (versioned) ? "used_dltensor_versioned"
                                                : "used_dltensor";
            fail = PyCapsule_SetName(capsule.ptr(), used_name);
        }
        if (fail)
            check(false, "ndarray_import(): could not mark capsule as used");
    }

    mt_unique_ptr.release();
    return result.release();
}

ndarray_handle *ndarray_import(nb_internals *p, PyObject *src,
                               const ndarray_config *c,
                               bool convert,
                               cleanup_list *cleanup) noexcept {
    return ndarray_import_impl(p, src, *c, convert, cleanup);
}

dlpack::dltensor *ndarray_inc_ref(ndarray_handle *th) noexcept {
    if (!th)
        return nullptr;
    ++th->refcount;
    return &th->tensor();
}

// Final teardown of a handle whose refcount reached zero.
static void ndarray_dec_ref_free(ndarray_handle *th) noexcept {
    Py_XDECREF(th->owner);
    Py_XDECREF(th->self);
    if (th->versioned) {
        managed_dltensor_versioned *mt = th->mt_versioned;
        if (th->free_strides) {
            PyMem_Free(mt->dltensor.strides);
            mt->dltensor.strides = nullptr;
        }
        if (th->call_deleter) {
            if (mt->deleter)
                mt->deleter(mt);
        } else {
            PyMem_Free(mt);  // This also frees shape and size arrays.
        }
    } else {
        managed_dltensor *mt = th->mt_unversioned;
        if (th->free_strides) {
            PyMem_Free(mt->dltensor.strides);
            mt->dltensor.strides = nullptr;
        }
        assert(th->call_deleter);
        if (mt->deleter)
            mt->deleter(mt);
    }
    PyMem_Free(th);
}

void ndarray_dec_ref(ndarray_handle *th) noexcept {
    if (!th)
        return;
    size_t rc_value = th->refcount--;

    if (rc_value == 0) {
        check(false, "ndarray_dec_ref(): reference count became negative!");
    } else if (rc_value == 1) {
        // Don't run the cleanup if the interpreter has been shut down
        if (cleanup_guard guard{})
            ndarray_dec_ref_free(th);
    }
}

ndarray_handle *ndarray_create(nb_internals *, const ndarray_create_args *a) {
    ndarray_create_args args = *a;

    check(args.ndim <= (size_t) max_ndim,
          "ndarray_create(): ndim is too large!");

    // A comment in the DLPack header file suggests 256-byte alignment of the
    // DLTensor::data field, but this is generally (and necessarily) ignored.
    // Note that the pointer dltensor.data must point to allocated memory
    // (i.e., memory that can be accessed), so it cannot simply be rounded
    // down by zeroing its lowest 8 bits.
    // A byte_offset can be used to support array slicing when data is an
    // opaque device pointer or handle, on which arithmetic is impossible.
    // See also: https://github.com/data-apis/array-api/discussions/779

    if (args.device_type == 0)
        args.device_type = device::cpu::value;

    static_assert(alignof(managed_dltensor_versioned) >= alignof(int64_t));
    scoped_pymalloc<managed_dltensor_versioned> mt(
        1, 2 * sizeof(int64_t) * args.ndim);
    int64_t* shape = nullptr;
    int64_t* strides = nullptr;
    if (args.ndim > 0) {
        shape = new ((void*) (mt.get() + 1)) int64_t[2 * args.ndim];
        strides = shape + args.ndim;
    }

    for (size_t i = 0; i < args.ndim; ++i)
        shape[i] = (int64_t) args.shape[i];

    if (args.ndim > 0) {
        int64_t prod = 1;
        if (args.strides) {
            for (size_t i = 0; i < args.ndim; ++i)
                strides[i] = args.strides[i];
        } else if (args.order == 'F') {
            for (size_t i = 0; i < args.ndim; ++i) {
                strides[i] = prod;
                prod *= (int64_t) args.shape[i];
            }
        } else if (args.order == '\0' || args.order == 'A' ||
                   args.order == 'C') {
            for (ssize_t i = (ssize_t) args.ndim - 1; i >= 0; --i) {
                strides[i] = prod;
                prod *= (int64_t) args.shape[i];
            }
        } else {
            check(false, "ndarray_create(): unknown memory order requested!");
        }
    }

    scoped_pymalloc<ndarray_handle> result;

    mt->version = {dlpack::major_version, dlpack::minor_version};
    mt->manager_ctx = result.get();
    mt->deleter = [](managed_dltensor_versioned *self) {
                      ndarray_dec_ref((ndarray_handle *) self->manager_ctx);
                  };
    mt->flags = (args.ro) ? dlpack::flag_bitmask_read_only : 0;
    mt->dltensor.data = args.data;
    mt->dltensor.device.device_type = args.device_type;
    mt->dltensor.device.device_id = args.device_id;
    mt->dltensor.ndim = (int32_t) args.ndim;
    mt->dltensor.dtype = args.dtype;
    mt->dltensor.shape = shape;
    mt->dltensor.strides = strides;
    mt->dltensor.byte_offset = args.byte_offset;
    result->mt_versioned = mt.release();
    result->refcount = 0;
    result->owner = Py_XNewRef(args.owner);
    result->self = nullptr;
    result->versioned = true;
    result->free_strides = false;
    result->call_deleter = false;
    result->ro = args.ro;
    return result.release();
}

/// Module + attribute of export callables, indexed by `ndarray_export_slot`.
static constexpr struct { const char *pkg, *attr; }
    ndarray_export_spec[nd_export_count] = {
        { "numpy",                          "asarray"     },
        { "numpy",                          "copy"        },
        { "torch.utils.dlpack",             "from_dlpack" },
        { "tensorflow.experimental.dlpack", "from_dlpack" },
        { "jax.dlpack",                     "from_dlpack" },
        { "cupy",                           "from_dlpack" },
        { "mlx.core",                       "array"       },
    };

/// Resolve (and cache) the callable for an ``ndarray_export`` cache slot.
static PyObject *ndarray_export_fn(nb_internals *internals_,
                                   ndarray_export_slot slot) {
    PyObject *fn = internals_->ndarray_export[slot].load_acquire();
    if (NB_LIKELY(fn))
        return fn;

    // Resolve before taking the lock: the import runs arbitrary Python code
    // that may reenter nanobind, and the internals mutex is non-reentrant
    object obj = str_getattr(internals_,
                             steal(module_import(ndarray_export_spec[slot].pkg)),
                             ndarray_export_spec[slot].attr);

    lock_internals guard(internals_);
    fn = internals_->ndarray_export[slot].load_relaxed();
    if (fn)
        return fn;

    fn = obj.release().ptr();
    new_object(internals_, fn);
    internals_->ndarray_export[slot].store_release(fn);
    return fn;
}

PyObject *ndarray_export(nb_internals *p, ndarray_handle *th, int framework,
                         rv_policy policy, cleanup_list *cleanup) noexcept {
    if (!th)
        return none().release().ptr();

    bool copy;
    switch (policy) {
        case rv_policy::reference_internal:
            if (cleanup && cleanup->self() != th->owner && !th->self) {
                if (th->owner) {
                    PyErr_SetString(PyExc_RuntimeError,
                                    "nanobind::detail::ndarray_export(): "
                                    "reference_internal policy cannot be "
                                    "applied (ndarray already has an owner)");
                    return nullptr;
                } else {
                    th->owner = Py_NewRef(cleanup->self());
                }
            }
            [[fallthrough]];

        case rv_policy::automatic:
        case rv_policy::automatic_reference:
            copy = th->owner == nullptr && th->self == nullptr;
            break;

        case rv_policy::copy:
        case rv_policy::move:
            copy = true;
            break;

        default:
            copy = false;
            break;
    }

    if (!copy) {
        if (th->self) {
            return Py_NewRef(th->self);
        } else if (policy == rv_policy::none) {
            return nullptr;
        }
    }

    // These frameworks export a raw DLPack capsule or buffer view rather than
    // a framework array with a copy method, so the requested copy cannot be
    // performed. Refuse the cast rather than returning a view that would
    // alias (and possibly outlive) the original storage.
    if (copy && !th->self &&
        (framework == no_framework::value || framework == tensorflow::value ||
         framework == memview::value || framework == array_api::value)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "nanobind::detail::ndarray_export(): copying the "
                        "array contents is not supported for this framework; "
                        "please specify an 'owner' so that the array can be "
                        "returned without a copy.");
        return nullptr;
    }

    nb_internals *internals_ = p;

    object o;
    if (copy && framework == no_framework::value && th->self) {
        o = borrow(th->self);
    } else if (framework == no_framework::value ||
               framework == tensorflow::value) {
        // Make a new capsule wrapping an unversioned managed_dltensor.
        o = steal(th->make_capsule<false>());
    } else {
        // Make a Python object providing the buffer interface and having
        // the two DLPack methods __dlpack__() and __dlpack_device__().
        nb_ndarray *h = PyObject_New(nb_ndarray, nb_ndarray_tp(internals_));
        if (!h)
            return nullptr;
        h->th = th;
        h->internals = internals_;
        ndarray_inc_ref(th);
        o = steal((PyObject *) h);
    }

    if (framework == numpy::value) {
        try {
            // Call nump.asarray(o) to create a view, and numpy.copy(o) to copy
            PyObject *export_fn = ndarray_export_fn(
                internals_, copy ? nd_export_numpy_copy : nd_export_numpy_view);
            PyObject *stack[] = {nullptr, o.ptr()};
            size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
            return PyObject_Vectorcall(export_fn, stack + 1, nargsf, nullptr);
        } catch (const std::exception &e) {
            PyErr_Format(PyExc_TypeError,
                         "could not export nanobind::ndarray: %s", e.what());
            return nullptr;
        }
    }

    // The DLPack frameworks build a view via <pkg>.from_dlpack(o); no_framework
    // and array_api leave `o` as-is; memview returns a memoryview directly.
    try {
        ndarray_export_slot slot;
        switch (framework) {
            case pytorch::value:    slot = nd_export_pytorch;    break;
            case tensorflow::value: slot = nd_export_tensorflow; break;
            case jax::value:        slot = nd_export_jax;        break;
            case cupy::value:       slot = nd_export_cupy;       break;
            case mlx::value:        slot = nd_export_mlx;        break;
            case memview::value:    return PyMemoryView_FromObject(o.ptr());
            default:                slot = nd_export_count;  // no export call
        }

        if (slot != nd_export_count) {
            PyObject *export_fn = ndarray_export_fn(internals_, slot);
            PyObject *stack[] = {nullptr, o.ptr()};
            size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
            o = steal(PyObject_Vectorcall(export_fn, stack + 1, nargsf, nullptr));
            if (!o.is_valid())
                return nullptr;
        }
    } catch (const std::exception &e) {
        PyErr_Format(PyExc_TypeError,
                     "could not export nanobind::ndarray: %s",
                     e.what());
        return nullptr;
    }

    // MLX has no copy()/clone() method; mlx.core.array() already returned an
    // owned copy, so the copy policy is satisfied without an extra step.
    if (copy && framework != mlx::value) {
        PyObject* copy_fn_name = framework == pytorch::value
                                     ? NB_INTERNED(p, clone)
                                     : NB_INTERNED(p, copy);
        try {
            o = obj_call(p, steal(getattr(o.ptr(), copy_fn_name)));
        } catch (std::exception &e) {
            PyErr_Format(PyExc_RuntimeError,
                         "copying nanobind::ndarray failed: %s",
                         e.what());
            return nullptr;
        }
    }

    return o.release().ptr();
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
