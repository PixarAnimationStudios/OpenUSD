//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_ANNOTATED_BOOL_RESULT_H
#define PXR_BASE_TF_PY_ANNOTATED_BOOL_RESULT_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_by_value.hpp"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

template <class Annotation>
struct TfPyAnnotatedBoolResult
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

    friend bool operator==(bool lhs, const TfPyAnnotatedBoolResult& rhs) {
        return rhs == lhs;
    }

    friend bool operator!=(const TfPyAnnotatedBoolResult& lhs, bool rhs) {
        return !(lhs == rhs);
    }

    friend bool operator!=(bool lhs, const TfPyAnnotatedBoolResult& rhs) {
        return !(lhs == rhs);
    }

    template <class Derived>
    static pxr_boost::python::class_<Derived>
    Wrap(char const *name, char const *annotationName) {
        typedef TfPyAnnotatedBoolResult<Annotation> This;
        using namespace pxr_boost::python;
        TfPyLock lock;
        return class_<Derived>(name, init<bool, Annotation>())
            .def("__bool__", &Derived::GetValue)
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
    static pxr_boost::python::object _GetItem(const Derived& x, int i)
    {
        if (i == 0) {
            return pxr_boost::python::object(x._val);
        }
        if (i == 1) {
            return pxr_boost::python::object(x._annotation);
        }
        
        PyErr_SetString(PyExc_IndexError, "Index must be 0 or 1.");
        pxr_boost::python::throw_error_already_set();

        return pxr_boost::python::object();
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
