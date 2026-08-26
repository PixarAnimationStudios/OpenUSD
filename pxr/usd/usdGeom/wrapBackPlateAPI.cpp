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
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateScaleTweakAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleTweakAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateRotateXYZTweakAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRotateXYZTweakAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateTranslateTweakAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTranslateTweakAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateImageAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateImageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateAlphaImageAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAlphaImageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateDepthImageAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDepthImageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateDepthMinOffsetAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDepthMinOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDepthNormalizingFactorAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDepthNormalizingFactorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDepthCameraSpaceOffsetAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDepthCameraSpaceOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateLumaSlopeAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLumaSlopeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateLumaOffsetAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLumaOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateLumaPowerAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLumaPowerAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreatePlateVisibilityAttr(UsdGeomBackPlateAPI &self,
                                      object defaultVal, bool writeSparsely) {
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

void wrapUsdGeomBackPlateAPI()
{
    typedef UsdGeomBackPlateAPI This;

    UsdGeomBackPlateAPI_CanApplyResult::Wrap<UsdGeomBackPlateAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("BackPlateAPI");

    cls
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name"))))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name"))))
        .def(TfTypePythonClass())

        .def("Get",
            (UsdGeomBackPlateAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            (UsdGeomBackPlateAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
        .staticmethod("Get")

        .def("GetAll",
            (std::vector<UsdGeomBackPlateAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            arg("prim"),
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAll")

        .def("CanApply", &_WrapCanApply, (arg("prim"), arg("name")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim"), arg("name")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .def("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             arg("includeInherited"),
             arg("instanceName"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetScaleTweakAttr",
             &This::GetScaleTweakAttr)
        .def("CreateScaleTweakAttr",
             &_CreateScaleTweakAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRotateXYZTweakAttr",
             &This::GetRotateXYZTweakAttr)
        .def("CreateRotateXYZTweakAttr",
             &_CreateRotateXYZTweakAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTranslateTweakAttr",
             &This::GetTranslateTweakAttr)
        .def("CreateTranslateTweakAttr",
             &_CreateTranslateTweakAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetImageAttr",
             &This::GetImageAttr)
        .def("CreateImageAttr",
             &_CreateImageAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAlphaImageAttr",
             &This::GetAlphaImageAttr)
        .def("CreateAlphaImageAttr",
             &_CreateAlphaImageAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDepthImageAttr",
             &This::GetDepthImageAttr)
        .def("CreateDepthImageAttr",
             &_CreateDepthImageAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDepthMinOffsetAttr",
             &This::GetDepthMinOffsetAttr)
        .def("CreateDepthMinOffsetAttr",
             &_CreateDepthMinOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDepthNormalizingFactorAttr",
             &This::GetDepthNormalizingFactorAttr)
        .def("CreateDepthNormalizingFactorAttr",
             &_CreateDepthNormalizingFactorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDepthCameraSpaceOffsetAttr",
             &This::GetDepthCameraSpaceOffsetAttr)
        .def("CreateDepthCameraSpaceOffsetAttr",
             &_CreateDepthCameraSpaceOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLumaSlopeAttr",
             &This::GetLumaSlopeAttr)
        .def("CreateLumaSlopeAttr",
             &_CreateLumaSlopeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLumaOffsetAttr",
             &This::GetLumaOffsetAttr)
        .def("CreateLumaOffsetAttr",
             &_CreateLumaOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLumaPowerAttr",
             &This::GetLumaPowerAttr)
        .def("CreateLumaPowerAttr",
             &_CreateLumaPowerAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPlateVisibilityAttr",
             &This::GetPlateVisibilityAttr)
        .def("CreatePlateVisibilityAttr",
             &_CreatePlateVisibilityAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("IsBackPlateAPIPath", _WrapIsBackPlateAPIPath)
            .staticmethod("IsBackPlateAPIPath")
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
             (arg("computeWidth"),
              arg("time")))
          .def("SetCameraSpacePosition",
               &This::SetCameraSpacePosition,
             (arg("pos"),
              arg("time")))
          .def("GetCameraSpacePosition",
               &This::GetCameraSpacePosition,
             (arg("time")))
          .def("SetWorldSpacePosition",
               &This::SetWorldSpacePosition,
             (arg("pos"),
              arg("time")))
          .def("GetWorldSpacePosition",
               &This::GetWorldSpacePosition,
             (arg("time")))
          .def("SetAspectRatio",
               &This::SetAspectRatio,
             (arg("width"),
              arg("height"),
              arg("time")))
          ;
}

}
