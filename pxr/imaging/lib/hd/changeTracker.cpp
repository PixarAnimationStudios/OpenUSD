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

HdChangeTracker::HdChangeTracker() 
    : _needsGarbageCollection(false)
    , _instancerRprimMap()
    , _varyingStateVersion(1)
    , _indexVersion(0)
    , _changeCount(1)       // changeCount in DirtyList starts from 0.
    , _visChangeCount(1)    // Clients (commandBuffer) start from 0.
    , _shaderBindingsVersion(1)
    , _drawTargetSetVersion(1) // Clients (draw target task) start from 0.
{
    /*NOTHING*/
}

HdChangeTracker::~HdChangeTracker()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();
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
HdChangeTracker::RprimInserted(SdfPath const& id, int initialDirtyState)
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

/*static*/
HdChangeTracker::DirtyBits
HdChangeTracker::_PropagateDirtyBits(DirtyBits bits)
{
    // propagate point dirtiness to normal
    bits |= (bits & DirtyPoints ? DirtyNormals : 0);

    // when refine level changes, topology becomes dirty.
    // XXX: can we remove DirtyRefineLevel then?
    if (bits & DirtyRefineLevel) {
        bits |=  DirtyTopology;
    }

    // if topology changes, all dependent bits become dirty.
    if (bits & DirtyTopology) {
        bits |= (DirtyPoints|
                 DirtyNormals|
                 DirtyPrimVar);
    }
    return bits;
}

void 
HdChangeTracker::MarkRprimDirty(SdfPath const& id, DirtyBits bits)
{
    bits = _PropagateDirtyBits(bits);

    _IDStateMap::iterator it = _rprimState.find(id);
    if (not TF_VERIFY(it != _rprimState.end(), "%s\n", id.GetText()))
        return;

    // used for force-sync on new repr. don't touch changeCount
    if (bits == HdChangeTracker::ForceSync) {
        it->second |= HdChangeTracker::ForceSync;
        return;
    }

    // set Varying bit if it's not set
    DirtyBits oldBits = it->second;
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
HdChangeTracker::MarkRprimClean(SdfPath const& id, DirtyBits newBits)
{
    TF_DEBUG(HD_RPRIM_CLEANED).Msg("Rprim Cleaned: %s\n", id.GetText());
    _IDStateMap::iterator it = _rprimState.find(id);
    if (not TF_VERIFY(it != _rprimState.end()))
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
    if (not TF_VERIFY(it != _instancerRprimMap.end()))
        return;

    SdfPathSet &rprimSet = it->second;

    TF_VERIFY(rprimSet.erase(rprimId) != 0);

    if (rprimSet.empty())
    {
        _instancerRprimMap.erase(it);
    }
}

// -------------------------------------------------------------------------- //
/// \name Shader Object Tracking
// -------------------------------------------------------------------------- //

void
HdChangeTracker::ShaderInserted(SdfPath const& id)
{
    TF_DEBUG(HD_SHADER_ADDED).Msg("Shader Added: %s\n", id.GetText());
    _shaderState[id] = AllDirty;
}

void
HdChangeTracker::ShaderRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_SHADER_REMOVED).Msg("Shader Removed: %s\n", id.GetText());
    _shaderState.erase(id);
}

void
HdChangeTracker::MarkShaderDirty(SdfPath const& id, DirtyBits bits)
{
    _IDStateMap::iterator it = _shaderState.find(id);
    if (not TF_VERIFY(it != _shaderState.end()))
        return;
    it->second = it->second | bits;
}

HdChangeTracker::DirtyBits
HdChangeTracker::GetShaderDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _shaderState.find(id);
    if (not TF_VERIFY(it != _shaderState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkShaderClean(SdfPath const& id, DirtyBits newBits)
{
    _IDStateMap::iterator it = _shaderState.find(id);
    if (not TF_VERIFY(it != _shaderState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
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
HdChangeTracker::MarkTaskDirty(SdfPath const& id, DirtyBits bits)
{
    _IDStateMap::iterator it = _taskState.find(id);
    if (not TF_VERIFY(it != _taskState.end()))
        return;
    it->second = it->second | bits;
}

HdChangeTracker::DirtyBits
HdChangeTracker::GetTaskDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _taskState.find(id);
    if (not TF_VERIFY(it != _taskState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkTaskClean(SdfPath const& id, DirtyBits newBits)
{
    _IDStateMap::iterator it = _taskState.find(id);
    if (not TF_VERIFY(it != _taskState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
}

// -------------------------------------------------------------------------- //
/// \name Texture Object Tracking
// -------------------------------------------------------------------------- //

void
HdChangeTracker::TextureInserted(SdfPath const& id)
{
    TF_DEBUG(HD_TEXTURE_ADDED).Msg("Texture Added: %s\n", id.GetText());
    _textureState[id] = AllDirty;
}

void
HdChangeTracker::TextureRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_TEXTURE_REMOVED).Msg("Texture Removed: %s\n", id.GetText());
    _textureState.erase(id);
}

void
HdChangeTracker::MarkTextureDirty(SdfPath const& id, DirtyBits bits)
{
    _IDStateMap::iterator it = _textureState.find(id);
    if (not TF_VERIFY(it != _textureState.end()))
        return;
    it->second = it->second | bits;
}

HdChangeTracker::DirtyBits
HdChangeTracker::GetTextureDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _textureState.find(id);
    if (not TF_VERIFY(it != _textureState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkTextureClean(SdfPath const& id, DirtyBits newBits)
{
    _IDStateMap::iterator it = _textureState.find(id);
    if (not TF_VERIFY(it != _textureState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
}

// -------------------------------------------------------------------------- //
/// \name Instancer State Tracking
// -------------------------------------------------------------------------- //

HdChangeTracker::DirtyBits
HdChangeTracker::GetInstancerDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _instancerState.find(id);
    if (not TF_VERIFY(it != _instancerState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkInstancerDirty(SdfPath const& id, DirtyBits bits)
{
    _IDStateMap::iterator it = _instancerState.find(id);
    if (not TF_VERIFY(it != _instancerState.end()))
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
HdChangeTracker::MarkInstancerClean(SdfPath const& id, DirtyBits newBits)
{
    TF_DEBUG(HD_INSTANCER_CLEANED).Msg("Instancer Cleaned: %s\n", id.GetText());
    _IDStateMap::iterator it = _instancerState.find(id);
    if (not TF_VERIFY(it != _instancerState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
}

// -------------------------------------------------------------------------- //
/// \name Camera Object Tracking
// -------------------------------------------------------------------------- //

void
HdChangeTracker::CameraInserted(SdfPath const& id)
{
    TF_DEBUG(HD_CAMERA_ADDED).Msg("Camera Added: %s\n", id.GetText());
    _cameraState[id] = AllDirty;
}

void
HdChangeTracker::CameraRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_CAMERA_REMOVED).Msg("Camera Removed: %s\n", id.GetText());
    _cameraState.erase(id);
}

HdChangeTracker::DirtyBits
HdChangeTracker::GetCameraDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _cameraState.find(id);
    if (not TF_VERIFY(it != _cameraState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkCameraDirty(SdfPath const& id, DirtyBits bits)
{
    _IDStateMap::iterator it = _cameraState.find(id);
    if (not TF_VERIFY(it != _cameraState.end()))
        return;
    it->second = it->second | bits;
}

void
HdChangeTracker::MarkCameraClean(SdfPath const& id, DirtyBits newBits)
{
    _IDStateMap::iterator it = _cameraState.find(id);
    if (not TF_VERIFY(it != _cameraState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
}

// -------------------------------------------------------------------------- //
/// \name Light Object Tracking
// -------------------------------------------------------------------------- //

void
HdChangeTracker::LightInserted(SdfPath const& id)
{
    TF_DEBUG(HD_LIGHT_ADDED).Msg("Light Added: %s\n", id.GetText());
    _lightState[id] = AllDirty;
}

void
HdChangeTracker::LightRemoved(SdfPath const& id)
{
    TF_DEBUG(HD_LIGHT_REMOVED).Msg("Light Removed: %s\n", id.GetText());
    _lightState.erase(id);
}

HdChangeTracker::DirtyBits
HdChangeTracker::GetLightDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _lightState.find(id);
    if (not TF_VERIFY(it != _lightState.end()))
        return Clean;
    return it->second;
}

void
HdChangeTracker::MarkLightDirty(SdfPath const& id, DirtyBits bits)
{
    _IDStateMap::iterator it = _lightState.find(id);
    if (not TF_VERIFY(it != _lightState.end()))
        return;
    it->second = it->second | bits;
}

void
HdChangeTracker::MarkLightClean(SdfPath const& id, DirtyBits newBits)
{
    _IDStateMap::iterator it = _lightState.find(id);
    if (not TF_VERIFY(it != _lightState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
}

// -------------------------------------------------------------------------- //
/// \name Draw Target Object Tracking
// -------------------------------------------------------------------------- //

void
HdChangeTracker::DrawTargetInserted(SdfPath const& id)
{
    _drawTargetState[id] = AllDirty;
    ++_drawTargetSetVersion;
}

void
HdChangeTracker::DrawTargetRemoved(SdfPath const& id)
{
    _drawTargetState.erase(id);
    ++_drawTargetSetVersion;
}

HdChangeTracker::DirtyBits
HdChangeTracker::GetDrawTargetDirtyBits(SdfPath const& id)
{
    _IDStateMap::iterator it = _drawTargetState.find(id);
    if (not TF_VERIFY(it != _drawTargetState.end())) {
        return Clean;
    }
    return it->second;
}

void
HdChangeTracker::MarkDrawTargetDirty(SdfPath const& id, DirtyBits bits)
{
    _IDStateMap::iterator it = _drawTargetState.find(id);
    if (not TF_VERIFY(it != _drawTargetState.end())) {
        return;
    }
    it->second = it->second | bits;

    if (bits & DirtyDTEnable) {
        ++_drawTargetSetVersion;
    }
}

unsigned
HdChangeTracker::GetDrawTargetSetVersion()
{
    return _drawTargetSetVersion;
}

void
HdChangeTracker::MarkDrawTargetClean(SdfPath const& id, DirtyBits newBits)
{
    _IDStateMap::iterator it = _drawTargetState.find(id);
    if (not TF_VERIFY(it != _drawTargetState.end()))
        return;
    // preserve the variability bit
    it->second = (it->second & Varying) | newBits;
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
HdChangeTracker::IsTopologyDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyTopology);
    _LogCacheAccess(HdTokens->topology, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsDoubleSidedDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyDoubleSided);
    _LogCacheAccess(HdTokens->doubleSided, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsCullStyleDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyCullStyle);
    _LogCacheAccess(HdTokens->cullStyle, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsRefineLevelDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyRefineLevel);
    _LogCacheAccess(HdTokens->refineLevel, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsSubdivTagsDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtySubdivTags);
    _LogCacheAccess(HdTokens->subdivTags, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsTransformDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyTransform);
    _LogCacheAccess(HdTokens->transform, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsVisibilityDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyVisibility);
    _LogCacheAccess(HdTokens->visibility, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsExtentDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyExtent);
    _LogCacheAccess(HdTokens->extent, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsPrimIdDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyPrimID);
    _LogCacheAccess(HdTokens->primID, id, not isDirty);
    return isDirty;
}

/*static*/
bool 
HdChangeTracker::IsInstancerDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyInstancer);
    _LogCacheAccess(HdTokens->instancer, id, not isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsInstanceIndexDirty(DirtyBits dirtyBits, SdfPath const& id)
{
    bool isDirty = bool(dirtyBits & DirtyInstanceIndex);
    _LogCacheAccess(HdTokens->instanceIndices, id, not isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsAnyPrimVarDirty(DirtyBits dirtyBits, SdfPath const &id)
{
    bool isDirty = bool(dirtyBits & (DirtyPoints|
                                     DirtyNormals|
                                     DirtyWidths|
                                     DirtyPrimVar));
    _LogCacheAccess(HdTokens->primVar, id, not isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsPrimVarDirty(DirtyBits dirtyBits, SdfPath const& id,
                                TfToken const& name)
{
    bool isDirty = false;
    if (name == HdTokens->points) {
        isDirty = dirtyBits & DirtyPoints;
    } else if (name == HdTokens->normals) {
        isDirty = dirtyBits & DirtyNormals;
    } else if (name == HdTokens->widths) {
        isDirty = dirtyBits & DirtyWidths;
    } else {
        isDirty = dirtyBits & DirtyPrimVar;
    }
    _LogCacheAccess(name, id, not isDirty);
    return isDirty;
}

/*static*/
bool
HdChangeTracker::IsReprDirty(DirtyBits dirtyBits, SdfPath const &id)
{
    bool isDirty = bool(dirtyBits & DirtyRepr);
    return isDirty;
}

void 
HdChangeTracker::MarkPrimVarDirty(SdfPath const& id, TfToken const& name)
{
    DirtyBits flag = Clean;
    MarkPrimVarDirty(&flag, name);
    MarkRprimDirty(id, flag);
}

/*static*/
void
HdChangeTracker::MarkPrimVarDirty(DirtyBits *dirtyBits, TfToken const &name)
{
    DirtyBits setBits = Clean;
    if (name == HdTokens->points) {
        setBits = DirtyPoints;
    } else if (name == HdTokens->normals) {
        setBits = DirtyNormals;
    } else if (name == HdTokens->widths) {
        setBits = DirtyWidths;
    } else {
        setBits = DirtyPrimVar;
    }
    *dirtyBits |= _PropagateDirtyBits(setBits);
}

HdChangeTracker::DirtyBits
HdChangeTracker::GetRprimDirtyBits(SdfPath const& id) const
{
    _IDStateMap::const_iterator it = _rprimState.find(id);
    if (not TF_VERIFY(it != _rprimState.end()))
        return Clean;

    // not masking the varying bit, since we use that bit
    // in HdRenderIndex::GetDelegateIDsWithDirtyRprims to extract
    // all varying rprims.
    return it->second;// & (~Varying);
}

void 
HdChangeTracker::AddCollection(TfToken const& collectionName)
{
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
    _CollectionStateMap::iterator it = _collectionState.find(collectionName);
    if (not TF_VERIFY(it != _collectionState.end(),
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
    ++_indexVersion;
    ++_varyingStateVersion;

    // Also force DirtyLists to refresh: 
    // This is needed in the event that a collection changes due to changes in
    // the delegate's scene graph, but those changes have no direct effect on
    // the RenderIndex.
    ++_changeCount;
}

unsigned
HdChangeTracker::GetCollectionVersion(TfToken const& collectionName)
{
    _CollectionStateMap::iterator it = _collectionState.find(collectionName);
    if (not (it != _collectionState.end())) {
        TF_CODING_ERROR("Change Tracker unable to find collection %s",
                        collectionName.GetText());
        return _indexVersion;
    }
    return it->second + _indexVersion;
}

unsigned
HdChangeTracker::GetVisibilityChangeCount()
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

/*static*/
std::string
HdChangeTracker::StringifyDirtyBits(int dirtyBits)
{
    if (dirtyBits == HdChangeTracker::Clean) {
        return std::string("Clean");
    }

    std::stringstream ss;

    if (dirtyBits & Varying) {
        ss << "<Varying> ";
    }
    if (dirtyBits & ForceSync) {
        ss << "<ForceSync> ";
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
    if (dirtyBits & DirtySurfaceShader) {
        ss << "SurfaceShader ";
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
HdChangeTracker::DumpDirtyBits(int dirtyBits)
{
    std::cerr
        << "DirtyBits:"
        << HdChangeTracker::StringifyDirtyBits(dirtyBits)
        << "\n";
}
