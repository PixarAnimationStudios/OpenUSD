//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/shadowAPI.h"
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
_CreateShadowEnableAttr(UsdLuxShadowAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShadowEnableAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateShadowColorAttr(UsdLuxShadowAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShadowColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateShadowDistanceAttr(UsdLuxShadowAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShadowDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShadowFalloffAttr(UsdLuxShadowAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShadowFalloffAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShadowFalloffGammaAttr(UsdLuxShadowAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShadowFalloffGammaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static std::string
_Repr(const UsdLuxShadowAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdLux.ShadowAPI(%s)",
        primRepr.c_str());
}

struct UsdLuxShadowAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdLuxShadowAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdLuxShadowAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdLuxShadowAPI::CanApply(prim, &whyNot);
    return UsdLuxShadowAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdLuxShadowAPI(nb::module_ &m)
{
    typedef UsdLuxShadowAPI This;

    UsdLuxShadowAPI_CanApplyResult::WrapNb<UsdLuxShadowAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "ShadowAPI");

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

        
        .def("GetShadowEnableAttr",
             &This::GetShadowEnableAttr)
        .def("CreateShadowEnableAttr",
             &_CreateShadowEnableAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShadowColorAttr",
             &This::GetShadowColorAttr)
        .def("CreateShadowColorAttr",
             &_CreateShadowColorAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShadowDistanceAttr",
             &This::GetShadowDistanceAttr)
        .def("CreateShadowDistanceAttr",
             &_CreateShadowDistanceAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShadowFalloffAttr",
             &This::GetShadowFalloffAttr)
        .def("CreateShadowFalloffAttr",
             &_CreateShadowFalloffAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShadowFalloffGammaAttr",
             &This::GetShadowFalloffGammaAttr)
        .def("CreateShadowFalloffGammaAttr",
             &_CreateShadowFalloffGammaAttr,
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
        .def("ConnectableAPI", &UsdLuxShadowAPI::ConnectableAPI)

        .def("CreateOutput", &UsdLuxShadowAPI::CreateOutput,
             nb::arg("name"), nb::arg("type"))
        .def("GetOutput", &UsdLuxShadowAPI::GetOutput, nb::arg("name"))
        .def("GetOutputs", &UsdLuxShadowAPI::GetOutputs,
             (nb::arg("onlyAuthored")=true),
             nb::rv_policy::copy)

        .def("CreateInput", &UsdLuxShadowAPI::CreateInput,
             nb::arg("name"), nb::arg("type"))
        .def("GetInput", &UsdLuxShadowAPI::GetInput, nb::arg("name"))
        .def("GetInputs", &UsdLuxShadowAPI::GetInputs,
             (nb::arg("onlyAuthored")=true),
             nb::rv_policy::copy)
        ;
}

}
