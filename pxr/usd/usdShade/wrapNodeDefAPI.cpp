//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/nodeDefAPI.h"
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
_CreateImplementationSourceAttr(UsdShadeNodeDefAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateImplementationSourceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIdAttr(UsdShadeNodeDefAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateIdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdShadeNodeDefAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdShade.NodeDefAPI(%s)",
        primRepr.c_str());
}

struct UsdShadeNodeDefAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdShadeNodeDefAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdShadeNodeDefAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdShadeNodeDefAPI::CanApply(prim, &whyNot);
    return UsdShadeNodeDefAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdShadeNodeDefAPI(nb::module_ &m)
{
    typedef UsdShadeNodeDefAPI This;

    UsdShadeNodeDefAPI_CanApplyResult::WrapNb<UsdShadeNodeDefAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "NodeDefAPI");

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

        
        .def("GetImplementationSourceAttr",
             &This::GetImplementationSourceAttr)
        .def("CreateImplementationSourceAttr",
             &_CreateImplementationSourceAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetIdAttr",
             &This::GetIdAttr)
        .def("CreateIdAttr",
             &_CreateIdAttr,
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

static nb::object 
_WrapGetShaderId(const UsdShadeNodeDefAPI &shader)
{
    TfToken id;
    if (shader.GetShaderId(&id)) {
        return nb::cast(id);
    }
    return nb::none();
}

static nb::object 
_WrapGetSourceAsset(const UsdShadeNodeDefAPI &shader,
                    const TfToken &sourceType)
{
    SdfAssetPath asset;
    if (shader.GetSourceAsset(&asset, sourceType)) {
        return nb::cast(asset);
    }
    return nb::none();
}

static nb::object
_WrapGetSourceAssetSubIdentifier(const UsdShadeNodeDefAPI &shader,
                    const TfToken &sourceType)
{
    TfToken subIdentifier;
    if (shader.GetSourceAssetSubIdentifier(&subIdentifier, sourceType)) {
        return nb::cast(subIdentifier);
    }
    return nb::none();
}

static nb::object 
_WrapGetSourceCode(const UsdShadeNodeDefAPI &shader,
                   const TfToken &sourceType)
{
    std::string code;
    if (shader.GetSourceCode(&code, sourceType)) {
        return nb::cast(code);
    }
    return nb::none();
}

WRAP_CUSTOM {
    _class
        .def("GetImplementationSource", &UsdShadeNodeDefAPI::GetImplementationSource)
        .def("SetShaderId", &UsdShadeNodeDefAPI::SetShaderId)
        .def("SetSourceAsset", &UsdShadeNodeDefAPI::SetSourceAsset,
            nb::arg("sourceAsset"), 
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("SetSourceAssetSubIdentifier",
            &UsdShadeNodeDefAPI::SetSourceAssetSubIdentifier,
            nb::arg("subIdentifier"),
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("SetSourceCode", &UsdShadeNodeDefAPI::SetSourceCode,
            nb::arg("sourceCode"), 
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetShaderId", _WrapGetShaderId)
        .def("GetSourceAsset", _WrapGetSourceAsset, 
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceAssetSubIdentifier", _WrapGetSourceAssetSubIdentifier,
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceCode", _WrapGetSourceCode, 
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceTypes", 
             &UsdShadeNodeDefAPI::GetSourceTypes)
        .def("GetShaderNodeForShadingSystem", 
             &UsdShadeNodeDefAPI::GetShaderNodeForShadingSystem,
             nb::arg("system"),
             nb::rv_policy::reference_internal)
        .def("GetShaderNodeForSourceType", 
             &UsdShadeNodeDefAPI::GetShaderNodeForSourceType,
             nb::arg("sourceType"),
             nb::rv_policy::reference_internal)
        ;
}

}
