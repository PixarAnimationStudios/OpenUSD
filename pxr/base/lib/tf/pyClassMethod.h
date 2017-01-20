//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef TF_PY_CLASS_METHOD_H
#define TF_PY_CLASS_METHOD_H

#include "pxr/pxr.h"

#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/object.hpp>
#include <boost/python/def_visitor.hpp>

PXR_NAMESPACE_OPEN_SCOPE

namespace Tf_PyClassMethod {

using namespace boost::python;

// Visitor for wrapping functions as Python class methods.
// See typedef below for docs.
// This is very simliar to the staticmethod() method on boost::python::class,
// except it uses PyClassMethod_New() instead of PyStaticMethod_New().
struct _TfPyClassMethod : def_visitor<_TfPyClassMethod>
{
    friend class def_visitor_access;

    _TfPyClassMethod(const std::string &methodName) :
        _methodName(methodName) {}
    explicit _TfPyClassMethod(const char *methodName) :
        _methodName(methodName) {}

    template <typename CLS>
    void visit(CLS &c) const
    {
        PyTypeObject* self = downcast<PyTypeObject>( c.ptr() );
        dict d((handle<>(borrowed(self->tp_dict))));

        object method(d[_methodName]);

        c.attr(_methodName.c_str()) = object(
            handle<>( PyClassMethod_New((_CallableCheck)(method.ptr()) )));
    }

private:

    PyObject* _CallableCheck(PyObject* callable) const
    {
        if (PyCallable_Check(expect_non_null(callable)))
            return callable;

        PyErr_Format( PyExc_TypeError,
           "classmethod expects callable object; got an object of type %s, "
           "which is not callable",
           callable->ob_type->tp_name);

        throw_error_already_set();
        return 0;
    }

    const std::string _methodName;
};

}

/// A boost.python class visitor which replaces the named method with a
/// classmethod()-wrapped one.
///
/// \code
///    void Foo( boost::python::object & pyClassObject ) { /* ... */ }
///    
///    class_<...>(...)
///         .def("Foo", &Foo)
///         .def(TfPyClassMethod("Foo"))
///         ;
/// \endcode
///
typedef Tf_PyClassMethod::_TfPyClassMethod TfPyClassMethod;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_PY_CLASS_METHOD_H
