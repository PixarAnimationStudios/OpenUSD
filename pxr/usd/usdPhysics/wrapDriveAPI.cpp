//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/driveAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_CreateTypeAttr(UsdPhysicsDriveAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateMaxForceAttr(UsdPhysicsDriveAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMaxForceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateTargetPositionAttr(UsdPhysicsDriveAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTargetPositionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateTargetVelocityAttr(UsdPhysicsDriveAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTargetVelocityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDampingAttr(UsdPhysicsDriveAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDampingAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateStiffnessAttr(UsdPhysicsDriveAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateStiffnessAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static bool _WrapIsPhysicsDriveAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdPhysicsDriveAPI::IsPhysicsDriveAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdPhysicsDriveAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "UsdPhysics.DriveAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdPhysicsDriveAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdPhysicsDriveAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdPhysicsDriveAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdPhysicsDriveAPI::CanApply(prim, name, &whyNot);
    return UsdPhysicsDriveAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdPhysicsDriveAPI(nb::module_ &m)
{
    typedef UsdPhysicsDriveAPI This;

    UsdPhysicsDriveAPI_CanApplyResult::WrapNb<UsdPhysicsDriveAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "DriveAPI");

    cls
        .def(nb::init<UsdPrim, TfToken>(), nb::arg("prim")=UsdPrim(), nb::arg("name")=TfToken())
        .def(nb::init<UsdSchemaBase const&, TfToken>(), nb::arg("schemaObj"), nb::arg("name"))
        .def(TfTypePythonClass())

        .def_static("Get",
            (UsdPhysicsDriveAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            nb::arg("stage"), nb::arg("path"))
        .def_static("Get",
            (UsdPhysicsDriveAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            nb::arg("prim"), nb::arg("name"))

        .def_static("GetAll",
            (std::vector<UsdPhysicsDriveAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            nb::arg("prim"),
             nb::rv_policy::copy)

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"), nb::arg("name"))

        .def_static("Apply", &This::Apply, nb::arg("prim"), nb::arg("name"))

        .def_static("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)
        .def_static("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             nb::arg("includeInherited"),
             nb::arg("instanceName"),
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetTypeAttr",
             &This::GetTypeAttr)
        .def("CreateTypeAttr",
             &_CreateTypeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMaxForceAttr",
             &This::GetMaxForceAttr)
        .def("CreateMaxForceAttr",
             &_CreateMaxForceAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTargetPositionAttr",
             &This::GetTargetPositionAttr)
        .def("CreateTargetPositionAttr",
             &_CreateTargetPositionAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTargetVelocityAttr",
             &This::GetTargetVelocityAttr)
        .def("CreateTargetVelocityAttr",
             &_CreateTargetVelocityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetDampingAttr",
             &This::GetDampingAttr)
        .def("CreateDampingAttr",
             &_CreateDampingAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetStiffnessAttr",
             &This::GetStiffnessAttr)
        .def("CreateStiffnessAttr",
             &_CreateStiffnessAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def_static("IsPhysicsDriveAPIPath", _WrapIsPhysicsDriveAPIPath)
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
