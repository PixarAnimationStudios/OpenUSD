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
#ifndef PXR_BASE_TF_PY_ANNOTATED_BOOL_RESULT_H
#define PXR_BASE_TF_PY_ANNOTATED_BOOL_RESULT_H

#include "pxr/pxr.h"

#include "pxr/base/tf/py3Compat.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/operators.hpp>
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_by_value.hpp>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

template <class Annotation>
struct TfPyAnnotatedBoolResult :
    boost::equality_comparable<TfPyAnnotatedBoolResult<Annotation>, bool>
{
    TfPyAnnotatedBoolResult() {}
    
    TfPyAnnotatedBoolResult(bool val, Annotation const &annotation) :
        _val(val), _annotation(annotation) {}

    bool GetValue() const {
        return _val;
    }

    Annotation const &GetAnnotation() const {
        return _annotation;
    }

    std::string GetRepr() const {
        return GetValue() ? "True" :
            "(False, " + TfPyRepr(GetAnnotation()) + ")";
    }

    /// Returns \c true if the result is the same as \p rhs.
    bool operator==(bool rhs) const {
        return _val == rhs;
    }

    template <class Derived>
    static boost::python::class_<Derived>
    Wrap(char const *name, char const *annotationName) {
        typedef TfPyAnnotatedBoolResult<Annotation> This;
        using namespace boost::python;
        TfPyLock lock;
        return class_<Derived>(name, no_init)
            .def(TfPyBoolBuiltinFuncName, &Derived::GetValue)
            .def("__repr__", &Derived::GetRepr)
            .def(self == bool())
            .def(self != bool())
            .def(bool() == self)
            .def(bool() != self)
            // Use a helper function.  We'd like to def_readonly the
            // _annotation member but there are two problems with that.
            // First, we can't control the return_value_policy and if the
            // Annotation type has a custom to-Python converter then the
            // def_readonly return_value_policy of return_internal_reference
            // won't work since the object needs conversion.  Second, if we
            // try to use GetAnnotation() with add_property then we'll get
            // a failure at runtime because Python has a Derived but
            // GetAnnotation takes a TfPyAnnotatedBoolResult<Annotation>
            // and boost python doesn't know the former is-a latter because
            // TfPyAnnotatedBoolResult<Annotation> is not wrapped.
            //
            // So we provide a templated static method that takes a Derived
            // and returns Annotation by value.  We can add_property that
            // with no problem.
            .add_property(annotationName, &This::_GetAnnotation<Derived>)
            .def("__getitem__", &This::_GetItem<Derived>)
            ;
    }

    using AnnotationType = Annotation;

private:
    // Helper function for wrapper.
    template <class Derived>
    static Annotation _GetAnnotation(const Derived& x)
    {
        return x.GetAnnotation();
    }

    template <class Derived>
    static boost::python::object _GetItem(const Derived& x, int i)
    {
        if (i == 0) {
            return boost::python::object(x._val);
        }
        if (i == 1) {
            return boost::python::object(x._annotation);
        }
        
        PyErr_SetString(PyExc_IndexError, "Index must be 0 or 1.");
        boost::python::throw_error_already_set();

        return boost::python::object();
    }

private:
    bool _val;
    Annotation _annotation;

};

/// Returns \c true if the result of \p lhs is the same as \p rhs.
template <class Annotation>
bool operator==(bool lhs, TfPyAnnotatedBoolResult<Annotation>& rhs)
{
    return rhs == lhs;
}

/// Returns \c false if the result of \p lhs is the same as \p rhs.
template <class Annotation>
bool operator!=(bool lhs, TfPyAnnotatedBoolResult<Annotation>& rhs)
{
    return rhs != lhs;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_ANNOTATED_BOOL_RESULT_H
