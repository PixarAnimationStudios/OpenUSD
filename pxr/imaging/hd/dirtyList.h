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
#ifndef PXR_IMAGING_HD_DIRTY_LIST_H
#define PXR_IMAGING_HD_DIRTY_LIST_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;

using HdDirtyListSharedPtr = std::shared_ptr<class HdDirtyList>;

/// \class HdDirtyList
///
/// Used for faster iteration of dirty rprims, filtered by mask.
///
/// GetDirtyRprims implicitly refresh and cache the list if needed.
/// The returning prims list will be used for sync.
///
/// DirtyList construction can expensive. We have 3 layer
/// versioning to make it efficient.
///
/// 1. Nothing changed since last time (super fast),
///   no prims need to be synced.
///   DirtyList returns empty vector GetDirtyRprims.
///   This can be detected by HdChangeTracker::GetSceneStateVersion.
///   It's incremented when any change made on any prim.
///
/// 2. Constantly updating Prims in a stable set (fast)
///   when munging or playing back, the same set of prims are being updated,
///   while the remaining prims (could be huge -- for example a large set)
///   are static.
///   Those animating prims can be distinguished by the Varying bit. The Varying
///   bit is set on a prim when any dirty bit is set, and stays even after clean
///   the dirty bit until HdChangeTracker::ResetVaryingState clears out.
///
///   DirtyList caches those prims in a list at the first time (described in 3.),
///   and returns the list for the subsequent queries. Since that list is
///   conservatively picked by the Varying bit instead of the actual DirtyBits
///   needed for various reprs, consumer of DirtyList needs to check the
///   dirtybits again (this is a common pattern in HdRprim, HdMesh and other).
///
/// 3. Varying state changed (medium cost)
///   when a exisitng prim newly starts updating (start munging), or when
///   a majority of the dirtylist stop updating, we need to reconstruct
///   the dirtylist. HdChangeTracker::GetVaryingStateVersion() tells the
///   right timing to refresh, by comparing the cached version number in
///   the dirtylist.
///
///   To construct a dirtylist, the Varying bit is checked instead of other
///   dirtybits, since effective dirtybits may differ over prims, by prim
///   type (mesh vs curve) or by per-prim repr style (flat vs smooth)
///
///   example: [x]=Varying   [x*]=Dirty,Varying
///
///    say in change tracker:
///       A B C D E [F*] [G] [H*] [I*] [J] [K] L M N ...
///    then the dirtylist will be:
///                  F*, G, H*, I*, J, K
///
///    Note that G, J and K are not dirty, but it exists in the dirtylist.
///    This optimization gives the maximum efficiency when all of Varying
///    prims are being updated.
///
/// 4. Initial creation, filter changes (most expensive)
///   If we fail to early out all the above condition, such as when we add
///   new prims or switch the render tag set, all prims should be
///   passed down to HdRenderIndex::Sync, except ones we know that are
///   completely clean. Although it requires to sweep all prims in the
///   render index, this traversal has already been optimized
///   using the Gather utility.
///
class HdDirtyList {
public:
    HD_API
    HdDirtyList(HdRprimCollection const& collection,
                 HdRenderIndex &index);
    HD_API
    ~HdDirtyList();

    /// Returns a reference of dirty ids.
    /// If the change tracker hasn't changed any state since the last time
    /// GetDirtyRprims gets called, it simply returns an empty list.
    /// Otherwise depending on what changed, it will return a list of
    /// prims to be synced.
    /// Therefore, it is expected that GetDirtyRprims is called only once
    /// per render index sync.
    HD_API
    SdfPathVector const& GetDirtyRprims();

    /// Update the tracking state for this HdDirtyList with the new collection,
    /// if the update cannot be applied, return false.
    HD_API
    bool ApplyEdit(HdRprimCollection const& newCollection);

private:
    HdRprimCollection _collection;
    SdfPathVector _dirtyIds;
    HdRenderIndex &_renderIndex;

    unsigned int _sceneStateVersion;
    unsigned int _rprimIndexVersion;
    unsigned int _renderTagVersion;
    unsigned int _varyingStateVersion;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_DIRTY_LIST_H
