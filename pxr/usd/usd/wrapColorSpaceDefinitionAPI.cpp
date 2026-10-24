//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/colorSpaceDefinitionAPI.h"
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
_CreateNameAttr(UsdColorSpaceDefinitionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateNameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateRedChromaAttr(UsdColorSpaceDefinitionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateRedChromaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateGreenChromaAttr(UsdColorSpaceDefinitionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateGreenChromaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateBlueChromaAttr(UsdColorSpaceDefinitionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateBlueChromaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateWhitePointAttr(UsdColorSpaceDefinitionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateWhitePointAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateGammaAttr(UsdColorSpaceDefinitionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateGammaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateLinearBiasAttr(UsdColorSpaceDefinitionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateLinearBiasAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static bool _WrapIsColorSpaceDefinitionAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdColorSpaceDefinitionAPI::IsColorSpaceDefinitionAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdColorSpaceDefinitionAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "Usd.ColorSpaceDefinitionAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdColorSpaceDefinitionAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdColorSpaceDefinitionAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdColorSpaceDefinitionAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdColorSpaceDefinitionAPI::CanApply(prim, name, &whyNot);
    return UsdColorSpaceDefinitionAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdColorSpaceDefinitionAPI(nb::module_ &m)
{
    typedef UsdColorSpaceDefinitionAPI This;

    UsdColorSpaceDefinitionAPI_CanApplyResult::WrapNb<UsdColorSpaceDefinitionAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "ColorSpaceDefinitionAPI");

    cls
        .def(nb::init<UsdPrim, TfToken>(), nb::arg("prim")=UsdPrim(), nb::arg("name")=TfToken())
        .def(nb::init<UsdSchemaBase const&, TfToken>(), nb::arg("schemaObj"), nb::arg("name"))
        .def(TfTypePythonClass())

        .def_static("Get",
            (UsdColorSpaceDefinitionAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            nb::arg("stage"), nb::arg("path"))
        .def_static("Get",
            (UsdColorSpaceDefinitionAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            nb::arg("prim"), nb::arg("name"))

        .def_static("GetAll",
            (std::vector<UsdColorSpaceDefinitionAPI>(*)(const UsdPrim &prim))
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

        
        .def("GetNameAttr",
             &This::GetNameAttr)
        .def("CreateNameAttr",
             &_CreateNameAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetRedChromaAttr",
             &This::GetRedChromaAttr)
        .def("CreateRedChromaAttr",
             &_CreateRedChromaAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetGreenChromaAttr",
             &This::GetGreenChromaAttr)
        .def("CreateGreenChromaAttr",
             &_CreateGreenChromaAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetBlueChromaAttr",
             &This::GetBlueChromaAttr)
        .def("CreateBlueChromaAttr",
             &_CreateBlueChromaAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetWhitePointAttr",
             &This::GetWhitePointAttr)
        .def("CreateWhitePointAttr",
             &_CreateWhitePointAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetGammaAttr",
             &This::GetGammaAttr)
        .def("CreateGammaAttr",
             &_CreateGammaAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetLinearBiasAttr",
             &This::GetLinearBiasAttr)
        .def("CreateLinearBiasAttr",
             &_CreateLinearBiasAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def_static("IsColorSpaceDefinitionAPIPath", _WrapIsColorSpaceDefinitionAPIPath)
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
    _class
        .def("CreateColorSpaceAttrsWithChroma",
             &UsdColorSpaceDefinitionAPI::CreateColorSpaceAttrsWithChroma,
             nb::arg("redChroma"), nb::arg("greenChroma"), nb::arg("blueChroma"),
              nb::arg("whitePoint"), nb::arg("gamma"), nb::arg("linearBias"))
        .def("CreateColorSpaceAttrsWithMatrix",
             &UsdColorSpaceDefinitionAPI::CreateColorSpaceAttrsWithMatrix,
             nb::arg("rgbToXYZ"), nb::arg("gamma"), nb::arg("linearBias"))
        .def("ComputeColorSpaceFromDefinitionAttributes",
             &UsdColorSpaceDefinitionAPI::ComputeColorSpaceFromDefinitionAttributes)
    ;
}

}
