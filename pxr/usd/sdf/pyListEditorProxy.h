//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PY_LIST_EDITOR_PROXY_H
#define PXR_USD_SDF_PY_LIST_EDITOR_PROXY_H

/// \file sdf/pyListEditorProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/listEditorProxy.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/pyListProxy.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_OPEN_SCOPE

class Sdf_PyListEditorUtils {
public:
    template <class T, class V>
    class ApplyHelper {
    public:
        ApplyHelper(const T& owner, const nanobind::object& callback) :
            _owner(owner),
            _callback(callback)
        {
            // Do nothing
        }

        std::optional<V> operator()(SdfListOpType op, const V& value)
        {
            TfPyLock pyLock;
            nanobind::object result = _callback(_owner, value, op);
            if (! TfPyIsNone(result)) {
                V extracted;
                if (nanobind::try_cast<V>(result, extracted)) {
                    return std::optional<V>(extracted);
                }
                else {
                    TF_CODING_ERROR("ApplyEditsToList callback has "
                                    "incorrect return type.");
                }
            }
            return std::optional<V>();
        }

    private:
        const T& _owner;
        TfPyCall<nanobind::object> _callback;
    };

    template <class V>
    class ModifyHelper {
    public:
        ModifyHelper(const nanobind::object& callback) :
            _callback(callback)
        {
            // Do nothing
        }

        std::optional<V> operator()(const V& value)
        {
            TfPyLock pyLock;
            nanobind::object result = _callback(value);
            if (! TfPyIsNone(result)) {
                V extracted;
                if (nanobind::try_cast<V>(result, extracted)) {
                    return std::optional<V>(extracted);
                }
                else {
                    TF_CODING_ERROR("ModifyItemEdits callback has "
                                    "incorrect return type.");
                }
            }
            return std::optional<V>();
        }

    private:
        TfPyCall<nanobind::object> _callback;
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
        namespace nb = nanobind;

        nb::handle scope = TfPyGetCurrentNanobindModule();
        if (!scope.is_valid()) {
            scope = nb::borrow(PyImport_AddModule("pxr.Sdf"));
        }

        nb::class_<Type>(scope, _GetName().c_str())
            .def("__str__", &This::_GetStr)
            .def_prop_ro("isExpired", &Type::IsExpired)
            .def_prop_rw("explicitItems",
                &Type::GetExplicitItems,
                &This::_SetExplicitProxy)
            .def_prop_rw("addedItems",
                &Type::GetAddedItems,
                &This::_SetAddedProxy)
            .def_prop_rw("prependedItems",
                &Type::GetPrependedItems,
                &This::_SetPrependedProxy)
            .def_prop_rw("appendedItems",
                &Type::GetAppendedItems,
                &This::_SetAppendedProxy)
            .def_prop_rw("deletedItems",
                &Type::GetDeletedItems,
                &This::_SetDeletedProxy)
            .def_prop_rw("orderedItems",
                &Type::GetOrderedItems,
                &This::_SetOrderedProxy)
            .def("GetAddedOrExplicitItems", [](const Type& x) {
                return TfNbSequenceToTuple(x.GetAppliedItems());
            }) // deprecated
            .def("GetAppliedItems", [](const Type& x) {
                return TfNbSequenceToTuple(x.GetAppliedItems());
            })
            .def_prop_ro("isExplicit", &Type::IsExplicit)
            .def_prop_ro("isOrderedOnly", &Type::IsOrderedOnly)
            .def("ApplyEditsToList", &This::_ApplyEditsToList)
            .def("ApplyEditsToList", &This::_ApplyEditsToList2)

            .def("CopyItems", &Type::CopyItems)
            .def("ClearEdits", &Type::ClearEdits)
            .def("ClearEditsAndMakeExplicit", &Type::ClearEditsAndMakeExplicit)
            .def("ContainsItemEdit", &Type::ContainsItemEdit,
                 nb::arg("item"), nb::arg("onlyAddOrExplicit") = false)
            .def("RemoveItemEdits", &Type::RemoveItemEdits)
            .def("ReplaceItemEdits", &Type::ReplaceItemEdits)
            .def("ModifyItemEdits", &This::_ModifyEdits)

            // New API (see bug 8710)
            .def("Add", &Type::Add)
            .def("Prepend", &Type::Prepend)
            .def("Append", &Type::Append)
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
        return x._listEditor ? TfStringify(*x._listEditor) : std::string();
    }

    static void _SetExplicitProxy(Type& x, const value_vector_type& v)
    {
        x.GetExplicitItems() = v;
    }

    static void _SetAddedProxy(Type& x, const value_vector_type& v)
    {
        x.GetAddedItems() = v;
    }

    static void _SetPrependedProxy(Type& x, const value_vector_type& v)
    {
        x.GetPrependedItems() = v;
    }

    static void _SetAppendedProxy(Type& x, const value_vector_type& v)
    {
        x.GetAppendedItems() = v;
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
                                                const nanobind::object& cb)
    {
        value_vector_type tmp = v;
        x.ApplyEditsToList(&tmp,
            Sdf_PyListEditorUtils::ApplyHelper<Type, value_type>(x, cb));
        return tmp;
    }

    static void _ModifyEdits(Type& x, const nanobind::object& cb)
    {
        x.ModifyItemEdits(Sdf_PyListEditorUtils::ModifyHelper<value_type>(cb));
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PY_LIST_EDITOR_PROXY_H
