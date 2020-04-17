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
#ifndef PXR_IMAGING_HD_ST_RPRIM_UTILS_H
#define PXR_IMAGING_HD_ST_RPRIM_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdChangeTracker;
class HdDrawItem;
class HdRenderIndex;
class HdRprim;
struct HdRprimSharedData;
class HdStDrawItem;
class HdStInstancer;

using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;

using HdBufferSourceSharedPtrVector = std::vector<HdBufferSourceSharedPtr>;
using HdBufferSpecVector = std::vector<struct HdBufferSpec>;
using HdStShaderCodeSharedPtr = std::shared_ptr<class HdStShaderCode>;

using HdComputationSharedPtr = std::shared_ptr<class HdComputation>;
using HdComputationSharedPtrVector = std::vector<HdComputationSharedPtr>;

using HdStResourceRegistrySharedPtr = 
    std::shared_ptr<class HdStResourceRegistry>;

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
    HdRprim const * prim,
    HdStDrawItem const * drawItem,
    HdSceneDelegate * delegate);

// -----------------------------------------------------------------------------
// Material shader utility
// -----------------------------------------------------------------------------
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
    HdComputationSharedPtrVector const& computations,
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
    HdRenderIndex &renderIndex);

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
HDST_API
void HdStPopulateConstantPrimvars(
    HdRprim *prim,
    HdRprimSharedData *sharedData,
    HdSceneDelegate *delegate,
    HdDrawItem *drawItem,
    HdDirtyBits *dirtyBits,
    HdPrimvarDescriptorVector const& constantPrimvars);

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
    HdChangeTracker *changeTracker,
    HdStResourceRegistrySharedPtr const &resourceRegistry,
    SdfPath const& rprimId);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_RPRIM_UTILS_H
