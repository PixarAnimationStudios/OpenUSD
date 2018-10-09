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
#include "pxr/imaging/hd/dirtyList.h"

#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

struct _FilterParam {
    const HdRprimCollection &collection;
    const HdRenderIndex     &renderIndex;
    HdDirtyBits              mask;
};

static bool
_DirtyListFilterPredicate(const SdfPath &rprimID, const void *predicateParam)
{
    const _FilterParam *filterParam =
                              static_cast<const _FilterParam *>(predicateParam);

    const HdRprimCollection &collection = filterParam->collection;
    const HdRenderIndex &renderIndex    = filterParam->renderIndex;
    HdDirtyBits mask                    = filterParam->mask;

    HdChangeTracker const &tracker = renderIndex.GetChangeTracker();

   if (mask == 0 || tracker.GetRprimDirtyBits(rprimID) & mask) {
       if (collection.HasRenderTag(renderIndex.GetRenderTag(rprimID))) {
           return true;
       }
   }

   return false;
}



HdDirtyList::HdDirtyList(HdRprimCollection const& collection,
                  HdRenderIndex & index)
        : _collection(collection)
        , _renderIndex(index)
        , _collectionVersion(
            _renderIndex.GetChangeTracker().GetCollectionVersion(_collection.GetName()) - 1)
        , _varyingStateVersion(
            _renderIndex.GetChangeTracker().GetVaryingStateVersion() - 1)
        , _changeCount(
            _renderIndex.GetChangeTracker().GetChangeCount() - 1)
        , _isEmpty(false)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->dirtyLists);
}

HdDirtyList::~HdDirtyList()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HD_PERF_COUNTER_DECR(HdPerfTokens->dirtyLists);
}

void
HdDirtyList::_UpdateIDs(SdfPathVector* ids, HdDirtyBits mask)
{
    HD_TRACE_FUNCTION();
    HD_PERF_COUNTER_INCR(HdPerfTokens->dirtyListsRebuilt);

    
    const SdfPathVector &paths        = _renderIndex.GetRprimIds();
    const SdfPathVector &includePaths = _collection.GetRootPaths();
    const SdfPathVector &excludePaths = _collection.GetExcludePaths();

    _FilterParam filterParam = {_collection, _renderIndex, mask};

    HdPrimGather gather;

    gather.PredicatedFilter(paths,
                  includePaths,
                  excludePaths,
                  _DirtyListFilterPredicate,
                  &filterParam,
                  ids);
}

void
HdDirtyList::Clear()
{
    HdChangeTracker &changeTracker = _renderIndex.GetChangeTracker();
    unsigned int currentCollectionVersion
        = changeTracker.GetCollectionVersion(_collection.GetName());

    TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList(%p): Clear()"
            "(collection: %s, ver: %d, cur-ver: %d)\n",
            (void*)this,
            _collection.GetName().GetText(),
            _collectionVersion,
            currentCollectionVersion);

    if (_collectionVersion != currentCollectionVersion) {
        unsigned int currentVaryingStateVersion
            = changeTracker.GetVaryingStateVersion();
        // we just cleaned the initialization set.
        // this collection is clean, and the next step is to find out stable
        // varying set.
        _collectionVersion = currentCollectionVersion;
        _varyingStateVersion = currentVaryingStateVersion - 1;
    }

    // in any case, this list is now clean until the changeCount changes.
    // Don't clear dirtyIds so that we can reuse the saved list for
    // the next stable change (playback) rather than rebuilding again.
    _isEmpty = true;
}

bool
HdDirtyList::ApplyEdit(HdRprimCollection const& col)
{
    HD_TRACE_FUNCTION();

    // Don't attempt to transition dirty lists where the collection
    // fundamentally changed, we can't reused filtered paths in those cases.
    //
    // when repr changes, don't reuse the dirty list, since the required
    // DirtyBits may change.
    if (col.GetName() != _collection.GetName()
        || col.GetReprSelector() != _collection.GetReprSelector()
        || col.IsForcedRepr() != _collection.IsForcedRepr()
        || col.GetRenderTags() != _collection.GetRenderTags()) {
        return false;
    }

    // Also don't attempt to fix-up dirty lists when the collection is radically
    // different in terms of root paths; here a heuristic of 100 root paths is
    // used as a threshold for when we will stop attempting to fix the list.
    if (std::abs(int(col.GetRootPaths().size()) -
                 int(_collection.GetRootPaths().size())) > 100) {
        return false;
    }

    // If the either the old or new collection has Exclude paths do
    // the full rebuild.
    if (!col.GetExcludePaths().empty() ||
        !_collection.GetExcludePaths().empty()) {
        return false;
    }

    // If the varying state has changed - Rebuild the base list
    // before adding the new items
    HdChangeTracker &changeTracker = _renderIndex.GetChangeTracker();

    unsigned int currentVaryingStateVersion =
                                         changeTracker.GetVaryingStateVersion();

    if (_varyingStateVersion != currentVaryingStateVersion) {
           TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList(%p): varying state changed "
                   "(%s, %d -> %d)\n",
                   (void*)this,
                   _collection.GetName().GetText(),
                   _varyingStateVersion,
                   currentVaryingStateVersion);

           // populate only varying prims in the collection
           _UpdateIDs(&_dirtyIds, HdChangeTracker::Varying);
           _varyingStateVersion = currentVaryingStateVersion;
    }

    SdfPathVector added, removed;
    typedef SdfPathVector::const_iterator ITR;
    ITR newI = col.GetRootPaths().cbegin();
    ITR newEnd = col.GetRootPaths().cend();
    ITR oldI = _collection.GetRootPaths().cbegin();
    ITR oldEnd = _collection.GetRootPaths().cend();
    HdRenderIndex& index = _renderIndex;

    TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList(%p): ApplyEdit\n", (void*)this);

    if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {
        std::cout << "  Old Collection: " << std::endl;
        for (auto const& i : _collection.GetRootPaths()) {
            std::cout << "    " << i << std::endl;
        }
        std::cout << "  Old _dirtyIds: " << std::endl;
        for (auto const& i : _dirtyIds) {
            std::cout << "    " << i << std::endl;
        }
    }

    const SdfPathVector &paths = _renderIndex.GetRprimIds();

    while (newI != newEnd || oldI != oldEnd) {
        if (newI != newEnd && oldI != oldEnd && *newI == *oldI) {
            ++newI;
            ++oldI;
            continue;
        }
        // If any paths in the two sets are prefixed by one another, the logic
        // below doesn't work, since the subtree has to be fixed up (it's not
        // just a simple prefix scan). In these cases, we'll just rebuild the
        // entire list.
        if (newI != newEnd && oldI != oldEnd && newI->HasPrefix(*oldI)) {
            return false;          
        }
        if (newI != newEnd && oldI != oldEnd && oldI->HasPrefix(*newI)) {
            return false;          
        }
        if (newI != newEnd && (oldI == oldEnd || *newI < *oldI)) {
            HdPrimGather gather;

            SdfPathVector newPaths;
            gather.Subtree(paths, *newI, &newPaths);

            size_t numNewPaths = newPaths.size();
            for (size_t newPathNum = 0;
                        newPathNum < numNewPaths;
                      ++newPathNum) {
                const SdfPath &newPath = newPaths[newPathNum];

                if (col.HasRenderTag(index.GetRenderTag(newPath))) {
                    _dirtyIds.push_back(newPath);
                    changeTracker.MarkRprimDirty(newPath,
                                                 HdChangeTracker::InitRepr);
                }
            }
            ++newI;
        } else if (oldI != oldEnd) { 
            // oldI < newI: Item removed in new list
            SdfPath const& oldPath = *oldI;
            _dirtyIds.erase(std::remove_if(_dirtyIds.begin(), _dirtyIds.end(), 
                  [&oldPath](SdfPath const& p){return p.HasPrefix(oldPath);}),
                  _dirtyIds.end());
            ++oldI;
        }
    }

    _collection = col;
    _collectionVersion
                    = changeTracker.GetCollectionVersion(_collection.GetName());


    // make sure the next GetDirtyRprims() picks up the updated list.
    _isEmpty = false;

    if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {
        std::cout << "  New Collection: " << std::endl;
        for (auto const& i : _collection.GetRootPaths()) {
            std::cout << "    " << i << std::endl;
        }
        std::cout << "  New _dirtyIds: " << std::endl;
        for (auto const& i : _dirtyIds) {
            std::cout << "    " << i << std::endl;
        }
    }

    return true;
}

SdfPathVector const&
HdDirtyList::GetDirtyRprims()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    /*
       HdDirtyList has 3-states:
          - initialization list (any dirty bits)
          - stable varying list (Varying bit)
          - empty               (isEmpty = true)

                                            MarkDirtyStable -----------+
                                                   ^                   |
  [init list build] <-+- CollectionChange          |                   |
         |            ^          ^                 |                   |
         v            |          |                 |    +-------+      |
     +---------+      |          +<----------------+<---| empty |      |
     |init list|--> MarkDirty    |                 |    +-------+      |
     +---------+                 |                 |        ^       [reuse]
         |                       |                 |        |          |
       Clean                     v                 |      Clean        |
         |                MarkDirtyUnstable        |        ^          |
         v                       |                 |        |          |
     +-------+                   |                 |        |          |
     | empty |                   |             +---------------+       |
     +-------+                   |             |  varying list | <-----+
         |                       |             +---------------+
      MarkDirty                  |                   ^
         |                       v                   |
         +----------->  [varying list build] --------+
    */

    // see if there's any variability change or not.
    HdChangeTracker &changeTracker = _renderIndex.GetChangeTracker();

    unsigned int currentCollectionVersion
        = changeTracker.GetCollectionVersion(_collection.GetName());
    unsigned int currentVaryingStateVersion
        = changeTracker.GetVaryingStateVersion();
    unsigned int currentChangeCount
        = changeTracker.GetChangeCount();

    // if nothing changed, and if it's clean, returns empty.
    if (_isEmpty && _changeCount == currentChangeCount) {
        static SdfPathVector _EMPTY;
        return _EMPTY;
    }
    // if nothing changed, but not yet cleaned, returns the cached result.
    // this list can be either initialization-set or varying-set
    if (_changeCount == currentChangeCount) {
        return _dirtyIds;
    }

    if (_collectionVersion != currentCollectionVersion) {
        TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList(%p): collection version"
                " changed (%s, %d -> %d)\n",
                (void*)this,
                _collection.GetName().GetText(),
                _collectionVersion, currentCollectionVersion);

        // populate dirty rprims in the collection
        _UpdateIDs(&_dirtyIds, 0);
        TF_FOR_ALL(it, _dirtyIds) {
            changeTracker.MarkRprimDirty(*it, HdChangeTracker::InitRepr);
        }

        // this is very conservative list and is expected to be rebuilt
        // once it gets cleaned.
        //
        // Don't update _collectionVersion so that Clear() can detect that
        // we'll need to build varying set next.
    } else if (_varyingStateVersion != currentVaryingStateVersion) {
        TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList(%p): varying state changed "
                "(%s, %d -> %d)\n",
                (void*)this,
                _collection.GetName().GetText(),
                _varyingStateVersion,
                currentVaryingStateVersion);

        // populate only varying prims in the collection
        _UpdateIDs(&_dirtyIds, HdChangeTracker::Varying);
        _varyingStateVersion = currentVaryingStateVersion;
    } else if (_changeCount != currentChangeCount) {
        // reuse the existing varying prims list.
        // note that the varying prims list may contain cleaned rprims,
        // clients still need to ask the actual dirtyBits to ChangeTracker
    }

    if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {
        std::cout << "  Collection: " << std::endl;
        for (auto const& i : _collection.GetRootPaths()) {
            std::cout << "    " << i << std::endl;
        }
        std::cout << "  _dirtyIds: " << std::endl;
        for (auto const& i : _dirtyIds) {
            std::cout << "    " << i << std::endl;
        }
    }


    // this dirtyList reflects the latest state of change tracker.
    _changeCount = currentChangeCount;
    _isEmpty = false;

    return _dirtyIds;
}

PXR_NAMESPACE_CLOSE_SCOPE

