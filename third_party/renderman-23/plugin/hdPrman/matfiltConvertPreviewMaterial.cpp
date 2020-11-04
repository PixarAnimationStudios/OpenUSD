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
#include "hdPrman/matfiltConvertPreviewMaterial.h"
#include "hdPrman/context.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_OPEN_SCOPE

// Tokens for converting UsdPreviewSurface
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

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
    (presence)
    (presenceOut)
    (allowPresenceWithGlass)

    // UsdUVTexture parameters
    (wrapS)
    (wrapT)
    (useMetadata)
);

void
MatfiltConvertPreviewMaterial(
    const SdfPath & networkId,
    HdMaterialNetwork2 & network,
    const std::map<TfToken, VtValue> & contextValues,
    const NdrTokenVec & shaderTypePriority,
    std::vector<std::string> * outputErrorMessages)
{
    std::map<SdfPath, HdMaterialNode2> nodesToAdd;

    SdfPath pxrSurfacePath;

    for (auto& nodeEntry: network.nodes) {
        SdfPath const& nodePath = nodeEntry.first;
        HdMaterialNode2 &node = nodeEntry.second;

        if (node.nodeTypeId == _tokens->UsdPreviewSurface) {
            if (!pxrSurfacePath.IsEmpty()) {
                outputErrorMessages->push_back(
                    TfStringPrintf("Found multiple UsdPreviewSurface "
                                   "nodes in <%s>", networkId.GetText()));
                continue;
            }
            // Modify the node to a UsdPreviewSurfaceParameters node, which
            // translates the params to outputs that feed a PxrSurface node.
            node.nodeTypeId = _tokens->UsdPreviewSurfaceParameters;

            // Insert a PxrSurface and connect it to the above node.
            pxrSurfacePath =
                nodePath.GetParentPath().AppendChild(
                TfToken(nodePath.GetName() + "_PxrSurface"));

            nodesToAdd[pxrSurfacePath] = HdMaterialNode2 {
                _tokens->PxrSurface, 
                // parameters:
                {
                    {_tokens->allowPresenceWithGlass, VtValue(1)},
                },
                // connections:
                {
                    {_tokens->bumpNormal,
                        {{nodePath, _tokens->bumpNormalOut}}},
                    {_tokens->diffuseColor,
                        {{nodePath, _tokens->diffuseColorOut}}},
                    {_tokens->diffuseGain,
                        {{nodePath, _tokens->diffuseGainOut}}},
                    {_tokens->glassIor,
                        {{nodePath, _tokens->glassIorOut}}},
                    {_tokens->glowColor,
                        {{nodePath, _tokens->glowColorOut}}},
                    {_tokens->glowGain,
                        {{nodePath, _tokens->glowGainOut}}},
                    {_tokens->refractionGain,
                        {{nodePath, _tokens->refractionGainOut}}},
                    {_tokens->specularFaceColor,
                        {{nodePath, _tokens->specularFaceColorOut}}},
                    {_tokens->specularEdgeColor,
                        {{nodePath, _tokens->specularEdgeColorOut}}},
                    {_tokens->specularRoughness,
                        {{nodePath, _tokens->specularRoughnessOut}}},
                    {_tokens->specularIor,
                        {{nodePath, _tokens->specularIorOut}}},
                    {_tokens->clearcoatFaceColor,
                        {{nodePath, _tokens->clearcoatFaceColorOut}}},
                    {_tokens->clearcoatEdgeColor,
                        {{nodePath, _tokens->clearcoatEdgeColorOut}}},
                    {_tokens->clearcoatRoughness,
                        {{nodePath, _tokens->clearcoatRoughnessOut}}},
                    {_tokens->presence,
                        {{nodePath, _tokens->presenceOut}}},
                },
            };

        } else if (node.nodeTypeId == _tokens->UsdUVTexture) {
            // Update texture nodes that use non-native texture formats
            // to read them via a Renderman texture plugin.
            for (auto& param: node.parameters) {
                if (param.first == _tokens->file &&
                    param.second.IsHolding<SdfAssetPath>()) {
                    std::string path =
                        param.second.Get<SdfAssetPath>().GetResolvedPath();
                    std::string ext = ArGetResolver().GetExtension(path);
                    if (!ext.empty() && ext != "tex") {
                        std::string pluginName = 
                            std::string("RtxHioImage") + ARCH_LIBRARY_SUFFIX;
                        // Check for wrap mode. In Renderman, the
                        // texture asset specifies its wrap mode, so we
                        // must pass this from the shading node into the
                        // texture plugin parameters.
                        VtValue wrapSVal, wrapTVal;
                        TfMapLookup(node.parameters, _tokens->wrapS, &wrapSVal);
                        TfMapLookup(node.parameters, _tokens->wrapT, &wrapTVal);
                        TfToken wrapS =
                            wrapSVal.GetWithDefault(_tokens->useMetadata);
                        TfToken wrapT =
                            wrapSVal.GetWithDefault(_tokens->useMetadata);
                        param.second =
                            TfStringPrintf("rtxplugin:%s?filename=%s"
                                           "&wrapS=%s&wrapT=%s",
                                           pluginName.c_str(), path.c_str(),
                                           wrapS.GetText(), wrapT.GetText());
                    }
                }
            }
        }
    }

    network.nodes.insert(nodesToAdd.begin(), nodesToAdd.end());
    if (!pxrSurfacePath.IsEmpty()) {
        // Use PxrSurface as sole terminal.  Displacement is not supported.
        network.terminals = {
            {HdMaterialTerminalTokens->surface, {pxrSurfacePath, TfToken()}}
        };
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
