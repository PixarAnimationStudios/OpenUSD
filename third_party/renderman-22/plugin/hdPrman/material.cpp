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
#include "hdPrman/matfiltConversions.h"
#include "hdPrman/matfiltConvertPreviewMaterial.h"
#include "hdPrman/matfiltFilterChain.h"
#include "hdPrman/matfiltResolveVstructs.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"
#include "RixParamList.h"

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

static void
_FindShaders(
    const std::vector<HdMaterialNode> &nodes,
    std::vector<SdrShaderNodeConstPtr> *result)
{
    auto& reg = SdrRegistry::GetInstance();
    result->resize(nodes.size());
    for (size_t i=0; i < nodes.size(); ++i) {
        NdrIdentifier id = nodes[i].identifier;
        if (SdrShaderNodeConstPtr node =
            reg.GetShaderNodeByIdentifier(id, *_sourceTypes)) {
            (*result)[i] = node;
        } else {
            (*result)[i] = nullptr;
            TF_WARN("Did not find shader %s\n", id.GetText());
        }
    }
}

static VtArray<GfVec3f>
_ConvertToVec3fArray(const VtArray<GfVec3d>& v)
{
    VtArray<GfVec3f> out;
    out.resize(v.size());

    for (size_t i=0; i<v.size(); ++i)
        for (uint8_t e=0; e<3; ++e)
            out[i][e] = v[i][e];


    return out;
}

static void
_MapHdNodesToRileyNodes(
    RixRileyManager *mgr,
    const HdMaterialNetwork &mat,
    const std::vector<SdrShaderNodeConstPtr> &shaders,
    std::vector<riley::ShadingNode> *result)
{
    result->clear();
    result->reserve(mat.nodes.size());
    if (!TF_VERIFY(mat.nodes.size() == shaders.size())) {
        return;
    }
    for (size_t i=0; i < mat.nodes.size(); ++i) {
        if (!shaders[i]) {
            TF_WARN("Unknown shader entry '%s' for node '%s'",
                             mat.nodes[i].identifier.GetText(),
                             mat.nodes[i].path.GetText());
            // It is possible the material is unusable now, but continue
            // to pull in as much of the graph as possible in case it
            // helps diagnosis.
            continue;
        }

        // Create equivalent Riley shading node.
        riley::ShadingNode sn;
        if (shaders[i]->GetContext() == _tokens->bxdf ||
            shaders[i]->GetContext() == SdrNodeContext->Surface ||
            shaders[i]->GetContext() == SdrNodeContext->Volume) {
            sn.type = riley::ShadingNode::k_Bxdf;
        } else if (shaders[i]->GetContext() == SdrNodeContext->Pattern ||
                   shaders[i]->GetContext() == _tokens->OSL) {
            sn.type = riley::ShadingNode::k_Pattern;
        } else if (shaders[i]->GetContext() == SdrNodeContext->Displacement) {
            sn.type = riley::ShadingNode::k_Displacement;
        } else {
            // This can happen if the material accidentally references
            // a non-shading node type such as a light or light-filter.
            TF_WARN("Unknown shader entry type '%s' for shader '%s'",
                    shaders[i]->GetContext().GetText(),
                    shaders[i]->GetName().c_str());
            continue;
        }
        sn.handle = RtUString(mat.nodes[i].path.GetText());
        std::string implName = shaders[i]->GetImplementationName();
        if (shaders[i]->GetSourceType() == _tokens->OSL) {
            // Explicitly specify the .oso extension to avoid possible
            // mix-up between C++ and OSL shaders of the same name.
            implName += ".oso";
        }
        sn.name = RtUString(implName.c_str());
        RixParamList *params = mgr->CreateRixParamList();
        sn.params = params;
        result->push_back(sn);

        // Convert params
        for (const auto param: mat.nodes[i].parameters) {
            const SdrShaderProperty* prop =
                shaders[i]->GetShaderInput(param.first);
            if (!prop) {
                TF_DEBUG(HDPRMAN_MATERIALS)
                    .Msg("Unknown shader property '%s' for "
                         "shader '%s' at '%s'; ignoring.\n",
                         param.first.GetText(),
                         shaders[i]->GetName().c_str(),
                         mat.nodes[i].path.GetText());
                continue;
            }
            TfToken propType = prop->GetType();
            if (propType.IsEmpty()) {
                // As a special case, silently ignore these on PxrDisplace.
                // Automatically promoting the same network for this
                // case causes a lot of errors.
                if (mat.nodes[i].identifier == _tokens->PxrDisplace) {
                    continue;
                }
                TF_DEBUG(HDPRMAN_MATERIALS)
                    .Msg("Unknown shader entry field type for "
                         "field '%s' on shader '%s' at '%s'; ignoring.\n",
                         param.first.GetText(),
                         shaders[i]->GetName().c_str(),
                         mat.nodes[i].path.GetText());
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
                    params->SetFloatArray(name, v.data(), 2);
                    ok = true;
                } 
            } else if (param.second.IsHolding<GfVec3f>()) {
                GfVec3f v = param.second.UncheckedGet<GfVec3f>();
                if (propType == SdrPropertyTypes->Color) {
                    params->SetColor(name, RtColorRGB(v[0], v[1], v[2]));
                    ok = true;
                } else if (propType == SdrPropertyTypes->Vector) {
                    params->SetVector(name, RtVector3(v[0], v[1], v[2]));
                    ok = true;
                } else if (propType == SdrPropertyTypes->Point) {
                    params->SetPoint(name, RtPoint3(v[0], v[1], v[2]));
                    ok = true;
                } else if (propType == SdrPropertyTypes->Normal) {
                    params->SetNormal(name, RtNormal3(v[0], v[1], v[2]));
                    ok = true;
                }
            } else if (param.second.IsHolding<GfVec4f>()) {
                GfVec4f v = param.second.UncheckedGet<GfVec4f>();
                if (propType == SdrPropertyTypes->Float) {
                    params->SetFloatArray(name, v.data(), 4);
                    ok = true;
                } 
            } else if (param.second.IsHolding<VtArray<GfVec3f>>()) {
                const VtArray<GfVec3f>& v =
                    param.second.UncheckedGet<VtArray<GfVec3f>>();
                if (propType == SdrPropertyTypes->Color) {
                    params->SetColorArray(
                        name,
                        reinterpret_cast<const RtColorRGB*>(v.cdata()),
                        v.size());
                    ok = true;
                } else if (propType == SdrPropertyTypes->Vector) {
                    params->SetVectorArray(
                        name,
                        reinterpret_cast<const RtVector3*>(v.cdata()),
                        v.size());
                    ok = true;
                } else if (propType == SdrPropertyTypes->Point) {
                    params->SetPointArray(
                        name,
                        reinterpret_cast<const RtPoint3*>(v.cdata()),
                        v.size());
                    ok = true;
                } else if (propType == SdrPropertyTypes->Normal) {
                    params->SetNormalArray(
                        name,
                        reinterpret_cast<const RtNormal3*>(v.cdata()),
                        v.size());
                    ok = true;
                }
            } else if (param.second.IsHolding<GfVec3d>()) {
                const GfVec3d& v = param.second.UncheckedGet<GfVec3d>();
                if (propType == SdrPropertyTypes->Color) {
                    params->SetColor(name, RtColorRGB(v[0], v[1], v[2]));
                    ok = true;
                }
            } else if (param.second.IsHolding<VtArray<GfVec3d>>()) {
                if (propType == SdrPropertyTypes->Color) {
                    const VtArray<GfVec3d>& vd =
                        param.second.UncheckedGet<VtArray<GfVec3d>>();
                    VtArray<GfVec3f> v = _ConvertToVec3fArray(vd);
                    params->SetColorArray(
                        name,
                        reinterpret_cast<const RtColorRGB*>(v.cdata()),
                        v.size());
                    ok = true;
                }
            } else if (param.second.IsHolding<float>()) {
                float v = param.second.UncheckedGet<float>();
                if (propType == SdrPropertyTypes->Int) {
                    params->SetInteger(name, int(v));
                    ok = true;
                } else if (propType == SdrPropertyTypes->Float) {
                    params->SetFloat(name, v);
                    ok = true;
                }
            } else if (param.second.IsHolding<VtArray<float>>()) {
                const VtArray<float>& v =
                    param.second.UncheckedGet<VtArray<float>>();
                if (propType == SdrPropertyTypes->Float) {
                    params->SetFloatArray(name, v.cdata(), v.size());
                    ok = true;
                }
            } else if (param.second.IsHolding<int>()) {
                int v = param.second.UncheckedGet<int>();
                if (propType == SdrPropertyTypes->Float) {
                    params->SetFloat(name, v);
                    ok = true;
                } else if (propType == SdrPropertyTypes->Int) {
                    params->SetInteger(name, v);
                    ok = true;
                }
            } else if (param.second.IsHolding<TfToken>()) {
                TfToken v = param.second.UncheckedGet<TfToken>();
                params->SetString(name, RtUString(v.GetText()));
                ok = true;
            } else if (param.second.IsHolding<std::string>()) {
                std::string v = param.second.UncheckedGet<std::string>();
                params->SetString(name, RtUString(v.c_str()));
                ok = true;
            } else if (param.second.IsHolding<SdfAssetPath>()) {
                SdfAssetPath p = param.second.Get<SdfAssetPath>();
                std::string v = p.GetResolvedPath();
                if (v.empty()) {
                    v = p.GetAssetPath();
                }
                params->SetString(name, RtUString(v.c_str()));
                ok = true;
            } else if (param.second.IsHolding<bool>()) {
                // RixParamList (specifically, RixDataType) doesn't have
                // a bool entry; we convert to integer instead.
                int v = param.second.UncheckedGet<bool>();
                params->SetInteger(name, v);
                ok = true;
            }
            if (!ok) {
                TF_DEBUG(HDPRMAN_MATERIALS)
                    .Msg("Unknown shading parameter type '%s'; skipping "
                         "parameter '%s' on node '%s' ('%s'); "
                         "expected type '%s'\n",
                         param.second.GetTypeName().c_str(),
                         param.first.GetText(),
                         mat.nodes[i].path.GetText(),
                         shaders[i]->GetName().c_str(),
                         propType.GetText());
            }
        }

        // Convert relationships.
        for (const HdMaterialRelationship &rel: mat.relationships) {
            // Handle only the relationships that feed this node's inputs.
            if (rel.outputId != mat.nodes[i].path) {
                continue;
            }
            // Find the output & input shader nodes of the connection.
            SdrShaderNodeConstPtr outputShader = shaders[i];
            SdrShaderNodeConstPtr inputShader = nullptr;
            for (size_t j=0; j < mat.nodes.size(); ++j) {
                if (mat.nodes[j].path == rel.inputId) {
                    inputShader = shaders[j];
                    break;
                }
            }
            if (!inputShader) {
                TF_WARN("Unknown shader for connection input '%s' on '%s' "
                        "connected from '%s'; ignoring.",
                        rel.inputName.GetText(),
                        rel.inputId.GetText(),
                        mat.nodes[i].path.GetText());
                continue;
            }

            // Find the shader properties, so that we can look up
            // the property implementation names.
            SdrShaderPropertyConstPtr outputProp =
                outputShader->GetShaderInput(rel.outputName);
            SdrShaderPropertyConstPtr inputProp =
                inputShader->GetShaderOutput(rel.inputName);
            if (!outputProp) {
                TF_WARN("Unknown input '%s' on shader '%s' at '%s'; ignoring.",
                        rel.outputName.GetText(),
                        outputShader->GetName().c_str(),
                        mat.nodes[i].path.GetText());
                continue;
            }
            if (!inputProp) {
                TF_WARN("Unknown shader output '%s' for "
                        "shader '%s' at '%s'; ignoring.",
                        rel.inputName.GetText(),
                        inputShader->GetName().c_str(),
                        mat.nodes[i].path.GetText());
                continue;
            }

            // Prman syntax for parameter references is "handle:param".
            RtUString name(outputProp->GetImplementationName().c_str());
            RtUString inputRef(
                (rel.inputId.GetString()+":"
                 + inputProp->GetImplementationName().c_str())
                .c_str());

            // Establish the Riley connection.
            TfToken propType = outputProp->GetType();
            if (propType == SdrPropertyTypes->Color) {
                params->ReferenceColor(name, inputRef);
            } else if (propType == SdrPropertyTypes->Vector) {
                params->ReferenceVector(name, inputRef);
            } else if (propType == SdrPropertyTypes->Point) {
                params->ReferencePoint(name, inputRef);
            } else if (propType == SdrPropertyTypes->Normal) {
                params->ReferenceNormal(name, inputRef);
            } else if (propType == SdrPropertyTypes->Float) {
                params->ReferenceFloat(name, inputRef);
            } else if (propType == SdrPropertyTypes->Int) {
                params->ReferenceInteger(name, inputRef);
            } else if (propType == SdrPropertyTypes->String) {
                params->ReferenceString(name, inputRef);
            } else if (propType == SdrPropertyTypes->Struct) {
                params->ReferenceStruct(name, inputRef);
            } else {
                TF_WARN("Unknown type '%s' for property '%s' "
                        "on shader '%s' at %s; ignoring.",
                        propType.GetText(),
                        rel.outputName.GetText(),
                        shaders[i]->GetName().c_str(),
                        mat.nodes[i].path.GetText());
            }
        }
    }
}
    
// Debug helper
void
HdPrman_DumpMat(std::string const& label,
                SdfPath const& id, HdMaterialNetwork const& mat)
{
    printf("%s material network for %s:\n", label.c_str(), id.GetText());
    for (HdMaterialNode const& node: mat.nodes) {
        printf("  --Node--\n");
        printf("    %s\n", node.path.GetText());
        printf("    %s\n", node.identifier.GetText());
        for (auto const& entry: node.parameters) {
            printf("\tparam %s: %s\n", entry.first.GetText(),
                   TfStringify(entry.second).c_str());
        }
    }
    if (!mat.relationships.empty()) {
        printf("  --Connections--\n");
        for (HdMaterialRelationship const& rel: mat.relationships) {
            printf("    %s.%s -> %s.%s\n",
                rel.outputId.GetText(),
                rel.outputName.GetText(),
                rel.inputId.GetText(),
                rel.inputName.GetText());
        }
    }
    if (!mat.primvars.empty()) {
        printf("  --Primvars--\n");
        for (TfToken const& primvar: mat.primvars) {
            printf("    %s\n", primvar.GetText());
        }
    }
}
    
// Convert an HdMaterialNetwork to a Riley shading network.
static bool
_ConvertHdMaterialToRman(
    RixRileyManager *mgr,
    const HdMaterialNetwork &sourceMaterialNetwork,
    std::vector<riley::ShadingNode> *result)
{
    HdMaterialNetwork mat = sourceMaterialNetwork;
    std::vector<SdrShaderNodeConstPtr> shaders;
    _FindShaders(mat.nodes, &shaders);
    _MapHdNodesToRileyNodes(mgr, mat, shaders, result);
    return !result->empty();
}

/* virtual */
void
HdPrmanMaterial::Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits)
{  
    HdPrman_Context *context =
        static_cast<HdPrman_RenderParam*>(renderParam)->AcquireContext();

    SdfPath id = GetId();

    RixRileyManager *mgr = context->mgr;
    riley::Riley *riley = context->riley;

    if ((*dirtyBits & HdMaterial::DirtyResource) ||
        (*dirtyBits & HdMaterial::DirtyParams)) {
        VtValue vtMat = sceneDelegate->GetMaterialResource(id);
        if (vtMat.IsHolding<HdMaterialNetworkMap>()) {
            HdMaterialNetworkMap networkMap =
                vtMat.UncheckedGet<HdMaterialNetworkMap>();

            // Apply material filter chain to each terminal.
            if (!_filterChain->empty()) {
                std::vector<std::string> errors;
                for (auto& entry: networkMap.map) {
                    MatfiltNetwork network;
                    MatfiltConvertFromHdMaterialNetworkMapTerminal(
                        networkMap, entry.first, &network);
                    MatfiltExecFilterChain(*_filterChain, id, network, {},
                                           *_sourceTypes, &errors);
                    // Note that we count on the fact this won't modify
                    // anything but the original terminal here.
                    TfReset(entry.second.nodes);
                    TfReset(entry.second.relationships);
                    MatfiltConvertToHdMaterialNetworkMap(network, &networkMap);
                }
                if (!errors.empty()) {
                    TF_RUNTIME_ERROR("HdPrmanMaterial: %s\n",
                        TfStringJoin(errors).c_str());
                    // Policy choice: Attempt to use the material, regardless.
                }
            }

            HdMaterialNetwork surfaceNet, dispNet, volNet;
            TfMapLookup(networkMap.map,
                        HdMaterialTerminalTokens->surface, &surfaceNet);
            TfMapLookup(networkMap.map,
                        HdMaterialTerminalTokens->displacement, &dispNet);
            TfMapLookup(networkMap.map,
                        HdMaterialTerminalTokens->volume, &volNet);

            if (TfDebug::IsEnabled(HDPRMAN_MATERIALS)) {
                if (!surfaceNet.nodes.empty()) {
                    HdPrman_DumpMat("Surface", id, surfaceNet);
                }
                if (!dispNet.nodes.empty()) {
                    HdPrman_DumpMat("Displacement", id, dispNet);
                }
                if (!volNet.nodes.empty()) {
                    HdPrman_DumpMat("Volume", id, volNet);
                }
            }

            std::vector<riley::ShadingNode> nodes;

            // Surface/Volume
            if (_ConvertHdMaterialToRman(mgr, surfaceNet, &nodes) ||
                _ConvertHdMaterialToRman(mgr, volNet, &nodes)) {
                if (_materialId == riley::MaterialId::k_InvalidId) {
                    _materialId = riley->CreateMaterial(&nodes[0],
                                                        nodes.size());
                } else {
                    riley->ModifyMaterial(_materialId, &nodes[0], nodes.size());
                }
            } else {
                // Clear out any previous material.
                // _ConvertHdMaterialToRman will have already emitted
                // any appropriate diagnostics on failure.
                if (_materialId != riley::MaterialId::k_InvalidId) {
                    riley->DeleteMaterial(_materialId);
                    _materialId = riley::MaterialId::k_InvalidId;
                }
            }
            for (const auto &node: nodes) {
                mgr->DestroyRixParamList(
                    const_cast<RixParamList*>(node.params));
            }
            nodes.clear();

            // Displacement
            if (_ConvertHdMaterialToRman(mgr, dispNet, &nodes)) {
                if (_displacementId == riley::DisplacementId::k_InvalidId) {
                    _displacementId = riley->CreateDisplacement(&nodes[0],
                                                                nodes.size());
                } else {
                    riley->ModifyDisplacement(_displacementId, &nodes[0],
                                              nodes.size());
                }
            } else {
                // Clear out any previous displacement.
                // _ConvertHdMaterialToRman will have already emitted
                // any appropriate diagnostics on failure.
                if (_displacementId != riley::DisplacementId::k_InvalidId) {
                    riley->DeleteDisplacement(_displacementId);
                    _displacementId = riley::DisplacementId::k_InvalidId;
                }
            }
            for (const auto &node: nodes) {
                mgr->DestroyRixParamList(
                    const_cast<RixParamList*>(node.params));
            }
            nodes.clear();

        } else {
            TF_WARN("HdPrmanMaterial: Expected material resource "
                    "for <%s> to contain HdMaterialNodes, but "
                    "found %s instead.",
                    id.GetText(), vtMat.GetTypeName().c_str());
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

//virtual
void
HdPrmanMaterial::Reload()
{
    // TODO Is it possible to reload shaders during an rman session?
}

bool
HdPrmanMaterial::IsValid() const
{
    return _materialId != riley::MaterialId::k_InvalidId;
}

PXR_NAMESPACE_CLOSE_SCOPE

