//
// Copyright 2019 Pixar
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
#include "hdPrman/material.h"
#include "hdPrman/context.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

PXR_NAMESPACE_OPEN_SCOPE

// Tokens for converting UsdPreviewSurface
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Network types
    (bxdf)
    (displacement)

    // Usd preview shading node types
    (UsdPreviewSurface)
    (UsdUVTexture)
    (UsdPrimvarReader_float)
    (UsdPrimvarReader_float2)
    (UsdPrimvarReader_float3)

    // UsdPreviewSurface tokens
    (file)

    // UsdPreviewSurface conversion to Pxr nodes
    (PxrSurface)

    // Usd preview shading nodes osl tokens
    (UsdPreviewSurfaceParameters)
    (bumpNormal)
    (bumpNormalOut)
    (clearcoatEdgeColor)
    (clearcoatEdgeColorOut)
    (clearcoatFaceColor)
    (clearcoatFaceColorOut)
    (clearcoatRoughness)
    (clearcoatRoughnessOut)
    (diffuseGain)
    (diffuseGainOut)
    (diffuseColor)
    (diffuseColorOut)
    (glassIor)
    (glassIorOut)
    (glowGain)
    (glowGainOut)
    (glowColor)
    (glowColorOut)
    (refractionGain)
    (refractionGainOut)
    (specularEdgeColor)
    (specularEdgeColorOut)
    (specularFaceColor)
    (specularFaceColorOut)
    (specularIor)
    (specularIorOut)
    (specularRoughness)
    (specularRoughnessOut)
);

void HdPrman_ConvertUsdPreviewMaterial(HdMaterialNetworkMap *netMap)
{
    HdMaterialNetwork bxdfNet, dispNet;
    TfMapLookup(netMap->map, _tokens->bxdf, &bxdfNet);
    TfMapLookup(netMap->map, _tokens->displacement, &dispNet);

    bool previewMaterialFound = false;
    std::vector<HdMaterialNode> nodesToAdd;
    for (HdMaterialNode &node: bxdfNet.nodes) {
        if (node.identifier == _tokens->UsdPreviewSurface) {
            previewMaterialFound = true;

            // Change the node to a UsdPreviewSurfaceParameters node, which
            // translates the params to outputs that feed into a PxrSurface
            // node.
            node.identifier = _tokens->UsdPreviewSurfaceParameters;

            // Create a new PxrSurface node and add it to the network.
            SdfPath pxrSurfacePath = node.path.GetParentPath().AppendChild(
                TfToken(node.path.GetName() + "_PxrSurface"));
            HdMaterialNode pxrSurfaceNode = { 
                pxrSurfacePath, 
                _tokens->PxrSurface, 
                {},
            };
            nodesToAdd.push_back(pxrSurfaceNode);

            // Create new connections from the UsdPreviewSurfaceParameters 
            // node to the PxrSurface node.
            std::vector<std::pair<TfToken, TfToken>> inputOutputMapping = {
                {_tokens->bumpNormalOut, _tokens->bumpNormal},
                {_tokens->diffuseColorOut, _tokens->diffuseColor},
                {_tokens->diffuseGainOut, _tokens->diffuseGain},
                {_tokens->glassIorOut, _tokens->glassIor},
                {_tokens->glowColorOut, _tokens->glowColor},
                {_tokens->glowGainOut, _tokens->glowGain},
                {_tokens->refractionGainOut, _tokens->refractionGain},
                {_tokens->specularFaceColorOut, _tokens->specularFaceColor},
                {_tokens->specularEdgeColorOut, _tokens->specularEdgeColor},
                {_tokens->specularRoughnessOut, _tokens->specularRoughness},
                {_tokens->specularIorOut, _tokens->specularIor},
                {_tokens->clearcoatFaceColorOut, _tokens->clearcoatFaceColor},
                {_tokens->clearcoatEdgeColorOut, _tokens->clearcoatEdgeColor},
                {_tokens->clearcoatRoughnessOut, _tokens->clearcoatRoughness},
            };

            for (const auto &mapping : inputOutputMapping) {
                bxdfNet.relationships.emplace_back( HdMaterialRelationship{
                    node.path, mapping.first, pxrSurfacePath, mapping.second
                });
            }
        } else if (node.identifier == _tokens->UsdUVTexture) {
            for (auto &param: node.parameters) {
                if (param.first == _tokens->file) {
                    if (param.second.IsHolding<SdfAssetPath>()) {
                        std::string path =
                            param.second.Get<SdfAssetPath>().GetResolvedPath();
                        std::string ext = ArGetResolver().GetExtension(path);

                        // Renderman can read its .tex format natively,
                        // but in other cases we must use a texture plugin.
                        if (!ext.empty() && ext != "tex") {
                            const std::string pluginName = 
                                std::string("RtxGlfImage") + ARCH_LIBRARY_SUFFIX;
                            path = TfStringPrintf("rtxplugin:%s?filename=%s",
                                pluginName.c_str(), path.c_str());
                            param.second = VtValue(path);
                        }
                    }
                }
            }
        }
    }

    bxdfNet.nodes.insert(bxdfNet.nodes.end(), nodesToAdd.begin(), 
            nodesToAdd.end());

    if (previewMaterialFound) {
        // Commit converted networks
        netMap->map[_tokens->bxdf] = bxdfNet;
        // XXX: Support displacement.  For now, just eject it.
        netMap->map.erase(_tokens->displacement);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
