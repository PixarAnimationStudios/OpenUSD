//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DIRTY_LIST_H
#define PXR_IMAGING_HD_DIRTY_LIST_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class HdChangeTracker;
using HdReprSelectorVector = std::vector<HdReprSelector>;

/// \class HdDirtyList
///
/// Used for faster iteration of dirty Rprims by the render index.
///
/// GetDirtyRprims() implicitly refreshes and caches the list if needed.
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
///   bit is set on a prim when any dirty bit is set, and stays even after
///   cleaning the scene dirty bits, until HdChangeTracker::ResetVaryingState
///   clears it out.
///
///   DirtyList caches those prims in a list at the first time (described in 3),
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
    explicit HdDirtyList(HdRenderIndex &index);

    /// Returns a reference of dirty rprim ids.
    /// If the change tracker hasn't changed any state since the last time
    /// GetDirtyRprims gets called, and if the tracked filtering parameters
    /// (set via UpdateRenderTagsAndReprSelectors) are the same, it simply
    /// returns an empty list.
    /// Otherwise depending on what changed, it will return a list of
    /// Rprim ids to be synced.
    /// Therefore, it is expected that GetDirtyRprims is called _only once_
    /// per render index sync.
    HD_API
    SdfPathVector const& GetDirtyRprims();

    /// Updates the tracked filtering parameters.
    /// This typically comes from the tasks submitted to HdEngine::Execute.
    HD_API
    void UpdateRenderTagsAndReprSelectors(
        TfTokenVector const &tags, HdReprSelectorVector const &reprs);

    /// Sets the flag to prune to dirty list to just the varying Rprims on
    /// the next call to GetDirtyRprims.
    void PruneToVaryingRprims() {
        _pruneDirtyList = true;
    }

private:
    HdChangeTracker & _GetChangeTracker() const;
    void _UpdateDirtyIdsIfNeeded();

    // Note: Can't use a const ref to the renderIndex because
    // HdRenderIndex::GetRprimIds() isn't a const member fn.
    HdRenderIndex &_renderIndex;
    TfTokenVector _trackedRenderTags;
    HdReprSelectorVector _trackedReprs;
    SdfPathVector _dirtyIds;

    unsigned int _sceneStateVersion;
    unsigned int _rprimIndexVersion;
    unsigned int _rprimRenderTagVersion;
    unsigned int _varyingStateVersion;

    bool _rebuildDirtyList;
    bool _pruneDirtyList;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_DIRTY_LIST_H
