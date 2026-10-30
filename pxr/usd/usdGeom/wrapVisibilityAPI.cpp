//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/visibilityAPI.h"
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
_CreateGuideVisibilityAttr(UsdGeomVisibilityAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateGuideVisibilityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateProxyVisibilityAttr(UsdGeomVisibilityAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateProxyVisibilityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateRenderVisibilityAttr(UsdGeomVisibilityAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateRenderVisibilityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdGeomVisibilityAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.VisibilityAPI(%s)",
        primRepr.c_str());
}

struct UsdGeomVisibilityAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdGeomVisibilityAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdGeomVisibilityAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdGeomVisibilityAPI::CanApply(prim, &whyNot);
    return UsdGeomVisibilityAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdGeomVisibilityAPI(nb::module_ &m)
{
    typedef UsdGeomVisibilityAPI This;

    UsdGeomVisibilityAPI_CanApplyResult::WrapNb<UsdGeomVisibilityAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "VisibilityAPI");

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

        
        .def("GetGuideVisibilityAttr",
             &This::GetGuideVisibilityAttr)
        .def("CreateGuideVisibilityAttr",
             &_CreateGuideVisibilityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetProxyVisibilityAttr",
             &This::GetProxyVisibilityAttr)
        .def("CreateProxyVisibilityAttr",
             &_CreateProxyVisibilityAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetRenderVisibilityAttr",
             &This::GetRenderVisibilityAttr)
        .def("CreateRenderVisibilityAttr",
             &_CreateRenderVisibilityAttr,
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
    _class
        .def("GetPurposeVisibilityAttr",
             &UsdGeomVisibilityAPI::GetPurposeVisibilityAttr,
             nb::arg("purpose"))
        ;
}

}
