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

#include "pxr/external/boost/python/def_visitor.hpp"
#include "pxr/external/boost/python/raw_function.hpp"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace Tf_PySingleton {

namespace bp = pxr_boost::python;

TF_API
bp::object _DummyInit(bp::tuple const & /* args */,
                      bp::dict const & /* kw */);

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
   
template <typename PtrType>
PtrType _GetSingletonWeakPtr(bp::object const & /* classObj */) {
    typedef typename PtrType::DataType Singleton;
    return GetWeakPtr(Singleton::GetInstance());
}

TF_API
std::string _Repr(bp::object const &self, std::string const &prefix);
    
struct Visitor : bp::def_visitor<Visitor> {
    explicit Visitor() {}
    
    friend class bp::def_visitor_access;
    template <typename CLS>
    void visit(CLS &c) const {
        typedef typename CLS::metadata::held_type PtrType;

        // Singleton implies WeakPtr.
        c.def(TfPyWeakPtr());

        // Wrap __new__ to return a weak pointer to the singleton instance.
        c.def("__new__", _GetSingletonWeakPtr<PtrType>).staticmethod("__new__");
        // Make __init__ do nothing.
        c.def("__init__", bp::raw_function(_DummyInit));
    }
};

}

TF_API
Tf_PySingleton::Visitor TfPySingleton();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_SINGLETON_H
