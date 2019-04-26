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
        , _sceneStateVersion(
            _renderIndex.GetChangeTracker().GetSceneStateVersion() - 1)
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

    // After exploration, it was determined that the vast majority of cases
    // if we calculated the union of all the collections used in generating
    // a frame, the entire render index got Sync'ed.
    //
    // With the issue of some tasks needing Sprims to be Sync'ed before they
    // can know the include/exclude paths.  It be was decided to remove
    // the task based include/exclude filter.
    //
    // We still use the prim gather system to obtain the path list and
    // run the predicate filter.  As the include path is root and an empty
    // exclude path.  This should hit the filter's fast path.
    static const SdfPathVector includePaths = {SdfPath::AbsoluteRootPath()};
    static const SdfPathVector excludePaths;
    
    const SdfPathVector &paths        = _renderIndex.GetRprimIds();


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

    // As we don't do path filtering, changes to paths have no effect on the
    // dirtylist
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
    unsigned int currentSceneStateVersion
        = changeTracker.GetSceneStateVersion();

    // if nothing changed, and if it's clean, returns empty.
    if (_isEmpty && _sceneStateVersion == currentSceneStateVersion) {
        static SdfPathVector _EMPTY;
        return _EMPTY;
    }
    // if nothing changed, but not yet cleaned, returns the cached result.
    // this list can be either initialization-set or varying-set
    if (_sceneStateVersion == currentSceneStateVersion) {
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
    } else if (_sceneStateVersion != currentSceneStateVersion) {
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
    _sceneStateVersion = currentSceneStateVersion;
    _isEmpty = false;

    return _dirtyIds;
}

PXR_NAMESPACE_CLOSE_SCOPE

