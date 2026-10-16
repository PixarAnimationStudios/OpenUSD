//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_WRAP_TYPE_HELPERS_H
#define PXR_BASE_TF_WRAP_TYPE_HELPERS_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include <nanobind/nanobind.h>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// \struct TfTypePythonClass
/// A nanobind visitor that associates the Python class object
/// created by the wrapping with the TfType of the C++ type being wrapped.
///
/// Example use:
/// \code
/// class_<Foo, ...>("Foo", ...)
///     .def( TfTypePythonClass() )
/// \endcode
///
struct TfTypePythonClass
    : public nanobind::def_visitor<TfTypePythonClass> {
    template <typename T, typename... Ts>
    static void execute(nanobind::class_<T, Ts...> &c) {
        if (TfType t = TfType::Find<T>()) {
            if (!t.GetPythonClass().ptr() || t.GetPythonClass().ptr() == Py_None) {
                t.DefinePythonClass(TfPyObjWrapper(
                    nanobind::borrow<nanobind::object>(c.ptr())));
            }
        }
        if (PyObject_HasAttrString(c.ptr(), "__getinitargs__") &&
            !PyObject_HasAttrString(c.ptr(), "__safe_for_unpickling__")) {
            c.attr("__safe_for_unpickling__") = true;
            c.def("__reduce__", [](nanobind::handle self) {
                nanobind::object initargs = self.attr("__getinitargs__")();
                if (PyObject_HasAttrString(self.ptr(), "__dict__")) {
                    nanobind::object d = self.attr("__dict__");
                    if (nanobind::len(d) > 0) {
                        return nanobind::make_tuple(self.type(), initargs, d);
                    }
                }
                return nanobind::make_tuple(self.type(), initargs);
            });
        }
        if constexpr (std::is_copy_constructible_v<T>) {
        }
    }
};

using TfNbTypePythonClass = TfTypePythonClass;

/// A helper for wrapping C++ types.
/// This method defines a TfType for the given python class object, and also
/// recursively defines TfTypes for all the Python bases if necessary.
TF_API TfType TfType_DefinePythonTypeAndBases( const nanobind::object & classObj );

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_WRAP_TYPE_HELPERS_H
