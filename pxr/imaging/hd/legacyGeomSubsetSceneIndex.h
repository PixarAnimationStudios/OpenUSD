//
// Copyright 2024 Pixar
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
#ifndef PXR_IMAGING_HD_LEGACY_GEOM_SUBSET_SCENE_INDEX_H
#define PXR_IMAGING_HD_LEGACY_GEOM_SUBSET_SCENE_INDEX_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/declarePtrs.h"

#include "pxr/pxr.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdLegacyGeomSubsetSceneIndex);

///
/// HdLegacyGeomSubsetSceneIndex
///
/// This scene index converts legacy geom subsets (from mesh or basis curves
/// topology, including invisible components) into Hydra geomSubset prims. It
/// preserves the authored order of named mesh subsets as USD requires.
/// It MUST have a notice-batching scene index before it so that it can access
/// topology via the scene delegate during insertion.
///
/// For the most part, this scene index will pull information from the scene
/// delegate on demand. However, it does keep a cache of all the subset paths
/// it has added (organized by parent) so it can be more precise about
/// invalidation. Having this cache incidentally provides a few other
/// shortcuts to avoid expensive operations.
class HdLegacyGeomSubsetSceneIndex
  : public HdSingleInputFilteringSceneIndexBase
{
public:
    HD_API
    static HdLegacyGeomSubsetSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    
    HD_API
    ~HdLegacyGeomSubsetSceneIndex() override;
    
    HD_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    
    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    HdLegacyGeomSubsetSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    
    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;
    
    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;
    
    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

private:    
    static SdfPathVector
    _ListDelegateSubsets(
        const SdfPath& parentPath,
        const HdSceneIndexPrim& parentPrim);
    
    // Unordered map of parent path -> [subset paths...]
    // XXX: Do not use SdfPathTable because we do not want it
    // to implicitly include the extra ancestor paths.
    std::unordered_map<SdfPath, SdfPathVector, SdfPath::Hash> _parentPrims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_LEGACY_GEOM_SUBSET_SCENE_INDEX_H
