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

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingMaterialAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

static TfToken
_GetShaderNodeForSourceTypeFallback(
    UsdShadeShader const& shader,
    TfToken const& networkSelector)
{
    std::string identifier;
    TfToken implSource = shader.GetImplementationSource();

    if (implSource == UsdShadeTokens->id) {
        TfToken shaderId;

        if (shader.GetShaderId(&shaderId)) {
            auto &shaderReg = SdrRegistry::GetInstance();
            if (SdrShaderNodeConstPtr sdrNode = 
                    shaderReg.GetShaderNodeByIdentifierAndType(shaderId, 
                        networkSelector)) {
                identifier = sdrNode->GetSourceURI();
            }
        }
    } else if (implSource == UsdShadeTokens->sourceAsset) {
        SdfAssetPath sourceAsset;
        if (shader.GetSourceAsset(&sourceAsset, networkSelector)) {
            identifier = ArGetResolver().Resolve(sourceAsset.GetAssetPath());
        }
    } else if (implSource == UsdShadeTokens->sourceCode) {
        std::string sourceCode;
        if (shader.GetSourceCode(&sourceCode, networkSelector)) {
            identifier = sourceCode;
        }
    }

    return TfToken(identifier);
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
        TfToken const& networkSelector = _GetMaterialNetworkSelector();

        // Walk the material network and generate a HdMaterialNetworkMap
        // structure to store it in the value cache.
        HdMaterialNetworkMap networkMap;
        _GetMaterialNetworkMap(prim, networkSelector, &networkMap);

        valueCache->GetMaterialResource(cachePath) = networkMap;
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
    MarkDirty(prim, cachePath, HdMaterial::DirtyResource, index);
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

// Walk the shader graph and emit nodes in topological order to avoid
// forward-references.
// This current implementation of _WalkGraph flattens the shading network into
// a single graph with connectivity and values. It does not try to identify
// NodeGraphs that can be processed once and shared, or even look for a
// pre-baked implementation. Currently neither the material processing in Hydra
// nor any of the back-ends (like HdPrman) can make use of this anyway.
static
void _WalkGraph(
    UsdShadeShader const & shadeNode,
    HdMaterialNetwork* materialNetwork,
    TfToken const& networkSelector,
    SdfPathSet* visitedNodes,
    TfTokenVector const & shaderSourceTypes)
{
    // Store the path of the node
    HdMaterialNode node;
    node.path = shadeNode.GetPath();
    if (!TF_VERIFY(node.path != SdfPath::EmptyPath())) {
        return;
    }

    // If this node has already been found via another path, we do
    // not need to add it again.
    if (visitedNodes->count(node.path) > 0) {
        return;
    }

    // Visit the inputs of this node to ensure they are emitted first.
    const std::vector<UsdShadeInput> shadeNodeInputs = shadeNode.GetInputs();
    for (UsdShadeInput input: shadeNodeInputs) {

        TfToken inputName = input.GetBaseName();

        // Find the attribute this input is getting its value from, which might
        // be an output or an input, including possibly itself if not connected
        UsdShadeAttributeType attrType;
        UsdAttribute attr = input.GetValueProducingAttribute(&attrType);

        if (attrType == UsdShadeAttributeType::Output) {
            // If it is an output on a shading node we visit the node and also
            // create a relationship in the network
            _WalkGraph(UsdShadeShader(
                attr.GetPrim()),
                materialNetwork,
                networkSelector,
                visitedNodes,
                shaderSourceTypes);

            HdMaterialRelationship relationship;
            relationship.outputId = node.path;
            relationship.outputName = inputName;
            relationship.inputId = attr.GetPrim().GetPath();
            relationship.inputName = UsdShadeOutput(attr).GetBaseName();
            materialNetwork->relationships.push_back(relationship);
        } else if (attrType == UsdShadeAttributeType::Input) {
            // If it is an input attribute we get the authored value
            VtValue value;
            if (attr.Get(&value)) {
                node.parameters[inputName] = value;
            }
        }
    }

    // Extract the identifier of the node.
    // GetShaderNodeForSourceType will try to find/create an Sdr node for all
    // three info cases: info:id, info:sourceAsset and info:sourceCode.
    TfToken id;
    if (!shadeNode.GetShaderId(&id)) {
        for (auto const& sourceType : shaderSourceTypes) {
            if (SdrShaderNodeConstPtr sdrNode = 
                    shadeNode.GetShaderNodeForSourceType(sourceType)) {
                id = sdrNode->GetIdentifier();
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
    } 
    
    materialNetwork->nodes.push_back(node);
    visitedNodes->emplace(node.path);
}

static void
_BuildHdMaterialNetworkFromTerminal(
    UsdShadeShader const& usdTerminal,
    TfToken const& terminalIdentifier,
    TfToken const& networkSelector,
    TfTokenVector const& shaderSourceTypes,
    HdMaterialNetworkMap *materialNetworkMap)
{
    HdMaterialNetwork& network = materialNetworkMap->map[terminalIdentifier];
    std::vector<HdMaterialNode>& nodes = network.nodes;
    SdfPathSet visitedNodes;

    _WalkGraph(
        usdTerminal, 
        &network,
        networkSelector,
        &visitedNodes, 
        shaderSourceTypes);

    if (!TF_VERIFY(!nodes.empty())) return;

    // _WalkGraph() inserts the terminal last in the nodes list.
    HdMaterialNode& terminalNode = nodes.back();

    // If the identifier could not be found, there likely isn't a Sdr plugin
    // for this network type. This is currently the case for glslfx files.
    // Put source path/code as identifier for backend to resolve.
    // XXX Deprecate when glslfx has a Sdr plugin?
    if (terminalNode.identifier.IsEmpty()) {
        terminalNode.identifier = _GetShaderNodeForSourceTypeFallback(
            usdTerminal, networkSelector);
    }

    if (terminalNode.identifier.IsEmpty()) {
        TF_WARN("UsdShade Shader without id: %s.", terminalNode.path.GetText());
    }

    // Store terminals on material network so backend can easily access them.
    materialNetworkMap->terminals.push_back(terminalNode.path);
};

void 
UsdImagingMaterialAdapter::_GetMaterialNetworkMap(
    UsdPrim const &usdPrim, 
    TfToken const& networkSelector,
    HdMaterialNetworkMap *networkMap) const
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
    TfTokenVector shaderSourceTypes = _GetShaderSourceTypes();

    if (UsdShadeShader s = material.ComputeSurfaceSource(context)) {
        _BuildHdMaterialNetworkFromTerminal(
            s, 
            HdMaterialTerminalTokens->surface,
            networkSelector,
            shaderSourceTypes,
            networkMap);
    }

    if (UsdShadeShader d = material.ComputeDisplacementSource(context)) {
        _BuildHdMaterialNetworkFromTerminal(
            d,
            HdMaterialTerminalTokens->displacement,
            networkSelector,
            shaderSourceTypes,
            networkMap);
    }

    if (UsdShadeShader v = material.ComputeVolumeSource(context)) {
        _BuildHdMaterialNetworkFromTerminal(
            v,
            HdMaterialTerminalTokens->volume,
            networkSelector,
            shaderSourceTypes,
            networkMap);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
