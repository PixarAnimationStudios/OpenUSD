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
#include "pxr/imaging/hd/changeTracker.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/stackTrace.h"

#include <iostream>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

HdChangeTracker::HdChangeTracker() 
    : _rprimState()
    , _instancerState()
    , _taskState()
    , _sprimState()
    , _bprimState()
    , _generalState()
    , _collectionState()
    , _needsGarbageCollection(false)
    , _needsBprimGarbageCollection(false)
    , _instancerRprimDependencies()
    , _instancerInstancerDependencies()
    // Note: Version numbers start at 1, with observers resetting theirs to 0.
    // This is to cause a version mismatch during first-time processing.
    , _varyingStateVersion(1)
    , _rprimIndexVersion(1)
    , _sprimIndexVersion(1)
    , _bprimIndexVersion(1)
    , _instancerIndexVersion(1)
    , _sceneStateVersion(1)
    , _visChangeCount(1)
    , _renderTagVersion(1)
    , _batchVersion(1)
{
    /*NOTHING*/
}

HdChangeTracker::~HdChangeTracker()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
}

void 
HdChangeTracker::_LogCacheAccess(TfToken const& cacheName, 
                                 SdfPath const& id, 
                                 bool hit)
{
    if (hit) {
        HD_PERF_CACHE_HIT(cacheName, id);
    } else {
        HD_PERF_CACHE_MISS(cacheName, id);
    }
}

void
HdChangeTracker::RprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState)
{
    TF_DEBUG(HD_RPRIM_ADDED).Msg("Rprim Added: %s\n", id.GetText());
    _rprimState[id] = initialDirtyState;

    ++_sceneStateVersion;
    ++_rprimIndexVersion;
}

void
HdChangeTracker::RprimRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_RPRIM_REMOVED).Msg("Rprim Removed: %s\n", id.GetText());
    _rprimState.erase(id);
    // Make sure cached DrawItems get flushed out and their buffers are
    // reclaimed.
    _needsGarbageCollection = true;

    ++_sceneStateVersion;
    ++_rprimIndexVersion;
}


void 
HdChangeTracker::MarkRprimDirty(SdfPath const& id, HdDirtyBits bits)
{
    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkRprimDirty called with bits == clean!");
        return;
    }

    _IDStateMap::iterator it = _rprimState.find(id);
    if (!TF_VERIFY(it != _rprimState.end(), "%s\n", id.GetText())) {
        return;
    }

    // Early out if no new bits are being set.
    if ((bits & (~it->second)) == 0) {
        // Can not early out if the change represents
        // a change to the prim filter.  These need to
        // trigger a re-evaluation of the dirty list so need
        // certain version flags to be incremented.
        // These may not be marked clean if the prim
        // is filtered out, so don't early out!
        if ((bits & (DirtyRenderTag | DirtyRepr)) == 0) {
            return;
        }
    }

    // used ensure the repr has been created. don't touch scene state version
    if (bits == HdChangeTracker::InitRepr) {
        it->second |= HdChangeTracker::InitRepr;
        return;
    }

    // set Varying bit if it's not set
    HdDirtyBits oldBits = it->second;
    if ((oldBits & HdChangeTracker::Varying) == 0) {
        TF_DEBUG(HD_VARYING_STATE).Msg("New Varying State %s: %s\n",
                                       id.GetText(),
                                       StringifyDirtyBits(bits).c_str());

        // varying state changed.
        bits |= HdChangeTracker::Varying;
        ++_varyingStateVersion;
    }
    it->second = oldBits | bits;
    ++_sceneStateVersion;

    if ((bits & DirtyVisibility) != 0) {
        ++_visChangeCount;
    }

    if ((bits & DirtyRenderTag) != 0) {
        ++_renderTagVersion;
    }

    if ((bits & (DirtyRenderTag | DirtyRepr)) != 0) {
        // Need to treat these like a scene edits
        // For Render Tag
        //  - DirtyLists will filter out prims that don't match render tag,
        //  - Batches filter out prim that don't match render tag,
        // With Repr, it may require the new repr to be initialized
        //  - DirtyLists manages repr initialization
        //  - Batches gather only draw items that match the repr.
        // So both need to be rebuilt.
        // So increment the render index version.
        ++_rprimIndexVersion;
    }
}

void
HdChangeTracker::ResetVaryingState()
{ 
    ++_varyingStateVersion;

    // reset all variability bit
    TF_FOR_ALL (it, _rprimState) {
        if (IsClean(it->second)) {
            it->second &= ~Varying;
        }
    }
}

void
HdChangeTracker::ResetRprimVaryingState(SdfPath const& id)
{
    TF_DEBUG(HD_VARYING_STATE).Msg("Resetting Rprim Varying State: %s\n",
                                   id.GetText());

    _IDStateMap::iterator it = _rprimState.find(id);
    if (!TF_VERIFY(it != _rprimState.end(), "%s\n", id.GetText())) {
        return;
    }

    // Don't update varying state or change count as we don't want to
    // cause re-evaluation of the varying state now, but
    // want to pick up the possible change on the next iteration.

    it->second &= ~Varying;
}

void
HdChangeTracker::MarkRprimClean(SdfPath const& id, HdDirtyBits newBits)
{
    TF_DEBUG(HD_RPRIM_CLEANED).Msg("Rprim Cleaned: %s\n", id.GetText());
    _IDStateMap::iterator it = _rprimState.find(id);
    if (!TF_VERIFY(it != _rprimState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
}

void
HdChangeTracker::InstancerInserted(SdfPath const& id)
{
    TF_DEBUG(HD_INSTANCER_ADDED).Msg("Instancer Added: %s\n", id.GetText());
    _instancerState[id] = AllDirty;
    ++_sceneStateVersion;
    ++_instancerIndexVersion;
}

void
HdChangeTracker::InstancerRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_INSTANCER_REMOVED).Msg("Instancer Removed: %s\n", id.GetText());
    _instancerState.erase(id);
    ++_sceneStateVersion;
    ++_instancerIndexVersion;
}


// -------------------------------------------------------------------------- //
/// \name Dependency Tracking
// -------------------------------------------------------------------------- //

void
HdChangeTracker::AddInstancerRprimDependency(SdfPath const& instancerId,
                                             SdfPath const& rprimId)
{
    _AddDependency(_instancerRprimDependencies, instancerId, rprimId);
}

void
HdChangeTracker::RemoveInstancerRprimDependency(SdfPath const& instancerId,
                                                SdfPath const& rprimId)
{
    _RemoveDependency(_instancerRprimDependencies, instancerId, rprimId);
}

void
HdChangeTracker::AddInstancerInstancerDependency(SdfPath const& parentId,
                                                 SdfPath const& instancerId)
{
    _AddDependency(_instancerInstancerDependencies, parentId, instancerId);
}

void
HdChangeTracker::RemoveInstancerInstancerDependency(SdfPath const& parentId,
                                                    SdfPath const& instancerId)
{
    _RemoveDependency(_instancerInstancerDependencies, parentId, instancerId);
}

void
HdChangeTracker::_AddDependency(HdChangeTracker::_DependencyMap &depMap,
        SdfPath const& parent, SdfPath const& child)
{
    depMap[parent].insert(child);
}

void
HdChangeTracker::_RemoveDependency(HdChangeTracker::_DependencyMap &depMap,
        SdfPath const& parent, SdfPath const& child)
{
    _DependencyMap::iterator it = depMap.find(parent);
    if (!TF_VERIFY(it != depMap.end()))
        return;

    SdfPathSet &childSet = it->second;
    TF_VERIFY(childSet.erase(child) != 0);

    if (childSet.empty())
    {
        depMap.erase(it);
    }
}

// -------------------------------------------------------------------------- //
/// \name Task Object Tracking
// -------------------------------------------------------------------------- //

void
HdChangeTracker::TaskInserted(SdfPath const& id, HdDirtyBits initialDirtyState)
{
    TF_DEBUG(HD_TASK_ADDED).Msg("Task Added: %s\n", id.GetText());
    _taskState[id] = initialDirtyState;
    ++_sceneStateVersion;
}

void
HdChangeTracker::TaskRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_TASK_REMOVED).Msg("Task Removed: %s\n", id.GetText());
    _taskState.erase(id);
    ++_sceneStateVersion;
}

void
HdChangeTracker::MarkTaskDirty(SdfPath const& id, HdDirtyBits bits)
{
    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkTaskDirty called with bits == clean!");
        return;
    }

    _IDStateMap::iterator it = _taskState.find(id);
    if (!TF_VERIFY(it != _taskState.end(), "Task Id = %s", id.GetText())) {
        return;
    }

    if (((bits & DirtyRenderTags) != 0) &&
        ((it->second & DirtyRenderTags) == 0)) {
        MarkRenderTagsDirty();
    }

    it->second = it->second | bits;
    ++_sceneStateVersion;
}

HdDirtyBits
HdChangeTracker::GetTaskDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _taskState.find(id);
    if (!TF_VERIFY(it != _taskState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkTaskClean(SdfPath const& id, HdDirtyBits newBits)
{
    _IDStateMap::iterator it = _taskState.find(id);
    if (!TF_VERIFY(it != _taskState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
}

void
HdChangeTracker::MarkRenderTagsDirty()
{
    ++_renderTagVersion;
    ++_sceneStateVersion;
}

unsigned
HdChangeTracker::GetRenderTagVersion() const
{
    return _renderTagVersion;
}

// -------------------------------------------------------------------------- //
/// \name Instancer State Tracking
// -------------------------------------------------------------------------- //

HdDirtyBits
HdChangeTracker::GetInstancerDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _instancerState.find(id);
    if (!TF_VERIFY(it != _instancerState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkInstancerDirty(SdfPath const& id, HdDirtyBits bits)
{
    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkInstancerDirty called with bits == clean!");
        return;
    }

    _IDStateMap::iterator it = _instancerState.find(id);
    if (!TF_VERIFY(it != _instancerState.end()))
        return;

    // not calling _PropagateDirtyBits here. Currenly instancer uses
    // scale, translate, rotate primvars and there's no dependency between them
    // unlike points and normals on rprim.

#if 0
    // Early out if no new bits are being set.
    // XXX: First, we need to get rid of the usage of DirtyInstancer
    // in usdImaging.
    if ((bits & (~it->second)) == 0) {
        return;
    }
#endif

    it->second = it->second | bits;
    ++_sceneStateVersion;

    // Now mark any associated rprims or instancers dirty.
    _DependencyMap::iterator instancerDepIt =
        _instancerInstancerDependencies.find(id);
    if (instancerDepIt != _instancerInstancerDependencies.end()) {
        for (SdfPath const& dep : instancerDepIt->second) {
            MarkInstancerDirty(dep, DirtyInstancer);
        }
    }

    _DependencyMap::iterator rprimDepIt =
        _instancerRprimDependencies.find(id);
    if (rprimDepIt != _instancerRprimDependencies.end()) {
        for (SdfPath const& dep : rprimDepIt->second) {
            MarkRprimDirty(dep, DirtyInstancer);
        }
    }
}

void
HdChangeTracker::MarkInstancerClean(SdfPath const& id, HdDirtyBits newBits)
{
    TF_DEBUG(HD_INSTANCER_CLEANED).Msg("Instancer Cleaned: %s\n", id.GetText());
    _IDStateMap::iterator it = _instancerState.find(id);
    if (!TF_VERIFY(it != _instancerState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
}

// -------------------------------------------------------------------------- //
/// \name Sprim Tracking (camera, light...)
// -------------------------------------------------------------------------- //

void
HdChangeTracker::SprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState)
{
    TF_DEBUG(HD_SPRIM_ADDED).Msg("Sprim Added: %s\n", id.GetText());
    _sprimState[id] = initialDirtyState;
    ++_sceneStateVersion;
    ++_sprimIndexVersion;
}

void
HdChangeTracker::SprimRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_SPRIM_REMOVED).Msg("Sprim Removed: %s\n", id.GetText());
    _sprimState.erase(id);
    // Make sure sprim resources are reclaimed.
    _needsGarbageCollection = true;
    ++_sceneStateVersion;
    ++_sprimIndexVersion;
}

HdDirtyBits
HdChangeTracker::GetSprimDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _sprimState.find(id);
    if (!TF_VERIFY(it != _sprimState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkSprimDirty(SdfPath const& id, HdDirtyBits bits)
{
    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkSprimDirty called with bits == clean!");
        return;
    }

    _IDStateMap::iterator it = _sprimState.find(id);
    if (!TF_VERIFY(it != _sprimState.end()))
        return;
    it->second = it->second | bits;
    ++_sceneStateVersion;
}

void
HdChangeTracker::MarkSprimClean(SdfPath const& id, HdDirtyBits newBits)
{
    _IDStateMap::iterator it = _sprimState.find(id);
    if (!TF_VERIFY(it != _sprimState.end()))
        return;
    it->second = newBits;
}

// -------------------------------------------------------------------------- //
/// \name Bprim Tracking (texture, buffer...)
// -------------------------------------------------------------------------- //

void
HdChangeTracker::BprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState)
{
    TF_DEBUG(HD_BPRIM_ADDED).Msg("Bprim Added: %s\n", id.GetText());
    _bprimState[id] = initialDirtyState;
    ++_sceneStateVersion;
    ++_bprimIndexVersion;
}

void
HdChangeTracker::BprimRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_BPRIM_REMOVED).Msg("Bprim Removed: %s\n", id.GetText());
    _bprimState.erase(id);
    _needsBprimGarbageCollection = true;
    ++_sceneStateVersion;
    ++_bprimIndexVersion;
}

HdDirtyBits
HdChangeTracker::GetBprimDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _bprimState.find(id);
    if (!TF_VERIFY(it != _bprimState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkBprimDirty(SdfPath const& id, HdDirtyBits bits)
{
    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkBprimDirty called with bits == clean!");
        return;
    }

    _IDStateMap::iterator it = _bprimState.find(id);
    if (!TF_VERIFY(it != _bprimState.end()))
        return;
    it->second = it->second | bits;
    ++_sceneStateVersion;
}

void
HdChangeTracker::MarkBprimClean(SdfPath const& id, HdDirtyBits newBits)
{
    _IDStateMap::iterator it = _bprimState.find(id);
    if (!TF_VERIFY(it != _bprimState.end()))
        return;
    it->second = newBits;
}

// -------------------------------------------------------------------------- //
/// \name RPrim Object Tracking
// -------------------------------------------------------------------------- //


bool
HdChangeTracker::IsRprimDirty(SdfPath const& id)
{
    return IsDirty(GetRprimDirtyBits(id));
}

bool 
HdChangeTracker::IsTopologyDirty(SdfPath const& id)
{
    return IsTopologyDirty(GetRprimDirtyBits(id), id);
}

bool 
HdChangeTracker::IsDoubleSidedDirty(SdfPath const& id)
{
    return IsDoubleSidedDirty(GetRprimDirtyBits(id), id);
}

bool 
HdChangeTracker::IsCullStyleDirty(SdfPath const& id)
{
    return IsCullStyleDirty(GetRprimDirtyBits(id), id);
}

bool 
HdChangeTracker::IsDisplayStyleDirty(SdfPath const& id)
{
    return IsDisplayStyleDirty(GetRprimDirtyBits(id), id);
}

bool 
HdChangeTracker::IsSubdivTagsDirty(SdfPath const& id)
{
    return IsSubdivTagsDirty(GetRprimDirtyBits(id), id);
}

bool 
HdChangeTracker::IsTransformDirty(SdfPath const& id)
{
    return IsTransformDirty(GetRprimDirtyBits(id), id);
}

bool 
HdChangeTracker::IsVisibilityDirty(SdfPath const& id)
{
    return IsVisibilityDirty(GetRprimDirtyBits(id), id);
}

bool 
HdChangeTracker::IsExtentDirty(SdfPath const& id)
{
    return IsExtentDirty(GetRprimDirtyBits(id), id);
}

bool 
HdChangeTracker::IsPrimIdDirty(SdfPath const& id)
{
    return IsPrimIdDirty(GetRprimDirtyBits(id), id);
}

bool
HdChangeTracker::IsAnyPrimvarDirty(SdfPath const &id)
{
    return IsAnyPrimvarDirty(GetRprimDirtyBits(id), id);
}

bool
HdChangeTracker::IsPrimvarDirty(SdfPath const& id, TfToken const& name)
{
    return IsPrimvarDirty(GetRprimDirtyBits(id), id, name);
}

/*static*/
bool 
HdChangeTracker::IsTopologyDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyTopology) != 0;
    _LogCacheAccess(HdTokens->topology, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsDoubleSidedDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyDoubleSided) != 0;
    _LogCacheAccess(HdTokens->doubleSided, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsCullStyleDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyCullStyle) != 0;
    _LogCacheAccess(HdTokens->cullStyle, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsDisplayStyleDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyDisplayStyle) != 0;
    _LogCacheAccess(HdTokens->displayStyle, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsSubdivTagsDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtySubdivTags) != 0;
    _LogCacheAccess(HdTokens->subdivTags, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsTransformDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyTransform) != 0;
    _LogCacheAccess(HdTokens->transform, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsVisibilityDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyVisibility) != 0;
    _LogCacheAccess(HdTokens->visibility, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsExtentDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyExtent) != 0;
    _LogCacheAccess(HdTokens->extent, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsPrimIdDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyPrimID) != 0;
    _LogCacheAccess(HdTokens->primID, id, !isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsInstancerDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyInstancer) != 0;
    _LogCacheAccess(HdInstancerTokens->instancer, id, !isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsInstanceIndexDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyInstanceIndex) != 0;
    _LogCacheAccess(HdInstancerTokens->instanceIndices, id, !isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsAnyPrimvarDirty(HdDirtyBits dirtyBits, SdfPath const &id)
{
    bool isDirty = (dirtyBits & (DirtyPoints|
                                 DirtyNormals|
                                 DirtyWidths|
                                 DirtyPrimvar)) != 0;
    _LogCacheAccess(HdTokens->primvar, id, !isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsPrimvarDirty(HdDirtyBits dirtyBits, SdfPath const& id,
                                TfToken const& name)
{
    bool isDirty = false;
    if (name == HdTokens->points) {
        isDirty = (dirtyBits & DirtyPoints) != 0;
    } else if (name == HdTokens->velocities) {
        isDirty = (dirtyBits & DirtyPoints) != 0;
    } else if (name == HdTokens->accelerations) {
        isDirty = (dirtyBits & DirtyPoints) != 0;
    } else if (name == HdTokens->normals) {
        isDirty = (dirtyBits & DirtyNormals) != 0;
    } else if (name == HdTokens->widths) {
        isDirty = (dirtyBits & DirtyWidths) != 0;
    } else {
        isDirty = (dirtyBits & DirtyPrimvar) != 0;
    }
    _LogCacheAccess(name, id, !isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsReprDirty(HdDirtyBits dirtyBits, SdfPath const &id)
{
    bool isDirty = (dirtyBits & DirtyRepr) != 0;
    return isDirty;
}

void 
HdChangeTracker::MarkPrimvarDirty(SdfPath const& id, TfToken const& name)
{
    HdDirtyBits flag = Clean;
    MarkPrimvarDirty(&flag, name);
    MarkRprimDirty(id, flag);
}


void
HdChangeTracker::MarkAllRprimsDirty(HdDirtyBits bits)
{
    HD_TRACE_FUNCTION();

    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkAllRprimsDirty called with bits == clean!");
        return;
    }

    //
    // This function runs similar to calling MarkRprimDirty on every prim.
    // First it checks to see if the request will set any new dirty bits that
    // are not already set on the prim.  If there are, it will set the new bits
    // as see if the prim is in the varying state.  If it is not it will
    // transition the prim to varying.
    //
    // If any prim was transitioned to varying then the varying state version
    // counter is incremented.
    //
    // This complexity is due to some important optimizations.
    // The main case is dealing with invisible prims, but equally applies
    // to other cases where dirty bits don't get cleaned during sync.
    //
    // For these cases, we want to avoid having the prim in the dirty list
    // as there would be no work for it to do.  This is done by clearing the
    // varying flag.  On the flip-side, we want to avoid thrashing the varying
    // state, so that if the prim has an attribute that is varying, but
    // it doesn't get cleared, we don't want to set varying on that prim
    // every frame.
    //

    bool varyingStateUpdated = false;

    for (_IDStateMap::iterator it  = _rprimState.begin();
                               it != _rprimState.end(); ++it) {

        HdDirtyBits &rprimDirtyBits = it->second;

        // If RenderTag or Repr are marked dirty, we always want to update
        // the varying state (This matches the don't early out condition in
        // MarkRprim dirty).
        if ((bits & ((~rprimDirtyBits) | DirtyRenderTag | DirtyRepr)) != 0) {
            rprimDirtyBits |= bits;

            if ((rprimDirtyBits & HdChangeTracker::Varying) == 0) {
                rprimDirtyBits |= HdChangeTracker::Varying;
                varyingStateUpdated = true;
            }
        }
    }

    if (varyingStateUpdated) {
        ++_varyingStateVersion;
    }

    // These counters get updated every time, even if no prims
    // have moved into the dirty state.
    ++_sceneStateVersion;
    if ((bits & DirtyVisibility) != 0) {
        ++_visChangeCount;
    }
    if ((bits & DirtyRenderTag) != 0) {
        ++_renderTagVersion;
    }
    if ((bits & (DirtyRenderTag | DirtyRepr)) != 0) {
        // Render tags affect dirty lists and batching, so they need to be
        // treated like a scene edit: see comment in MarkRprimDirty.
        ++_rprimIndexVersion;
    }
}


/*static*/
void
HdChangeTracker::MarkPrimvarDirty(HdDirtyBits *dirtyBits, TfToken const &name)
{
    HdDirtyBits setBits = Clean;
    if (name == HdTokens->points) {
        setBits = DirtyPoints;
    } else if (name == HdTokens->normals) {
        setBits = DirtyNormals;
    } else if (name == HdTokens->widths) {
        setBits = DirtyWidths;
    } else {
        setBits = DirtyPrimvar;
    }
    *dirtyBits |= setBits;
}

HdDirtyBits
HdChangeTracker::GetRprimDirtyBits(SdfPath const& id) const
{
    _IDStateMap::const_iterator it = _rprimState.find(id);
    if (!TF_VERIFY(it != _rprimState.end()))
        return Clean;

    // not masking the varying bit, since we use that bit
    // in HdRenderIndex::GetDelegateIDsWithDirtyRprims to extract
    // all varying rprims.
    return it->second;// & (~Varying);
}

void 
HdChangeTracker::AddCollection(TfToken const& collectionName)
{
    HD_TRACE_FUNCTION();

    _CollectionStateMap::iterator it = _collectionState.find(collectionName);
    // if it already exists, just return.
    if (it != _collectionState.end()) {
        return;
    }
    _collectionState[collectionName] = 1;
}

void 
HdChangeTracker::MarkCollectionDirty(TfToken const& collectionName)
{
    HD_TRACE_FUNCTION();

    _CollectionStateMap::iterator it = _collectionState.find(collectionName);
    if (!TF_VERIFY(it != _collectionState.end(),
                      "Collection %s not found\n", collectionName.GetText())) {
        return;
    }
    // bump the version number
    it->second += 1;

    ++_sceneStateVersion;
}

unsigned
HdChangeTracker::GetCollectionVersion(TfToken const& collectionName) const
{
    _CollectionStateMap::const_iterator it = _collectionState.find(collectionName);
    if (!(it != _collectionState.end())) {
        TF_CODING_ERROR("Change Tracker unable to find collection %s",
                        collectionName.GetText());
        return _rprimIndexVersion;
    }
    return it->second + _rprimIndexVersion;
}

unsigned
HdChangeTracker::GetVisibilityChangeCount() const
{
    return _visChangeCount;
}

void
HdChangeTracker::MarkBatchesDirty()
{
    ++_batchVersion;
}

unsigned
HdChangeTracker::GetBatchVersion() const
{
    return _batchVersion;
}

void
HdChangeTracker::AddState(TfToken const& name)
{
    _GeneralStateMap::iterator it = _generalState.find(name);
    if (it != _generalState.end()) {
        // mark state dirty
        ++it->second;
    } else {
        _generalState[name] = 1;
    }
}

void
HdChangeTracker::MarkStateDirty(TfToken const& name)
{
    _GeneralStateMap::iterator it = _generalState.find(name);
    if (it != _generalState.end()) {
        ++it->second;
    } else {
        TF_CODING_ERROR("Change Tracker unable to find state %s",
                        name.GetText());
    }
}

unsigned
HdChangeTracker::GetStateVersion(TfToken const &name) const
{
    _GeneralStateMap::const_iterator it = _generalState.find(name);
    if (it != _generalState.end()) {
        return it->second;
    } else {
        TF_CODING_ERROR("Change Tracker unable to find state %s",
                        name.GetText());
        return 0;
    }
}

/*static*/
std::string
HdChangeTracker::StringifyDirtyBits(HdDirtyBits dirtyBits)
{
    if (dirtyBits == HdChangeTracker::Clean) {
        return std::string("Clean");
    }

    std::stringstream ss;

    if (dirtyBits & Varying) {
        ss << "<Varying> ";
    }
    if (dirtyBits & InitRepr) {
        ss << "<InitRepr> ";
    }
    if (dirtyBits & DirtyPrimID) {
        ss << " PrimID ";
    }
    if (dirtyBits & DirtyExtent) {
        ss << "Extent ";
    }
    if (dirtyBits & DirtyDisplayStyle) {
        ss << "DisplayStyle ";
    }
    if (dirtyBits & DirtyPoints) {
        ss << "Points ";
    }
    if (dirtyBits & DirtyPrimvar) {
        ss << "Primvar ";
    }
    if (dirtyBits & DirtyMaterialId) {
        ss << "MaterialId ";
    }
    if (dirtyBits & DirtyTopology) {
        ss << "Topology ";
    }
    if (dirtyBits & DirtyTransform) {
        ss << "Transform ";
    }
    if (dirtyBits & DirtyVisibility) {
        ss << "Visibility ";
    }
    if (dirtyBits & DirtyNormals) {
        ss << "Normals ";
    }
    if (dirtyBits & DirtyDoubleSided) {
        ss << "DoubleSided ";
    }
    if (dirtyBits & DirtyCullStyle) {
        ss << "CullStyle ";
    }
    if (dirtyBits & DirtySubdivTags) {
        ss << "SubdivTags "; 
    }
    if (dirtyBits & DirtyWidths) {
        ss << "Widths ";
    }
    if (dirtyBits & DirtyInstancer) {
        ss << "Instancer ";
    }
    if (dirtyBits & DirtyInstanceIndex) {
        ss << "InstanceIndex ";
    }
    if (dirtyBits & DirtyRepr) {
        ss << "Repr ";
    }
    if (dirtyBits & DirtyCategories) {
        ss << "Categories ";
    }
    if (dirtyBits & ~AllSceneDirtyBits) {
        ss << "CustomBits:";
        for (size_t i = CustomBitsBegin; i <= CustomBitsEnd; i<<=1) {
            ss << ((dirtyBits & i) ? "1" : "0");
        }
    }
    return ss.str();
}

/*static*/
void
HdChangeTracker::DumpDirtyBits(HdDirtyBits dirtyBits)
{
    std::cerr
        << "DirtyBits:"
        << HdChangeTracker::StringifyDirtyBits(dirtyBits)
        << "\n";
}

PXR_NAMESPACE_CLOSE_SCOPE

