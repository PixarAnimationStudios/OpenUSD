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
#include "pxr/usdImaging/usdImaging/delegate.h"

#include "pxr/usdImaging/usdImaging/adapterRegistry.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/instanceAdapter.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/basisCurvesTopology.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/kind/registry.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/modelAPI.h"

#include "pxr/usd/usdLux/domeLight.h"

#include "pxr/base/work/loops.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/type.h"

#include <functional>
#include <limits>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


// TODO: 
// reduce material traversals to a single pass; currently usdImaging will traverse
// a single material network multiple times during sync.

// XXX: Perhaps all interpolation tokens for Hydra should come from Hd and
// UsdGeom tokens should be passed through a mapping function.
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (instance)
    (texturePath)
    (Material)
    (HydraPbsSurface)
);

// This environment variable matches a set of similar ones in
// primAdapter.cpp, controlling other attribute caches.
TF_DEFINE_ENV_SETTING(USDIMAGING_ENABLE_DRAWMODE_CACHE, 1,
                      "Enable a cache for model:drawMode.");
static bool _IsEnabledDrawModeCache() {
    static bool _v = TfGetEnvSetting(USDIMAGING_ENABLE_DRAWMODE_CACHE) == 1;
    return _v;
}

// -------------------------------------------------------------------------- //
// Delegate Implementation.
// -------------------------------------------------------------------------- //
constexpr int UsdImagingDelegate::ALL_INSTANCES;

UsdImagingDelegate::UsdImagingDelegate(
        HdRenderIndex *parentIndex,
        SdfPath const& delegateID)
    : HdSceneDelegate(parentIndex, delegateID)
    , _valueCache()
    , _rootXf(1.0)
    , _rootIsVisible(true)
    , _time(std::numeric_limits<double>::infinity())
    , _refineLevelFallback(0)
    , _reprFallback()
    , _cullStyleFallback(HdCullStyleDontCare)
    , _xformCache(GetTime())
    , _materialBindingImplData(parentIndex->GetRenderDelegate()->
                               GetMaterialBindingPurpose())
    , _materialBindingCache(GetTime(), &_materialBindingImplData)
    , _visCache(GetTime())
    , _purposeCache() // note that purpose is uniform, so no GetTime()
    , _drawModeCache(GetTime())
    , _displayGuides(true)
    , _enableUsdDrawModes(true)
    , _hasDrawModeAdapter( UsdImagingAdapterRegistry::GetInstance()
                           .HasAdapter(UsdImagingAdapterKeyTokens
                                       ->drawModeAdapterKey) )
    , _sceneMaterialsEnabled(true)
{
    // Default to 2 samples: this frame and the next frame.
    // XXX In the future this should be configurable via negotation
    // between frontend and backend, or be provided otherwise.
    _timeSampleOffsets.push_back(0.0);
    _timeSampleOffsets.push_back(1.0);
}

UsdImagingDelegate::~UsdImagingDelegate()
{
    TfNotice::Revoke(_objectsChangedNoticeKey);

    // Remove all prims from the render index.

    // Even though this delegate is going out of scope
    // the render index may not be.  So, need to make
    // sure we properly remove all prims from the
    // render index.
    //
    // Note: This is not going through the adapters
    // as we are destroying the whole delegate.  It is
    // assumed that adapters are not shared between delegates.
    HdRenderIndex& index = GetRenderIndex();
    index.RemoveSubtree(GetDelegateID(), this);

    _instancerPrimPaths.clear();
    _refineLevelMap.clear();
    _pickablesMap.clear();
    _primInfoMap.clear();
    _usdIds.Clear();
    _adapterMap.clear();
}


bool
UsdImagingDelegate::_IsDrawModeApplied(UsdPrim const& prim)
{
    // Compute the inherited drawMode.
    TfToken drawMode = _GetModelDrawMode(prim);
    // If draw mode is "default", no draw mode is applied.
    if (drawMode == UsdGeomTokens->default_) {
        return false;
    }

    // Draw mode is only applied on models that are components, or which have
    // applyDrawMode = true.
    UsdModelAPI model(prim);
    bool applyDrawMode = false;
    TfToken kind;
    UsdAttribute attr;
    if (model.GetKind(&kind) && KindRegistry::IsA(kind, KindTokens->component))
        applyDrawMode = true;
    else {
        UsdGeomModelAPI geomModel(prim);
        if ((attr = geomModel.GetModelApplyDrawModeAttr()))
            attr.Get(&applyDrawMode);
    }

    return applyDrawMode;
}


TfToken
UsdImagingDelegate::_GetModelDrawMode(UsdPrim const& prim)
{
    HD_TRACE_FUNCTION();

    // Draw modes can only be applied to models.
    if (!prim.IsModel()) { return UsdGeomTokens->default_; }

    // Draw modes can't be applied to the pseudo-root.
    if (!prim.GetParent()) { return UsdGeomTokens->default_; }

    if (_IsEnabledDrawModeCache())
        return _drawModeCache.GetValue(prim);
    else
        return UsdImaging_DrawModeStrategy::ComputeDrawMode(prim);
}


UsdImagingDelegate::_AdapterSharedPtr const& 
UsdImagingDelegate::_AdapterLookup(UsdPrim const& prim, bool ignoreInstancing)
{
    static UsdImagingDelegate::_AdapterSharedPtr const NULL_ADAPTER;

    // Future Work:
    //  * Only enable plugins on demand.
    //
    //  * Implement a more robust prim typename mapping. This could be a
    //    secondary map from TfType->token to avoid TfType locks in background
    //    threads.

    TfToken adapterKey;
    if (!ignoreInstancing && prim.IsInstance()) {
        adapterKey = UsdImagingAdapterKeyTokens->instanceAdapterKey;
    } else if (_hasDrawModeAdapter && _enableUsdDrawModes &&
               _IsDrawModeApplied(prim)) {
        adapterKey = UsdImagingAdapterKeyTokens->drawModeAdapterKey;
    } else {
        adapterKey = prim.GetTypeName();
        // XXX: transitional code
        // If we are using material networks, we want Looks to be 
        // treated like Materials. When not using networks,
        // we want Shaders to be treated like HydraPbsSurface
        // for backwards compatibility.
        TfToken bindingPurpose = GetRenderIndex().
            GetRenderDelegate()->GetMaterialBindingPurpose();
        if (bindingPurpose == HdTokens->preview &&
            adapterKey == _tokens->Material) {
            adapterKey = _tokens->HydraPbsSurface;
        }
    }

    _AdapterMap::const_iterator it = _adapterMap.find(adapterKey);
    if (it != _adapterMap.end())
        return it->second;

    UsdImagingAdapterRegistry& reg = UsdImagingAdapterRegistry::GetInstance();
    _AdapterSharedPtr adapter(reg.ConstructAdapter(adapterKey));

    // For prims that have no PrimAdapter, adapter will be NULL.
    // If the adapter type isn't supported by the render index,
    // we force the adapter to be null.
    if (adapter) {
        UsdImagingIndexProxy indexProxy(this, nullptr);
        if (adapter->IsSupported(&indexProxy)) {
            adapter->SetDelegate(this);
        } else {
            TF_WARN("Selected hydra renderer doesn't support prim type '%s'",
                    adapterKey.GetText());
            adapter.reset();
        }
    }

    // NULL adapters are also cached, to avoid redundant lookups.
    return _adapterMap.insert(
        std::make_pair(adapterKey, adapter)).first->second;
}

UsdImagingDelegate::_PrimInfo *
UsdImagingDelegate::GetPrimInfo(const SdfPath &usdPath)
{
    _PrimInfoMap::iterator it = _primInfoMap.find(usdPath);
    if (it == _primInfoMap.end()) {
        return nullptr;
    }

    return &(it->second);
}

// -------------------------------------------------------------------------- //
// Parallel Dispatch
// -------------------------------------------------------------------------- //

class UsdImagingDelegate::_Worker {
public:
    typedef std::vector<std::pair<SdfPath, int> > ResultVector;

private:
    struct _Task {
        _Task() : delegate(nullptr) { }
        _Task(UsdImagingDelegate* delegate_, const SdfPath& path_)
            : delegate(delegate_)
            , path(path_)
        {
        }

        UsdImagingDelegate* delegate;
        SdfPath path;
    };
    std::vector<_Task> _tasks;

public:
    _Worker()
    {
    }

    void AddTask(UsdImagingDelegate* delegate, SdfPath const& usdPath) {
        _tasks.push_back(_Task(delegate, usdPath));
    }

    size_t GetTaskCount() {
        return _tasks.size();
    }

    // Disables value cache mutations for all imaging delegates that have
    // added tasks to this worker.
    void DisableValueCacheMutations() {
        TF_FOR_ALL(it, _tasks) {
            it->delegate->_valueCache.DisableMutation();
        }
    }

    // Enables value cache mutations for all imaging delegates that have
    // added tasks to this worker.
    void EnableValueCacheMutations() {
        TF_FOR_ALL(it, _tasks) {
            it->delegate->_valueCache.EnableMutation();
        }
    }

    // Preps all tasks for parallel update.
    void UpdateVariabilityPrep() {
        TF_FOR_ALL(it, _tasks) {
            UsdImagingDelegate* delegate = it->delegate;
            SdfPath const& usdPath = it->path;

            _PrimInfo *primInfo = delegate->GetPrimInfo(usdPath);
            if (TF_VERIFY(primInfo, "%s\n", usdPath.GetText())) {
                _AdapterSharedPtr const& adapter = primInfo->adapter;
                if (TF_VERIFY(adapter, "%s\n", usdPath.GetText())) {
                    adapter->TrackVariabilityPrep(primInfo->usdPrim, usdPath);
                }
            }
        }
    }

    // Populates prim variability and initial state.
    // Used as a parallel callback method for use with WorkParallelForN.
    void UpdateVariability(size_t start, size_t end) {
        for (size_t i = start; i < end; i++) {
            UsdImagingDelegate* delegate = _tasks[i].delegate;
            UsdImagingIndexProxy indexProxy(delegate, nullptr);
            SdfPath const& usdPath = _tasks[i].path;

            _PrimInfo *primInfo = delegate->GetPrimInfo(usdPath);
            if (TF_VERIFY(primInfo, "%s\n", usdPath.GetText())) {
                _AdapterSharedPtr const& adapter = primInfo->adapter;
                if (TF_VERIFY(adapter, "%s\n", usdPath.GetText())) {
                    adapter->TrackVariability(primInfo->usdPrim,
                                              usdPath,
                                              &primInfo->timeVaryingBits);
                    if (primInfo->timeVaryingBits != HdChangeTracker::Clean) {
                        adapter->MarkDirty(primInfo->usdPrim,
                                           usdPath,
                                           primInfo->timeVaryingBits,
                                           &indexProxy);
                    }
                }
            }
        }
    }

    // Updates prim data on time change.
    // Used as a parallel callback method for use with WorkParallelForN.
    void UpdateForTime(size_t start, size_t end) {
        for (size_t i = start; i < end; i++) {
            UsdImagingDelegate* delegate = _tasks[i].delegate;
            UsdTimeCode const& time = delegate->_time;
            SdfPath const& usdPath = _tasks[i].path;

            _PrimInfo *primInfo = delegate->GetPrimInfo(usdPath);
            if (TF_VERIFY(primInfo, "%s\n", usdPath.GetText())) {
                _AdapterSharedPtr const& adapter = primInfo->adapter;
                if (TF_VERIFY(adapter, "%s\n", usdPath.GetText())) {
                    adapter->UpdateForTime(primInfo->usdPrim,
                                           usdPath,
                                           time,
                                           primInfo->dirtyBits);

                    // Prim is now clean
                    primInfo->dirtyBits = 0;
                }
            }
        }
    }
};

void 
UsdImagingDelegate::_AddTask(
    UsdImagingDelegate::_Worker *worker, SdfPath const& usdPath)
{
    worker->AddTask(this, usdPath);
}

// -------------------------------------------------------------------------- //
// Population & Update
// -------------------------------------------------------------------------- //

void
UsdImagingDelegate::SyncAll(bool includeUnvarying)
{
    UsdImagingDelegate::_Worker worker;

    TF_FOR_ALL(it, _primInfoMap) {
        const SdfPath &usdPath = it->first;
        _PrimInfo &primInfo    = it->second;

        if (includeUnvarying) {
            primInfo.dirtyBits |= HdChangeTracker::AllDirty;
        } else if (primInfo.dirtyBits == HdChangeTracker::Clean) {
            continue;
        }

        // In this case, the path is coming from our internal state, so it is
        // not prefixed with the delegate ID.
        _AdapterSharedPtr adapter = primInfo.adapter;

        if (TF_VERIFY(adapter, "%s\n", usdPath.GetText())) {
            TF_DEBUG(USDIMAGING_UPDATES).Msg(
                      "[Sync] PREP: <%s> dirtyFlags: 0x%x [%s]\n",
                      usdPath.GetText(), 
                      primInfo.dirtyBits,
                      HdChangeTracker::StringifyDirtyBits(
                                                   primInfo.dirtyBits).c_str());

            adapter->UpdateForTimePrep(primInfo.usdPrim,
                                       usdPath,
                                       _time,
                                       primInfo.dirtyBits);
            worker.AddTask(this, usdPath);
        }
    }

    _ExecuteWorkForTimeUpdate(&worker);
}

void
UsdImagingDelegate::Sync(HdSyncRequestVector* request)
{
    UsdImagingDelegate::_Worker worker;
    if (!TF_VERIFY(request)) {
        return;
    }
    if (!TF_VERIFY(request->IDs.size() == request->dirtyBits.size())) {
        return;
    }

    // Iterate over each HdSyncRequest.
    for (size_t i = 0; i < request->IDs.size(); i++) {

        // Note that the incoming ID may be prefixed with the DelegateID, so we
        // must translate it via GetPathForUsd.
        SdfPath usdPath = GetPathForUsd(request->IDs[i]);
        HdDirtyBits dirtyFlags = request->dirtyBits[i];

        _PrimInfo *primInfo = GetPrimInfo(usdPath);
        if (!TF_VERIFY(primInfo != nullptr, "%s\n", usdPath.GetText())) {
            continue;
        }

        // Merge UsdImaging's own dirty flags with those coming from hydra.
        primInfo->dirtyBits |= dirtyFlags;

        _AdapterSharedPtr &adapter = primInfo->adapter;
        if (TF_VERIFY(adapter, "%s\n", usdPath.GetText())) {
            TF_DEBUG(USDIMAGING_UPDATES).Msg(
                    "[Sync] PREP: <%s> dirtyFlags: 0x%x [%s]\n",
                    usdPath.GetText(), 
                    primInfo->dirtyBits,
                    HdChangeTracker::StringifyDirtyBits(primInfo->dirtyBits).c_str());

            adapter->UpdateForTimePrep(primInfo->usdPrim,
                                       usdPath,
                                       _time,
                                       primInfo->dirtyBits);
            worker.AddTask(this, usdPath);
        }
    }

    // We always include instancers.
    TF_FOR_ALL(it, _instancerPrimPaths) {
        // In this case, the path is coming from our internal state, so it is
        // not prefixed with the delegate ID.
        SdfPath usdPath = *it;

        _PrimInfo *primInfo = GetPrimInfo(usdPath);
        if (!TF_VERIFY(primInfo != nullptr, "%s\n", usdPath.GetText())) {
            continue;
        }

        if (primInfo->dirtyBits == HdChangeTracker::Clean) {
            continue;
        }

        _AdapterSharedPtr &adapter = primInfo->adapter;
        if (TF_VERIFY(adapter, "%s\n", usdPath.GetText())) {
            TF_DEBUG(USDIMAGING_UPDATES).Msg(
                    "[Sync] PREP Instancer: <%s> dirtyFlags: 0x%x [%s]\n",
                    usdPath.GetText(),
                    primInfo->dirtyBits,
                    HdChangeTracker::StringifyDirtyBits(
                                              primInfo->dirtyBits).c_str());
            adapter->UpdateForTimePrep(primInfo->usdPrim,
                                       usdPath,
                                       _time,
                                       primInfo->dirtyBits);
            worker.AddTask(this, usdPath);
        }
    }

    _ExecuteWorkForTimeUpdate(&worker);
}

void
UsdImagingDelegate::PostSyncCleanup()
{
    _valueCache.GarbageCollect();
}

void
UsdImagingDelegate::Populate(UsdPrim const& rootPrim)
{
    SdfPathVector empty;
    Populate(rootPrim, empty);
}

void
UsdImagingDelegate::Populate(UsdPrim const& rootPrim, 
                             SdfPathVector const& excludedPrimPaths,
                             SdfPathVector const &invisedPrimPaths)
{
    HD_TRACE_FUNCTION();

    if (!_CanPopulate(rootPrim))
        return;

    _SetStateForPopulation(rootPrim, excludedPrimPaths, invisedPrimPaths);

    UsdImagingDelegate::_Worker worker;
    UsdImagingIndexProxy indexProxy(this, &worker);

    indexProxy.Repopulate(rootPrim.GetPath());

    _Populate(&indexProxy);
    _ExecuteWorkForVariabilityUpdate(&worker);
}

bool 
UsdImagingDelegate::_CanPopulate(UsdPrim const& rootPrim) const
{
    // Currently, Populate is only allowed to be called once, but we could relax
    // this restriction if there is a need to do so.
    //
    // If we change this, we must also revoke the objectsChangedNoticeKey.
    if (!TF_VERIFY(!_stage, "Attempted to call Populate more than once"))
        return false;

    if (!rootPrim) {
        TF_CODING_ERROR("Expired rootPrim \n");
        return false;
    }

    return true;
}

void 
UsdImagingDelegate::_SetStateForPopulation(
    UsdPrim const& rootPrim,
    SdfPathVector const& excludedPrimPaths,
    SdfPathVector const& invisedPrimPaths)
{
    if (_stage)
        return;

    // Hold onto the stage from which we will be drawing. The delegate will keep
    // the stage alive, holding it by strong reference.
    _stage = rootPrim.GetStage();
    _rootPrimPath = rootPrim.GetPath();
    _excludedPrimPaths = excludedPrimPaths;
    _invisedPrimPaths = invisedPrimPaths;

    // Set the root path of the inherited transform cache.
    // XXX: Ideally, we'd like to deprecate the inherited cache's SetRootPath(),
    // but the root prim is defined as having identity transform over all time,
    // even when its transform within the full stage is animated; and transform
    // overrides are defined as relative to the root prim.  This means resolving
    // transforms without involving the inherited cache is impossible.
    //
    // If the transform override mechanism is deprecated in favor of a USD
    // session layer, we could do something nicer here.
    _xformCache.SetRootPath(_rootPrimPath);

    // Start listening for change notices from this stage.
    UsdImagingDelegatePtr self = TfCreateWeakPtr(this);
    _objectsChangedNoticeKey = 
                     TfNotice::Register(self, &This::_OnObjectsChanged, _stage);
}

namespace {
    struct _PopulateMaterialBindingCache {
        UsdPrim primToBind;
        UsdImaging_MaterialBindingCache const* materialBindingCache;
        void operator()() const {
            // Just calling GetValue will populate the cache for this prim and
            // potentially all ancestors.
            materialBindingCache->GetValue(primToBind);
        }
    };
};

void
UsdImagingDelegate::_Populate(UsdImagingIndexProxy* proxy)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPathVector const& pathsToRepopulate = proxy->_GetPathsToRepopulate();
    if (pathsToRepopulate.empty())
        return;

    // Force initialization of SchemaRegistry (doing this in parallel causes all
    // threads to block).
    UsdSchemaRegistry::GetInstance();

    // Build a TfHashSet of excluded prims for fast rejection.
    TfHashSet<SdfPath, SdfPath::Hash> excludedSet;
    TF_FOR_ALL(pathIt, _excludedPrimPaths) {
        excludedSet.insert(*pathIt);
    }

    TF_DEBUG(USDIMAGING_CHANGES)
        .Msg("[Repopulate] Populating <%s> on stage %s\n",
             _rootPrimPath.GetString().c_str(),
             _stage->GetRootLayer()->GetDisplayName().c_str());

    WorkDispatcher bindingDispatcher;

    // For each root that has been scheduled for repopulation
    std::vector<std::pair<UsdPrim, _AdapterSharedPtr> > leafPaths;
    leafPaths.reserve(pathsToRepopulate.size());

    TF_FOR_ALL(rootPathIt, pathsToRepopulate) {
        // Discover and insert all renderable prims into the worker for later
        // execution.
        TF_DEBUG(USDIMAGING_CHANGES).Msg("[Repopulate] Root path: <%s>\n",
                            rootPathIt->GetText());

        UsdPrimRange range(_GetPrim(*rootPathIt));
        for (auto iter = range.begin(); iter != range.end(); ++iter) {
            if (!iter->GetPath().HasPrefix(_rootPrimPath)) {
                iter.PruneChildren();
                TF_DEBUG(USDIMAGING_CHANGES).Msg("[Repopulate] Pruned at <%s> "
                            "not under root prim path <%s>\n",
                            iter->GetPath().GetText(),
                            _rootPrimPath.GetText());
                continue;
            }
            if (excludedSet.find(iter->GetPath()) != excludedSet.end()) {
                iter.PruneChildren();
                TF_DEBUG(USDIMAGING_CHANGES).Msg("[Repopulate] Pruned at <%s> "
                            "due to exclusion list\n",
                            iter->GetPath().GetText());
                continue;
            }
            if (UsdImagingPrimAdapter::ShouldCullSubtree(*iter)) {
                iter.PruneChildren();
                TF_DEBUG(USDIMAGING_CHANGES).Msg("[Repopulate] Pruned at <%s> "
                            "due to prim type <%s>\n",
                            iter->GetPath().GetText(),
                            iter->GetTypeName().GetText());
                continue;
            }
            if (_AdapterSharedPtr adapter = _AdapterLookup(*iter)) {
                // Schedule the prim for population and discovery
                // of material bindings.
                //
                // If we are using full networks, we will populate the 
                // binding cache that has the strategy to compute the correct
                // bindings.
                _PopulateMaterialBindingCache wu = 
                    { *iter, &_materialBindingCache};
                 bindingDispatcher.Run(wu);
                
                leafPaths.push_back(std::make_pair(*iter, adapter));
                if (adapter->ShouldCullChildren()) {
                   TF_DEBUG(USDIMAGING_CHANGES).Msg("[Repopulate] Pruned "
                                    "children of <%s> due to adapter\n",
                            iter->GetPath().GetText());
                   iter.PruneChildren();
                }
            }
        }
    }

    // Populate the RenderIndex while we're still discovering material bindings.
    TF_FOR_ALL(it, leafPaths) {
        it->second->Populate(it->first, proxy);
    }

    // In the event that Population finishes before look binding cache 
    // population, we must block here to ensure it isn't running in the
    // background, since we might mutate the look binding cache later.
    bindingDispatcher.Wait();
}

void 
UsdImagingDelegate::_ExecuteWorkForVariabilityUpdate(_Worker* worker)
{
    HD_TRACE_FUNCTION();

    TF_DEBUG(USDIMAGING_CHANGES)
        .Msg("[Repopulate] %zu variability tasks in worker\n", 
             worker->GetTaskCount());

    worker->UpdateVariabilityPrep(); 
    worker->DisableValueCacheMutations();
    {
        // Release the GIL to ensure that threaded work won't deadlock if
        // they also need the GIL.
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        WorkParallelForN(
            worker->GetTaskCount(), 
            std::bind(&UsdImagingDelegate::_Worker::UpdateVariability, 
                      worker, std::placeholders::_1, std::placeholders::_2));
    }
    worker->EnableValueCacheMutations();
}

void 
UsdImagingDelegate::Populate(std::vector<UsdImagingDelegate*> const& delegates,
                         UsdPrimVector const& rootPrims,
                         std::vector<SdfPathVector> const& excludedPrimPaths,
                         std::vector<SdfPathVector> const& invisedPrimPaths)
{
    if (!(delegates.size() == rootPrims.size()            && 
             delegates.size() == excludedPrimPaths.size() && 
             delegates.size() == invisedPrimPaths.size())) {
        TF_CODING_ERROR("Mismatched parameters");
        return;
    }

    if (delegates.empty()) {
        return;
    }

    HD_TRACE_FUNCTION();

    UsdImagingDelegate::_Worker worker;

    for (size_t i = 0; i < delegates.size(); ++i) {
        if (!delegates[i]->_CanPopulate(rootPrims[i]))
            continue;

        delegates[i]->_SetStateForPopulation(rootPrims[i], 
            excludedPrimPaths[i], invisedPrimPaths[i]);

        UsdImagingIndexProxy indexProxy(delegates[i], &worker);
        indexProxy.Repopulate(rootPrims[i].GetPath());

        delegates[i]->_Populate(&indexProxy);
    }

    _ExecuteWorkForVariabilityUpdate(&worker);

}

void
UsdImagingDelegate::_ExecuteWorkForTimeUpdate(_Worker* worker)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    worker->DisableValueCacheMutations();
    {
        // Release the GIL to ensure that threaded work won't deadlock if
        // they also need the GIL.
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        WorkParallelForN(
            worker->GetTaskCount(), 
            std::bind(&UsdImagingDelegate::_Worker::UpdateForTime, 
                      worker, std::placeholders::_1, std::placeholders::_2));
    }
    worker->EnableValueCacheMutations();
}

void
UsdImagingDelegate::SetTime(UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX: Many clients rely on SetTime(currentTime) to apply pending
    // scene edits. If we fix them to call ApplyPendingUpdates(), we can
    // remove this.
    ApplyPendingUpdates();

    // Early out if the time code is the same.
    if (_time == time) {
        return;
    }

    TF_DEBUG(USDIMAGING_UPDATES).Msg("[Update] Update for time (%f)\n",
        time.GetValue());

    _time = time;
    _xformCache.SetTime(_time);
    _visCache.SetTime(_time);
    // No need to set time on the look binding cache here, since we know we're
    // only querying relationships.

    UsdImagingIndexProxy indexProxy(this, nullptr);

    // Mark varying attributes as dirty and build a work queue for threads to
    // populate caches for the new time.
    TF_FOR_ALL(it, _primInfoMap) {
        const SdfPath &usdPath = it->first;
        _PrimInfo &primInfo    = it->second;

        if (primInfo.timeVaryingBits != HdChangeTracker::Clean) {
            primInfo.adapter->MarkDirty(primInfo.usdPrim,
                                        usdPath,
                                        primInfo.timeVaryingBits,
                                        &indexProxy);
        }
    }
}

void 
UsdImagingDelegate::SetTimes(const std::vector<UsdImagingDelegate*>& delegates,
                             const std::vector<UsdTimeCode>& times)
{
    if (delegates.size() != times.size()) {
        TF_CODING_ERROR("Mismatched parameters");
        return;
    }

    if (delegates.empty()) {
        return;
    }

    // Collect work from the batch of delegates into a single worker.
    // This has to be done single-threaded due to potential mutations
    // to the render index that is shared among these delegates.
    for (size_t i = 0; i < delegates.size(); ++i) {
        delegates[i]->SetTime(times[i]);
    }
}

UsdTimeCode
UsdImagingDelegate::GetTimeWithOffset(float offset) const
{
    return _time.IsNumeric() ? UsdTimeCode(_time.GetValue() + offset) : _time;
}

// -------------------------------------------------------------------------- //
// Change Processing 
// -------------------------------------------------------------------------- //

void
UsdImagingDelegate::ApplyPendingUpdates()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Early out if there are no updates.
    if (_pathsToResync.empty() && _pathsToUpdate.empty()) {
        return;
    }

    TF_DEBUG(USDIMAGING_UPDATES).Msg("[Update] Update for scene edits\n");

    // Need to invalidate all caches if any stage objects have changed. This
    // invalidation is overly conservative, but correct.
    _xformCache.Clear();
    _materialBindingImplData.ClearCaches();
    _materialBindingCache.Clear();
    _visCache.Clear();
    _purposeCache.Clear();
    _drawModeCache.Clear();

    UsdImagingDelegate::_Worker worker;
    UsdImagingIndexProxy indexProxy(this, &worker);

    if (!_pathsToResync.empty()) {
        // Make a copy of pathsToResync, but uniqued with a prefix-check, which
        // removes all elements that are prefixed by other elements.
        SdfPathVector pathsToResync;
        pathsToResync.reserve(_pathsToResync.size());
        std::sort(_pathsToResync.begin(), _pathsToResync.end());
        std::unique_copy(_pathsToResync.begin(), _pathsToResync.end(),
                         std::back_inserter(pathsToResync),
                         [](SdfPath const &l, SdfPath const &r) {
                             return r.HasPrefix(l);
                         });
        _pathsToResync.clear();

        TF_FOR_ALL(pathIt, pathsToResync) {
            SdfPath path = *pathIt;
            if (path.IsPropertyPath()) {
                _RefreshObject(path, TfTokenVector(), &indexProxy);
            } else if (path.IsTargetPath()) {
                // TargetPaths are their own path type, when they change, resync
                // the relationship at which they're rooted; i.e. per-target
                // invalidation is not supported.
                _RefreshObject(path.GetParentPath(), TfTokenVector(),
                               &indexProxy);
            } else if (path.IsAbsoluteRootOrPrimPath()) {
                _ResyncPrim(path, &indexProxy);
            } else {
                TF_WARN("Unexpected path type to resync: <%s>",
                        path.GetText());
            }
        }
    }

    // Remove any objects that were queued for removal to ensure RefreshObject
    // doesn't apply changes to removed objects.
    indexProxy._ProcessRemovals();

    if (!_pathsToUpdate.empty()) {
        _PathsToUpdateMap pathsToUpdate;
        std::swap(pathsToUpdate, _pathsToUpdate);
        TF_FOR_ALL(pathIt, pathsToUpdate) {
            const SdfPath& path = pathIt->first;
            const TfTokenVector& changedPrimInfoFields = pathIt->second;

            if (path.IsPropertyPath() || path.IsAbsoluteRootOrPrimPath()){
                // Note that changedPrimInfoFields will be empty if the
                // path is a property path.
                _RefreshObject(path, changedPrimInfoFields, &indexProxy);

                // If any objects were removed as a result of the refresh (if it
                // internally decided to resync), they must be ejected now,
                // before the next call to _RefereshObject.
                indexProxy._ProcessRemovals();
            } else {
                TF_RUNTIME_ERROR("Unexpected path type to update: <%s>",
                        path.GetText());
            }
        }
    }

    // If any changes called Repopulate() on the indexProxy, we need to
    // repopulate them before any updates. If the list is empty, _Populate is a
    // no-op.
    _Populate(&indexProxy);
    _ExecuteWorkForVariabilityUpdate(&worker);
}

void 
UsdImagingDelegate::_OnObjectsChanged(UsdNotice::ObjectsChanged const& notice,
                                      UsdStageWeakPtr const& sender)
{
    if (!sender || !TF_VERIFY(sender == _stage))
        return;
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Objects Changed] Notice recieved "
                            "from stage with root layer @%s@\n",
                        sender->GetRootLayer()->GetIdentifier().c_str());

    using PathRange = UsdNotice::ObjectsChanged::PathRange;

    // These paths are subtree-roots representing entire subtrees that may have
    // changed. In this case, we must dump all cached data below these points
    // and repopulate those trees.
    const PathRange pathsToResync = notice.GetResyncedPaths();
    _pathsToResync.insert(_pathsToResync.end(), 
                          pathsToResync.begin(), pathsToResync.end());
    
    // These paths represent objects which have been modified in a 
    // non-structural way, for example setting a value. These paths may be paths
    // to prims or properties, in which case we should sparsely invalidate
    // cached data associated with the path.
    const PathRange pathsToUpdate = notice.GetChangedInfoOnlyPaths();
    for (PathRange::const_iterator it = pathsToUpdate.begin(); 
         it != pathsToUpdate.end(); ++it) {
        if (it->IsAbsoluteRootOrPrimPath()) {
            // Ignore all changes to prims that have not changed any field
            // values, since these changes cannot affect any composed values 
            // consumed by the adapters.
            const TfTokenVector changedFields = it.GetChangedFields();
            if (!changedFields.empty()) {
                TfTokenVector& changedPrimInfoFields = _pathsToUpdate[*it];
                changedPrimInfoFields.insert(
                    changedPrimInfoFields.end(),
                    changedFields.begin(), changedFields.end());
            }
        } else if (it->IsPropertyPath()) {
            _pathsToUpdate.emplace(*it, TfTokenVector());
        }
    }

    if (TfDebug::IsEnabled(USDIMAGING_CHANGES)) {
        TF_FOR_ALL(it, pathsToResync) {
            TF_DEBUG(USDIMAGING_CHANGES).Msg(" - Resync queued: %s\n",
                        it->GetText());
        }
        TF_FOR_ALL(it, pathsToUpdate) {
            TF_DEBUG(USDIMAGING_CHANGES).Msg(" - Refresh queued: %s\n",
                        it->GetText());
        }
    }
}

void 
UsdImagingDelegate::_ResyncPrim(SdfPath const& rootPath, 
                                UsdImagingIndexProxy* proxy,
                                bool repopulateFromRoot) 
{
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: <%s>\n",
            rootPath.GetText());

    // Note: it is only appropriate to call adapter in the primInfo in the code
    // below, since we want the adapter that was registered to handle change
    // processing, which may be different from the default adapter registered
    // for the UsdPrim type.

    // The following code is fairly subtle, it attempts to handle the following
    // scenarios:
    //
    //  * A prim was created/modified/removed
    //  * A prim was created/modified/removed below an existing prim that may
    //    have pruned children during initial population.
    //  * A prim was created/modified/removed below a prototype-root of an
    //    instancer
    //
    //  * This is happening as a result of a resync notice
    //  * This is happening as a result of a refresh notice that then decided to
    //    resync
    //

    // If an instancer is detected as an ancestor, we track the path. See notes
    // below where instancerPath is used.
    SdfPath instancerPath;

    //
    // Detect if the prim that is being resynced is in a sub-tree that was
    // pruned by an ancestral prim adapter.
    //
    UsdPrim prim = _stage->GetPrimAtPath(rootPath);
    if (!prim) {
        TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: "
                   "Root prim <%s> no longer exists\n",
                        rootPath.GetText());
    } else {
        if (prim.IsInMaster()) {
            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: "
                   "Skipping root prim <%s>, is in master\n",
                        rootPath.GetText());
            return;
        }
        // First, search parents for pruning opinions.
        bool prunedByParent = false;
        for (UsdPrim curPrim = prim; curPrim; curPrim = curPrim.GetParent()) {
            // XXX: We're baking in the requirement that all instancer
            // prototypes must be nested below them here, would be nice to not
            // do this, however we would need to track yet another bit of state.
            //
            // See additional notes around instancerPath use below for why this
            // is needed.
            if (_instancerPrimPaths.find(curPrim.GetPath()) 
                    != _instancerPrimPaths.end()) {
                instancerPath = curPrim.GetPath();
                prunedByParent = true;
            }

            // Check for type-based pruning opinions.
            // XXX: If the path-to-resync is a geom subset, that skips regular
            // type-based pruning. It would be nice to not have to special-case
            // this...
            if (UsdImagingPrimAdapter::ShouldCullSubtree(curPrim) &&
                !(curPrim == prim && prim.IsA<UsdGeomSubset>())) {
                TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: "
                    "Discovery of new prims below <%s> pruned by "
                    "prim type of <%s>: (%s)\n",
                    rootPath.GetText(),
                    curPrim.GetPath().GetText(),
                    curPrim.GetTypeName().GetText());
                prunedByParent = true;
                break;
            }

            _PrimInfo *primInfo = GetPrimInfo(curPrim.GetPath());

            // If we've already seen one of the parent prims and the associated
            // adapter desires the children to be pruned, we shouldn't
            // repopulate this root.
            if (primInfo != nullptr &&
                TF_VERIFY(primInfo->adapter, "%s\n",
                          curPrim.GetPath().GetText())) {
                _AdapterSharedPtr &adapter = primInfo->adapter;

                if (adapter->ShouldCullChildren()) {
                    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: "
                           "Discovery of new prims below <%s> pruned by "
                           "adapter of <%s>\n",
                                rootPath.GetText(),
                                curPrim.GetPath().GetText());
                    prunedByParent = true;
                    break;
                }
            }
        }
        
        // If this path was not pruned by a parent, discover all prims that were
        // newly added with this change.
        if (!prunedByParent) {
            UsdPrimRange range(prim);

            for (auto iter = range.begin(); iter != range.end(); ++iter) {
                if (prunedByParent)
                    break;

                const UsdPrim &usdPrim = *iter;
                _PrimInfo *primInfo = GetPrimInfo(usdPrim.GetPath());

                // Special case for adding UsdGeomSubset prims (which do not
                // get an adapter); resync the containing mesh.
                if (iter->IsA<UsdGeomSubset>()) {
                    UsdPrim parentPrim = iter->GetParent();
                    _PrimInfo *parentPrimInfo =
                        GetPrimInfo(parentPrim.GetPath());
                    if (parentPrimInfo != nullptr &&
                        TF_VERIFY(parentPrimInfo->adapter, "%s\n",
                                  parentPrim.GetPath().GetText())) {
                        TF_DEBUG(USDIMAGING_CHANGES)
                            .Msg("[Resync Prim]: Resyncing parent <%s> on "
                                 "behalf of subset <%s>\n",
                                 parentPrim.GetPath().GetText(),
                                 iter->GetPath().GetText());
                        parentPrimInfo->adapter->ProcessPrimResync(
                            parentPrim.GetPath(), proxy);
                    }
                    iter.PruneChildren();
                    continue;
                }

                // Check if this prim (& subtree) should be pruned based on
                // prim type.
                if (UsdImagingPrimAdapter::ShouldCullSubtree(usdPrim)) {
                    iter.PruneChildren();
                    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: "
                        "[Re]population of subtree <%s> pruned by prim type "
                        "(%s)\n",
                        usdPrim.GetPath().GetText(),
                        usdPrim.GetTypeName().GetText());
                    continue;
                }

                // If this prim in the tree wants to prune children, we must
                // respect that and ignore any additions under this descendant.
                if (primInfo != nullptr  &&
                    TF_VERIFY(primInfo->adapter,"%s\n", 
                              usdPrim.GetPath().GetText())) {
                    _AdapterSharedPtr &adapter = primInfo->adapter;

                    if (adapter->ShouldCullChildren()) {
                        iter.PruneChildren();
                        TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: "
                                "[Re]population of children of <%s> pruned by "
                                "adapter\n",
                                usdPrim.GetPath().GetText());
                    }
                    continue;
                }

                // The prim wasn't in the _primInfoMap, this could happen
                // because the prim just came into existence.
                _AdapterSharedPtr adapter = _AdapterLookup(*iter);
                if (!adapter) {
                    // This prim has no prim adapter, continue traversing
                    // descendants.    
                    continue;
                }

                // This prim has an adapter, but wasn't in our adapter map, so
                // it just came into existence and wasn't pruned by any
                // ancestors; we can now safely repopulate from this root and
                // prune children (repopulation is recursive).
                TF_DEBUG(USDIMAGING_CHANGES).Msg(
                                            "[Resync Prim]: Populating <%s>\n",
                                            iter->GetPath().GetText());
                proxy->Repopulate(iter->GetPath());
                iter.PruneChildren();
            }
        }
    }

    // Ensure we resync all prims that may have previously existed, but were
    // removed with this change.
    SdfPathVector affectedPrims;
    HdPrimGather gather;

    gather.Subtree(_usdIds.GetIds(), rootPath, &affectedPrims);

    if (affectedPrims.empty()) {
        // When we have no affected prims and all new prims were culled, the
        // instancer may still need to be notified that the child was resync'd,
        // in the event that a new prim came into existence under the root of an
        // existing prototype.
        //
        // TODO: proposes we expose an API on the adapter to query if the
        // path is of interest, which would allow the instancer (any ancestral
        // adapter) to hook in and get the event. We should do this in a future
        // change.
        if (instancerPath.IsEmpty()) {
            // We had no affected paths, which means the prim wasn't populated,
            // skip population below.
            return;
        } else {
            TF_DEBUG(USDIMAGING_CHANGES).Msg(
                    "  - affected instancer prim: <%s>\n",
                    instancerPath.GetText());

            _PrimInfo *primInfo = GetPrimInfo(instancerPath);
            if (!TF_VERIFY(primInfo, "%s\n", instancerPath.GetText()) ||
                !TF_VERIFY(primInfo->adapter, "%s\n",
                           instancerPath.GetText())) {
                return;
            }

            primInfo->adapter->ProcessPrimResync(instancerPath, proxy);
            return;
        }
    }

    // Apply changes.
    TF_FOR_ALL(primIt, affectedPrims) {
        SdfPath const& usdPath = *primIt;
        _PrimInfo *primInfo = GetPrimInfo(usdPath);

        TF_DEBUG(USDIMAGING_CHANGES).Msg("  - affected prim: <%s>\n",
                usdPath.GetText());

        // We discovered these paths using the _primInfoMap above, this
        // method should never return a null adapter here.
        if (!TF_VERIFY(primInfo, "%s\n", usdPath.GetText()) ||
            !TF_VERIFY(primInfo->adapter, "%s\n", usdPath.GetText()))
            return;

        // PrimResync will:
        //  * Remove the rprim from the index, if it needs to be re-built
        //  * Schedule the prim to be repopulated
        // Note: primInfo may be invalid after this call
        if (repopulateFromRoot) {
            primInfo->adapter->ProcessPrimRemoval(usdPath, proxy);
        } else {
            primInfo->adapter->ProcessPrimResync(usdPath, proxy);
        }
    }

    if (repopulateFromRoot) {
        proxy->Repopulate(rootPath);
    }
}

void 
UsdImagingDelegate::_RefreshObject(SdfPath const& usdPath, 
                                   TfTokenVector const& changedInfoFields,
                                   UsdImagingIndexProxy* proxy) 
{
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Refresh Object]: %s %s\n",
            usdPath.GetText(), TfStringify(changedInfoFields).c_str());

    SdfPathVector affectedPrims;

    if (usdPath.IsAbsoluteRootOrPrimPath()) {
        if (!GetPrimInfo(usdPath)) {
            return;
        }
        affectedPrims.push_back(usdPath);
    } else if (usdPath.IsPropertyPath()) {
        SdfPath const& usdPrimPath = usdPath.GetPrimPath();
        TfToken const& attrName = usdPath.GetNameToken();

        // If either model:drawMode or model:applyDrawMode changes, we need to
        // repopulate the whole subtree starting at the owning prim.
        // If the binding has changed we need to make sure we are resyncing
        // the prim so the material gets an opportunity to populate itself.
        // This is very conservative but it is correct.
        if (attrName == UsdGeomTokens->modelDrawMode ||
            attrName == UsdGeomTokens->modelApplyDrawMode ||
            TfStringStartsWith(attrName.GetString(),
                UsdShadeTokens->materialBinding.GetString())) {
            _ResyncPrim(usdPath.GetPrimPath(), proxy, true);
            return;
        }

        // If we're sync'ing a non-inherited property on a parent prim, we 
        // should fall through this function without updating anything. 
        // The following if-statement should ensure this.
        
        // XXX: We must always scan for prefixed children, due to rprim fan-out 
        // from plugins (such as the PointInstancer).
        if (attrName == UsdGeomTokens->visibility
            || attrName == UsdGeomTokens->purpose
            || UsdGeomXformable::IsTransformationAffectedByAttrNamed(attrName))
        {
            // Because these are inherited attributes, we must update all
            // children.
            HdPrimGather gather;
            gather.Subtree(_usdIds.GetIds(), usdPrimPath, &affectedPrims);
        } else if (TfStringStartsWith(attrName.GetString(),
                                      UsdTokens->collection.GetString())) {
            // XXX Performance: Collections used for material bindings
            // can refer to prims at arbitrary locations in the scene.
            // Accordingly, we conservatively invalidate everything.
            // If we preserved _materialBindingCache rather than
            // blowing it in _ProcessChangesForTimeUpdate(), we could
            // potentially use it to analyze affected paths and
            // perform more narrow invalidation.
            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Refresh Object]: "
                "Collection property <%s> modified; conservatively "
                "invalidating all prims to ensure that we discover "
                "material binding changes.", usdPath.GetText());
            HdPrimGather gather;
            gather.Subtree(_usdIds.GetIds(), SdfPath::AbsoluteRootPath(),
                           &affectedPrims);
        } else {
            // Only include non-inherited properties for prims that we are
            // explicitly tracking in the render index.
            if (!GetPrimInfo(usdPrimPath)) {
                return;
            }
            affectedPrims.push_back(usdPrimPath);
        }
    }

    // PERFORMANCE: We could execute this in parallel, for large numbers of
    // prims.
    TF_FOR_ALL(affectedPrimPathIt, affectedPrims) {
        SdfPath const& affectedPrimPath = *affectedPrimPathIt;

        _PrimInfo *primInfo = GetPrimInfo(affectedPrimPath);

        // Due to the ResyncPrim condition when AllDirty is returned below, we
        // may or may not find an associated primInfo for every prim in
        // affectedPrims. If we find no primInfo, the prim that was previously
        // affected by this refresh no longer exists and can be ignored.
        //
        // It is also possible that we find a primInfo, but the prim it refers
        // to has been deleted from the stage and is no longer valid. Such a
        // prim may end up in the affectedPrims during the refresh of a
        // collection that previously pointed directly to a prim that has
        // been deleted. The primInfo for this prim will still be in the index
        // because we haven't had the index process removals yet.
        if (primInfo != nullptr &&
            primInfo->usdPrim.IsValid() &&
            TF_VERIFY(primInfo->adapter, "%s", affectedPrimPath.GetText())) {
            _AdapterSharedPtr &adapter = primInfo->adapter;

            // For the dirty bits that we've been told changed, go re-discover
            // variability and stage the associated data.
            HdDirtyBits dirtyBits = HdChangeTracker::Clean;
            if (usdPath.IsAbsoluteRootOrPrimPath()) {
                dirtyBits = adapter->ProcessPrimChange(
                    primInfo->usdPrim, affectedPrimPath, changedInfoFields);
            } else if (usdPath.IsPropertyPath()) {
                dirtyBits = adapter->ProcessPropertyChange(
                    primInfo->usdPrim, affectedPrimPath, usdPath.GetNameToken());
            } else {
                TF_VERIFY(false, "Unexpected path: <%s>", usdPath.GetText());
            }

            if (dirtyBits == HdChangeTracker::Clean) {
                // Do nothing
            } else if (dirtyBits != HdChangeTracker::AllDirty) {
                // Update Variability
                adapter->TrackVariabilityPrep(primInfo->usdPrim, 
                                              affectedPrimPath);
                adapter->TrackVariability(primInfo->usdPrim,
                                          affectedPrimPath,
                                          &primInfo->timeVaryingBits);

                // Propagate the dirty bits back out to the change tracker.
                HdDirtyBits combinedBits =
                    dirtyBits | primInfo->timeVaryingBits;
                if (combinedBits != HdChangeTracker::Clean) {
                    adapter->MarkDirty(primInfo->usdPrim, affectedPrimPath,
                                       combinedBits, proxy);
                }
            } else {
                _ResyncPrim(affectedPrimPath, proxy);
            }
        }
    }
}

// -------------------------------------------------------------------------- //
// Data Collection
// -------------------------------------------------------------------------- //
void
UsdImagingDelegate::_UpdateSingleValue(SdfPath const& usdPath, int requestBits)
{
    // XXX: potential race condition? UpdateSingleValue may be called from
    // multiple thread on a same path. we should probably need a guard here,
    // or in adapter
    _PrimInfo *primInfo = GetPrimInfo(usdPath);
    if (TF_VERIFY(primInfo, "%s\n", usdPath.GetText()) &&
        TF_VERIFY(primInfo->adapter, "%s\n", usdPath.GetText())) {
        _AdapterSharedPtr &adapter = primInfo->adapter;
        adapter->UpdateForTimePrep(primInfo->usdPrim, usdPath, _time, requestBits);
        adapter->UpdateForTime(primInfo->usdPrim, usdPath, _time, requestBits);
    }
}

void
UsdImagingDelegate::ClearPickabilityMap()
{
    _pickablesMap.clear();
}

void 
UsdImagingDelegate::SetPickability(SdfPath const& path, bool pickable)
{
    _pickablesMap[GetPathForIndex(path)] = pickable;
}

PickabilityMap
UsdImagingDelegate::GetPickabilityMap() const
{
    return _pickablesMap; 
}

void
UsdImagingDelegate::SetDisplayGuides(bool displayGuides) 
{
    _displayGuides = displayGuides;
    
    // Geometry that was assigned to a command buffer to be rendered might
    // now be hidden or the contrary, so we need to rebuild the collections.
    GetRenderIndex().GetChangeTracker().MarkAllCollectionsDirty();
}

void
UsdImagingDelegate::SetUsdDrawModesEnabled(bool enableUsdDrawModes)
{
    if (_enableUsdDrawModes != enableUsdDrawModes) {
        if (_primInfoMap.size() > 0) {
            TF_CODING_ERROR("SetUsdDrawModesEnabled() was called after "
                            "population; this is currently unsupported...");
        } else {
            _enableUsdDrawModes = enableUsdDrawModes;
        }
    }
}


void
UsdImagingDelegate::SetSceneMaterialsEnabled(bool enable)
{
    if (_sceneMaterialsEnabled != enable)
    {
        _sceneMaterialsEnabled = enable;

        UsdImagingIndexProxy indexProxy(this, nullptr);

        // Mark dirty.
        TF_FOR_ALL(it, _primInfoMap) {
            const SdfPath &usdPath = it->first;
            _PrimInfo &primInfo = it->second;
            if (TF_VERIFY(primInfo.adapter, "%s", usdPath.GetText())) {
                primInfo.adapter->MarkMaterialDirty(primInfo.usdPrim,
                                                    usdPath,
                                                    &indexProxy);
            }
        }
    }
}


/*virtual*/
TfToken
UsdImagingDelegate::GetRenderTag(SdfPath const& id)
{
    SdfPath usdPath = GetPathForUsd(id);

    // Check the purpose of the rpim
    TfToken purpose = UsdGeomTokens->default_;
    TF_VERIFY(_valueCache.FindPurpose(usdPath, &purpose), "%s", 
              usdPath.GetText());

    // If it is a property path then let's resolve it.
    // parent opinion wins if it is not default
    if (usdPath.IsPropertyPath()) {
        SdfPath usdPathParent = usdPath.GetPrimPath();
        TfToken purposeParent = UsdGeomTokens->default_;
        TF_VERIFY(_valueCache.FindPurpose(usdPathParent, &purposeParent), "%s", 
                  usdPathParent.GetText());
        
        if (purposeParent != UsdGeomTokens->default_) {
            purpose = purposeParent;
        }
    }

    if (purpose == UsdGeomTokens->default_) {
        // Simple mapping so all render tags in multiple delegates match
        purpose = HdTokens->geometry;    
    } else if (purpose == UsdGeomTokens->guide && !_displayGuides) {
        // When guides are disabled on the delegate we move the
        // guide prims to the hidden command buffer
        purpose = HdTokens->hidden;
    }

    TF_DEBUG(USDIMAGING_COLLECTIONS).Msg("GetRenderTag %s -> %s \n",
                                usdPath.GetText(),
                                purpose.GetText());
    return purpose;
}

/*virtual*/
HdBasisCurvesTopology 
UsdImagingDelegate::GetBasisCurvesTopology(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    SdfPath usdPath = GetPathForUsd(id);
    HdBasisCurvesTopology topology;
    VtValue tmp;

    if (_valueCache.ExtractTopology(usdPath, &tmp)) {
        return tmp.UncheckedGet<HdBasisCurvesTopology>();
    }
    _UpdateSingleValue(usdPath, HdChangeTracker::DirtyTopology);
    if (TF_VERIFY(_valueCache.ExtractTopology(usdPath, &tmp)))
        return tmp.UncheckedGet<HdBasisCurvesTopology>();

    return topology;
}

/*virtual*/
HdMeshTopology 
UsdImagingDelegate::GetMeshTopology(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    SdfPath usdPath = GetPathForUsd(id);
    HdMeshTopology topology;
    VtValue tmp;

    if (_valueCache.ExtractTopology(usdPath, &tmp)) {
        return tmp.UncheckedGet<HdMeshTopology>();
    }
    _UpdateSingleValue(usdPath, HdChangeTracker::DirtyTopology);
    if (TF_VERIFY(_valueCache.ExtractTopology(usdPath, &tmp)))
        return tmp.UncheckedGet<HdMeshTopology>();
    
    return topology;
}

/*virtual*/
UsdImagingDelegate::SubdivTags 
UsdImagingDelegate::GetSubdivTags(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    SdfPath usdPath = GetPathForUsd(id);
    SubdivTags tags;

    if (_valueCache.ExtractSubdivTags(usdPath, &tags)) {
        return tags;
    }
    _UpdateSingleValue(usdPath, HdChangeTracker::DirtySubdivTags);
    if (TF_VERIFY(_valueCache.ExtractSubdivTags(usdPath, &tags))) {
        return tags;
    }

    return tags;
}

/*virtual*/
GfRange3d 
UsdImagingDelegate::GetExtent(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    SdfPath usdPath = GetPathForUsd(id);
    GfRange3d extent;
    if (_valueCache.ExtractExtent(usdPath, &extent)) {
        return extent;
    }
    // Slow path, we should not hit this.
    TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow extent fetch for %s\n", 
                               id.GetText());
    _UpdateSingleValue(usdPath, HdChangeTracker::DirtyExtent);
    TF_VERIFY(_valueCache.ExtractExtent(usdPath, &extent));
    return extent;
}

/*virtual*/ 
bool 
UsdImagingDelegate::GetDoubleSided(SdfPath const& id)
{
    bool doubleSided = false;
    SdfPath usdPath = GetPathForUsd(id);
    if (_valueCache.ExtractDoubleSided(usdPath, &doubleSided))
        return doubleSided; 

    _UpdateSingleValue(usdPath, HdChangeTracker::DirtyDoubleSided);
    TF_VERIFY(_valueCache.ExtractDoubleSided(usdPath, &doubleSided));
    return doubleSided;
}

/*virtual*/
HdCullStyle
UsdImagingDelegate::GetCullStyle(SdfPath const &id)
{
    // XXX: Cull style works a bit weirdly. Most adapters aren't
    // expected to use cullstyle, so: if it's there, use it, but otherwise
    // just use the fallback value.
    //
    // This way, prims that don't care about it don't need to pay the price
    // of populating it in the value cache.
    HdCullStyle cullStyle = HdCullStyleDontCare;
    SdfPath usdPath = GetPathForUsd(id);
    if (_valueCache.ExtractCullStyle(usdPath, &cullStyle)) {
        return cullStyle;
    }

    return _cullStyleFallback;
}

/*virtual*/ 
HdDisplayStyle 
UsdImagingDelegate::GetDisplayStyle(SdfPath const& id) { 
    SdfPath usdPath = GetPathForUsd(id);
    int level = 0;
    if (TfMapLookup(_refineLevelMap, usdPath, &level))
        return HdDisplayStyle(level);
    return HdDisplayStyle(GetRefineLevelFallback());
}

void
UsdImagingDelegate::SetRefineLevelFallback(int level) 
{ 
    if (level == _refineLevelFallback || !_ValidateRefineLevel(level))
        return;
    _refineLevelFallback = level; 

    UsdImagingIndexProxy indexProxy(this, nullptr);

    TF_FOR_ALL(it, _primInfoMap) {
        const SdfPath &usdPath = it->first;
        _PrimInfo &primInfo = it->second;

        // Dont mark prims with explicit refine levels as dirty.
        if (_refineLevelMap.find(usdPath) == _refineLevelMap.end()) {
            if (TF_VERIFY(primInfo.adapter, "%s", usdPath.GetText())) {
                primInfo.adapter->MarkRefineLevelDirty(primInfo.usdPrim,
                                                       usdPath,
                                                       &indexProxy);
            }
        }
    }
}

void
UsdImagingDelegate::SetRefineLevel(SdfPath const& usdPath, int level) 
{ 
    if (!_ValidateRefineLevel(level))
        return;
    
    _RefineLevelMap::iterator it = _refineLevelMap.find(usdPath);
    if (it != _refineLevelMap.end()) {
        if (it->second == level)
            return;
        it->second = level;
    } else {
        _refineLevelMap[usdPath] = level;
        // Avoid triggering changes if the new level is the same as the
        // fallback.
        if (level == _refineLevelFallback)
            return;
    }

    UsdImagingIndexProxy indexProxy(this, nullptr);

    _PrimInfo *primInfo = GetPrimInfo(usdPath);
    if (TF_VERIFY(primInfo, "%s", usdPath.GetText()) &&
        TF_VERIFY(primInfo->adapter, "%s", usdPath.GetText())) {
        primInfo->adapter->MarkRefineLevelDirty(primInfo->usdPrim,
                                                usdPath,
                                                &indexProxy);
    }
}

void
UsdImagingDelegate::ClearRefineLevel(SdfPath const& usdPath) 
{ 
    _RefineLevelMap::iterator it = _refineLevelMap.find(usdPath);
    if (it == _refineLevelMap.end())
        return;

    int oldLevel = it->second;
    _refineLevelMap.erase(it);
    if (oldLevel != _refineLevelFallback) {
        UsdImagingIndexProxy indexProxy(this, nullptr);

        _PrimInfo *primInfo = GetPrimInfo(usdPath);
        if (TF_VERIFY(primInfo, "%s", usdPath.GetText()) &&
            TF_VERIFY(primInfo->adapter, "%s", usdPath.GetText())) {
            primInfo->adapter->MarkRefineLevelDirty(primInfo->usdPrim,
                                                    usdPath,
                                                    &indexProxy);
        }
    }
}


bool
UsdImagingDelegate::IsRefined(SdfPath const& usdPath) const
{
    _RefineLevelMap::const_iterator it = _refineLevelMap.find(usdPath);
    if (it == _refineLevelMap.end()) {
        return (GetRefineLevelFallback() > 0);
    }

    return (it->second > 0);
}


void
UsdImagingDelegate::SetReprFallback(HdReprSelector const &repr)
{
    HD_TRACE_FUNCTION();

    if (_reprFallback == repr) {
        return;
    }
    _reprFallback = repr;

    UsdImagingIndexProxy indexProxy(this, nullptr);

    TF_FOR_ALL(it, _primInfoMap) {
        const SdfPath &usdPath = it->first;
        _PrimInfo &primInfo = it->second;

        if (TF_VERIFY(primInfo.adapter, "%s", usdPath.GetText())) {
            primInfo.adapter->MarkReprDirty(primInfo.usdPrim,
                                            usdPath,
                                            &indexProxy);
        }
    }

    // XXX: currently we need to make collection dirty so that
    // HdRenderPass::_PrepareCommandBuffer gathers new drawitem from scratch.
    GetRenderIndex().GetChangeTracker().MarkAllCollectionsDirty();
}

void
UsdImagingDelegate::SetCullStyleFallback(HdCullStyle cullStyle)
{
    HD_TRACE_FUNCTION();

    if (_cullStyleFallback == cullStyle) {
        return;
    }
    _cullStyleFallback = cullStyle;

    UsdImagingIndexProxy indexProxy(this, nullptr);

    TF_FOR_ALL(it, _primInfoMap) {
        const SdfPath &usdPath = it->first;
        _PrimInfo &primInfo = it->second;

        if (TF_VERIFY(primInfo.adapter, "%s", usdPath.GetText())) {
            primInfo.adapter->MarkCullStyleDirty(primInfo.usdPrim,
                                                 usdPath,
                                                 &indexProxy);
        }
    }

    // XXX: currently we need to make collection dirty so that
    // HdRenderPass::_PrepareCommandBuffer gathers new drawitem from scratch.
    GetRenderIndex().GetChangeTracker().MarkAllCollectionsDirty();
}

void
UsdImagingDelegate::SetRootTransform(GfMatrix4d const& xf)
{
    HD_TRACE_FUNCTION();

    // TODO: do IsClose check.
    if (xf == _rootXf)
        return;

    _rootXf = xf;

    UsdImagingIndexProxy indexProxy(this, nullptr);

    // Mark dirty.
    TF_FOR_ALL(it, _primInfoMap) {
        const SdfPath &usdPath = it->first;
        _PrimInfo &primInfo = it->second;
        if (TF_VERIFY(primInfo.adapter, "%s", usdPath.GetText())) {
            primInfo.adapter->MarkTransformDirty(primInfo.usdPrim,
                                                 usdPath,
                                                 &indexProxy);
        }
    }
}

void 
UsdImagingDelegate::SetInvisedPrimPaths(SdfPathVector const &invisedPaths)
{
    HD_TRACE_FUNCTION();

    if (_invisedPrimPaths == invisedPaths) {
        return;
    }

    SdfPathSet sortedNewInvisedPaths(invisedPaths.begin(), invisedPaths.end());
    SdfPathSet sortedExistingInvisedPaths(_invisedPrimPaths.begin(), 
                                          _invisedPrimPaths.end());
    SdfPathVector changingInvisPaths;
    std::set_symmetric_difference(sortedNewInvisedPaths.begin(), 
                                  sortedNewInvisedPaths.end(),
                                  sortedExistingInvisedPaths.begin(),
                                  sortedExistingInvisedPaths.end(),
                                  std::back_inserter(changingInvisPaths));

    SdfPath::RemoveDescendentPaths(&changingInvisPaths);
    TF_FOR_ALL(changingInvisPathIt, changingInvisPaths) {
        const SdfPath &subtreeRoot = *changingInvisPathIt;
        const UsdPrim &usdPrim = _GetPrim(subtreeRoot);
        if (!usdPrim) {
            TF_CODING_ERROR("Could not find prim at path <%s>.", 
                            subtreeRoot.GetText());
            continue;
        }

        TF_DEBUG(USDIMAGING_CHANGES).Msg("[Vis/Invis Prim] <%s>\n",
                                         subtreeRoot.GetText());
        _MarkSubtreeVisibilityDirty(subtreeRoot);
    }

    _invisedPrimPaths = invisedPaths;

    // process instance visibility.
    // this call is needed because we use _RefreshObject to repopulate
    // vis-ed/invis-ed instanced prims (accumulated in _pathsToUpdate)
    ApplyPendingUpdates();
}

void
UsdImagingDelegate::_MarkSubtreeVisibilityDirty(SdfPath const &subtreeRoot)
{
    UsdImagingIndexProxy indexProxy(this, nullptr);

    HdPrimGather gather;
    SdfPathVector affectedPrims;
    gather.Subtree(_usdIds.GetIds(), subtreeRoot, &affectedPrims);

    // Propagate dirty bits to all descendents and outside dependent prims.
    //
    size_t numPrims = affectedPrims.size();
    for (size_t primNum = 0; primNum < numPrims; ++primNum) {
        const SdfPath &usdPath = affectedPrims[primNum];

        _PrimInfo *primInfo = GetPrimInfo(usdPath);
        if (primInfo == nullptr) {
            TF_CODING_ERROR("Prim in id list is not in prim info: %s",
                    usdPath.GetText());
            continue;
        }
        if (!TF_VERIFY(primInfo->adapter, "%s", usdPath.GetText())) {
            continue;
        }

        _AdapterSharedPtr const &adapter = primInfo->adapter;

        SdfPath instancer = adapter->GetInstancer(usdPath);
        if (!instancer.IsEmpty()) {
            // XXX: workaround for per-instance visibility in nested case.
            // testPxUsdGeomGLPopOut/test_*_5, test_*_6
            _pathsToResync.push_back(subtreeRoot);
            return;
        } else if (_instancerPrimPaths.find(usdPath) != _instancerPrimPaths.end()) {
            // XXX: workaround for per-instance visibility in nested case.
            // testPxUsdGeomGLPopOut/test_*_5, test_*_6
            _pathsToResync.push_back(subtreeRoot);
            return;
        } else {
            adapter->MarkVisibilityDirty(primInfo->usdPrim,
                                         usdPath,
                                         &indexProxy);
        }
    }
}

void 
UsdImagingDelegate::SetRigidXformOverrides(
    RigidXformOverridesMap const &rigidXformOverrides)
{
    HD_TRACE_FUNCTION();

    if (_rigidXformOverrides == rigidXformOverrides) {
        return;
    }

    TfHashMap<UsdPrim, GfMatrix4d, boost::hash<UsdPrim> > overridesToUpdate;

    // Compute the set of overrides to update and update their values in the 
    // inherited xform cache.
    TF_FOR_ALL(it, rigidXformOverrides) {
        const SdfPath &overridePath = it->first;
        const UsdPrim &overridePrim = _GetPrim(overridePath);

        RigidXformOverridesMap::const_iterator existingIt = 
            _rigidXformOverrides.find(overridePath);

        // If the existing value matches the incoming value, then skip the 
        // update.
        if (existingIt != _rigidXformOverrides.end()) {
            if (existingIt->second != it->second) {                
                overridesToUpdate[overridePrim] = it->second;
            }

            // Erase the entry in the existing map. At the end of the loop
            // the existing overrides map should contain only the overrides 
            // to be removed.
            _rigidXformOverrides.erase(overridePath);
        } else {
            // In this case, we're adding a new override.
            overridesToUpdate[overridePrim] = it->second;
        }
    }

    // Now, _rigidXformOverrides has the set of overrides to remove. 
    std::vector<UsdPrim> overridesToRemove;
    TF_FOR_ALL(it, _rigidXformOverrides) {
        UsdPrim overridePrim = _GetPrim(it->first);
        overridesToRemove.push_back(overridePrim);
    }

    SdfPathVector dirtySubtreeRoots;
    _xformCache.UpdateValueOverrides(overridesToUpdate, overridesToRemove,
        &dirtySubtreeRoots);

    SdfPath::RemoveDescendentPaths(&dirtySubtreeRoots);

    // Mark dirty.
    TF_FOR_ALL(it, dirtySubtreeRoots) {
        const SdfPath &subtreeRoot = *it;
        const UsdPrim &usdPrim = _GetPrim(subtreeRoot);
        if (!usdPrim) {
            TF_CODING_ERROR("Could not find prim at path <%s>.", 
                subtreeRoot.GetText());
            continue;
        }

        TF_DEBUG(USDIMAGING_CHANGES).Msg("[RigidXform override] <%s>\n",
                                         subtreeRoot.GetText());

        _MarkSubtreeTransformDirty(subtreeRoot);
    }

    _rigidXformOverrides = rigidXformOverrides;
}

void
UsdImagingDelegate::_MarkSubtreeTransformDirty(SdfPath const &subtreeRoot)
{
    UsdImagingIndexProxy indexProxy(this, nullptr);

    HdPrimGather gather;
    SdfPathVector affectedPrims;
    gather.Subtree(_usdIds.GetIds(), subtreeRoot, &affectedPrims);

    // Propagate dirty bits to all descendents and outside dependent prims.
    //
    size_t numPrims = affectedPrims.size();
    for (size_t primNum = 0; primNum < numPrims; ++primNum) {
        const SdfPath &usdPath = affectedPrims[primNum];

        _PrimInfo *primInfo = GetPrimInfo(usdPath);
        if (primInfo == nullptr) {
            TF_CODING_ERROR("Prim in id list is not in prim info: %s",
                            usdPath.GetText());
            continue;
        }
        if (!TF_VERIFY(primInfo->adapter, "%s", usdPath.GetText())) {
            continue;
        }

        _AdapterSharedPtr const &adapter = primInfo->adapter;

        SdfPath instancer = adapter->GetInstancer(usdPath);
        if (!instancer.IsEmpty()) {
            _PrimInfo *instancerInfo = GetPrimInfo(instancer);
            if (!TF_VERIFY(instancerInfo, "%s", usdPath.GetText()) ||
                !TF_VERIFY(instancerInfo->adapter, "%s", usdPath.GetText())) {
                continue;
            }

            // redirect to native instancer.
            instancerInfo->adapter->MarkTransformDirty(instancerInfo->usdPrim,
                                                       instancer,
                                                       &indexProxy);

            // also communicate adapter to get the list of instanced proto prims
            // to be marked as dirty. for those are not in the namespace children
            // of the instancer (needed for NI-PI cases).
            SdfPathVector const &paths = adapter->GetDependPaths(instancer);
            TF_FOR_ALL (instIt, paths) {
                // recurse
                _MarkSubtreeTransformDirty(*instIt);
            }
        } else if (_instancerPrimPaths.find(usdPath) != _instancerPrimPaths.end()) {

            // instancer itself
            adapter->MarkTransformDirty(primInfo->usdPrim,
                                        usdPath,
                                        &indexProxy);

            // also communicate adapter to get the list of instanced proto prims
            // to be marked as dirty. for those are not in the namespace children
            // of the instancer.
            SdfPathVector const &paths = adapter->GetDependPaths(usdPath);
            TF_FOR_ALL (instIt, paths) {
                // recurse
                _MarkSubtreeTransformDirty(*instIt);
            }
        } else {
            adapter->MarkTransformDirty(primInfo->usdPrim,
                                        usdPath,
                                        &indexProxy);
        }
    }
}

void
UsdImagingDelegate::SetRootVisibility(bool isVisible)
{
    if (isVisible == _rootIsVisible)
        return;
    _rootIsVisible = isVisible;

    UsdImagingIndexProxy indexProxy(this, nullptr);

    TF_FOR_ALL(it, _primInfoMap) {
        const SdfPath &usdPath = it->first;
        _PrimInfo &primInfo = it->second;

        if (TF_VERIFY(primInfo.adapter, "%s", usdPath.GetText())) {
            primInfo.adapter->MarkVisibilityDirty(primInfo.usdPrim,
                                                  usdPath,
                                                  &indexProxy);
        }
    }
}

SdfPath 
UsdImagingDelegate::GetPathForInstanceIndex(SdfPath const& protoPrimPath,
                                            int instanceIndex,
                                            int *absoluteInstanceIndex,
                                            SdfPath *rprimPath,
                                            SdfPathVector *instanceContext)
{
    SdfPath usdPath = GetPathForUsd(protoPrimPath);

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "GetPathForInstanceIndex(%s, %d)\n",
        usdPath.GetText(), instanceIndex);

    // resolve all instancer hierarchy.
    int instanceCount = 0;
    int protoInstanceIndex = instanceIndex;
    int absIndex = ALL_INSTANCES; // PointInstancer may overwrite.
    SdfPathVector resolvedInstanceContext;
    SdfPath resolvedRprimPath;
    do {
        _PrimInfo *primInfo = GetPrimInfo(usdPath);
        if (!TF_VERIFY(primInfo, "%s\n", usdPath.GetText()) ||
            !TF_VERIFY(primInfo->adapter, "%s\n", usdPath.GetText())) {
            return GetPathForIndex(usdPath);
        }

        _AdapterSharedPtr const& adapter = primInfo->adapter;
        usdPath = adapter->GetPathForInstanceIndex(
            usdPath, instanceIndex, &instanceCount, &absIndex, 
            &resolvedRprimPath, &resolvedInstanceContext);

        if (usdPath.IsEmpty()) {
            break;
        }

        // reach to non-prototype node or native instancer's instance path.
        if (instanceCount == 0) {
            break;
        }

        // decode instanceIndex to the next level
        if (instanceCount > 0) {
            instanceIndex /= instanceCount;
        }

    } while(true);

    TF_DEBUG(USDIMAGING_SELECTION).Msg("GetPathForInstanceIndex(%s, %d) = "
        "(%s, %d, %s)\n", protoPrimPath.GetText(), protoInstanceIndex,
        usdPath.GetText(), absIndex,
        resolvedInstanceContext.empty() ? "(empty)" :
        resolvedInstanceContext.back().GetText());

    if (absoluteInstanceIndex) {
        *absoluteInstanceIndex = absIndex;
    }

    if (rprimPath) {
        *rprimPath = resolvedRprimPath;
    }

    if (instanceContext) {
        *instanceContext = resolvedInstanceContext;
    }

    return GetPathForIndex(usdPath);
}

bool
UsdImagingDelegate::PopulateSelection(
              HdSelection::HighlightMode const& highlightMode,
              SdfPath const &path,
              int instanceIndex,
              HdSelectionSharedPtr const &result)
{
    HD_TRACE_FUNCTION();

    // Process any pending path resyncs/updates first to ensure all
    // adapters are up-to-date.
    //
    // XXX:
    // It feels a bit unsatisfying to have to do this here. UsdImagingDelegate
    // should provide better guidance about when scene description changes are
    // handled.
    ApplyPendingUpdates();

    // UsdImagingDelegate currently only supports hiliting an instance in its
    // entirety.  With the advent of UsdPrim "instance proxies", it will be
    // natural to select prims inside of instances.  When 'path' is such
    // a sub-instance path, rather than hilite nothing, we will find and hilite
    // our top-level instance.
    SdfPath usdPath = GetPathForUsd(path);
    // Since it is technically possible to call PopulateSelection() before
    // Populate(), we guard access to _stage.  Ideally this would be a TF_VERIFY
    // but some clients need to be fixed first.
    if (_stage) {
        UsdPrim usdPrim = _stage->GetPrimAtPath(usdPath);
        // Should not need to check for pseudoroot since it can never be 
        // an instance proxy
        while (usdPrim && usdPrim.IsInstanceProxy()){
            usdPrim = usdPrim.GetParent();
        }
        if (usdPrim){
            usdPath = usdPrim.GetPath();
        }
    }
    
    _PrimInfo *primInfo = GetPrimInfo(usdPath);

    bool added = false;

    // UsdImagingDelegate only supports top-most level per-instance highlighting
    VtIntArray instanceIndices;
    if (instanceIndex != ALL_INSTANCES) {
        instanceIndices.push_back(instanceIndex);
    }

    if (primInfo && TF_VERIFY(primInfo->adapter, "%s\n", usdPath.GetText())) {
        _AdapterSharedPtr const& adapter = primInfo->adapter;

        // Prim, or instancer
        return adapter->PopulateSelection(highlightMode, usdPath,
                                          instanceIndices, result);
    } else {

        // Select prims that are part of the path subtree. Exclude prototypes
        // since they are handled by their instancers' PopulateSelection calls.
        SdfPathVector affectedPrims;
        HdPrimGather gather;
        gather.Subtree(_usdIds.GetIds(), usdPath, &affectedPrims);

        size_t numPrims = affectedPrims.size();
        for (size_t primNum = 0; primNum < numPrims; ++primNum) {
            const SdfPath &primPath = affectedPrims[primNum];

            _PrimInfo *primInfo = GetPrimInfo(primPath);
            if (primInfo == nullptr) {
                TF_CODING_ERROR("Prim in usd ids is not in prim info: %s",
                                primPath.GetText());
                continue;
            }
            if (!TF_VERIFY(primInfo->adapter, "%s\n", primPath.GetText())) {
                continue;
            }

            _AdapterSharedPtr const &adapter = primInfo->adapter;

            // PopulateSelection works as expected on un-instanced rprims.
            // For PointInstancers, PopulateSelection adds all of their
            // children. For native instances, PopulateSelection will add
            // selections for all of the prims/instances that are logically
            // below primPath.
            //
            // This means that if we run across a property path (instanced
            // rprim), we should skip it so the instance adapters can work.
            if (primPath.IsPropertyPath()) {
                continue;
            }
            added |= adapter->PopulateSelection(highlightMode, primPath,
                                                VtIntArray(), result);
        }
    }
    return added;
}

/*virtual*/
GfMatrix4d 
UsdImagingDelegate::GetTransform(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    SdfPath usdPath = GetPathForUsd(id);
    GfMatrix4d ctm(1.0);
    if (_valueCache.ExtractTransform(usdPath, &ctm)) {
        return ctm;
    }
    // Slow path, we should not hit this.
    TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow transform fetch for %s\n", 
                               id.GetText());

    _UpdateSingleValue(usdPath, HdChangeTracker::DirtyTransform);
    TF_VERIFY(_valueCache.ExtractTransform(usdPath, &ctm));
    return ctm;
}

size_t
UsdImagingDelegate::SampleTransform(SdfPath const & id, size_t maxNumSamples,
                                    float *times, GfMatrix4d *samples)
{
    if (maxNumSamples < 1) {
        return 0;
    }

    SdfPath usdPath = GetPathForUsd(id);
    UsdPrim prim = _stage->GetPrimAtPath(usdPath);
    if (!prim) {
        // If this is not a literal USD prim, it is an instance of
        // other object synthesized by UsdImaging.  Just return
        // the single transform sample from the ValueCache.
        samples[0] = GetTransform(id);
        return 1;
    }

    // Provide the number of time samples configured in _timeSampleOffsets,
    // but limited to the caller's declared capacity.
    size_t numSamples = std::min(maxNumSamples, _timeSampleOffsets.size());

    // XXX: We should add caching to the transform computation if this shows
    // up in profiling, but all of our current caches are cleared on time change
    // so we'd need to write a new structure.
    for (size_t i=0; i < numSamples; ++i) {
        UsdTimeCode offsetTime = GetTimeWithOffset(_timeSampleOffsets[i]);
        times[i] = _timeSampleOffsets[i];
        samples[i] = UsdImaging_XfStrategy::ComputeTransform(
            prim, _rootPrimPath, offsetTime, _rigidXformOverrides) * _rootXf;
    }

    // Some backends benefit if they can avoid time sample animation
    // for fixed transforms.  This is difficult to compute explicitly
    // due to the hierarchial nature of concated transforms, so we
    // do a post-pass sweep to detect static transforms here.
    for (size_t i=1; i < numSamples; ++i) {
        if (samples[i] != samples[0]) {
            // At least 1 sample is different, so return them all.
            return numSamples;
        }
    }
    // All samples are the same, so just return 1.
    return 1;
}

bool
UsdImagingDelegate::IsInInvisedPaths(SdfPath const &usdPath) const
{
    TF_FOR_ALL(it, _invisedPrimPaths) {
        if (usdPath.HasPrefix(*it)) {
            return true;
        }
    }
    return false;
}

/*virtual*/
bool
UsdImagingDelegate::GetVisible(SdfPath const& id)
{
    HD_TRACE_FUNCTION();

    // Root visibility overrides prim visibility.
    if (!_rootIsVisible)
        return false;

    SdfPath usdPath = GetPathForUsd(id);
    // for instance protos (not IsPrimPath), visibility is
    // controlled by instanceIndices.
    if (usdPath.IsPrimPath() && IsInInvisedPaths(usdPath)) {
        return false;
    }

    bool vis = true;
    if (_valueCache.FindVisible(usdPath, &vis)) {
        return vis;
    }

    // Slow path, we should not hit this.
    TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow visible fetch for %s\n", 
                               id.GetText());

    _UpdateSingleValue(usdPath, HdChangeTracker::DirtyVisibility);
    if (TF_VERIFY(_valueCache.ExtractVisible(usdPath, &vis),
                "<%s>\n", usdPath.GetText())) {
        return vis;
    }
    return false;
}

/*virtual*/ 
VtValue 
UsdImagingDelegate::Get(SdfPath const& id, TfToken const& key)
{
    HD_TRACE_FUNCTION();

    SdfPath usdPath = GetPathForUsd(id);
    VtValue value;

    if (!_valueCache.ExtractPrimvar(usdPath, key, &value)) {
        if (key == HdTokens->points) {
            _UpdateSingleValue(usdPath,HdChangeTracker::DirtyPoints);
            if (!TF_VERIFY(_valueCache.ExtractPoints(usdPath, &value))) {
                VtVec3fArray vec;
                value = VtValue(vec);
            }
        } else if (key == HdTokens->displayColor) {
            // XXX: Getting all primvars here when we only want color is wrong.
            _UpdateSingleValue(usdPath,HdChangeTracker::DirtyPrimvar);
            if (!TF_VERIFY(_valueCache.ExtractColor(usdPath, &value))){
                VtVec3fArray vec(1);
                vec.push_back(GfVec3f(.5,.5,.5));
                value = VtValue(vec);
            }
        } else if (key == HdTokens->displayOpacity) {
            // XXX: Getting all primvars here when we only want opacity is bad.
            _UpdateSingleValue(usdPath,HdChangeTracker::DirtyPrimvar);
            if (!TF_VERIFY(_valueCache.ExtractOpacity(usdPath, &value))){
                VtFloatArray vec(1);
                vec.push_back(1.0f);
                value = VtValue(vec);
            }
        } else if (key == HdTokens->widths) {
            _UpdateSingleValue(usdPath,HdChangeTracker::DirtyWidths);
            if (!TF_VERIFY(_valueCache.ExtractWidths(usdPath, &value))){
                VtFloatArray vec(1);
                vec.push_back(1.0f);
                value = VtValue(vec);
            }
        } else if (key == HdTokens->transform) {
            value = VtValue(
                UsdImaging_XfStrategy::ComputeTransform(_GetPrim(usdPath), 
                    _rootPrimPath, GetTime(), _rigidXformOverrides) * _rootXf);
        } else if (UsdGeomPrimvar pv = UsdGeomGprim(_GetPrim(usdPath))
                                                .GetPrimvar(key)) {
            // Note here that Hydra requested "color" (e.g.) and we've converted
            // it to primvars:color automatically by virtue of UsdGeomPrimvar.
            TF_VERIFY(pv.ComputeFlattened(&value, _time), "%s, %s\n", 
                      id.GetText(), key.GetText());
        } else {
            // XXX: This does not work for point instancer child prims; while we
            // do not hit this code path given the current state of the
            // universe, we need to rethink UsdImagingDelegate::Get().
            TF_VERIFY(_GetPrim(usdPath).GetAttribute(key).Get(&value, _time),
                  "%s, %s\n", id.GetText(), key.GetText());
        }
    }

    if (value.IsEmpty()) {
        TF_WARN("Empty VtValue: <%s> %s\n", id.GetText(), key.GetText());
    }

    // We generally don't want Vec2d arrays, convert to vec2f.
    if (value.IsHolding<VtVec2dArray>()) {
        value = VtValue::Cast<VtVec2fArray>(value);
    }

    return value;
}

/*virtual*/
size_t
UsdImagingDelegate::SamplePrimvar(SdfPath const& id, TfToken const& key,
                                  size_t maxNumSamples,
                                  float *times, VtValue *samples)
{
    SdfPath usdPath = GetPathForUsd(id);
    _PrimInfo *primInfo = GetPrimInfo(usdPath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->SamplePrimvar(primInfo->usdPrim, usdPath, key,
                            _time, _timeSampleOffsets,
                            maxNumSamples, times, samples);
    }
    return 0;
}

/*virtual*/
HdReprSelector
UsdImagingDelegate::GetReprSelector(SdfPath const &id)
{
    return _reprFallback;
}

/*virtual*/
VtArray<TfToken>
UsdImagingDelegate::GetCategories(SdfPath const &id)
{
    SdfPath usdPath = GetPathForUsd(id);
    return _collectionCache.ComputeCollectionsContainingPath(usdPath);
}

// -------------------------------------------------------------------------- //
// Primvar Support Methods
// -------------------------------------------------------------------------- //

HdPrimvarDescriptorVector
UsdImagingDelegate::GetPrimvarDescriptors(SdfPath const& id,
                                          HdInterpolation interpolation)
{
    HD_TRACE_FUNCTION();
    SdfPath usdPath = GetPathForUsd(id);
    HdPrimvarDescriptorVector primvars;
    HdPrimvarDescriptorVector allPrimvars;
    // We expect to populate an entry always (i.e., we don't use a slow path
    // fetch)
    if (!TF_VERIFY(_valueCache.FindPrimvars(usdPath, &allPrimvars), 
                   "<%s> interpolation: %s", usdPath.GetText(),
                   TfEnum::GetName(interpolation).c_str())) {
        return primvars;
    }
    // It's valid to have no authored primvars (they could be computed)
    for (HdPrimvarDescriptor const& pv: allPrimvars) {
        // Filter the stored primvars to just ones of the requested type.
        if (pv.interpolation == interpolation) {
            primvars.push_back(pv);
        }
    }
    return primvars;
}

/*virtual*/
VtIntArray
UsdImagingDelegate::GetInstanceIndices(SdfPath const &instancerId,
                                       SdfPath const &prototypeId)
{
    HD_TRACE_FUNCTION();

    // if prototypeId is also a point instancer (nested case),
    // this function may be called multiple times with the same arguments:
    //
    //  instancer1
    //    |
    //    +-- instancer2
    //          |
    //          +-- protoMesh1
    //          +-- protoMesh2
    //
    //  a) (instancer2, protoMesh1) then (instancer1, instancer2)
    //  b) (instancer2, protoMesh2) then (instancer1, instancer2)
    //
    //  when multithreaded sync is enabled, (a) and (b) happen concurrently.
    //  use FindInstanceIndices instead of ExtractInstanceIndices to avoid
    //  clearing the cached value.

    SdfPath usdPath = GetPathForUsd(prototypeId);
    VtValue indices;

    // TODO: it would be nice to only call Find on instancers and call Extract
    // otherwise, however we have no way of making that distinction currently.
    if (!_valueCache.FindInstanceIndices(usdPath, &indices)) {
        // Slow path, we should not hit this.
        TF_DEBUG(HD_SAFE_MODE).Msg(
                                "WARNING: Slow instance indices fetch for %s\n", 
                                prototypeId.GetText());
        _UpdateSingleValue(usdPath, HdChangeTracker::DirtyInstanceIndex);
        TF_VERIFY(_valueCache.FindInstanceIndices(usdPath, &indices));
    }

    if (indices.IsEmpty()) {
        TF_WARN("Empty InstanceIndices (%s, %s)\n",
                instancerId.GetText(), prototypeId.GetText());
        return VtIntArray();
    }

    return indices.Get<VtIntArray>();
}

/*virtual*/
GfMatrix4d
UsdImagingDelegate::GetInstancerTransform(SdfPath const &instancerId)
{
    HD_TRACE_FUNCTION();

    // InstancerTransform is cached on instancer prim, not prototype prim

    SdfPath usdPath = GetPathForUsd(instancerId);
    GfMatrix4d ctm(1.0);

    // same as GetInstanceIndices, the instancer transform may be
    // asked multiple times for all prototypes. use Find instead of Extract
    // to preserve the result for further lookup.

    if (!_valueCache.FindInstancerTransform(usdPath, &ctm)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow instancer transform fetch for %s\n", 
            instancerId.GetText());
        _UpdateSingleValue(usdPath, HdChangeTracker::DirtyTransform);
        TF_VERIFY(_valueCache.FindInstancerTransform(usdPath, &ctm));
    }

    return ctm;
}

/*virtual*/
size_t
UsdImagingDelegate::SampleInstancerTransform(SdfPath const &instancerId,
                                             size_t maxSampleCount,
                                             float *times,
                                             GfMatrix4d *samples)
{
    SdfPath usdPath = GetPathForUsd(instancerId);
    _PrimInfo *primInfo = GetPrimInfo(usdPath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->SampleInstancerTransform(primInfo->usdPrim, usdPath,
                                       _time, _timeSampleOffsets,
                                       maxSampleCount, times, samples);
    }
    return 0;
}

/*virtual*/ 
SdfPath 
UsdImagingDelegate::GetMaterialId(SdfPath const &rprimId)
{
    SdfPath usdPath = GetPathForUsd(rprimId);
    SdfPath pathValue;
    if (!_valueCache.ExtractMaterialId(usdPath, &pathValue)) {
        _UpdateSingleValue(usdPath, HdChangeTracker::DirtyMaterialId);
        TF_VERIFY(_valueCache.ExtractMaterialId(usdPath, &pathValue));
    }
    return GetPathForIndex(pathValue);
}

/*virtual*/
std::string
UsdImagingDelegate::GetSurfaceShaderSource(SdfPath const &materialId)
{
    HD_TRACE_FUNCTION();

    if (materialId.IsEmpty()) {
        return std::string();
    }

    // If custom shading is disabled, use fallback
    if (!_sceneMaterialsEnabled) {
        return std::string();
    }

    SdfPath usdPath = GetPathForUsd(materialId);
    std::string source;

    if (!_valueCache.ExtractSurfaceShaderSource(usdPath, &source)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow surface shader source fetch for %s\n",
            materialId.GetText());
        _UpdateSingleValue(usdPath, HdMaterial::DirtySurfaceShader);
        TF_VERIFY(_valueCache.ExtractSurfaceShaderSource(usdPath, &source));
    }

    return source;
}

/*virtual*/
std::string
UsdImagingDelegate::GetDisplacementShaderSource(SdfPath const &materialId)
{
    HD_TRACE_FUNCTION();

    if (materialId.IsEmpty()) {
        return std::string();
    }

    // If custom shading is disabled, use fallback
    if (!_sceneMaterialsEnabled) {
        return std::string();
    }

    SdfPath usdPath = GetPathForUsd(materialId);
    std::string source;

    if (!_valueCache.ExtractDisplacementShaderSource(usdPath, &source)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow displacement shader source fetch for %s\n",
            materialId.GetText());
        _UpdateSingleValue(usdPath, HdMaterial::DirtySurfaceShader);
        TF_VERIFY(_valueCache.ExtractDisplacementShaderSource(
                  usdPath, &source));
    }

    return source;
}

/*virtual*/
VtValue
UsdImagingDelegate::GetMaterialParamValue(SdfPath const &materialId, 
                                          TfToken const &paramName)
{
    HD_TRACE_FUNCTION();

    if (materialId.IsEmpty()) {
        // Handle fallback material
        VtFloatArray dummy;
        dummy.resize(1);
        return VtValue(dummy);
    }

    SdfPath usdPath = GetPathForUsd(materialId);
    VtValue param;

    // XXX: See comment in GetMaterialParams.
    TF_VERIFY(_valueCache.ExtractMaterialParam(
                usdPath, paramName, &param));

    if (param.IsEmpty()) {
        // XXX: hydra crashes with empty vt values, should fix
        VtFloatArray dummy;
        dummy.resize(1);
        param = VtValue(dummy);
    }
    return param;
}

/*virtual*/
HdMaterialParamVector
UsdImagingDelegate::GetMaterialParams(SdfPath const &materialId)
{
    HD_TRACE_FUNCTION();

    if (materialId.IsEmpty()) {
        return HdMaterialParamVector();
    }

    // If custom shading is disabled, use fallback
    if (!_sceneMaterialsEnabled) {
        return HdMaterialParamVector();
    }


    SdfPath usdPath = GetPathForUsd(materialId);
    HdMaterialParamVector params;

    // XXX: This is a little complicated. Materials aren't part of the
    // delegate sync, since they aren't rprims. We can manually call
    // UpdateForTime() on materials via _UpdateSingleValue, but we can't rely
    // on the value cache's "ExtractFoo" to fail if unpopulated, like we do
    // elsewhere, because the value cache GarbageCollect is called *ONLY* on
    // delegates with rprims that participated in delegate sync.  So if a
    // material is the only thing changing this frame, you'll have stale empty
    // values from the last time you called Extract (since Extract just
    // swap()s with an empty value, and doesn't delete the cache entry until
    // GC).
    //
    // As a workaround: Every time we update materials, we'll call
    // GetSurfaceShaderParams() once, and then GetSurfaceShaderParamValue()
    // many times.  We unconditionally update params here, and let GetParamValue
    // hitch a free ride. This happens to work with HdStShader's implementation.
    //
    // The correct long-term solution is to include sprims in delegate sync!

    _UpdateSingleValue(usdPath, HdMaterial::DirtyParams);
    TF_VERIFY(_valueCache.FindMaterialParams(usdPath, &params));

    // Connections need to be represented as index paths...
    TF_FOR_ALL(paramIt, params) {
        if (paramIt->IsTexture()) {
            // Unfortunately, HdMaterialParam is immutable;
            // fortunately, it has relatively lightweight members.
            *paramIt = HdMaterialParam(
                HdMaterialParam::ParamTypeTexture,
                paramIt->GetName(),
                paramIt->GetFallbackValue(),
                GetPathForIndex(paramIt->GetConnection()),
                paramIt->GetSamplerCoordinates(),
                paramIt->GetTextureType());
        }
    }

    return params;
}

HdTextureResource::ID
UsdImagingDelegate::GetTextureResourceID(SdfPath const &textureId)
{
    SdfPath usdPath = GetPathForUsd(textureId);
    _PrimInfo *primInfo = GetPrimInfo(usdPath);
    if (primInfo) {
        return primInfo->adapter
            ->GetTextureResourceID(primInfo->usdPrim, usdPath, _time,
                                   (size_t) &GetRenderIndex() );
    } 

    // A bad asset can cause GetPrimInfo() to fail. Hence, issue a warning and 
    // return an invalid resource ID.
    TF_WARN("Could not get prim tracking data for path <%s>. Unable to get "
            "associated texture resource ID.", textureId.GetText());

    return HdTextureResource::ID(-1);
}

HdTextureResourceSharedPtr
UsdImagingDelegate::GetTextureResource(SdfPath const &textureId)
{
    // PERFORMANCE: We should schedule this to be updated during Sync, rather
    // than pulling values on demand.
    SdfPath usdPath = GetPathForUsd(textureId);
    _PrimInfo *primInfo = GetPrimInfo(usdPath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->GetTextureResource(primInfo->usdPrim, usdPath, _time);
    }
    return nullptr;
}

VtValue 
UsdImagingDelegate::GetLightParamValue(SdfPath const &id, 
                                       TfToken const &paramName)
{
    // PERFORMANCE: We should schedule this to be updated during Sync, rather
    // than pulling values on demand.
 
    if (!TF_VERIFY(id != SdfPath())) {
        return VtValue();
    }

    SdfPath usdPath = GetPathForUsd(id);
    UsdPrim prim = _GetPrim(usdPath);
    if (!TF_VERIFY(prim)) {
        return VtValue();
    }
    UsdLuxLight light = UsdLuxLight(prim);
    if (!light) {
        // XXX Should it be a coding error to query light params
        // on non-light prims?
        return VtValue();
    }

    // Special handling of non-attribute parameters
    if (paramName == _tokens->texturePath) {
        // This can be moved to a separate function as we add support for 
        // other light types that use textures in multiple ways
        UsdLuxDomeLight domeLight(prim);
        SdfAssetPath asset; 
        if (!domeLight.GetTextureFileAttr().Get(&asset)) {
            return VtValue();
        }
        return VtValue(asset.GetResolvedPath());
    } else if (paramName == HdTokens->lightLink) {
        UsdCollectionAPI lightLink = light.GetLightLinkCollectionAPI();
        return VtValue(_collectionCache.GetIdForCollection(lightLink));
    } else if (paramName == HdTokens->shadowLink) {
        UsdCollectionAPI shadowLink = light.GetShadowLinkCollectionAPI();
        return VtValue(_collectionCache.GetIdForCollection(shadowLink));
    }

    // Fallback to USD attributes.
    if (prim.HasAttribute(paramName)) {
        UsdAttribute attr = prim.GetAttribute(paramName);
        VtValue value;
        // Reading the value may fail, should we warn here when it does?
        attr.Get(&value, GetTime());
        return value;
    }

    return VtValue();
}

HdVolumeFieldDescriptorVector
UsdImagingDelegate::GetVolumeFieldDescriptors(SdfPath const &volumeId)
{
    // PERFORMANCE: We should schedule this to be updated during Sync, rather
    // than pulling values on demand.
    SdfPath usdPath = GetPathForUsd(volumeId);
    _PrimInfo *primInfo = GetPrimInfo(usdPath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->GetVolumeFieldDescriptors(primInfo->usdPrim, usdPath, _time);
    }

    return HdVolumeFieldDescriptorVector();
}

VtValue
UsdImagingDelegate::GetMaterialResource(SdfPath const &materialId)
{
    VtValue vtMatResource;

    if (!TF_VERIFY(materialId != SdfPath())) {
        return vtMatResource;
    }

    SdfPath usdPath = GetPathForUsd(materialId);
    _UpdateSingleValue(usdPath, HdMaterial::DirtyResource);
    TF_VERIFY(_valueCache.FindMaterialResource(usdPath, &vtMatResource));
    return vtMatResource;
}

TfTokenVector 
UsdImagingDelegate::GetMaterialPrimvars(SdfPath const &materialId)
{
    if (!TF_VERIFY(materialId != SdfPath())) {
        return TfTokenVector();
    }

    SdfPath usdPath = GetPathForUsd(materialId);
    TfTokenVector materialPrimvars;
    _valueCache.FindMaterialPrimvars(usdPath, &materialPrimvars);

    return materialPrimvars;
}

VtDictionary
UsdImagingDelegate::GetMaterialMetadata(SdfPath const &materialId)
{

    HD_TRACE_FUNCTION();

    if (!TF_VERIFY(materialId != SdfPath())) {
        return VtDictionary();
    }

    // If custom shading is disabled, use fallback
    if (!_sceneMaterialsEnabled) {
        return VtDictionary();
    }

    SdfPath usdPath = GetPathForUsd(materialId);
    VtValue value;

    if (!_valueCache.ExtractMaterialMetadata(usdPath, &value)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow material metadata fetch for %s\n",
            materialId.GetText());
        // MaterialMetadata updates along with DirtySurfaceShader
        _UpdateSingleValue(usdPath, HdMaterial::DirtySurfaceShader);
        TF_VERIFY(_valueCache.ExtractMaterialMetadata(usdPath, &value));
    }

    return value.GetWithDefault<VtDictionary>();
}

TfTokenVector
UsdImagingDelegate::GetExtComputationSceneInputNames(
    SdfPath const& computationId)
{
    HD_TRACE_FUNCTION();

    SdfPath usdPath = GetPathForUsd(computationId);

    TfTokenVector inputNames;
    if (!_valueCache.ExtractExtComputationSceneInputNames(
            usdPath, &inputNames)) {

        TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow extComputation input "
                                   "descriptor fetch for %s\n", 
                                   computationId.GetText());
        
        _UpdateSingleValue(usdPath, HdExtComputation::DirtyInputDesc);
        TF_VERIFY(_valueCache.ExtractExtComputationSceneInputNames(
                      usdPath, &inputNames));
    }

    return inputNames;
}

HdExtComputationInputDescriptorVector
UsdImagingDelegate::GetExtComputationInputDescriptors(
    SdfPath const& computationId)
{
    HD_TRACE_FUNCTION();

    SdfPath usdPath = GetPathForUsd(computationId);

    HdExtComputationInputDescriptorVector inputs;
    if (!_valueCache.ExtractExtComputationInputs(usdPath, &inputs)) {

        TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow extComputation input "
                                   "descriptor fetch for %s\n", 
                                   computationId.GetText());
        
        _UpdateSingleValue(usdPath, HdExtComputation::DirtyInputDesc);
        TF_VERIFY(_valueCache.ExtractExtComputationInputs(usdPath, &inputs));
    }

    return inputs;
}

HdExtComputationOutputDescriptorVector
UsdImagingDelegate::GetExtComputationOutputDescriptors(
    SdfPath const& computationId)
{
    HD_TRACE_FUNCTION();

    SdfPath usdPath = GetPathForUsd(computationId);

    HdExtComputationOutputDescriptorVector outputs;
    if (!_valueCache.ExtractExtComputationOutputs(usdPath, &outputs)) {

        TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow extComputation output "
                                   "descriptor fetch for %s\n", 
                                   computationId.GetText());
        
        _UpdateSingleValue(usdPath, HdExtComputation::DirtyOutputDesc);
        TF_VERIFY(_valueCache.ExtractExtComputationOutputs(usdPath, &outputs));
    }

    return outputs;
}

HdExtComputationPrimvarDescriptorVector
UsdImagingDelegate::GetExtComputationPrimvarDescriptors(
    SdfPath const& computationId,
    HdInterpolation interpolation)
{
    HD_TRACE_FUNCTION();
    SdfPath usdPath = GetPathForUsd(computationId);

    HdExtComputationPrimvarDescriptorVector allPrimvars;
    // We don't require an entry to be populated.
    _valueCache.FindExtComputationPrimvars(usdPath, &allPrimvars);
    
    // Don't use a verify below because it is often the case that there are
    // no computed primvars on an rprim.
    if (allPrimvars.empty()) {
        return allPrimvars;
    }

    HdExtComputationPrimvarDescriptorVector primvars;
    for (const auto& pv : allPrimvars) {
        // Filter the stored primvars to just ones of the requested type.
        if (pv.interpolation == interpolation) {
            primvars.push_back(pv);
        }
    }
    return primvars;
}

VtValue
UsdImagingDelegate::GetExtComputationInput(SdfPath const& computationId,
                                           TfToken const& input)
{
    SdfPath usdPath = GetPathForUsd(computationId);
    VtValue value;

    if (!_valueCache.ExtractExtComputationInput(usdPath, input, &value)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow fetch for token %s for computation %s\n", 
                            input.GetText(), computationId.GetText());
        if (input == HdTokens->dispatchCount) {
            _UpdateSingleValue(usdPath, HdExtComputation::DirtyDispatchCount);
        } else if (input == HdTokens->elementCount) {
            _UpdateSingleValue(usdPath, HdExtComputation::DirtyElementCount);
        } else {
            _UpdateSingleValue(usdPath, HdExtComputation::DirtySceneInput);
        }

        TF_VERIFY(_valueCache.ExtractExtComputationInput(usdPath, input, 
                                                         &value));
    }
    return value;
}

std::string
UsdImagingDelegate::GetExtComputationKernel(SdfPath const& computationId)
{
    HD_TRACE_FUNCTION();
    
    std::string kernel;
    if (!computationId.IsEmpty()) {

        SdfPath usdPath = GetPathForUsd(computationId);
        if (!_valueCache.ExtractExtComputationKernel(usdPath, &kernel)) {
            TF_DEBUG(HD_SAFE_MODE).Msg(
                "WARNING: Slow extComputation kernel fetch for %s\n",
                computationId.GetText());
            _UpdateSingleValue(usdPath, HdExtComputation::DirtyKernel);
            TF_VERIFY(_valueCache.ExtractExtComputationKernel(
                          usdPath, &kernel));
        }
    }
    return kernel;
}

void
UsdImagingDelegate::InvokeExtComputation(SdfPath const& computationId,
                                         HdExtComputationContext *context)
{
    _PrimInfo *primInfo = GetPrimInfo(computationId);

    _PrimInfoMap::iterator it = _primInfoMap.find(computationId);
    if (TF_VERIFY(primInfo, "%s\n", computationId.GetText()) &&
        TF_VERIFY(primInfo->adapter, "%s\n", computationId.GetText())) {
        primInfo->adapter->InvokeComputation(computationId, context);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

