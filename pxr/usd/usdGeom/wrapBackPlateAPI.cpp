//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/backPlateAPI.h"
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
_CreateScaleTweakAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateScaleTweakAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateRotateXYZTweakAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateRotateXYZTweakAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateTranslateTweakAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTranslateTweakAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateImageAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateImageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateAlphaImageAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateAlphaImageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateDepthImageAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDepthImageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateDepthMinOffsetAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDepthMinOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDepthNormalizingFactorAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDepthNormalizingFactorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDepthCameraSpaceOffsetAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDepthCameraSpaceOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateLumaSlopeAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLumaSlopeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateLumaOffsetAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLumaOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateLumaPowerAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLumaPowerAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreatePlateVisibilityAttr(UsdGeomBackPlateAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePlateVisibilityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static bool _WrapIsBackPlateAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdGeomBackPlateAPI::IsBackPlateAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdGeomBackPlateAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "UsdGeom.BackPlateAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdGeomBackPlateAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdGeomBackPlateAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdGeomBackPlateAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdGeomBackPlateAPI::CanApply(prim, name, &whyNot);
    return UsdGeomBackPlateAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdGeomBackPlateAPI(nb::module_ &m)
{
    typedef UsdGeomBackPlateAPI This;

    UsdGeomBackPlateAPI_CanApplyResult::WrapNb<UsdGeomBackPlateAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "BackPlateAPI");

    cls
        .def(nb::init<UsdPrim, TfToken>(), nb::arg("prim")=UsdPrim(), nb::arg("name")=TfToken())
        .def(nb::init<UsdSchemaBase const&, TfToken>(), nb::arg("schemaObj"), nb::arg("name"))
        .def(TfTypePythonClass())

        .def_static("Get",
            (UsdGeomBackPlateAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            nb::arg("stage"), nb::arg("path"))
        .def_static("Get",
            (UsdGeomBackPlateAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            nb::arg("prim"), nb::arg("name"))

        .def_static("GetAll",
            (std::vector<UsdGeomBackPlateAPI>(*)(const UsdPrim &prim))
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

        
        .def("GetScaleTweakAttr",
             &This::GetScaleTweakAttr)
        .def("CreateScaleTweakAttr",
             &_CreateScaleTweakAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetRotateXYZTweakAttr",
             &This::GetRotateXYZTweakAttr)
        .def("CreateRotateXYZTweakAttr",
             &_CreateRotateXYZTweakAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetTranslateTweakAttr",
             &This::GetTranslateTweakAttr)
        .def("CreateTranslateTweakAttr",
             &_CreateTranslateTweakAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetImageAttr",
             &This::GetImageAttr)
        .def("CreateImageAttr",
             &_CreateImageAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetAlphaImageAttr",
             &This::GetAlphaImageAttr)
        .def("CreateAlphaImageAttr",
             &_CreateAlphaImageAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetDepthImageAttr",
             &This::GetDepthImageAttr)
        .def("CreateDepthImageAttr",
             &_CreateDepthImageAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetDepthMinOffsetAttr",
             &This::GetDepthMinOffsetAttr)
        .def("CreateDepthMinOffsetAttr",
             &_CreateDepthMinOffsetAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetDepthNormalizingFactorAttr",
             &This::GetDepthNormalizingFactorAttr)
        .def("CreateDepthNormalizingFactorAttr",
             &_CreateDepthNormalizingFactorAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetDepthCameraSpaceOffsetAttr",
             &This::GetDepthCameraSpaceOffsetAttr)
        .def("CreateDepthCameraSpaceOffsetAttr",
             &_CreateDepthCameraSpaceOffsetAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetLumaSlopeAttr",
             &This::GetLumaSlopeAttr)
        .def("CreateLumaSlopeAttr",
             &_CreateLumaSlopeAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetLumaOffsetAttr",
             &This::GetLumaOffsetAttr)
        .def("CreateLumaOffsetAttr",
             &_CreateLumaOffsetAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetLumaPowerAttr",
             &This::GetLumaPowerAttr)
        .def("CreateLumaPowerAttr",
             &_CreateLumaPowerAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetPlateVisibilityAttr",
             &This::GetPlateVisibilityAttr)
        .def("CreatePlateVisibilityAttr",
             &_CreatePlateVisibilityAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)

        .def_static("IsBackPlateAPIPath", _WrapIsBackPlateAPIPath)
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
     using This = UsdGeomBackPlateAPI;

     _class 
          .def("ComputeEffectiveDimension",
               &This::ComputeEffectiveDimension,
               nb::arg("computeWidth"),
               nb::arg("time"))
          .def("SetCameraSpacePosition",
               &This::SetCameraSpacePosition,
               nb::arg("pos"),
               nb::arg("time"))
          .def("GetCameraSpacePosition",
               &This::GetCameraSpacePosition,
               nb::arg("time"))
          .def("SetWorldSpacePosition",
               &This::SetWorldSpacePosition,
               nb::arg("pos"),
               nb::arg("time"))
          .def("GetWorldSpacePosition",
               &This::GetWorldSpacePosition,
               nb::arg("time"))
          .def("SetAspectRatio",
               &This::SetAspectRatio,
               nb::arg("width"),
               nb::arg("height"),
               nb::arg("time"))
          ;
}

}
