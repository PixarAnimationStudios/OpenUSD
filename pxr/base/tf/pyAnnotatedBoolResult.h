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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>
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
    static nanobind::class_<Derived>
    Wrap(char const *name, char const *annotationName) {
        return WrapNb<Derived>(TfPyGetCurrentNanobindModule(), name, annotationName);
    }

    template <class Derived>
    static nanobind::class_<Derived>
    Wrap(nanobind::handle scope, char const *name, char const *annotationName) {
        return WrapNb<Derived>(scope, name, annotationName);
    }

    template <class Derived>
    static nanobind::class_<Derived>
    WrapNb(nanobind::handle scope, char const *name, char const *annotationName) {
        typedef TfPyAnnotatedBoolResult<Annotation> This;
        TfPyLock lock;
        auto cls = nanobind::class_<Derived>(scope, name)
            .def(nanobind::init<bool, Annotation>())
            .def("__bool__", &Derived::GetValue)
            .def("__repr__", &Derived::GetRepr)
            .def("__eq__", [](const Derived &self, bool rhs) { return self == rhs; })
            .def("__ne__", [](const Derived &self, bool rhs) { return self != rhs; })
            .def_prop_ro(annotationName, &This::_GetAnnotation<Derived>)
            .def("__getitem__", [](const Derived &x, int i) -> nanobind::object {
                if (i == 0) {
                    return nanobind::cast(x._val);
                }
                if (i == 1) {
                    return nanobind::cast(x._annotation);
                }
                throw nanobind::index_error("Index must be 0 or 1.");
            });
        Py_INCREF(cls.ptr());
        return cls;
    }

    using AnnotationType = Annotation;

private:
    // Helper function for wrapper.
    template <class Derived>
    static Annotation _GetAnnotation(const Derived& x)
    {
        return x.GetAnnotation();
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
