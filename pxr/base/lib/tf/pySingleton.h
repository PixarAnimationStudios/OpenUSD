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
#ifndef TF_PYSINGLETON_H
#define TF_PYSINGLETON_H

#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakPtr.h"

#include <boost/bind.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/python/class.hpp>
#include <boost/python/default_call_policies.hpp>
#include <boost/python/def_visitor.hpp>
#include <boost/python/raw_function.hpp>

#include <string>

namespace Tf_PySingleton {

namespace bp = boost::python;

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

std::string _Repr(bp::object const &self, std::string const &prefix);
    
struct Visitor : bp::def_visitor<Visitor> {
    explicit Visitor(std::string const &reprPrefix = std::string()) :
        _reprPrefix(reprPrefix) {}
    
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

        // If they supplied a repr prefix, provide a repr implementation.
        if (not _reprPrefix.empty())
            c.def("__repr__",
                  make_function(boost::bind(_Repr, _1, _reprPrefix),
                                bp::default_call_policies(),
                                boost::mpl::vector<std::string,
                                                   bp::object const &>()));
    }
private:
    std::string _reprPrefix;
};

}

Tf_PySingleton::Visitor TfPySingleton();
Tf_PySingleton::Visitor TfPySingleton(std::string const &reprPrefix);

#endif // TF_PYSINGLETON_H
