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

#include "pxr/imaging/hdSt/materialNetwork.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

#include "pxr/usd/sdf/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

typedef std::unique_ptr<HioGlslfx> HioGlslfxUniquePtr;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (opacity)
);


/// \struct HdStMaterialConnection
///
/// Describes a single connection to an upsream node and output port 
///
/// XXX Replacement for HdRelationship. Unify with HdPrman: MatfiltConnection.
struct HdSt_MaterialConnection {
    SdfPath upstreamNode;
    TfToken upstreamOutputName;
};

/// \struct HdStMaterialNode
///
/// Describes an instance of a node within a network
/// A node contains a (shader) type identifier, parameter values, and 
/// connections to upstream nodes. A single input (mapped by TfToken) may have
/// multiple upstream connections to describe connected array elements.
///
/// XXX Replacement for HdMaterialNode. Unify with HdPrman: MatfiltNode.
struct HdSt_MaterialNode {
    TfToken nodeTypeId;
    std::map<TfToken, VtValue> parameters;
    std::map<TfToken, std::vector<HdSt_MaterialConnection>> inputConnections;
};

/// \struct HdStMaterialNetwork
/// 
/// Container of nodes and top-level terminal connections. This is the mutable
/// representation of a shading network sent to filtering functions by a
/// MatfiltFilterChain.
///
/// XXX Replacement for HdMaterialNetwork. Unify with HdPrman: MatfiltNetwork.
struct HdSt_MaterialNetwork {
    std::map<SdfPath, HdSt_MaterialNode> nodes;
    std::map<TfToken, HdSt_MaterialConnection> terminals;
    TfTokenVector primvars;
};

// XXX We wish to transition to a new material network description and unify
// with HdPrman's MatfiltNetwork. For now we internally convert the deprecated
// HdMaterialNetwork over to the new description so we do not have to redo all
// the Storm code when we swap the classes in Hd to the new description.
//
// Equivilant of: HdPrman's MatfiltConvertFromHdMaterialNetworkMapTerminal.
// With some modifications since HdMaterialNetworkMap now has 'terminals'.
bool
_ConvertLegacyHdMaterialNetwork(
    HdMaterialNetworkMap const& hdNetworkMap,
    TfToken const & terminalName,
    HdSt_MaterialNetwork *result)
{
    auto iter = hdNetworkMap.map.find(terminalName);
    if (iter == hdNetworkMap.map.end()) {
        return false;
    }
    const HdMaterialNetwork & hdNetwork = iter->second;

    // Transfer over individual nodes
    for (const HdMaterialNode & node : hdNetwork.nodes) {
        HdSt_MaterialNode & newNode = result->nodes[node.path];
        newNode.nodeTypeId = node.identifier;
        newNode.parameters = node.parameters;

        // Check if this node is a terminal
        auto const& termIt = std::find(
            hdNetworkMap.terminals.begin(), 
            hdNetworkMap.terminals.end(), 
            node.path);

        if (termIt != hdNetworkMap.terminals.end()) {
            result->terminals[terminalName].upstreamNode = node.path;
        }
    }

    // Transfer relationships to inputConnections on receiving/downstream nodes.
    for (HdMaterialRelationship const& rel : hdNetwork.relationships) {
        // outputId (in hdMaterial terms) is the input of the receiving node
        auto iter = result->nodes.find(rel.outputId);
        // skip connection if the destination node doesn't exist
        if (iter == result->nodes.end()) {
            continue;
        }
        iter->second.inputConnections[rel.outputName]
            .emplace_back( HdSt_MaterialConnection{rel.inputId, rel.inputName});
    }

    // Transfer primvars:
    result->primvars = hdNetwork.primvars;

    return true;
}

static TfToken
_GetMaterialTag(
    VtDictionary const& metadata,
    HdSt_MaterialNode const& terminal)
{
    // Strongest materialTag opinion is a hardcoded tag in glslfx meta data.
    // This can be used for additive, translucent or volume materials.
    // See HdMaterialTagTokens.
    VtValue vtMetaTag = TfMapLookupByValue(
        metadata,
        HdShaderTokens->materialTag,
        VtValue());

    if (vtMetaTag.IsHolding<std::string>()) {
        return TfToken(vtMetaTag.UncheckedGet<std::string>());
    }

    bool isTranslucent = false;

    // Next strongest opinion is a connection to 'terminal.opacity'
    auto const& opacityConIt = terminal.inputConnections.find(_tokens->opacity);
    isTranslucent = (opacityConIt != terminal.inputConnections.end());

    // Weakest opinion is an authored terminal.opacity value.
    if (!isTranslucent) {
        for (auto const& paramIt : terminal.parameters) {
            if (paramIt.first != _tokens->opacity) continue;

            VtValue const& vtOpacity = paramIt.second;
            isTranslucent = vtOpacity.Get<float>() < 1.0f;
            break;
        }
    }

    if (isTranslucent) {
        // Default to our cheapest blending: unsorted additive
        return HdStMaterialTagTokens->additive;
    }

    // An empty materialTag on the HdRprimCollection level means: 'ignore all
    // materialTags and add everything to the collection'. Instead we return a
    // default token because we want materialTags to drive HdSt collections.
    return HdStMaterialTagTokens->defaultMaterialTag;
}

static void
_GetGlslfxForTerminal(
    HioGlslfxUniquePtr& glslfxOut,
    TfToken const& nodeTypeId)
{
    // 1. info::id was set in usda (token info:id = "UsdPreviewSurface").
    //
    // We have an info:id so we can use Sdr to get to the source code path
    // for glslfx. GetShaderNodeByIdentifierAndType() will insert a SdrNode
    // and we can use GetSourceURI to query the source code path.
    auto &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifierAndType(
        nodeTypeId, HioGlslfxTokens->glslfx);

    if (sdrNode) {
        std::string const& glslfxPath = sdrNode->GetSourceURI();
        glslfxOut.reset(new HioGlslfx(glslfxPath));
        return;
    }

    // 2. info::sourceAsset (asset info:glslfx:sourceAsset = @custm.glslfx@)
    // We did not have info::id so we expect the terminal type id token to
    // have been resolved into the path or source code for the glslfx.
    // E.g. UsdImagingMaterialAdapter handles this for us.
    if (TF_VERIFY(!nodeTypeId.IsEmpty())) {
        // Most likely: the identifier is a path to a glslfx file
        glslfxOut.reset(new HioGlslfx(nodeTypeId));
        if (!glslfxOut->IsValid()) {
            // Less likely: the identifier is a glslfx code snippet
            std::istringstream sourceCodeStream(nodeTypeId);
            glslfxOut.reset(new HioGlslfx(sourceCodeStream));
        }
    }
}

static HdSt_MaterialNode const*
_GetTerminalNode(
    SdfPath const& id,
    HdSt_MaterialNetwork const& network)
{
    if (network.terminals.size() != 1) {
        if (network.terminals.size() > 1) {
            TF_WARN("Unsupported number of terminals [%d] in material [%s]", 
                    (int)network.terminals.size(), id.GetText());
        }
        return nullptr;
    }

    auto const& terminalConnIt = network.terminals.begin();
    HdSt_MaterialConnection const& connection = terminalConnIt->second;
    SdfPath const& terminalPath = connection.upstreamNode;
    auto const& terminalIt = network.nodes.find(terminalPath);
    return &terminalIt->second;
}

static VtValue
_GetParamFallbackValue(
    SdrShaderNodeConstPtr const& sdrNode,
    HdSt_MaterialNode const& node,
    TfToken const& paramName)
{
    // Find the value of the input. This 'fallback value' will be the 
    // value of the material param if nothing is connected.

    auto const& it = node.parameters.find(paramName);
    if (it != node.parameters.end()) {
        return it->second;
    }

    // Sdr node will be null for custom glslfx shaders.
    if (sdrNode) {
        if (SdrShaderPropertyConstPtr const& sdrInput = 
                sdrNode->GetShaderInput(paramName)) {
            return sdrInput->GetDefaultValue();
        } else if (const auto &defaultInput = sdrNode->GetDefaultInput()) {
            VtValue defaultValue = defaultInput->GetDefaultValue();
            if (defaultValue.IsEmpty()) {
                return defaultInput->GetTypeAsSdfType().first.GetDefaultValue();
            } else {
                return defaultValue;
            }
        }
    }

    // Returning an empty value will likely result in a shader compile error,
    // because the buffer source will not be able to determine the HdTupleType.
    TF_VERIFY(false, "Couldn't determine default value for: %s on nodeType: %s", 
              paramName.GetText(), node.nodeTypeId.GetText());
    return VtValue();
}

static HdMaterialParam
_MakeMaterialParamForUnconnectedParam(
    SdrShaderNodeConstPtr const& sdrNode,
    HdSt_MaterialNode const& node,
    TfToken const& paramName)
{
    HdMaterialParam matParam;
    matParam.paramType = HdMaterialParam::ParamTypeFallback;
    matParam.name = paramName;
    matParam.fallbackValue = _GetParamFallbackValue(sdrNode, node, paramName);
    matParam.connection = SdfPath();          /*No connection*/
    matParam.samplerCoords = TfTokenVector(); /*No UV*/
    matParam.textureType = HdTextureType::Uv  /*No Texture*/;
    return matParam;
}

static TfToken
_GetPrimvarNameAttributeValue(
    SdrShaderNodeConstPtr const& sdrNode,
    HdSt_MaterialNode const& node,
    TfToken const& propName)
{
    VtValue vtName;

    // If the name of the primvar was authored, the material adapter would have
    // put that that authored value in the node's parameter list. 
    // The authored value is the strongest opinion/

    auto const& paramIt = node.parameters.find(propName);
    if (paramIt != node.parameters.end()) {
        vtName = paramIt->second;
    }

    // If we didn't find an authored value consult Sdr for the default value.
    if (vtName.IsEmpty() && sdrNode) {
        if (SdrShaderPropertyConstPtr sdrPrimvarInput = 
                sdrNode->GetShaderInput(propName)) {
            vtName = sdrPrimvarInput->GetDefaultValue();
        }
    }

    if (vtName.IsHolding<TfToken>()) {
        return vtName.UncheckedGet<TfToken>();
    } else if (vtName.IsHolding<std::string>()) {
        return TfToken(vtName.UncheckedGet<std::string>());
    }

    return TfToken();
}

static HdMaterialParam
_MakeMaterialParamForPrimvarInput(
    SdrShaderNodeConstPtr const& sdrNode,
    HdSt_MaterialNode const& node,
    SdfPath const& nodePath,
    TfToken const& paramName)
{
    HdMaterialParam matParam;
    matParam.paramType = HdMaterialParam::ParamTypePrimvar;
    matParam.name = paramName;
    matParam.fallbackValue = _GetParamFallbackValue(sdrNode, node, paramName);
    matParam.connection = SdfPath("primvar." + nodePath.GetName());
    matParam.textureType = HdTextureType::Uv  /*No Texture*/;

    // A node may require 'additional primvars' to function correctly.

    TfTokenVector varNames;
    for (auto const& propName: sdrNode->GetAdditionalPrimvarProperties()) {
        TfToken primvarName = 
            _GetPrimvarNameAttributeValue(sdrNode, node, propName);

        if (!primvarName.IsEmpty()) {
            matParam.samplerCoords.push_back(primvarName);
        }
    }

    return matParam;
}

//static HdMaterialParam
//_MakeMaterialParamForTextureInput()
//{
//// todo see if it has a primvar connected for texCoords
//}

static HdMaterialParam
_MakeParamForInputParameter(
    SdrRegistry & shaderReg,
    HdSt_MaterialNetwork const& network,
    SdrShaderNodeConstPtr const& sdrNode,
    HdSt_MaterialNode const& node,
    TfToken const& paramName)
{
    // Resolve what is connected to this param (eg. primvar, texture, nothing)
    // and then make the correct HdMaterialParam for it.
    auto const& conIt = node.inputConnections.find(paramName);

    if (conIt != node.inputConnections.end()) {

        std::vector<HdSt_MaterialConnection> const& cons = conIt->second;
        if (!cons.empty()) {

            // Find the node that is connected to this input
            HdSt_MaterialConnection const& con = cons.front();
            auto upIt = network.nodes.find(con.upstreamNode);

            if (upIt != network.nodes.end()) {

                SdfPath const& upstreamPath = upIt->first;
                HdSt_MaterialNode const& upstreamNode = upIt->second;

                SdrShaderNodeConstPtr upstreamSdr = 
                        shaderReg.GetShaderNodeByIdentifierAndType(
                            upstreamNode.nodeTypeId,
                            HioGlslfxTokens->glslfx);

                if (upstreamSdr) {
                    TfToken sdrFamily(upstreamSdr->GetFamily());
                    TfToken sdrRole(upstreamSdr->GetRole());
                    if (sdrRole == SdrNodeRole->Texture) {
                        // todo
                    } else if (sdrRole == SdrNodeRole->Primvar) {
                        return _MakeMaterialParamForPrimvarInput(
                            upstreamSdr, upstreamNode, upstreamPath, paramName);
                    } else if (sdrRole == SdrNodeRole->Field) {
                        // todo
                    }
                }
            }
        }
    } 

    // Nothing (supported) was connected, output a fallback material param    
    return _MakeMaterialParamForUnconnectedParam(sdrNode, node, paramName);
}

static HdMaterialParamVector 
_GatherMaterialParams(
    HdSt_MaterialNetwork const& network,
    HdSt_MaterialNode const& node,
    HioGlslfxUniquePtr const& glslfx) 
{
    HdMaterialParamVector params;

    auto &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifierAndType(
        node.nodeTypeId, HioGlslfxTokens->glslfx);

    // For custom glslfx, that have no schema, we pull the input parameter list 
    // from the glslfx instead of Sdr, because we dont have a glslfx Sdr parser.
    // The Sdr node will be null in those cases.

    if (sdrNode) {
        for (TfToken const& inputName : sdrNode->GetInputNames()) {
            HdMaterialParam matParam = _MakeParamForInputParameter(
                shaderReg, network, sdrNode, node, inputName);
            params.emplace_back(std::move(matParam));
        }
    } else if (glslfx) {
        for (HioGlslfxConfig::Parameter const& input: glslfx->GetParameters()) {
            HdMaterialParam matParam = _MakeParamForInputParameter(
                shaderReg, network, sdrNode, node, TfToken(input.name));

            if (matParam.fallbackValue.IsEmpty()) {
                matParam.fallbackValue = input.defaultValue;
            }

            params.emplace_back(std::move(matParam));
        }
    } else {
        TF_WARN("Unknown material configuration");
    }

    return params;
}

HdStMaterialNetwork::HdStMaterialNetwork()
{
}

HdStMaterialNetwork::~HdStMaterialNetwork()
{

}

void
HdStMaterialNetwork::ProcessMaterialNetwork(
    SdfPath const& materialId,
    HdMaterialNetworkMap const& hdNetworkMap)
{
    HdSt_MaterialNetwork surfaceNetwork;
    HdSt_MaterialNetwork displacementNetwork;

    // The fragment source comes from the 'surface' network or the
    // 'volume' network.
    _ConvertLegacyHdMaterialNetwork(
        hdNetworkMap,
        HdMaterialTerminalTokens->surface,
        &surfaceNetwork);

    bool isVolume = surfaceNetwork.terminals.empty();
    if (!isVolume) {
        _ConvertLegacyHdMaterialNetwork(
            hdNetworkMap,
            HdMaterialTerminalTokens->volume,
            &surfaceNetwork);
    }

    // Geometry source can be provided via a 'displacement' network.
    _ConvertLegacyHdMaterialNetwork(
        hdNetworkMap,
        HdMaterialTerminalTokens->displacement,
        &displacementNetwork);

    if (HdSt_MaterialNode const* surfTerminal = 
            _GetTerminalNode(materialId, surfaceNetwork)) 
    {
        // Extract the glslfx and metadata for surface/volume.
        HioGlslfxUniquePtr surfaceGfx;
        _GetGlslfxForTerminal(surfaceGfx, surfTerminal->nodeTypeId);
        if (surfaceGfx && surfaceGfx->IsValid()) {
            _fragmentSource = isVolume ? surfaceGfx->GetVolumeSource() : 
                surfaceGfx->GetSurfaceSource();
            _materialMetadata = surfaceGfx->GetMetadata();
            _materialTag = _GetMaterialTag(_materialMetadata, *surfTerminal);
            _materialParams = _GatherMaterialParams(
                surfaceNetwork, *surfTerminal, surfaceGfx);
        }
    }

    if (HdSt_MaterialNode const* dispTerminal = 
            _GetTerminalNode(materialId, displacementNetwork)) 
    {
        // Extract the glslfx for displacement.
        HioGlslfxUniquePtr displacementGfx;
        _GetGlslfxForTerminal(displacementGfx, dispTerminal->nodeTypeId);
        if (displacementGfx && displacementGfx->IsValid()) {
            _geometrySource = displacementGfx->GetDisplacementSource();
        }
    }
}

TfToken const&
HdStMaterialNetwork::GetMaterialTag() const
{
    return _materialTag;
}

std::string const& 
HdStMaterialNetwork::GetFragmentCode() const
{
    return _fragmentSource;
}

std::string const&
HdStMaterialNetwork::GetGeometryCode() const
{
    return _geometrySource;
}

VtDictionary const&
HdStMaterialNetwork::GetMetadata() const
{
    return _materialMetadata;
}

HdMaterialParamVector const&
HdStMaterialNetwork::GetMaterialParams() const
{
    return _materialParams;
}

PXR_NAMESPACE_CLOSE_SCOPE

