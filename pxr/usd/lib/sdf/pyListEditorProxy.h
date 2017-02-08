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
#ifndef SDF_PYLISTEDITORPROXY_H
#define SDF_PYLISTEDITORPROXY_H

/// \file sdf/pyListEditorProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/listEditorProxy.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/pyListProxy.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include <boost/python.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class Sdf_PyListEditorUtils {
public:
    template <class T, class V>
    class ApplyHelper {
    public:
        ApplyHelper(const T& owner, const boost::python::object& callback) :
            _owner(owner),
            _callback(callback)
        {
            // Do nothing
        }

        boost::optional<V> operator()(SdfListOpType op, const V& value)
        {
            using namespace boost::python;

            TfPyLock pyLock;
            object result = _callback(_owner, value, op);
            if (not TfPyIsNone(result)) {
                extract<V> e(result);
                if (e.check()) {
                    return boost::optional<V>(e());
                }
                else {
                    TF_CODING_ERROR("ApplyEditsToList callback has "
                                    "incorrect return type.");
                }
            }
            return boost::optional<V>();
        }

    private:
        const T& _owner;
        TfPyCall<boost::python::object> _callback;
    };

    template <class V>
    class ModifyHelper {
    public:
        ModifyHelper(const boost::python::object& callback) :
            _callback(callback)
        {
            // Do nothing
        }

        boost::optional<V> operator()(const V& value)
        {
            using namespace boost::python;

            TfPyLock pyLock;
            object result = _callback(value);
            if (not TfPyIsNone(result)) {
                extract<V> e(result);
                if (e.check()) {
                    return boost::optional<V>(e());
                }
                else {
                    TF_CODING_ERROR("ModifyItemEdits callback has "
                                    "incorrect return type.");
                }
            }
            return boost::optional<V>();
        }

    private:
        TfPyCall<boost::python::object> _callback;
    };
};

template <class T>
class SdfPyWrapListEditorProxy {
public:
    typedef T Type;
    typedef typename Type::TypePolicy TypePolicy;
    typedef typename Type::value_type value_type;
    typedef typename Type::value_vector_type value_vector_type;
    typedef typename Type::ApplyCallback ApplyCallback;
    typedef typename Type::ModifyCallback ModifyCallback;
    typedef SdfPyWrapListEditorProxy<Type> This;
    typedef SdfListProxy<TypePolicy> ListProxy;

    SdfPyWrapListEditorProxy()
    {
        TfPyWrapOnce<Type>(&This::_Wrap);
        SdfPyWrapListProxy<ListProxy>();
    }

private:
    static void _Wrap()
    {
        using namespace boost::python;

        class_<Type>(_GetName().c_str(), no_init)
            .def("__str__", &This::_GetStr)
            .add_property("isExpired", &Type::IsExpired)
            .add_property("explicitItems",
                &Type::GetExplicitItems,
                &This::_SetExplicitProxy)
            .add_property("addedItems",
                &Type::GetAddedItems,
                &This::_SetAddedProxy)
            .add_property("deletedItems",
                &Type::GetDeletedItems,
                &This::_SetDeletedProxy)
            .add_property("orderedItems",
                &Type::GetOrderedItems,
                &This::_SetOrderedProxy)
            .add_property("addedOrExplicitItems",
                &Type::GetAddedOrExplicitItems)
            .add_property("isExplicit", &Type::IsExplicit)
            .add_property("isOrderedOnly", &Type::IsOrderedOnly)
            .def("ApplyEditsToList",
                &This::_ApplyEditsToList,
                return_value_policy<TfPySequenceToList>())
            .def("ApplyEditsToList",
                &This::_ApplyEditsToList2,
                return_value_policy<TfPySequenceToList>())

            .def("CopyItems", &Type::CopyItems)
            .def("ClearEdits", &Type::ClearEdits)
            .def("ClearEditsAndMakeExplicit", &Type::ClearEditsAndMakeExplicit)
            .def("ContainsItemEdit", &Type::ContainsItemEdit,
                 (arg("item"), arg("onlyAddOrExplicit")=false))
            .def("RemoveItemEdits", &Type::RemoveItemEdits)
            .def("ReplaceItemEdits", &Type::ReplaceItemEdits)
            .def("ModifyItemEdits", &This::_ModifyEdits)

            // New API (see bug 8710)
            .def("Add", &Type::Add)
            .def("Remove", &Type::Remove)
            .def("Erase", &Type::Erase)
            ;
    }

    static std::string _GetName()
    {
        std::string name = "ListEditorProxy_" +
                           ArchGetDemangled<TypePolicy>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    static std::string _GetStr(const Type& x)
    {
        return x._listEditor ? 
            boost::lexical_cast<std::string>(*x._listEditor) : std::string();
    }

    static void _SetExplicitProxy(Type& x, const value_vector_type& v)
    {
        x.GetExplicitItems() = v;
    }

    static void _SetAddedProxy(Type& x, const value_vector_type& v)
    {
        x.GetAddedItems() = v;
    }

    static void _SetDeletedProxy(Type& x, const value_vector_type& v)
    {
        x.GetDeletedItems() = v;
    }

    static void _SetOrderedProxy(Type& x, const value_vector_type& v)
    {
        x.GetOrderedItems() = v;
    }

    static value_vector_type _ApplyEditsToList(const Type& x,
                                               const value_vector_type& v)
    {
        value_vector_type tmp = v;
        x.ApplyEditsToList(&tmp);
        return tmp;
    }

    static value_vector_type _ApplyEditsToList2(const Type& x,
                                                const value_vector_type& v,
                                                const boost::python::object& cb)
    {
        value_vector_type tmp = v;
        x.ApplyEditsToList(&tmp,
            Sdf_PyListEditorUtils::ApplyHelper<Type, value_type>(x, cb));
        return tmp;
    }

    static void _ModifyEdits(Type& x, const boost::python::object& cb)
    {
        x.ModifyItemEdits(Sdf_PyListEditorUtils::ModifyHelper<value_type>(cb));
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_PYLISTEDITORPROXY_H
