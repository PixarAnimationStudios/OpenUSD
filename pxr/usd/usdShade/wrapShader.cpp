//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/shader.h"
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
_Repr(const UsdShadeShader &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdShade.Shader(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdShadeShader(nb::module_ &m)
{
    typedef UsdShadeShader This;

    nb::class_<This, UsdTyped> cls(m, "Shader");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))

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

#include "pxr/usd/usdShade/connectableAPI.h"
#include <nanobind/stl/unordered_map.h>

namespace {

static UsdAttribute
_CreateImplementationSourceAttr(UsdShadeShader &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateImplementationSourceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIdAttr(UsdShadeShader &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateIdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static nb::object 
_WrapGetShaderId(const UsdShadeShader &shader)
{
    TfToken id;
    if (shader.GetShaderId(&id)) {
        return nb::cast(id);
    }
    return nb::none();
}

static nb::object 
_WrapGetSourceAsset(const UsdShadeShader &shader,
                    const TfToken &sourceType)
{
    SdfAssetPath asset;
    if (shader.GetSourceAsset(&asset, sourceType)) {
        return nb::cast(asset);
    }
    return nb::none();
}

static nb::object
_WrapGetSourceAssetSubIdentifier(const UsdShadeShader &shader,
                    const TfToken &sourceType)
{
    TfToken subIdentifier;
    if (shader.GetSourceAssetSubIdentifier(&subIdentifier, sourceType)) {
        return nb::cast(subIdentifier);
    }
    return nb::none();
}

static nb::object 
_WrapGetSourceCode(const UsdShadeShader &shader,
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
        .def(nb::init<UsdShadeConnectableAPI>(), nb::arg("connectable"))

        .def("ConnectableAPI", &UsdShadeShader::ConnectableAPI)

        .def("GetImplementationSourceAttr",
             &UsdShadeShader::GetImplementationSourceAttr)
        .def("CreateImplementationSourceAttr",
             &_CreateImplementationSourceAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetIdAttr",
             &UsdShadeShader::GetIdAttr)
        .def("CreateIdAttr",
             &_CreateIdAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("GetImplementationSource", &UsdShadeShader::GetImplementationSource)

        .def("SetShaderId", &UsdShadeShader::SetShaderId)
        .def("SetSourceAsset", &UsdShadeShader::SetSourceAsset,
            nb::arg("sourceAsset"), 
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("SetSourceAssetSubIdentifier",
            &UsdShadeShader::SetSourceAssetSubIdentifier,
            nb::arg("subIdentifier"),
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("SetSourceCode", &UsdShadeShader::SetSourceCode,
            nb::arg("sourceCode"), 
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)

        .def("GetShaderId", _WrapGetShaderId)
        .def("GetSourceAsset", _WrapGetSourceAsset, 
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceAssetSubIdentifier", _WrapGetSourceAssetSubIdentifier,
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceCode", _WrapGetSourceCode, 
             nb::arg("sourceType")=UsdShadeTokens->universalSourceType)

        .def("GetSdrMetadata", &UsdShadeShader::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &UsdShadeShader::GetSdrMetadataByKey,
             nb::arg("key"))

        .def("SetSdrMetadata", &UsdShadeShader::SetSdrMetadata,
             nb::arg("sdrMetadata"))
        .def("SetSdrMetadataByKey", &UsdShadeShader::SetSdrMetadataByKey,
             nb::arg("key"), nb::arg("value"))

        .def("HasSdrMetadata", &UsdShadeShader::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &UsdShadeShader::HasSdrMetadataByKey,
             nb::arg("key"))

        .def("ClearSdrMetadata", &UsdShadeShader::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &UsdShadeShader::ClearSdrMetadataByKey, nb::arg("key"))

        .def("GetSourceTypes", 
             &UsdShadeShader::GetSourceTypes)

        .def("GetShaderNodeForSourceType", 
             &UsdShadeShader::GetShaderNodeForSourceType,
             nb::arg("sourceType"),
             nb::rv_policy::reference_internal)

        .def("CreateOutput", &UsdShadeShader::CreateOutput,
             nb::arg("name"), nb::arg("type"))
        .def("GetOutput", &UsdShadeShader::GetOutput, nb::arg("name"))
        .def("GetOutputs", &UsdShadeShader::GetOutputs,
             (nb::arg("onlyAuthored") = true),
             nb::rv_policy::copy)

        .def("CreateInput", &UsdShadeShader::CreateInput,
             nb::arg("name"), nb::arg("type"))
        .def("GetInput", &UsdShadeShader::GetInput, nb::arg("name"))
        .def("GetInputs", &UsdShadeShader::GetInputs,
             (nb::arg("onlyAuthored") = true),
             nb::rv_policy::copy)

        ;
}

} // anonymous namespace
