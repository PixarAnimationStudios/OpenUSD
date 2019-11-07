//
// Copyright 2018 Pixar
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
#include "pxr/usd/usdShade/shaderDefParser.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include "pxr/usd/usd/stageCache.h"
#include "pxr/usd/usdShade/shader.h"

PXR_NAMESPACE_OPEN_SCOPE

using ShaderMetadataHelpers::IsPropertyATerminal;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    /* discoveryTypes */
    (usda)
    (usdc)
    (usd)

    /* property metadata */
    (primvarProperty)
    (defaultInput)
);

UsdStageCache UsdShadeShaderDefParserPlugin::_cache;

namespace {

// Called within _GetShaderPropertyTypeAndArraySize in order to fix up the
// default value's type if it was originally a token type because Sdr doesn't
// support token types
static
void
_ConformStringTypeDefaultValue(
    const SdfValueTypeName& typeName,
    VtValue* defaultValue)
{
    // If the SdrPropertyType would be string but the SdfTypeName is token,
    // we need to convert the default value's type from token to string so that
    // there is no inconsistency between the property type and the default value
    if (defaultValue && !defaultValue->IsEmpty()) {
        if (typeName == SdfValueTypeNames->Token) {
            if (defaultValue->IsHolding<TfToken>()) {
                const TfToken& tokenVal =
                    defaultValue->UncheckedGet<TfToken>();
                *defaultValue = VtValue(tokenVal.GetString());
            }
        } else if (typeName == SdfValueTypeNames->TokenArray) {
            if (defaultValue->IsHolding< VtArray<TfToken> >()) {
                const VtArray<TfToken>& tokenVals =
                    defaultValue->UncheckedGet< VtArray<TfToken> >();
                VtStringArray stringVals;
                stringVals.reserve(tokenVals.size());
                for (const TfToken& tokenVal : tokenVals) {
                    stringVals.push_back(tokenVal.GetString());
                }
                *defaultValue = VtValue::Take(stringVals);
            }
        }
    }
}

// Called within _GetShaderPropertyTypeAndArraySize in order to return the
// correct array size as determined by the given default value
static
size_t
_GetArraySize(VtValue* defaultValue) {
    if (defaultValue && !defaultValue->IsEmpty()
        && defaultValue->IsArrayValued()) {
        return defaultValue->GetArraySize();
    }
    return 0;
}

// This function is called to determine a shader property's type and array size,
// and it will also conform the default value to the correct type if needed
static 
std::pair<TfToken, size_t>
_GetShaderPropertyTypeAndArraySize(
    const SdfValueTypeName &typeName,
    const NdrTokenMap& metadata,
    VtValue* defaultValue)
{
    // XXX Note that the shaderDefParser does not currently parse 'struct' or
    //     'vstruct' types.
    //     Structs are not supported in USD but are allowed as an Sdr property
    //     type. Vstructs are not parsed at the moment because we have no need
    //     for them in USD-backed shaders currently.

    // Determine SdrPropertyType from metadata first, since metadata can
    // override the type dictated otherwise by the SdfValueTypeName
    if (IsPropertyATerminal(metadata)) {
        return std::make_pair(SdrPropertyTypes->Terminal,
                              _GetArraySize(defaultValue));
    }

    // Determine SdrPropertyType from given SdfValueTypeName
    if (typeName == SdfValueTypeNames->Int ||
        typeName == SdfValueTypeNames->IntArray) {
        return std::make_pair(SdrPropertyTypes->Int,
                              _GetArraySize(defaultValue));
    } else if (typeName == SdfValueTypeNames->String ||
               typeName == SdfValueTypeNames->Token ||
               typeName == SdfValueTypeNames->Asset || 
               typeName == SdfValueTypeNames->StringArray || 
               typeName == SdfValueTypeNames->TokenArray || 
               typeName == SdfValueTypeNames->AssetArray) {
        _ConformStringTypeDefaultValue(typeName, defaultValue);
        return std::make_pair(SdrPropertyTypes->String,
                              _GetArraySize(defaultValue));
    } else if (typeName == SdfValueTypeNames->Float || 
               typeName == SdfValueTypeNames->FloatArray) {
        return std::make_pair(SdrPropertyTypes->Float,
                              _GetArraySize(defaultValue));
    } else if (typeName == SdfValueTypeNames->Float2 || 
               typeName == SdfValueTypeNames->Float2Array) {
        return std::make_pair(SdrPropertyTypes->Float, 2);
    } else if (typeName == SdfValueTypeNames->Float3 || 
               typeName == SdfValueTypeNames->Float3Array) {
        return std::make_pair(SdrPropertyTypes->Float, 3);
    } else if (typeName == SdfValueTypeNames->Float4 || 
               typeName == SdfValueTypeNames->Float4Array) {
        return std::make_pair(SdrPropertyTypes->Float, 4);
    } else if (typeName == SdfValueTypeNames->Color3f || 
               typeName == SdfValueTypeNames->Color3fArray) {
        return std::make_pair(SdrPropertyTypes->Color,
                              _GetArraySize(defaultValue));
    } else if (typeName == SdfValueTypeNames->Point3f || 
               typeName == SdfValueTypeNames->Point3fArray) {
        return std::make_pair(SdrPropertyTypes->Point,
                              _GetArraySize(defaultValue));
    } else if (typeName == SdfValueTypeNames->Vector3f || 
               typeName == SdfValueTypeNames->Vector3fArray) {
        return std::make_pair(SdrPropertyTypes->Vector,
                              _GetArraySize(defaultValue));
    } else if (typeName == SdfValueTypeNames->Normal3f|| 
               typeName == SdfValueTypeNames->Normal3fArray) {
        return std::make_pair(SdrPropertyTypes->Normal,
                              _GetArraySize(defaultValue));
    } else if (typeName == SdfValueTypeNames->Matrix4d || 
               typeName == SdfValueTypeNames->Matrix4dArray) {
        return std::make_pair(SdrPropertyTypes->Matrix,
                              _GetArraySize(defaultValue));
    } else {
        TF_RUNTIME_ERROR("Shader property has unsupported type '%s'", 
            typeName.GetAsToken().GetText());
        return std::make_pair(SdrPropertyTypes->Unknown, 0);
    }
}

static
NdrTokenMap
_GetSdrMetadata(const UsdShadeShader &shaderDef,
                const NdrTokenMap &discoveryResultMetadata) 
{
    // XXX Currently, this parser does not support 'vstruct' parsing, but if
    //     we decide to support 'vstruct' type in the future, we would need to
    //     identify 'vstruct' types in this function by examining the metadata.

    NdrTokenMap metadata = discoveryResultMetadata;

    auto shaderDefMetadata = shaderDef.GetSdrMetadata();
    metadata.insert(shaderDefMetadata.begin(), shaderDefMetadata.end());

    // If there's an existing value in the definition, we must append to 
    // it.
    std::vector<std::string> primvarNames; 
    if (metadata.count(SdrNodeMetadata->Primvars)) {
        primvarNames.push_back(metadata.at(SdrNodeMetadata->Primvars));
    }

    for (auto &shdInput : shaderDef.GetInputs()) {
        if (shdInput.HasSdrMetadataByKey(_tokens->primvarProperty)) {
            // Check if the input holds a string here and issue a warning if it 
            // doesn't.
            if (_GetShaderPropertyTypeAndArraySize(
                    shdInput.GetTypeName(),
                    shdInput.GetSdrMetadata(),
                    nullptr).first !=
                    SdrPropertyTypes->String) {
                TF_WARN("Shader input <%s> is tagged as a primvarProperty, "
                    "but isn't string-valued.", 
                    shdInput.GetAttr().GetPath().GetText());
            }

            primvarNames.push_back("$" + shdInput.GetBaseName().GetString());
        }
    }

    metadata[SdrNodeMetadata->Primvars] = TfStringJoin(primvarNames, "|");

    return metadata;
}

template <class ShaderProperty>
static
SdrShaderPropertyUniquePtr
_CreateSdrShaderProperty(
    const ShaderProperty& shaderProperty,
    bool isOutput,
    const VtValue& shaderDefaultValue,
    const NdrTokenMap& shaderMetadata)
{
    const std::string propName = shaderProperty.GetBaseName();
    VtValue defaultValue = shaderDefaultValue;
    NdrTokenMap metadata = shaderMetadata;
    NdrTokenMap hints;
    NdrOptionVec options;

    // Update metadata if string should represent a SdfAssetPath
    if (shaderProperty.GetTypeName() == SdfValueTypeNames->Asset ||
        shaderProperty.GetTypeName() == SdfValueTypeNames->AssetArray) {
        metadata[SdrPropertyMetadata->IsAssetIdentifier] = "1";
    }

    TfToken propertyType;
    size_t arraySize;
    std::tie(propertyType, arraySize) = _GetShaderPropertyTypeAndArraySize(
        shaderProperty.GetTypeName(), shaderMetadata, &defaultValue);

    return SdrShaderPropertyUniquePtr(new SdrShaderProperty(
            shaderProperty.GetBaseName(),
            propertyType,
            defaultValue,
            isOutput,
            arraySize,
            metadata, hints, options));
}

static 
NdrPropertyUniquePtrVec
_GetShaderProperties(const UsdShadeShader &shaderDef) 
{
    NdrPropertyUniquePtrVec result;
    for (auto &shaderInput : shaderDef.GetInputs()) {
        // Only inputs will have default value provided
        VtValue defaultValue;
        shaderInput.Get(&defaultValue);

        NdrTokenMap metadata = shaderInput.GetSdrMetadata();

        // Only inputs might have this metadata key
        auto iter = metadata.find(_tokens->defaultInput);
        if (iter != metadata.end()) {
            metadata[SdrPropertyMetadata->DefaultInput] = "1";
            metadata.erase(_tokens->defaultInput);
        }

        // Only inputs have the GetConnectability method
        metadata[SdrPropertyMetadata->Connectable] =
            shaderInput.GetConnectability() == UsdShadeTokens->interfaceOnly ?
            "0" : "1";

        result.emplace_back(
            _CreateSdrShaderProperty(
                /* shaderProperty */ shaderInput,
                /* isOutput */ false,
                /* shaderDefaultValue */ defaultValue,
                /* shaderMetadata */ metadata));
    }

    for (auto &shaderOutput : shaderDef.GetOutputs()) {
        result.emplace_back(
            _CreateSdrShaderProperty(
                /* shaderProperty */ shaderOutput,
                /* isOutput */ true,
                /* shaderDefaultValue */ VtValue() ,
                /* shaderMetadata */ shaderOutput.GetSdrMetadata()));
    }

    return result;
}

} // end of anonymous namespace

NdrNodeUniquePtr 
UsdShadeShaderDefParserPlugin::Parse(
    const NdrNodeDiscoveryResult &discoveryResult)
{
    const std::string &rootLayerPath = discoveryResult.resolvedUri;

    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(rootLayerPath);
    UsdStageRefPtr stage = 
        UsdShadeShaderDefParserPlugin::_cache.FindOneMatching(rootLayer);
    if (!stage) {
        stage = UsdStage::Open(rootLayer);
        UsdShadeShaderDefParserPlugin::_cache.Insert(stage);
    }

    if (!stage) {
        return NdrParserPlugin::GetInvalidNode(discoveryResult);;
    }

    UsdPrim shaderDefPrim;
    // Fallback to looking for the subidentifier if the identifier does not
    // produce a valid shader def prim
    TfTokenVector identifiers =
        { discoveryResult.identifier, discoveryResult.subIdentifier };
    for (const TfToken& identifier : identifiers) {
        SdfPath shaderDefPath =
            SdfPath::AbsoluteRootPath().AppendChild(identifier);
        shaderDefPrim = stage->GetPrimAtPath(shaderDefPath);

        if (shaderDefPrim) {
            break;
        }
    }

    if (!shaderDefPrim) {
        return NdrParserPlugin::GetInvalidNode(discoveryResult);;
    }

    UsdShadeShader shaderDef(shaderDefPrim);
    if (!shaderDef) {
        return NdrParserPlugin::GetInvalidNode(discoveryResult);;
    }

    SdfAssetPath nodeUriAssetPath;
    if (!shaderDef.GetSourceAsset(&nodeUriAssetPath,
                                  discoveryResult.sourceType)) {
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    const std::string &resolvedNodeUri = nodeUriAssetPath.GetResolvedPath();
    if (resolvedNodeUri.empty()) {
        TF_RUNTIME_ERROR("Unable to resolve path @%s@ in shader "
            "definition file '%s'", nodeUriAssetPath.GetAssetPath().c_str(), 
            rootLayerPath.c_str());
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    return NdrNodeUniquePtr(new SdrShaderNode(
        discoveryResult.identifier, 
        discoveryResult.version,
        discoveryResult.name,
        discoveryResult.family,
        discoveryResult.discoveryType, /* discoveryType */
        discoveryResult.sourceType, /* sourceType */
        nodeUriAssetPath.GetAssetPath(),
        resolvedNodeUri,
        _GetShaderProperties(shaderDef),
        _GetSdrMetadata(shaderDef, discoveryResult.metadata),
        discoveryResult.sourceCode
    ));
    
}

const NdrTokenVec &
UsdShadeShaderDefParserPlugin::GetDiscoveryTypes() const 
{
    static const NdrTokenVec discoveryTypes{_tokens->usda, 
                                            _tokens->usdc, 
                                            _tokens->usd};
    return discoveryTypes;
}

const TfToken &
UsdShadeShaderDefParserPlugin::GetSourceType() const
{
    // The sourceType if this parser plugin is empty, because it can generate 
    // nodes of any sourceType.
    static TfToken empty;
    return empty;
}

NDR_REGISTER_PARSER_PLUGIN(UsdShadeShaderDefParserPlugin);

PXR_NAMESPACE_CLOSE_SCOPE
