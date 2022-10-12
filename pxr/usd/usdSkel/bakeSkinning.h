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
#ifndef PXR_USD_USD_SKEL_BAKE_SKINNING_H
#define PXR_USD_USD_SKEL_BAKE_SKINNING_H

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdSkel/binding.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrimRange;
class UsdSkelCache;
class UsdSkelRoot;

/// Parameters for configuring UsdSkelBakeSkinning.
struct UsdSkelBakeSkinningParms
{
    /// Flags for identifying different deformation paths.
    enum DeformationFlags {
        DeformPointsWithLBS = 1 << 0,
        DeformNormalsWithLBS = 1 << 1,
        DeformXformWithLBS = 1 << 2,
        DeformPointsWithBlendShapes = 1 << 3,
        DeformNormalsWithBlendShapes = 1 << 4,
        DeformWithLBS = (DeformPointsWithLBS|
                         DeformNormalsWithLBS|
                         DeformXformWithLBS),
        DeformWithBlendShapes = (DeformPointsWithBlendShapes|
                                 DeformNormalsWithBlendShapes),
        DeformAll = DeformWithLBS|DeformWithBlendShapes,
        /// Flags indicating which components of skinned prims may be
        /// modified, based on the active deformations.
        ModifiesPoints = DeformPointsWithLBS|DeformPointsWithBlendShapes,
        ModifiesNormals = DeformNormalsWithLBS|DeformNormalsWithBlendShapes,
        ModifiesXform = DeformXformWithLBS
    };

    /// Flags determining which deformation paths are enabled.
    int deformationFlags = DeformAll;

    /// Determines whether or not layers are saved during skinning.
    /// If disabled, all skinning data is kept in-memory, and it is up
    /// to the caller to save or export the affected layers.
    bool saveLayers = true;

    /// Memory limit for pending stage writes, given in bytes.
    /// If zero, memory limits are ignored. Otherwise, output stages
    /// are flushed each time pending writes exceed this amount.
    /// Note that at least one frame of data for *all* skinned prims
    /// will be held in memory prior to values being written to disk,
    /// regardless of this memory limit.    
    /// Since flushing pending changes requires layers to be saved,
    /// memory limiting is only active when _saveLayers_ is enabled.
    size_t memoryLimit = 0;

    /// If true, extents of UsdGeomPointBased-derived prims are updated
    /// as new skinned values are produced. This is made optional
    /// in case an alternate procedure is being used to compute
    /// extents elsewhere.
    bool updateExtents = true;

    /// If true, extents hints of models that already stored
    /// an extentsHint are updated to reflect skinning changes.
    /// All extent hints are authored to the stage's current edit target.
    bool updateExtentHints = true;

    /// The set of bindings to bake.
    std::vector<UsdSkelBinding> bindings;

    /// Data layers being written to.
    /// Layer authoring is not thread-safe, but if multiple layers are
    /// provided, then each of those layers may be written to on separate
    /// threads, improving parallelism of writes.
    /// Note that each layer must already be in the layer stack of the stage on
    /// which the _bindings_ are defined *before* running baking. This is
    /// necessary in order for composition of some properties during the
    /// baking process. If this is not done, then extents of some models
    /// may be incorrect.
    std::vector<SdfLayerHandle> layers;

    /// Array providing an index per elem in _bindings_, indicating
    /// which layer the skinned result of the binding should be written to.
    /// The length of this array must be equal to the length of
    /// the _bindings_ array.
    VtUIntArray layerIndices;
};


/// Bake the effect of skinning prims directly into points and transforms,
/// over \p interval.
/// This is intended to serve as a complete reference implementation,
/// providing a ground truth for testing and validation purposes.
///
/// Although this process attempts to bake skinning as efficiently as possible,
/// beware that this will undo the IO gains that deferred deformations provide.
/// A USD file, once skinning has been baked, may easily see an increase of 100x
/// in disk usage, if not more. The render-time costs of invoking skinning
/// tend to be low relative to the IO gains, so there is little render-time
/// benefit in baking the result down. Whatever wins are achieved may in fact
/// be undone by the increased IO costs.
/// The intent of the UsdSkel encoding is to defer skinning until as late in
/// the pipeline as possible (I.e., render time), partially for the sake of
/// improving IO in distributed renderings contexts. We encourage users to
/// bring similar deferred-deformation capabalities to their renderer, rather
/// than relying on baking data down. 
USDSKEL_API
bool
UsdSkelBakeSkinning(const UsdSkelCache& skelCache,
                    const UsdSkelBakeSkinningParms& parms,
                    const GfInterval& interval=GfInterval::GetFullInterval());


/// Overload of UsdSkelBakeSkinning, which bakes the effect of skinning prims
/// directly into points and transforms, for all skels bound beneath \p root,
/// over \p interval.
/// Skinning is baked into the current edit target. The edit target is *not*
/// saved during skinning: the caller should Save() or Export() the result.
USDSKEL_API
bool
UsdSkelBakeSkinning(const UsdSkelRoot& root,
                    const GfInterval& interval=GfInterval::GetFullInterval());


/// Overload of UsdSkelBakeSkinning, which bakes the effect of skinning prims
/// directly into points and transforms, for all SkelRoot prims in \p range,
/// over \p interval.
/// Skinning is baked into the current edit target. The edit target is *not*
/// saved during skinning: the caller should Save() or Export() the result.
USDSKEL_API
bool
UsdSkelBakeSkinning(const UsdPrimRange& range,
                    const GfInterval& interval=GfInterval::GetFullInterval());


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXR_USD_USD_SKEL_BAKE_SKINNING_H
