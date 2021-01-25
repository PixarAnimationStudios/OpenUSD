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
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/tf/pyLock.h"

#include <iostream>
#include <mutex>
#include <unordered_set>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

PXR_NAMESPACE_OPEN_SCOPE

HdRenderIndex::HdRenderIndex(
    HdRenderDelegate *renderDelegate,
    HdDriverVector const& drivers)
    :  _rprimMap()
    , _rprimIds()
    , _taskMap()
    , _sprimIndex()
    , _bprimIndex()
    , _tracker()
    , _instancerMap()
    , _syncQueue()
    , _renderDelegate(renderDelegate)
    , _drivers(drivers)
    , _activeRenderTags()
    , _renderTagVersion(_tracker.GetRenderTagVersion() - 1)
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
}

HdRenderIndex::~HdRenderIndex()
{
    HD_TRACE_FUNCTION();
    Clear();
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
HdRenderIndex::RemoveSubtree(const SdfPath &root,
                             HdSceneDelegate* sceneDelegate)
{
    HD_TRACE_FUNCTION();

    _RemoveRprimSubtree(root, sceneDelegate);
    _sprimIndex.RemoveSubtree(root, sceneDelegate, _tracker, _renderDelegate);
    _bprimIndex.RemoveSubtree(root, sceneDelegate, _tracker, _renderDelegate);
    _RemoveInstancerSubtree(root, sceneDelegate);
    _RemoveTaskSubtree(root, sceneDelegate);
}


void
HdRenderIndex::InsertRprim(TfToken const& typeId,
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

    _tracker.RprimInserted(rprimId, rprim->GetInitialDirtyBitsMask());
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
    TF_FOR_ALL(it, _rprimMap) {
        SdfPath const &id = it->first;
        _RprimInfo &rprimInfo = it->second;

        SdfPath const &instancerId = rprimInfo.rprim->GetInstancerId();
        if (!instancerId.IsEmpty()) {
            _tracker.RemoveInstancerRprimDependency(instancerId, id);
        }

        _tracker.RprimRemoved(id);

        // Ask delegate to actually delete the rprim
        rprimInfo.rprim->Finalize(_renderDelegate->GetRenderParam());
        _renderDelegate->DestroyRprim(rprimInfo.rprim);
        rprimInfo.rprim = nullptr;
    }
    // Clear Rprims, Rprim IDs, and delegate mappings.
    _rprimMap.clear();
    _rprimIds.Clear();
    _CompactPrimIds();

    // Clear S & B prims
    _sprimIndex.Clear(_tracker, _renderDelegate);
    _bprimIndex.Clear(_tracker, _renderDelegate);

    // Clear instancers.
    TF_FOR_ALL(it, _instancerMap) {
        SdfPath const &id = it->first;
        HdInstancer *instancer = it->second;

        SdfPath const &instancerId = instancer->GetParentId();
        if (!instancerId.IsEmpty()) {
            _tracker.RemoveInstancerInstancerDependency(instancerId, id);
        }

        _tracker.InstancerRemoved(id);

        instancer->Finalize(_renderDelegate->GetRenderParam());
        _renderDelegate->DestroyInstancer(instancer);
    }
    _instancerMap.clear();

    // Clear tasks.
    TF_FOR_ALL(it, _taskMap) {
        _tracker.TaskRemoved(it->first);
    }
    _taskMap.clear();
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

    _sprimIndex.InsertPrim(typeId, sceneDelegate, sprimId,
                           _tracker, _renderDelegate);
}

void
HdRenderIndex::RemoveSprim(TfToken const& typeId, SdfPath const& id)
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

    _bprimIndex.InsertPrim(typeId, sceneDelegate, bprimId,
                           _tracker, _renderDelegate);
}

void
HdRenderIndex::RemoveBprim(TfToken const& typeId, SdfPath const& id)
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
    const HdRprimCollection &collection;
    const TfTokenVector     &renderTags;
    const HdRenderIndex     *renderIndex;
};

static bool
_DrawItemFilterPredicate(const SdfPath &rprimID, const void *predicateParam)
{
    const _FilterParam *filterParam =
                              static_cast<const _FilterParam *>(predicateParam);

    const HdRprimCollection &collection  = filterParam->collection;
    const TfTokenVector     &renderTags  = filterParam->renderTags;
    const HdRenderIndex     *renderIndex = filterParam->renderIndex;

    //
    // Render Tag Filter
    //
    bool passedRenderTagFilter = false;
    if (renderTags.empty()) {
        // An empty render tag set means everything passes the filter
        // Primary user is tests, but some single task render delegates
        // that don't support render tags yet also use it.
        passedRenderTagFilter = true;
    } else {
        // As the number of tags is expected to be low (<10)
        // use a simple linear search.
        TfToken primRenderTag = renderIndex->GetRenderTag(rprimID);
        size_t numRenderTags = renderTags.size();
        size_t tagNum = 0;
        while (!passedRenderTagFilter && tagNum < numRenderTags) {
            if (renderTags[tagNum] == primRenderTag) {
                passedRenderTagFilter = true;
            }
            ++tagNum;
        }
    }

    //
    // Material Tag Filter
    //
    bool passedMaterialTagFilter = false;

    // Filter out rprims that do not match the collection's materialTag.
    // E.g. We may want to gather only opaque or translucent prims.
    // An empty materialTag on collection means: ignore material-tags.
    // This is important for tasks such as the selection-task which wants
    // to ignore materialTags and receive all prims in its collection.
    TfToken const& collectionMatTag = collection.GetMaterialTag();
    if (collectionMatTag.IsEmpty() ||
        renderIndex->GetMaterialTag(rprimID) == collectionMatTag) {
        passedMaterialTagFilter = true;
    }

   return (passedRenderTagFilter && passedMaterialTagFilter);
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

    _FilterParam filterParam = {collection, renderTags, this};

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

    return info->rprim->GetRenderTag(info->sceneDelegate);
}

TfToken
HdRenderIndex::GetMaterialTag(SdfPath const& id) const
{
    _RprimInfo const* info = TfMapLookupPtr(_rprimMap, id);
    if (info == nullptr) {
        return HdMaterialTagTokens->defaultMaterialTag;
    }

    return info->rprim->GetMaterialTag();
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
    struct _RprimSyncRequestVector {
        void PushBack(HdRprim *rprim,
                      size_t reprsMask,
                      HdDirtyBits dirtyBits)
        {
            rprims.push_back(rprim);
            reprsMasks.push_back(reprsMask);
            request.IDs.push_back(rprim->GetId());
            request.dirtyBits.push_back(dirtyBits);
        }

        std::vector<HdRprim *> rprims;
        std::vector<size_t> reprsMasks;

        HdSyncRequestVector request;

        _RprimSyncRequestVector() = default;
        // XXX: This is a heavy structure and should not be copied.
        //_RprimSyncRequestVector(const _RprimSyncRequestVector&) = delete;
        _RprimSyncRequestVector& operator =(const _RprimSyncRequestVector&) = delete;
    };

    typedef TfHashMap<HdSceneDelegate*,
                      _RprimSyncRequestVector, TfHash> _RprimSyncRequestMap;

    struct _Worker {
        _RprimSyncRequestMap* _map;
        std::vector<HdSceneDelegate*> _index;
    public:
        _Worker(_RprimSyncRequestMap* map) : _map(map)
        {
            _index.reserve(map->size());
            TF_FOR_ALL(dlgIt, *map) {
                _index.push_back(dlgIt->first);
            }
        }

        void Process(size_t begin, size_t end)
        {
            for (size_t i = begin; i < end; i++) {
                HdSceneDelegate* dlg = _index[i];
                _RprimSyncRequestVector* r = TfMapLookupPtr(*_map, dlg);
                dlg->Sync(&r->request);
            }
        }
    };

    struct _ReprSpec {
        _ReprSpec(HdReprSelector const &repr, bool forced) :
            reprSelector(repr), forcedRepr(forced) {}
        HdReprSelector reprSelector;
        bool forcedRepr;

        bool operator == (_ReprSpec const &other) const {
            return  (reprSelector == other.reprSelector) &&
                    (forcedRepr == other.forcedRepr);
        }
    };

    typedef std::vector<_ReprSpec> _ReprList;

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
        _ReprList const &_reprs;
        HdChangeTracker &_tracker;
        HdRenderParam *_renderParam;
    public:
        _SyncRPrims( HdSceneDelegate *sceneDelegate,
                     _RprimSyncRequestVector& r,
                     _ReprList const &reprs,
                     HdChangeTracker &tracker,
                     HdRenderParam *renderParam)
         : _sceneDelegate(sceneDelegate)
         , _r(r)
         , _reprs(reprs)
         , _tracker(tracker)
         , _renderParam(renderParam)
        {
        }

        void Sync(size_t begin, size_t end)
        {
            for (size_t i = begin; i < end; ++i)
            {
                HdRprim &rprim = *_r.rprims[i];
                size_t reprsMask = _r.reprsMasks[i];

                HdDirtyBits dirtyBits = _r.request.dirtyBits[i];

                for (const _ReprSpec& spec : _reprs) {
                    if (reprsMask & 1) {
                        HdReprSelector reprSelector = 
                            _GetResolvedReprSelector(rprim.GetReprSelector(),
                                                     spec.reprSelector,
                                                     spec.forcedRepr);

                        // Call Rprim::Sync(..) on each valid repr of the
                        // resolved  repr selector.
                        // The rprim's authored repr selector is
                        // guaranteed to have been set at this point (via
                        // InitRepr in the pre-sync)
                        for (size_t i = 0;
                             i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {

                            if (reprSelector.IsActiveRepr(i)) {
                                TfToken const& reprToken = reprSelector[i];

                                rprim.Sync(_sceneDelegate,
                                           _renderParam,
                                           &dirtyBits,
                                           reprToken);
                            }
                        }
                    }
                    reprsMask >>= 1;
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
                   _ReprList const& reprs,
                   size_t begin,
                   size_t end)
    {
        for (size_t i = begin; i < end; ++i)
        {
            HdRprim         *rprim         = syncReq->rprims[i];
            HdDirtyBits     &dirtyBits     = syncReq->request.dirtyBits[i];
            size_t          reprsMask      = syncReq->reprsMasks[i];

            // Initialize all utilized repr's for the rprim.
            //
            // The request vector is built by combining multiple rprim
            // collections and each collection can have it's own
            // repr.  The reprs param is a list of all the unique reprs used
            // by these collections.
            //
            // As such each Rprim can be included in multiple collections,
            // each Rprim can have multiple repr's in the same sync.
            // Thus the reprs mask, specifies which subset of reprs from the
            // repr list is used by collections the prim belongs to.
            //
            // An Rprim may require additional data to perform a sync of a repr
            // for the first time.  Therefore, inform the Rprim of the new repr
            // and give it the opportunity to modify the dirty bits in the
            // request before providing them to the scene delegate.
            //
            // The InitRepr bit is set when a collection changes and we need
            // to re-evalutate the repr state of a prim to ensure the repr
            // was initalised.
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

                for (const _ReprSpec& spec : reprs) {
                    if (reprsMask & 1) {
                        _InitRprimReprs(sceneDelegate,
                                        spec.reprSelector,
                                        spec.forcedRepr,
                                        rprim,
                                        &dirtyBits);
                    }
                    reprsMask >>= 1;
                }
                dirtyBits &= ~HdChangeTracker::InitRepr;
                // Update the InitRepr bit in the change tracker.
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
                          _ReprList const &reprs)
    {
        size_t numPrims = syncReq->rprims.size();
        WorkParallelForN(numPrims,
                         std::bind(&_PreSyncRPrims,
                                   sceneDelegate, tracker,
                                   syncReq, std::cref(reprs),
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
                    syncReq->reprsMasks.clear();
                    syncReq->request.IDs.clear();
                    syncReq->request.dirtyBits.clear();
                    ++primIdx;
                } else {

                    std::swap(syncReq->rprims[primIdx],
                              syncReq->rprims[numPrims -1]);
                    std::swap(syncReq->reprsMasks[primIdx],
                              syncReq->reprsMasks[numPrims -1]);
                    std::swap(syncReq->request.IDs[primIdx],
                              syncReq->request.IDs[numPrims -1]);
                    std::swap(syncReq->request.dirtyBits[primIdx],
                              syncReq->request.dirtyBits[numPrims -1]);

                    syncReq->rprims.pop_back();
                    syncReq->reprsMasks.pop_back();
                    syncReq->request.IDs.pop_back();
                    syncReq->request.dirtyBits.pop_back();
                    --numPrims;
                }
            } else {
                ++primIdx;
            }
        }
    }
};

void
HdRenderIndex::EnqueuePrimsToSync(
    HdDirtyListSharedPtr const &dirtyList,
    HdRprimCollection const &collection)
{
    _syncQueue.emplace_back(_SyncQueueEntry{dirtyList, collection});
}

struct _DirtyFilterParam {
    const HdRenderIndex* renderIndex;
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

    const HdRenderIndex* renderIndex = filterParam->renderIndex;
    HdDirtyBits mask = filterParam->mask;

    const HdChangeTracker& tracker = renderIndex->GetChangeTracker();

    if (mask == 0 || tracker.GetRprimDirtyBits(rprimID) & mask) {
        // An empty render tag set means everything passes the filter
        // Primary user is tests, but some single task render delegates
        // that don't support render tags yet also use it.
        if (filterParam->renderTags.empty()) {
            return true;
        }

        // As the number of tags is expected to be low (<10)
        // use a simple linear search.
        const TfToken& primRenderTag = renderIndex->GetRenderTag(rprimID);
        const size_t numRenderTags = filterParam->renderTags.size();
        for (size_t tagNum = 0u; tagNum < numRenderTags; ++tagNum) {
            if (filterParam->renderTags[tagNum] == primRenderTag) {
                return true;
            }
        }
   }

   return false;
}

const SdfPathVector&
HdRenderIndex::_GetDirtyRprimIds(HdDirtyBits mask)
{
    HD_TRACE_FUNCTION();

    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        // In safe mode, we clear the cached lists of dirty IDs, forcing a
        // fresh list to be generated with every call. This is primarily for
        // use in unit tests.
        _dirtyRprimIdsMap.clear();
    }

    // Look for a cached list of dirty IDs first and return that if we have it.
    const auto iter = _dirtyRprimIdsMap.find(mask);
    if (iter != _dirtyRprimIdsMap.cend()) {
        return iter->second;
    }

    // No cached list, so we need to generate one.
    SdfPathVector dirtyRprimIds;

    {
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

        const SdfPathVector& paths = GetRprimIds();

        _DirtyFilterParam filterParam = {this, _activeRenderTags, mask};

        HdPrimGather gather;

        gather.PredicatedFilter(
            paths,
            includePaths,
            excludePaths,
            _DirtyRprimIdsFilterPredicate,
            &filterParam,
            &dirtyRprimIds);
    }

    if (mask == 0) {
        // There may be new prims in the list that might have reprs they've not
        // seen before. Flag these up as needing re-evaluating.
        for (const SdfPath& dirtyRprimId : dirtyRprimIds) {
            _tracker.MarkRprimDirty(
                dirtyRprimId,
                HdChangeTracker::InitRepr);
        }
    }

    if (TfDebug::IsEnabled(HD_DIRTY_LIST)) {
        TF_DEBUG(HD_DIRTY_LIST).Msg("  dirtyRprimIds:\n");
        for (const SdfPath& dirtyRprimId : dirtyRprimIds) {
            TF_DEBUG(HD_DIRTY_LIST).Msg("    %s\n", dirtyRprimId.GetText());
        }
    }

    const auto inserted = _dirtyRprimIdsMap.emplace(
        std::make_pair(mask, std::move(dirtyRprimIds)));

    return inserted.first->second;
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
    // could be in parallel...
    //
    // These tasks will call Sync() adding dirty lists to _syncQueue for
    // processing below.
    //
    size_t numTasks = tasks->size();
    for (size_t taskNum = 0; taskNum < numTasks; ++taskNum) {
        HdTaskSharedPtr &task = (*tasks)[taskNum];

        if (!TF_VERIFY(task)) {
            TF_CODING_ERROR("Null Task in task list.  Entry Num: %zu", taskNum);
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

    ////////////////////////////////////////////////////////////////////////////
    //
    // Render Tag Gather
    //
    // Because the task list is not state tracked and can vary from
    // Sync to Sync as different views use different task lists.
    // Therefore, the render tags cannot be cached and need to be gathered
    // every Sync.
    _GatherRenderTags(tasks);

    // Merge IDs using the slow SdfPath less-than so that all delegate IDs group
    // together. Unfortunately, FastLessThan makes the optimization below less
    // effective, however the time to build the std::map dominates when using
    // the lexicographic less than.
    std::map<SdfPath, /*reprMask=*/size_t, SdfPath::FastLessThan> dirtyIds;
    _ReprList reprs;
    {
        HF_TRACE_FUNCTION_SCOPE("Merge Dirty Lists");
        // If dirty list prims are all sorted, we could do something more
        // efficient here.
        size_t numSyncQueueEntries = _syncQueue.size();
        for (size_t entryNum = 0; entryNum < numSyncQueueEntries; ++entryNum) {
            _SyncQueueEntry &entry = _syncQueue[entryNum];
            HdDirtyListSharedPtr &hdDirtyList = entry.dirtyList;
            HdRprimCollection const& collection = entry.collection;

            _ReprSpec reprSpec(collection.GetReprSelector(),
                               collection.IsForcedRepr());

            // find reprIndex and append it if not exists.
            int reprIndex = (int)reprs.size();
            for (size_t i = 0; i < reprs.size(); ++i) {
                if (reprs[i] == reprSpec) {
                    reprIndex = i;
                    break;
                }
            }
            if (reprIndex == (int)reprs.size()) {
                reprs.push_back(reprSpec);
            }

            // up-to 64 (collection's) reprs can be synced at once.
            // Note that per-prim repr is not limited here, so this
            // is a fair assumption.
            //
            // XXX: WBN to iterate SyncAll if there are more than 64 reprs
            //      in the extreme case.
            //
            if (!TF_VERIFY(reprIndex < 64)) {
                break;
            }

            // PERFORMANCE: this loop can be expensive.
            SdfPathVector const &dirtyPrims = hdDirtyList->GetDirtyRprims();

            size_t numDirtyPrims = dirtyPrims.size();
            for (size_t primNum = 0; primNum < numDirtyPrims; ++primNum) {
                SdfPath const &primPath = dirtyPrims[primNum];
                dirtyIds[primPath] |= (1ULL << reprIndex);
            }
        }
    }

    _RprimSyncRequestMap syncMap;
    bool resetVaryingState = false;
    {
        HF_TRACE_FUNCTION_SCOPE("Build Sync Map: Rprims");
        // Collect dirty Rprim IDs.
        HdSceneDelegate* curdel = nullptr;
        _RprimSyncRequestVector* curvec = nullptr;

        // PERFORMANCE: Hot loop.
        int numSkipped = 0;
        int numNonVarying = 0;
        TF_FOR_ALL(idIt, dirtyIds) {
            _RprimMap::const_iterator it = _rprimMap.find(idIt->first);
            if (!TF_VERIFY(it != _rprimMap.end())) {
                continue;
            }

            const _RprimInfo &rprimInfo = it->second;
            const SdfPath &rprimId = rprimInfo.rprim->GetId();
            HdDirtyBits dirtyBits =
                           _tracker.GetRprimDirtyBits(rprimId);
            size_t reprsMask = idIt->second;

            if (HdChangeTracker::IsClean(dirtyBits)) {
                numSkipped++;
                continue;
            }

            if (!HdChangeTracker::IsVarying(dirtyBits)) {
                ++numNonVarying;
            }

            // PERFORMANCE: This loop is constrained by memory access, avoid
            // re-fetching the sync request vector if possible.
            if (curdel != rprimInfo.sceneDelegate) {
                curdel = rprimInfo.sceneDelegate;
                curvec = &syncMap[curdel];
            }

            curvec->PushBack(rprimInfo.rprim,
                             reprsMask, dirtyBits);
        }

        // Use a heuristic to determine whether or not to destroy the entire
        // dirty state.  We say that if we've skipped more than 25% of the
        // rprims that were claimed dirty, then it's time to clean up this
        // list.
        // Alternatively if the list contains more the 10% rprims that
        // are not marked as varying.  (This can happen when prims are
        // invisible for example).
        //
        // This leads to performance improvements after many rprims
        // get dirty and then cleaned one, and the steady state becomes a
        // small number of dirty items.
        if (!dirtyIds.empty()) {
            resetVaryingState =
                ((float )numSkipped / (float)dirtyIds.size()) > 0.25f;

            resetVaryingState |=
                ((float )numNonVarying / (float)dirtyIds.size()) > 0.10f;


            if (TfDebug::IsEnabled(HD_VARYING_STATE)) {
                std::cout << "Dirty List Redundancy: "
                          << "Skipped  = "
                          << ((float )numSkipped * 100.0f /
                              (float)dirtyIds.size())
                          << "% (" <<  numSkipped << " / "
                          << dirtyIds.size() << ") "
                          << "Non-Varying  = "
                          << ((float )numNonVarying * 100.0f /
                              (float)dirtyIds.size())
                          << "% (" <<  numNonVarying << " / "
                          << dirtyIds.size() << ")"
                          << std::endl;
            }
        }
    }

    // Drop the GIL before we spawn parallel tasks.
    TF_PY_ALLOW_THREADS_IN_SCOPE();


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
        WorkArenaDispatcher dirtyBitDispatcher;

        TF_FOR_ALL(dlgIt, syncMap) {
            HdSceneDelegate *sceneDelegate = dlgIt->first;
            _RprimSyncRequestVector *r = &dlgIt->second;
            dirtyBitDispatcher.Run(
                                   std::bind(&_PreSyncRequestVector,
                                             sceneDelegate,
                                             &_tracker,
                                             r,
                                             std::cref(reprs)));

        }
        dirtyBitDispatcher.Wait();
    }

    {
        HF_TRACE_FUNCTION_SCOPE("Delegate Sync");
        // Dispatch synchronization work to each delegate.
        _Worker worker(&syncMap);
        WorkParallelForN(syncMap.size(),
                         std::bind(&_Worker::Process,
                                   std::ref(worker),
                                   std::placeholders::_1,
                                   std::placeholders::_2));
    }

    // Collect results and synchronize.
    WorkArenaDispatcher dispatcher;
    TF_FOR_ALL(dlgIt, syncMap) {
        HdSceneDelegate* sceneDelegate = dlgIt->first;
        _RprimSyncRequestVector& r = dlgIt->second;

        {
            _SyncRPrims workerState(sceneDelegate, r, reprs, _tracker, renderParam);

            if (!TfDebug::IsEnabled(HD_DISABLE_MULTITHREADED_RPRIM_SYNC) &&
                  sceneDelegate->IsEnabled(HdOptionTokens->parallelRprimSync)) {
                TRACE_SCOPE("Parallel Rprim Sync");
                // In the lambda below, we capture workerState by value and 
                // incur a copy in the std::bind because the lambda execution 
                // may be delayed (until we call Wait), resulting in
                // workerState going out of scope.
                dispatcher.Run([&r, workerState]() {
                    WorkParallelForN(r.rprims.size(),
                        std::bind(&_SyncRPrims::Sync, workerState,
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
    dispatcher.Wait();

    {
        HF_TRACE_FUNCTION_SCOPE("Clean Up");
        // Give Delegate's to do any post-parrellel work,
        // such as garbage collection.
        TF_FOR_ALL(dlgIt, syncMap) {
            HdSceneDelegate *delegate = dlgIt->first;
            delegate->PostSyncCleanup();
        }
        const HdSceneDelegatePtrVector& sprimDelegates =
            _sprimIndex.GetSceneDelegatesForDirtyPrims();
        for (HdSceneDelegate* delegate : sprimDelegates) {
            delegate->PostSyncCleanup();
        }

        if (resetVaryingState) {
            _tracker.ResetVaryingState();
        }


        // Clear all pending dirty lists
        _syncQueue.clear();

        // Clear the cached dirty rprim ID lists
        _dirtyRprimIdsMap.clear();
    }
}

void HdRenderIndex::_GatherRenderTags(const HdTaskSharedPtrVector *tasks)
{
    TfTokenVector currentRenderTags;
    size_t numTasks = tasks->size();
    for (size_t taskNum = 0; taskNum < numTasks; ++taskNum) {
        const HdTaskSharedPtr &task = (*tasks)[taskNum];

        const TfTokenVector &taskRenderTags = task->GetRenderTags();

        // Append this tasks render tags with those in the set.
        currentRenderTags.insert(currentRenderTags.end(),
                                 taskRenderTags.begin(),
                                 taskRenderTags.end());
    }

    // Deduplicate the render tag set
    std::sort(currentRenderTags.begin(), currentRenderTags.end());

    TfTokenVector::iterator newEnd =
            std::unique(currentRenderTags.begin(), currentRenderTags.end());

    currentRenderTags.erase(newEnd, currentRenderTags.end());

    unsigned int currentRenderTagVersion = _tracker.GetRenderTagVersion();
    if (currentRenderTagVersion != _renderTagVersion) {
        // If the render tag version has changed, we reset tracking of the
        // current set of render tags, the the new set built by this sync.
        _activeRenderTags.swap(currentRenderTags);
    } else {
        // As the tasks list is not consistent between runs of Sync and there
        // is no tracking of the when the task list changes.
        // The set of active render tags needs to build up over time.
        // This is an additive only aproach, with the list reset when something
        // marks render tags dirty in the change tracker.

        TfTokenVector combinedRenderTags;
        std::set_union(_activeRenderTags.cbegin(),
                       _activeRenderTags.cend(),
                       currentRenderTags.cbegin(),
                       currentRenderTags.cend(),
                       std::back_inserter(combinedRenderTags));

        if (_activeRenderTags != combinedRenderTags) {
            _activeRenderTags.swap(combinedRenderTags);

            // Mark render tags dirty to cause dirty list to rebuild with the
            // new active set
            _tracker.MarkRenderTagsDirty();
        }
    }

    // Active Render Tags have been updated, so update the version
    _renderTagVersion = currentRenderTagVersion;
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
    TF_FOR_ALL(it, _rprimMap) {
        it->second.rprim->SetPrimId(nextPrimId);
        _tracker.MarkRprimDirty(it->first, HdChangeTracker::DirtyPrimID);
        _rprimPrimIdMap[nextPrimId] = it->first;
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

        *delegateId  = rprimInfo.sceneDelegate->GetDelegateID();
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

    // Get draw items for this thread.
    HdDrawItemPtrVector &drawItems = result->local();

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

            for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {
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
}

PXR_NAMESPACE_CLOSE_SCOPE
