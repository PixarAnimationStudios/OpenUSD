//
// Copyright 2022 Pixar
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
#include "hdPrman/displayFilter.h"

#include "hdPrman/renderDelegate.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/utils.h"

#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (resource)
);

TF_MAKE_STATIC_DATA(NdrTokenVec, _sourceTypes) {
    *_sourceTypes = { TfToken("OSL"), 
                      TfToken("RmanCpp"), }; }

HdPrman_DisplayFilter::HdPrman_DisplayFilter(
    SdfPath const& id)
    : HdSprim(id)
{
}

void
HdPrman_DisplayFilter::Finalize(HdRenderParam *renderParam)
{
}

void 
HdPrman_DisplayFilter::_CreateRmanDisplayFilter(
    HdSceneDelegate *sceneDelegate,
    HdPrman_RenderParam *renderParam,
    SdfPath const& filterPrimPath,
    HdMaterialNode2 const& displayFilterNode)
{
    // Create Display Filter Riley Node
    riley::ShadingNode rileyNode;
    rileyNode.type = riley::ShadingNode::Type::k_DisplayFilter;
    rileyNode.handle = RtUString(filterPrimPath.GetText());

    // Get the Display Filter ShaderPath from the ShaderRegister
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrEntry = sdrRegistry.GetShaderNodeByIdentifier(
        displayFilterNode.nodeTypeId, *_sourceTypes);
    if (!sdrEntry) {
        TF_WARN("Unknown shader ID '%s' for node <%s>\n",
                displayFilterNode.nodeTypeId.GetText(), filterPrimPath.GetText());
        return;
    }
    std::string shaderPath = sdrEntry->GetResolvedImplementationURI();
    if (shaderPath.empty()) {
        TF_WARN("Shader '%s' did not provide a valid implementation path.",
                sdrEntry->GetName().c_str());
        return;
    }
    rileyNode.name = RtUString(shaderPath.c_str());

    // Initialize the Display Filter parameters 
    for (const auto &param : displayFilterNode.parameters) {
        const SdrShaderProperty* prop = sdrEntry->GetShaderInput(param.first);
        if (!prop) {
            TF_WARN("Unknown shaderProperty '%s' for the '%s' "
                    "shader at '%s', ignoring.\n",
                    param.first.GetText(), 
                    displayFilterNode.nodeTypeId.GetText(), 
                    filterPrimPath.GetText());
            continue;
        }
        HdPrman_Utils::SetParamFromVtValue(
            RtUString(prop->GetImplementationName().c_str()),
            param.second, prop->GetType(), &rileyNode.params);
    }
    renderParam->AddDisplayFilter(sceneDelegate, filterPrimPath, rileyNode);
    return;
}

void
HdPrman_DisplayFilter::Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdDirtyBits *dirtyBits)
{
    const SdfPath &id = GetId();
    HdPrman_RenderParam *param = static_cast<HdPrman_RenderParam*>(renderParam);

    if (*dirtyBits & HdChangeTracker::DirtyParams) {
        // Only Create the DisplayFilter if connected to the RenderSettings
        // Note that this works because the RenderSettings, being a Bprim,
        // always gets synced before the DisplayFilter Sprim.
        const SdfPathVector& connectedFilters =
            param->GetConnectedDisplayFilterPaths();
        if (std::find(connectedFilters.begin(), connectedFilters.end(), id)
            != connectedFilters.end()) {
            const VtValue displayFilterResourceValue =
                sceneDelegate->Get(id, _tokens->resource);

            if (displayFilterResourceValue.IsHolding<HdMaterialNode2>()) {
                HdMaterialNode2 displayFilterNode =
                    displayFilterResourceValue.UncheckedGet<HdMaterialNode2>();
                _CreateRmanDisplayFilter(
                    sceneDelegate, param, id, displayFilterNode);
            }
        }
    } 
    // the _CreateRmanDisplayFilter() call above regenerates the filter network,
    // so if DirtyParams was set there's nothing left to do for DirtyVisibility.
    else if (*dirtyBits & HdChangeTracker::DirtyVisibility) {
        param->CreateDisplayFilterNetwork(sceneDelegate);
    }

    *dirtyBits = HdChangeTracker::Clean;
}


HdDirtyBits HdPrman_DisplayFilter::GetInitialDirtyBitsMask() const
{
    int mask = 
        HdChangeTracker::Clean | 
        HdChangeTracker::DirtyParams | 
        HdChangeTracker::DirtyVisibility;
    return (HdDirtyBits)mask;
}

PXR_NAMESPACE_CLOSE_SCOPE
