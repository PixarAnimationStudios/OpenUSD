//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRender/settingsBase.h"
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
_CreateResolutionAttr(UsdRenderSettingsBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateResolutionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int2), writeSparsely);
}
        
static UsdAttribute
_CreatePixelAspectRatioAttr(UsdRenderSettingsBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePixelAspectRatioAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAspectRatioConformPolicyAttr(UsdRenderSettingsBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateAspectRatioConformPolicyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateDataWindowNDCAttr(UsdRenderSettingsBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDataWindowNDCAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float4), writeSparsely);
}
        
static UsdAttribute
_CreateInstantaneousShutterAttr(UsdRenderSettingsBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateInstantaneousShutterAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateDisableMotionBlurAttr(UsdRenderSettingsBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDisableMotionBlurAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateDisableDepthOfFieldAttr(UsdRenderSettingsBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDisableDepthOfFieldAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}

static std::string
_Repr(const UsdRenderSettingsBase &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdRender.SettingsBase(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdRenderSettingsBase(nb::module_ &m)
{
    typedef UsdRenderSettingsBase This;

    nb::class_<This, UsdTyped> cls(m, "SettingsBase");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetResolutionAttr",
             &This::GetResolutionAttr)
        .def("CreateResolutionAttr",
             &_CreateResolutionAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetPixelAspectRatioAttr",
             &This::GetPixelAspectRatioAttr)
        .def("CreatePixelAspectRatioAttr",
             &_CreatePixelAspectRatioAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetAspectRatioConformPolicyAttr",
             &This::GetAspectRatioConformPolicyAttr)
        .def("CreateAspectRatioConformPolicyAttr",
             &_CreateAspectRatioConformPolicyAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetDataWindowNDCAttr",
             &This::GetDataWindowNDCAttr)
        .def("CreateDataWindowNDCAttr",
             &_CreateDataWindowNDCAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetInstantaneousShutterAttr",
             &This::GetInstantaneousShutterAttr)
        .def("CreateInstantaneousShutterAttr",
             &_CreateInstantaneousShutterAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetDisableMotionBlurAttr",
             &This::GetDisableMotionBlurAttr)
        .def("CreateDisableMotionBlurAttr",
             &_CreateDisableMotionBlurAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetDisableDepthOfFieldAttr",
             &This::GetDisableDepthOfFieldAttr)
        .def("CreateDisableDepthOfFieldAttr",
             &_CreateDisableDepthOfFieldAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetCameraRel",
             &This::GetCameraRel)
        .def("CreateCameraRel",
             &This::CreateCameraRel)
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
