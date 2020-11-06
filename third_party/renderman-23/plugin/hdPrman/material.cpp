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
#include "hdPrman/matfiltConvertPreviewMaterial.h"
#include "hdPrman/matfiltFilterChain.h"
#include "hdPrman/matfiltResolveVstructs.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"
#include "RtParamList.h"

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (PxrDisplace)
    (bxdf)
    (OSL)
);

TF_MAKE_STATIC_DATA(NdrTokenVec, _sourceTypes) {
    *_sourceTypes = { TfToken("OSL"), TfToken("RmanCpp") };
}

TfTokenVector const&
HdPrmanMaterial::GetShaderSourceTypes()
{
    return *_sourceTypes;
}

TF_MAKE_STATIC_DATA(MatfiltFilterChain, _filterChain) {
    *_filterChain = {
        MatfiltConvertPreviewMaterial,
        MatfiltResolveVstructs
    };
}

MatfiltFilterChain
HdPrmanMaterial::GetFilterChain()
{
    return *_filterChain;
}

void
HdPrmanMaterial::SetFilterChain(MatfiltFilterChain const& chain)
{
    *_filterChain = chain;
}

HdPrmanMaterial::HdPrmanMaterial(SdfPath const& id)
    : HdMaterial(id)
    , _materialId(riley::MaterialId::k_InvalidId)
    , _displacementId(riley::DisplacementId::k_InvalidId)
{
    /* NOTHING */
}

HdPrmanMaterial::~HdPrmanMaterial()
{
}

void
HdPrmanMaterial::Finalize(HdRenderParam *renderParam)
{
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();
    _ResetMaterial(context);
}

void
HdPrmanMaterial::_ResetMaterial(HdPrman_Context *context)
{
    riley::Riley *riley = context->riley;
    if (_materialId != riley::MaterialId::k_InvalidId) {
        riley->DeleteMaterial(_materialId);
        _materialId = riley::MaterialId::k_InvalidId;
    }
    if (_displacementId != riley::DisplacementId::k_InvalidId) {
        riley->DeleteDisplacement(_displacementId);
        _displacementId = riley::DisplacementId::k_InvalidId;
    }
}

static VtArray<GfVec3f>
_ConvertToVec3fArray(const VtArray<GfVec3d>& v)
{
    VtArray<GfVec3f> out;
    out.resize(v.size());
    for (size_t i=0; i<v.size(); ++i) {
        for (uint8_t e=0; e<3; ++e) {
            out[i][e] = v[i][e];
        }
    }
    return out;
}

using _PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;

// Recursively convert a HdMaterialNode2 and its upstream dependencies
// to Riley equivalents.  Avoids adding redundant nodes in the case
// of multi-path dependencies.
static bool
_ConvertNodes(
    HdMaterialNetwork2 const& network,
    SdfPath const& nodePath,
    std::vector<riley::ShadingNode> *result,
    _PathSet* visitedNodes)
{
    // Check if we've processed this node before. If we have, we'll just return.
    // This is not an error, since we often have multiple connection paths
    // leading to the same upstream node.
    if (visitedNodes->count(nodePath) > 0) {
        return false;
    }
    visitedNodes->insert(nodePath);

    // Find HdMaterialNetwork2 node.
    auto iter = network.nodes.find(nodePath);
    if (iter == network.nodes.end()) {
        // This could be caused by a bad connection to a non-existent node.
        TF_WARN("Unknown material node '%s'", nodePath.GetText());
        return false;
    }
    HdMaterialNode2 const& node = iter->second;
    // Riley expects nodes to be provided in topological dependency order.
    // Pre-traverse upstream nodes.
    for (auto const& connEntry: node.inputConnections) {
        for (auto const& e: connEntry.second) {
            // This method will just return if we've visited this upstream node
            // before
            _ConvertNodes(network, e.upstreamNode, result, visitedNodes);
        }
    }
    // Find shader registry entry.
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrEntry =
        sdrRegistry.GetShaderNodeByIdentifier(node.nodeTypeId, *_sourceTypes);
    if (!sdrEntry) {
        TF_WARN("Unknown shader ID %s for node <%s>\n",
                node.nodeTypeId.GetText(), nodePath.GetText());
        return false;
    }
    // Create equivalent Riley shading node.
    riley::ShadingNode sn;
    if (sdrEntry->GetContext() == _tokens->bxdf ||
        sdrEntry->GetContext() == SdrNodeContext->Surface ||
        sdrEntry->GetContext() == SdrNodeContext->Volume) {
        sn.type = riley::ShadingNode::k_Bxdf;
    } else if (sdrEntry->GetContext() == SdrNodeContext->Pattern ||
               sdrEntry->GetContext() == _tokens->OSL) {
        sn.type = riley::ShadingNode::k_Pattern;
    } else if (sdrEntry->GetContext() == SdrNodeContext->Displacement) {
        sn.type = riley::ShadingNode::k_Displacement;
    } else {
        // This can happen if the material accidentally references
        // a non-shading node type such as a light or light-filter.
        TF_WARN("Unknown shader entry type '%s' for shader '%s'",
                sdrEntry->GetContext().GetText(), sdrEntry->GetName().c_str());
        return false;
    }
    sn.handle = RtUString(nodePath.GetText());
    std::string shaderPath = sdrEntry->GetResolvedImplementationURI();
    if (shaderPath.empty()){
        // This can happen if the material accidentally references
        // a non-shading node type such as a light or light-filter.
        TF_WARN("Shader '%s' did not provide a valid implementation path.",
                sdrEntry->GetName().c_str());
        return false;
    }
        
    sn.name = RtUString(shaderPath.c_str());
    // Convert params
    for (const auto& param: node.parameters) {
        const SdrShaderProperty* prop = sdrEntry->GetShaderInput(param.first);
        if (!prop) {
            TF_DEBUG(HDPRMAN_MATERIALS)
                .Msg("Unknown shader property '%s' for "
                     "shader '%s' at '%s'; ignoring.\n",
                     param.first.GetText(),
                     sdrEntry->GetName().c_str(),
                     nodePath.GetText());
            continue;
        }
        TfToken propType = prop->GetType();
        if (propType.IsEmpty()) {
            // As a special case, silently ignore these on PxrDisplace.
            // Automatically promoting the same network for this
            // case causes a lot of errors.
            if (node.nodeTypeId == _tokens->PxrDisplace) {
                continue;
            }
            TF_DEBUG(HDPRMAN_MATERIALS)
                .Msg("Unknown shader entry field type for "
                     "field '%s' on shader '%s' at '%s'; ignoring.\n",
                     param.first.GetText(),
                     sdrEntry->GetName().c_str(),
                     nodePath.GetText());
            continue;
        }

        // Dispatch by propType and VtValue-held type.
        // Cast value types to match where feasible.
        bool ok = false;
        RtUString name(prop->GetImplementationName().c_str());
        if (propType == SdrPropertyTypes->Struct ||
            propType == SdrPropertyTypes->Vstruct) {
            // Ignore structs.  They are only used as ways to
            // pass data between shaders, not as a way to pass
            // in parameters.
            ok = true;
        } else if (param.second.IsHolding<GfVec2f>()) {
            GfVec2f v = param.second.UncheckedGet<GfVec2f>();
            if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloatArray(name, v.data(), 2);
                ok = true;
            } 
        } else if (param.second.IsHolding<GfVec3f>()) {
            GfVec3f v = param.second.UncheckedGet<GfVec3f>();
            if (propType == SdrPropertyTypes->Color) {
                sn.params.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
                ok = true;
            } else if (propType == SdrPropertyTypes->Vector) {
                sn.params.SetVector(name, RtVector3(v[0], v[1], v[2]));
                ok = true;
            } else if (propType == SdrPropertyTypes->Point) {
                sn.params.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
                ok = true;
            } else if (propType == SdrPropertyTypes->Normal) {
                sn.params.SetNormal(name, RtNormal3(v[0], v[1], v[2]));
                ok = true;
            }
        } else if (param.second.IsHolding<GfVec4f>()) {
            GfVec4f v = param.second.UncheckedGet<GfVec4f>();
            if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloatArray(name, v.data(), 4);
                ok = true;
            } 
        } else if (param.second.IsHolding<VtArray<GfVec3f>>()) {
            const VtArray<GfVec3f>& v =
                param.second.UncheckedGet<VtArray<GfVec3f>>();
            if (propType == SdrPropertyTypes->Color) {
                sn.params.SetColorArray(
                                      name,
                                      reinterpret_cast<const RtColorRGB*>(v.cdata()),
                                      v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Vector) {
                sn.params.SetVectorArray(
                                       name,
                                       reinterpret_cast<const RtVector3*>(v.cdata()),
                                       v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Point) {
                sn.params.SetPointArray(
                                      name,
                                      reinterpret_cast<const RtPoint3*>(v.cdata()),
                                      v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Normal) {
                sn.params.SetNormalArray(
                                       name,
                                       reinterpret_cast<const RtNormal3*>(v.cdata()),
                                       v.size());
                ok = true;
            }
        } else if (param.second.IsHolding<GfVec3d>()) {
            const GfVec3d& v = param.second.UncheckedGet<GfVec3d>();
            if (propType == SdrPropertyTypes->Color) {
                sn.params.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
                ok = true;
            }
        } else if (param.second.IsHolding<VtArray<GfVec3d>>()) {
            if (propType == SdrPropertyTypes->Color) {
                const VtArray<GfVec3d>& vd =
                    param.second.UncheckedGet<VtArray<GfVec3d>>();
                VtArray<GfVec3f> v = _ConvertToVec3fArray(vd);
                sn.params.SetColorArray(
                                      name,
                                      reinterpret_cast<const RtColorRGB*>(v.cdata()),
                                      v.size());
                ok = true;
            }
        } else if (param.second.IsHolding<float>()) {
            float v = param.second.UncheckedGet<float>();
            if (propType == SdrPropertyTypes->Int) {
                sn.params.SetInteger(name, int(v));
                ok = true;
            } else if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloat(name, v);
                ok = true;
            }
        } else if (param.second.IsHolding<VtArray<float>>()) {
            const VtArray<float>& v =
                param.second.UncheckedGet<VtArray<float>>();
            if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloatArray(name, v.cdata(), v.size());
                ok = true;
            }
        } else if (param.second.IsHolding<int>()) {
            int v = param.second.UncheckedGet<int>();
            if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloat(name, v);
                ok = true;
            } else if (propType == SdrPropertyTypes->Int) {
                sn.params.SetInteger(name, v);
                ok = true;
            }
        } else if (param.second.IsHolding<VtArray<int>>()) {
            if (propType == SdrPropertyTypes->Float) {
                const VtArray<float>& v =
                    param.second.UncheckedGet<VtArray<float>>();
                sn.params.SetFloatArray(name, v.cdata(), v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Int) {
                const VtArray<int>& v =
                    param.second.UncheckedGet<VtArray<int>>();
                sn.params.SetIntegerArray(name, v.cdata(), v.size());
                ok = true;
            }
        } else if (param.second.IsHolding<TfToken>()) {
            TfToken v = param.second.UncheckedGet<TfToken>();
            sn.params.SetString(name, RtUString(v.GetText()));
            ok = true;
        } else if (param.second.IsHolding<std::string>()) {
            std::string v = param.second.UncheckedGet<std::string>();
            sn.params.SetString(name, RtUString(v.c_str()));
            ok = true;
        } else if (param.second.IsHolding<SdfAssetPath>()) {
            SdfAssetPath p = param.second.Get<SdfAssetPath>();
            std::string v = p.GetResolvedPath();
            if (v.empty()) {
                v = p.GetAssetPath();
            }
            sn.params.SetString(name, RtUString(v.c_str()));
            ok = true;
        } else if (param.second.IsHolding<bool>()) {
            // RixParamList (specifically, RixDataType) doesn't have
            // a bool entry; we convert to integer instead.
            int v = param.second.UncheckedGet<bool>();
            sn.params.SetInteger(name, v);
            ok = true;
        }
        if (!ok) {
            TF_DEBUG(HDPRMAN_MATERIALS)
                .Msg("Unknown shading parameter type '%s'; skipping "
                     "parameter '%s' on node '%s'; "
                     "expected type '%s'\n",
                     param.second.GetTypeName().c_str(),
                     param.first.GetText(),
                     nodePath.GetText(),
                     propType.GetText());
        }
    }
    // Convert connected inputs.
    for (auto const& connEntry: node.inputConnections) {
        for (auto const& e: connEntry.second) {
            // Find the output & input shader nodes of the connection.
            HdMaterialNode2 const* upstreamNode =
                TfMapLookupPtr(network.nodes, e.upstreamNode);
            if (!upstreamNode) {
                TF_WARN("Unknown upstream node %s", e.upstreamNode.GetText());
                continue;
            }
            SdrShaderNodeConstPtr upstreamSdrEntry =
                sdrRegistry.GetShaderNodeByIdentifier(
                      upstreamNode->nodeTypeId, *_sourceTypes);
            if (!upstreamSdrEntry) {
                TF_WARN("Unknown shader for upstream node %s",
                        e.upstreamNode.GetText());
                continue;
            }
            // Find the shader properties, so that we can look up
            // the property implementation names.
            SdrShaderPropertyConstPtr downstreamProp =
                sdrEntry->GetShaderInput(connEntry.first);
            SdrShaderPropertyConstPtr upstreamProp =
                upstreamSdrEntry->GetShaderOutput(e.upstreamOutputName);
            if (!downstreamProp) {
                TF_WARN("Unknown downstream property %s",
                        connEntry.first.data());
                continue;
            }
            if (!upstreamProp) {
                TF_WARN("Unknown upstream property %s",
                        e.upstreamOutputName.data());
                continue;
            }
            // Prman syntax for parameter references is "handle:param".
            RtUString name(downstreamProp->GetImplementationName().c_str());
            RtUString inputRef(
                               (e.upstreamNode.GetString()+":"
                                + upstreamProp->GetImplementationName().c_str())
                               .c_str());

            // Establish the Riley connection.
            TfToken propType = downstreamProp->GetType();
            if (propType == SdrPropertyTypes->Color) {
                sn.params.ReferenceColor(name, inputRef);
            } else if (propType == SdrPropertyTypes->Vector) {
                sn.params.ReferenceVector(name, inputRef);
            } else if (propType == SdrPropertyTypes->Point) {
                sn.params.ReferencePoint(name, inputRef);
            } else if (propType == SdrPropertyTypes->Normal) {
                sn.params.ReferenceNormal(name, inputRef);
            } else if (propType == SdrPropertyTypes->Float) {
                sn.params.ReferenceFloat(name, inputRef);
            } else if (propType == SdrPropertyTypes->Int) {
                sn.params.ReferenceInteger(name, inputRef);
            } else if (propType == SdrPropertyTypes->String) {
                sn.params.ReferenceString(name, inputRef);
            } else if (propType == SdrPropertyTypes->Struct) {
                sn.params.ReferenceStruct(name, inputRef);
            } else {
                TF_WARN("Unknown type '%s' for property '%s' "
                        "on shader '%s' at %s; ignoring.",
                        propType.GetText(),
                        connEntry.first.data(),
                        sdrEntry->GetName().c_str(),
                        nodePath.GetText());
            }
        }
    }

    result->emplace_back(std::move(sn));

    return true;
}
    
// Debug helper
void
HdPrman_DumpNetwork(HdMaterialNetwork2 const& network, SdfPath const& id)
{
    printf("material network for %s:\n", id.GetText());
    for (auto const& nodeEntry: network.nodes) {
        printf("  --Node--\n");
        printf("    path: %s\n", nodeEntry.first.GetText());
        printf("    type: %s\n", nodeEntry.second.nodeTypeId.GetText());
        for (auto const& paramEntry: nodeEntry.second.parameters) {
            printf("    param: %s = %s\n",
                   paramEntry.first.GetText(),
                   TfStringify(paramEntry.second).c_str());
        }
        for (auto const& connEntry: nodeEntry.second.inputConnections) {
            for (auto const& e: connEntry.second) {
                printf("    connection: %s <-> %s @ %s\n",
                       connEntry.first.GetText(),
                       e.upstreamOutputName.GetText(),
                       e.upstreamNode.GetText());
        }
    }
}
    printf("  --Terminals--\n");
    for (auto const& terminalEntry: network.terminals) {
        printf("    %s (downstream) <-> %s @ %s (upstream)\n",
               terminalEntry.first.GetText(),
               terminalEntry.second.upstreamOutputName.GetText(),
               terminalEntry.second.upstreamNode.GetText());
    }
}
    
// Convert given HdMaterialNetwork2 to Riley material and displacement
// shader networks. If the Riley network exists, it will be modified;
// otherwise it will be created as needed.
static void
_ConvertHdMaterialNetwork2ToRman(
    HdPrman_Context *context,
    SdfPath const& id,
    const HdMaterialNetwork2 &network,
    riley::MaterialId *materialId,
    riley::DisplacementId *displacementId)
{
    HD_TRACE_FUNCTION();
    riley::Riley *riley = context->riley;
    std::vector<riley::ShadingNode> nodes;
    nodes.reserve(network.nodes.size());
    bool materialFound = false, displacementFound = false;
    for (auto const& terminal: network.terminals) {
        _PathSet visitedNodes;
        if (_ConvertNodes(network, terminal.second.upstreamNode, &nodes,
                          &visitedNodes)) {
            if (terminal.first == HdMaterialTerminalTokens->surface ||
                terminal.first == HdMaterialTerminalTokens->volume) {
                // Create or modify Riley material.
                materialFound = true;
                if (*materialId == riley::MaterialId::k_InvalidId) {
                    *materialId = riley->CreateMaterial(&nodes[0],
                                                        nodes.size());
                } else {
                    riley->ModifyMaterial(*materialId, &nodes[0], nodes.size());
                }
                if (*materialId == riley::MaterialId::k_InvalidId) {
                    TF_RUNTIME_ERROR("Failed to create material %s\n",
                                     id.GetText());
                }
            } else if (terminal.first ==
                       HdMaterialTerminalTokens->displacement) {
                // Create or modify Riley displacement.
                displacementFound = true;
                if (*displacementId == riley::DisplacementId::k_InvalidId) {
                    *displacementId = riley->CreateDisplacement(&nodes[0],
                                                                nodes.size());
                } else {
                    riley->ModifyDisplacement(*displacementId, &nodes[0],
                                              nodes.size());
                }
                if (*displacementId == riley::DisplacementId::k_InvalidId) {
                    TF_RUNTIME_ERROR("Failed to create displacement %s\n",
                                     id.GetText());
                }
            }
        } else {
            TF_RUNTIME_ERROR("Failed to convert nodes for %s\n", id.GetText());
        }
        nodes.clear();
    }
    // Free dis-used networks.
    if (!materialFound) {
        riley->DeleteMaterial(*materialId);
        *materialId = riley::MaterialId::k_InvalidId;
    }
    if (!displacementFound) {
        riley->DeleteDisplacement(*displacementId);
        *displacementId = riley::DisplacementId::k_InvalidId;
    }
}

/* virtual */
void
HdPrmanMaterial::Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits)
{  
    HD_TRACE_FUNCTION();
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    SdfPath id = GetId();

    if ((*dirtyBits & HdMaterial::DirtyResource) ||
        (*dirtyBits & HdMaterial::DirtyParams)) {
        VtValue hdMatVal = sceneDelegate->GetMaterialResource(id);
        if (hdMatVal.IsHolding<HdMaterialNetworkMap>()) {
            // Convert HdMaterial to HdMaterialNetwork2 form.
            HdMaterialNetwork2 matNetwork2;
            HdMaterialNetwork2ConvertFromHdMaterialNetworkMap(
                hdMatVal.UncheckedGet<HdMaterialNetworkMap>(), &matNetwork2);
            // Apply material filter chain to the network.
            if (!_filterChain->empty()) {
                std::vector<std::string> errors;
                MatfiltExecFilterChain(*_filterChain, id, matNetwork2, {},
                                           *_sourceTypes, &errors);
                if (!errors.empty()) {
                    TF_RUNTIME_ERROR("HdPrmanMaterial: %s\n",
                        TfStringJoin(errors).c_str());
                    // Policy choice: Attempt to use the material, regardless.
                }
            }
            if (TfDebug::IsEnabled(HDPRMAN_MATERIALS)) {
                HdPrman_DumpNetwork(matNetwork2, id);
                }
            _ConvertHdMaterialNetwork2ToRman(context, id, matNetwork2,
                                             &_materialId, &_displacementId);
        } else {
            TF_WARN("HdPrmanMaterial: Expected material resource "
                    "for <%s> to contain HdMaterialNodes, but "
                    "found %s instead.",
                    id.GetText(), hdMatVal.GetTypeName().c_str());
            _ResetMaterial(context);
        }
    }
    *dirtyBits = HdChangeTracker::Clean;
}

/* virtual */
HdDirtyBits
HdPrmanMaterial::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

bool
HdPrmanMaterial::IsValid() const
{
    return _materialId != riley::MaterialId::k_InvalidId;
}

PXR_NAMESPACE_CLOSE_SCOPE

