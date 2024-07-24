//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (wrapS)
    (wrapT)
    (wrapR)

    (repeat)
    (mirror)
    (clamp)
    (black)
    (useMetadata)

    (HwUvTexture_1)

    (minFilter)
    (magFilter)

    (nearest)
    (linear)
    (nearestMipmapNearest)
    (nearestMipmapLinear)
    (linearMipmapNearest)
    (linearMipmapLinear)
);

HdMaterial::HdMaterial(SdfPath const& id)
 : HdSprim(id)
{
    // NOTHING
}

HdMaterial::~HdMaterial() = default;

HdMaterialNetwork2
HdConvertToHdMaterialNetwork2(
    const HdMaterialNetworkMap & hdNetworkMap,
    bool *isVolume)
{
    HD_TRACE_FUNCTION();
    HdMaterialNetwork2 result;

    for (auto const& iter: hdNetworkMap.map) {
        const TfToken & terminalName = iter.first;
        const HdMaterialNetwork & hdNetwork = iter.second;

        // Check if there are nodes associated with the volume terminal
        // This value is used in Storm to get the proper glslfx fragment shader
        if (terminalName == HdMaterialTerminalTokens->volume && isVolume) {
            *isVolume = !hdNetwork.nodes.empty();
        }

        // Transfer over individual nodes.
        // Note that the same nodes may be shared by multiple terminals.
        // We simply overwrite them here.
        if (hdNetwork.nodes.empty()) {
            continue;
        }
        for (const HdMaterialNode & node : hdNetwork.nodes) {
            HdMaterialNode2 & materialNode2 = result.nodes[node.path];
            materialNode2.nodeTypeId = node.identifier;
            materialNode2.parameters = node.parameters;
        }
        // Assume that the last entry is the terminal 
        result.terminals[terminalName].upstreamNode = 
                hdNetwork.nodes.back().path;

        // Transfer relationships to inputConnections on receiving/downstream nodes.
        for (const HdMaterialRelationship & rel : hdNetwork.relationships) {
            
            // outputId (in hdMaterial terms) is the input of the receiving node
            auto iter = result.nodes.find(rel.outputId);

            // skip connection if the destination node doesn't exist
            if (iter == result.nodes.end()) {
                continue;
            }

            std::vector<HdMaterialConnection2> &conns =
                iter->second.inputConnections[rel.outputName];
            HdMaterialConnection2 conn {rel.inputId, rel.inputName};
            
            // skip connection if it already exists (it may be shared
            // between surface and displacement)
            if (std::find(conns.begin(), conns.end(), conn) == conns.end()) {
                conns.push_back(std::move(conn));
            }
        }

        // Transfer primvars:
        result.primvars = hdNetwork.primvars;
    }
    return result;
}

// Look up value from the parameters map and fallback to corresponding value on
// given SdrNode.
template<typename T>
static
auto
_ResolveParameter(
    const std::map<TfToken, VtValue>& parameters,
    const SdrShaderNodeConstPtr& sdrNode,
    const TfToken& name,
    const T& defaultValue) -> T
{
    // First consult parameters...
    const auto it = parameters.find(name);
    if (it != parameters.end()) {
        const VtValue& value = it->second;
        if (value.IsHolding<T>()) {
            return value.UncheckedGet<T>();
        }
    }

    // Then fallback to SdrNode.
    if (sdrNode) {
        if (const SdrShaderPropertyConstPtr input =
                                        sdrNode->GetShaderInput(name)) {
            const VtValue& value = input->GetDefaultValueAsSdfType();
            if (value.IsHolding<T>()) {
                return value.UncheckedGet<T>();
            }
        }
    }

    return defaultValue;
}

static
HdWrap
_ResolveWrapSamplerParameter(
    const TfToken& nodeTypeId,
    const std::map<TfToken, VtValue>& parameters,
    const SdrShaderNodeConstPtr& sdrNode,
    const SdfPath& nodePath,
    const TfToken& name)
{
    const TfToken value = _ResolveParameter(
        parameters, sdrNode, name, _tokens->useMetadata);

    if (value == _tokens->repeat) {
        return HdWrapRepeat;
    }

    if (value == _tokens->mirror) {
        return HdWrapMirror;
    }

    if (value == _tokens->clamp) {
        return HdWrapClamp;
    }

    if (value == _tokens->black) {
        return HdWrapBlack;
    }

    if (value == _tokens->useMetadata) {
        if (nodeTypeId == _tokens->HwUvTexture_1) {
            return HdWrapLegacy;
        }
        return HdWrapUseMetadata;
    }

    if (!nodePath.IsEmpty()) {
        TF_WARN("Unknown wrap mode on prim %s: %s",
                nodePath.GetText(), value.GetText());
    } else {
        TF_WARN("Unknown wrap mode: %s", value.GetText());
    }

    return HdWrapUseMetadata;
}

static
HdMinFilter
_ResolveMinSamplerParameter(
    const TfToken& nodeTypeId,
    const std::map<TfToken, VtValue>& parameters,
    const SdrShaderNodeConstPtr& sdrNode,
    const SdfPath& nodePath)
{
    // Using linearMipmapLinear as fallback value.

    // Note that it is ambiguous whether the fallback value in the old
    // texture system (usdImagingGL/textureUtils.cpp) was linear or
    // linearMipmapLinear: when nothing was authored in USD for the
    // min filter, linearMipmapLinear was used, but when an empty
    // token was authored, linear was used.

    const TfToken value = _ResolveParameter(
        parameters, sdrNode, _tokens->minFilter,
        _tokens->linearMipmapLinear);

    if (value == _tokens->nearest) {
        return HdMinFilterNearest;
    }

    if (value == _tokens->linear) {
        return HdMinFilterLinear;
    }

    if (value == _tokens->nearestMipmapNearest) {
        return HdMinFilterNearestMipmapNearest;
    }

    if (value == _tokens->nearestMipmapLinear) {
        return HdMinFilterNearestMipmapLinear;
    }

    if (value == _tokens->linearMipmapNearest) {
        return HdMinFilterLinearMipmapNearest;
    }

    if (value == _tokens->linearMipmapLinear) {
        return HdMinFilterLinearMipmapLinear;
    }

    return HdMinFilterLinearMipmapLinear;
}

static
HdMagFilter
_ResolveMagSamplerParameter(
    const TfToken& nodeTypeId,
    const std::map<TfToken, VtValue>& parameters,
    const SdrShaderNodeConstPtr& sdrNode,
    const SdfPath& nodePath)
{
    const TfToken value = _ResolveParameter(
        parameters, sdrNode, _tokens->magFilter, _tokens->linear);

    if (value == _tokens->nearest) {
        return HdMagFilterNearest;
    }

    return HdMagFilterLinear;
}

static
HdSamplerParameters
_GetSamplerParameters(
    const TfToken& nodeTypeId,
    const std::map<TfToken, VtValue>& parameters,
    const SdrShaderNodeConstPtr& sdrNode,
    const SdfPath& nodePath)
{
    return { _ResolveWrapSamplerParameter(
                 nodeTypeId, parameters, sdrNode, nodePath, _tokens->wrapS),
             _ResolveWrapSamplerParameter(
                 nodeTypeId, parameters, sdrNode, nodePath, _tokens->wrapT),
             _ResolveWrapSamplerParameter(
                 nodeTypeId, parameters, sdrNode, nodePath, _tokens->wrapR),
             _ResolveMinSamplerParameter(
                 nodeTypeId, parameters, sdrNode, nodePath),
             _ResolveMagSamplerParameter(
                 nodeTypeId, parameters, sdrNode, nodePath),
             HdBorderColorTransparentBlack, 
             /*enableCompare*/false, 
             HdCmpFuncNever };
}

HdSamplerParameters
HdGetSamplerParameters(
    const HdMaterialNode2& node,
    const SdrShaderNodeConstPtr& sdrNode,
    const SdfPath& nodePath)
{
    return _GetSamplerParameters(
        node.nodeTypeId,
        node.parameters,
        sdrNode,
        nodePath);
}

HdSamplerParameters
HdGetSamplerParameters(
    const TfToken& nodeTypeId,
    const std::map<TfToken, VtValue>& parameters,
    const SdfPath& nodePath)
{
    return _GetSamplerParameters(
        nodeTypeId,
        parameters,
        nullptr,
        nodePath);
}


// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdMaterialNetwork& pv)
{
    out << "HdMaterialNetwork Params: (...) " ;
    return out;
}

bool operator==(const HdMaterialRelationship& lhs, 
                const HdMaterialRelationship& rhs)
{
    return lhs.outputId   == rhs.outputId && 
           lhs.outputName == rhs.outputName &&
           lhs.inputId    == rhs.inputId &&
           lhs.inputName  == rhs.inputName;
}


bool operator==(const HdMaterialNode& lhs, const HdMaterialNode& rhs)
{
    return lhs.path == rhs.path &&
           lhs.identifier == rhs.identifier &&
           lhs.parameters == rhs.parameters;
}


bool operator==(const HdMaterialNetwork& lhs, const HdMaterialNetwork& rhs) 
{
    return lhs.relationships           == rhs.relationships && 
           lhs.nodes                   == rhs.nodes &&
           lhs.primvars                == rhs.primvars;
}

bool operator!=(const HdMaterialNetwork& lhs, const HdMaterialNetwork& rhs) 
{
    return !(lhs == rhs);
}


std::ostream& operator<<(std::ostream& out, const HdMaterialNetworkMap& pv)
{
    out << "HdMaterialNetworkMap Params: (...) " ;
    return out;
}

bool operator==(const HdMaterialNetworkMap& lhs,
                const HdMaterialNetworkMap& rhs) 
{
    return lhs.map == rhs.map &&
           lhs.terminals == rhs.terminals;
}

bool operator!=(const HdMaterialNetworkMap& lhs,
                const HdMaterialNetworkMap& rhs) 
{
    return !(lhs == rhs);
}


std::ostream& operator<<(std::ostream& out, const HdMaterialNode2& pv)
{
    out << "HdMaterialNode2 Params: (...) " ;
    return out;
}

bool operator==(const HdMaterialNode2& lhs,
                const HdMaterialNode2& rhs) 
{
    return lhs.nodeTypeId == rhs.nodeTypeId
        && lhs.parameters == rhs.parameters
        && lhs.inputConnections == rhs.inputConnections;
}

bool operator!=(const HdMaterialNode2& lhs,
                const HdMaterialNode2& rhs) 
{
    return !(lhs == rhs);
}


PXR_NAMESPACE_CLOSE_SCOPE
