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
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/material.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"

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

    // Also register this adapter on behalf of any descendent
    // UsdShadeShader prims, since they are consumed to
    // create the material network.
    for (UsdPrim const& child: prim.GetDescendants()) {
        if (child.IsA<UsdShadeShader>()) {
            index->AddPrimInfo(child.GetPath(), child, shared_from_this());
        }
    }

    return prim.GetPath();
}

/* virtual */
void
UsdImagingMaterialAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const*
                                              instancerContext) const
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
                                           instancerContext) const
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
    if (propertyName == UsdGeomTokens->visibility) {
        // Materials aren't affected by visibility
        return HdChangeTracker::Clean;
    }

    // The only meaningful change is to dirty the computed resource,
    // an HdMaterialNetwork.
    return HdMaterial::DirtyResource;
}

/* virtual */
void
UsdImagingMaterialAdapter::MarkDirty(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   HdDirtyBits dirty,
                                   UsdImagingIndexProxy* index)
{
    // If this is invoked on behalf of a Shader prim underneath a
    // Material prim, walk up to the enclosing Material.
    SdfPath materialCachePath = cachePath;
    UsdPrim materialPrim = prim;
    while (materialPrim && !materialPrim.IsA<UsdShadeMaterial>()) {
        materialPrim = materialPrim.GetParent();
        materialCachePath = materialCachePath.GetParentPath();
    }
    if (!TF_VERIFY(materialPrim)) {
        return;
    }

    index->MarkSprimDirty(materialCachePath, dirty);
}


/* virtual */
void
UsdImagingMaterialAdapter::MarkMaterialDirty(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index)
{
    // If this is invoked on behalf of a Shader prim underneath a
    // Material prim, walk up to the enclosing Material.
    SdfPath materialCachePath = cachePath;
    UsdPrim materialPrim = prim;
    while (materialPrim && !materialPrim.IsA<UsdShadeMaterial>()) {
        materialPrim = materialPrim.GetParent();
        materialCachePath = materialCachePath.GetParentPath();
    }
    if (!TF_VERIFY(materialPrim)) {
        return;
    }

    index->MarkSprimDirty(materialCachePath, HdMaterial::DirtyResource);
}


/* virtual */
void
UsdImagingMaterialAdapter::_RemovePrim(SdfPath const& cachePath,
                                 UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->material, cachePath);
}

static
void _ExtractPrimvarsFromNode(UsdShadeShader const & shadeNode, 
                             HdMaterialNode const & node, 
                             HdMaterialNetwork *materialNetwork)
{
    // Check if it is a node that reads primvars.
    // XXX : We could be looking at more stuff here like manifolds..
    if (node.identifier == TfToken("Primvar_3")) {
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

// Walk the shader graph and emit nodes in topological order
// to avoid forward-references.
static
void _WalkGraph(UsdShadeShader const & shadeNode, 
               HdMaterialNetwork *materialNetwork,
               const TfTokenVector &shaderSourceTypes)
{
    // Store the path of the node
    HdMaterialNode node;
    node.path = shadeNode.GetPath();
    if (!TF_VERIFY(node.path != SdfPath::EmptyPath())) {
        return;
    }
    // If this node has already been found via another path, we do
    // not need to add it again.
    for (HdMaterialNode const& existingNode: materialNetwork->nodes) {
        if (existingNode.path == node.path) {
            return;
        }
    }

    // Visit the inputs of this node to ensure they are emitted first.
    const std::vector<UsdShadeInput> shadeNodeInputs = shadeNode.GetInputs();
    for (UsdShadeInput const& input: shadeNodeInputs) {
        // Check if this input is a connection and if so follow the path
        UsdShadeConnectableAPI source;
        TfToken sourceName;
        UsdShadeAttributeType sourceType;
        if (UsdShadeConnectableAPI::GetConnectedSource(input, 
                &source, &sourceName, &sourceType)) {
            // When we find a connection to a shading node output,
            // walk the upstream shading node.  Do not do this for
            // other sources (ex: a connection to a material
            // public interface parameter), since they are not
            // part of the shading node graph.
            if (sourceType == UsdShadeAttributeType::Output) {
                UsdShadeShader connectedNode(source);
                _WalkGraph(connectedNode, materialNetwork, shaderSourceTypes);
            }
        }
    }

    // Extract the identifier of the node
    TfToken id;
    if (!shadeNode.GetShaderId(&id)) {
        for (auto &sourceType : shaderSourceTypes) {
            if (SdrShaderNodeConstPtr n = 
                    shadeNode.GetShaderNodeForSourceType(sourceType)) {
                id = n->GetIdentifier();
                break;
            }
        }
    }

    if (!id.IsEmpty()) {
        node.identifier = id;

        // If a node is recognizable, we will try to extract the primvar 
        // names that is using since this can help render delegates 
        // optimize what what is needed from a prim when making data 
        // accessible for renderers.
        _ExtractPrimvarsFromNode(shadeNode, node, materialNetwork);
    } else {
        TF_WARN("UsdShade Shader without an id: %s.", node.path.GetText());
        node.identifier = TfToken("PbsNetworkMaterialStandIn_2");
    }

    // Add the parameters and the relationships of this node
    VtValue value;
    for (UsdShadeInput const& input: shadeNodeInputs) {
        // Check if this input is a connection and if so follow the path
        UsdShadeConnectableAPI source;
        TfToken sourceName;
        UsdShadeAttributeType sourceType;
        if (UsdShadeConnectableAPI::GetConnectedSource(input,
            &source, &sourceName, &sourceType)) {
            if (sourceType == UsdShadeAttributeType::Output) {
                // Store the relationship
                HdMaterialRelationship relationship;
                relationship.outputId = shadeNode.GetPath();
                relationship.outputName = input.GetBaseName();
                relationship.inputId = source.GetPath();
                relationship.inputName = sourceName;
                materialNetwork->relationships.push_back(relationship);
            } else if (sourceType == UsdShadeAttributeType::Input) {
                // Connected to an input on the public interface.
                // The source is not a node in the shader network, so
                // pull the value and pass it in as a parameter.
                if (UsdShadeInput connectedInput =
                    source.GetInput(sourceName)) {
                    if (connectedInput.Get(&value)) {
                        node.parameters[input.GetBaseName()] = value;
                    }
                }
            }
        } else {
            // Parameters detected, let's store it
            if (input.Get(&value)) {
                node.parameters[input.GetBaseName()] = value;
            }
        }
    }

    materialNetwork->nodes.push_back(node);
}

void 
UsdImagingMaterialAdapter::_GetMaterialNetworkMap(UsdPrim const &usdPrim, 
    HdMaterialNetworkMap *materialNetworkMap) const
{
    UsdShadeMaterial material(usdPrim);
    if (!material) {
        TF_RUNTIME_ERROR("Expected material prim at <%s> to be of type "
                         "'UsdShadeMaterial', not type '%s'; ignoring",
                         usdPrim.GetPath().GetText(),
                         usdPrim.GetTypeName().GetText());
        return;
    }
    const TfToken context = _GetMaterialNetworkSelector();
    if (UsdShadeShader s = material.ComputeSurfaceSource(context)) {
        _WalkGraph(s, &materialNetworkMap->map[UsdImagingTokens->bxdf],
                  _GetShaderSourceTypes());
    }
    if (UsdShadeShader d = material.ComputeDisplacementSource(context)) {
        _WalkGraph(d, &materialNetworkMap->map[UsdImagingTokens->displacement],
                  _GetShaderSourceTypes());
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
