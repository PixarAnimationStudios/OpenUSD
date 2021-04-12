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
#ifndef PXR_IMAGING_HD_ST_PRIM_UTILS_H
#define PXR_IMAGING_HD_ST_PRIM_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdChangeTracker;
class HdDrawItem;
class HdRenderIndex;
class HdRenderParam;
class HdRprim;
struct HdRprimSharedData;
class HdStDrawItem;
class HdStInstancer;

using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;

using HdBufferSourceSharedPtrVector = std::vector<HdBufferSourceSharedPtr>;
using HdBufferSpecVector = std::vector<struct HdBufferSpec>;
using HdStShaderCodeSharedPtr = std::shared_ptr<class HdStShaderCode>;

using HdComputationSharedPtr = std::shared_ptr<class HdComputation>;

using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<HdStResourceRegistry>;

// -----------------------------------------------------------------------------
// Draw invalidation and garbage collection utilities
// -----------------------------------------------------------------------------
HDST_API
void HdStMarkDrawBatchesDirty(HdRenderParam *renderParam);

HDST_API
void HdStMarkMaterialTagsDirty(HdRenderParam *renderParam);

HDST_API
void HdStMarkGarbageCollectionNeeded(HdRenderParam *renderParam);

// -----------------------------------------------------------------------------
// Primvar descriptor filtering utilities
// -----------------------------------------------------------------------------
// Get filtered primvar descriptors for drawItem
HDST_API
HdPrimvarDescriptorVector
HdStGetPrimvarDescriptors(
    HdRprim const * prim,
    HdStDrawItem const * drawItem,
    HdSceneDelegate * delegate,
    HdInterpolation interpolation);

// Get filtered instancer primvar descriptors for drawItem
HDST_API
HdPrimvarDescriptorVector
HdStGetInstancerPrimvarDescriptors(
    HdStInstancer const * instancer,
    HdSceneDelegate * delegate);

// -----------------------------------------------------------------------------
// Material processing utilities
// -----------------------------------------------------------------------------
HDST_API
void HdStSetMaterialId(HdSceneDelegate *delegate,
                       HdRenderParam *renderParam,
                       HdRprim *rprim);

HDST_API
void HdStSetMaterialTag(HdSceneDelegate *delegate,
                        HdRenderParam *renderParam,
                        HdRprim *rprim,
                        bool hasDisplayOpacityPrimvar,
                        bool occludedSelectionShowsThrough);
// Resolves the material shader for the given prim (using a fallback
// material as necessary), including optional mixin shader source code.
HDST_API
HdStShaderCodeSharedPtr
HdStGetMaterialShader(
    HdRprim const * prim,
    HdSceneDelegate * delegate,
    std::string const & mixinSource = std::string());

// -----------------------------------------------------------------------------
// Primvar processing and BAR allocation utilities
// -----------------------------------------------------------------------------
// Returns true if range is non-empty and valid.
HDST_API
bool HdStIsValidBAR(HdBufferArrayRangeSharedPtr const& range);

// Returns true if curRange can be used as-is (even if it's empty) during
// primvar processing.
HDST_API
bool HdStCanSkipBARAllocationOrUpdate(
    HdBufferSourceSharedPtrVector const& sources,
    HdStComputationSharedPtrVector const& computations,
    HdBufferArrayRangeSharedPtr const& curRange,
    HdDirtyBits dirtyBits);

HDST_API
bool HdStCanSkipBARAllocationOrUpdate(
    HdBufferSourceSharedPtrVector const& sources,
    HdBufferArrayRangeSharedPtr const& curRange,
    HdDirtyBits dirtyBits);

// Returns the buffer specs that have been removed from curRange based on the
// new primvar descriptors and internally generated primvar names.
//
// Internally generated primvar names will never be among the specs returned,
HDST_API
HdBufferSpecVector
HdStGetRemovedPrimvarBufferSpecs(
    HdBufferArrayRangeSharedPtr const& curRange,
    HdPrimvarDescriptorVector const& newPrimvarDescs,
    HdExtComputationPrimvarDescriptorVector const& newCompPrimvarDescs,
    TfTokenVector const& internallyGeneratedPrimvarNames,
    SdfPath const& rprimId);

HDST_API
HdBufferSpecVector
HdStGetRemovedPrimvarBufferSpecs(
    HdBufferArrayRangeSharedPtr const& curRange,
    HdPrimvarDescriptorVector const& newPrimvarDescs,
    TfTokenVector const& internallyGeneratedPrimvarNames,
    SdfPath const& rprimId);

// Updates the existing range at drawCoordIndex with newRange and flags garbage
// collection (for the existing range) and rebuild of all draw batches when
// necessary.
HDST_API
void HdStUpdateDrawItemBAR(
    HdBufferArrayRangeSharedPtr const& newRange,
    int drawCoordIndex,
    HdRprimSharedData *sharedData,
    HdRenderParam *renderParam,
    HdChangeTracker *changeTracker);

// Returns true if primvar with primvarName exists within primvar descriptor 
// vector primvars and primvar has a valid value
HDST_API
bool HdStIsPrimvarExistentAndValid(
    HdRprim *prim,
    HdSceneDelegate *delegate,
    HdPrimvarDescriptorVector const& primvars,
    TfToken const& primvarName);

// -----------------------------------------------------------------------------
// Constant primvar processing utilities
// -----------------------------------------------------------------------------
// Returns whether constant primvars need to be populated/updated based on the
// dirty bits for a given rprim.
HDST_API
bool HdStShouldPopulateConstantPrimvars(
    HdDirtyBits const *dirtyBits,
    SdfPath const& id);

// Given prim information it will create sources representing
// constant primvars and hand it to the resource registry.
// If transforms are dirty, updates the optional bool.
HDST_API
void HdStPopulateConstantPrimvars(
    HdRprim *prim,
    HdRprimSharedData *sharedData,
    HdSceneDelegate *delegate,
    HdRenderParam *renderParam,
    HdDrawItem *drawItem,
    HdDirtyBits *dirtyBits,
    HdPrimvarDescriptorVector const& constantPrimvars,
    bool *hasMirroredTransform = nullptr);

// -----------------------------------------------------------------------------
// Instancer processing utilities
// -----------------------------------------------------------------------------

// Updates drawItem bindings for changes to instance topology/primvars.
HDST_API
void HdStUpdateInstancerData(
    HdRenderIndex &renderIndex,
    HdRenderParam *renderParam,
    HdRprim *prim,
    HdStDrawItem *drawItem,
    HdRprimSharedData *sharedData,
    HdDirtyBits rprimDirtyBits);

// Returns true if primvar with primvarName exists among instance primvar
// descriptors.
HDST_API
bool HdStIsInstancePrimvarExistentAndValid(
    HdRenderIndex &renderIndex,
    HdRprim *prim,
    TfToken const& primvarName);

// -----------------------------------------------------------------------------
// Topological visibility processing utility
// -----------------------------------------------------------------------------
// Creates/Updates/Migrates the topology visiblity BAR with element and point
// visibility encoded using one bit per element/point of the topology.
HDST_API
void HdStProcessTopologyVisibility(
    VtIntArray invisibleElements,
    int numTotalElements,
    VtIntArray invisiblePoints,
    int numTotalPoints,
    HdRprimSharedData *sharedData,
    HdStDrawItem *drawItem,
    HdRenderParam *renderParam,
    HdChangeTracker *changeTracker,
    HdStResourceRegistrySharedPtr const &resourceRegistry,
    SdfPath const& rprimId);

//
// De-duplicating and sharing immutable primvar data.
// 
// Primvar data is identified using a hash computed from the
// sources of the primvar data, of which there are generally
// two kinds:
//   - data provided by the scene delegate
//   - data produced by computations
// 
// Immutable and mutable buffer data is managed using distinct
// heaps in the resource registry. Aggregation of buffer array
// ranges within each heap is managed separately.
// 
// We attempt to balance the benefits of sharing vs efficient
// varying update using the following simple strategy:
//
//  - When populating the first repr for an rprim, allocate
//    the primvar range from the immutable heap and attempt
//    to deduplicate the data by looking up the primvarId
//    in the primvar instance registry.
//
//  - When populating an additional repr for an rprim using
//    an existing immutable primvar range, compute an updated
//    primvarId and allocate from the immutable heap, again
//    attempting to deduplicate.
//
//  - Otherwise, migrate the primvar data to the mutable heap
//    and abandon further attempts to deduplicate.
//
//  - The computation of the primvarId for an rprim is cumulative
//    and includes the new sources of data being committed
//    during each successive update.
//
//  - Once we have migrated a primvar allocation to the mutable
//    heap we will no longer spend time computing a primvarId.
//

HDST_API
bool HdStIsEnabledSharedVertexPrimvar();

HDST_API
uint64_t HdStComputeSharedPrimvarId(
    uint64_t baseId,
    HdBufferSourceSharedPtrVector const &sources,
    HdStComputationSharedPtrVector const &computations);

HDST_API
void HdStGetBufferSpecsFromCompuations(
    HdStComputationSharedPtrVector const& computations,
    HdBufferSpecVector *bufferSpecs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_PRIM_UTILS_H
