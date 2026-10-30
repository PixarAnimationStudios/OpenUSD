//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "./paramsAPI.h"
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

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateMassAttr(UsdSchemaExamplesParamsAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMassAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateVelocityAttr(UsdSchemaExamplesParamsAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVelocityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateVolumeAttr(UsdSchemaExamplesParamsAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVolumeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}

static std::string
_Repr(const UsdSchemaExamplesParamsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdSchemaExamples.ParamsAPI(%s)",
        primRepr.c_str());
}

struct UsdSchemaExamplesParamsAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdSchemaExamplesParamsAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdSchemaExamplesParamsAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdSchemaExamplesParamsAPI::CanApply(prim, &whyNot);
    return UsdSchemaExamplesParamsAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdSchemaExamplesParamsAPI(nb::module_ &m)
{
    typedef UsdSchemaExamplesParamsAPI This;

    UsdSchemaExamplesParamsAPI_CanApplyResult::Wrap<UsdSchemaExamplesParamsAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "ParamsAPI");

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

        
        .def("GetMassAttr",
             &This::GetMassAttr)
        .def("CreateMassAttr",
             &_CreateMassAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVelocityAttr",
             &This::GetVelocityAttr)
        .def("CreateVelocityAttr",
             &_CreateVelocityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVolumeAttr",
             &This::GetVolumeAttr)
        .def("CreateVolumeAttr",
             &_CreateVolumeAttr,
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

namespace {

WRAP_CUSTOM {
}

} // anonymous namespace 
