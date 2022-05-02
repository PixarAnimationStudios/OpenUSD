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
#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/dataSourceLegacyPrim.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/prefixingSceneIndex.h"
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/pyLock.h"

#include <iostream>
#include <mutex>
#include <unordered_set>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_ENABLE_SCENE_INDEX_EMULATION, true,
                      "Enable scene index emulation in the render index.");

static bool
_IsEnabledSceneIndexEmulation()
{
    static bool enabled = 
        (TfGetEnvSetting(HD_ENABLE_SCENE_INDEX_EMULATION) == true);
    return enabled;
}

bool
HdRenderIndex::IsSceneIndexEmulationEnabled()
{
    return _IsEnabledSceneIndexEmulation();
}

HdRenderIndex::HdRenderIndex(
    HdRenderDelegate *renderDelegate,
    HdDriverVector const& drivers)
    : _renderDelegate(renderDelegate)
    , _drivers(drivers)
    , _rprimDirtyList(*this)
{
    // Note: HdRenderIndex::New(...) guarantees renderDelegate is non-null.

    _rprimPrimIdMap.reserve(128);

    // Register well-known reprs (to be deprecated).
    static std::once_flag reprsOnce;
    std::call_once(reprsOnce, _ConfigureReprs);

    // Register well-known collection types (to be deprecated)
    // XXX: for compatibility and smooth transition,
    //      leave geometry collection for a while.
    _tracker.AddCollection(HdTokens->geometry);

    // Let render delegate choose drivers its interested in
    renderDelegate->SetDrivers(drivers);

    // Register the prim types our render delegate supports.
    _InitPrimTypes();
    // Create fallback prims.
    _CreateFallbackPrims();

    // If we need to emulate a scene index we create the 
    // data structures now.
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex = HdLegacyPrimSceneIndex::New();
        _emulationNoticeBatchingSceneIndex =
            HdNoticeBatchingSceneIndex::New(_emulationSceneIndex);
        _mergingSceneIndex = HdMergingSceneIndex::New();
        _mergingSceneIndex->AddInputScene(
            _emulationNoticeBatchingSceneIndex, SdfPath::AbsoluteRootPath());

        HdSceneIndexBaseRefPtr terminalSceneIndex = _mergingSceneIndex;

        terminalSceneIndex =
            HdSceneIndexAdapterSceneDelegate::AppendDefaultSceneFilters(
                terminalSceneIndex, SdfPath::AbsoluteRootPath());

        const std::string &rendererDisplayName =
            renderDelegate->GetRendererDisplayName();

        if (!rendererDisplayName.empty()) {
            terminalSceneIndex =
                HdSceneIndexPluginRegistry::GetInstance()
                    .AppendSceneIndicesForRenderer(
                        rendererDisplayName, terminalSceneIndex);
        }

        _siSd = std::make_unique<HdSceneIndexAdapterSceneDelegate>(
            terminalSceneIndex, 
            this, 
            SdfPath::AbsoluteRootPath());

        _tracker._SetTargetSceneIndex(get_pointer(_emulationSceneIndex));
    }
}

HdRenderIndex::~HdRenderIndex()
{
    HD_TRACE_FUNCTION();
    
    // Get rid of prims first.
    Clear();

    // Delete the emulated scene index datastructures
    // (although they should be depopulated already by Clear).
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex.Reset();
        _siSd.reset();
    }

    _DestroyFallbackPrims();
}

HdRenderIndex*
HdRenderIndex::New(
    HdRenderDelegate *renderDelegate,
    HdDriverVector const& drivers)
{
    if (renderDelegate == nullptr) {
        TF_CODING_ERROR(
            "Null Render Delegate provided to create render index");
        return nullptr;
    }
    return new HdRenderIndex(renderDelegate, drivers);
}

void
HdRenderIndex::InsertSceneIndex(
        HdSceneIndexBaseRefPtr inputSceneIndex,
        SdfPath const& scenePathPrefix)
{
    if (!_IsEnabledSceneIndexEmulation()) {
        TF_WARN("Unable to add scene index at prefix %s because emulation is off.",
                scenePathPrefix.GetText());
        return;
    }

    if (scenePathPrefix != SdfPath::AbsoluteRootPath()) {
        inputSceneIndex = HdPrefixingSceneIndex::New(
                inputSceneIndex, scenePathPrefix);
    }
    _mergingSceneIndex->AddInputScene(
            inputSceneIndex, scenePathPrefix);
}

void
HdRenderIndex::RemoveSceneIndex(
        HdSceneIndexBaseRefPtr inputSceneIndex)
{
    if (!_IsEnabledSceneIndexEmulation()) {
        return;
    }

    _mergingSceneIndex->RemoveInputScene(inputSceneIndex);
}

void
HdRenderIndex::RemoveSubtree(const SdfPath &root,
                             HdSceneDelegate* sceneDelegate)
{
    HD_TRACE_FUNCTION();

    // Remove tasks here, since they aren't part of emulation.
    _RemoveTaskSubtree(root, sceneDelegate);

    // If we're using emulation, RemoveSubtree is routed through scene indices.
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->RemovePrims({root});
        return;
    }
    
    _RemoveSubtree(root, sceneDelegate);
}

void
HdRenderIndex::_RemoveSubtree(
    const SdfPath &root, 
    HdSceneDelegate* sceneDelegate)
{
    HD_TRACE_FUNCTION();

    _RemoveRprimSubtree(root, sceneDelegate);
    _sprimIndex.RemoveSubtree(root, sceneDelegate, _tracker, _renderDelegate);
    _bprimIndex.RemoveSubtree(root, sceneDelegate, _tracker, _renderDelegate);
    _RemoveInstancerSubtree(root, sceneDelegate);
}


void
HdRenderIndex::InsertRprim(TfToken const& typeId,
                 HdSceneDelegate* sceneDelegate,
                 SdfPath const& rprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // If we are using emulation, we will need to populate 
    // a data source with the prim information
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->AddLegacyPrim(rprimId, typeId, sceneDelegate);
        return;
    }

    _InsertRprim(typeId, sceneDelegate, rprimId);
}

void 
HdRenderIndex::_InsertRprim(TfToken const& typeId,
                            HdSceneDelegate* sceneDelegate,
                            SdfPath const& rprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (ARCH_UNLIKELY(TfMapLookupPtr(_rprimMap, rprimId))) {
        return;
    }

    SdfPath const &sceneDelegateId = sceneDelegate->GetDelegateID();
    if (!rprimId.HasPrefix(sceneDelegateId)) {
        TF_CODING_ERROR("Scene Delegate Id (%s) must prefix prim Id (%s)",
                        sceneDelegateId.GetText(), rprimId.GetText());
        return;
    }

    HdRprim *rprim = _renderDelegate->CreateRprim(typeId, rprimId);
    if (rprim == nullptr) {
        return;
    }

    _rprimIds.Insert(rprimId);

    // Force an initial "renderTag" sync.  We add the bit here since the
    // render index manages render tags, rather than the rprim implementation.
    _tracker.RprimInserted(rprimId, rprim->GetInitialDirtyBitsMask() |
                                    HdChangeTracker::DirtyRenderTag);
    _AllocatePrimId(rprim);

    _RprimInfo info = {
      sceneDelegate,
      rprim
    };
    _rprimMap[rprimId] = std::move(info);
}

void 
HdRenderIndex::RemoveRprim(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    // If we are emulating let's remove from the scene index
    // which will trigger render index removals later.
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->RemovePrims({id});
        return;
    }
    
    _RemoveRprim(id);
}

void HdRenderIndex::_RemoveRprim(SdfPath const &id)
{
    _RprimMap::iterator rit = _rprimMap.find(id);
    if (rit == _rprimMap.end())
        return;

    _RprimInfo &rprimInfo = rit->second;

    SdfPath instancerId = rprimInfo.rprim->GetInstancerId();

    _rprimIds.Remove(id);


    if (!instancerId.IsEmpty()) {
        _tracker.RemoveInstancerRprimDependency(instancerId, id);
    }

    _tracker.RprimRemoved(id);

    // Ask delegate to actually delete the rprim
    rprimInfo.rprim->Finalize(_renderDelegate->GetRenderParam());
    _renderDelegate->DestroyRprim(rprimInfo.rprim);
    rprimInfo.rprim = nullptr;

    _rprimMap.erase(rit);
}

void
HdRenderIndex::_RemoveRprimSubtree(const SdfPath &root,
                                   HdSceneDelegate* sceneDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    struct _Range {
        size_t _start;
        size_t _end;

        _Range() = default;
        _Range(size_t start, size_t end)
         : _start(start)
         , _end(end)
        {
        }
    };

    HdPrimGather gather;
    _Range totalRange;
    std::vector<_Range> rangesToRemove;

    const SdfPathVector &ids = _rprimIds.GetIds();
    if (!gather.SubtreeAsRange(ids,
                               root,
                               &totalRange._start,
                               &totalRange._end)) {
        return;
    }

    // end is inclusive!
    size_t currentRangeStart = totalRange._start;
    for (size_t rprimIdIdx  = totalRange._start;
                rprimIdIdx <= totalRange._end;
              ++rprimIdIdx) {
        const SdfPath &id = ids[rprimIdIdx];

        _RprimMap::iterator rit = _rprimMap.find(id);
        if (rit == _rprimMap.end()) {
            TF_CODING_ERROR("Rprim in id list not in info map: %s",
                             id.GetText());
        } else {
            _RprimInfo &rprimInfo = rit->second;

            if (rprimInfo.sceneDelegate == sceneDelegate) {
                SdfPath instancerId = rprimInfo.rprim->GetInstancerId();
                if (!instancerId.IsEmpty()) {
                    _tracker.RemoveInstancerRprimDependency(instancerId, id);
                }

                _tracker.RprimRemoved(id);

                // Ask delegate to actually delete the rprim
                rprimInfo.rprim->Finalize(_renderDelegate->GetRenderParam());
                _renderDelegate->DestroyRprim(rprimInfo.rprim);
                rprimInfo.rprim = nullptr;

                _rprimMap.erase(rit);
            } else {
                if (currentRangeStart < rprimIdIdx) {
                    rangesToRemove.emplace_back(currentRangeStart,
                                                rprimIdIdx - 1);
                }

                currentRangeStart = rprimIdIdx + 1;
            }
        }
    }

    // Remove final range
    if (currentRangeStart <= totalRange._end) {
        rangesToRemove.emplace_back(currentRangeStart,
                                    totalRange._end);
    }

    // Remove ranges from id's in back to front order to not invalidate indices
    while (!rangesToRemove.empty()) {
        _Range &range = rangesToRemove.back();

        _rprimIds.RemoveRange(range._start, range._end);
        rangesToRemove.pop_back();
    }
}


void
HdRenderIndex::Clear()
{
    HD_TRACE_FUNCTION();

    // Clear tasks.
    for (const auto &pair : _taskMap) {
        _tracker.TaskRemoved(pair.first);
    }
    _taskMap.clear();

    // If we're using emulation, Clear is routed through scene indices.
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->RemovePrims({SdfPath::AbsoluteRootPath()});
        return;
    }

    _Clear();
}

void
HdRenderIndex::_Clear()
{
    HD_TRACE_FUNCTION();

    for (const auto &pair : _rprimMap) {
        SdfPath const &id = pair.first;
        _RprimInfo const &rprimInfo = pair.second;

        SdfPath const &instancerId = rprimInfo.rprim->GetInstancerId();
        if (!instancerId.IsEmpty()) {
            _tracker.RemoveInstancerRprimDependency(instancerId, id);
        }

        _tracker.RprimRemoved(id);

        // Ask delegate to actually delete the rprim
        rprimInfo.rprim->Finalize(_renderDelegate->GetRenderParam());
        _renderDelegate->DestroyRprim(rprimInfo.rprim);
    }
    // Clear Rprims, Rprim IDs, and delegate mappings.
    _rprimMap.clear();
    _rprimIds.Clear();
    _rprimPrimIdMap.clear();

    // Clear S & B prims
    _sprimIndex.Clear(_tracker, _renderDelegate);
    _bprimIndex.Clear(_tracker, _renderDelegate);

    // Clear instancers.
    for (const auto &pair : _instancerMap) {
        SdfPath const &id = pair.first;
        HdInstancer *instancer = pair.second;

        SdfPath const &instancerId = instancer->GetParentId();
        if (!instancerId.IsEmpty()) {
            _tracker.RemoveInstancerInstancerDependency(instancerId, id);
        }

        _tracker.InstancerRemoved(id);

        instancer->Finalize(_renderDelegate->GetRenderParam());
        _renderDelegate->DestroyInstancer(instancer);
    }
    _instancerMap.clear();
}

// -------------------------------------------------------------------------- //
/// \name Task Support
// -------------------------------------------------------------------------- //

void
HdRenderIndex::_TrackDelegateTask(HdSceneDelegate* delegate,
                                    SdfPath const& taskId,
                                    HdTaskSharedPtr const& task)
{
    if (taskId == SdfPath()) {
        return;
    }
    _tracker.TaskInserted(taskId, task->GetInitialDirtyBitsMask());
    _taskMap.emplace(taskId, _TaskInfo{delegate, task});
}

HdTaskSharedPtr const&
HdRenderIndex::GetTask(SdfPath const& id) const {
    _TaskMap::const_iterator it = _taskMap.find(id);
    if (it != _taskMap.end()) {
        return it->second.task;
    }

    static HdTaskSharedPtr EMPTY;
    return EMPTY;
}

void
HdRenderIndex::RemoveTask(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _TaskMap::iterator it = _taskMap.find(id);
    if (it == _taskMap.end()) {
        return;
    }

    _tracker.TaskRemoved(id);
    _taskMap.erase(it);
}


void
HdRenderIndex::_RemoveTaskSubtree(const SdfPath &root,
                                  HdSceneDelegate* sceneDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _TaskMap::iterator it = _taskMap.begin();
    while (it != _taskMap.end()) {
        const SdfPath &id = it->first;
        const _TaskInfo &taskInfo = it->second;

        if ((taskInfo.sceneDelegate == sceneDelegate) &&
            (id.HasPrefix(root))) {
            _tracker.TaskRemoved(id);

            it = _taskMap.erase(it);
        } else {
            ++it;
        }
    }
}

// -------------------------------------------------------------------------- //
/// \name Sprim Support (scene state prim: light, camera...)
// -------------------------------------------------------------------------- //

void
HdRenderIndex::InsertSprim(TfToken const& typeId,
                           HdSceneDelegate* sceneDelegate,
                           SdfPath const& sprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // If we are using emulation, we will need to populate 
    // a data source with the prim information
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->AddLegacyPrim(sprimId, typeId, sceneDelegate);
        return;
    }
    
    _InsertSprim(typeId, sceneDelegate, sprimId);
}

void 
HdRenderIndex::_InsertSprim(TfToken const& typeId,
                            HdSceneDelegate* delegate,
                            SdfPath const& sprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _sprimIndex.InsertPrim(typeId, delegate, sprimId,
                           _tracker, _renderDelegate);
}

void
HdRenderIndex::RemoveSprim(TfToken const& typeId, SdfPath const& id)
{
    if (_IsEnabledSceneIndexEmulation()) {

        // Removing an sprim doesn't remove any descendant prims from the
        // renderIndex. Removing a prim from the scene index does remove
        // all descendant prims. Special case removal of an sprim which has
        // children to instead be replaced with an empty type.
        if (!_emulationSceneIndex->GetChildPrimPaths(id).empty()) {
             _emulationSceneIndex->AddPrims({{id, TfToken(), nullptr}});
             return;
        }
        
        _emulationSceneIndex->RemovePrims({id});

        return;
    }

    _RemoveSprim(typeId, id);
}

void
HdRenderIndex::_RemoveSprim(TfToken const& typeId, SdfPath const &id)
{
    _sprimIndex.RemovePrim(typeId, id, _tracker, _renderDelegate);
}

HdSprim*
HdRenderIndex::GetSprim(TfToken const& typeId, SdfPath const& id) const
{
    return _sprimIndex.GetPrim(typeId, id);
}

SdfPathVector
HdRenderIndex::GetSprimSubtree(TfToken const& typeId,
                               SdfPath const& rootPath)
{
    SdfPathVector result;
    _sprimIndex.GetPrimSubtree(typeId, rootPath, &result);
    return result;
}

HdSprim *
HdRenderIndex::GetFallbackSprim(TfToken const& typeId) const
{
    return _sprimIndex.GetFallbackPrim(typeId);
}


// -------------------------------------------------------------------------- //
/// \name Bprim Support (Buffer prim: texture, buffers...)
// -------------------------------------------------------------------------- //

void
HdRenderIndex::InsertBprim(TfToken const& typeId,
                           HdSceneDelegate* sceneDelegate,
                           SdfPath const& bprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // If we are using emulation, we will need to populate a data source with
    // the prim information
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->AddLegacyPrim(bprimId, typeId, sceneDelegate);
        return;
    }

    _InsertBprim(typeId, sceneDelegate, bprimId);
}

void
HdRenderIndex::_InsertBprim(TfToken const& typeId,
                           HdSceneDelegate* sceneDelegate,
                           SdfPath const& bprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _bprimIndex.InsertPrim(typeId, sceneDelegate, bprimId,
                           _tracker, _renderDelegate);
}

void
HdRenderIndex::RemoveBprim(TfToken const& typeId, SdfPath const& id)
{
    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->RemovePrims({id});
        return;
    }

    _RemoveBprim(typeId, id);
}

void
HdRenderIndex::_RemoveBprim(TfToken const& typeId, SdfPath const &id)
{
    _bprimIndex.RemovePrim(typeId, id, _tracker, _renderDelegate);
}

HdBprim *
HdRenderIndex::GetBprim(TfToken const& typeId, SdfPath const& id) const
{
    return _bprimIndex.GetPrim(typeId, id);
}

SdfPathVector
HdRenderIndex::GetBprimSubtree(TfToken const& typeId,
                               SdfPath const& rootPath)
{
    SdfPathVector result;
    _bprimIndex.GetPrimSubtree(typeId, rootPath, &result);
    return result;
}

HdBprim *
HdRenderIndex::GetFallbackBprim(TfToken const& typeId) const
{
    return _bprimIndex.GetFallbackPrim(typeId);
}


// ---------------------------------------------------------------------- //
// Render Delegate
// ---------------------------------------------------------------------- //
HdRenderDelegate *HdRenderIndex::GetRenderDelegate() const
{
    return _renderDelegate;
}

HdResourceRegistrySharedPtr
HdRenderIndex::GetResourceRegistry() const
{
    return _renderDelegate->GetResourceRegistry();
}

void
HdRenderIndex::SceneIndexEmulationNoticeBatchBegin()
{
    if (_emulationNoticeBatchingSceneIndex) {
        _emulationNoticeBatchingSceneIndex->SetBatchingEnabled(true);
    }
}

void
HdRenderIndex::SceneIndexEmulationNoticeBatchEnd()
{
    if (_emulationNoticeBatchingSceneIndex) {
        _emulationNoticeBatchingSceneIndex->SetBatchingEnabled(false);
    }
}

bool
HdRenderIndex::_CreateFallbackPrims()
{
    bool success = true;

    success &= _sprimIndex.CreateFallbackPrims(_renderDelegate);
    success &= _bprimIndex.CreateFallbackPrims(_renderDelegate);

    return success;
}

void
HdRenderIndex::_DestroyFallbackPrims()
{
    _sprimIndex.DestroyFallbackPrims(_renderDelegate);
    _bprimIndex.DestroyFallbackPrims(_renderDelegate);
}

/* static */
void
HdRenderIndex::_ConfigureReprs()
{
    // pre-defined reprs (to be deprecated or minimalized)
    HdMesh::ConfigureRepr(HdReprTokens->hull,
                          HdMeshReprDesc(HdMeshGeomStyleHull,
                                         HdCullStyleDontCare,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/true,
                                         /*blendWireframeColor=*/false));
    HdMesh::ConfigureRepr(HdReprTokens->smoothHull,
                          HdMeshReprDesc(HdMeshGeomStyleHull,
                                         HdCullStyleDontCare,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/false,
                                         /*blendWireframeColor=*/false));
    HdMesh::ConfigureRepr(HdReprTokens->wire,
                          HdMeshReprDesc(HdMeshGeomStyleHullEdgeOnly,
                                         HdCullStyleDontCare,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/false,
                                         /*blendWireframeColor=*/true));
    HdMesh::ConfigureRepr(HdReprTokens->wireOnSurf,
                          HdMeshReprDesc(HdMeshGeomStyleHullEdgeOnSurf,
                                         HdCullStyleDontCare,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/false,
                                         /*blendWireframeColor=*/true));
    HdMesh::ConfigureRepr(HdReprTokens->refined,
                          HdMeshReprDesc(HdMeshGeomStyleSurf,
                                         HdCullStyleDontCare,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/false,
                                         /*blendWireframeColor=*/false));
    HdMesh::ConfigureRepr(HdReprTokens->refinedWire,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnly,
                                         HdCullStyleDontCare,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/false,
                                         /*blendWireframeColor=*/true));
    HdMesh::ConfigureRepr(HdReprTokens->refinedWireOnSurf,
                          HdMeshReprDesc(HdMeshGeomStyleEdgeOnSurf,
                                         HdCullStyleDontCare,
                                         HdMeshReprDescTokens->surfaceShader,
                                         /*flatShadingEnabled=*/false,
                                         /*blendWireframeColor=*/true));
    HdMesh::ConfigureRepr(HdReprTokens->points,
                          HdMeshReprDesc(HdMeshGeomStylePoints,
                                         HdCullStyleNothing,
                                         HdMeshReprDescTokens->pointColor,
                                         /*flatShadingEnabled=*/false,
                                         /*blendWireframeColor=*/false));

    HdBasisCurves::ConfigureRepr(HdReprTokens->hull,
                                 HdBasisCurvesGeomStylePatch);
    HdBasisCurves::ConfigureRepr(HdReprTokens->smoothHull,
                                 HdBasisCurvesGeomStylePatch);
    HdBasisCurves::ConfigureRepr(HdReprTokens->wire,
                                 HdBasisCurvesGeomStyleWire);
    HdBasisCurves::ConfigureRepr(HdReprTokens->wireOnSurf,
                                 HdBasisCurvesGeomStylePatch);
    HdBasisCurves::ConfigureRepr(HdReprTokens->refined,
                                 HdBasisCurvesGeomStylePatch);
    HdBasisCurves::ConfigureRepr(HdReprTokens->refinedWire,
                                 HdBasisCurvesGeomStyleWire);
    HdBasisCurves::ConfigureRepr(HdReprTokens->refinedWireOnSurf,
                                 HdBasisCurvesGeomStylePatch);
    HdBasisCurves::ConfigureRepr(HdReprTokens->points,
                                 HdBasisCurvesGeomStylePoints);

    HdPoints::ConfigureRepr(HdReprTokens->hull,
                            HdPointsGeomStylePoints);
    HdPoints::ConfigureRepr(HdReprTokens->smoothHull,
                            HdPointsGeomStylePoints);
    HdPoints::ConfigureRepr(HdReprTokens->wire,
                            HdPointsGeomStylePoints);
    HdPoints::ConfigureRepr(HdReprTokens->wireOnSurf,
                            HdPointsGeomStylePoints);
    HdPoints::ConfigureRepr(HdReprTokens->refined,
                            HdPointsGeomStylePoints);
    HdPoints::ConfigureRepr(HdReprTokens->refinedWire,
                            HdPointsGeomStylePoints);
    HdPoints::ConfigureRepr(HdReprTokens->refinedWireOnSurf,
                            HdPointsGeomStylePoints);
    HdPoints::ConfigureRepr(HdReprTokens->points,
                            HdPointsGeomStylePoints);
}
// -------------------------------------------------------------------------- //
/// \name Draw Item Handling
// -------------------------------------------------------------------------- //


struct _FilterParam {
    const TfTokenVector     &renderTags;
    const HdRenderIndex     *renderIndex;
};

static bool
_DrawItemFilterPredicate(const SdfPath &rprimID, const void *predicateParam)
{
    const _FilterParam *filterParam =
                              static_cast<const _FilterParam *>(predicateParam);

    const TfTokenVector     &renderTags  = filterParam->renderTags;
    const HdRenderIndex     *renderIndex = filterParam->renderIndex;

    //
    // Render Tag Filter
    //
    if (renderTags.empty()) {
        // An empty render tag set means everything passes the filter
        // Primary user is tests, but some single task render delegates
        // that don't support render tags yet also use it.
        return true;
    } else {
        // As the number of tags is expected to be low (<10)
        // use a simple linear search.
        TfToken primRenderTag = renderIndex->GetRenderTag(rprimID);
        size_t numRenderTags = renderTags.size();
        size_t tagNum = 0;
        while (tagNum < numRenderTags) {
            if (renderTags[tagNum] == primRenderTag) {
                return true;
            }
            ++tagNum;
        }
    }

   return false;
}

HdRenderIndex::HdDrawItemPtrVector
HdRenderIndex::GetDrawItems(HdRprimCollection const& collection,
                            TfTokenVector const& renderTags)
{
    HD_TRACE_FUNCTION();

    SdfPathVector rprimIds;

    const SdfPathVector &paths        = GetRprimIds();
    const SdfPathVector &includePaths = collection.GetRootPaths();
    const SdfPathVector &excludePaths = collection.GetExcludePaths();

    _FilterParam filterParam = {renderTags, this};

    HdPrimGather gather;

    gather.PredicatedFilter(paths,
                            includePaths,
                            excludePaths,
                            _DrawItemFilterPredicate,
                            &filterParam,
                            &rprimIds);

    _ConcurrentDrawItems concurrentDrawItems;


    WorkParallelForN(rprimIds.size(),
                         std::bind(&HdRenderIndex::_AppendDrawItems,
                                   this,
                                   std::cref(rprimIds),
                                   std::placeholders::_1,     // begin
                                   std::placeholders::_2,     // end
                                   std::cref(collection),
                                   &concurrentDrawItems));


    typedef tbb::flattened2d<_ConcurrentDrawItems> _FlattenDrawItems;

    _FlattenDrawItems result = tbb::flatten2d(concurrentDrawItems);

    // Merge thread results to the output data structure
    HdDrawItemPtrVector finalResult;
    finalResult.insert(finalResult.end(),
                       result.begin(),
                       result.end());

    return finalResult;
}

TfToken
HdRenderIndex::GetRenderTag(SdfPath const& id) const
{
    _RprimInfo const* info = TfMapLookupPtr(_rprimMap, id);
    if (info == nullptr) {
        return HdRenderTagTokens->hidden;
    }

    return info->rprim->GetRenderTag();
}

TfToken
HdRenderIndex::UpdateRenderTag(SdfPath const& id,
                               HdDirtyBits bits)
{
    _RprimInfo const* info = TfMapLookupPtr(_rprimMap, id);
    if (info == nullptr) {
        return HdRenderTagTokens->hidden;
    }

    if (bits & HdChangeTracker::DirtyRenderTag) {
        info->rprim->UpdateRenderTag(info->sceneDelegate,
                                    _renderDelegate->GetRenderParam());
        _tracker.MarkRprimClean(id,
                                bits & ~HdChangeTracker::DirtyRenderTag);
    }
    return info->rprim->GetRenderTag();
}

SdfPathVector
HdRenderIndex::GetRprimSubtree(SdfPath const& rootPath)
{
    SdfPathVector paths;

    HdPrimGather gather;
    gather.Subtree(_rprimIds.GetIds(), rootPath, &paths);

    return paths;
}

namespace {
    // A struct that captures (just) the repr opinion of a collection.
    struct _CollectionReprSpec {
        _CollectionReprSpec(HdReprSelector const &repr, bool forced) :
            reprSelector(repr), useCollectionRepr(forced) {}
        HdReprSelector reprSelector;
        bool useCollectionRepr;

        bool operator == (_CollectionReprSpec const &other) const {
            return  (reprSelector == other.reprSelector) &&
                    (useCollectionRepr == other.useCollectionRepr);
        }
    };
    // Repr specs to sync for all the dirty Rprims. This information is
    // gathered during task sync from the render pass' collection opinion. 
    using _CollectionReprSpecVector = std::vector<_CollectionReprSpec>;

    // -------------------------------------------------------------------------
    struct _RprimSyncRequestVector {
        void PushBack(HdRprim *rprim,
                      SdfPath const &rprimId,
                      HdDirtyBits dirtyBits)
        {
            rprims.push_back(rprim);
            request.IDs.push_back(rprimId);
            request.dirtyBits.push_back(dirtyBits);
        }

        std::vector<HdRprim *> rprims;
        HdSyncRequestVector request;

        _RprimSyncRequestVector() = default;
        // XXX: This is a heavy structure and should not be copied.
        //_RprimSyncRequestVector(const _RprimSyncRequestVector&) = delete;
        _RprimSyncRequestVector& operator =(const _RprimSyncRequestVector&)
            = delete;
    };
    // A map of the Rprims to sync per scene delegate.
    using _SceneDelegateRprimSyncRequestMap =
        TfHashMap<HdSceneDelegate*, _RprimSyncRequestVector, TfHash>;

    struct _SceneDelegateSyncWorker {
        _SceneDelegateRprimSyncRequestMap* _map;
        std::vector<HdSceneDelegate*> _delegates;
    public:
        _SceneDelegateSyncWorker(
            _SceneDelegateRprimSyncRequestMap* map) : _map(map)
        {
            _delegates.reserve(map->size());
            for (auto const &entry : *map) {
                _delegates.push_back(entry.first);
            }
        }

        void Process(size_t begin, size_t end)
        {
            for (size_t i = begin; i < end; i++) {
                HdSceneDelegate* sd = _delegates[i];
                _RprimSyncRequestVector* r = TfMapLookupPtr(*_map, sd);
                sd->Sync(&r->request);
            }
        }
    };

    static HdReprSelector
    _GetResolvedReprSelector(HdReprSelector const &rprimReprSelector,
                             HdReprSelector const &colReprSelector,
                             bool forceColRepr)
    {
        // if not forced, the prim's authored opinion composites over the
        // collection's repr, otherwise we respect the collection's repr
        // (used for shadows)
        if (!forceColRepr) {
            return rprimReprSelector.CompositeOver(colReprSelector);
        }
        return colReprSelector;
    }

    struct _SyncRPrims {
        HdSceneDelegate *_sceneDelegate;
        _RprimSyncRequestVector &_r;
        _CollectionReprSpecVector const &_reprSpecs;
        HdChangeTracker &_tracker;
        HdRenderParam *_renderParam;
    public:
        _SyncRPrims( HdSceneDelegate *sceneDelegate,
                     _RprimSyncRequestVector& r,
                     _CollectionReprSpecVector const &reprSpecs,
                     HdChangeTracker &tracker,
                     HdRenderParam *renderParam)
         : _sceneDelegate(sceneDelegate)
         , _r(r)
         , _reprSpecs(reprSpecs)
         , _tracker(tracker)
         , _renderParam(renderParam)
        {
        }

        void Sync(size_t begin, size_t end)
        {
            for (size_t i = begin; i < end; ++i)
            {
                HdRprim &rprim = *_r.rprims[i];

                HdDirtyBits dirtyBits = _r.request.dirtyBits[i];

                TfTokenVector reprsSynced;
                for (const _CollectionReprSpec& spec : _reprSpecs) {
                    // The rprim's authored repr selector is
                    // guaranteed to have been set at this point (via
                    // InitRepr/DirtyRepr handling during PreSync)
                    HdReprSelector reprSelector = 
                        _GetResolvedReprSelector(rprim.GetReprSelector(),
                                                 spec.reprSelector,
                                                 spec.useCollectionRepr);

                    for (size_t i = 0;
                         i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {

                        TfToken const& reprToken = reprSelector[i];
                        // Sync valid repr tokens once.
                        if (reprSelector.IsActiveRepr(i) &&
                            std::find(reprsSynced.begin(), reprsSynced.end(),
                                      reprToken) == reprsSynced.end()) {

                            rprim.Sync(_sceneDelegate,
                                        _renderParam,
                                        &dirtyBits,
                                        reprToken);

                            reprsSynced.push_back(reprToken);
                        }
                    }
                }

                _tracker.MarkRprimClean(rprim.GetId(), dirtyBits);
            }
        }
    };

    static void
    _InitRprimReprs(HdSceneDelegate *sceneDelegate,
                    HdReprSelector const& colReprSelector,
                    bool forceColRepr,
                    HdRprim *rprim,
                    HdDirtyBits *dirtyBits)
    {
        HdReprSelector reprSelector = _GetResolvedReprSelector(
                                            rprim->GetReprSelector(),
                                            colReprSelector,
                                            forceColRepr);

        for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {
            if (reprSelector.IsActiveRepr(i)) {
                TfToken const& reprToken = reprSelector[i];
                rprim->InitRepr(sceneDelegate,
                                reprToken,
                                dirtyBits);
            }
        }
    }

    static void
    _PreSyncRPrims(HdSceneDelegate *sceneDelegate,
                   HdChangeTracker *tracker,
                   _RprimSyncRequestVector *syncReq,
                   _CollectionReprSpecVector const& reprSpecs,
                   size_t begin,
                   size_t end)
    {
        for (size_t i = begin; i < end; ++i)
        {
            HdRprim         *rprim         = syncReq->rprims[i];
            HdDirtyBits     &dirtyBits     = syncReq->request.dirtyBits[i];

            // Initialize all utilized reprs for the rprim.
            //
            // An Rprim may require additional data to perform a sync of a repr
            // for the first time.  Therefore, inform the Rprim of the new repr
            // and give it the opportunity to modify the dirty bits in the
            // request before providing them to the scene delegate.
            //
            // The InitRepr bit is set when the dirty list is reset to all
            // the Rprim ids. See HdDirtyList::_UpdateDirtyIdsIfNeeded().
            //
            // The DirtyRepr bit on the otherhand is set when the scene
            // delegate's prim repr state changes and thus the prim must
            // fetch it again from the scene delgate.
            //
            // In both cases, if the repr is new for the prim, this leaves the
            // NewRepr dirty bit on the prim (otherwise NewRepr is clean).
            if ((dirtyBits &
                     (HdChangeTracker::InitRepr | HdChangeTracker::DirtyRepr))
                  != 0) {

                rprim->UpdateReprSelector(sceneDelegate, &dirtyBits);

                for (const _CollectionReprSpec& spec : reprSpecs) {
                    _InitRprimReprs(sceneDelegate,
                                    spec.reprSelector,
                                    spec.useCollectionRepr,
                                    rprim,
                                    &dirtyBits);
                }
                dirtyBits &= ~HdChangeTracker::InitRepr;
                // Clear the InitRepr bit in the change tracker.
                tracker->MarkRprimClean(rprim->GetId(), dirtyBits);
            }

            if (rprim->CanSkipDirtyBitPropagationAndSync(dirtyBits)) {
                // XXX: This is quite hacky. See comment in the implementation
                // of HdRprim::CanSkipDirtyBitPropagationAndSync
                dirtyBits = HdChangeTracker::Clean;
                tracker->ResetRprimVaryingState(rprim->GetId());
                continue;
            }

            // A render delegate may require additional information
            // from the scene delegate to process a change.
            //
            // Calling PropagateRprimDirtyBits gives the Rprim an opportunity
            // to update the dirty bits in order to request the information
            // before passing the request to the scene deleate.
            dirtyBits = rprim->PropagateRprimDirtyBits(dirtyBits);
        }
    }

    static void
    _PreSyncRequestVector(HdSceneDelegate *sceneDelegate,
                          HdChangeTracker *tracker,
                          _RprimSyncRequestVector *syncReq,
                          _CollectionReprSpecVector const &reprSpecs)
    {
        size_t numPrims = syncReq->rprims.size();
        WorkParallelForN(numPrims,
                         std::bind(&_PreSyncRPrims,
                                   sceneDelegate, tracker,
                                   syncReq, std::cref(reprSpecs),
                                   std::placeholders::_1,
                                   std::placeholders::_2));

        // Pre-sync may have completely cleaned prims, so as an optimization
        // remove them from the sync request list.
        size_t primIdx = 0;
        while (primIdx < numPrims)
        {
            if (HdChangeTracker::IsClean(syncReq->request.dirtyBits[primIdx])) {
                if (numPrims == 1) {
                    syncReq->rprims.clear();
                    syncReq->request.IDs.clear();
                    syncReq->request.dirtyBits.clear();
                    ++primIdx;
                } else {

                    std::swap(syncReq->rprims[primIdx],
                              syncReq->rprims[numPrims -1]);
                    std::swap(syncReq->request.IDs[primIdx],
                              syncReq->request.IDs[numPrims -1]);
                    std::swap(syncReq->request.dirtyBits[primIdx],
                              syncReq->request.dirtyBits[numPrims -1]);

                    syncReq->rprims.pop_back();
                    syncReq->request.IDs.pop_back();
                    syncReq->request.dirtyBits.pop_back();
                    --numPrims;
                }
            } else {
                ++primIdx;
            }
        }
    }

    // Gather the unique set of render tags requested by the tasks.
    static TfTokenVector
    _GatherRenderTags(const HdTaskSharedPtrVector *tasks)
    {
        TfTokenVector tags;
        size_t numTasks = tasks->size();
        for (size_t taskNum = 0; taskNum < numTasks; ++taskNum) {
            const HdTaskSharedPtr &task = (*tasks)[taskNum];
            const TfTokenVector &taskRenderTags = task->GetRenderTags();

            tags.insert(tags.end(),
                        taskRenderTags.begin(),
                        taskRenderTags.end());
        }

        // Deduplicate.
        std::sort(tags.begin(), tags.end());
        TfTokenVector::iterator newEnd =
                std::unique(tags.begin(), tags.end());
        tags.erase(newEnd, tags.end());

        return tags;
    }

    static _CollectionReprSpecVector
    _GatherReprSpecs(const HdRprimCollectionVector &collections)
    {
        _CollectionReprSpecVector reprSpecs;
        for (HdRprimCollection const &collection : collections) {
            HdReprSelector const &rs = collection.GetReprSelector();
            if (!rs.AnyActiveRepr()) {
                continue; // Skip empty/disabled reprs
            }
            _CollectionReprSpec reprSpec(rs, collection.IsForcedRepr());
        
            if (std::find(reprSpecs.begin(), reprSpecs.end(), reprSpec)
                == reprSpecs.end()) {
                
                reprSpecs.push_back(reprSpec);
            }
        }

        if (TfDebug::IsEnabled(HD_SYNC_ALL)) {
            std::cout << "Reprs to sync: [";
            for (auto const &rs : reprSpecs) {
                std::cout << "    " << rs.reprSelector << std::endl;
            }
            std::cout << "]" << std::endl;
        }

        return reprSpecs;
    }

    static HdReprSelectorVector
    _GetReprSelectors(_CollectionReprSpecVector const &specs)
    {
        HdReprSelectorVector reprs;

        for (const auto &spec : specs) {
            HdReprSelector const &repr = spec.reprSelector;
            if (std::find(reprs.begin(), reprs.end(), repr) == reprs.end()) {
                reprs.push_back(repr);
            }
        }

        return reprs;
    }
};

void
HdRenderIndex::EnqueueCollectionToSync(HdRprimCollection const &col)
{
    _collectionsToSync.push_back(col);
}

void
HdRenderIndex::SyncAll(HdTaskSharedPtrVector *tasks,
                       HdTaskContext *taskContext)
{
    HD_TRACE_FUNCTION();

    HdRenderParam *renderParam = _renderDelegate->GetRenderParam();

    _bprimIndex.SyncPrims(_tracker, _renderDelegate->GetRenderParam());

    _sprimIndex.SyncPrims(_tracker, _renderDelegate->GetRenderParam());

    ////////////////////////////////////////////////////////////////////////////
    //
    // Task Sync
    //
    // NOTE: Syncing tasks may update _collectionsToSync for
    // processing the dirty rprims below.
    //
    {
        TRACE_FUNCTION_SCOPE("Task Sync");

        size_t numTasks = tasks->size();
        for (size_t taskNum = 0; taskNum < numTasks; ++taskNum) {
            HdTaskSharedPtr &task = (*tasks)[taskNum];

            if (!TF_VERIFY(task)) {
                TF_CODING_ERROR("Null Task in task list.  Entry Num: %zu",
                                taskNum);
                continue;
            }

            SdfPath taskId = task->GetId();

            // Is this a tracked task?
            _TaskMap::iterator taskMapIt = _taskMap.find(taskId);
            if (taskMapIt != _taskMap.end()) {
                _TaskInfo &taskInfo = taskMapIt->second;

                // If the task is in the render index, then we have the
                // possibility that the task passed in points to a
                // different instance than the one stored in the render index
                // even though they have the same id.
                //
                // For consistency, we always use the registered task in the
                // render index for a given id, as that is the one the state is
                // tracked for.
                //
                // However, this is still a weird situation, so report the
                // issue as a verify so it can be addressed.
                TF_VERIFY(taskInfo.task == task);

                HdDirtyBits taskDirtyBits = _tracker.GetTaskDirtyBits(taskId);

                taskInfo.task->Sync(taskInfo.sceneDelegate,
                                    taskContext,
                                    &taskDirtyBits);

                _tracker.MarkTaskClean(taskId, taskDirtyBits);

            } else {
                // Dummy dirty bits
                HdDirtyBits taskDirtyBits = 0;

                // This is an untracked task, never added to the render index.
                task->Sync(nullptr,
                           taskContext,
                           &taskDirtyBits);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    //
    // Rprim Sync
    //

    // a. Gather render tags and reprSpecs.
    TfTokenVector taskRenderTags = _GatherRenderTags(tasks);
    
    // NOTE: This list of reprSpecs is used to sync every dirty rprim.
    _CollectionReprSpecVector reprSpecs = _GatherReprSpecs(_collectionsToSync);
    HdReprSelectorVector reprSelectors = _GetReprSelectors(reprSpecs);

    // b. Update dirty list params, if needed sync render tags, 
    // and get dirty rprim ids
    _rprimDirtyList.UpdateRenderTagsAndReprSelectors(taskRenderTags,
                                                     reprSelectors);

    // NOTE: GetDirtyRprims relies on up-to-date render tags; if render tags
    // are dirty, this call will sync render tags before compiling the dirty
    // list. This is outside of the usual sync order, but is necessary for now.
    SdfPathVector const& dirtyRprimIds = _rprimDirtyList.GetDirtyRprims();
 
    // c. Bucket rprims by their scene delegate to help build the the list
    //    of rprims to sync for each scene delegate.
    _SceneDelegateRprimSyncRequestMap sdRprimSyncMap;
    bool resetVaryingState = false;
    bool pruneDirtyList = false;
    {
        HF_TRACE_FUNCTION_SCOPE("Build Sync Map: Rprims");
        HdSceneDelegate* curDel = nullptr;
        _RprimSyncRequestVector* curVec = nullptr;
        int numSkipped = 0;
        int numNonVarying = 0;
        for (SdfPath const &rprimId : dirtyRprimIds) {
            _RprimMap::const_iterator it = _rprimMap.find(rprimId);
            if (!TF_VERIFY(it != _rprimMap.end())) {
                continue;
            }

            HdDirtyBits dirtyBits = _tracker.GetRprimDirtyBits(rprimId);
            if (!HdChangeTracker::IsVarying(dirtyBits)) {
                ++numNonVarying;
            }
            if (HdChangeTracker::IsClean(dirtyBits)) {
                ++numSkipped;
                continue;
            }

            const _RprimInfo &rprimInfo = it->second;
            // PERFORMANCE: This loop is constrained by memory access, avoid
            // re-fetching the sync request vector if possible.
            if (curDel != rprimInfo.sceneDelegate) {
                curDel = rprimInfo.sceneDelegate;
                curVec = &sdRprimSyncMap[curDel];
            }

            curVec->PushBack(rprimInfo.rprim, rprimId, dirtyBits);
        }

        // Use a heuristic to determine whether or not to destroy the entire
        // dirty state.  We say that if we've skipped more than 25% of the
        // rprims that were claimed dirty, then it's time to clean up this
        // list on the next iteration. This is done by resetting the varying
        // state of all clean rprims.
        //
        // Alternatively if the list contains more the 10% rprims that
        // are not marked as varying (e.g., when rprims are invisible, or when 
        // the dirty list is reset to all rprims), we flag the dirty list for
        // pruning on the next iteration.
        //
        // Since both these operations can be expensive (especially the former),
        // we use a size heuristic to avoid doing it for a small dirty list.
        //
        // This leads to performance improvements after many rprims
        // get dirty and then cleaned up, and the steady state becomes a
        // small number of dirty items.
        //
        constexpr size_t MIN_DIRTY_LIST_SIZE = 500;
        constexpr float MIN_RATIO_RPRIMS_SKIPPED = 0.25f; // 25 %
        constexpr float MIN_RATIO_RPRIMS_NON_VARYING = 0.10f; // 10 %
        const size_t numDirtyRprims = dirtyRprimIds.size();

        if (numDirtyRprims > MIN_DIRTY_LIST_SIZE) {
            float ratioNumSkipped = numSkipped / (float) numDirtyRprims;
            float ratioNonVarying = numNonVarying / (float) numDirtyRprims;
            
            resetVaryingState = ratioNumSkipped > MIN_RATIO_RPRIMS_SKIPPED;
            pruneDirtyList =
                ratioNonVarying > MIN_RATIO_RPRIMS_NON_VARYING;

            if (TfDebug::IsEnabled(HD_VARYING_STATE)) {
                std::stringstream ss;

                ss  << "Dirty List Redundancy: Skipped = "
                    << ratioNumSkipped * 100.0f << "% ("
                    <<  numSkipped << " / " << numDirtyRprims << ") "
                    << "Non-Varying  = "
                    << ratioNonVarying * 100.0f << "% ("
                    << numNonVarying << " / "  << numDirtyRprims << ") \n";

                TfDebug::Helper().Msg(ss.str());
            }
        }
    }

    // Drop the GIL before we spawn parallel tasks.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // d. Rprim "Pre-Sync"
    // Give the render delegates the chance to modify the sync request
    // before passing it to the scene delegates.
    //
    // This allows the render delegate to request more data that it needs
    // to process the changes that are marked in the change tracker.
    //
    // So that the entity marking the changes does not need to be aware of
    // render delegate specific data dependencies.
    {
        HF_TRACE_FUNCTION_SCOPE("Pre-Sync Rprims");

        // Dispatch synchronization work to each delegate.
        WorkWithScopedParallelism([&]() {
            WorkDispatcher preSyncDispatcher;

            for (auto &entry : sdRprimSyncMap) {
                HdSceneDelegate *sceneDelegate = entry.first;
                _RprimSyncRequestVector *r = &entry.second;
                preSyncDispatcher.Run(
                    std::bind(&_PreSyncRequestVector,
                              sceneDelegate,
                              &_tracker,
                              r,
                              std::cref(reprSpecs)));

            }
        });
    }

    // e. Scene delegate sync
    // Note: This is for the Rprim dirty bits alone.
    {
        HF_TRACE_FUNCTION_SCOPE("Scene Delegate Sync");
        // Dispatch synchronization work to each delegate.
        _SceneDelegateSyncWorker worker(&sdRprimSyncMap);
        WorkParallelForN(sdRprimSyncMap.size(),
                         std::bind(&_SceneDelegateSyncWorker::Process,
                                   std::ref(worker),
                                   std::placeholders::_1,
                                   std::placeholders::_2));
    }

    // f. Rprim Sync
    WorkWithScopedParallelism([&]() {
        WorkDispatcher dispatcher;
        for (auto &entry : sdRprimSyncMap) {
            HdSceneDelegate* sceneDelegate = entry.first;
            _RprimSyncRequestVector& r = entry.second;
            
            {
                _SyncRPrims workerState(
                    sceneDelegate, r, reprSpecs, _tracker, renderParam);

                if (!TfDebug::IsEnabled(HD_DISABLE_MULTITHREADED_RPRIM_SYNC) &&
                    sceneDelegate->IsEnabled(
                        HdOptionTokens->parallelRprimSync)) {
                    TRACE_SCOPE("Parallel Rprim Sync");
                    // In the lambda below, we capture workerState by value and
                    // incur a copy in the std::bind because the lambda
                    // execution may be delayed (until we call Wait), resulting
                    // in workerState going out of scope.
                    dispatcher.Run([&r, workerState]() {
                        WorkParallelForN(r.rprims.size(),
                                         std::bind(
                                             &_SyncRPrims::Sync, workerState,
                                             std::placeholders::_1,
                                             std::placeholders::_2));
                    });
                } else {
                    TRACE_SCOPE("Serial Rprim Sync");
                    // Single-threaded version: Call worker directly
                    workerState.Sync(0, r.rprims.size());
                }
            }
        }
    });

    {
        HF_TRACE_FUNCTION_SCOPE("Clean Up");
        // Give scene delegates a chance to do any post-sync work,
        // such as garbage collection.
        for (auto &entry : sdRprimSyncMap) {
            HdSceneDelegate *delegate = entry.first;
            delegate->PostSyncCleanup();
        }
        const HdSceneDelegatePtrVector& sprimDelegates =
            _sprimIndex.GetSceneDelegatesForDirtyPrims();
        for (HdSceneDelegate* delegate : sprimDelegates) {
            delegate->PostSyncCleanup();
        }

        if (resetVaryingState) {
            _tracker.ResetVaryingState();
        } else if (pruneDirtyList) {
            _rprimDirtyList.PruneToVaryingRprims();
        }
        _collectionsToSync.clear();
    }
}

HdDriverVector const&
HdRenderIndex::GetDrivers() const
{
    return _drivers;
}

void
HdRenderIndex::_CompactPrimIds()
{
    _rprimPrimIdMap.resize(_rprimMap.size());
    int32_t nextPrimId = 0;
    for (const auto &pair : _rprimMap) {
        pair.second.rprim->SetPrimId(nextPrimId);
        _tracker.MarkRprimDirty(pair.first, HdChangeTracker::DirtyPrimID);
        _rprimPrimIdMap[nextPrimId] = pair.first;
        ++nextPrimId;
    }
}

void
HdRenderIndex::_AllocatePrimId(HdRprim *prim)
{
    const size_t maxId = (1 << 24) - 1;
    if (_rprimPrimIdMap.size() > maxId) {
        // We are wrapping around our max prim id.. time to reallocate
        _CompactPrimIds();
        // Make sure we have a valid next id after compacting
        TF_VERIFY(_rprimPrimIdMap.size() < maxId);
    }
    int32_t nextPrimId = _rprimPrimIdMap.size();
    prim->SetPrimId(nextPrimId);
    // note: not marking DirtyPrimID here to avoid undesirable variability tracking.
    _rprimPrimIdMap.push_back(prim->GetId());
}

SdfPath
HdRenderIndex::GetRprimPathFromPrimId(int primId) const
{
    if (static_cast<size_t>(primId) >= _rprimPrimIdMap.size()) {
        return SdfPath();
    }

    return _rprimPrimIdMap[primId];
}

void
HdRenderIndex::InsertInstancer(HdSceneDelegate* delegate,
                               SdfPath const &id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->AddLegacyPrim(
            id, HdPrimTypeTokens->instancer, delegate);
        return;
    }

    _InsertInstancer(delegate, id);
}

void
HdRenderIndex::_InsertInstancer(HdSceneDelegate* delegate,
                                SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (ARCH_UNLIKELY(_instancerMap.find(id) != _instancerMap.end())) {
        return;
    }

    SdfPath const &sceneDelegateId = delegate->GetDelegateID();
    if (!id.HasPrefix(sceneDelegateId)) {
        TF_CODING_ERROR("Scene Delegate Id (%s) must prefix prim Id (%s)",
                        sceneDelegateId.GetText(), id.GetText());
        return;
    }

    HdInstancer *instancer =
        _renderDelegate->CreateInstancer(delegate, id);
    if (instancer == nullptr) {
        return;
    }

    _instancerMap[id] = instancer;
    _tracker.InstancerInserted(id, instancer->GetInitialDirtyBitsMask());
}

void
HdRenderIndex::RemoveInstancer(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_IsEnabledSceneIndexEmulation()) {
        _emulationSceneIndex->RemovePrims({{id}});
        return;
    }

    _RemoveInstancer(id);
}

void
HdRenderIndex::_RemoveInstancer(SdfPath const& id)
{
    _InstancerMap::iterator it = _instancerMap.find(id);
    if (it == _instancerMap.end())
        return;

    HdInstancer *instancer = it->second;

    SdfPath const& instancerId = instancer->GetParentId();
    if (!instancerId.IsEmpty()) {
        _tracker.RemoveInstancerInstancerDependency(instancerId, id);
    }

    _tracker.InstancerRemoved(id);

    instancer->Finalize(_renderDelegate->GetRenderParam());
    _renderDelegate->DestroyInstancer(instancer);

    _instancerMap.erase(it);
}

void
HdRenderIndex::_RemoveInstancerSubtree(const SdfPath &root,
                                       HdSceneDelegate* sceneDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _InstancerMap::iterator it = _instancerMap.begin();
    while (it != _instancerMap.end()) {
        const SdfPath &id = it->first;
        HdInstancer *instancer = it->second;

        if ((instancer->GetDelegate() == sceneDelegate) &&
            (id.HasPrefix(root))) {

            HdInstancer *instancer = it->second;
            SdfPath const& instancerId = instancer->GetParentId();
            if (!instancerId.IsEmpty()) {
                _tracker.RemoveInstancerInstancerDependency(instancerId, id);
            }

            _tracker.InstancerRemoved(id);

            instancer->Finalize(_renderDelegate->GetRenderParam());
            _renderDelegate->DestroyInstancer(instancer);

            // Need to capture the iterator and increment it because
            // TfHashMap::erase() doesn't return the next iterator, like
            // the stl version does.
            _InstancerMap::iterator nextIt = it;
            ++nextIt;
            _instancerMap.erase(it);
            it = nextIt;
        } else {
            ++it;
        }
    }
}


HdInstancer *
HdRenderIndex::GetInstancer(SdfPath const &id) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdInstancer *instancer = nullptr;
    TfMapLookup(_instancerMap, id, &instancer);

    return instancer;
}

HdRprim const *
HdRenderIndex::GetRprim(SdfPath const &id) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _RprimMap::const_iterator it = _rprimMap.find(id);
    if (it != _rprimMap.end()) {
        return it->second.rprim;
    }

    return nullptr;
}

HdSceneDelegate *
HdRenderIndex::GetSceneDelegateForRprim(SdfPath const &id) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_IsEnabledSceneIndexEmulation()) {
        // Applications expect this to return the original scene delegate
        // responsible for inserting the prim at the specified id.
        // Emulation must provide the same value -- even if it could
        // potentially expose the scene without downstream scene index
        // notifications -- or some application assumptions will fail.
        // No known render delegates make use of this call.
        HdSceneIndexPrim prim = _emulationSceneIndex->GetPrim(id);
        if (prim.dataSource) {
            if (auto ds = HdTypedSampledDataSource<HdSceneDelegate*>::Cast(
                    prim.dataSource->Get(
                        HdSceneIndexEmulationTokens->sceneDelegate))) {
                HdSceneDelegate *delegate = ds->GetTypedValue(0.0f);
                return delegate;
            }
        } 

        // fallback value is the back-end emulation delegate
        return _siSd.get();
    }

    _RprimMap::const_iterator it = _rprimMap.find(id);
    if (it != _rprimMap.end()) {
        const _RprimInfo &rprimInfo = it->second;

        return rprimInfo.sceneDelegate;
    }

    return nullptr;
}

bool
HdRenderIndex::GetSceneDelegateAndInstancerIds(SdfPath const &id,
                                               SdfPath* delegateId,
                                               SdfPath* instancerId) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _RprimMap::const_iterator it = _rprimMap.find(id);
    if (it != _rprimMap.end()) {
        const _RprimInfo &rprimInfo = it->second;

        if (_IsEnabledSceneIndexEmulation()) {
            // Applications expect this to return the original scene delegate
            // responsible for inserting the prim at the specified id.
            // Emulation must provide the same value -- even if it could
            // potentially expose the scene without downstream scene index
            // motifications -- or some application assumptions will fail.
            // No known render delegates make use of this call.
            HdSceneIndexPrim prim = _emulationSceneIndex->GetPrim(id);
            if (prim.dataSource) {
                if (auto ds = HdTypedSampledDataSource<HdSceneDelegate*>::Cast(
                        prim.dataSource->Get(
                            HdSceneIndexEmulationTokens->sceneDelegate))) {
                    HdSceneDelegate *delegate = ds->GetTypedValue(0.0f);
                    if (delegate) {
                        *delegateId = delegate->GetDelegateID();
                    }
                }
            } else {
                return false;
            }
        } else {
            *delegateId  = rprimInfo.sceneDelegate->GetDelegateID();
        }

        *instancerId = rprimInfo.rprim->GetInstancerId();

        return true;
    }

    return false;
}

void
HdRenderIndex::_InitPrimTypes()
{
    _sprimIndex.InitPrimTypes(_renderDelegate->GetSupportedSprimTypes());
    _bprimIndex.InitPrimTypes(_renderDelegate->GetSupportedBprimTypes());
}

bool
HdRenderIndex::IsRprimTypeSupported(TfToken const& typeId) const
{
    TfTokenVector const& supported = _renderDelegate->GetSupportedRprimTypes();
    return (std::find(supported.begin(), supported.end(), typeId) != supported.end());
}

bool
HdRenderIndex::IsSprimTypeSupported(TfToken const& typeId) const
{
    TfTokenVector const& supported = _renderDelegate->GetSupportedSprimTypes();
    return (std::find(supported.begin(), supported.end(), typeId) != supported.end());
}

bool
HdRenderIndex::IsBprimTypeSupported(TfToken const& typeId) const
{
    TfTokenVector const& supported = _renderDelegate->GetSupportedBprimTypes();
    return (std::find(supported.begin(), supported.end(), typeId) != supported.end());
}

void
HdRenderIndex::_AppendDrawItems(
                const SdfPathVector &rprimIds,
                size_t begin,
                size_t end,
                HdRprimCollection const& collection,
                _ConcurrentDrawItems* result)
{
    HdReprSelector const& colReprSelector = collection.GetReprSelector();
    bool forceColRepr = collection.IsForcedRepr();
    TfToken const& materialTag = collection.GetMaterialTag();
    
    HdDrawItemPtrVector &drawItems = result->local();

    if (materialTag.IsEmpty()) {
        // Get draw items for this thread.
        for (size_t idNum = begin; idNum < end; ++idNum)
        {
            const SdfPath &rprimId = rprimIds[idNum];

            _RprimMap::const_iterator it = _rprimMap.find(rprimId);
            if (it != _rprimMap.end()) {
                const _RprimInfo &rprimInfo = it->second;
                HdRprim *rprim = rprimInfo.rprim;

                // Append the draw items for each valid repr in the resolved
                // composite representation to the command buffer.
                HdReprSelector reprSelector = _GetResolvedReprSelector(
                                                    rprim->GetReprSelector(),
                                                    colReprSelector,
                                                    forceColRepr);

                for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i)
                {
                    if (reprSelector.IsActiveRepr(i)) {
                        TfToken const& reprToken = reprSelector[i];

                        for (const HdRepr::DrawItemUniquePtr &rprimDrawItem
                                : rprim->GetDrawItems(reprToken)) {
                            drawItems.push_back(rprimDrawItem.get());
                        }
                    }
                }
            }
        }
    } else {
        // Filter draw items by material tag.
        for (size_t idNum = begin; idNum < end; ++idNum)
        {
            const SdfPath &rprimId = rprimIds[idNum];

            _RprimMap::const_iterator it = _rprimMap.find(rprimId);
            if (it != _rprimMap.end()) {
                const _RprimInfo &rprimInfo = it->second;
                HdRprim *rprim = rprimInfo.rprim;

                // Append the draw items for each valid repr in the resolved
                // composite representation to the command buffer.
                HdReprSelector reprSelector = _GetResolvedReprSelector(
                                                    rprim->GetReprSelector(),
                                                    colReprSelector,
                                                    forceColRepr);

                for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i)
                {
                    if (reprSelector.IsActiveRepr(i)) {
                        TfToken const& reprToken = reprSelector[i];

                        for (const HdRepr::DrawItemUniquePtr &rprimDrawItem
                                : rprim->GetDrawItems(reprToken)) {
                            if (rprimDrawItem->GetMaterialTag() == materialTag)
                            {
                                drawItems.push_back(rprimDrawItem.get());
                            }   
                        }
                    }
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
