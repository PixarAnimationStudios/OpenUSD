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
#include "pxr/pxr.h"

#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/computation.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/flatNormals.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/instancer.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/mesh.h"
#include "pxr/imaging/hdSt/meshShaderKey.h"
#include "pxr/imaging/hdSt/meshTopology.h"
#include "pxr/imaging/hdSt/primUtils.h"
#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/renderParam.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/smoothNormals.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/vertexAdjacency.h"

#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/base/arch/hash.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/flatNormals.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/smoothNormals.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/diagnostic.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/vt/value.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE


// for debugging
TF_DEFINE_ENV_SETTING(HD_ENABLE_FORCE_QUADRANGULATE, 0,
                      "Apply quadrangulation for all meshes for debug");

// default to use packed normals
TF_DEFINE_ENV_SETTING(HD_ENABLE_PACKED_NORMALS, 1,
                      "Use packed normals");

// Use more recognizable names for each compute queue the mesh computations use.
namespace {
    constexpr HdStComputeQueue _CopyExtCompQueue = HdStComputeQueueZero;
    constexpr HdStComputeQueue _RefinePrimvarCompQueue = HdStComputeQueueOne;
    constexpr HdStComputeQueue _NormalsCompQueue = HdStComputeQueueTwo;
    constexpr HdStComputeQueue _RefineNormalsCompQueue = HdStComputeQueueThree;
}

HdStMesh::HdStMesh(SdfPath const& id)
    : HdMesh(id)
    , _topology()
    , _vertexAdjacencyBuilder()
    , _topologyId(0)
    , _vertexPrimvarId(0)
    , _customDirtyBitsInUse(0)
    , _pointsDataType(HdTypeInvalid)
    , _sceneNormalsInterpolation()
    , _cullStyle(HdCullStyleDontCare)
    , _hasMirroredTransform(false)
    , _doubleSided(false)
    , _flatShadingEnabled(false)
    , _displacementEnabled(true)
    , _limitNormals(false)
    , _sceneNormals(false)
    , _hasVaryingTopology(false)
    , _displayOpacity(false)
    , _occludedSelectionShowsThrough(false)
    , _pointsShadingEnabled(false)
    , _fvarTopologyTracker(std::make_unique<_FvarTopologyTracker>())
{
    /*NOTHING*/
}

HdStMesh::~HdStMesh() = default;

void
HdStMesh::UpdateRenderTag(HdSceneDelegate *delegate,
                          HdRenderParam *renderParam)
{
    HdStUpdateRenderTag(delegate, renderParam, this);
}

void
HdStMesh::Sync(HdSceneDelegate *delegate,
               HdRenderParam   *renderParam,
               HdDirtyBits     *dirtyBits,
               TfToken const   &reprToken)
{
    _UpdateVisibility(delegate, dirtyBits);

    bool updateMaterialTags = false;
    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        HdStSetMaterialId(delegate, renderParam, this);
        updateMaterialTags = true;
    }
    if (*dirtyBits & (HdChangeTracker::DirtyDisplayStyle|
                      HdChangeTracker::NewRepr)) {
        updateMaterialTags = true;
    }

    // Check if either the material or geometric shaders need updating for
    // draw items of all the reprs.
    bool updateMaterialNetworkShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyMaterialId|
                      HdChangeTracker::NewRepr)) {
        updateMaterialNetworkShader = true;
    }

    bool updateGeometricShader = false;
    if (*dirtyBits & (HdChangeTracker::DirtyDisplayStyle|
                      HdChangeTracker::DirtyCullStyle|
                      HdChangeTracker::DirtyDoubleSided|
                      HdChangeTracker::DirtyMaterialId|
                      HdChangeTracker::DirtyTopology| // topological visibility
                      HdChangeTracker::DirtyInstancer|
                      HdChangeTracker::NewRepr)) {
        updateGeometricShader = true;
    }

    bool displayOpacity = _displayOpacity;
    bool hasMirroredTransform = _hasMirroredTransform;
    _UpdateRepr(delegate, renderParam, reprToken, dirtyBits);

    if (hasMirroredTransform != _hasMirroredTransform) {
        updateGeometricShader = true;
    }

    if (updateMaterialTags || 
        (GetMaterialId().IsEmpty() && displayOpacity != _displayOpacity)) {
        _UpdateMaterialTagsForAllReprs(delegate, renderParam);
    }

    if (updateMaterialNetworkShader || updateGeometricShader) {
        _UpdateShadersForAllReprs(delegate,
                                  renderParam,
                                  updateMaterialNetworkShader,
                                  updateGeometricShader);
    }

    // This clears all the non-custom dirty bits. This ensures that the rprim
    // doesn't have pending dirty bits that add it to the dirty list every
    // frame.
    // XXX: GetInitialDirtyBitsMask sets certain dirty bits that aren't
    // reset (e.g. DirtyExtent, DirtyPrimID) that make this necessary.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

void
HdStMesh::Finalize(HdRenderParam *renderParam)
{
    HdStMarkGarbageCollectionNeeded(renderParam);

    HdStRenderParam * const stRenderParam =
        static_cast<HdStRenderParam*>(renderParam);

    // Decrement material tag counts for each draw item material tag
    for (auto const& reprPair : _reprs) {
        const TfToken &reprToken = reprPair.first;
        _MeshReprConfig::DescArray const &descs =
            _GetReprDesc(reprToken);
        HdReprSharedPtr repr = reprPair.second;
        int drawItemIndex = 0;
        int geomSubsetDescIndex = 0;
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            if (descs[descIdx].geomStyle == HdMeshGeomStyleInvalid) {
                continue;
            }

            {
                HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItem(drawItemIndex++));
                stRenderParam->DecreaseMaterialTagCount(
                    drawItem->GetMaterialTag());
            }

            if (descs[descIdx].geomStyle == HdMeshGeomStylePoints) {
                continue;
            }

            const HdGeomSubsets &geomSubsets = _topology->GetGeomSubsets();
            const size_t numGeomSubsets = geomSubsets.size();
            for (size_t i = 0; i < numGeomSubsets; ++i) {
                HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItemForGeomSubset(
                        geomSubsetDescIndex, numGeomSubsets, i));
                if (!TF_VERIFY(drawItem)) {
                    continue;
                }
                stRenderParam->DecreaseMaterialTagCount(
                    drawItem->GetMaterialTag());
            }
            geomSubsetDescIndex++;
        }
    }

    stRenderParam->DecreaseRenderTagCount(GetRenderTag());
}

HdMeshTopologySharedPtr
HdStMesh::GetTopology() const
{
    return _topology;
}

static bool
_IsEnabledForceQuadrangulate()
{
    static bool enabled = (TfGetEnvSetting(HD_ENABLE_FORCE_QUADRANGULATE) == 1);
    return enabled;
}

/* static */
bool
HdStMesh::IsEnabledPackedNormals()
{
    static bool enabled = (TfGetEnvSetting(HD_ENABLE_PACKED_NORMALS) == 1);
    return enabled;
}

int
HdStMesh::_GetRefineLevelForDesc(const HdMeshReprDesc &desc) const
{
    if (desc.geomStyle == HdMeshGeomStyleHull         ||
        desc.geomStyle == HdMeshGeomStyleHullEdgeOnly ||
        desc.geomStyle == HdMeshGeomStyleHullEdgeOnSurf) {
        return 0;
    }
    if (!TF_VERIFY(_topology)) return 0;
    return _topology->GetRefineLevel();
}

void 
HdStMesh::_GatherFaceVaryingTopologies(HdSceneDelegate *sceneDelegate,
                                       const HdReprSharedPtr &repr,
                                       const HdMeshReprDesc &desc,
                                       HdStDrawItem *drawItem,
                                       int geomSubsetDescIndex,
                                       HdDirtyBits *dirtyBits,
                                       const SdfPath &id,
                                       HdSt_MeshTopologySharedPtr topology)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdPrimvarDescriptorVector primvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
            HdInterpolationFaceVarying, repr, desc.geomStyle,
                geomSubsetDescIndex, topology->GetGeomSubsets().size());

    if (!primvars.empty()) {
        for (HdPrimvarDescriptor const& primvar: primvars) {
            if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name)) {
                continue;
            }
            const int numFaceVaryings = topology->GetNumFaceVaryings();

            VtValue value;
            VtIntArray indices(0);
            if (primvar.indexed) {
                value = GetIndexedPrimvar(
                    sceneDelegate, primvar.name, &indices);

                if (indices.empty()) {
                    HF_VALIDATION_WARN(id, 
                        "Found empty indices for indexed face-varying primvar "
                        "%s. Skipping indices update.",
                        primvar.name.GetText());
                    continue;
                } else if ((int)indices.size() < numFaceVaryings) {
                    HF_VALIDATION_WARN(id, 
                        "Indices for face-varying primvar %s has only %d " 
                        "elements, while its topology expects at least %d "
                        "elements. Skipping indices update.", 
                        primvar.name.GetText(), (int)indices.size(), 
                        numFaceVaryings);
                    continue;
                }
            } else {
                value = GetPrimvar(sceneDelegate, primvar.name);
                for (int i = 0; i < numFaceVaryings; ++i) {
                    indices.push_back(i);
                }
            }
                        
            _fvarTopologyTracker->AddOrUpdateTopology(primvar.name, indices);
        }
    }

    // Also check for removed primvars
    HdBufferSpecVector removedSpecs;
    if (*dirtyBits & HdChangeTracker::DirtyPrimvar) {
        HdBufferArrayRangeSharedPtr const &bar =
            drawItem->GetFaceVaryingPrimvarRange();
        TfTokenVector internallyGeneratedPrimvars; // empty
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars, 
            internallyGeneratedPrimvars, id);
    }

    for (size_t i = 0; i < removedSpecs.size(); ++i) {
        TfToken const& removedName = removedSpecs[i].name;
        _fvarTopologyTracker->RemovePrimvar(removedName);
    }
    
    _fvarTopologyTracker->RemoveUnusedTopologies();
}

void
HdStMesh::_UpdateDrawItemsForGeomSubsets(HdSceneDelegate *sceneDelegate,
                                         HdRenderParam *renderParam,
                                         HdStDrawItem *drawItem,
                                         const TfToken &reprToken,
                                         const HdReprSharedPtr &repr,
                                         const HdGeomSubsets &geomSubsets,
                                         size_t oldNumGeomSubsets)
{
    HdChangeTracker &changeTracker =
        sceneDelegate->GetRenderIndex().GetChangeTracker();
    
    const size_t numGeomSubsets = geomSubsets.size();
    const int newInstancePvIndex = HdStMesh::FreeSlot + 2 * numGeomSubsets;

    if (numGeomSubsets != oldNumGeomSubsets) {
        // Shift the instance primvars if necessary
        if (drawItem->HasInstancer()) {
            const size_t numInstanceLevels = 
                drawItem->GetInstancePrimvarNumLevels();
            if (numGeomSubsets < oldNumGeomSubsets) {
                // less geom susbets than before
                // move instance primvar levels toward start
                for (size_t i = 0; i < numInstanceLevels; ++i) {
                    HdBufferArrayRangeSharedPtr instancePvRange = 
                        drawItem->GetInstancePrimvarRange(i);
                    HdStUpdateDrawItemBAR(
                        instancePvRange,
                        newInstancePvIndex + i,
                        &_sharedData,
                        renderParam,
                        &changeTracker);
                }
            } else {
                // more geom subsets than before
                // move instance primvar levels toward end
                for (size_t i = numInstanceLevels - 1; i >= 0; --i) {
                    HdBufferArrayRangeSharedPtr instancePvRange = 
                        drawItem->GetInstancePrimvarRange(i);
                    HdStUpdateDrawItemBAR(
                        instancePvRange,
                        newInstancePvIndex + i,
                        &_sharedData,
                        renderParam,
                        &changeTracker);
                }
            }
        }

        // (Re)create geom subset draw items
        for (auto const& reprPair : _reprs) {
            _MeshReprConfig::DescArray descs = _GetReprDesc(reprPair.first);
            HdReprSharedPtr currRepr = reprPair.second;

            // Clear all previous geom subset draw items.
            currRepr->ClearGeomSubsetDrawItems();
            
            if (oldNumGeomSubsets != 0) {
                // Adjust material tag count for removed geom subset draw items.
                HdStRenderParam * const stRenderParam =
                    static_cast<HdStRenderParam*>(renderParam);
                size_t geomSubsetDescIndex = 0;
                for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
                    const HdMeshReprDesc &desc = descs[descIdx];
                    if (desc.geomStyle == HdMeshGeomStyleInvalid ||
                        desc.geomStyle == HdMeshGeomStylePoints) {
                        continue;
                    }

                    for (size_t i = 0; i < oldNumGeomSubsets; ++i) {
                        HdStDrawItem *subsetDrawItem = static_cast<HdStDrawItem*>(
                            currRepr->GetDrawItemForGeomSubset(
                                geomSubsetDescIndex, oldNumGeomSubsets, i));
                        if (!TF_VERIFY(subsetDrawItem)) {
                            continue;
                        }
                        stRenderParam->DecreaseMaterialTagCount(subsetDrawItem->GetMaterialTag());
                    }
                    geomSubsetDescIndex++;
                }
                // Clear all previous geom subset draw items.
                currRepr->ClearGeomSubsetDrawItems();
            }

            int mainDrawItemIndex = 0;
            for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
                const HdMeshReprDesc &desc = descs[descIdx];
                if (desc.geomStyle == HdMeshGeomStyleInvalid) {
                    continue;
                }

                // Update main draw item's instance primvar drawing coord
                HdStDrawItem *mainDrawItem = static_cast<HdStDrawItem*>(
                    currRepr->GetDrawItem(mainDrawItemIndex++));
                mainDrawItem->GetDrawingCoord()->SetInstancePrimvarBaseIndex(
                    newInstancePvIndex);

                // Don't create geom subset draw items for points geom styles
                if (desc.geomStyle == HdMeshGeomStylePoints) {
                    continue;
                }

                for (size_t i = 0; i < numGeomSubsets; ++i) {
                    const HdGeomSubset &geomSubset = geomSubsets[i];

                    std::unique_ptr<HdStDrawItem> subsetDrawItem =
                        std::make_unique<HdStDrawItem>(&_sharedData);
                    subsetDrawItem->SetMaterialNetworkShader(
                        HdStGetMaterialNetworkShader(
                            this, sceneDelegate, geomSubset.materialId));
                    
                    // Each of the geom subset draw items need to have a unique
                    // topology drawing coord
                    HdDrawingCoord * drawingCoord = 
                        subsetDrawItem->GetDrawingCoord();
                    switch (desc.geomStyle) {
                        case HdMeshGeomStyleHull:
                        case HdMeshGeomStyleHullEdgeOnly:
                        case HdMeshGeomStyleHullEdgeOnSurf:
                        {
                            drawingCoord->SetTopologyIndex(
                                HdStMesh::FreeSlot + 2 * i + 1);
                            break;
                        }
                        default:
                        {
                            drawingCoord->SetTopologyIndex(
                                HdStMesh::FreeSlot + 2 * i);
                            break;
                        }
                    }
                    drawingCoord->SetInstancePrimvarBaseIndex(
                        newInstancePvIndex);
                    currRepr->AddGeomSubsetDrawItem(std::move(subsetDrawItem));
                }
            }
        }
        
        // When geom subsets are added or removed, the rprim index version 
        // number will be incremented via another mechanism. The below dirtying
        // is relevant when the number of geom subsets requiring draw items 
        // changes due to another reason e.g. a geom subset had its material id 
        // removed or its indices removed. We expect such cases to be rare.
        HdStMarkGeomSubsetDrawItemsDirty(renderParam);
    } else {
        // If number of geom subsets requiring draw items is the same, but geom
        // subsets have changed, we might need to update their material shaders.
        _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
        size_t geomSubsetDescIndex = 0;
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            const HdMeshReprDesc &desc = descs[descIdx];
            if (desc.geomStyle == HdMeshGeomStyleInvalid ||
                desc.geomStyle == HdMeshGeomStylePoints) {
                continue;
            }

            for (size_t i = 0; i < numGeomSubsets; ++i) {
                const HdGeomSubset &geomSubset = geomSubsets[i];
                HdStDrawItem *subsetDrawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItemForGeomSubset(
                        geomSubsetDescIndex, numGeomSubsets, i));
                if (!TF_VERIFY(subsetDrawItem)) {
                    continue;
                }
                subsetDrawItem->SetMaterialNetworkShader(
                        HdStGetMaterialNetworkShader(
                                this, sceneDelegate, geomSubset.materialId));
            }
            geomSubsetDescIndex++;
        }
    }    
}

void
HdStMesh::_PopulateTopology(HdSceneDelegate *sceneDelegate,
                            HdRenderParam *renderParam,
                            HdStDrawItem *drawItem,
                            HdDirtyBits *dirtyBits,
                            const TfToken &reprToken,
                            const HdReprSharedPtr &repr,
                            const HdMeshReprDesc &desc,
                            int geomSubsetDescIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());
    HdChangeTracker &changeTracker =
        sceneDelegate->GetRenderIndex().GetChangeTracker();

    // note: there's a potential optimization if _topology is already registered
    // and it's not shared across prims, it can be updated without inserting new
    // entry into the topology registry. But in mose case topology varying prim
    // requires range resizing (reallocation), so for code simplicity we always
    // register as a new topology (it still can be shared if possible) and
    // allocate a new range for varying topology (= dirty topology)
    // for the time being. In other words, each range of index buffer is
    // immutable.
    // 
    bool dirtyTopology = HdChangeTracker::IsTopologyDirty(*dirtyBits, id);

    TopologyToPrimvarVector oldFvarTopologies = 
        _fvarTopologyTracker->GetTopologyToPrimvarVector();

    if (dirtyTopology ||
        HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id) ||
        HdChangeTracker::IsSubdivTagsDirty(*dirtyBits, id) ||
        HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        // make a shallow copy and the same time expand the topology to a
        // stream extended representation
        // note: if we add topologyId computation in delegate,
        // we can move this copy into topologyInstance.IsFirstInstance() block
        HdDisplayStyle const displayStyle = GetDisplayStyle(sceneDelegate);

        int refineLevel = displayStyle.refineLevel;
        HdSt_MeshTopology::RefineMode refineMode =
                HdSt_MeshTopology::RefineModeUniform;
        _limitNormals = false;

        _flatShadingEnabled = displayStyle.flatShadingEnabled;
        _displacementEnabled = displayStyle.displacementEnabled;
        _occludedSelectionShowsThrough =
            displayStyle.occludedSelectionShowsThrough;
        _pointsShadingEnabled = displayStyle.pointsShadingEnabled;

        HdMeshTopology meshTopology = HdMesh::GetMeshTopology(sceneDelegate);

        // Topological visibility (of points, faces) comes in as DirtyTopology.
        // We encode this information in a separate BAR.
        if (dirtyTopology) {
            HdStProcessTopologyVisibility(
                meshTopology.GetInvisibleFaces(),
                meshTopology.GetNumFaces(),
                meshTopology.GetInvisiblePoints(),
                meshTopology.GetNumPoints(),
                &_sharedData,
                drawItem,
                renderParam,
                &changeTracker,
                resourceRegistry,
                id);    
        }

        // If flat shading is enabled for this prim, make sure we're computing
        // flat normals. It's ok to set the dirty bit here because it's a
        // custom (non-scene) dirty bit, and DirtyTopology will propagate to
        // DirtyPoints if we're computing CPU normals (since flat normals
        // computation requires points data).
        if (_flatShadingEnabled) {
            if (!(_customDirtyBitsInUse & DirtyFlatNormals)) {
                _customDirtyBitsInUse |= DirtyFlatNormals;
                *dirtyBits |= DirtyFlatNormals;
            }
        }
              
        // If the topology requires none subdivision scheme then force
        // refinement level to be 0 since we do not want subdivision.
        if (meshTopology.GetScheme() == PxOsdOpenSubdivTokens->none) {
            refineLevel = 0;
        }

        // If the topology supports adaptive refinement and that's what this
        // prim wants, note that and also that our normals will be generated
        // in the shader.
        if (meshTopology.GetScheme() != PxOsdOpenSubdivTokens->bilinear &&
            meshTopology.GetScheme() != PxOsdOpenSubdivTokens->none &&
            refineLevel > 0 &&
            _UseLimitRefinement(sceneDelegate->GetRenderIndex(), meshTopology)) {
            refineMode = HdSt_MeshTopology::RefineModePatches;
            _limitNormals = true;
        }

        bool const hasBuiltinBarycentrics =
            resourceRegistry->GetHgi()->GetCapabilities()->
                IsSet(HgiDeviceCapabilitiesBitsBuiltinBarycentrics);

        bool const hasMetalTessellation =
            resourceRegistry->GetHgi()->GetCapabilities()->
                IsSet(HgiDeviceCapabilitiesBitsMetalTessellation);

        HdSt_MeshTopologySharedPtr topology =
            HdSt_MeshTopology::New(meshTopology, refineLevel, refineMode,
                (hasBuiltinBarycentrics || hasMetalTessellation)
                    ? HdSt_MeshTopology::QuadsTriangulated
                    : HdSt_MeshTopology::QuadsUntriangulated);
        
        // Gather and sanitize geom subsets
        const HdGeomSubsets &oldGeomSubsets = _topology ? 
            _topology->GetGeomSubsets() : HdGeomSubsets();
        const HdGeomSubsets &geomSubsets = topology->GetGeomSubsets();
        // This will handle draw item creation/update for all existing reprs.
        if (geomSubsets != oldGeomSubsets) {
            _UpdateDrawItemsForGeomSubsets(sceneDelegate, renderParam, drawItem,
                reprToken, repr, geomSubsets, oldGeomSubsets.size());
        }

        if (refineLevel > 0) {
            // add subdiv tags before compute hash
            topology->SetSubdivTags(GetSubdivTags(sceneDelegate));
        }

        TfToken fvarLinearInterpRule =
            topology->GetSubdivTags().GetFaceVaryingInterpolationRule();

        if ((refineLevel > 0) && 
            (fvarLinearInterpRule != PxOsdOpenSubdivTokens->all) && 
            HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
            _GatherFaceVaryingTopologies(
                sceneDelegate, repr, desc, drawItem, geomSubsetDescIndex, 
                    dirtyBits, id, topology);
            topology->SetFvarTopologies(
                _fvarTopologyTracker->GetFvarTopologies());
            _sharedData.fvarTopologyToPrimvarVector = 
                _fvarTopologyTracker->GetTopologyToPrimvarVector();
        }        
        
        // Compute id here. In the future delegate can provide id directly 
        // without hashing.
        _topologyId = topology->ComputeHash();

        // Salt the hash with face-varying topologies
        for (const auto& it : _fvarTopologyTracker->GetTopologyToPrimvarVector()) {
            _topologyId = ArchHash64((const char*)it.first.cdata(),
                                     sizeof(int) * it.first.size(), 
                                     _topologyId);
            for (const TfToken& it2 : it.second) {
                _topologyId = ArchHash64(it2.GetText(), it2.size(),
                                        _topologyId);
            }
        }

        // Salt the hash with refinement level and useQuadIndices.
        // (refinement level is moved into HdMeshTopology)
        //
        // Specifically for quad indices, we could do better here because all we
        // really need is the ability to compute quad indices late, however
        // splitting the topology shouldn't be a huge cost either.
        bool useQuadIndices = _UseQuadIndices(sceneDelegate->GetRenderIndex(), topology);
        _topologyId = ArchHash64((const char*)&useQuadIndices,
            sizeof(useQuadIndices), _topologyId);

        {
            // ask registry if there's a sharable mesh topology
            HdInstance<HdSt_MeshTopologySharedPtr> topologyInstance =
                resourceRegistry->RegisterMeshTopology(_topologyId);

            if (topologyInstance.IsFirstInstance()) {
                // if this is the first instance, set this topology to registry.
                topologyInstance.SetValue(topology);

                // if refined, we submit a subdivision preprocessing
                // no matter what desc says
                // (see the lengthy comment in PopulateVertexPrimvar)
                if (refineLevel > 0) {
                    // OpenSubdiv preprocessing
                    HdBufferSourceSharedPtr
                        topologySource = topology->GetOsdTopologyComputation(id);
                    resourceRegistry->AddSource(topologySource);
                }

                // we also need quadinfo if requested.
                // Note that this is needed even if refineLevel > 0, in case
                // HdMeshGeomStyleHull is going to be used.
                if (useQuadIndices) {
                    // Quadrangulate preprocessing
                    HdSt_QuadInfoBuilderComputationSharedPtr quadInfoBuilder =
                        topology->GetQuadInfoBuilderComputation(
                            /*GPU*/ true, id, resourceRegistry.get());
                    resourceRegistry->AddSource(quadInfoBuilder);
                }
            }
            _topology = topologyInstance.GetValue();
        }
        TF_VERIFY(_topology);

        // hash collision check
        if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
            TF_VERIFY(*topology == *_topology);
        }

        _vertexAdjacencyBuilder.reset();
    }

    // here, we have _topology up-to-date.

    int refineLevelForDesc = _GetRefineLevelForDesc(desc);
    TfToken indexToken;  // bar-instance identifier

    // It's possible for topology to not be dirty but a face-varying topology is
    const bool canSkipFvarTopologyComp = (refineLevelForDesc == 0) || 
        (_topology->GetSubdivTags().GetFaceVaryingInterpolationRule() == 
        PxOsdOpenSubdivTokens->all) || 
        (!HdChangeTracker::IsDisplayStyleDirty(*dirtyBits, id) && 
        oldFvarTopologies ==  
            _fvarTopologyTracker->GetTopologyToPrimvarVector());

    // bail out if the index bar is already synced
    if (drawItem->GetDrawingCoord()->GetTopologyIndex() == 
        HdStMesh::HullTopology) {
        if ((*dirtyBits & DirtyHullIndices) == 0 && canSkipFvarTopologyComp) {
            return;
        }
        *dirtyBits &= ~DirtyHullIndices;
        indexToken = HdTokens->hullIndices;
    } else if (drawItem->GetDrawingCoord()->GetTopologyIndex() == 
        HdStMesh::PointsTopology) {
        if ((*dirtyBits & DirtyPointsIndices) == 0 && canSkipFvarTopologyComp) {
            return;
        }
        *dirtyBits &= ~DirtyPointsIndices;
        indexToken = HdTokens->pointsIndices;
    } else {
        if ((*dirtyBits & DirtyIndices) == 0 && canSkipFvarTopologyComp) { 
            return;
        }
        *dirtyBits &= ~DirtyIndices;
        indexToken = HdTokens->indices;
    }

    // note: don't early out even if the topology has no faces,
    // otherwise codegen takes inconsistent configuration and
    // fails to compile ( or even segfaults: filed as nvidia-bug 1719609 )

    {
        const HdGeomSubsets &geomSubsets = _topology->GetGeomSubsets();

        // Normal case
        if (geomSubsets.empty() || desc.geomStyle == HdMeshGeomStylePoints) {

            // ask again registry if there's a shareable buffer range for the topology
            HdInstance<HdBufferArrayRangeSharedPtr> rangeInstance =
                resourceRegistry->RegisterMeshIndexRange(_topologyId, indexToken);

            if (rangeInstance.IsFirstInstance()) {
                // if not exists, update actual topology buffer to range.
                // Allocate new one if necessary.
                HdBufferSourceSharedPtrVector sources;
                HdBufferSourceSharedPtr source;

                if (desc.geomStyle == HdMeshGeomStylePoints) {
                    // create coarse points indices
                    source = _topology->GetPointsIndexBuilderComputation();
                    sources.push_back(source);
                } else if (refineLevelForDesc > 0) {
                    // create refined indices, primitiveParam and edgeIndices
                    source = _topology->GetOsdIndexBuilderComputation();
                    sources.push_back(source);

                    // Add face-varying indices and patch params to topology BAR if 
                    // necessary
                    if (_topology->GetSubdivTags().GetFaceVaryingInterpolationRule() !=
                        PxOsdOpenSubdivTokens->all) {
                        for (size_t i = 0; 
                            i < _fvarTopologyTracker->GetNumTopologies(); 
                            ++i) {
                            HdBufferSourceSharedPtr fvarIndicesSource = 
                                _topology->GetOsdFvarIndexBuilderComputation(i);
                            sources.push_back(fvarIndicesSource);
                        }
                    }
                } else if (_UseQuadIndices(sceneDelegate->GetRenderIndex(), _topology)) {
                    // not refined = quadrangulate
                    // create quad indices, primitiveParam and edgeIndices
                    source = _topology->GetQuadIndexBuilderComputation(GetId());
                    sources.push_back(source);

                } else {
                    // create triangle indices, primitiveParam and edgeIndices
                    source = _topology->GetTriangleIndexBuilderComputation(GetId());
                    sources.push_back(source);  
                }
                
                // initialize buffer array
                //   * indices
                //   * primitiveParam
                //   * fvarIndices (optional)
                //   * fvarPatchParam (optional)
                HdBufferSpecVector bufferSpecs;
                HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

                // Set up the usage hints to mark topology as varying if
                // there is a previously set range
                HdBufferArrayUsageHint usageHint = 
                    HdBufferArrayUsageHintBitsIndex |
                    HdBufferArrayUsageHintBitsStorage;
                if (drawItem->GetTopologyRange()) {
                    usageHint |= HdBufferArrayUsageHintBitsSizeVarying;
                }

                // allocate new range
                HdBufferArrayRangeSharedPtr range =
                    resourceRegistry->AllocateNonUniformBufferArrayRange(
                        HdTokens->topology, bufferSpecs, usageHint);

                // add sources to update queue
                resourceRegistry->AddSources(range, std::move(sources));

                // save new range to registry
                rangeInstance.SetValue(range);
            }

            // If we are updating an existing topology, notify downstream
            // systems of the change
            HdBufferArrayRangeSharedPtr const& orgRange =
                drawItem->GetTopologyRange();
            HdBufferArrayRangeSharedPtr newRange = rangeInstance.GetValue();

            if (HdStIsValidBAR(orgRange) && (newRange != orgRange)) {
                TF_DEBUG(HD_RPRIM_UPDATED).Msg("%s has varying topology"
                    " (topology index = %d).\n", id.GetText(),
                    drawItem->GetDrawingCoord()->GetTopologyIndex());
                
                // Setup a flag to say this mesh's topology is varying
                _hasVaryingTopology = true;
            }

            HdStUpdateDrawItemBAR(
                newRange,
                drawItem->GetDrawingCoord()->GetTopologyIndex(),
                &_sharedData,
                renderParam,
                &changeTracker);
        } else {
            // Geom subsets case
            HdBufferSourceSharedPtr indicesSource;
            HdBufferSourceSharedPtr fvarIndicesSource;

            bool refined = false;
            bool quadrangulated = false;
            if (refineLevelForDesc > 0) {
                // create refined indices, primitiveParam and edgeIndices
                indicesSource = _topology->GetOsdIndexBuilderComputation();
                resourceRegistry->AddSource(indicesSource);
                // Add face-varying indices and patch params to topology BAR if 
                // necessary
                if (_topology->GetSubdivTags().GetFaceVaryingInterpolationRule() !=
                    PxOsdOpenSubdivTokens->all) {
                    for (size_t i = 0; 
                           i < _fvarTopologyTracker->GetNumTopologies(); 
                            ++i) {
                        fvarIndicesSource = 
                            _topology->GetOsdFvarIndexBuilderComputation(i);
                        resourceRegistry->AddSource(fvarIndicesSource);
                    }
                }

                refined = true;
                if (_topology->GetScheme() == PxOsdOpenSubdivTokens->catmullClark ||
                    _topology->GetScheme() == PxOsdOpenSubdivTokens->bilinear) {
                    quadrangulated = true;
                }
            } else if (_UseQuadIndices(sceneDelegate->GetRenderIndex(), _topology)) {
                // not refined = quadrangulate
                // create quad indices, primitiveParam and edgeIndices
                indicesSource = _topology->GetQuadIndexBuilderComputation(GetId());
                resourceRegistry->AddSource(indicesSource);
                quadrangulated = true;
            } else {
                // create triangle indices, primitiveParam and edgeIndices
                indicesSource = _topology->GetTriangleIndexBuilderComputation(GetId());
                resourceRegistry->AddSource(indicesSource);
            }

            // If the mesh has been triangulated, quadrangulated, or refined (as 
            // refined indices are first triangulated or quadrangulated), we 
            // need to transform the subset's authored face indices, which are 
            // given in reference to the base faces of the mesh, to the indices
            // of the triangulated/quadrangulated faces. These buffer source
            // computations help us do that.
            HdBufferSourceSharedPtr geomSubsetFaceIndicesHelperSource = 
                _topology->GetGeomSubsetFaceIndexHelperComputation(
                    refined, quadrangulated);
            resourceRegistry->AddSource(geomSubsetFaceIndicesHelperSource);

            if (refined) {
                _topology->GetOsdBaseFaceToRefinedFacesMapComputation(
                    resourceRegistry.get());
            }

            // For original draw item
            const std::vector<int> *nonSubsetFaces = 
                _topology->GetNonSubsetFaces();
            _CreateTopologyRangeForGeomSubset(resourceRegistry, changeTracker, 
                renderParam, drawItem, indexToken, indicesSource,
                fvarIndicesSource, geomSubsetFaceIndicesHelperSource,
                VtIntArray(nonSubsetFaces->begin(), nonSubsetFaces->end()), 
                refined);

            // For geom subsets draw items
            const size_t numGeomSubsets = geomSubsets.size();
            for (size_t i = 0; i < geomSubsets.size(); ++i) {
                HdGeomSubset geomSubset = geomSubsets[i];
                HdStDrawItem *subsetDrawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItemForGeomSubset(
                        geomSubsetDescIndex, numGeomSubsets, i));
                _CreateTopologyRangeForGeomSubset(resourceRegistry, 
                    changeTracker, renderParam, subsetDrawItem, indexToken, 
                    indicesSource, fvarIndicesSource, 
                    geomSubsetFaceIndicesHelperSource, geomSubset.indices, 
                    refined);
            }

        }
    } // Release regLock
}

void HdStMesh::_CreateTopologyRangeForGeomSubset(
    HdStResourceRegistrySharedPtr resourceRegistry,
    HdChangeTracker &changeTracker, 
    HdRenderParam *renderParam, 
    HdStDrawItem *drawItem, 
    const TfToken &indexToken,
    HdBufferSourceSharedPtr indicesSource, 
    HdBufferSourceSharedPtr fvarIndicesSource, 
    HdBufferSourceSharedPtr geomSubsetFaceIndicesHelperSource,
    const VtIntArray &faceIndices,
    bool refined)
{
    HdTopology::ID subsetTopologyId = ArchHash64(
        (const char*)faceIndices.cdata(),
        sizeof(int)*faceIndices.size(), _topologyId);

    // ask registry if there's a shareable buffer range for the topology
    HdInstance<HdBufferArrayRangeSharedPtr> rangeInstance =
        resourceRegistry->RegisterMeshIndexRange(subsetTopologyId, indexToken);

    if (rangeInstance.IsFirstInstance()) {
        // if not exists, update actual topology buffer to range.
        // Allocate new one if necessary.
        HdBufferSourceSharedPtrVector sources;

        HdBufferSourceSharedPtr geomSubsetFaceIndicesSource = 
            _topology->GetGeomSubsetFaceIndexBuilderComputation(
                geomSubsetFaceIndicesHelperSource, faceIndices);

        if (refined) {
            resourceRegistry->AddSource(geomSubsetFaceIndicesSource);

            HdBufferSourceSharedPtr subsetSource = 
                _topology->GetRefinedIndexSubsetComputation(
                    indicesSource, geomSubsetFaceIndicesSource);
            sources.push_back(subsetSource);

            if (fvarIndicesSource) {
                HdBufferSourceSharedPtr fvarSubsetSource = 
                    _topology->GetRefinedIndexSubsetComputation(
                        fvarIndicesSource, geomSubsetFaceIndicesSource);
                sources.push_back(fvarSubsetSource);
            }
        } else {
            HdBufferSourceSharedPtr subsetSource = 
                _topology->GetIndexSubsetComputation(
                    indicesSource, geomSubsetFaceIndicesSource);
            sources.push_back(subsetSource);

            // This source also becomes the face index for coarse 
            // triangles/quads (instead of gl_PrimitiveId).
            sources.push_back(geomSubsetFaceIndicesSource);
        }

        // initialize buffer array
        //   * indices
        //   * primitiveParam
        //   * fvarIndices (optional)
        //   * fvarPatchParam (optional)
        HdBufferSpecVector bufferSpecs;
        HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);

        // Set up the usage hints to mark topology as varying if there is a 
        // previously set range
        HdBufferArrayUsageHint usageHint =
            HdBufferArrayUsageHintBitsIndex | HdBufferArrayUsageHintBitsStorage;
        if (drawItem->GetTopologyRange()) {
            usageHint |= HdBufferArrayUsageHintBitsSizeVarying;
        }

        // allocate new range
        HdBufferArrayRangeSharedPtr range =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs, usageHint);

        // add sources to update queue
        resourceRegistry->AddSources(range, std::move(sources));

        // save new range to registry
        rangeInstance.SetValue(range);
    } 

    // If we are updating an existing topology, notify downstream systems of the
    // change
    HdBufferArrayRangeSharedPtr const& orgRange = drawItem->GetTopologyRange();
    HdBufferArrayRangeSharedPtr newRange = rangeInstance.GetValue();

    if (HdStIsValidBAR(orgRange) && (newRange != orgRange)) {
        TF_DEBUG(HD_RPRIM_UPDATED).Msg("%s has varying topology"
            " (topology index = %d).\n", GetId().GetText(),
                drawItem->GetDrawingCoord()->GetTopologyIndex());
                    
        // Setup a flag to say this mesh's topology is varying
        _hasVaryingTopology = true;
    }

    HdStUpdateDrawItemBAR(
        newRange,
        drawItem->GetDrawingCoord()->GetTopologyIndex(),
        &_sharedData,
        renderParam,
        &changeTracker);
}

void
HdStMesh::_PopulateAdjacency(
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // The topology may be null in the event that it has zero faces.
    if (!_topology) return;

    // ask registry if there's a sharable vertex adjacency
    HdInstance<HdSt_VertexAdjacencyBuilderSharedPtr>
        vertexAdjacencyBuilderInstance =
            resourceRegistry->RegisterVertexAdjacencyBuilder(_topologyId);

    if (vertexAdjacencyBuilderInstance.IsFirstInstance()) {
         HdSt_VertexAdjacencyBuilderSharedPtr vertexAdjacencyBuilder =
             std::make_shared<HdSt_VertexAdjacencyBuilder>();

        // create adjacency table for smooth normals
        HdBufferSourceSharedPtr vertexAdjacencyComputation =
            vertexAdjacencyBuilder->
                GetSharedVertexAdjacencyBuilderComputation(_topology.get());

        resourceRegistry->AddSource(vertexAdjacencyComputation);

        // also send adjacency table to gpu
        HdBufferSourceSharedPtr vertexAdjacencyBufferSource =
            std::make_shared<HdSt_VertexAdjacencyBufferSource>(
                vertexAdjacencyBuilder->GetVertexAdjacency(),
                vertexAdjacencyComputation);

        HdBufferSpecVector bufferSpecs;
        vertexAdjacencyBufferSource->GetBufferSpecs(&bufferSpecs);

        HdBufferArrayRangeSharedPtr vertexAdjacencyRange =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs,
                HdBufferArrayUsageHintBitsStorage);

        vertexAdjacencyBuilder->SetVertexAdjacencyRange(vertexAdjacencyRange);
        resourceRegistry->AddSource(vertexAdjacencyRange,
                                    vertexAdjacencyBufferSource);

        vertexAdjacencyBuilderInstance.SetValue(vertexAdjacencyBuilder);
    }
    _vertexAdjacencyBuilder = vertexAdjacencyBuilderInstance.GetValue();
}


// Enqueues a quadrangulation computation to 'computations' for the primvar data
// in 'source',
static void
_QuadrangulatePrimvar(HdBufferSourceSharedPtr const &source,
                      HdSt_MeshTopologySharedPtr const &topology,
                      SdfPath const &id,
                      HdStComputationComputeQueuePairVector *computations)
{
    if (!TF_VERIFY(computations)) {
        return;
    }
    // GPU quadrangulation computation needs original vertices to be transfered
    HdStComputationSharedPtr computation =
        topology->GetQuadrangulateComputationGPU(
            source->GetName(), source->GetTupleType().type, id);
    // computation can be null for all quad mesh.
    if (computation) {
        computations->emplace_back(computation, _RefinePrimvarCompQueue);
    }
}

static HdBufferSourceSharedPtr
_QuadrangulateFaceVaryingPrimvar(
    HdBufferSourceSharedPtr const &source,
    HdSt_MeshTopologySharedPtr const &topology,
    SdfPath const &id,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    // note: currently we don't support GPU facevarying quadrangulation.

    // set quadrangulation as source instead of original source.
    HdBufferSourceSharedPtr quadSource =
        topology->GetQuadrangulateFaceVaryingComputation(source, id);

    // don't transfer source to gpu, it needs to be quadrangulated.
    // but it still has to be resolved, so add it to registry.
    resourceRegistry->AddSource(source);

    return quadSource;
}

static HdBufferSourceSharedPtr
_TriangulateFaceVaryingPrimvar(HdBufferSourceSharedPtr const &source,
                               HdSt_MeshTopologySharedPtr const &topology,
                               SdfPath const &id,
                               HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    HdBufferSourceSharedPtr triSource =
        topology->GetTriangulateFaceVaryingComputation(source, id);

    // don't transfer source to gpu, it needs to be triangulated.
    // but it still has to be resolved, so add it to registry.
    resourceRegistry->AddSource(source);

    return triSource;
}

// Enqueues a refinement computation to 'computations' for the primvar data
// in 'source',
static void
_RefinePrimvar(HdBufferSourceSharedPtr const &source,
               HdSt_MeshTopologySharedPtr const &topology,
               HdStResourceRegistrySharedPtr const &resourceRegistry,
               HdStComputationComputeQueuePairVector *computations,
               HdSt_MeshTopology::Interpolation interpolation,
               int channel = 0)
{
    if (!TF_VERIFY(computations)) {
        return;
    }
    // GPU subdivision
    HdStComputationSharedPtr computation =
        topology->GetOsdRefineComputationGPU(
            source->GetName(),
            source->GetTupleType().type, 
            resourceRegistry.get(),
            interpolation,
            channel);
    // computation can be null for empty mesh
    if (computation) {
        computations->emplace_back(computation, _RefinePrimvarCompQueue);
    }
}

static void
_RefineOrQuadrangulateVertexAndVaryingPrimvar(
    HdBufferSourceSharedPtr const &source,
    HdSt_MeshTopologySharedPtr const &topology,
    SdfPath const &id,
    bool doRefine,
    bool doQuadrangulate,
    HdStResourceRegistrySharedPtr const &resourceRegistry,
    HdStComputationComputeQueuePairVector *computations,
    HdSt_MeshTopology::Interpolation interpolation)
{
    if (doRefine) {
        _RefinePrimvar(source, topology,
                       resourceRegistry, computations, interpolation);
    } else if (doQuadrangulate) {
        _QuadrangulatePrimvar(source, topology, id, computations);
    } 
}

static HdBufferSourceSharedPtr
_RefineOrQuadrangulateOrTriangulateFaceVaryingPrimvar(
    HdBufferSourceSharedPtr source,
    HdSt_MeshTopologySharedPtr const &topology,
    SdfPath const &id,
    bool doRefine,
    bool doQuadrangulate,
    HdStResourceRegistrySharedPtr const &resourceRegistry,
    HdStComputationComputeQueuePairVector *computations,
    int channel)
{
    //
    // XXX: there is a bug of quad and tris confusion. see bug 121414
    //
    if (doRefine) {
        _RefinePrimvar(source, topology, resourceRegistry, computations, 
                       HdSt_MeshTopology::INTERPOLATE_FACEVARYING, channel);
    } else if (doQuadrangulate) {
        source = _QuadrangulateFaceVaryingPrimvar(source, topology, id, 
                                                  resourceRegistry);
    } else {
        source = _TriangulateFaceVaryingPrimvar(source, topology, id, 
                                                resourceRegistry);
    }

    return source;
}

static bool
_GetDoubleSupport(
    const HdStResourceRegistrySharedPtr& resourceRegistry)
{
    const HgiCapabilities* capabilities =
        resourceRegistry->GetHgi()->GetCapabilities();
    return capabilities->IsSet(HgiDeviceCapabilitiesBitsShaderDoublePrecision);
}

void
HdStMesh::_PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                  HdRenderParam *renderParam,
                                  const HdReprSharedPtr &repr,
                                  const HdMeshReprDesc &desc,
                                  HdStDrawItem *drawItem,
                                  int geomSubsetDescIndex,
                                  HdDirtyBits *dirtyBits,
                                  bool requireSmoothNormals)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    HdStResourceRegistrySharedPtr const &resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
        renderIndex.GetResourceRegistry());

    // The "points" attribute is expected to be in this list.
    HdPrimvarDescriptorVector primvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
            HdInterpolationVertex, repr, desc.geomStyle, geomSubsetDescIndex,
                _topology->GetGeomSubsets().size());

    // Track the last vertex index to distinguish between vertex and varying
    // while processing.
    const int vertexPartitionIndex = int(primvars.size()-1);

    // Add varying primvars so we can process them all together, below.
    HdPrimvarDescriptorVector varyingPvs =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
            HdInterpolationVarying, repr, desc.geomStyle, geomSubsetDescIndex,
                _topology->GetGeomSubsets().size());
    primvars.insert(primvars.end(), varyingPvs.begin(), varyingPvs.end());

    HdExtComputationPrimvarDescriptorVector compPrimvars =
        sceneDelegate->GetExtComputationPrimvarDescriptors(id,
            HdInterpolationVertex);

    HdBufferSourceSharedPtrVector sources;
    HdBufferSourceSharedPtrVector reserveOnlySources;
    HdBufferSourceSharedPtrVector separateComputationSources;
    HdStComputationComputeQueuePairVector computations;
    sources.reserve(primvars.size());

    int numPoints = _topology ? _topology->GetNumPoints() : 0;
    int refineLevel = _topology ? _topology->GetRefineLevel() : 0;

    // Don't call _GetRefineLevelForDesc(desc) instead of GetRefineLevel(). Why?
    //
    // We share the vertex BAR from both refined and hull topologies so that
    // the change tracker doesn't have to keep track the refined primvars.
    //
    // The hull topology refers coarse vertices that are placed on the beginning
    // of the vertex bar (this is a nature of OpenSubdiv adaptive/uniform
    // refinement). The refined topology refers entire vertex bar.
    //
    // If we only update the coarse vertices for the hull repr, and if we also
    // have a refined repr which stucks in an old state, DirtyPoints gets
    // cleared/ just updating coarse vertices and we lost a chance of updating
    // refined primvars. This state discrepancy could happen over frame, so
    // somebody has to maintain the versioning of each buffers.
    //
    // For topology, _indicesValid and _hullIndicesValid are used for that
    // purpose and it's possible because mesh topology is cached and shared in
    // the instance registry. We don't need to ask sceneDelegate, thus
    // individual (hull and refined) change trackings aren't needed.
    //
    // For vertex primvars, here we simply force to update all vertices at the
    // prim's authored refine level. Then both hull and refined topology can
    // safely access all valid data without having separate change tracking.
    //
    // This could be a performance concern, where a prim has higher refine level
    // and a Storm client keeps drawing only hull repr for some reason.
    // Currently we assume it's not likely a use-case, but we may revisit later
    // and optimize if necessary.
    //

    HdSt_GetExtComputationPrimvarsComputations(
        id,
        sceneDelegate,
        compPrimvars,
        *dirtyBits,
        &sources,
        &reserveOnlySources,
        &separateComputationSources,
        &computations);
    
    bool isPointsComputedPrimvar = false;
    {
        // Update tracked state for points and normals that are computed.
        for (HdBufferSourceSharedPtrVector const& computedSources :
             {reserveOnlySources, sources}) {
            for (HdBufferSourceSharedPtr const& source: computedSources) {
                if (source->GetName() == HdTokens->points) {
                    isPointsComputedPrimvar = true;
                    _pointsDataType = source->GetTupleType().type;
                }
                if (source->GetName() == HdTokens->normals) {
                    _sceneNormalsInterpolation = HdInterpolationVertex;
                    _sceneNormals = true;
                }
            }
        }
    }
    
    const bool doRefine = (refineLevel > 0);
    const bool doQuadrangulate = _UseQuadIndices(renderIndex, _topology);

    {
        for (HdBufferSourceSharedPtr const & source : reserveOnlySources) {
            _RefineOrQuadrangulateVertexAndVaryingPrimvar(
                source, _topology, id,  doRefine, doQuadrangulate,
                resourceRegistry,
                &computations, HdSt_MeshTopology::INTERPOLATE_VERTEX);
        }

        for (HdBufferSourceSharedPtr const & source : sources) {
            _RefineOrQuadrangulateVertexAndVaryingPrimvar(
                source, _topology, id,  doRefine, doQuadrangulate,
                resourceRegistry,
                &computations, HdSt_MeshTopology::INTERPOLATE_VERTEX);
        }
    }

    // Track primvars that are skipped because they have zero elements
    HdPrimvarDescriptorVector zeroElementPrimvars;

    // If any primvars use doubles, we need to know if the Hgi backend supports
    // these, or if they need to be converted to floats.
    const bool doublesSupported = _GetDoubleSupport(resourceRegistry);

    // Track index to identify varying primvars.
    int i = 0;
    for (HdPrimvarDescriptor const& primvar: primvars) {
        // If the index is greater than the last vertex index, isVarying=true.
        bool isVarying = i++ > vertexPartitionIndex;

        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name)) {
            continue;
        }

        // TODO: We don't need to pull primvar metadata every time a
        // value changes, but we need support from the delegate.

        VtValue value =  GetPrimvar(sceneDelegate, primvar.name);

        if (!value.IsEmpty()) {
            HdBufferSourceSharedPtr source =
                std::make_shared<HdVtBufferSource>(primvar.name, value, 1,
                                                   doublesSupported);

            if (source->GetNumElements() == 0 &&
                source->GetName() != HdTokens->points) {
                // zero elements for primvars other than points will be treated
                // as if the primvar doesn't exist, so no warning is necessary
                zeroElementPrimvars.push_back(primvar);
                continue;
            }

            // verify primvar length -- it is alright to have more data than we
            // index into; the inverse is when we issue a warning and skip
            // update.
            if ((int)source->GetNumElements() < numPoints) {
                HF_VALIDATION_WARN(id, 
                    "Vertex primvar %s has only %d elements, while"
                    " its topology expects at least %d elements. Skipping "
                    " primvar update.",
                    primvar.name.GetText(),
                    (int)source->GetNumElements(), numPoints);

                if (primvar.name == HdTokens->points) {
                    // If points data is invalid, it pretty much invalidates
                    // the whole prim.  Drop the Bar, to invalidate the prim and
                    // stop further processing.
                    _sharedData.barContainer.Set(
                           drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(),
                           HdBufferArrayRangeSharedPtr());

                    HF_VALIDATION_WARN(id, 
                      "Skipping prim because its points data is insufficient.");

                    return;
                }

                continue;

            } else if ((int)source->GetNumElements() > numPoints) {
                HF_VALIDATION_WARN(id,
                    "Vertex primvar %s has %d elements, while"
                    " its topology references only upto element index %d.",
                    primvar.name.GetText(),
                    (int)source->GetNumElements(), numPoints);

                // If the primvar has more data than needed, we issue a warning,
                // but don't skip the primvar update. Truncate the buffer to
                // the expected length.
                std::static_pointer_cast<HdVtBufferSource>(source)
                    ->Truncate(numPoints);
            }

            if (source->GetName() == HdTokens->normals) {
                _sceneNormalsInterpolation =
                    isVarying ? HdInterpolationVarying : HdInterpolationVertex;
                _sceneNormals = true;
            } else if (source->GetName() == HdTokens->displayOpacity) {
                _displayOpacity = true;
            }

            // Special handling of points primvar.
            // We need to capture state about the points primvar
            // for use with smooth normal computation.
            if (primvar.name == HdTokens->points) {
                if (!TF_VERIFY(!isPointsComputedPrimvar)) {
                    HF_VALIDATION_WARN(id, 
                        "'points' specified as both computed and authored "
                        "primvar. Skipping authored value.");
                    continue;
                }
                _pointsDataType = source->GetTupleType().type;
            }

            _RefineOrQuadrangulateVertexAndVaryingPrimvar(
                source, _topology, id,  doRefine, doQuadrangulate,
                resourceRegistry,
                &computations, isVarying ? 
                    HdSt_MeshTopology::INTERPOLATE_VARYING : 
                    HdSt_MeshTopology::INTERPOLATE_VERTEX);

            sources.push_back(source);
        }
    }

    // remove the primvars with zero elements from further processing
    for (HdPrimvarDescriptor const& primvar: zeroElementPrimvars) {
        auto pos = std::find(primvars.begin(), primvars.end(), primvar);
        if (pos != primvars.end()) {
            primvars.erase(pos);
        }
    }

    TfToken generatedNormalsName;
    if (requireSmoothNormals && (*dirtyBits & DirtySmoothNormals)) {
        // note: normals gets dirty when points are marked as dirty,
        // at changetracker.

        // clear DirtySmoothNormals (this is not a scene dirtybit)
        *dirtyBits &= ~DirtySmoothNormals;

        TF_VERIFY(_vertexAdjacencyBuilder);

        // we can't use packed normals for refined/quad,
        // let's migrate the buffer to full precision
        bool usePackedSmoothNormals =
            IsEnabledPackedNormals() && !(doRefine || doQuadrangulate);

        generatedNormalsName = usePackedSmoothNormals ? 
            HdStTokens->packedSmoothNormals : HdStTokens->smoothNormals;
        
        if (_pointsDataType != HdTypeInvalid) {
            // Smooth normals will compute normals as the same datatype
            // as points, unless we ask for packed normals.
            // This is unfortunate; can we force them to be float?
            HdStComputationSharedPtr smoothNormalsComputation =
                std::make_shared<HdSt_SmoothNormalsComputationGPU>(
                    _vertexAdjacencyBuilder.get(),
                    HdTokens->points,
                    generatedNormalsName,
                    _pointsDataType,
                    usePackedSmoothNormals);
            computations.emplace_back(
                smoothNormalsComputation, _NormalsCompQueue);

            // note: we haven't had explicit dependency for GPU
            // computations just yet. Currently they are executed
            // sequentially, so the dependency is expressed by
            // registering order.
            //
            // note: we can use "pointsDataType" as the normals data type
            // because, if we decided to refine/quadrangulate, we will have
            // forced unpacked normals.
            if (doRefine) {
                HdStComputationSharedPtr computation =
                    _topology->GetOsdRefineComputationGPU(
                        HdStTokens->smoothNormals, _pointsDataType,
                        resourceRegistry.get(),
                        HdSt_MeshTopology::INTERPOLATE_VERTEX);

                // computation can be null for empty mesh
                if (computation) {
                    computations.emplace_back(
                        computation, _RefineNormalsCompQueue);
                }
            } else if (doQuadrangulate) {
                HdStComputationSharedPtr computation =
                    _topology->GetQuadrangulateComputationGPU(
                        HdStTokens->smoothNormals,
                        _pointsDataType, GetId());

                // computation can be null for all-quad mesh
                if (computation) {
                    computations.emplace_back(
                        computation, _RefineNormalsCompQueue);
                }
            }
        }
    }

    HdBufferArrayRangeSharedPtr const &bar = drawItem->GetVertexPrimvarRange();
    
    if (HdStCanSkipBARAllocationOrUpdate(
            sources, computations, bar,*dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        // If we've just generated normals then make sure those
        // are preserved, otherwise allow either previously existing
        // packed or non-packed normals to remain.
        TfTokenVector internallyGeneratedPrimvars;
        if (! generatedNormalsName.IsEmpty()) {
            internallyGeneratedPrimvars = { generatedNormalsName };
        } else {
            internallyGeneratedPrimvars =
                { HdStTokens->packedSmoothNormals, HdStTokens->smoothNormals };
        }

        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars,
            compPrimvars, internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    HdBufferSpec::GetBufferSpecs(reserveOnlySources, &bufferSpecs);
    HdStGetBufferSpecsFromCompuations(computations, &bufferSpecs);

    HdBufferSourceSharedPtrVector allSources(sources);
    for (HdBufferSourceSharedPtr& src : reserveOnlySources) {
        allSources.emplace_back(src);
    }

    HdBufferArrayRangeSharedPtr range;

    if (HdStIsEnabledSharedVertexPrimvar()) {
        // When primvar sharing is enabled, we have the following scenarios:
        // (a) BAR hasn't been allocated,
        //    - See if an existing immutable BAR may be shared.
        //    - If one cannot be found, allocate an immutable BAR and register
        //      it, so other prims may benefit from sharing it.
        //
        // (b) BAR has been allocated and is immutable
        //    (b1) If the topology is varying OR data in the existing buffers is
        //      changing (e.g. points are being updated) OR if primvar(s) were
        //      removed, it is expensive to recompute a hash over the contents
        //      to check if an existing immutable BAR may be shared.
        //          - Transition to a mutable BAR by migrating existing contents
        //      
        //    (b2) Else if we have new sources (e.g smoothNormals), follow 
        //      the same steps as in (a) to use/allocate an immutable BAR.
        //          - This is done to avoid transitioning to a mutable BAR (and 
        //            thus prevent sharing) when changing reprs. This also
        //            handles authored primvars that were added, which may not
        //            be something we want.
        //    
        //    (b3) No new sources: Use the existing BAR.
        //          
        // (c) BAR has been allocated and is mutable
        //    - This means we transitioned to a mutable BAR (b1) earlier, and
        //      can handle it as though primvar sharing wasn't enabled.

        // (a)
        if (!HdStIsValidBAR(bar)) {
            // see if we can share an immutable primvar range
            // include topology and other topological computations
            // in the sharing id so that we can take into account
            // sharing of computed primvar data.
            _vertexPrimvarId = HdStComputeSharedPrimvarId(
                _topologyId, allSources, computations);
            
            bool isFirstInstance = true;
            range = _GetSharedPrimvarRange(_vertexPrimvarId,
                                           /*updatedOrAddedSpecs*/bufferSpecs,
                                           /*removedSpecs*/removedSpecs,
                                           /*curRange*/bar,
                                           &isFirstInstance,
                                           resourceRegistry);
            if (!isFirstInstance) {
                TF_DEBUG(HD_RPRIM_UPDATED).Msg(
                    "%s: Found an immutable BAR (%p) for sharing.\n",
                    id.GetText(), (void*)range.get());

                // this is not the first instance, skip redundant
                // sources and computations.
                sources.clear();
                computations.clear();
            } else {
                TF_DEBUG(HD_RPRIM_UPDATED).Msg(
                    "%s: Allocated an immutable BAR (%p).\n",
                    id.GetText(), (void*)range.get());
            }
        } else {
            if (bar->IsImmutable()) {
                HdBufferSpecVector barSpecs;
                bar->GetBufferSpecs(&barSpecs);

                bool updatingExistingBuffers =
                    !bufferSpecs.empty() &&
                    HdBufferSpec::IsSubset(bufferSpecs, /*superSet*/barSpecs);
                bool notNewRepr = !(*dirtyBits & HdChangeTracker::NewRepr);

                bool transitionToMutableBAR =
                    _hasVaryingTopology ||
                    (updatingExistingBuffers && notNewRepr) ||
                    !removedSpecs.empty();
                
                if (transitionToMutableBAR) {
                    // (b1)
                    HdBufferArrayUsageHint newUsageHint = bar->GetUsageHint();
                    newUsageHint &= ~HdBufferArrayUsageHintBitsImmutable;
                    _vertexPrimvarId = 0;

                    range = resourceRegistry->UpdateNonUniformBufferArrayRange(
                        HdTokens->primvar, bar, bufferSpecs, removedSpecs,
                        newUsageHint);
                    
                    TF_DEBUG(HD_RPRIM_UPDATED).Msg(
                        "Transitioning from immutable to mutable BAR\n");

                } else if (!bufferSpecs.empty()) {
                    // (b2) Continue to use an immutable BAR (even if it means
                    // allocating a new one)

                    // See if we can share an immutable buffer primvar range
                    // include our existing sharing id so that we can take
                    // into account previously committed sources along
                    // with our new sources and computations.
                    _vertexPrimvarId = HdStComputeSharedPrimvarId(
                        _vertexPrimvarId, allSources, computations);

                    bool isFirstInstance = true;
                    range = _GetSharedPrimvarRange(_vertexPrimvarId,
                                           /*updatedOrAddedSpecs*/bufferSpecs,
                                           /*removedSpecs*/removedSpecs,
                                           /*curRange*/bar,
                                           &isFirstInstance,
                                           resourceRegistry);
                    
                    if (!isFirstInstance) {
                        sources.clear();
                        computations.clear();
                    }

                    TF_DEBUG(HD_RPRIM_UPDATED).Msg(
                        "Migrating from immutable to another immutable BAR\n");
                } else {
                    // No changes are being made to the existing immutable BAR.
                    range = bar;
                }

            } else {
                // (c) Exiting BAR is a mutable one.
                HdBufferArrayUsageHint usageHint =
                    HdBufferArrayUsageHintBitsVertex |
                    HdBufferArrayUsageHintBitsStorage;
                range = resourceRegistry->UpdateNonUniformBufferArrayRange(
                    HdTokens->primvar, bar, bufferSpecs, removedSpecs,
                    usageHint);
            }
        }
    } else {
        // When primvar sharing is disabled, a mutable BAR is allocated/updated/
        // migrated as necessary.
        HdBufferArrayUsageHint usageHint = 
            HdBufferArrayUsageHintBitsVertex |
            HdBufferArrayUsageHintBitsStorage;

        range = resourceRegistry->UpdateNonUniformBufferArrayRange(
                HdTokens->primvar, bar, bufferSpecs, removedSpecs,
                usageHint);
    }

    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(),
        &_sharedData,
        renderParam,
        &(renderIndex.GetChangeTracker()));

    if (!sources.empty() || !computations.empty()) {
        // If sources or computations are to be queued against the resulting
        // BAR, we expect it to be valid.
        if (!TF_VERIFY(drawItem->GetVertexPrimvarRange()->IsValid())) {
            return;
        }
    }

    // schedule buffer sources
    if (!sources.empty()) {
        // add sources to update queue
        resourceRegistry->AddSources(drawItem->GetVertexPrimvarRange(),
                                     std::move(sources));
    }
    // add gpu computations to queue.
    for (auto const& compQueuePair : computations) {
        HdStComputationSharedPtr const& comp = compQueuePair.first;
        HdStComputeQueue queue = compQueuePair.second;
        resourceRegistry->AddComputation(
            drawItem->GetVertexPrimvarRange(), comp, queue);
    }
    if (!separateComputationSources.empty()) {
        for (auto const& src : separateComputationSources) {
            resourceRegistry->AddSource(src);
        }
    }
}

void
HdStMesh::_PopulateFaceVaryingPrimvars(HdSceneDelegate *sceneDelegate,
                                       HdRenderParam *renderParam,
                                       const HdReprSharedPtr &repr,
                                       const HdMeshReprDesc &desc,
                                       HdStDrawItem *drawItem,
                                       int geomSubsetDescIndex,
                                       HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdPrimvarDescriptorVector primvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
            HdInterpolationFaceVarying, repr, desc.geomStyle,
                geomSubsetDescIndex, _topology->GetGeomSubsets().size());
    if (primvars.empty() &&
        !drawItem->GetFaceVaryingPrimvarRange())
    {
        return;
    }

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdBufferSourceSharedPtrVector sources;
    sources.reserve(primvars.size());
    HdStComputationComputeQueuePairVector computations;

    int refineLevel = _GetRefineLevelForDesc(desc);
    int numFaceVaryings = _topology ? _topology->GetNumFaceVaryings() : 0;

    TfToken fvarLinearInterpRule =
        _topology->GetSubdivTags().GetFaceVaryingInterpolationRule();
    
    // Fvar primvars only need to be refined when the fvar linear interpolation 
    // rule is not "linear all"
    const bool doRefine = (refineLevel > 0 && 
        fvarLinearInterpRule != PxOsdOpenSubdivTokens->all);
    // At higher levels of refinement that do not require full OSD primvar 
    // refinement, we might want to quadrangulate instead
    const bool doQuadrangulate =
       _UseQuadIndices(sceneDelegate->GetRenderIndex(), _topology) ||
       (refineLevel > 0 && !_topology->RefinesToTriangles());

    // Track primvars that are skipped because they have zero elements
    HdPrimvarDescriptorVector zeroElementPrimvars;

    // If any primvars use doubles, we need to know if the Hgi backend supports
    // these, or if they need to be converted to floats.
    const bool doublesSupported = _GetDoubleSupport(resourceRegistry);

    for (HdPrimvarDescriptor const& primvar: primvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name)) {
            continue;
        }

        VtValue value;
        // If refining and primvar is indexed, get unflattened primvar
        const bool useUnflattendPrimvar = doRefine && primvar.indexed;
        if (useUnflattendPrimvar) {
            VtIntArray indices(0);
            value = GetIndexedPrimvar(sceneDelegate, primvar.name, &indices);
        } else {
            value = GetPrimvar(sceneDelegate, primvar.name);
        }
        
        if (!value.IsEmpty()) {
            HdBufferSourceSharedPtr source =
                std::make_shared<HdVtBufferSource>(primvar.name, value, 1,
                                                   doublesSupported);

            if (!useUnflattendPrimvar && source->GetNumElements() == 0) {
                // zero elements for primvars will be treated as if the primvar
                // doesn't exist, so no warning is necessary
                zeroElementPrimvars.push_back(primvar);
                continue;
            }

            // verify primvar length
            if ((int)source->GetNumElements() != numFaceVaryings && 
                !useUnflattendPrimvar) {
                HF_VALIDATION_WARN(id, 
                    "# of facevaryings mismatch (%d != %d)"
                    " for primvar %s",
                    (int)source->GetNumElements(), numFaceVaryings,
                    primvar.name.GetText());
                continue;
            }

            if (source->GetName() == HdTokens->normals) {
                _sceneNormalsInterpolation = HdInterpolationFaceVarying;
                _sceneNormals = true;
            } else if (source->GetName() == HdTokens->displayOpacity) {
                _displayOpacity = true;
            }

            int channel = 0;
            if (doRefine) {
                channel = 
                    _fvarTopologyTracker->GetChannelFromPrimvar(primvar.name);

                // Invalid fvar topologies may have been skipped when
                // processed by _GatherFaceVaryingTopologies() in which
                // case a validation warning will have been posted already
                // and we should skip further refinement here.
                if (channel < 0) {
                    continue;
                }
            }

            source = _RefineOrQuadrangulateOrTriangulateFaceVaryingPrimvar(
                source, _topology, id,  doRefine, doQuadrangulate, 
                resourceRegistry, &computations, channel);
            
            sources.push_back(source);
        }
    }

    // remove the primvars with zero elements from further processing
    for (HdPrimvarDescriptor const& primvar: zeroElementPrimvars) {
        auto pos = std::find(primvars.begin(), primvars.end(), primvar);
        if (pos != primvars.end()) {
            primvars.erase(pos);
        }
    }

    HdBufferArrayRangeSharedPtr const &bar =
        drawItem->GetFaceVaryingPrimvarRange();

    if (HdStCanSkipBARAllocationOrUpdate(sources, computations, bar, *dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        // no internally generated facevarying primvars
        TfTokenVector internallyGeneratedPrimvars; // empty
        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars, 
            internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    HdStGetBufferSpecsFromCompuations(computations, &bufferSpecs);

    HdBufferArrayRangeSharedPtr range =
        resourceRegistry->UpdateNonUniformBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHintBitsStorage);
    
    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetFaceVaryingPrimvarIndex(),
        &_sharedData,
        renderParam,
        &(sceneDelegate->GetRenderIndex().GetChangeTracker()));

    if (!sources.empty() || !computations.empty()) {
        // If sources or computations are to be queued against the resulting
        // BAR, we expect it to be valid.
        if (!TF_VERIFY(drawItem->GetFaceVaryingPrimvarRange()->IsValid())) {
            return;
        }
    }

    if (!sources.empty()) {
        resourceRegistry->AddSources(
            drawItem->GetFaceVaryingPrimvarRange(), std::move(sources));
    }

    // add gpu computations to queue.
    for (auto const& compQueuePair : computations) {
        HdStComputationSharedPtr const& comp = compQueuePair.first;
        HdStComputeQueue queue = compQueuePair.second;
        resourceRegistry->AddComputation(
            drawItem->GetFaceVaryingPrimvarRange(), comp, queue);
    }
}

void
HdStMesh::_PopulateElementPrimvars(HdSceneDelegate *sceneDelegate,
                                   HdRenderParam *renderParam,
                                   const HdReprSharedPtr &repr,
                                   const HdMeshReprDesc &desc,
                                   HdStDrawItem *drawItem,
                                   int geomSubsetDescIndex,
                                   HdDirtyBits *dirtyBits,
                                   bool requireFlatNormals)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdPrimvarDescriptorVector primvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
            HdInterpolationUniform, repr, desc.geomStyle, geomSubsetDescIndex,
                _topology->GetGeomSubsets().size());

    HdBufferSourceSharedPtrVector sources;
    sources.reserve(primvars.size());

    int numFaces = _topology ? _topology->GetNumFaces() : 0;

    // Track primvars that are skipped because they have zero elements
    HdPrimvarDescriptorVector zeroElementPrimvars;

    // If any primvars use doubles, we need to know if the Hgi backend supports
    // these, or if they need to be converted to floats.
    const bool doublesSupported = _GetDoubleSupport(resourceRegistry);

    for (HdPrimvarDescriptor const& primvar: primvars) {
        if (!HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name))
            continue;

        VtValue value = GetPrimvar(sceneDelegate, primvar.name);
        if (!value.IsEmpty()) {
            HdBufferSourceSharedPtr source =
                std::make_shared<HdVtBufferSource>(primvar.name, value, 1,
                                                   doublesSupported);

            if (source->GetNumElements() == 0) {
                // zero elements for primvars other will be treated as if the
                // primvar doesn't exist, so no warning is necessary
                zeroElementPrimvars.push_back(primvar);
                continue;
            }

            // verify primvar length
            if ((int)source->GetNumElements() != numFaces) {
                HF_VALIDATION_WARN(id,
                    "# of faces mismatch (%d != %d) for uniform primvar %s",
                    (int)source->GetNumElements(), numFaces, 
                    primvar.name.GetText());
                continue;
            }

            if (source->GetName() == HdTokens->normals) {
                _sceneNormalsInterpolation = HdInterpolationUniform;
                _sceneNormals = true;
            } else if (source->GetName() == HdTokens->displayOpacity) {
                _displayOpacity = true;
            }
            sources.push_back(source);
        }
    }

    // remove the primvars with zero elements from further processing
    for (HdPrimvarDescriptor const& primvar: zeroElementPrimvars) {
        auto pos = std::find(primvars.begin(), primvars.end(), primvar);
        if (pos != primvars.end()) {
            primvars.erase(pos);
        }
    }

    HdStComputationComputeQueuePairVector computations;

    TfToken generatedNormalsName;

    if (requireFlatNormals && (*dirtyBits & DirtyFlatNormals))
    {
        *dirtyBits &= ~DirtyFlatNormals;
        TF_VERIFY(_topology);

        bool usePackedNormals = IsEnabledPackedNormals();
        generatedNormalsName = usePackedNormals ?
            HdStTokens->packedFlatNormals : HdStTokens->flatNormals;

        if (_pointsDataType != HdTypeInvalid) {
            // Flat normals will compute normals as the same datatype
            // as points, unless we ask for packed normals.
            // This is unfortunate; can we force them to be float?
            HdStComputationSharedPtr flatNormalsComputation =
                std::make_shared<HdSt_FlatNormalsComputationGPU>(
                    drawItem->GetTopologyRange(),
                    drawItem->GetVertexPrimvarRange(),
                    numFaces,
                    HdTokens->points,
                    generatedNormalsName,
                    _pointsDataType,
                    usePackedNormals);
            computations.emplace_back(flatNormalsComputation, _NormalsCompQueue);
        }
    }

    HdBufferArrayRangeSharedPtr const &bar = drawItem->GetElementPrimvarRange();
    
    if (HdStCanSkipBARAllocationOrUpdate(sources, computations, bar,
            *dirtyBits)) {
        return;
    }

    // XXX: This should be based off the DirtyPrimvarDesc bit.
    bool hasDirtyPrimvarDesc = (*dirtyBits & HdChangeTracker::DirtyPrimvar);
    HdBufferSpecVector removedSpecs;
    if (hasDirtyPrimvarDesc) {
        // If we've just generated normals then make sure those
        // are preserved, otherwise allow either previously existing
        // packed or non-packed normals to remain.
        TfTokenVector internallyGeneratedPrimvars;
        if (! generatedNormalsName.IsEmpty()) {
            internallyGeneratedPrimvars = { generatedNormalsName };
        } else {
            internallyGeneratedPrimvars =
                { HdStTokens->packedFlatNormals, HdStTokens->flatNormals };
        }

        removedSpecs = HdStGetRemovedPrimvarBufferSpecs(bar, primvars, 
            internallyGeneratedPrimvars, id);
    }

    HdBufferSpecVector bufferSpecs;
    HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
    HdStGetBufferSpecsFromCompuations(computations, &bufferSpecs);

    HdBufferArrayRangeSharedPtr range =
        resourceRegistry->UpdateNonUniformBufferArrayRange(
            HdTokens->primvar, bar, bufferSpecs, removedSpecs,
            HdBufferArrayUsageHintBitsStorage);

    HdStUpdateDrawItemBAR(
        range,
        drawItem->GetDrawingCoord()->GetElementPrimvarIndex(),
        &_sharedData,
        renderParam,
        &(sceneDelegate->GetRenderIndex().GetChangeTracker()));

    if (!sources.empty() || !computations.empty()) {
        // If sources or computations are to be queued against the resulting
        // BAR, we expect it to be valid.
        if (!TF_VERIFY(drawItem->GetElementPrimvarRange()->IsValid())) {
            return;
        }
    }

    if (!sources.empty()) {
        resourceRegistry->AddSources(
            drawItem->GetElementPrimvarRange(), std::move(sources));
    }
    // add gpu computations to queue.
    for (auto const& compQueuePair : computations) {
        HdStComputationSharedPtr const& comp = compQueuePair.first;
        HdStComputeQueue queue = compQueuePair.second;
        resourceRegistry->AddComputation(
            drawItem->GetElementPrimvarRange(), comp, queue);
    }
}

bool
HdStMesh::_MaterialHasPtex(
    const HdRenderIndex &renderIndex, 
    const SdfPath &materialId) const
{
    const HdStMaterial *material = static_cast<const HdStMaterial *>(
        renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

    return (material && material->HasPtex());
}

bool
HdStMesh::_UseQuadIndices(
    const HdRenderIndex &renderIndex,
    const HdSt_MeshTopologySharedPtr &topology) const
{
    // We should never quadrangulate for subdivision schemes
    // which refine to triangles (like Loop)
    if (topology->RefinesToTriangles()) {
        return false;
    }

    // Return true if any bound materials use ptex
    bool materialHasPtex = false;

    materialHasPtex = materialHasPtex ||
        _MaterialHasPtex(renderIndex, GetMaterialId());

    const HdGeomSubsets &geomSubsets = topology->GetGeomSubsets();
    for (const HdGeomSubset &geomSubset : geomSubsets) {
        materialHasPtex = materialHasPtex ||
            _MaterialHasPtex(renderIndex, geomSubset.materialId);
    }

    // Fallback to the environment variable, which allows forcing of
    // quadrangulation for debugging/testing.
    return materialHasPtex || _IsEnabledForceQuadrangulate();
}

bool
HdStMesh::_MaterialHasLimitSurface(
    const HdRenderIndex &renderIndex,
    const SdfPath &materialId) const
{
    const HdStMaterial *material = static_cast<const HdStMaterial *>(
        renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

    return (material && material->HasLimitSurfaceEvaluation());
}

bool
HdStMesh::_UseLimitRefinement(
    const HdRenderIndex &renderIndex,
    const HdMeshTopology &topology) const
{
    // Return true if any bound materials have a limit surface evaluation
    bool materialHasLimitSurface = false;

    materialHasLimitSurface = materialHasLimitSurface ||
        _MaterialHasLimitSurface(renderIndex, GetMaterialId());

    const HdGeomSubsets &geomSubsets = topology.GetGeomSubsets();
    for (const HdGeomSubset &geomSubset : geomSubsets) {
        materialHasLimitSurface = materialHasLimitSurface ||
            _MaterialHasLimitSurface(renderIndex, geomSubset.materialId);
    }

    return materialHasLimitSurface;
}

bool
HdStMesh::_UseSmoothNormals(HdSt_MeshTopologySharedPtr const& topology) const
{
    if (_flatShadingEnabled || _limitNormals ||
        topology->GetScheme() == PxOsdOpenSubdivTokens->none ||
        topology->GetScheme() == PxOsdOpenSubdivTokens->bilinear) {
        return false;
    }
    return true;
}

bool
HdStMesh::_UseFlatNormals(const HdMeshReprDesc &desc) const
{
    if (_GetRefineLevelForDesc(desc) > 0 ||
        desc.geomStyle == HdMeshGeomStylePoints) {
        return false;
    }
    return true;
}

static bool
_CanUseTriangulatedFlatNormals(HdSt_MeshTopologySharedPtr const &topology)
{
    // For triangle subdivison or subdivision scheme "none" we
    // can use triangulated flat normals. 
    return topology->RefinesToTriangles() ||
           topology->GetScheme() == PxOsdOpenSubdivTokens->none;
}

HdBufferArrayRangeSharedPtr
HdStMesh::_GetSharedPrimvarRange(uint64_t primvarId,
    HdBufferSpecVector const &updatedOrAddedSpecs,
    HdBufferSpecVector const &removedSpecs,
    HdBufferArrayRangeSharedPtr const &curRange,
    bool * isFirstInstance,
    HdStResourceRegistrySharedPtr const &resourceRegistry) const
{
    HdInstance<HdBufferArrayRangeSharedPtr> barInstance =
        resourceRegistry->RegisterPrimvarRange(primvarId);

    HdBufferArrayRangeSharedPtr range;

    if (barInstance.IsFirstInstance()) {
        HdBufferArrayUsageHint usageHint =
            HdBufferArrayUsageHintBitsVertex |
            HdBufferArrayUsageHintBitsStorage;

        range = resourceRegistry->UpdateNonUniformImmutableBufferArrayRange(
                    HdTokens->primvar,
                    curRange,
                    updatedOrAddedSpecs,
                    removedSpecs,
                    usageHint);

        barInstance.SetValue(range);
    } else {
        range = barInstance.GetValue();
    }

    if (isFirstInstance) {
        *isFirstInstance = barInstance.IsFirstInstance();
    }
    return range;
}

void
HdStMesh::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                          HdRenderParam *renderParam,
                          HdStDrawItem *drawItem,
                          HdDirtyBits *dirtyBits,
                          const TfToken &reprToken,
                          const HdReprSharedPtr &repr,
                          const HdMeshReprDesc &desc,
                          bool requireSmoothNormals,
                          bool requireFlatNormals,
                          int geomSubsetDescIndex)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        std::static_pointer_cast<HdStResourceRegistry>(
        sceneDelegate->GetRenderIndex().GetResourceRegistry());

    /* MATERIAL SHADER (may affect subsequent primvar population) */
    if ((*dirtyBits & HdChangeTracker::NewRepr) ||
        HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        drawItem->SetMaterialNetworkShader
                (HdStGetMaterialNetworkShader(this, sceneDelegate));

        if (desc.geomStyle != HdMeshGeomStylePoints) {
            const HdGeomSubsets &geomSubsets = _topology ? 
                _topology->GetGeomSubsets() : HdGeomSubsets();
            const size_t numGeomSubsets = geomSubsets.size();
            for (size_t i = 0; i < numGeomSubsets; ++i) {
                HdStDrawItem *subsetDrawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItemForGeomSubset(geomSubsetDescIndex, 
                        numGeomSubsets, i));
                if (!TF_VERIFY(subsetDrawItem)) {
                    continue;
                }
                subsetDrawItem->SetMaterialNetworkShader(
                        HdStGetMaterialNetworkShader(
                            this, sceneDelegate, geomSubsets[i].materialId));
            }
        }
    }

    /* TOPOLOGY */
    // XXX: _PopulateTopology should be split into two phase
    //      for scene dirtybits and for repr dirtybits.
    if (*dirtyBits & (HdChangeTracker::DirtyTopology
                    | HdChangeTracker::DirtyDisplayStyle
                    | HdChangeTracker::DirtySubdivTags
                    | HdChangeTracker::DirtyNormals
                    | HdChangeTracker::DirtyWidths 
                    | HdChangeTracker::DirtyPrimvar
                                     | DirtyIndices
                                     | DirtyHullIndices
                                     | DirtyPointsIndices)) {
        _PopulateTopology(sceneDelegate,
                          renderParam,
                          drawItem,
                          dirtyBits,
                          reprToken,
                          repr,
                          desc,
                          geomSubsetDescIndex);
    }

    if (*dirtyBits & HdChangeTracker::DirtyDoubleSided) {
        _doubleSided = IsDoubleSided(sceneDelegate);
    }
    if (*dirtyBits & HdChangeTracker::DirtyCullStyle) {
        _cullStyle = GetCullStyle(sceneDelegate);
    }

    // If it's impossible for this mesh to use smooth normals, we can clear
    // the dirty bit without computing them.  This is ok because the
    // conditions that are checked (topology, display style) will forward their
    // invalidation to smooth normals in PropagateDirtyBits.
    if (!_UseSmoothNormals(_topology)) {
        requireSmoothNormals = false;
        *dirtyBits &= ~DirtySmoothNormals;
    }

    // If the subdivision scheme can use triangle normals,
    // disable flat normal generation.
    if (_CanUseTriangulatedFlatNormals(_topology)) {
        requireFlatNormals = false;
        *dirtyBits &= ~DirtyFlatNormals;
    }
    // Flat shading is based on whether the repr wants flat shading (captured
    // in the passed-in requireFlatNormals), whether the prim wants flat
    // shading, and whether the repr desc allows it.
    requireFlatNormals |= _flatShadingEnabled;
    if (!_UseFlatNormals(desc)) {
        requireFlatNormals = false;
    }

    if (requireSmoothNormals && !_vertexAdjacencyBuilder) {
        _PopulateAdjacency(resourceRegistry);
    }

    // Reset value of _displayOpacity and _sceneNormals if dirty
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
        HdTokens->displayOpacity)) {
        _displayOpacity = false;
    }
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
        HdTokens->normals)) {
        _sceneNormals = false;
    }

    /* INSTANCE PRIMVARS */
    _UpdateInstancer(sceneDelegate, dirtyBits);
    HdStUpdateInstancerData(sceneDelegate->GetRenderIndex(),
                            renderParam,
                            this,
                            drawItem,
                            &_sharedData,
                            *dirtyBits);
    
    _displayOpacity = _displayOpacity ||
            HdStIsInstancePrimvarExistentAndValid(
            sceneDelegate->GetRenderIndex(), this, HdTokens->displayOpacity);

    /* CONSTANT PRIMVARS, TRANSFORM, EXTENT AND PRIMID */
    if (HdStShouldPopulateConstantPrimvars(dirtyBits, id)) {
        HdPrimvarDescriptorVector constantPrimvars =
            HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                HdInterpolationConstant, repr, desc.geomStyle,
                    geomSubsetDescIndex, _topology->GetGeomSubsets().size());
        
        bool hasMirroredTransform = _hasMirroredTransform;
        HdStPopulateConstantPrimvars(this,
                                     &_sharedData,
                                     sceneDelegate,
                                     renderParam, 
                                     drawItem,
                                     dirtyBits,
                                     constantPrimvars,
                                     &hasMirroredTransform);

        _hasMirroredTransform = hasMirroredTransform;
        
        // Check if normals are provided as a constant primvar
        for (const HdPrimvarDescriptor& pv : constantPrimvars) {
            if (pv.name == HdTokens->normals) {
                _sceneNormalsInterpolation = HdInterpolationConstant;
                _sceneNormals = true;
            }
        }

        // Also want to check existence of displayOpacity primvar
        _displayOpacity = _displayOpacity ||
            HdStIsPrimvarExistentAndValid(this, sceneDelegate, 
            constantPrimvars, HdTokens->displayOpacity);
    }

    /* VERTEX PRIMVARS */
    if ((*dirtyBits & HdChangeTracker::NewRepr) ||
        HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        _PopulateVertexPrimvars(sceneDelegate,
                                renderParam,
                                repr,
                                desc,
                                drawItem,
                                geomSubsetDescIndex,
                                dirtyBits,
                                requireSmoothNormals);
    }

    /* FACEVARYING PRIMVARS */
    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        _PopulateFaceVaryingPrimvars(sceneDelegate,
                                     renderParam,
                                     repr,
                                     desc,
                                     drawItem,
                                     geomSubsetDescIndex,
                                     dirtyBits);
    }

    /* ELEMENT PRIMVARS */
    if ((requireFlatNormals && (*dirtyBits & DirtyFlatNormals)) ||
        HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        _PopulateElementPrimvars(sceneDelegate,
                                 renderParam,
                                 repr,
                                 desc,
                                 drawItem,
                                 geomSubsetDescIndex,
                                 dirtyBits,
                                 requireFlatNormals);
    }

    // When we have multiple drawitems for the same mesh we need to clean the
    // bits for all the data fields touched in this function, otherwise it
    // will try to extract topology (for instance) twice, and this won't
    // work with delegates that don't keep information around once extracted.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;

    TF_VERIFY(drawItem->GetConstantPrimvarRange());
    // Topology and VertexPrimvar may be null, if the mesh has zero faces.
    // Element primvar, Facevarying primvar and Instance primvar are optional
}

void
HdStMesh::_UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                         HdRenderParam *renderParam,
                                         HdStDrawItem *drawItem,
                                         const HdMeshReprDesc &desc,
                                         const SdfPath &materialId)
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    bool hasFaceVaryingPrimvars =
        (bool)drawItem->GetFaceVaryingPrimvarRange();

    int refineLevel = _GetRefineLevelForDesc(desc);

    using PrimitiveType = HdSt_GeometricShader::PrimitiveType;
    PrimitiveType primType = PrimitiveType::PRIM_MESH_COARSE_TRIANGLES;

    if (desc.geomStyle == HdMeshGeomStylePoints) {
        primType = PrimitiveType::PRIM_POINTS;
    } else if (refineLevel > 0) {
        if (_topology->RefinesToBSplinePatches()) {
            primType = PrimitiveType::PRIM_MESH_BSPLINE;
        } else if (_topology->RefinesToBoxSplineTrianglePatches()) {
            primType = PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE;
        } else if (_topology->RefinesToTriangles()) {
            // uniform loop subdivision generates triangles.
            primType = PrimitiveType::PRIM_MESH_REFINED_TRIANGLES;
        } else {
            // uniform catmark/bilinear subdivision generates quads.
            if (_topology->TriangulateQuads()) {
                primType = PrimitiveType::PRIM_MESH_REFINED_TRIQUADS;
            } else {
                primType = PrimitiveType::PRIM_MESH_REFINED_QUADS;
            }
        }
    } else if (_UseQuadIndices(renderIndex, _topology)) {
        // quadrangulate coarse mesh (e.g. for ptex)
        if (_topology->TriangulateQuads()) {
            primType = PrimitiveType::PRIM_MESH_COARSE_TRIQUADS;
        } else {
            primType = PrimitiveType::PRIM_MESH_COARSE_QUADS;
        }
    }

    // Determine fvar patch type based on refinement level, uniform/adaptive
    // subdivision, and fvar linear interpolation rule
    using FvarPatchType = HdSt_GeometricShader::FvarPatchType;
    FvarPatchType fvarPatchType = FvarPatchType::PATCH_COARSE_TRIANGLES;
    TfToken fvarLinearInterpRule =
        _topology->GetSubdivTags().GetFaceVaryingInterpolationRule();

    if (refineLevel > 0 && fvarLinearInterpRule != PxOsdOpenSubdivTokens->all) {
        if (_topology->RefinesToBSplinePatches()) {
            fvarPatchType = FvarPatchType::PATCH_BSPLINE;
        } else if (_topology->RefinesToBoxSplineTrianglePatches()) {
            fvarPatchType = FvarPatchType::PATCH_BOXSPLINETRIANGLE;
        } else if (_topology->RefinesToTriangles()) {
            fvarPatchType = FvarPatchType::PATCH_REFINED_TRIANGLES;
        } else {
            fvarPatchType = FvarPatchType::PATCH_REFINED_QUADS;
        }
    } else if (((refineLevel == 0) && 
               ((primType == PrimitiveType::PRIM_MESH_COARSE_QUADS) || 
                (primType == PrimitiveType::PRIM_MESH_COARSE_TRIQUADS))) || 
               ((refineLevel > 0) && (!_topology->RefinesToTriangles()))) {
        fvarPatchType = FvarPatchType::PATCH_COARSE_QUADS;  
    }

    // resolve geom style, cull style
    HdCullStyle cullStyle = desc.cullStyle;
    HdMeshGeomStyle geomStyle = desc.geomStyle;

    // Should the geometric shader expect computed smooth normals for this mesh?
    bool hasGeneratedSmoothNormals = !_limitNormals &&
        _topology->GetScheme() != PxOsdOpenSubdivTokens->none &&
        _topology->GetScheme() != PxOsdOpenSubdivTokens->bilinear;

    // Should the geometric shader expect computed flat normals for this mesh?
    bool hasGeneratedFlatNormals = _UseFlatNormals(desc) &&
         !_CanUseTriangulatedFlatNormals(_topology);

    // Has the draw style been forced to flat-shading?
    bool forceFlatShading =
        _flatShadingEnabled || desc.flatShadingEnabled;

    // Resolve normals interpolation.
    HdInterpolation normalsInterpolation = HdInterpolationVertex;
    if (_sceneNormals) {
        normalsInterpolation = _sceneNormalsInterpolation;
    }

    // Resolve normals source.
    HdSt_MeshShaderKey::NormalSource normalsSource;
    if (forceFlatShading) {
        if (hasGeneratedFlatNormals) {
            normalsSource = HdSt_MeshShaderKey::NormalSourceFlat;
        } else if (_CanUseTriangulatedFlatNormals(_topology)) {
            normalsSource = HdSt_MeshShaderKey::NormalSourceFlatScreenSpace;
        } else {
            normalsSource = HdSt_MeshShaderKey::NormalSourceFlatGeometric;
        }
    } else if (_limitNormals) {
        normalsSource = HdSt_MeshShaderKey::NormalSourceLimit;
    } else if (hasGeneratedSmoothNormals) {
        normalsSource = HdSt_MeshShaderKey::NormalSourceSmooth;
    } else if (_sceneNormals) {
        normalsSource = HdSt_MeshShaderKey::NormalSourceScene;
    } else {
        normalsSource = HdSt_MeshShaderKey::NormalSourceFlatGeometric;
    }

    // if the repr doesn't have an opinion about cullstyle, use the
    // prim's default (it could also be DontCare, then renderPass's
    // cullStyle is going to be used).
    //
    // i.e.
    //   Repr CullStyle > Rprim CullStyle > RenderPass CullStyle
    //
    if (cullStyle == HdCullStyleDontCare) {
        cullStyle = _cullStyle;
    }

    bool blendWireframeColor = desc.blendWireframeColor;

    // Check if the shader bound to this mesh has a custom displacement
    // terminal, or uses ptex, so that we know whether to include the geometry
    // shader.
    HdStMaterial *material = static_cast<HdStMaterial *>(
            renderIndex.GetSprim(HdPrimTypeTokens->material, materialId));

    bool hasCustomDisplacementTerminal =
        material && material->HasDisplacement();
    bool hasPtex = material && material->HasPtex();

    // FaceVarying primvars or ptex requires per-face interpolation.
    bool const hasPerFaceInterpolation = hasFaceVaryingPrimvars || hasPtex;

    bool hasTopologicalVisibility =
        (bool) drawItem->GetTopologyVisibilityRange();

    // Enable displacement shading only if the repr enables it, and the
    // entrypoint exists.
    bool hasCustomDisplacement = hasCustomDisplacementTerminal &&
        desc.useCustomDisplacement && _displacementEnabled;

    bool hasInstancer = !GetInstancerId().IsEmpty();

    // Process shadingTerminal (including shadingStyle)
    TfToken shadingTerminal = desc.shadingTerminal;
    if (shadingTerminal == HdMeshReprDescTokens->surfaceShader) {
        TfToken shadingStyle =
                    GetShadingStyle(sceneDelegate).GetWithDefault<TfToken>();
        if (shadingStyle == HdStTokens->constantLighting) {
            shadingTerminal = HdMeshReprDescTokens->surfaceShaderUnlit;
        }
    }

    HdStResourceRegistrySharedPtr resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            renderIndex.GetResourceRegistry());

    bool const hasBuiltinBarycentrics =
        resourceRegistry->GetHgi()->GetCapabilities()->
            IsSet(HgiDeviceCapabilitiesBitsBuiltinBarycentrics);

    bool const hasMetalTessellation =
        resourceRegistry->GetHgi()->GetCapabilities()->
            IsSet(HgiDeviceCapabilitiesBitsMetalTessellation);

    // create a shaderKey and set to the geometric shader.
    HdSt_MeshShaderKey shaderKey(primType,
                                 shadingTerminal,
                                 normalsSource,
                                 normalsInterpolation,
                                 cullStyle,
                                 geomStyle,
                                 fvarPatchType,
                                 desc.lineWidth,
                                 _doubleSided || desc.doubleSided,
                                 hasBuiltinBarycentrics,
                                 hasMetalTessellation,
                                 hasCustomDisplacement,
                                 hasPerFaceInterpolation,
                                 hasTopologicalVisibility,
                                 blendWireframeColor,
                                 _hasMirroredTransform,
                                 hasInstancer,
                                 desc.enableScalarOverride,
                                 _pointsShadingEnabled,
                                 desc.forceOpaqueEdges);

    HdSt_GeometricShaderSharedPtr geomShader =
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry);

    TF_VERIFY(geomShader);

    if (geomShader != drawItem->GetGeometricShader())
    {
        drawItem->SetGeometricShader(geomShader);

        // If the geometric shader changes, we need to do a deep validation of
        // batches, so they can be rebuilt if necessary.
        HdStMarkDrawBatchesDirty(renderParam);

        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: Marking all batches dirty to trigger deep validation because"
            " the geometric shader was updated.\n", GetId().GetText());
    }
}

// virtual
HdDirtyBits
HdStMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    // If subdiv tags are dirty, topology needs to be recomputed.
    // The latter implies we'll need to recompute all primvar data.
    // Any data fetched by the scene delegate should be marked dirty here.
    if (bits & HdChangeTracker::DirtySubdivTags) {
        bits |= (HdChangeTracker::DirtyPoints   |
                HdChangeTracker::DirtyNormals  |
                HdChangeTracker::DirtyPrimvar  |
                HdChangeTracker::DirtyTopology |
                HdChangeTracker::DirtyDisplayStyle);
    } else if (bits & HdChangeTracker::DirtyTopology) {
        // Unlike basis curves, we always request refineLevel when topology is
        // dirty
        bits |= HdChangeTracker::DirtySubdivTags |
                HdChangeTracker::DirtyDisplayStyle;
    }

    // A change of material means that the Quadrangulate state may have
    // changed.
    if (bits & HdChangeTracker::DirtyMaterialId) {
        bits |= (HdChangeTracker::DirtyPoints   |
                HdChangeTracker::DirtyNormals  |
                HdChangeTracker::DirtyPrimvar  |
                HdChangeTracker::DirtyTopology);
    }

    // If points, display style, or topology changed, recompute normals.
    if (bits & (HdChangeTracker::DirtyPoints |
                HdChangeTracker::DirtyDisplayStyle |
                HdChangeTracker::DirtyTopology)) {
        bits |= _customDirtyBitsInUse &
            (DirtySmoothNormals | DirtyFlatNormals);
    }

    // If the topology is dirty, recompute custom indices resources.
    if (bits & HdChangeTracker::DirtyTopology) {
        bits |= _customDirtyBitsInUse &
                   (DirtyIndices      |
                    DirtyHullIndices  |
                    DirtyPointsIndices);
    }

    return bits;
}

void
HdStMesh::_InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits)
{
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        _reprs.emplace_back(reprToken, std::make_shared<HdRepr>());
        HdReprSharedPtr &repr = _reprs.back().second;

        // set dirty bit to say we need to sync a new repr (buffer array
        // ranges may change)
        *dirtyBits |= HdChangeTracker::NewRepr;

        _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);

        // allocate all draw items
        size_t numGeomSubsets = _topology ? 
            _topology->GetGeomSubsets().size() : 0;
        
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            const HdMeshReprDesc &desc = descs[descIdx];

            if (desc.geomStyle == HdMeshGeomStyleInvalid) {
                continue;
            }

            int geomSubsetTopologyIndexOffset = 0;
            {
                HdRepr::DrawItemUniquePtr drawItem =
                    std::make_unique<HdStDrawItem>(&_sharedData);
                HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
                repr->AddDrawItem(std::move(drawItem));

                switch (desc.geomStyle) {
                    case HdMeshGeomStyleHull:
                    case HdMeshGeomStyleHullEdgeOnly:
                    case HdMeshGeomStyleHullEdgeOnSurf:
                    {
                        geomSubsetTopologyIndexOffset = 1; 
                        drawingCoord->SetTopologyIndex(HdStMesh::HullTopology);
                        if (!(_customDirtyBitsInUse & DirtyHullIndices)) {
                            _customDirtyBitsInUse |= DirtyHullIndices;
                            *dirtyBits |= DirtyHullIndices;
                        }
                        break;
                    }

                    case HdMeshGeomStylePoints:
                    {
                        // in the current implementation, we use topology
                        // for points too, to draw a subset of vertex primvars
                        // (note that the points may be followed by the refined
                        // vertices)
                        drawingCoord->SetTopologyIndex(HdStMesh::PointsTopology);
                        if (!(_customDirtyBitsInUse & DirtyPointsIndices)) {
                            _customDirtyBitsInUse |= DirtyPointsIndices;
                            *dirtyBits |= DirtyPointsIndices;
                        }
                        break;
                    }

                    default:
                    {
                        if (!(_customDirtyBitsInUse & DirtyIndices)) {
                            _customDirtyBitsInUse |= DirtyIndices;
                            *dirtyBits |= DirtyIndices;
                        }
                    }
                }

                // Set up drawing coord instance primvars.
                drawingCoord->SetInstancePrimvarBaseIndex(
                    HdStMesh::FreeSlot + 2 * numGeomSubsets);
            }

            // Allocate geom subset draw items
            if (desc.geomStyle != HdMeshGeomStylePoints) {
                for (size_t i = 0; i < numGeomSubsets; ++i) {
                    HdRepr::DrawItemUniquePtr drawItem =
                        std::make_unique<HdStDrawItem>(&_sharedData);
                    HdDrawingCoord *drawingCoord = drawItem->GetDrawingCoord();
                    repr->AddGeomSubsetDrawItem(std::move(drawItem));
                    drawingCoord->SetTopologyIndex(
                        HdStMesh::FreeSlot + 2 * i + 
                            geomSubsetTopologyIndexOffset);
                    drawingCoord->SetInstancePrimvarBaseIndex(
                        HdStMesh::FreeSlot + 2 * numGeomSubsets);
                }
            }

            if (desc.flatShadingEnabled) {
                if (!(_customDirtyBitsInUse & DirtyFlatNormals)) {
                    _customDirtyBitsInUse |= DirtyFlatNormals;
                    *dirtyBits |= DirtyFlatNormals;
                }
            } else {
                if (!(_customDirtyBitsInUse & DirtySmoothNormals)) {
                    _customDirtyBitsInUse |= DirtySmoothNormals;
                    *dirtyBits |= DirtySmoothNormals;
                }
            }
        } // for each repr desc for the repr
    } // if new repr
}

void
HdStMesh::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                      HdRenderParam *renderParam,
                      TfToken const &reprToken,
                      HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdReprSharedPtr const &curRepr = _GetRepr(reprToken);
    if (!curRepr) {
        return;
    }

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        TfDebug::Helper().Msg(
            "HdStMesh::_UpdateRepr for %s : Repr = %s\n",
            GetId().GetText(), reprToken.GetText());
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    _MeshReprConfig::DescArray reprDescs = _GetReprDesc(reprToken);

    // Iterate through all reprdescs for the current repr to figure out if any 
    // of them requires smooth normals or flat normals. If either (or both)
    // are required, we will calculate them once and clean the bits.
    bool requireSmoothNormals = false;
    bool requireFlatNormals =  false;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc &desc = reprDescs[descIdx];
        if (desc.geomStyle == HdMeshGeomStyleInvalid) {
            continue;
        }
        if (desc.flatShadingEnabled) {
            requireFlatNormals = true;
        } else {
            requireSmoothNormals = true;
        }
    }

    // For each relevant draw item, update dirty buffer sources.
    int drawItemIndex = 0;
    int geomSubsetDescIndex = 0;
    for (size_t descIdx = 0; descIdx < reprDescs.size(); ++descIdx) {
        const HdMeshReprDesc &desc = reprDescs[descIdx];
        if (desc.geomStyle == HdMeshGeomStyleInvalid) {
            continue;
        }

        HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
            curRepr->GetDrawItem(drawItemIndex++));

        if (HdChangeTracker::IsDirty(*dirtyBits)) {
            _UpdateDrawItem(sceneDelegate,
                            renderParam,
                            drawItem,
                            dirtyBits,
                            reprToken,
                            curRepr,
                            desc,
                            requireSmoothNormals,
                            requireFlatNormals,
                            geomSubsetDescIndex);
        }

        if (desc.geomStyle != HdMeshGeomStylePoints) {
            geomSubsetDescIndex++;
        }
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

void
HdStMesh::_UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                    HdRenderParam *renderParam,
                                    bool updateMaterialNetworkShader,
                                    bool updateGeometricShader)
{
    TF_DEBUG(HD_RPRIM_UPDATED). Msg(
        "(%s) - Updating geometric and material shaders for draw "
        "items of all reprs.\n", GetId().GetText());

    HdSt_MaterialNetworkShaderSharedPtr materialNetworkShader;

    const bool materialIsFinal = GetDisplayStyle(sceneDelegate).materialIsFinal;
    bool materialIsFinalChanged = false;

    for (auto const& reprPair : _reprs) {
        const TfToken &reprToken = reprPair.first;
        _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
        HdReprSharedPtr repr = reprPair.second;

        int drawItemIndex = 0;
        int geomSubsetDescIndex = 0;
        // For each desc
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            if (descs[descIdx].geomStyle == HdMeshGeomStyleInvalid) {
                continue;
            }

            // Update original draw item
            {
                HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItem(drawItemIndex++));

                if (materialIsFinal != drawItem->GetMaterialIsFinal()) {
                    materialIsFinalChanged = true;
                }
                drawItem->SetMaterialIsFinal(materialIsFinal);

                if (updateMaterialNetworkShader) {
                    materialNetworkShader =
                        HdStGetMaterialNetworkShader(this, sceneDelegate);
                    drawItem->SetMaterialNetworkShader(materialNetworkShader);
                }
                if (updateGeometricShader) {
                    _UpdateDrawItemGeometricShader(sceneDelegate, renderParam,
                        drawItem, descs[descIdx], GetMaterialId());
                }
            }

            // Update geom subset draw items if they exist 
            if (descs[descIdx].geomStyle == HdMeshGeomStylePoints) {
                continue;
            }

            const HdGeomSubsets &geomSubsets = _topology->GetGeomSubsets();
            const size_t numGeomSubsets = geomSubsets.size();
            for (size_t i = 0; i < numGeomSubsets; ++i) {
                const SdfPath &materialId = geomSubsets[i].materialId;

                HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItemForGeomSubset(
                        geomSubsetDescIndex, numGeomSubsets, i));
                if (!TF_VERIFY(drawItem)) {
                    continue;
                }

                drawItem->SetMaterialIsFinal(materialIsFinal);

                if (updateMaterialNetworkShader) {
                    materialNetworkShader = HdStGetMaterialNetworkShader(
                        this, sceneDelegate, materialId);
                    drawItem->SetMaterialNetworkShader(materialNetworkShader);
                }
                if (updateGeometricShader) {
                    _UpdateDrawItemGeometricShader(sceneDelegate, renderParam,
                        drawItem, descs[descIdx], materialId);
                }
            }
            geomSubsetDescIndex++;
        }
    }

    if (materialIsFinalChanged) {
        HdStMarkDrawBatchesDirty(renderParam);
        TF_DEBUG(HD_RPRIM_UPDATED).Msg(
            "%s: Marking all batches dirty to trigger deep validation because "
            "the materialIsFinal was updated.\n", GetId().GetText());
    }
}

void
HdStMesh::_UpdateMaterialTagsForAllReprs(HdSceneDelegate *sceneDelegate,
                                         HdRenderParam *renderParam)
{
    TF_DEBUG(HD_RPRIM_UPDATED). Msg(
        "(%s) - Updating material tags for draw items of all reprs.\n", 
        GetId().GetText());

    for (auto const& reprPair : _reprs) {
        const TfToken &reprToken = reprPair.first;
        _MeshReprConfig::DescArray descs = _GetReprDesc(reprToken);
        HdReprSharedPtr repr = reprPair.second;

        int drawItemIndex = 0;
        int geomSubsetDescIndex = 0;
        // For each desc
        for (size_t descIdx = 0; descIdx < descs.size(); ++descIdx) {
            if (descs[descIdx].geomStyle == HdMeshGeomStyleInvalid) {
                continue;
            }

            // Update original draw item
            {
                HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItem(drawItemIndex++));
                HdStSetMaterialTag(sceneDelegate, renderParam, drawItem, 
                    this->GetMaterialId(), _displayOpacity, 
                    _occludedSelectionShowsThrough);
            }

            // Update geom subset draw items if they exist 
            if (descs[descIdx].geomStyle == HdMeshGeomStylePoints) {
                continue;
            }

            const HdGeomSubsets &geomSubsets = _topology->GetGeomSubsets();
            const size_t numGeomSubsets = geomSubsets.size();
            for (size_t i = 0; i < numGeomSubsets; ++i) {
                const SdfPath &materialId = geomSubsets[i].materialId;

                HdStDrawItem *drawItem = static_cast<HdStDrawItem*>(
                    repr->GetDrawItemForGeomSubset(
                        geomSubsetDescIndex, numGeomSubsets, i));
                if (!TF_VERIFY(drawItem)) {
                    continue;
                }
                HdStSetMaterialTag(sceneDelegate, renderParam, drawItem,
                    materialId, _displayOpacity, 
                    _occludedSelectionShowsThrough);
            }
            geomSubsetDescIndex++;
        }
    }
}

HdDirtyBits
HdStMesh::GetInitialDirtyBitsMask() const
{
    HdDirtyBits mask = HdChangeTracker::Clean
        | HdChangeTracker::InitRepr
        | HdChangeTracker::DirtyCullStyle
        | HdChangeTracker::DirtyDoubleSided
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyDisplayStyle
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyInstancer;
        ;

    return mask;
}

PXR_NAMESPACE_CLOSE_SCOPE

