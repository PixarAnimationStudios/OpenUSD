//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/joint.h"
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
_CreateLocalPos0Attr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLocalPos0Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3f), writeSparsely);
}
        
static UsdAttribute
_CreateLocalRot0Attr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLocalRot0Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quatf), writeSparsely);
}
        
static UsdAttribute
_CreateLocalPos1Attr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLocalPos1Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3f), writeSparsely);
}
        
static UsdAttribute
_CreateLocalRot1Attr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLocalRot1Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quatf), writeSparsely);
}
        
static UsdAttribute
_CreateJointEnabledAttr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateJointEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCollisionEnabledAttr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCollisionEnabledAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateExcludeFromArticulationAttr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExcludeFromArticulationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateBreakForceAttr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateBreakForceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateBreakTorqueAttr(UsdPhysicsJoint &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateBreakTorqueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static std::string
_Repr(const UsdPhysicsJoint &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdPhysics.Joint(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdPhysicsJoint(nb::module_ &m)
{
    typedef UsdPhysicsJoint This;

    nb::class_<This, UsdGeomImageable> cls(m, "Joint");

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

        
        .def("GetLocalPos0Attr",
             &This::GetLocalPos0Attr)
        .def("CreateLocalPos0Attr",
             &_CreateLocalPos0Attr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetLocalRot0Attr",
             &This::GetLocalRot0Attr)
        .def("CreateLocalRot0Attr",
             &_CreateLocalRot0Attr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetLocalPos1Attr",
             &This::GetLocalPos1Attr)
        .def("CreateLocalPos1Attr",
             &_CreateLocalPos1Attr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetLocalRot1Attr",
             &This::GetLocalRot1Attr)
        .def("CreateLocalRot1Attr",
             &_CreateLocalRot1Attr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetJointEnabledAttr",
             &This::GetJointEnabledAttr)
        .def("CreateJointEnabledAttr",
             &_CreateJointEnabledAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCollisionEnabledAttr",
             &This::GetCollisionEnabledAttr)
        .def("CreateCollisionEnabledAttr",
             &_CreateCollisionEnabledAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetExcludeFromArticulationAttr",
             &This::GetExcludeFromArticulationAttr)
        .def("CreateExcludeFromArticulationAttr",
             &_CreateExcludeFromArticulationAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetBreakForceAttr",
             &This::GetBreakForceAttr)
        .def("CreateBreakForceAttr",
             &_CreateBreakForceAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetBreakTorqueAttr",
             &This::GetBreakTorqueAttr)
        .def("CreateBreakTorqueAttr",
             &_CreateBreakTorqueAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetBody0Rel",
             &This::GetBody0Rel)
        .def("CreateBody0Rel",
             &This::CreateBody0Rel)
        
        .def("GetBody1Rel",
             &This::GetBody1Rel)
        .def("CreateBody1Rel",
             &This::CreateBody1Rel)
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

WRAP_CUSTOM {
}

}
