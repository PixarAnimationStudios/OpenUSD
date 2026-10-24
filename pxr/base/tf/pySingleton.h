//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_SINGLETON_H
#define PXR_BASE_TF_PY_SINGLETON_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakPtr.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace Tf_PySingleton {

template <class T>
TfWeakPtr<T> GetWeakPtr(T &t) {
    return TfCreateWeakPtr(&t);
}

template <class T>
TfWeakPtr<T> GetWeakPtr(T const &t) {
    // cast away constness for python...
    return TfConst_cast<TfWeakPtr<T> >(TfCreateWeakPtr(&t));
}

template <class T>
TfWeakPtr<T> GetWeakPtr(TfWeakPtr<T> const &t) {
    return t;
}

}

template <typename T, typename... Extra>
void TfNbSingleton(nanobind::class_<T, Extra...> &cls) {
    TfNbWeakPtr<T>(cls);
    cls.def_static(
        "__new__",
        [](nanobind::handle, nanobind::args, nanobind::kwargs) {
            return Tf_PySingleton::GetWeakPtr(T::GetInstance());
        });
    cls.def("__init__",
            [](nanobind::handle, nanobind::args, nanobind::kwargs) {});
}

struct Tf_NbSingletonVisitor : nanobind::def_visitor<Tf_NbSingletonVisitor> {
    template <typename Class, typename... Extra>
    static void execute(Class &cls, const Extra &...) {
        TfNbSingleton(cls);
    }
};

inline Tf_NbSingletonVisitor TfNbSingleton() { return {}; }
inline Tf_NbSingletonVisitor TfPySingleton() { return {}; }

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_SINGLETON_H
