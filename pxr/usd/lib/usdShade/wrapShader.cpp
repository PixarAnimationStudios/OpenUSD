//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
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

        
        .def("GetImplementationSourceAttr",
             &This::GetImplementationSourceAttr)
        .def("CreateImplementationSourceAttr",
             &_CreateImplementationSourceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetIdAttr",
             &This::GetIdAttr)
        .def("CreateIdAttr",
             &_CreateIdAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

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
#include <boost/python/return_internal_reference.hpp>

namespace {

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

        .def("GetImplementationSource", &UsdShadeShader::GetImplementationSource)

        .def("SetShaderId", &UsdShadeShader::SetShaderId)
        .def("SetSourceAsset", &UsdShadeShader::SetSourceAsset,
            (arg("sourceAsset"), 
             arg("sourceType")=UsdShadeTokens->universalSourceType))
        .def("SetSourceCode", &UsdShadeShader::SetSourceCode,
            (arg("sourceCode"), 
             arg("sourceType")=UsdShadeTokens->universalSourceType))

        .def("GetShaderId", _WrapGetShaderId)
        .def("GetSourceAsset", _WrapGetSourceAsset, 
             arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceCode", _WrapGetSourceCode, 
             arg("sourceType")=UsdShadeTokens->universalSourceType)

        .def("GetShaderMetadata", &UsdShadeShader::GetShaderMetadata)
        .def("GetShaderMetadataByKey", &UsdShadeShader::GetShaderMetadataByKey,
             (arg("key")))

        .def("SetShaderMetadata", &UsdShadeShader::SetShaderMetadata,
             (arg("shaderMetadata")))
        .def("SetShaderMetadataByKey", &UsdShadeShader::SetShaderMetadataByKey,
             (arg("key"), arg("value")))

        .def("HasShaderMetadata", &UsdShadeShader::HasShaderMetadata)
        .def("HasShaderMetadataByKey", &UsdShadeShader::HasShaderMetadataByKey,
             (arg("key")))

        .def("ClearShaderMetadata", &UsdShadeShader::ClearShaderMetadata)
        .def("ClearShaderMetadataByKey", 
             &UsdShadeShader::ClearShaderMetadataByKey, (arg("key")))

        .def("GetShaderNodeForSourceType", 
             &UsdShadeShader::GetShaderNodeForSourceType,
             (arg("sourceType")),
             return_internal_reference<>())

        .def("CreateOutput", &UsdShadeShader::CreateOutput,
             (arg("name"), arg("type")))
        .def("GetOutput", &UsdShadeShader::GetOutput, arg("name"))
        .def("GetOutputs", &UsdShadeShader::GetOutputs,
             return_value_policy<TfPySequenceToList>())

        .def("CreateInput", &UsdShadeShader::CreateInput,
             (arg("name"), arg("type")))
        .def("GetInput", &UsdShadeShader::GetInput, arg("name"))
        .def("GetInputs", &UsdShadeShader::GetInputs,
             return_value_policy<TfPySequenceToList>())

        ;
}

} // anonymous namespace
