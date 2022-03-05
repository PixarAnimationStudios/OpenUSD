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

void wrapUsdShadeShader()
{
    typedef UsdShadeShader This;

    boost::python::class_<This, boost::python::bases<UsdTyped> >
        cls("Shader");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)


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
#include <boost/python/return_internal_reference.hpp>

namespace {

static UsdAttribute
_CreateImplementationSourceAttr(UsdShadeShader &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateImplementationSourceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIdAttr(UsdShadeShader &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateIdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}


static boost::python::object 
_WrapGetShaderId(const UsdShadeShader &shader)
{
    TfToken id;
    if (shader.GetShaderId(&id)) {
        return boost::python::object(id);
    }
    return boost::python::object();
}

static boost::python::object 
_WrapGetSourceAsset(const UsdShadeShader &shader,
                    const TfToken &sourceType)
{
    SdfAssetPath asset;
    if (shader.GetSourceAsset(&asset, sourceType)) {
        return boost::python::object(asset);
    }
    return boost::python::object();
}

static boost::python::object
_WrapGetSourceAssetSubIdentifier(const UsdShadeShader &shader,
                    const TfToken &sourceType)
{
    TfToken subIdentifier;
    if (shader.GetSourceAssetSubIdentifier(&subIdentifier, sourceType)) {
        return boost::python::object(subIdentifier);
    }
    return boost::python::object();
}

static boost::python::object 
_WrapGetSourceCode(const UsdShadeShader &shader,
                   const TfToken &sourceType)
{
    std::string code;
    if (shader.GetSourceCode(&code, sourceType)) {
        return boost::python::object(code);
    }
    return boost::python::object();
}

WRAP_CUSTOM {
    _class
        .def(boost::python::init<UsdShadeConnectableAPI>(boost::python::arg("connectable")))

        .def("ConnectableAPI", &UsdShadeShader::ConnectableAPI)

        .def("GetImplementationSourceAttr",
             &UsdShadeShader::GetImplementationSourceAttr)
        .def("CreateImplementationSourceAttr",
             &_CreateImplementationSourceAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetIdAttr",
             &UsdShadeShader::GetIdAttr)
        .def("CreateIdAttr",
             &_CreateIdAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        .def("GetImplementationSource", &UsdShadeShader::GetImplementationSource)

        .def("SetShaderId", &UsdShadeShader::SetShaderId)
        .def("SetSourceAsset", &UsdShadeShader::SetSourceAsset,
            (boost::python::arg("sourceAsset"), 
             boost::python::arg("sourceType")=UsdShadeTokens->universalSourceType))
        .def("SetSourceAssetSubIdentifier",
            &UsdShadeShader::SetSourceAssetSubIdentifier,
            (boost::python::arg("subIdentifier"),
             boost::python::arg("sourceType")=UsdShadeTokens->universalSourceType))
        .def("SetSourceCode", &UsdShadeShader::SetSourceCode,
            (boost::python::arg("sourceCode"), 
             boost::python::arg("sourceType")=UsdShadeTokens->universalSourceType))

        .def("GetShaderId", _WrapGetShaderId)
        .def("GetSourceAsset", _WrapGetSourceAsset, 
             boost::python::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceAssetSubIdentifier", _WrapGetSourceAssetSubIdentifier,
             boost::python::arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceCode", _WrapGetSourceCode, 
             boost::python::arg("sourceType")=UsdShadeTokens->universalSourceType)

        .def("GetSdrMetadata", &UsdShadeShader::GetSdrMetadata)
        .def("GetSdrMetadataByKey", &UsdShadeShader::GetSdrMetadataByKey,
             (boost::python::arg("key")))

        .def("SetSdrMetadata", &UsdShadeShader::SetSdrMetadata,
             (boost::python::arg("sdrMetadata")))
        .def("SetSdrMetadataByKey", &UsdShadeShader::SetSdrMetadataByKey,
             (boost::python::arg("key"), boost::python::arg("value")))

        .def("HasSdrMetadata", &UsdShadeShader::HasSdrMetadata)
        .def("HasSdrMetadataByKey", &UsdShadeShader::HasSdrMetadataByKey,
             (boost::python::arg("key")))

        .def("ClearSdrMetadata", &UsdShadeShader::ClearSdrMetadata)
        .def("ClearSdrMetadataByKey", 
             &UsdShadeShader::ClearSdrMetadataByKey, (boost::python::arg("key")))

        .def("GetShaderNodeForSourceType", 
             &UsdShadeShader::GetShaderNodeForSourceType,
             (boost::python::arg("sourceType")),
             boost::python::return_internal_reference<>())

        .def("CreateOutput", &UsdShadeShader::CreateOutput,
             (boost::python::arg("name"), boost::python::arg("type")))
        .def("GetOutput", &UsdShadeShader::GetOutput, boost::python::arg("name"))
        .def("GetOutputs", &UsdShadeShader::GetOutputs,
             (boost::python::arg("onlyAuthored") = true),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("CreateInput", &UsdShadeShader::CreateInput,
             (boost::python::arg("name"), boost::python::arg("type")))
        .def("GetInput", &UsdShadeShader::GetInput, boost::python::arg("name"))
        .def("GetInputs", &UsdShadeShader::GetInputs,
             (boost::python::arg("onlyAuthored") = true),
             boost::python::return_value_policy<TfPySequenceToList>())

        ;
}

} // anonymous namespace
