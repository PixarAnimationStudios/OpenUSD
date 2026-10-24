//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/collisionGroup.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateMergeGroupNameAttr(UsdPhysicsCollisionGroup &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMergeGroupNameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateInvertFilteredGroupsAttr(UsdPhysicsCollisionGroup &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateInvertFilteredGroupsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}

static std::string
_Repr(const UsdPhysicsCollisionGroup &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdPhysics.CollisionGroup(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdPhysicsCollisionGroup(nb::module_ &m)
{
    typedef UsdPhysicsCollisionGroup This;

    nb::class_<This, UsdTyped> cls(m, "CollisionGroup");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetMergeGroupNameAttr",
             &This::GetMergeGroupNameAttr)
        .def("CreateMergeGroupNameAttr",
             &_CreateMergeGroupNameAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetInvertFilteredGroupsAttr",
             &This::GetInvertFilteredGroupsAttr)
        .def("CreateInvertFilteredGroupsAttr",
             &_CreateInvertFilteredGroupsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetFilteredGroupsRel",
             &This::GetFilteredGroupsRel)
        .def("CreateFilteredGroupsRel",
             &This::CreateFilteredGroupsRel)
        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

static SdfPathVector
_WrapGetGroups(const UsdPhysicsCollisionGroup::CollisionGroupTable &table)
{
    return table.GetCollisionGroups();
}

static bool
_WrapIsCollisionEnabled(const UsdPhysicsCollisionGroup::CollisionGroupTable &table,
                        TfPyObjWrapper aObj, TfPyObjWrapper bObj)
{
    nb::object a = aObj.Get();
    nb::object b = bObj.Get();
    int intA, intB;
    if (nb::try_cast<int>(a, intA, false) && nb::try_cast<int>(b, intB, false))
    {
        return table.IsCollisionEnabled(intA, intB);
    }

    SdfPath pathA, pathB;
    if (nb::try_cast<SdfPath>(a, pathA, false) &&
        nb::try_cast<SdfPath>(b, pathB, false))
    {
        return table.IsCollisionEnabled(pathA, pathB);
    }

    UsdPhysicsCollisionGroup colGroupA, colGroupB;
    if (nb::try_cast<UsdPhysicsCollisionGroup>(a, colGroupA, false) &&
        nb::try_cast<UsdPhysicsCollisionGroup>(b, colGroupB, false))
    {
        return table.IsCollisionEnabled(
            colGroupA.GetPrim().GetPrimPath(),
            colGroupB.GetPrim().GetPrimPath());
    }

    UsdPrim primA, primB;
    if (nb::try_cast<UsdPrim>(a, primA, false) &&
        nb::try_cast<UsdPrim>(b, primB, false))
    {
        return table.IsCollisionEnabled(primA.GetPrimPath(), primB.GetPrimPath());
    }

    return true;
}

WRAP_CUSTOM {
    typedef UsdPhysicsCollisionGroup This;

    _class
        .def("GetCollidersCollectionAPI",
             &This::GetCollidersCollectionAPI)
        .def_static("ComputeCollisionGroupTable",
             &This::ComputeCollisionGroupTable,
             nb::arg("stage"),
             nb::rv_policy::copy)
        ;

    nb::class_<UsdPhysicsCollisionGroup::CollisionGroupTable> tableCls(
        TfPyGetCurrentNanobindModule(),
        "CollisionGroupTable");
    tableCls
        .def(nb::init<>())
        .def("GetGroups", &_WrapGetGroups,
             nb::rv_policy::copy)
        .def("IsCollisionEnabled", &_WrapIsCollisionEnabled)
        ;
    _class.attr("CollisionGroupTable") = tableCls;
}

}
