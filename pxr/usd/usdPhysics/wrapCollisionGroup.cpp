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
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateMergeGroupNameAttr(UsdPhysicsCollisionGroup &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMergeGroupNameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateInvertFilteredGroupsAttr(UsdPhysicsCollisionGroup &self,
                                      object defaultVal, bool writeSparsely) {
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

void wrapUsdPhysicsCollisionGroup()
{
    typedef UsdPhysicsCollisionGroup This;

    class_<This, bases<UsdTyped> >
        cls("CollisionGroup");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetMergeGroupNameAttr",
             &This::GetMergeGroupNameAttr)
        .def("CreateMergeGroupNameAttr",
             &_CreateMergeGroupNameAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInvertFilteredGroupsAttr",
             &This::GetInvertFilteredGroupsAttr)
        .def("CreateInvertFilteredGroupsAttr",
             &_CreateInvertFilteredGroupsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
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

#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace {

static SdfPathVector
_WrapGetGroups(const UsdPhysicsCollisionGroup::CollisionGroupTable &table)
{
    return table.GetCollisionGroups();
}

static bool
_WrapIsCollisionEnabled(const UsdPhysicsCollisionGroup::CollisionGroupTable &table,
boost::python::object a, boost::python::object b)
{
    boost::python::extract<int> extractIntA(a);
    boost::python::extract<int> extractIntB(b);
    if (extractIntA.check() && extractIntB.check())
    {
        return table.IsCollisionEnabled(extractIntA(), extractIntB());
    }

    boost::python::extract<SdfPath> extractPathA(a);
    boost::python::extract<SdfPath> extractPathB(b);
    if (extractPathA.check() && extractPathB.check())
    {
        return table.IsCollisionEnabled(extractPathA(), extractPathB());
    }

    boost::python::extract<UsdPhysicsCollisionGroup> extractColGroupA(a);
    boost::python::extract<UsdPhysicsCollisionGroup> extractColGroupB(b);
    if (extractColGroupA.check() && extractColGroupB.check())
    {
        return table.IsCollisionEnabled(extractColGroupA().GetPrim().GetPrimPath(), extractColGroupB().GetPrim().GetPrimPath());
    }

    boost::python::extract<UsdPrim> extractPrimA(a);
    boost::python::extract<UsdPrim> extractPrimB(b);
    if (extractPrimA.check() && extractPrimB.check())
    {
        return table.IsCollisionEnabled(extractPrimA().GetPrimPath(), extractPrimB().GetPrimPath());
    }

    return true;
}

WRAP_CUSTOM {
    typedef UsdPhysicsCollisionGroup This;

    _class
        .def("GetCollidersCollectionAPI",
             &This::GetCollidersCollectionAPI)
        .def("ComputeCollisionGroupTable",
             &This::ComputeCollisionGroupTable,
             arg("stage"),
             return_value_policy<return_by_value>())
        .staticmethod("ComputeCollisionGroupTable")
        ;

    class_<UsdPhysicsCollisionGroup::CollisionGroupTable>("CollisionGroupTable")
        .def("GetGroups", &_WrapGetGroups,
             return_value_policy<TfPySequenceToList>())
        .def("IsCollisionEnabled", &_WrapIsCollisionEnabled)
        ;
}

}
