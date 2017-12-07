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

#include <iostream>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


HdChangeTracker::HdChangeTracker() 
    : _rprimState()
    , _instancerState()
    , _taskState()
    , _sprimState()
    , _bprimState()
    , _extComputationState()
    , _generalState()
    , _collectionState()
    , _needsGarbageCollection(false)
    , _instancerRprimMap()
    , _varyingStateVersion(1)
    , _indexVersion(0)
    , _changeCount(1)       // changeCount in DirtyList starts from 0.
    , _visChangeCount(1)    // Clients (commandBuffer) start from 0.
    , _shaderBindingsVersion(1)
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

    // Make sure cached DrawItems get flushed out.
    ++_changeCount;
    ++_indexVersion;
    ++_varyingStateVersion;
}

void
HdChangeTracker::RprimRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_RPRIM_REMOVED).Msg("Rprim Removed: %s\n", id.GetText());
    _rprimState.erase(id);
    // Make sure cached DrawItems get flushed out and their buffers are
    // reclaimed.
    _needsGarbageCollection = true;
    ++_changeCount;
    ++_indexVersion;
    ++_varyingStateVersion;
}


void 
HdChangeTracker::MarkRprimDirty(SdfPath const& id, HdDirtyBits bits)
{
    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkRprimDirty called with bits == clean!");
        return;
    }

    _IDStateMap::iterator it = _rprimState.find(id);
    if (!TF_VERIFY(it != _rprimState.end(), "%s\n", id.GetText()))
        return;

    // used ensure the repr has been created. don't touch changeCount
    if (bits == HdChangeTracker::InitRepr) {
        it->second |= HdChangeTracker::InitRepr;
        return;
    }

    // set Varying bit if it's not set
    HdDirtyBits oldBits = it->second;
    if ((oldBits & HdChangeTracker::Varying) == 0) {
        // varying state changed.
        bits |= HdChangeTracker::Varying;
        ++_varyingStateVersion;
    }
    it->second = oldBits | bits;
    ++_changeCount;

    if (bits & DirtyVisibility) 
        ++_visChangeCount;
}

void
HdChangeTracker::ResetVaryingState()
{ 
    ++_varyingStateVersion;
    ++_changeCount;

    // reset all variability bit
    TF_FOR_ALL (it, _rprimState) {
        it->second &= ~Varying;
    }
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
}

void
HdChangeTracker::InstancerRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_INSTANCER_REMOVED).Msg("Instancer Removed: %s\n", id.GetText());
    _instancerState.erase(id);
}

void
HdChangeTracker::InstancerRPrimInserted(SdfPath const& instancerId,
                                        SdfPath const& rprimId)
{
    _instancerRprimMap[instancerId].insert(rprimId);
}

void
HdChangeTracker::InstancerRPrimRemoved(SdfPath const& instancerId, SdfPath const& rprimId)
{
    _InstancerRprimMap::iterator it = _instancerRprimMap.find(instancerId);
    if (!TF_VERIFY(it != _instancerRprimMap.end()))
        return;

    SdfPathSet &rprimSet = it->second;

    TF_VERIFY(rprimSet.erase(rprimId) != 0);

    if (rprimSet.empty())
    {
        _instancerRprimMap.erase(it);
    }
}

// -------------------------------------------------------------------------- //
/// \name Task Object Tracking
// -------------------------------------------------------------------------- //

void
HdChangeTracker::TaskInserted(SdfPath const& id)
{
    TF_DEBUG(HD_TASK_ADDED).Msg("Task Added: %s\n", id.GetText());
    _taskState[id] = AllDirty;
}

void
HdChangeTracker::TaskRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_TASK_REMOVED).Msg("Task Removed: %s\n", id.GetText());
    _taskState.erase(id);
}

void
HdChangeTracker::MarkTaskDirty(SdfPath const& id, HdDirtyBits bits)
{
    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkTaskDirty called with bits == clean!");
        return;
    }

    _IDStateMap::iterator it = _taskState.find(id);
    if (!TF_VERIFY(it != _taskState.end()))
        return;
    it->second = it->second | bits;
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
    it->second = it->second | bits;

    // Now mark any associated rprims dirty.
    _InstancerRprimMap::iterator mapIt = _instancerRprimMap.find(id);
    if (mapIt != _instancerRprimMap.end()) {
        SdfPathSet &rprimSet = mapIt->second;

        for (SdfPathSet::iterator rprimIt =  rprimSet.begin();
                                  rprimIt != rprimSet.end();
                                  ++rprimIt) {
            MarkRprimDirty(*rprimIt, DirtyInstancer);
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
}

void
HdChangeTracker::SprimRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_SPRIM_REMOVED).Msg("Sprim Removed: %s\n", id.GetText());
    _sprimState.erase(id);
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
}

void
HdChangeTracker::BprimRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_BPRIM_REMOVED).Msg("Bprim Removed: %s\n", id.GetText());
    _bprimState.erase(id);
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
}

void
HdChangeTracker::MarkBprimClean(SdfPath const& id, HdDirtyBits newBits)
{
    _IDStateMap::iterator it = _bprimState.find(id);
    if (!TF_VERIFY(it != _bprimState.end()))
        return;
    it->second = newBits;
}

// ---------------------------------------------------------------------- //
/// \name ExtComputation Object Tracking
// ---------------------------------------------------------------------- //
void
HdChangeTracker::ExtComputationInserted(SdfPath const& id,
                                        HdDirtyBits initialDirtyState)
{
    TF_DEBUG(HD_EXT_COMPUTATION_ADDED).Msg("ExtComputation Added: %s\n",
                                           id.GetText());
    _extComputationState[id] = initialDirtyState;
}

void
HdChangeTracker::ExtComputationRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_EXT_COMPUTATION_REMOVED).Msg("ExtComputation Removed: %s\n",
                                             id.GetText());
    _extComputationState.erase(id);
}

void
HdChangeTracker::MarkExtComputationDirty(SdfPath const& id, HdDirtyBits bits)
{
    if (ARCH_UNLIKELY(bits == HdChangeTracker::Clean)) {
        TF_CODING_ERROR("MarkExtComputationDirty called with bits == clean!");
        return;
    }

    _IDStateMap::iterator it = _extComputationState.find(id);
    if (!TF_VERIFY(it != _extComputationState.end()))
        return;
    it->second = it->second | bits;
}

HdDirtyBits
HdChangeTracker::GetExtComputationDirtyBits(SdfPath const& id) const
{
    _IDStateMap::const_iterator it = _extComputationState.find(id);
    if (!TF_VERIFY(it != _extComputationState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkExtComputationClean(SdfPath const& id, HdDirtyBits newBits)
{
    _IDStateMap::iterator it = _extComputationState.find(id);
    if (!TF_VERIFY(it != _extComputationState.end()))
        return;

    it->second =  newBits;
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
HdChangeTracker::IsRefineLevelDirty(SdfPath const& id)
{
    return IsRefineLevelDirty(GetRprimDirtyBits(id), id);
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
HdChangeTracker::IsAnyPrimVarDirty(SdfPath const &id)
{
    return IsAnyPrimVarDirty(GetRprimDirtyBits(id), id);
}

bool
HdChangeTracker::IsPrimVarDirty(SdfPath const& id, TfToken const& name)
{
    return IsPrimVarDirty(GetRprimDirtyBits(id), id, name);
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
HdChangeTracker::IsRefineLevelDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyRefineLevel) != 0;
    _LogCacheAccess(HdTokens->refineLevel, id, !isDirty);
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
    _LogCacheAccess(HdTokens->instancer, id, !isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsInstanceIndexDirty(HdDirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = (dirtyBits & DirtyInstanceIndex) != 0;
    _LogCacheAccess(HdTokens->instanceIndices, id, !isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsAnyPrimVarDirty(HdDirtyBits dirtyBits, SdfPath const &id)
{
    bool isDirty = (dirtyBits & (DirtyPoints|
                                 DirtyNormals|
                                 DirtyWidths|
                                 DirtyPrimVar)) != 0;
    _LogCacheAccess(HdTokens->primVar, id, !isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsPrimVarDirty(HdDirtyBits dirtyBits, SdfPath const& id,
                                TfToken const& name)
{
    bool isDirty = false;
    if (name == HdTokens->points) {
        isDirty = (dirtyBits & DirtyPoints) != 0;
    } else if (name == HdTokens->normals) {
        isDirty = (dirtyBits & DirtyNormals) != 0;
    } else if (name == HdTokens->widths) {
        isDirty = (dirtyBits & DirtyWidths) != 0;
    } else {
        isDirty = (dirtyBits & DirtyPrimVar) != 0;
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
HdChangeTracker::MarkPrimVarDirty(SdfPath const& id, TfToken const& name)
{
    HdDirtyBits flag = Clean;
    MarkPrimVarDirty(&flag, name);
    MarkRprimDirty(id, flag);
}


void
HdChangeTracker::MarkAllRprimsDirty(HdDirtyBits bits)
{
    HD_TRACE_FUNCTION();

    for (_IDStateMap::iterator it  = _rprimState.begin();
                               it != _rprimState.end(); ++it) {
        it->second |= bits;
    }

    ++_changeCount;

    if (bits & DirtyVisibility) {
        ++_visChangeCount;
    }
}


/*static*/
void
HdChangeTracker::MarkPrimVarDirty(HdDirtyBits *dirtyBits, TfToken const &name)
{
    HdDirtyBits setBits = Clean;
    if (name == HdTokens->points) {
        setBits = DirtyPoints;
    } else if (name == HdTokens->normals) {
        setBits = DirtyNormals;
    } else if (name == HdTokens->widths) {
        setBits = DirtyWidths;
    } else {
        setBits = DirtyPrimVar;
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

    // Also force DirtyLists to refresh: 
    // This is needed in the event that a collection changes due to changes in
    // the delegate's scene graph, but those changes have no direct effect on
    // the RenderIndex.
    ++_changeCount;
}

void 
HdChangeTracker::MarkAllCollectionsDirty()
{
    HD_TRACE_FUNCTION();

    ++_indexVersion;
    ++_varyingStateVersion;

    // Also force DirtyLists to refresh: 
    // This is needed in the event that a collection changes due to changes in
    // the delegate's scene graph, but those changes have no direct effect on
    // the RenderIndex.
    ++_changeCount;
}

unsigned
HdChangeTracker::GetCollectionVersion(TfToken const& collectionName) const
{
    _CollectionStateMap::const_iterator it = _collectionState.find(collectionName);
    if (!(it != _collectionState.end())) {
        TF_CODING_ERROR("Change Tracker unable to find collection %s",
                        collectionName.GetText());
        return _indexVersion;
    }
    return it->second + _indexVersion;
}

unsigned
HdChangeTracker::GetVisibilityChangeCount() const
{
    return _visChangeCount;
}

void
HdChangeTracker::MarkShaderBindingsDirty()
{
    ++_shaderBindingsVersion;
}

unsigned
HdChangeTracker::GetShaderBindingsVersion() const
{
    return _shaderBindingsVersion;
}

unsigned
HdChangeTracker::GetRenderIndexVersion() const
{
    return _indexVersion;
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
    if (dirtyBits & DirtyRefineLevel) {
        ss << "RefineLevel ";
    }
    if (dirtyBits & DirtyPoints) {
        ss << "Points ";
    }
    if (dirtyBits & DirtyPrimVar) {
        ss << "PrimVar ";
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

