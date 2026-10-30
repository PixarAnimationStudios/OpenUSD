//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_PY_BUFFER_UTILS_H
#define PXR_BASE_GF_PY_BUFFER_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/api.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/tf/pySafePython.h"

#include <nanobind/nanobind.h>

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// Format strings matching Python buffer proto / struct module scheme.

// This function template is explicitly instantiated for T =
//    bool, [unsigned] (char, short, int, long), half, float, and double.
template <class T>
char *Gf_GetPyBufferFmtFor();

PXR_NAMESPACE_CLOSE_SCOPE

namespace nanobind {
namespace detail {

template <typename T>
struct type_caster<T, enable_if_t<std::is_same_v<T, PXR_NS::GfHalf>>> {
    NB_TYPE_CASTER(T, const_name("float"))

    bool from_python(handle src, uint8_t, cleanup_list *) noexcept {
        PyObject *flt = PyNumber_Float(src.ptr());
        if (!flt) {
            PyErr_Clear();
            return false;
        }
        double d = PyFloat_AsDouble(flt);
        Py_DECREF(flt);
        if (PyErr_Occurred()) {
            PyErr_Clear();
            return false;
        }
        value = T(static_cast<float>(d));
        return true;
    }

    static handle from_cpp(T src, rv_policy, cleanup_list *) noexcept {
        return PyFloat_FromDouble(src);
    }
};

} // namespace detail
} // namespace nanobind

#endif // PXR_BASE_GF_PY_BUFFER_UTILS_H
