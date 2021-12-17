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
#include "hdPrman/debugCodes.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/materialNetwork2Interface.h"
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
    (specularModelType)
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
    HdMaterialNetwork2Interface interface(&network);
    MatfiltConvertPreviewMaterial(networkId, &interface, contextValues,
                                  shaderTypePriority, outputErrorMessages);
}

// Returns a sibling path to nodeName.
// e.g.: /path/to/foo with suffix _bar would return /path/to/foo_bar
static TfToken
_GetSiblingNodeName(std::string const &nodeName, std::string const &suffix)
{
    SdfPath nodePath = SdfPath(nodeName);
    std::string siblingName = nodePath.GetName() + suffix;
    return
        nodePath.GetParentPath().AppendChild(TfToken(siblingName)).GetAsToken();
}

static bool
_GetParameter(
    HdMaterialNetworkInterface *interface,
    TfToken const &nodeName,
    TfToken const &paramName,
    VtValue *v)
{
    if (!TF_VERIFY(v)) {
        return false;
    }
    *v = std::move(interface->GetNodeParameterValue(nodeName, paramName));
    return !v->IsEmpty();
}

static bool
_GetInputConnection(
    HdMaterialNetworkInterface *interface,
    TfToken const &nodeName,
    TfToken const &inputName,
    HdMaterialNetworkInterface::InputConnectionVector *v)
{
    if (!TF_VERIFY(v)) {
        return false;
    }
    *v = std::move(interface->GetNodeInputConnection(nodeName, inputName));
    // Just check the length of the InputConnectionVector returned.
    // This skips validation of the upstreamNodeName in each connection.
    return !v->empty();
}


void
_ProcessPreviewSurfaceNode(
    HdMaterialNetworkInterface *interface,
    const TfToken &nodeName,
    std::vector<std::string> * outputErrorMessages)
{
    TF_UNUSED(outputErrorMessages);

    // Modify the node to a UsdPreviewSurfaceParameters node, which
    // translates the params to outputs that feed a PxrSurface node.
    interface->SetNodeType(nodeName, _tokens->UsdPreviewSurfaceParameters);
    
    // Because UsdPreviewSurfaceParameters uses "normalIn" instead of
    // UsdPreviewSurface's "normal", adjust that here.
    {
        VtValue vtNormal;
        if (_GetParameter(interface, nodeName, _tokens->normal, &vtNormal)) {
            interface->SetNodeParameterValue(
                nodeName, _tokens->normalIn, vtNormal);
            interface->DeleteNodeParameter(nodeName, _tokens->normal);
        }

        HdMaterialNetworkInterface::InputConnectionVector cvNormal;
        if (_GetInputConnection(
                interface, nodeName, _tokens->normal, &cvNormal)) {
            interface->SetNodeInputConnection(
                nodeName, _tokens->normalIn, cvNormal);
            interface->DeleteNodeInputConnection(nodeName, _tokens->normal);
        }
    }

    // Insert a PxrSurface and connect it to the above node.
    TfToken pxrSurfaceNodeName =
        _GetSiblingNodeName(nodeName.GetString(), "_PxrSurface");
    interface->SetNodeType(pxrSurfaceNodeName, _tokens->PxrSurface);
    // parameters:
    {
        // UsdPreviewSurface uses GGX, not Beckmann
        interface->SetNodeParameterValue(
            pxrSurfaceNodeName, _tokens->specularModelType, VtValue(int(1)));
    }
    // connections:
    {
        using TfTokenPair = std::pair<TfToken, TfToken>;
        static const std::vector<TfTokenPair> mapping = {
            {_tokens->bumpNormal, _tokens->bumpNormalOut},
            {_tokens->diffuseColor, _tokens->diffuseColorOut},
            {_tokens->diffuseGain, _tokens->diffuseGainOut},
            {_tokens->glassIor, _tokens->glassIorOut},
            {_tokens->glowColor, _tokens->glowColorOut},
            {_tokens->glowGain, _tokens->glowGainOut},
            {_tokens->specularFaceColor, _tokens->specularFaceColorOut},
            {_tokens->specularEdgeColor, _tokens->specularEdgeColorOut},
            {_tokens->specularRoughness, _tokens->specularRoughnessOut},
            {_tokens->specularIor, _tokens->specularIorOut},
            {_tokens->clearcoatFaceColor, _tokens->clearcoatFaceColorOut},
            {_tokens->clearcoatEdgeColor, _tokens->clearcoatEdgeColorOut},
            {_tokens->clearcoatRoughness, _tokens->clearcoatRoughnessOut},
            {_tokens->presence, _tokens->presenceOut}
        };

        for (const auto &inOutPair : mapping) {
            interface->SetNodeInputConnection(
                pxrSurfaceNodeName, inOutPair.first,
                {{nodeName, inOutPair.second}});
        }

        // If opacityThreshold is > 0, do *not* use refraction.
        VtValue vtOpThres;
        if (_GetParameter(
                interface, nodeName, _tokens->opacityThreshold, &vtOpThres)) {

            if (vtOpThres.Get<float>() <= 0.0f) {
                interface->SetNodeInputConnection(
                    pxrSurfaceNodeName, _tokens->refractionGain,
                    {{nodeName, _tokens->refractionGainOut}});
            }
        }
    }

    // Check for non-zero displacement param or connection
    TfToken pxrDispNodeName;
    bool displacement = false;
    {
        VtValue vtDisp;
        if (_GetParameter(
                interface, nodeName, _tokens->displacement, &vtDisp)) {
            if (vtDisp.Get<float>() != 0.0f) {
                displacement = true;
            }
        }
        if (!displacement) {
            const auto connections = interface->GetNodeInputConnection(
                                        nodeName, _tokens->displacement);
            // Note that we don't validate the connection entries themselves.
            displacement = !connections.empty();
        }
    }
    // Need additional node, PxrDisplace, for displacement
    if (displacement) {
        pxrDispNodeName =
            _GetSiblingNodeName(nodeName.GetString(), "_PxrDisplace");
        interface->SetNodeType(pxrDispNodeName, _tokens->PxrDisplace);
        // No parameters, only connections
        interface->SetNodeInputConnection(
            pxrDispNodeName, _tokens->dispAmount,
            {{nodeName, _tokens->dispAmountOut}});
        interface->SetNodeInputConnection(
            pxrDispNodeName, _tokens->dispScalar,
            {{nodeName, _tokens->dispScalarOut}});
    }

    // One additional "dummy" node to author primvar opinions on the
    // material to be passed to the gprim.
    TfToken primvarPassNodeName =
        _GetSiblingNodeName(nodeName.GetString(), "_PrimvarPass");
    interface->SetNodeType(primvarPassNodeName, _tokens->PrimvarPass);
    // Parameters (no connections):
    // We wish to always set this primvar on meshes using 
    // UsdPreviewSurface, regardless of the material's displacement value.
    // The primvar should have no effect if there is no displacement on the
    // material, and we currently do not have the capabilities to efficiently
    // resync the mesh if the value of its UsdPreviewSurface's 
    // displacement input changes.
    interface->SetNodeParameterValue(
        primvarPassNodeName, _tokens->displacementBoundSphere, VtValue(1.f));
    
    // Update network terminals to point to the PxrSurface and PxrDisplacement
    // nodes that were added.
    interface->SetTerminalConnection(HdMaterialTerminalTokens->surface,
        {pxrSurfaceNodeName, TfToken()});
    if (displacement) {
        interface->SetTerminalConnection(HdMaterialTerminalTokens->displacement,
            {pxrDispNodeName, TfToken()});
    } else {
        interface->DeleteTerminal(HdMaterialTerminalTokens->displacement);
    }
}

// Update texture nodes that use non-native texture formats
// to read them via a Renderman texture plugin.
void
_ProcessUVTextureNode(
    HdMaterialNetworkInterface *interface,
    const TfToken &nodeName,
    std::vector<std::string> * outputErrorMessages)
{
    TF_UNUSED(outputErrorMessages);

    bool needInvertT = false;
    VtValue vtFile;
    if (_GetParameter(interface, nodeName, _tokens->file, &vtFile) &&
         vtFile.IsHolding<SdfAssetPath>()) {

        std::string path = vtFile.Get<SdfAssetPath>().GetResolvedPath();
        std::string ext = ArGetResolver().GetExtension(path);

        if (!ext.empty() && ext != "tex" && ext != "dds") {
            std::string pluginName = 
                std::string("RtxHioImage") + ARCH_LIBRARY_SUFFIX;
            // Check for wrap mode. In Renderman, the
            // texture asset specifies its wrap mode, so we
            // must pass this from the shading node into the
            // texture plugin parameters.
            VtValue wrapSVal =
                interface->GetNodeParameterValue(nodeName, _tokens->wrapS);
            VtValue wrapTVal =
                interface->GetNodeParameterValue(nodeName, _tokens->wrapT);
            TfToken wrapS =
                        wrapSVal.GetWithDefault(_tokens->useMetadata);
            TfToken wrapT =
                wrapSVal.GetWithDefault(_tokens->useMetadata);  

            // Check for source colorspace.
            VtValue sourceColorSpaceVal = interface->GetNodeParameterValue(
                nodeName, _tokens->sourceColorSpace);
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
            
            interface->SetNodeParameterValue(
                nodeName, _tokens->file, VtValue(path));

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
    } // handle 'file' parameter

    HdMaterialNetworkInterface::InputConnectionVector cvSt;
    if (needInvertT &&
        _GetInputConnection(interface, nodeName, _tokens->st, &cvSt)) {

        // Invert the T axis by splicing in a UsdTransform2d node.
        TfToken transform2dNodeName =
            _GetSiblingNodeName(nodeName.GetString(), "_InvertT");
        
        // Add new node.
        interface->SetNodeType(
            transform2dNodeName, _tokens->UsdTransform2d);

        // parameters:
        interface->SetNodeParameterValue(transform2dNodeName,
            _tokens->scale, VtValue(GfVec2f(1.0f, -1.0f)));
        interface->SetNodeParameterValue(transform2dNodeName,
            _tokens->translation, VtValue(GfVec2f(0.0f, 1.0f)));

        // connections:
        interface->SetNodeInputConnection(
            transform2dNodeName, _tokens->in, cvSt);
        
        // Splice it into UsdUvTexture, replacing the existing
        // connection.
        interface->SetNodeInputConnection(nodeName, _tokens->st,
            {{ transform2dNodeName, _tokens->result }});
    }
}

void
MatfiltConvertPreviewMaterial(
    const SdfPath & networkId,
    HdMaterialNetworkInterface *interface,
    const std::map<TfToken, VtValue> & contextValues,
    const NdrTokenVec & shaderTypePriority,
    std::vector<std::string> * outputErrorMessages)
{
    TF_UNUSED(contextValues);
    TF_UNUSED(shaderTypePriority);

    if (!interface) {
        return;
    }

    const TfTokenVector nodeNames = interface->GetNodeNames();
    bool foundPreviewSurface = false;

    for (TfToken const &nodeName : nodeNames) {
        const TfToken nodeType = interface->GetNodeType(nodeName);
    
        if (nodeType == _tokens->UsdPreviewSurface) {
            if (foundPreviewSurface) {
                outputErrorMessages->push_back(
                    TfStringPrintf("Found multiple UsdPreviewSurface "
                                   "nodes in <%s>", networkId.GetText()));
                continue;
            }
            foundPreviewSurface = true;
            _ProcessPreviewSurfaceNode(
                interface, nodeName, outputErrorMessages);

        } else if (nodeType == _tokens->UsdUVTexture) {
            _ProcessUVTextureNode(interface, nodeName, outputErrorMessages);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
