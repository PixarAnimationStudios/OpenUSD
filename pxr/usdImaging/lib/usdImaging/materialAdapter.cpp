//
// Copyright 2016 Pixar
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
#include "pxr/usdImaging/usdImaging/materialAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/material.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdRi/materialAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingMaterialAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingMaterialAdapter::~UsdImagingMaterialAdapter()
{
}

bool
UsdImagingMaterialAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsSprimTypeSupported(HdPrimTypeTokens->material);
}

bool
UsdImagingMaterialAdapter::IsPopulatedIndirectly()
{
    // Materials are populated as a consequence of populating a prim
    // which uses the material.
    return true;
}

SdfPath
UsdImagingMaterialAdapter::Populate(UsdPrim const& prim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    // Since material are populated by reference, they need to take care not to
    // be populated multiple times.
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    index->InsertSprim(HdPrimTypeTokens->material,
                       cachePath,
                       prim, shared_from_this());
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return prim.GetPath();
}

/* virtual */
void
UsdImagingMaterialAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const*
                                              instancerContext)
{
    // XXX: Time-varying parameters are not yet implemented
}

/* virtual */
void
UsdImagingMaterialAdapter::UpdateForTime(UsdPrim const& prim,
                                       SdfPath const& cachePath,
                                       UsdTimeCode time,
                                       HdDirtyBits requestedBits,
                                       UsdImagingInstancerContext const*
                                           instancerContext)
{
    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdMaterial::DirtyResource) {
        // Walk the material network and generate a HdMaterialNetworkMap
        // structure to store it in the value cache.
        HdMaterialNetworkMap materialNetworkMap;
        _GetMaterialNetworkMap(prim, &materialNetworkMap);

        valueCache->GetMaterialResource(cachePath) = materialNetworkMap;

        // Compute union of primvars from all networks
        std::vector<TfToken> primvars;
        for (const auto& entry: materialNetworkMap.map) {
            primvars.insert(primvars.end(),
                            entry.second.primvars.begin(),
                            entry.second.primvars.end());
        }
        std::sort(primvars.begin(), primvars.end());
        primvars.erase(std::unique(primvars.begin(), primvars.end()),
                       primvars.end());
        valueCache->GetMaterialPrimvars(cachePath) = primvars;
    }
}

/* virtual */
HdDirtyBits
UsdImagingMaterialAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               TfToken const& propertyName)
{
    // XXX: This doesn't get notifications for dependent nodes.
    return HdChangeTracker::AllDirty;
}

/* virtual */
void
UsdImagingMaterialAdapter::MarkDirty(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   HdDirtyBits dirty,
                                   UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

/* virtual */
void
UsdImagingMaterialAdapter::_RemovePrim(SdfPath const& cachePath,
                                 UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->material, cachePath);
}

static
void extractPrimvarsFromNode(UsdShadeShader const & shadeNode, 
                             HdMaterialNode const & node, 
                             HdMaterialNetwork *materialNetwork)
{
    // Check if it is a node that reads primvars.
    // XXX : We could be looking at more stuff here like manifolds..
    if (node.type == TfToken("Primvar_3")) {
        // Extract the primvar name from the usd shade node
        // and store it in the list of primvars in the network
        UsdShadeInput nameAttrib = shadeNode.GetInput(TfToken("varname"));
        if (nameAttrib) {
            VtValue value;
            nameAttrib.Get(&value);
            if (value.IsHolding<std::string>()) {
                materialNetwork->primvars.push_back(
                    TfToken(value.Get<std::string>()));
            }
        }        
    }
}

static
bool visitNode(UsdShadeShader const & shadeNode, 
               std::vector<UsdShadeInput> const & shadeNodeInputs,
               HdMaterialNetwork *materialNetwork)
{
    // Store the path of the node
    HdMaterialNode node;
    node.path = shadeNode.GetPath();

    // Extract the type of the node
    VtValue value;
    shadeNode.GetIdAttr().Get(&value);
    if (value.IsHolding<TfToken>()){
        node.type = value.UncheckedGet<TfToken>();

        // If a node is recognizable, we will try to extract the primvar 
        // names that is using since this can help render delegates 
        // optimize what what is needed from a prim when making data 
        // accessible for renderers.
        extractPrimvarsFromNode(shadeNode, node, materialNetwork);
    } else {
        TF_WARN("UsdShade Shader without an id: %s.", node.path.GetText());
        node.type = TfToken("PbsNetworkMaterialStandIn_2");
    }

    // Protect against inserting the same node twice.  This can happen
    // when, for example, multiple connections exist to the same node.
    auto dup = std::find(std::begin(materialNetwork->nodes), 
                         std::end(materialNetwork->nodes), 
                         node);
    if (dup != std::end(materialNetwork->nodes)) {
        return false;
    }

    // Add the parameters and the relationships of this node
    UsdShadeConnectableAPI source;
    TfToken outputName;
    UsdShadeAttributeType sourceType;
    for (size_t i = 0; i<shadeNodeInputs.size(); i++) {
        
        // Check if this input is a connection and if so follow the path
        if (UsdShadeConnectableAPI::GetConnectedSource(shadeNodeInputs[i], 
            &source, &outputName, &sourceType)) {
            
            // Store the relationship
            HdMaterialRelationship relationship;
            relationship.sourceId = shadeNode.GetPath();
            relationship.sourceTerminal = shadeNodeInputs[i].GetFullName();
            relationship.remoteId = source.GetPath();
            relationship.remoteTerminal = outputName;
            materialNetwork->relationships.push_back(relationship);
        } else {
            // Parameters detected, let's store it
            if (shadeNodeInputs[i].Get(&value)) {
                node.parameters[shadeNodeInputs[i].GetFullName()] = value;
            }
        }
    }

    materialNetwork->nodes.push_back(node);
    return true;
}

static
void walkGraph(UsdShadeShader const & shadeNode, 
               HdMaterialNetwork *materialNetwork)
{
    // Visit the current node, and if it was correctly inserted, visit the
    // inputs after. It is possible the node was not inserted (for instance
    // when its Id attribute is incorrect).
    std::vector<UsdShadeInput> shadeNodeInputs = shadeNode.GetInputs();
    if (!visitNode(shadeNode, shadeNodeInputs, materialNetwork)) {
        return;
    } 

    // Visit the inputs of this node
    UsdShadeConnectableAPI source;
    TfToken outputName;
    UsdShadeAttributeType sourceType;
    for (size_t i = 0; i<shadeNodeInputs.size(); i++) {
        // Check if this input is a connection and if so follow the path
        if (UsdShadeConnectableAPI::GetConnectedSource(shadeNodeInputs[i], 
                &source, &outputName, &sourceType)) {
            // When we find a connection, we try to walk it.
            UsdShadeShader connectedNode(source);
            walkGraph(connectedNode, materialNetwork);
        }
    }
}

void 
UsdImagingMaterialAdapter::_GetMaterialNetworkMap(UsdPrim const &usdPrim, 
                                               HdMaterialNetworkMap *materialNetworkMap)
{
    // This function expects a usdPrim of type Material. However, it will
    // only be able to fill the HdMaterialNetwork structures if the Material
    // contains a material network with a riLook relationship. Otherwise, it
    // will return the HdMaterialNetwork structures without any change.
    UsdRiMaterialAPI m(usdPrim);

    // For each network type:
    // Resolve the binding to the first node which
    // is usually the standin node in the case of a UsdRi.
    // If it fails to provide a relationship then we are not in a 
    // usdPrim that contains a correct terminal to a UsdShade Shader node.
    if( UsdRelationship matRel = m.GetBxdfOutput().GetRel() ) {
        UsdShadeShader shadeNode(
            UsdImaging_MaterialStrategy::GetTargetedShader(
                usdPrim.GetPrim(), matRel));
        walkGraph(shadeNode,
                  &materialNetworkMap->map[UsdImagingTokens->bxdf]);
    }
    if( UsdRelationship matRel = m.GetDisplacementOutput().GetRel() ) {
        UsdShadeShader shadeNode(
            UsdImaging_MaterialStrategy::GetTargetedShader(
                usdPrim.GetPrim(), matRel));
        walkGraph(shadeNode,
                  &materialNetworkMap->map[UsdImagingTokens->displacement]);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
