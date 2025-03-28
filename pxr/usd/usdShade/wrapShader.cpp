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
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

void wrapUsdShadeShader()
{
    typedef UsdShadeShader This;

    class_<This, bases<UsdTyped> >
        cls("Shader");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


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
#include "pxr/external/boost/python/return_internal_reference.hpp"

namespace {

static UsdAttribute
_CreateImplementationSourceAttr(UsdShadeShader &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateImplementationSourceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIdAttr(UsdShadeShader &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateIdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}


static object 
_WrapGetShaderId(const UsdShadeShader &shader)
{
    TfToken id;
    if (shader.GetShaderId(&id)) {
        return object(id);
    }
    return object();
}

static object 
_WrapGetSourceAsset(const UsdShadeShader &shader,
                    const TfToken &sourceType)
{
    SdfAssetPath asset;
    if (shader.GetSourceAsset(&asset, sourceType)) {
        return object(asset);
    }
    return object();
}

static object
_WrapGetSourceAssetSubIdentifier(const UsdShadeShader &shader,
                    const TfToken &sourceType)
{
    TfToken subIdentifier;
    if (shader.GetSourceAssetSubIdentifier(&subIdentifier, sourceType)) {
        return object(subIdentifier);
    }
    return object();
}

static object 
_WrapGetSourceCode(const UsdShadeShader &shader,
                   const TfToken &sourceType)
{
    std::string code;
    if (shader.GetSourceCode(&code, sourceType)) {
        return object(code);
    }
    return object();
}

WRAP_CUSTOM {
    _class
        .def(init<UsdShadeConnectableAPI>(arg("connectable")))

        .def("ConnectableAPI", &UsdShadeShader::ConnectableAPI)

        .def("GetImplementationSourceAttr",
             &UsdShadeShader::GetImplementationSourceAttr)
        .def("CreateImplementationSourceAttr",
             &_CreateImplementationSourceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetIdAttr",
             &UsdShadeShader::GetIdAttr)
        .def("CreateIdAttr",
             &_CreateIdAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("GetImplementationSource", &UsdShadeShader::GetImplementationSource)

        .def("SetShaderId", &UsdShadeShader::SetShaderId)
        .def("SetSourceAsset", &UsdShadeShader::SetSourceAsset,
            (arg("sourceAsset"), 
             arg("sourceType")=UsdShadeTokens->universalSourceType))
        .def("SetSourceAssetSubIdentifier",
            &UsdShadeShader::SetSourceAssetSubIdentifier,
            (arg("subIdentifier"),
             arg("sourceType")=UsdShadeTokens->universalSourceType))
        .def("SetSourceCode", &UsdShadeShader::SetSourceCode,
            (arg("sourceCode"), 
             arg("sourceType")=UsdShadeTokens->universalSourceType))

        .def("GetShaderId", _WrapGetShaderId)
        .def("GetSourceAsset", _WrapGetSourceAsset, 
             arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceAssetSubIdentifier", _WrapGetSourceAssetSubIdentifier,
             arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceCode", _WrapGetSourceCode, 
             arg("sourceType")=UsdShadeTokens->universalSourceType)

        .def("GetSdrMetadata", &UsdShadeShader::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &UsdShadeShader::GetSdrMetadataByKey,
             (arg("key")))

        .def("SetSdrMetadata", &UsdShadeShader::SetSdrMetadata,
             (arg("sdrMetadata")))
        .def("SetSdrMetadataByKey", &UsdShadeShader::SetSdrMetadataByKey,
             (arg("key"), arg("value")))

        .def("HasSdrMetadata", &UsdShadeShader::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &UsdShadeShader::HasSdrMetadataByKey,
             (arg("key")))

        .def("ClearSdrMetadata", &UsdShadeShader::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &UsdShadeShader::ClearSdrMetadataByKey, (arg("key")))

        .def("GetSourceTypes", 
             &UsdShadeShader::GetSourceTypes)

        .def("GetShaderNodeForSourceType", 
             &UsdShadeShader::GetShaderNodeForSourceType,
             (arg("sourceType")),
             return_internal_reference<>())

        .def("CreateOutput", &UsdShadeShader::CreateOutput,
             (arg("name"), arg("type")))
        .def("GetOutput", &UsdShadeShader::GetOutput, arg("name"))
        .def("GetOutputs", &UsdShadeShader::GetOutputs,
             (arg("onlyAuthored") = true),
             return_value_policy<TfPySequenceToList>())

        .def("CreateInput", &UsdShadeShader::CreateInput,
             (arg("name"), arg("type")))
        .def("GetInput", &UsdShadeShader::GetInput, arg("name"))
        .def("GetInputs", &UsdShadeShader::GetInputs,
             (arg("onlyAuthored") = true),
             return_value_policy<TfPySequenceToList>())

        ;
}

} // anonymous namespace
