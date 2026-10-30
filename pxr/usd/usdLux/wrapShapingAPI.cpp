//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/shapingAPI.h"
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
_CreateShapingFocusAttr(UsdLuxShapingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShapingFocusAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShapingFocusTintAttr(UsdLuxShapingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShapingFocusTintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateShapingConeAngleAttr(UsdLuxShapingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShapingConeAngleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShapingConeSoftnessAttr(UsdLuxShapingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShapingConeSoftnessAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShapingIesFileAttr(UsdLuxShapingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShapingIesFileAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateShapingIesAngleScaleAttr(UsdLuxShapingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShapingIesAngleScaleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShapingIesNormalizeAttr(UsdLuxShapingAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShapingIesNormalizeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}

static std::string
_Repr(const UsdLuxShapingAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLux.ShapingAPI(%s)",
        primRepr.c_str());
}

struct UsdLuxShapingAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdLuxShapingAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdLuxShapingAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdLuxShapingAPI::CanApply(prim, &whyNot);
    return UsdLuxShapingAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdLuxShapingAPI(nb::module_ &m)
{
    typedef UsdLuxShapingAPI This;

    UsdLuxShapingAPI_CanApplyResult::WrapNb<UsdLuxShapingAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "ShapingAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"))

        .def_static("Apply", &This::Apply, nb::arg("prim"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetShapingFocusAttr",
             &This::GetShapingFocusAttr)
        .def("CreateShapingFocusAttr",
             &_CreateShapingFocusAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShapingFocusTintAttr",
             &This::GetShapingFocusTintAttr)
        .def("CreateShapingFocusTintAttr",
             &_CreateShapingFocusTintAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShapingConeAngleAttr",
             &This::GetShapingConeAngleAttr)
        .def("CreateShapingConeAngleAttr",
             &_CreateShapingConeAngleAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShapingConeSoftnessAttr",
             &This::GetShapingConeSoftnessAttr)
        .def("CreateShapingConeSoftnessAttr",
             &_CreateShapingConeSoftnessAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShapingIesFileAttr",
             &This::GetShapingIesFileAttr)
        .def("CreateShapingIesFileAttr",
             &_CreateShapingIesFileAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShapingIesAngleScaleAttr",
             &This::GetShapingIesAngleScaleAttr)
        .def("CreateShapingIesAngleScaleAttr",
             &_CreateShapingIesAngleScaleAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShapingIesNormalizeAttr",
             &This::GetShapingIesNormalizeAttr)
        .def("CreateShapingIesNormalizeAttr",
             &_CreateShapingIesNormalizeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

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

#include "pxr/usd/usdShade/connectableAPI.h"

namespace {

WRAP_CUSTOM {
    _class
        .def(nb::init<UsdShadeConnectableAPI>(), nb::arg("connectable"))
        .def("ConnectableAPI", &UsdLuxShapingAPI::ConnectableAPI)

        .def("CreateOutput", &UsdLuxShapingAPI::CreateOutput,
             nb::arg("name"), nb::arg("type"))
        .def("GetOutput", &UsdLuxShapingAPI::GetOutput, nb::arg("name"))
        .def("GetOutputs", &UsdLuxShapingAPI::GetOutputs,
             (nb::arg("onlyAuthored")=true),
             nb::rv_policy::copy)

        .def("CreateInput", &UsdLuxShapingAPI::CreateInput,
             nb::arg("name"), nb::arg("type"))
        .def("GetInput", &UsdLuxShapingAPI::GetInput, nb::arg("name"))
        .def("GetInputs", &UsdLuxShapingAPI::GetInputs,
             (nb::arg("onlyAuthored")=true),
             nb::rv_policy::copy)
        ;
}

}
