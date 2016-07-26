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
#ifndef HD_DIRTY_LIST_H
#define HD_DIRTY_LIST_H

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/rprimCollection.h"

class HdRenderIndex;

typedef boost::shared_ptr<class HdDirtyList> HdDirtyListSharedPtr;
typedef boost::weak_ptr<class HdDirtyList> HdDirtyListPtr;

/// HdDirtyList is used for faster iteration of dirty rprims,
/// filtered by mask.
///
/// GetDirtyRprims/GetSize implicitly refresh and cache the list if needed.
/// The returning prims list will be used for sync.
///
/// DirtyList construction is tend to be expensive. We have 3 layer
/// versioning to make it efficient.
///
/// 1. Nothing changed on rprims since last time (super fast)
///   when orbiting a camera around, no prims need to be synced.
///   DirtyList returns empty vector GetDirtyRprims.
///   This can be detected by HdChangeTracker::GetChangeCount. It's incremented
///   when any change made on any prim.
///
/// 2. Constantly updating Prims in a stable set (fast)
///   when munging or playing back, the same set of prims are being updated,
///   while the remaining prims (could be huge -- entire cityset) are static.
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
///    and a collection has:
///               E  F  G  H
///    then the dirtylist will be:
///                  F*, G, H*
///
///    Note that G is not dirty, but it exists in the dirtylist.
///    This optimization gives the maximum efficiency when all of Varying
///    prims are being updated.
///
/// 4. Initial creation, collection changes (most expensive)
///   If we fail to early out all the above condition, such as when we add
///   new prims or switch to new repr, all prims in a collection should be
///   passed down to HdRenderIndex::Sync, except ones we know that are
///   completely clean. Although it requires to sweep all prims in a collection,
///   this traversal has already been optimized to some extent in
///   _FilterByRootPaths and we can still leverage that code.
///

class HdDirtyList {
public:
    HdDirtyList(HdRprimCollection const& collection,
                 HdRenderIndex &index);
    ~HdDirtyList();

    /// Return the collection associated to this dirty list
    HdRprimCollection const &GetCollection() const {
        return _collection;
    }

    /// Returns a reference of dirty ids.
    /// If the change tracker hasn't changed any state since the last time
    /// GetDirtyRprims gets called, it simply returns; Otherwise, refreshes
    /// the dirty ID list and returns it.
    SdfPathVector const& GetDirtyRprims();

    /// Return the number of dirty prims in the list.
    size_t GetSize() {
        return GetDirtyRprims().size();
    }

    /// Update the tracking state for this HdDirtyList with the new collection,
    /// if the update cannot be applied, return false.
    bool ApplyEdit(HdRprimCollection const& newCollection);

    /// Clears the dirty list, while preserving stable dirty state.
    void Clear();

private:
    void _UpdateIDs(SdfPathVector* ids, HdChangeTracker::DirtyBits mask);

    HdRprimCollection _collection;
    SdfPathVector _dirtyIds;
    HdRenderIndex &_renderIndex;

    unsigned int _collectionVersion;
    unsigned int _varyingStateVersion;
    unsigned int _changeCount;
    bool _isEmpty;
    bool _reprDirty;
};

#endif  // HD_DIRTY_LIST_H

