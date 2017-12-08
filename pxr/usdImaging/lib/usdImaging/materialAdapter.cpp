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

#include "pxr/imaging/hd/shader.h"

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
    return index->IsSprimTypeSupported(HdPrimTypeTokens->shader);
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
    // Since shaders are populated by reference, they need to take care not to
    // be populated multiple times.
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    index->InsertSprim(HdPrimTypeTokens->shader,
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

    if (requestedBits & HdShader::DirtyResource) {
        // Walk the material network and generate a HdMaterialNodes structure
        // to store it in the value cache.
        HdMaterialNodes materialNodes;
        _GetMaterialNodes(prim, &materialNodes);

        valueCache->GetMaterialResource(cachePath) = materialNodes;
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
    index->RemoveSprim(HdPrimTypeTokens->shader, cachePath);
}

static
bool visitNode(UsdShadeShader const & shadeNode, 
               std::vector<UsdShadeInput> const & shadeNodeInputs,
               HdMaterialNodes *materialNodes)
{
    // Store the path of the node
    HdMaterialNode node;
    node.path = shadeNode.GetPath();

    // Extract the type of the node
    VtValue value;
    shadeNode.GetIdAttr().Get(&value);
    if (value.IsHolding<TfToken>()){
        node.type = value.UncheckedGet<TfToken>();
    } else {
        TF_WARN("UsdShade Shader without an id: %s.", node.path.GetText());
        node.type = TfToken("PbsNetworkMaterialStandIn_2");
    }

    // Protect against inserting the same node twice, and return false. 
    // Authoring tools could guarantee that too during content creation.
    auto dup = std::find(std::begin(materialNodes->nodes), 
                         std::end(materialNodes->nodes), 
                         node);
    if (dup != std::end(materialNodes->nodes)) {
        TF_WARN("UsdShade Shader node duplicated: %s.", node.path.GetText());
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
            materialNodes->relationships.push_back(relationship);
        } else {
            // Parameters detected, let's store it
            HdValueAndRole entry;
            shadeNodeInputs[i].Get(&entry.value);
            entry.role = shadeNodeInputs[i].GetAttr().GetRoleName();
            node.parameters[shadeNodeInputs[i].GetFullName()] = entry;
        }
    }

    materialNodes->nodes.push_back(node);
    return true;
}

static
void walkGraph(UsdShadeShader const & shadeNode, HdMaterialNodes *materialNodes)
{
    // Visit the current node, and if it was correctly inserted, visit the
    // inputs after. It is possible the node was not inserted (for instance
    // when its Id attribute is incorrect).
    std::vector<UsdShadeInput> shadeNodeInputs = shadeNode.GetInputs();
    if (!visitNode(shadeNode, shadeNodeInputs, materialNodes)) {
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
            walkGraph(connectedNode, materialNodes);
        }
    }
}

void 
UsdImagingMaterialAdapter::_GetMaterialNodes(UsdPrim const &usdPrim, 
                                            HdMaterialNodes *materialNodes)
{
    // This function expects a usdPrim of type Material. However, it will
    // only be able to fill the HdMaterialNodes structures if the Material
    // contains a material network with a riLook relationship. Otherwise, it
    // will return the HdMaterialNodes structures without any change.
    UsdRiMaterialAPI m(usdPrim);

    // Resolve the binding to the first node which
    // is usually the standin node in the case of a UsdRi.
    // If it fails to provide a relationship then we are not in a 
    // usdPrim that contains a correct bxdf terminal to a UsdShade Shader node.
    if( UsdRelationship matRel = m.GetBxdfOutput().GetRel() ) {
        UsdShadeShader shadeNode(
            UsdImaging_MaterialStrategy::GetTargetedShader(
                usdPrim.GetPrim(), matRel));
        walkGraph(shadeNode, materialNodes);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
