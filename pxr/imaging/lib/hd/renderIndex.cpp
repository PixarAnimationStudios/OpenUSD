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
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/drawItem.h"
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

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


HdRenderIndex::HdRenderIndex(HdRenderDelegate *renderDelegate)
    :  _rprimMap()
    , _rprimIds()
    , _rprimPrimIdMap()
    , _taskMap()
    , _sprimIndex()
    , _bprimIndex()
    , _tracker()
    , _nextPrimId(1)
    , _instancerMap()
    , _syncQueue()
    , _renderDelegate(renderDelegate)
{
    // Note: HdRenderIndex::New(...) guarantees renderDelegate is non-null.

    // Register well-known reprs (to be deprecated).
    static std::once_flag reprsOnce;
    std::call_once(reprsOnce, _ConfigureReprs);

    // Register well-known collection types (to be deprecated)
    // XXX: for compatibility and smooth transition,
    //      leave geometry collection for a while.
    _tracker.AddCollection(HdTokens->geometry);

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
HdRenderIndex::New(HdRenderDelegate *renderDelegate)
{
    if (renderDelegate == nullptr) {
        TF_CODING_ERROR(
            "Null Render Delegate provided to create render index");
        return nullptr;
    }
    return new HdRenderIndex(renderDelegate);
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
                 SdfPath const& rprimId,
                 SdfPath const& instancerId /*= SdfPath()*/)
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


    HdRprim *rprim = _renderDelegate->CreateRprim(typeId,
                                                  rprimId,
                                                  instancerId);
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

    SdfPath instanceId = rprim->GetInstancerId();

    if (!instanceId.IsEmpty()) {
        _tracker.InstancerRPrimInserted(instanceId, rprimId);
    }
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
        _tracker.InstancerRPrimRemoved(instancerId, id);
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
                    _tracker.InstancerRPrimRemoved(instancerId, id);
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
        _tracker.RprimRemoved(it->first);
        _RprimInfo &rprimInfo = it->second;
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
        _tracker.InstancerRemoved(it->first);
    }
    _instancerMap.clear();

    // Clear tasks.
    TF_FOR_ALL(it, _taskMap) {
        _tracker.TaskRemoved(it->first);
    }
    _taskMap.clear();

    // After clearing the index, all collections must be invalidated to force
    // any dependent state to be updated.
    _tracker.MarkAllCollectionsDirty();
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
    _tracker.TaskInserted(taskId);
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

HdSprim const*
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


HdResourceRegistry::TextureKey
HdRenderIndex::GetTextureKey(HdTextureResource::ID id) const
{
    HdResourceRegistry::TextureKey key = boost::hash_value(this);
    boost::hash_combine(key, id);

    return key;
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
    const HdRenderIndex     *renderIndex;
};

static bool
_DrawItemFilterPredicate(const SdfPath &rprimID, const void *predicateParam)
{
    const _FilterParam *filterParam =
                              static_cast<const _FilterParam *>(predicateParam);

    const HdRprimCollection &collection = filterParam->collection;
    const HdRenderIndex *renderIndex    = filterParam->renderIndex;

   if (collection.HasRenderTag(renderIndex->GetRenderTag(rprimID))) {
       return true;
   }

   return false;
}

HdRenderIndex::HdDrawItemView
HdRenderIndex::GetDrawItems(HdRprimCollection const& collection)
{
    HD_TRACE_FUNCTION();

    SdfPathVector rprimIds;

    const SdfPathVector &paths        = GetRprimIds();
    const SdfPathVector &includePaths = collection.GetRootPaths();
    const SdfPathVector &excludePaths = collection.GetExcludePaths();

    _FilterParam filterParam = {collection, this};

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
    HdDrawItemView finalResult;
    for (_FlattenDrawItems::iterator it  = result.begin();
                                     it != result.end();
                                   ++it) {
        const TfToken &rprimTag = it->first;
        HdDrawItemPtrVector &threadDrawItems = it->second;

        HdDrawItemPtrVector &finalDrawItems = finalResult[rprimTag];

        finalDrawItems.insert(finalDrawItems.end(),
                              threadDrawItems.begin(),
                              threadDrawItems.end());
    }

    return finalResult;
}

TfToken
HdRenderIndex::GetRenderTag(SdfPath const& id) const
{
    _RprimInfo const* info = TfMapLookupPtr(_rprimMap, id);
    if (info == nullptr) {
        return HdTokens->hidden;
    }

    return info->rprim->GetRenderTag(info->sceneDelegate);
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
HdRenderIndex::Sync(HdDirtyListSharedPtr const &dirtyList)
{
    _syncQueue.push_back(dirtyList);
}

void
HdRenderIndex::SyncAll(HdTaskSharedPtrVector const &tasks,
                       HdTaskContext *taskContext)
{
    HD_TRACE_FUNCTION();

    HdRenderParam *renderParam = _renderDelegate->GetRenderParam();

    _bprimIndex.SyncPrims(_tracker, _renderDelegate->GetRenderParam());

    // Texture Bprims contain a deduplication system, where certain paramters
    // such as maximum texture size are resolved by looking at all the
    // references.
    //
    // At this point new textures may have been added to the system, but old
    // references haven't been removed yet.
    //
    // As Material Sprims need the resolved state of the textures.
    // Therefore, the old references are cleaned up here, before Sprim
    // processing
    if (_tracker.IsBprimGarbageCollectionNeeded()) {
        HdResourceRegistrySharedPtr registry = GetResourceRegistry();

        registry->GarbageCollectBprims();

        _tracker.ClearBprimGarbageCollectionNeeded();
    }


    _sprimIndex.SyncPrims(_tracker, _renderDelegate->GetRenderParam());

    // could be in parallel... but how?
    // may be just gathering dirtyLists at first, and then index->sync()?

    // These tasks will call Sync() adding dirty lists to _syncQueue for
    // processing below.
    TF_FOR_ALL(it, tasks) {
        if (!TF_VERIFY(*it)) {
            continue;
        }

        SdfPath taskId = (*it)->GetId();

        // Is this a tracked task?
        _TaskMap::iterator taskMapIt = _taskMap.find(taskId);
        if (taskMapIt != _taskMap.end()) {
            _TaskInfo &taskInfo = taskMapIt->second;

            // If in the task is in the render index, then we have to
            // possibility that the task pass in points to a
            // different instance than the one stored in the render index
            // even though they have the same id.
            //
            // For consistency, we always use the registered task in the
            // render index for a given id, as that is the one the state is
            // tracked for.
            //
            // However, this is still a weird situation, so report the
            // issue as a verify so it can be addressed.
            TF_VERIFY(taskInfo.task == (*it));

            HdDirtyBits taskDirtyBits = _tracker.GetTaskDirtyBits(taskId);

            taskInfo.task->Sync(taskInfo.sceneDelegate,
                                taskContext,
                                &taskDirtyBits);

            _tracker.MarkTaskClean(taskId, taskDirtyBits);

        } else {
            // Dummy dirty bits
            HdDirtyBits taskDirtyBits = 0;

            // This is an untracked task, never added to the render index.
            (*it)->Sync(nullptr,
                        taskContext,
                        &taskDirtyBits);
        }
    }

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
        for (auto const& hdDirtyList : _syncQueue) {
            HdRprimCollection const& collection = hdDirtyList->GetCollection();

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
            for (auto const& sdfPath : hdDirtyList->GetDirtyRprims()) {
                dirtyIds[sdfPath] |= (1ULL << reprIndex);
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

        // Clear out the dirty list for future consumers.
        for (auto const& hdDirtyList : _syncQueue) {
            hdDirtyList->Clear();
        }

        if (resetVaryingState) {
            _tracker.ResetVaryingState();
        }


        // Clear all pending dirty lists
        _syncQueue.clear();
    }
}

void
HdRenderIndex::_CompactPrimIds()
{
    _rprimPrimIdMap.clear();
    // Start prim id as 1 because background for id
    // render is black (id 0)
    _nextPrimId = 1;
    TF_FOR_ALL(it, _rprimMap) {
        it->second.rprim->SetPrimId(_nextPrimId);
        _tracker.MarkRprimDirty(it->first, HdChangeTracker::DirtyPrimID);
        _rprimPrimIdMap[_nextPrimId] = it->first;
        ++_nextPrimId;
    }

}

void
HdRenderIndex::_AllocatePrimId(HdRprim *prim)
{
    int32_t maxId = (1 << 24) - 1;
    if(_nextPrimId > maxId) {
        // We are wrapping around our max prim id.. time to reallocate
        _CompactPrimIds();
        // Make sure we have a valid next id after compacting
        TF_VERIFY(_nextPrimId <= maxId);
    }
    prim->SetPrimId(_nextPrimId);
    // note: not marking DirtyPrimID here to avoid undesirable variability tracking.
    _rprimPrimIdMap[_nextPrimId] = prim->GetId();

    ++ _nextPrimId;
}

SdfPath
HdRenderIndex::GetRprimPathFromPrimId(int primId) const
{
    _RprimPrimIDMap::const_iterator it = _rprimPrimIdMap.find(primId);
    if(it == _rprimPrimIdMap.end()) {
        return SdfPath();
    }

    return it->second;
}

void
HdRenderIndex::InsertInstancer(HdSceneDelegate* delegate,
                               SdfPath const &id,
                               SdfPath const &parentId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

#if 0
    // TODO: enable this after patching.
    if (!id.IsAbsolutePath()) {
        TF_CODING_ERROR("All Rprim IDs must be absolute paths <%s>\n",
                id.GetText());
        return;
    }
#endif

    HdInstancer *instancer =
        _renderDelegate->CreateInstancer(delegate, id, parentId);

    if (instancer == nullptr) {
        return;
    }

    _instancerMap[id] = instancer;
    _tracker.InstancerInserted(id);
}

void
HdRenderIndex::RemoveInstancer(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _InstancerMap::iterator it = _instancerMap.find(id);
    if (it == _instancerMap.end())
        return;

    _renderDelegate->DestroyInstancer(it->second);

    _tracker.InstancerRemoved(id);
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
            _renderDelegate->DestroyInstancer(it->second);

            _tracker.InstancerRemoved(id);

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

    // Get draw item view for this thread.
    HdDrawItemView &drawItemView = result->local();

    for (size_t idNum = begin; idNum < end; ++idNum)
    {
        const SdfPath &rprimId = rprimIds[idNum];

        _RprimMap::const_iterator it = _rprimMap.find(rprimId);
        if (it != _rprimMap.end()) {
            const _RprimInfo &rprimInfo = it->second;
            HdRprim *rprim = rprimInfo.rprim;

            // Append the draw items for each valid repr in the resolved
            // composite representation to the right command buffer
            // based on the tag.
            const TfToken &rprimTag = rprim->GetRenderTag(
                                               rprimInfo.sceneDelegate);

            HdDrawItemPtrVector &resultDrawItems = drawItemView[rprimTag];

            HdReprSelector reprSelector = _GetResolvedReprSelector(
                                                rprim->GetReprSelector(),
                                                colReprSelector,
                                                forceColRepr);

            for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {
                if (reprSelector.IsActiveRepr(i)) {
                    TfToken const& reprToken = reprSelector[i];

                    const HdRprim::HdDrawItemPtrVector* drawItems =
                        rprim->GetDrawItems(reprToken);

                    TF_VERIFY(drawItems);

                    resultDrawItems.insert( resultDrawItems.end(),
                                            drawItems->begin(),
                                            drawItems->end() );
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
