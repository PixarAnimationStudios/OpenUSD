//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/nonboundableLightBase.h"
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

static std::string
_Repr(const UsdLuxNonboundableLightBase &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLux.NonboundableLightBase(%s)",
        primRepr.c_str());
}

        
static UsdAttribute
_CreateShaderIdAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShaderIdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateMaterialSyncModeAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMaterialSyncModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIntensityAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateIntensityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateExposureAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExposureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDiffuseAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDiffuseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateSpecularAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateSpecularAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateNormalizeAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateNormalizeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateColorAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateEnableColorTemperatureAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateEnableColorTemperatureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateColorTemperatureAttr(UsdLuxNonboundableLightBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateColorTemperatureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
} // anonymous namespace

void wrapUsdLuxNonboundableLightBase(nb::module_ &m)
{
    typedef UsdLuxNonboundableLightBase This;

    nb::class_<This, UsdGeomXformable> cls(m, "NonboundableLightBase");

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

        
        .def("GetShaderIdAttr",
             &This::GetShaderIdAttr)
        .def("CreateShaderIdAttr",
             &_CreateShaderIdAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMaterialSyncModeAttr",
             &This::GetMaterialSyncModeAttr)
        .def("CreateMaterialSyncModeAttr",
             &_CreateMaterialSyncModeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetIntensityAttr",
             &This::GetIntensityAttr)
        .def("CreateIntensityAttr",
             &_CreateIntensityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetExposureAttr",
             &This::GetExposureAttr)
        .def("CreateExposureAttr",
             &_CreateExposureAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetDiffuseAttr",
             &This::GetDiffuseAttr)
        .def("CreateDiffuseAttr",
             &_CreateDiffuseAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetSpecularAttr",
             &This::GetSpecularAttr)
        .def("CreateSpecularAttr",
             &_CreateSpecularAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetNormalizeAttr",
             &This::GetNormalizeAttr)
        .def("CreateNormalizeAttr",
             &_CreateNormalizeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetColorAttr",
             &This::GetColorAttr)
        .def("CreateColorAttr",
             &_CreateColorAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetEnableColorTemperatureAttr",
             &This::GetEnableColorTemperatureAttr)
        .def("CreateEnableColorTemperatureAttr",
             &_CreateEnableColorTemperatureAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetColorTemperatureAttr",
             &This::GetColorTemperatureAttr)
        .def("CreateColorTemperatureAttr",
             &_CreateColorTemperatureAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("GetFiltersRel",
             &This::GetFiltersRel)
        .def("CreateFiltersRel",
             &This::CreateFiltersRel)

        .def("LightAPI", &This::LightAPI)
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
