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
#include "hdPrman/convertPreviewMaterial.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/types.h"
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
    (PbsNetworkMaterialStandIn_2)
    (PxrSurface)
    (PxrDisplace)
    (MaterialLayer_1)
    (MaterialLayer_2)
    (bxdf)
    (displacement)
    (pbsMaterialIn)
    (inputMaterial)
);

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


// Apply studio-specific network transformations, similar to what
// PxRfkPbsNetworkMaterialStandInResolveOp does in katana.
//
// Roughly speaking, this means: 
// - using the bxdf relationship to imply displacement as well
// - PbsNetworkMaterialStandIn_2 substitution
// - conditional vstruct expansion (vstructConditionalExpr metadata)
//
// XXX Ideally this logic can be eventually be driven by the
// SdrRegistry or a related helper library.
//
static void
_ApplyStudioFixes(HdMaterialNetworkMap *netMap)
{
    HdMaterialNetwork bxdfNet, dispNet;
    TfMapLookup(netMap->map, _tokens->bxdf, &bxdfNet);
    TfMapLookup(netMap->map, _tokens->displacement, &dispNet);

    // If no disp network was bound, try using the "bxdf" network
    // for that purpose.
    if (dispNet.nodes.empty() && !bxdfNet.nodes.empty()) {
        dispNet = bxdfNet;
    }

    // bxdf
    for (HdMaterialNode &node: bxdfNet.nodes) {
        if (node.identifier == _tokens->PbsNetworkMaterialStandIn_2) {
            node.identifier = _tokens->PxrSurface;
        }
        // XXX Hacky upgrade for testing w/ existing show assets
        if (node.identifier == _tokens->MaterialLayer_1) {
            node.identifier = _tokens->MaterialLayer_2;
        }
    }
    for (HdMaterialRelationship &rel: bxdfNet.relationships) {
        if (rel.outputName == _tokens->pbsMaterialIn) {
            rel.outputName = _tokens->inputMaterial;
        }
    }

    // displacement
    for (HdMaterialNode &node: dispNet.nodes) {
        if (node.identifier == _tokens->PbsNetworkMaterialStandIn_2) {
            node.identifier = _tokens->PxrDisplace;
            // XXX Ideally, we could prune any non-displacement
            // parameters, to avoid warnings from Renderman.
        }
    }
    for (HdMaterialRelationship &rel: dispNet.relationships) {
        if (rel.outputName == _tokens->pbsMaterialIn) {
            rel.outputName = _tokens->inputMaterial;
        }
    }

    // Commit fixed networks
    netMap->map[_tokens->bxdf] = bxdfNet;
    netMap->map[_tokens->displacement] = dispNet;
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
        if (SdrShaderNodeConstPtr oslNode = reg.GetShaderNodeByIdentifierAndType(
                id, TfToken("OSL"))) {
            (*result)[i] = oslNode;
        } else if (SdrShaderNodeConstPtr rmapCppNode= 
                reg.GetShaderNodeByIdentifierAndType(id, TfToken("RmanCpp"))) {
            (*result)[i] = rmapCppNode;
        } else {
            (*result)[i] = nullptr;
            TF_WARN("Did not find shader %s\n", id.GetText());
        }
    }
}

static void
_ExpandVstructs(
    HdMaterialNetwork *mat,
    const std::vector<SdrShaderNodeConstPtr> &shaders)
{
    if (!TF_VERIFY(mat->nodes.size() == shaders.size())) {
        return;
    }
    std::vector<HdMaterialRelationship> result;
    // Check all input relationships for ones that imply vstruct connections.
    for (HdMaterialRelationship const& rel: mat->relationships) {
        // To check vstruct-status we need the shader entry.
        // Find the downstream HdMaterialNetwork node.  O(n).
        int outputNodeIndex = -1;
        for (size_t i = 0; i < mat->nodes.size(); ++i) {
            if (mat->nodes[i].path == rel.outputId) {
                outputNodeIndex = i;
                break;
            }
        }
        if (outputNodeIndex == -1) {
            // This can happen if the material network contains a bogus
            // connection path.
            TF_WARN("Invalid connection to unknown output node '%s'; "
                    "ignoring.", rel.outputId.GetText());
            continue;
        }
        SdrShaderNodeConstPtr outputShader = shaders[outputNodeIndex];
        if (!outputShader) {
            TF_WARN("Invalid connection to output node '%s' with "
                             "unknown shader entry; ignoring.",
                             rel.outputId.GetText());
            continue;
        }
        // The output of the connection is an input of outputShader.
        SdrShaderPropertyConstPtr outputProp =
            outputShader->GetShaderInput(rel.outputName);
        if (!outputProp) {
            TF_WARN("Unknown output property %s on %s with id %s",
                    rel.outputName.GetText(),
                    rel.outputId.GetText(),
                    outputShader->GetName().c_str());
            continue;
        }

        // Look up the input shader and property.
        int inputNodeIndex = -1;
        for (size_t i = 0; i < mat->nodes.size(); ++i) {
            if (mat->nodes[i].path == rel.inputId) {
                inputNodeIndex = i;
                break;
            }
        }
        if (inputNodeIndex == -1) {
            // This can happen if the material network contains a bogus
            // connection path.
            TF_WARN("Invalid connection to unknown input node '%s'; "
                "ignoring.", rel.inputId.GetText());
            continue;
        }
        SdrShaderNodeConstPtr inputShader = shaders[inputNodeIndex];
        if (!inputShader) {
            TF_WARN("Invalid connection to input node '%s' with "
                "unknown shader entry; ignoring.", rel.inputId.GetText());
            continue;
        }
        SdrShaderPropertyConstPtr inputProp =
            inputShader->GetShaderOutput(rel.inputName);
        if (!inputProp) {
            TF_WARN("Unknown input property %s on %s for shader %s",
                    rel.inputName.GetText(),
                    rel.inputId.GetText(),
                    inputShader->GetName().c_str());
            continue;
        }

        // XXX src vs input vstruct ness
        if (!outputProp->IsVStruct() && !inputProp->IsVStruct()) {
            // Not a vstruct.  Retain as-is.
            result.push_back(rel);
            continue;
        }

        std::string outputVstructName = outputProp->GetName();
        std::string inputVstructName = inputProp->GetName();

        // Find corresponding vstruct properties on the nodes.
        for (TfToken const& outputName: inputShader->GetOutputNames()) {
            auto const& input = inputShader->GetShaderOutput(outputName);
            if (input->GetVStructMemberOf() != inputVstructName) {
                continue;
            }
            TF_VERIFY(input->IsVStructMember());
            std::string member = input->GetVStructMemberName();

            // Find the corresponding input on outputShader.
            for (TfToken const& inputName: outputShader->GetInputNames()) {
                auto const& output = outputShader->GetShaderInput(inputName);
                if (output->GetVStructMemberOf() != outputVstructName) {
                    // Different vstruct, or not part of a vstruct
                    continue;
                }
                if (output->GetVStructMemberName() != member) {
                    // Different field of this vstruct
                    continue;
                }

                // Check if there is already an explicit connection
                // or value for that input -- either will take
                // precedence over the implicit vstruct connection.
                bool hasLocalValue = false;
                for (auto const& param:
                     mat->nodes[outputNodeIndex].parameters) {
                    if (param.first == output->GetName()) {
                        hasLocalValue = true;
                        break;
                    }
                }
                if (hasLocalValue) {
                    // This member has a local ("output") value.  Skip.
                    continue;
                }
                bool hasExplicitConnection = false;
                for (HdMaterialRelationship const& r: mat->relationships) {
                    if (r.outputId == rel.outputId &&
                        r.outputName == output->GetName()) {
                        hasExplicitConnection = true;
                        break;
                    }
                }
                if (hasExplicitConnection) {
                    // This member is already connected.
                    continue;
                }

                // Create the implied connection.
                HdMaterialRelationship newRel {
                    rel.inputId,
                    input->GetName(),
                    rel.outputId,
                    output->GetName()
                };
                result.push_back(newRel);
            }
        }
    }
    std::swap(mat->relationships, result);
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
        //if (shaders[i]->GetContext() == SdrNodeContext->Surface) {
        if (shaders[i]->GetContext() == TfToken("bxdf")) {
            sn.type = riley::ShadingNode::k_Bxdf;
        } else if (shaders[i]->GetContext() == SdrNodeContext->Pattern ||
                   shaders[i]->GetContext() == TfToken("OSL")) {
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
        sn.name = RtUString(shaders[i]->GetImplementationName().c_str());
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
                         "shader '%s' at '%s'; ignoring.",
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
                         "field '%s' on shader '%s' at '%s'; ignoring.",
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
            } else if (param.second.IsHolding<VtArray<GfVec3f>>()) {
                VtArray<GfVec3f> v =
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
                VtArray<float> v = param.second.UncheckedGet<VtArray<float>>();
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
                         "expected type '%s'",
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
    _ExpandVstructs(&mat, shaders);
    _MapHdNodesToRileyNodes(mgr, mat, shaders, result);
    return true;
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

            HdPrman_ConvertUsdPreviewMaterial(&networkMap);
            _ApplyStudioFixes(&networkMap);

            HdMaterialNetwork bxdfNet, dispNet;
            TfMapLookup(networkMap.map, _tokens->bxdf, &bxdfNet);
            TfMapLookup(networkMap.map, _tokens->displacement, &dispNet);

            if (TfDebug::IsEnabled(HDPRMAN_MATERIALS)) {
                if (!bxdfNet.nodes.empty()) {
                    HdPrman_DumpMat("BXDF", id, bxdfNet);
                }
                if (!dispNet.nodes.empty()) {
                    HdPrman_DumpMat("Displacement", id, dispNet);
                }
            }

            std::vector<riley::ShadingNode> nodes;

            // Bxdf
            if (_ConvertHdMaterialToRman(mgr, bxdfNet, &nodes)) {
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

            // Displacement
            nodes.clear();
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

