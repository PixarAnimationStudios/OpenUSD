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
#include "hdPrman/debugCodes.h"
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
    (UsdTransform2d)
    (UsdPrimvarReader_float)
    (UsdPrimvarReader_float2)
    (UsdPrimvarReader_float3)

    // UsdPreviewSurface tokens
    (displacement)
    (file)
    (normal)
    (opacityThreshold)

    // UsdPreviewSurface conversion to Pxr nodes
    (PxrDisplace)
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
    (dispAmount)
    (dispAmountOut)
    (dispScalar)
    (dispScalarOut)
    (glassIor)
    (glassIorOut)
    (glowGain)
    (glowGainOut)
    (glowColor)
    (glowColorOut)
    (normalIn)
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

    // UsdUVTexture parameters
    (st)
    (wrapS)
    (wrapT)
    (useMetadata)
    (sourceColorSpace)
    (sRGB)
    (raw)
    ((colorSpaceAuto, "auto")) 

    // UsdTransform2d parameters
    (in)
    (scale)
    (translation)
    (result)

    // Dummy node used to express material primvar opinions
    (PrimvarPass)

    // Primvars set by the material
    ((displacementBoundSphere, "displacementbound:sphere"))
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
    SdfPath pxrDisplacePath;
    SdfPath primvarPassPath;

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

            // Because UsdPreviewSurfaceParameters uses "normalIn" instead of
            // UsdPreviewSurface's "normal", adjust that here.
            {
                auto it = node.parameters.find(_tokens->normal);
                if (it != node.parameters.end()) {
                    auto const value = std::move(it->second);
                    node.parameters.erase(it);
                    node.parameters.insert({_tokens->normalIn, 
                        std::move(value)});
                }
            }
            {
                auto it = node.inputConnections.find(_tokens->normal);
                if (it != node.inputConnections.end()) {
                    auto const value = std::move(it->second);
                    node.inputConnections.erase(it);
                    node.inputConnections.insert({_tokens->normalIn, 
                        std::move(value)});
                }
            }

            // Insert a PxrSurface and connect it to the above node.
            pxrSurfacePath =
                nodePath.GetParentPath().AppendChild(
                TfToken(nodePath.GetName() + "_PxrSurface"));

            // If opacityThreshold is > 0, do not use refraction.
            bool opacityThreshold = false;
            bool displacement = false;
            for (auto const& paramIt : node.parameters) {
                if (paramIt.first == _tokens->displacement) {
                    VtValue const& vtDisplacement = paramIt.second;
                    if (vtDisplacement.Get<float>() != 0.0f) {
                        displacement = true;
                    }
                } else if (paramIt.first == _tokens->opacityThreshold) {
                    VtValue const& vtOpacityThreshold = paramIt.second;
                    if (vtOpacityThreshold.Get<float>() > 0.0f) {
                        opacityThreshold = true;
                    }
                }
            }
            if (!displacement) {
                for (auto const& paramIt : node.inputConnections) {
                    if (paramIt.first == _tokens->displacement) {
                        displacement = true;
                        break;
                    }
                    continue;
                }
            }

            nodesToAdd[pxrSurfacePath] = HdMaterialNode2 {
                _tokens->PxrSurface, 
                // parameters:
                {},
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
            
            if (!opacityThreshold) {
                nodesToAdd[pxrSurfacePath].inputConnections.insert(
                    {_tokens->refractionGain,
                        {{nodePath, _tokens->refractionGainOut}}});
            }

            // Need additional node, PxrDisplace, for displacement
            if (displacement) {
                pxrDisplacePath = nodePath.GetParentPath().AppendChild(
                    TfToken(nodePath.GetName() + "_PxrDisplace"));

                nodesToAdd[pxrDisplacePath] = HdMaterialNode2 {
                    _tokens->PxrDisplace, 
                    // parameters:
                    {},
                    // connections:
                    {
                        {_tokens->dispAmount,
                            {{nodePath, _tokens->dispAmountOut}}},
                        {_tokens->dispScalar,
                            {{nodePath, _tokens->dispScalarOut}}},
                    },
                };
            }

            // One additional "dummy" node to author primvar opinions on the
            // material, to be passed to the gprim.
            primvarPassPath = nodePath.GetParentPath().AppendChild(
                TfToken(nodePath.GetName() + "_PrimvarPass"));

            nodesToAdd[primvarPassPath] = HdMaterialNode2 {
                _tokens->PrimvarPass, 
                // parameters:
                {
                    // We wish to always set this primvar on meshes using 
                    // UsdPreviewSurface, regardless of the material's
                    // displacement value. The primvar should have no effect if
                    // there is no displacement on the material, and we
                    // currently do not have the capabilities to efficiently
                    // resync the mesh if the value of its UsdPreviewSurface's 
                    // displacement input changes.
                    {_tokens->displacementBoundSphere, VtValue(1.f)}
                },
                // connections:
                {},
            };
        } else if (node.nodeTypeId == _tokens->UsdUVTexture) {
            // Update texture nodes that use non-native texture formats
            // to read them via a Renderman texture plugin.
            bool needInvertT = false;
            for (auto& param: node.parameters) {
                if (param.first == _tokens->file &&
                    param.second.IsHolding<SdfAssetPath>()) {
                    std::string path =
                        param.second.Get<SdfAssetPath>().GetResolvedPath();
                    std::string ext = ArGetResolver().GetExtension(path);
                    if (!ext.empty() && ext != "tex" && ext != "dds") {
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
                            
                        // Check for source colorspace.
                        VtValue sourceColorSpaceVal;
                        TfMapLookup(node.parameters, _tokens->sourceColorSpace,
                            &sourceColorSpaceVal);
                        // XXX: This is a workaround for Presto. If there's no
                        // colorspace token, check if there's a colorspace
                        // string.
                        TfToken sourceColorSpace = 
                            sourceColorSpaceVal.GetWithDefault(TfToken());
                        if (sourceColorSpace.IsEmpty()) {
                            const std::string sourceColorSpaceStr = 
                                sourceColorSpaceVal.GetWithDefault(
                                    _tokens->colorSpaceAuto.GetString());
                            sourceColorSpace = TfToken(sourceColorSpaceStr);
                        }
                        path =
                            TfStringPrintf("rtxplugin:%s?filename=%s"
                                           "&wrapS=%s&wrapT=%s&"
                                           "sourceColorSpace=%s",
                                           pluginName.c_str(), path.c_str(),
                                           wrapS.GetText(), wrapT.GetText(),
                                           sourceColorSpace.GetText());
                        param.second = path;
                    } else if (ext == "tex") {
                        // USD Preview Materials use a texture coordinate
                        // convention where (0,0) is in the bottom-left;
                        // RenderMan's texture system uses a convention
                        // where (0,0) is in the top-left.
                        needInvertT = true;
                    }
                    TF_DEBUG(HDPRMAN_IMAGE_ASSET_RESOLVE)
                        .Msg("Resolved preview material asset path: %s\n",
                             path.c_str());
                }
            }
            if (needInvertT &&
                node.inputConnections.find(_tokens->st)
                != node.inputConnections.end()) {
                // Invert the T axis by splicing in a UsdTransform2d node.
                SdfPath transform2dPath =
                    nodePath.GetParentPath().AppendChild(
                    TfToken(nodePath.GetName() + "_InvertT"));
                // Add new node.
                nodesToAdd[transform2dPath] = HdMaterialNode2 {
                    _tokens->UsdTransform2d, 
                    // parameters:
                    {
                        {_tokens->scale, VtValue(GfVec2f(1.0f, -1.0f))},
                        {_tokens->translation, VtValue(GfVec2f(0.0f, 1.0f))},
                    },
                    // connections:
                    {
                        {_tokens->in,
                            {node.inputConnections[_tokens->st]}},
                    },
                };
                // Splice it into UsdUvTexture, replacing the existing
                // connection.
                node.inputConnections[_tokens->st] =
                    {{ transform2dPath, _tokens->result }};
            }
        }
    }

    network.nodes.insert(nodesToAdd.begin(), nodesToAdd.end());
    if (!pxrSurfacePath.IsEmpty()) {
        network.terminals = {
            {HdMaterialTerminalTokens->surface, {pxrSurfacePath, TfToken()}}
        };

        if (!pxrDisplacePath.IsEmpty()) {
            network.terminals.insert(
                {HdMaterialTerminalTokens->displacement, {pxrDisplacePath, 
                    TfToken()}}
            );
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
