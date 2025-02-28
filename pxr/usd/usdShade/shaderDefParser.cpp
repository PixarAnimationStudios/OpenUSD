//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/shaderDefParser.h"

#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"

#include "pxr/usd/usd/stageCache.h"

#include "pxr/base/tf/staticData.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    /* discoveryTypes */
    (usda)
    (usdc)
    (usd)
);

static TfStaticData<UsdStageCache> _StageCache;

static
SdrTokenMap
_GetSdrMetadata(const UsdShadeShader &shaderDef,
                const SdrTokenMap &discoveryResultMetadata) 
{
    // XXX Currently, this parser does not support 'vstruct' parsing, but if
    //     we decide to support 'vstruct' type in the future, we would need to
    //     identify 'vstruct' types in this function by examining the metadata.

    SdrTokenMap metadata = discoveryResultMetadata;

    auto shaderDefMetadata = shaderDef.GetSdrMetadata();
    metadata.insert(shaderDefMetadata.begin(), shaderDefMetadata.end());

    metadata[SdrNodeMetadata->Primvars] = 
        UsdShadeShaderDefUtils::GetPrimvarNamesMetadataString(
            metadata, shaderDef.ConnectableAPI());

    return metadata;
}

SdrShaderNodeUniquePtr 
UsdShadeShaderDefParserPlugin::ParseShaderNode(
    const SdrShaderNodeDiscoveryResult &discoveryResult)
{
    const std::string &rootLayerPath = discoveryResult.resolvedUri;

    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(rootLayerPath);
    UsdStageRefPtr stage = _StageCache->FindOneMatching(rootLayer);
    if (!stage) {
        stage = UsdStage::Open(rootLayer);
        _StageCache->Insert(stage);
    }

    if (!stage) {
        return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);;
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
        return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);;
    }

    UsdShadeShader shaderDef(shaderDefPrim);
    if (!shaderDef) {
        return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);;
    }

    SdfAssetPath nodeUriAssetPath;
    if (!shaderDef.GetSourceAsset(&nodeUriAssetPath,
                                  discoveryResult.sourceType)) {
        return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);
    }

    const std::string &resolvedImplementationUri = nodeUriAssetPath.GetResolvedPath();
    if (resolvedImplementationUri.empty()) {
        TF_RUNTIME_ERROR("Unable to resolve path @%s@ in shader "
            "definition file '%s'", nodeUriAssetPath.GetAssetPath().c_str(), 
            rootLayerPath.c_str());
        return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);
    }

    return SdrShaderNodeUniquePtr(new SdrShaderNode(
        discoveryResult.identifier, 
        discoveryResult.version,
        discoveryResult.name,
        discoveryResult.family,
        discoveryResult.discoveryType, /* discoveryType */
        discoveryResult.sourceType, /* sourceType */
        rootLayerPath,
        resolvedImplementationUri,
        UsdShadeShaderDefUtils::GetProperties(
            shaderDef.ConnectableAPI()),
        _GetSdrMetadata(shaderDef, discoveryResult.metadata),
        discoveryResult.sourceCode
    ));
    
}

const SdrTokenVec &
UsdShadeShaderDefParserPlugin::GetDiscoveryTypes() const 
{
    static const SdrTokenVec discoveryTypes{_tokens->usda, 
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

SDR_REGISTER_PARSER_PLUGIN(UsdShadeShaderDefParserPlugin);

PXR_NAMESPACE_CLOSE_SCOPE
