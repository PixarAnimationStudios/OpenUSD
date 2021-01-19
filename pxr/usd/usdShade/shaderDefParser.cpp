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

#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"

#include "pxr/usd/usd/stageCache.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    /* discoveryTypes */
    (usda)
    (usdc)
    (usd)
);

UsdStageCache UsdShadeShaderDefParserPlugin::_cache;

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

    metadata[SdrNodeMetadata->Primvars] = 
        UsdShadeShaderDefUtils::GetPrimvarNamesMetadataString(
            metadata, shaderDef.ConnectableAPI());

    return metadata;
}

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

    const std::string &resolvedImplementationUri = nodeUriAssetPath.GetResolvedPath();
    if (resolvedImplementationUri.empty()) {
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
        nodeUriAssetPath.GetResolvedPath(),
        resolvedImplementationUri,
        UsdShadeShaderDefUtils::GetShaderProperties(
            shaderDef.ConnectableAPI()),
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
