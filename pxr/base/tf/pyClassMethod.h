//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_CLASS_METHOD_H
#define PXR_BASE_TF_PY_CLASS_METHOD_H

#include "pxr/pxr.h"
#include "pxr/base/tf/pySafePython.h"

#include <nanobind/nanobind.h>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace Tf_PyClassMethod {

// Visitor for wrapping functions as Python class methods.
// See typedef below for docs.
struct _TfPyClassMethod : nanobind::def_visitor<_TfPyClassMethod>
{
    _TfPyClassMethod(const std::string &methodName) :
        _methodName(methodName) {}
    explicit _TfPyClassMethod(const char *methodName) :
        _methodName(methodName) {}

    template <typename Class, typename... Extra>
    void execute(Class &c, const Extra &...) const
    {
        nanobind::object method = nanobind::getattr(c, _methodName.c_str());
        nanobind::object cm = nanobind::steal(
            PyClassMethod_New(_CallableCheck(method.ptr())));
        nanobind::setattr(c, _methodName.c_str(), cm);
    }

private:

    PyObject* _CallableCheck(PyObject* callable) const
    {
        if (callable && PyCallable_Check(callable))
            return callable;

        PyErr_Format(PyExc_TypeError,
           "classmethod expects callable object; got an object of type %s, "
           "which is not callable",
           callable ? Py_TYPE(callable)->tp_name : "None");

        throw nanobind::python_error();
    }

    const std::string _methodName;
};

}

/// A class visitor which replaces the named method with a
/// classmethod()-wrapped one.
typedef Tf_PyClassMethod::_TfPyClassMethod TfPyClassMethod;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_CLASS_METHOD_H
