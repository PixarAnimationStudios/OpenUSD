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

    // Doublesided PxrSurface parameters
    (diffuseDoubleSided)
    (specularDoubleSided)
    (roughSpecularDoubleSided)
    (clearcoatDoubleSided)
);

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
    HdMaterialNetworkInterface *netInterface,
    TfToken const &nodeName,
    TfToken const &paramName,
    VtValue *v)
{
    if (!TF_VERIFY(v)) {
        return false;
    }
    *v = netInterface->GetNodeParameterValue(nodeName, paramName);
    return !v->IsEmpty();
}

static bool
_GetInputConnection(
    HdMaterialNetworkInterface *netInterface,
    TfToken const &nodeName,
    TfToken const &inputName,
    HdMaterialNetworkInterface::InputConnectionVector *v)
{
    if (!TF_VERIFY(v)) {
        return false;
    }
    *v = netInterface->GetNodeInputConnection(nodeName, inputName);
    // Just check the length of the InputConnectionVector returned.
    // This skips validation of the upstreamNodeName in each connection.
    return !v->empty();
}


void
_ProcessPreviewSurfaceNode(
    HdMaterialNetworkInterface *netInterface,
    const TfToken &nodeName,
    std::vector<std::string> *outputErrorMessages)
{
    TF_UNUSED(outputErrorMessages);

    // Modify the node to a UsdPreviewSurfaceParameters node, which
    // translates the params to outputs that feed a PxrSurface node.
    netInterface->SetNodeType(nodeName, _tokens->UsdPreviewSurfaceParameters);
    
    // Because UsdPreviewSurfaceParameters uses "normalIn" instead of
    // UsdPreviewSurface's "normal", adjust that here.
    {
        VtValue vtNormal;
        if (_GetParameter(netInterface, nodeName, _tokens->normal, &vtNormal)) {
            netInterface->SetNodeParameterValue(
                nodeName, _tokens->normalIn, vtNormal);
            netInterface->DeleteNodeParameter(nodeName, _tokens->normal);
        }

        HdMaterialNetworkInterface::InputConnectionVector cvNormal;
        if (_GetInputConnection(
                netInterface, nodeName, _tokens->normal, &cvNormal)) {
            netInterface->SetNodeInputConnection(
                nodeName, _tokens->normalIn, cvNormal);
            netInterface->DeleteNodeInputConnection(nodeName, _tokens->normal);
        }
    }

    // Insert a PxrSurface and connect it to the above node.
    TfToken pxrSurfaceNodeName =
        _GetSiblingNodeName(nodeName.GetString(), "_PxrSurface");
    netInterface->SetNodeType(pxrSurfaceNodeName, _tokens->PxrSurface);
    // parameters:
    {
        // UsdPreviewSurface uses GGX, not Beckmann
        netInterface->SetNodeParameterValue(
            pxrSurfaceNodeName, _tokens->specularModelType, VtValue(int(1)));

        // Set up for backfacing -- the PxrSurface will always expect to shade
        // both front- and back-facing; UsdPreviewSurfaceParams will determine 
        // whether it's been called to shade a backface and whether it should.
        // NB: These parameters are NOT CONNECTABLE, so cannot be switched via
        // an output from the UsdPreviewSurfaceParameters shader!
        netInterface->SetNodeParameterValue(
            pxrSurfaceNodeName, _tokens->diffuseDoubleSided, VtValue(int(1))
        );
        netInterface->SetNodeParameterValue(
            pxrSurfaceNodeName, _tokens->specularDoubleSided, VtValue(int(1))
        );
        netInterface->SetNodeParameterValue(
            pxrSurfaceNodeName, _tokens->roughSpecularDoubleSided, VtValue(int(1))
        );
        netInterface->SetNodeParameterValue(
            pxrSurfaceNodeName, _tokens->clearcoatDoubleSided, VtValue(int(1))
        );

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
            netInterface->SetNodeInputConnection(
                pxrSurfaceNodeName, inOutPair.first,
                {{nodeName, inOutPair.second}});
        }

        // If opacityThreshold is > 0, do *not* use refraction.
        VtValue vtOpThres;
        if (_GetParameter(
                netInterface, nodeName, _tokens->opacityThreshold,
                &vtOpThres)) {

            if (vtOpThres.Get<float>() <= 0.0f) {
                netInterface->SetNodeInputConnection(
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
                netInterface, nodeName, _tokens->displacement, &vtDisp)) {
            if (vtDisp.Get<float>() != 0.0f) {
                displacement = true;
            }
        }
        if (!displacement) {
            const auto connections = netInterface->GetNodeInputConnection(
                                        nodeName, _tokens->displacement);
            // Note that we don't validate the connection entries themselves.
            displacement = !connections.empty();
        }
    }
    // Need additional node, PxrDisplace, for displacement
    if (displacement) {
        pxrDispNodeName =
            _GetSiblingNodeName(nodeName.GetString(), "_PxrDisplace");
        netInterface->SetNodeType(pxrDispNodeName, _tokens->PxrDisplace);
        // No parameters, only connections
        netInterface->SetNodeInputConnection(
            pxrDispNodeName, _tokens->dispAmount,
            {{nodeName, _tokens->dispAmountOut}});
        netInterface->SetNodeInputConnection(
            pxrDispNodeName, _tokens->dispScalar,
            {{nodeName, _tokens->dispScalarOut}});
    }

    // One additional "dummy" node to author primvar opinions on the
    // material to be passed to the gprim.
    TfToken primvarPassNodeName =
        _GetSiblingNodeName(nodeName.GetString(), "_PrimvarPass");
    netInterface->SetNodeType(primvarPassNodeName, _tokens->PrimvarPass);
    // Parameters (no connections):
    // We wish to always set this primvar on meshes using 
    // UsdPreviewSurface, regardless of the material's displacement value.
    // The primvar should have no effect if there is no displacement on the
    // material, and we currently do not have the capabilities to efficiently
    // resync the mesh if the value of its UsdPreviewSurface's 
    // displacement input changes.
    netInterface->SetNodeParameterValue(
        primvarPassNodeName, _tokens->displacementBoundSphere, VtValue(1.f));

    // XXX Wire the primvarPass node so it isn't pruned during network traversal.
    netInterface->SetNodeInputConnection(
        pxrSurfaceNodeName, _tokens->displacementBoundSphere,
        {{primvarPassNodeName, _tokens->displacementBoundSphere}});
    
    // Update network terminals to point to the PxrSurface and PxrDisplacement
    // nodes that were added.
    netInterface->SetTerminalConnection(HdMaterialTerminalTokens->surface,
        {pxrSurfaceNodeName, TfToken()});
    if (displacement) {
        netInterface->SetTerminalConnection(
            HdMaterialTerminalTokens->displacement,
            {pxrDispNodeName, TfToken()});
    } else {
        netInterface->DeleteTerminal(HdMaterialTerminalTokens->displacement);
    }
}

// Returns true if the given path is already a RtxPath meaning it is already
// of the form:
//  "rtxplugin:<>?filename=<>&wrapS=<>&wrapT=<>&sourceColorSpace=<>s"
// This function checks if the path contains the "rtxPlugin:" prefix
static bool
_RtxPath(std::string path)
{
    const std::string pathPrefix = "rtxplugin:";
    return path.substr(0, pathPrefix.length()) == pathPrefix;
}

// Update texture nodes that use non-native texture formats
// to read them via a Renderman texture plugin.
void
_ProcessUVTextureNode(
    HdMaterialNetworkInterface * netInterface,
    const TfToken &nodeName,
    std::vector<std::string> *outputErrorMessages)
{
    TF_UNUSED(outputErrorMessages);

    bool needInvertT = false;
    VtValue vtFile;
    if (_GetParameter(netInterface, nodeName, _tokens->file, &vtFile) &&
        (vtFile.IsHolding<SdfAssetPath>() || vtFile.IsHolding<std::string>())) {

        std::string path = vtFile.IsHolding<SdfAssetPath>()
            ? vtFile.Get<SdfAssetPath>().GetResolvedPath()
            : vtFile.Get<std::string>();
        std::string ext = ArGetResolver().GetExtension(path);

        if (!ext.empty() && ext != "tex" && ext != "dds" && !_RtxPath(path)) {
            std::string pluginName = 
                std::string("RtxHioImage") + ARCH_LIBRARY_SUFFIX;
            // Check for wrap mode. In Renderman, the
            // texture asset specifies its wrap mode, so we
            // must pass this from the shading node into the
            // texture plugin parameters.
            VtValue wrapSVal =
                netInterface->GetNodeParameterValue(nodeName, _tokens->wrapS);
            VtValue wrapTVal =
                netInterface->GetNodeParameterValue(nodeName, _tokens->wrapT);
            TfToken wrapS =
                wrapSVal.GetWithDefault(_tokens->useMetadata);
            TfToken wrapT =
                wrapSVal.GetWithDefault(_tokens->useMetadata);  

            // Check for source colorspace.
            VtValue sourceColorSpaceVal = netInterface->GetNodeParameterValue(
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

            netInterface->SetNodeParameterValue(
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
        _GetInputConnection(netInterface, nodeName, _tokens->st, &cvSt)) {

        // Invert the T axis by splicing in a UsdTransform2d node.
        TfToken transform2dNodeName =
            _GetSiblingNodeName(nodeName.GetString(), "_InvertT");
        
        // Add new node.
        netInterface->SetNodeType(
            transform2dNodeName, _tokens->UsdTransform2d);

        // parameters:
        netInterface->SetNodeParameterValue(transform2dNodeName,
            _tokens->scale, VtValue(GfVec2f(1.0f, -1.0f)));
        netInterface->SetNodeParameterValue(transform2dNodeName,
            _tokens->translation, VtValue(GfVec2f(0.0f, 1.0f)));

        // connections:
        netInterface->SetNodeInputConnection(
            transform2dNodeName, _tokens->in, cvSt);
        
        // Splice it into UsdUvTexture, replacing the existing
        // connection.
        netInterface->SetNodeInputConnection(nodeName, _tokens->st,
            {{ transform2dNodeName, _tokens->result }});
    }
}

void
MatfiltConvertPreviewMaterial(
    HdMaterialNetworkInterface *netInterface,
    std::vector<std::string> *outputErrorMessages)
{
    if (!netInterface) {
        return;
    }

    TfTokenVector nodeNames = netInterface->GetNodeNames();
    if(nodeNames.size() > 1) {
        std::unordered_set<TfToken, TfHash> nameSet;
        nameSet.insert(nodeNames.cbegin(), nodeNames.cend());
        nodeNames.clear();
        nodeNames.insert(nodeNames.end(), nameSet.cbegin(), nameSet.cend());
    }

    bool foundPreviewSurface = false;

    for (TfToken const &nodeName : nodeNames) {
        const TfToken nodeType = netInterface->GetNodeType(nodeName);
    
        if (nodeType == _tokens->UsdPreviewSurface) {
            if (foundPreviewSurface) {
                outputErrorMessages->push_back(TfStringPrintf(
                    "Found multiple UsdPreviewSurface nodes in <%s>",
                    netInterface->GetMaterialPrimPath().GetText()));
                continue;
            }
            foundPreviewSurface = true;
            _ProcessPreviewSurfaceNode(
                netInterface, nodeName, outputErrorMessages);

        } else if (nodeType == _tokens->UsdUVTexture) {
            _ProcessUVTextureNode(netInterface, nodeName, outputErrorMessages);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
