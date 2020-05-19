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

PXR_NAMESPACE_OPEN_SCOPE


HdDirtyList::HdDirtyList(HdRprimCollection const& collection,
                  HdRenderIndex & index)
        : _collection(collection)
        , _dirtyIds()
        , _renderIndex(index)
        , _sceneStateVersion(
            _renderIndex.GetChangeTracker().GetSceneStateVersion() - 1)
        , _rprimIndexVersion(
            _renderIndex.GetChangeTracker().GetRprimIndexVersion() - 1)
        , _renderTagVersion(
            _renderIndex.GetChangeTracker().GetRenderTagVersion() - 1)
        , _varyingStateVersion(
            _renderIndex.GetChangeTracker().GetVaryingStateVersion() - 1)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->dirtyLists);
}

HdDirtyList::~HdDirtyList()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HD_PERF_COUNTER_DECR(HdPerfTokens->dirtyLists);
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
        || col.IsForcedRepr() != _collection.IsForcedRepr()) {
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
          - Init                (any dirty bits)
          - Empty               (no changes)
          - Stable              (varying)

                              +---------+
        Start State O ----->  |  Init   |
                              +---------+
                                |     ^
                                |     |    Scene State Changed &&
                                |     |    Filter Params Changed
                                |     |    -----------------------
                                |     |    Build Init List,
                                V     |    Invalidate cached varying state.
                              +---------+
                              |  Empty  |
                              +---------+
        Scene State Changed && |   ^   |    Scene State Changed &&
     !Varying State Changed && |   |   |    Varying State Changed &&
     !Filter Params Changed    |   |   |   !Filter Params Changed
        -------------------    |   |   |   ------------------------
        Reused cached varying  |   |   |   Rebuild cached varying state list
        state list             |   |   |
                               V   |   V
                              +---------+
                              | Stable  |
                              +---------+

        Transitions to the empty state are automatic after the list has been
        returned.

        "Filter Params Changed" represent are the tracked parameters that
         effect the gather operation.  This is the render tag version and
         the * index version.
    */

    // see if there's any variability change or not.
    HdChangeTracker &changeTracker = _renderIndex.GetChangeTracker();


    unsigned int currentSceneStateVersion =
            changeTracker.GetSceneStateVersion();

    // The scene state hasn't changed since the last call.
    // Nothing to do.
    // This could happen in progressive rendering or in multi-viewer scenarios.
    // XXX: This could be caught earlier and avoid Sync altogether.
    if (_sceneStateVersion == currentSceneStateVersion) {
        TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList(%p): Scene State the same %d\n",
                                    (void*)this,
                                    _sceneStateVersion);

        static SdfPathVector _EMPTY;
        return _EMPTY;
    }
    _sceneStateVersion = currentSceneStateVersion;

    // Something has change, So which of the 3 possible transitions:
    //
    //  - New Prims Added/Removed (either because of a structural change
    //                             or a filter change)
    //  - Varying Set Changed
    //  - Time Step               (neither of the above)

    unsigned int currentRprimIndexVersion =
            changeTracker.GetRprimIndexVersion();
    unsigned int currentRenderTagVersion =
            changeTracker.GetRenderTagVersion();
    unsigned int currentVaryingStateVersion =
            changeTracker.GetVaryingStateVersion();

    if ((_rprimIndexVersion != currentRprimIndexVersion) ||
        (_renderTagVersion != currentRenderTagVersion))  {
        TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList(%p): Filter Changed:\n"
                "  (Rprim Index Version %d -> %d)\n"
                "  (Render Tag Version %d -> %d)\n",
                (void*)this,
                _rprimIndexVersion, currentRprimIndexVersion,
                _renderTagVersion, currentRenderTagVersion);

        _rprimIndexVersion = currentRprimIndexVersion;
        _renderTagVersion  = currentRenderTagVersion;

        // Get list including all dirty prims
        _dirtyIds = _renderIndex._GetDirtyRprimIds(0);

        // Need to invalidate the cache varying state
        _varyingStateVersion = currentVaryingStateVersion - 1;
    } else if (_varyingStateVersion != currentVaryingStateVersion) {
        TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList(%p): varying state changed "
                "(%d -> %d)\n",
                (void*)this,
                _varyingStateVersion,
                currentVaryingStateVersion);

        _varyingStateVersion = currentVaryingStateVersion;

        // Get list only with prims in varying state
        _dirtyIds = _renderIndex._GetDirtyRprimIds(HdChangeTracker::Varying);
    }
    // If not either of the above, we can used the cached results.

    return _dirtyIds;
}

PXR_NAMESPACE_CLOSE_SCOPE

