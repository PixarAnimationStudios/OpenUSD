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

#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/resolverContextBinder.h"

#include "pxr/usd/usd/stageCache.h"
#include "pxr/usd/usdShade/shader.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    /* discoveryTypes */
    (usda)
    (usdc)
    (usd)

    /* property metadata */
    (primvar)
    (primvarProperty)
    (defaultInput)
);

UsdStageCache UsdShadeShaderDefParserPlugin::_cache;

namespace {

static 
std::pair<TfToken, size_t>
_GetShaderPropertyTypeAndArraySize(const SdfValueTypeName &typeName) 
{
    if (typeName == SdfValueTypeNames->Int ||
        typeName == SdfValueTypeNames->IntArray) {
        return std::make_pair(SdrPropertyTypes->Int, 0);
    } else if (typeName == SdfValueTypeNames->String ||
               typeName == SdfValueTypeNames->Token ||
               typeName == SdfValueTypeNames->Asset || 
               typeName == SdfValueTypeNames->StringArray || 
               typeName == SdfValueTypeNames->TokenArray || 
               typeName == SdfValueTypeNames->AssetArray) {
        return std::make_pair(SdrPropertyTypes->String, 0);
    } else if (typeName == SdfValueTypeNames->Float || 
               typeName == SdfValueTypeNames->FloatArray) {
        return std::make_pair(SdrPropertyTypes->Float, 0);
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
        return std::make_pair(SdrPropertyTypes->Color, 0);
    } else if (typeName == SdfValueTypeNames->Point3f || 
               typeName == SdfValueTypeNames->Point3fArray) {
        return std::make_pair(SdrPropertyTypes->Point, 0);
    } else if (typeName == SdfValueTypeNames->Vector3f || 
               typeName == SdfValueTypeNames->Vector3fArray) {
        return std::make_pair(SdrPropertyTypes->Vector, 0);
    } else if (typeName == SdfValueTypeNames->Normal3f|| 
               typeName == SdfValueTypeNames->Normal3fArray) {
        return std::make_pair(SdrPropertyTypes->Normal, 0);
    } else if (typeName == SdfValueTypeNames->Matrix4d || 
               typeName == SdfValueTypeNames->Matrix4dArray) {
        return std::make_pair(SdrPropertyTypes->Matrix, 0);
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
                    shdInput.GetTypeName()).first != SdrPropertyTypes->String) {
                TF_WARN("Shader input <%s> is tagged as a primvarProperty, "
                    "but isn't string-valued.", 
                    shdInput.GetAttr().GetPath().GetText());
            }

            primvarNames.push_back("$" + shdInput.GetBaseName().GetString());
        } else if (shdInput.HasSdrMetadataByKey(_tokens->primvar)) {
            primvarNames.push_back(shdInput.GetBaseName().GetString());
        }
    }

    metadata[SdrNodeMetadata->Primvars] = TfStringJoin(primvarNames, "|");

    return metadata;
}

static 
NdrPropertyUniquePtrVec
_GetShaderProperties(const UsdShadeShader &shaderDef) 
{
    NdrPropertyUniquePtrVec result;
    for (auto &shaderInput : shaderDef.GetInputs()) {
        const std::string propName = shaderInput.GetBaseName();
        VtValue defaultValue;
        shaderInput.Get(&defaultValue);

        NdrTokenMap metadata = shaderInput.GetSdrMetadata();
        NdrTokenMap hints;
        NdrOptionVec options;
    
        // Convert SdfAssetPath values to strings.
        if (defaultValue.IsHolding<SdfAssetPath>()) {
            defaultValue = VtValue(
                defaultValue.UncheckedGet<SdfAssetPath>().GetAssetPath());
            metadata[SdrPropertyMetadata->IsAssetIdentifier] = "1";
        }

        auto iter = metadata.find(_tokens->defaultInput);
        if (iter != metadata.end()) {
            metadata[SdrPropertyMetadata->DefaultInput] = "1";
            metadata.erase(_tokens->defaultInput);
        }

        metadata[SdrPropertyMetadata->Connectable] = 
            shaderInput.GetConnectability() == UsdShadeTokens->interfaceOnly ?
                "0" : "1";

        TfToken propertyType;
        size_t arraySize;
        std::tie(propertyType, arraySize) = _GetShaderPropertyTypeAndArraySize(
            shaderInput.GetTypeName());
        
        result.emplace_back(
            SdrShaderPropertyUniquePtr(new SdrShaderProperty(
                shaderInput.GetBaseName(),
                propertyType, 
                defaultValue,
                /* isOutput */ false,
                arraySize,
                metadata, hints, options)));
    }

    for (auto &shaderOutput : shaderDef.GetOutputs()) {
        const std::string propName = shaderOutput.GetBaseName();

        VtValue defaultValue;
        NdrTokenMap metadata;
        NdrTokenMap hints;
        NdrOptionVec options;

        TfToken propertyType;
        size_t arraySize;
        std::tie(propertyType, arraySize) = _GetShaderPropertyTypeAndArraySize(
            shaderOutput.GetTypeName());

        result.emplace_back(
            SdrShaderPropertyUniquePtr(new SdrShaderProperty(
                shaderOutput.GetBaseName(),
                propertyType, 
                defaultValue,
                /* isOutput */ true,
                arraySize,
                metadata, hints, options)));
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

    SdfPath shaderDefPath = SdfPath::AbsoluteRootPath().AppendChild(
            discoveryResult.identifier);

    auto shaderDefPrim = stage->GetPrimAtPath(shaderDefPath);
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

    auto resolverContext = ArGetResolver().CreateDefaultContextForAsset(
            rootLayerPath);
    ArResolverContextBinder binder(resolverContext);
    std::string nodeUri = ArGetResolver().Resolve(
            nodeUriAssetPath.GetAssetPath());

    if (nodeUri.empty()) {
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
        nodeUri,
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
