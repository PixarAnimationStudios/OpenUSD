//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PY_LIST_OP_H
#define PXR_USD_SDF_PY_LIST_OP_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

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
        TfPyWrapOnce<T>([name]() {
            nanobind::handle scope = TfPyGetCurrentNanobindModule();
            if (!scope.is_valid()) {
                scope = nanobind::borrow(PyImport_AddModule("pxr.Sdf"));
            }
            nanobind::module_ m = nanobind::borrow<nanobind::module_>(scope);
            SdfPyWrapListOp::_WrapNb(m, name);
        });
    }

    SdfPyWrapListOp(nanobind::module_& m, const std::string& name)
    {
        _WrapNb(m, name);
    }
 
private:
    static ItemVector _ApplyOperations1(const T& listOp, ItemVector input) {
        ItemVector result = input;
        listOp.ApplyOperations(&result);
        return result;
    }
    static std::optional<T>
    _ApplyOperations2Nb(const T& outer, const T& inner) {
        return outer.ApplyOperations(inner);
    }

    static size_t _Hash(const T &self){
        return TfHash()(self);
    }

    static void _WrapNb(nanobind::module_& m, const std::string& name)
    {
        namespace nb = nanobind;
        using ItemVector = typename T::ItemVector;

        nb::class_<T>(m, name.c_str())
            .def(nb::init<>())
            .def("__str__", &This::_GetStr)
            .def("__hash__", &This::_Hash)

            .def_static("Create", &T::Create,
                 nb::arg("prependedItems") = ItemVector(),
                 nb::arg("appendedItems") = ItemVector(),
                 nb::arg("deletedItems") = ItemVector())

            .def_static("CreateExplicit", &T::CreateExplicit,
                 nb::arg("explicitItems") = ItemVector())

            .def(nb::self == nb::self)
            .def(nb::self != nb::self)

            .def("HasItem", &T::HasItem)

            .def("Clear", &T::Clear)
            .def("ClearAndMakeExplicit", &T::ClearAndMakeExplicit)
            .def("ApplyOperations", &This::_ApplyOperations1)
            .def("ApplyOperations", &This::_ApplyOperations2Nb)

            .def_prop_rw("explicitItems",
                [](const T& t) { return t.GetExplicitItems(); },
                [](T& t, const ItemVector& items) { t.SetExplicitItems(items); })
            .def_prop_rw("addedItems",
                [](const T& t) { return t.GetAddedItems(); },
                &T::SetAddedItems)
            .def_prop_rw("prependedItems",
                [](const T& t) { return t.GetPrependedItems(); },
                [](T& t, const ItemVector& items) { t.SetPrependedItems(items); })
            .def_prop_rw("appendedItems",
                [](const T& t) { return t.GetAppendedItems(); },
                [](T& t, const ItemVector& items) { t.SetAppendedItems(items); })
            .def_prop_rw("deletedItems",
                [](const T& t) { return t.GetDeletedItems(); },
                [](T& t, const ItemVector& items) { t.SetDeletedItems(items); })
            .def_prop_rw("orderedItems",
                [](const T& t) { return t.GetOrderedItems(); },
                &T::SetOrderedItems)
            .def("GetAddedOrExplicitItems",
                &T::GetAppliedItems) //deprecated
            .def("GetAppliedItems",
                &T::GetAppliedItems)

            .def_prop_ro("isExplicit", &T::IsExplicit)
            ;

    }

    static std::string _GetStr(const T& listOp)
    {
        return TfStringify(listOp);
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PY_LIST_OP_H
