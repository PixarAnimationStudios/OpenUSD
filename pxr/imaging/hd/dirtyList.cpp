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
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

struct _DirtyFilterParam {
    HdRenderIndex* renderIndex;
    const TfTokenVector& renderTags;
    HdDirtyBits mask;
};

static
bool
_DirtyRprimIdsFilterPredicate(
        const SdfPath& rprimID,
        const void* predicateParam)
{
    const _DirtyFilterParam* filterParam =
        static_cast<const _DirtyFilterParam*>(predicateParam);

    HdRenderIndex* renderIndex = filterParam->renderIndex;
    const HdDirtyBits mask = filterParam->mask;

    const HdChangeTracker& tracker = renderIndex->GetChangeTracker();
    const HdDirtyBits bits = tracker.GetRprimDirtyBits(rprimID);
    
    if (mask == HdChangeTracker::Clean || bits & mask) {
        // Update the render tag if needed.
        const TfToken& primRenderTag =
            renderIndex->UpdateRenderTag(rprimID, bits);

        // XXX An empty render tag set means everything passes the filter
        //     We should use an explicit token to indicate all render tags.
        //     When aggregating render tags from the tasks, an empty render
        //     tag opinion would get lost if a non-empty opinion exists.
        //     Primary user is tests, but some single task render delegates
        //     that don't support render tags yet also use it.
        if (filterParam->renderTags.empty()) {
            return true;
        }

        // As the number of tags is expected to be low (<10)
        // use a simple linear search.
        const size_t numRenderTags = filterParam->renderTags.size();
        for (size_t tagNum = 0u; tagNum < numRenderTags; ++tagNum) {
            if (filterParam->renderTags[tagNum] == primRenderTag) {
                return true;
            }
        }
   }

   return false;
}
//------------------------------------------------------------------------------

HdDirtyList::HdDirtyList(HdRenderIndex & index)
    : _renderIndex(index)
    , _sceneStateVersion(_GetChangeTracker().GetSceneStateVersion() - 1)
    , _rprimIndexVersion(_GetChangeTracker().GetRprimIndexVersion() - 1)
    , _rprimRenderTagVersion(_GetChangeTracker().GetRenderTagVersion() - 1)
    , _varyingStateVersion(_GetChangeTracker().GetVaryingStateVersion() - 1)
    , _rebuildDirtyList(false)
    , _pruneDirtyList(false)
{
}

SdfPathVector const&
HdDirtyList::GetDirtyRprims()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // The scene state version captures any changes to the render index and/or
    // any of its prims.
    const unsigned int currentSceneStateVersion =
                        _GetChangeTracker().GetSceneStateVersion();

    // If the scene state hasn't changed and the tracked filters
    // (render tags, reprs) are the same, all Rprims are up-to-date.
    // Instead of returning the cached _dirtyIds, return an empty vector.
    // This may happen in progressive rendering or in multi-viewer scenarios
    // wherein the HdRenderIndex::SyncAll is invoked multiple times.
    if (_sceneStateVersion == currentSceneStateVersion &&
        !_pruneDirtyList &&
        !_rebuildDirtyList) {
        // NOTE: Don't clear _dirtyIds. Its result is valuable and may be reused
        // when existing varying Rprims are alone dirtied.
        TF_DEBUG(HD_DIRTY_LIST).Msg(
            "DirtyList: Scene (%d) state version and filters unchanged.\n",
            _sceneStateVersion);

        static SdfPathVector _EMPTY;
        return _EMPTY;
    }

    _sceneStateVersion = currentSceneStateVersion;

    _UpdateDirtyIdsIfNeeded();
    
    return _dirtyIds;
}

static std::ostream &
operator <<(std::ostream &os, HdReprSelectorVector const &reprs)
{
    os << "[";
    for (auto const &repr : reprs) {
        os << repr << ", ";
    }
    os << "]";
    return os;
}

void
HdDirtyList::UpdateRenderTagsAndReprSelectors(
    TfTokenVector const & tags, HdReprSelectorVector const &reprs)
{
    bool trackedRenderTagsChanged = false;

    // Grow the tracked render tags set if necessary.
    // XXX The additive only nature of this policy can result in more Rprims
    // being sync'd than necessary.
    {
        // See comment in_DirtyRprimIdsFilterPredicate re: empty render tags.
        TRACE_SCOPE("Render tag combine");
        TfTokenVector combinedRenderTags;
        std::set_union(_trackedRenderTags.cbegin(),
                       _trackedRenderTags.cend(),
                       tags.cbegin(),
                       tags.cend(),
                       std::back_inserter(combinedRenderTags));

        if (_trackedRenderTags != combinedRenderTags) {
            _trackedRenderTags.swap(combinedRenderTags);
            trackedRenderTagsChanged = true;
        }
    }

    // Grow the tracked reprs set if possible.
    // We need to guarantee that all Rprims have had the chance to initialize
    // the tracked reprs. This in unfortunate and means that we need to reset
    // the tracked reprs when we can't guarantee that.
    // XXX This may result in rebuilding the dirty list more often.
    bool trackedReprsChanged = false;
    {
        TRACE_SCOPE("Repr selector combine");

        unsigned int currentRprimIndexVersion =
            _GetChangeTracker().GetRprimIndexVersion();
        unsigned int currentRprimRenderTagVersion =
            _GetChangeTracker().GetRenderTagVersion();
        if (trackedRenderTagsChanged ||
            _rprimIndexVersion != currentRprimIndexVersion ||
            _rprimRenderTagVersion != currentRprimRenderTagVersion) {
            
            // Reset tracked repr set.
            // XXX An alternative is to grow the tracked repr set similar to 
            //     render tags (above). This will require the render index to
            //     sync the tracked reprs rather than ones requested by the
            //     tasks.
            if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {
                std::stringstream ss;
                ss << "Resetting tracked reprs in dirty list from "
                   << _trackedReprs << " to " << reprs << "\n";
                TfDebug::Helper().Msg(ss.str());
            }
            _trackedReprs = reprs;
            trackedReprsChanged = true;
        } else {
            // Combine.
            HdReprSelectorVector combinedReprs;
            std::set_union(_trackedReprs.cbegin(),
                        _trackedReprs.cend(),
                        reprs.cbegin(),
                        reprs.cend(),
                        std::back_inserter(combinedReprs));

            if (_trackedReprs != combinedReprs) {
                _trackedReprs.swap(combinedReprs);
                trackedReprsChanged = true;
            }
        }
    }

    if (trackedRenderTagsChanged || trackedReprsChanged) {
        _rebuildDirtyList = true;
    }
}

HdChangeTracker &
HdDirtyList::_GetChangeTracker() const
{
    return _renderIndex.GetChangeTracker();
}

void
HdDirtyList::_UpdateDirtyIdsIfNeeded()
{
    // NOTE: We omit sceneStateVersion here,  since it is bumped on up any
    // change to the render index and/or its prims. See relevant comment in
    // GetDirtyRprims().
    const unsigned int currentRprimIndexVersion =
            _GetChangeTracker().GetRprimIndexVersion();
    const unsigned int currentRprimRenderTagVersion =
            _GetChangeTracker().GetRenderTagVersion();
    const unsigned int currentVaryingStateVersion =
            _GetChangeTracker().GetVaryingStateVersion();
    const bool gatherAllRprims =
                _rebuildDirtyList ||
                _rprimIndexVersion != currentRprimIndexVersion ||
                _rprimRenderTagVersion != currentRprimRenderTagVersion;

    const bool gatherVaryingRprims =
                _pruneDirtyList ||
                _varyingStateVersion != currentVaryingStateVersion;

    const bool reuseDirtyIds = !(gatherAllRprims || gatherVaryingRprims);
    if (reuseDirtyIds) {
        TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList: reusing cached dirtyIds\n");
        return;
    }

    HD_PERF_COUNTER_INCR(HdPerfTokens->dirtyListsRebuilt);

    HdDirtyBits mask = HdChangeTracker::AllSceneDirtyBits;
    // Figure out if we need to gather all Rprims (not just the dirty ones) or
    // just the varying ones.
    {
        if (gatherAllRprims)  {
            
            TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList: Filter Changed:\n"
                    "  (Rprim Index Version %d -> %d)\n"
                    "  (Render Tag Version %d -> %d)\n"
                    "  (Tracked Render Tags or Reprs changed %d)\n",
                    _rprimIndexVersion, currentRprimIndexVersion,
                    _rprimRenderTagVersion, currentRprimRenderTagVersion,
                    _rebuildDirtyList);

            _rprimIndexVersion = currentRprimIndexVersion;
            _rprimRenderTagVersion  = currentRprimRenderTagVersion;
            _varyingStateVersion = currentVaryingStateVersion;
            _rebuildDirtyList = false;
            _pruneDirtyList = true; // Trim the dirty list to just the varying
                                    // ids on the next iteration.

            // XXX: Clean is interpreted as an all-pass filter. See
            // _DirtyRprimIdsFilterPredicate
            mask = HdChangeTracker::Clean;

        } else if (gatherVaryingRprims) {
            TF_DEBUG(HD_DIRTY_LIST).Msg("DirtyList: varying state version "
                    "(%d -> %d)\n", _varyingStateVersion,
                    currentVaryingStateVersion);

            _varyingStateVersion = currentVaryingStateVersion;
            _pruneDirtyList = false;

            mask = HdChangeTracker::Varying;
        } else {
            TF_WARN("Unhandled scenario in dirty list update logic.\n");
        }
    }

    // Historial notes:
    // The dirty list logic factored the collection include/exclude paths
    // when it was owned by the render pass.
    // 
    // After exploration, it was determined that the vast majority of cases
    // if we calculated the union of all the collections used in generating
    // a frame, the entire render index got Sync'ed.
    //
    // With the issue of some tasks needing Sprims to be Sync'ed before they
    // can know the include/exclude paths, the collection based include/exclude 
    // filters were removed.
    // We still use the prim gather system to obtain the path list and
    // run the predicate filter.  As the include path is root and an empty
    // exclude path, this should hit the filter's fast path.

    static const SdfPathVector includePaths = {SdfPath::AbsoluteRootPath()};
    static const SdfPathVector excludePaths;
    const SdfPathVector & rprimPaths = _renderIndex.GetRprimIds();

    _DirtyFilterParam filterParam = {&_renderIndex, _trackedRenderTags, mask};
    _dirtyIds.clear();

    HdPrimGather gather;
    gather.PredicatedFilter(
        rprimPaths,
        includePaths,
        excludePaths,
        _DirtyRprimIdsFilterPredicate,
        &filterParam,
        &_dirtyIds);
    
    if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {

        std::cout << "Dirty list filter predicate:\n";
        std::cout << "  Render tags [";
        for (auto const &tag : filterParam.renderTags) {
            std::cout << tag << ", ";
        }
        std::cout << "]" << std::endl;
        std::cout << "  Mask : " << filterParam.mask << std::endl;
    }

    if (mask == HdChangeTracker::Clean) {
        TRACE_SCOPE("InitRepr post dirty gather");
        // XXX This is unfortunate but necessary for repr initialization in
        //     Storm.
        // There may be new prims in the list that might have reprs they've not
        // seen before. Flag these up as needing reevaluation.
        for (const SdfPath& dirtyRprimId : _dirtyIds) {
            _GetChangeTracker().MarkRprimDirty(
                dirtyRprimId, HdChangeTracker::InitRepr);
        }
    }

    if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {
        TF_DEBUG(HD_DIRTY_LIST).Msg("  dirtyRprimIds:\n");
        for (const SdfPath& dirtyRprimId : _dirtyIds) {
            TF_DEBUG(HD_DIRTY_LIST).Msg("    %s\n", dirtyRprimId.GetText());
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

