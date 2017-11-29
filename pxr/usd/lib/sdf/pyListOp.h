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
#ifndef SDF_PYLISTOP_H
#define SDF_PYLISTOP_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include <boost/python.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfPyWrapListOp
///
/// Helper class for wrapping SdfListOp objects for Python. The template
/// parameter is the specific SdfListOp type being wrapped (e.g.,
/// SdfPathListOp)
///
template <class T>
class SdfPyWrapListOp {
public:
    typedef typename T::ItemType   ItemType;
    typedef typename T::ItemVector ItemVector;

    typedef SdfPyWrapListOp<T> This;

    SdfPyWrapListOp(const std::string& name)
    {
        TfPyWrapOnce<T>([name]() { SdfPyWrapListOp::_Wrap(name); });
    }
 
private:
    static ItemVector _ApplyOperations1(const T& listOp, ItemVector input) {
        ItemVector result = input;
        listOp.ApplyOperations(&result);
        return result;
    }
    static boost::python::object
    _ApplyOperations2(const T& outer, const T& inner) {
        if (boost::optional<T> r = outer.ApplyOperations(inner)) {
            return boost::python::object(*r);
        } else {
            return boost::python::object();
        }
    }

    static void _Wrap(const std::string& name)
    {
        using namespace boost::python;

        class_<T>(name.c_str())
            .def("__str__", &This::_GetStr)

            .def(self == self)
            .def(self != self)

            .def("Clear", &T::Clear)
            .def("ClearAndMakeExplicit", &T::ClearAndMakeExplicit)
            .def("ApplyOperations", &This::_ApplyOperations1)
            .def("ApplyOperations", &This::_ApplyOperations2)

            .add_property("explicitItems",
                make_function(&T::GetExplicitItems,
                              return_value_policy<return_by_value>()),
                &T::SetExplicitItems)
            .add_property("addedItems",
                make_function(&T::GetAddedItems,
                              return_value_policy<return_by_value>()),
                &T::SetAddedItems)
            .add_property("prependedItems",
                make_function(&T::GetPrependedItems,
                              return_value_policy<return_by_value>()),
                &T::SetPrependedItems)
            .add_property("appendedItems",
                make_function(&T::GetAppendedItems,
                              return_value_policy<return_by_value>()),
                &T::SetAppendedItems)
            .add_property("deletedItems",
                make_function(&T::GetDeletedItems,
                              return_value_policy<return_by_value>()),
                &T::SetDeletedItems)
            .add_property("orderedItems",
                make_function(&T::GetOrderedItems,
                              return_value_policy<return_by_value>()),
                &T::SetOrderedItems)
            .def("GetAddedOrExplicitItems",
                &This::_GetAddedOrExplicitItems)

            .add_property("isExplicit", &T::IsExplicit)

            ;

    }

    static std::string _GetStr(const T& listOp)
    {
        return boost::lexical_cast<std::string>(listOp);
    }
    
    static 
    ItemVector _GetAddedOrExplicitItems(const T& listOp)
    {
        ItemVector result;
        listOp.ApplyOperations(&result);
        return result;
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_PYLISTOP_H
