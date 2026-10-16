//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRi/statementsAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

static std::string
_Repr(const UsdRiStatementsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdRi.StatementsAPI(%s)",
        primRepr.c_str());
}

struct UsdRiStatementsAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdRiStatementsAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdRiStatementsAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdRiStatementsAPI::CanApply(prim, &whyNot);
    return UsdRiStatementsAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdRiStatementsAPI(nb::module_ &m)
{
    typedef UsdRiStatementsAPI This;

    UsdRiStatementsAPI_CanApplyResult::WrapNb<UsdRiStatementsAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "StatementsAPI");

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

static SdfPathVector
_GetModelCoordinateSystems(const UsdRiStatementsAPI &self)
{
    SdfPathVector result;
    self.GetModelCoordinateSystems(&result);
    return result;
}

static SdfPathVector
_GetModelScopedCoordinateSystems(const UsdRiStatementsAPI &self)
{
    SdfPathVector result;
    self.GetModelScopedCoordinateSystems(&result);
    return result;
}

WRAP_CUSTOM {
    _class
        .def("CreateRiAttribute",
             (UsdAttribute (UsdRiStatementsAPI::*)(
                 const TfToken &, const TfType &, const std::string &))
             &UsdRiStatementsAPI::CreateRiAttribute,
             nb::arg("name"), nb::arg("tfType"), nb::arg("nameSpace")="user")
        .def("CreateRiAttribute",
             (UsdAttribute (UsdRiStatementsAPI::*)(
                 const TfToken &, const std::string &, const std::string &))
             &UsdRiStatementsAPI::CreateRiAttribute,
             nb::arg("name"), nb::arg("riType"), nb::arg("nameSpace")="user")
        .def("GetRiAttribute",
             &UsdRiStatementsAPI::GetRiAttribute,
             nb::arg("name"), nb::arg("nameSpace")="user")
        .def("GetRiAttributes", &UsdRiStatementsAPI::GetRiAttributes,
             nb::arg("nameSpace")="",
             nb::rv_policy::copy)
        .def_static("GetRiAttributeName",
             &UsdRiStatementsAPI::GetRiAttributeName, nb::arg("prop"))
        .def_static("GetRiAttributeNameSpace",
             &UsdRiStatementsAPI::GetRiAttributeNameSpace, nb::arg("prop"))
        .def_static("IsRiAttribute", &UsdRiStatementsAPI::IsRiAttribute, nb::arg("prop"))
        .def_static("MakeRiAttributePropertyName",
             &UsdRiStatementsAPI::MakeRiAttributePropertyName, nb::arg("attrName"))
        .def("SetCoordinateSystem", &UsdRiStatementsAPI::SetCoordinateSystem,
             nb::arg("coordSysName"))
        .def("GetCoordinateSystem", &UsdRiStatementsAPI::GetCoordinateSystem)
        .def("HasCoordinateSystem", &UsdRiStatementsAPI::HasCoordinateSystem)

        .def("SetScopedCoordinateSystem",
             &UsdRiStatementsAPI::SetScopedCoordinateSystem,
             nb::arg("coordSysName"))
        .def("GetScopedCoordinateSystem",
             &UsdRiStatementsAPI::GetScopedCoordinateSystem)
        .def("HasScopedCoordinateSystem",
             &UsdRiStatementsAPI::HasScopedCoordinateSystem)

        .def("GetModelCoordinateSystems", _GetModelCoordinateSystems,
             nb::rv_policy::copy)
        .def("GetModelScopedCoordinateSystems", _GetModelScopedCoordinateSystems,
             nb::rv_policy::copy)
        ;
}

} // anonymous namespace
