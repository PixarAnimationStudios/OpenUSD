#include <nanobind/nanobind.h>
#include "nb_ft.h"

#if defined(Py_GIL_DISABLED)
/// Make an object immortal when targeting free-threaded Python
void make_immortal(PyObject *op) noexcept {
    // See CPython's Objects/object.c
    if (PyObject_IS_GC(op))
        PyObject_GC_UnTrack(op);
    op->ob_tid = _Py_UNOWNED_TID;
    op->ob_ref_local = _Py_IMMORTAL_REFCNT_LOCAL;
    op->ob_ref_shared = 0;
}

#if PY_VERSION_HEX < 0x030E00A5
void nb_enable_try_inc_ref(PyObject *obj) noexcept {
    // Since this is called during object construction, we know that we have
    // the only reference to the object and can use a non-atomic write.
    assert(obj->ob_ref_shared == 0);
    obj->ob_ref_shared = _Py_REF_MAYBE_WEAKREF;
}

bool nb_try_inc_ref(PyObject *obj) noexcept {
    // See https://github.com/python/cpython/blob/d05140f9f77d7dfc753dd1e5ac3a5962aaa03eff/Include/internal/pycore_object.h#L761
    uint32_t local = _Py_atomic_load_uint32_relaxed(&obj->ob_ref_local);
    local += 1;
    if (local == 0) {
        // immortal
        return true;
    }
    if (_Py_IsOwnedByCurrentThread(obj)) {
        _Py_atomic_store_uint32_relaxed(&obj->ob_ref_local, local);
#ifdef Py_REF_DEBUG
        _Py_INCREF_IncRefTotal();
#endif
        return true;
    }
    Py_ssize_t shared = _Py_atomic_load_ssize_relaxed(&obj->ob_ref_shared);
    for (;;) {
        // If the shared refcount is zero and the object is either merged
        // or may not have weak references, then we cannot incref it.
        if (shared == 0 || shared == _Py_REF_MERGED) {
            return false;
        }

        if (_Py_atomic_compare_exchange_ssize(
                &obj->ob_ref_shared, &shared, shared + (1 << _Py_REF_SHARED_SHIFT))) {
#ifdef Py_REF_DEBUG
            _Py_INCREF_IncRefTotal();
#endif
            return true;
        }
    }
}
#endif
#endif
