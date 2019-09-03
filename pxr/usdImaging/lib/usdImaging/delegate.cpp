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
#include "pxr/usdImaging/usdImaging/coordSysAdapter.h"
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
    , _coordSysBindingCache(GetTime(), &_coordSysBindingImplData)
    , _visCache(GetTime())
    , _purposeCache() // note that purpose is uniform, so no GetTime()
    , _drawModeCache(GetTime())
    , _inheritedPrimvarCache()
    , _displayGuides(true)
    , _enableUsdDrawModes(true)
    , _hasDrawModeAdapter( UsdImagingAdapterRegistry::GetInstance()
                           .HasAdapter(UsdImagingAdapterKeyTokens
                                       ->drawModeAdapterKey) )
    , _sceneMaterialsEnabled(true)
    , _coordSysEnabled(parentIndex
                       ->IsSprimTypeSupported(HdPrimTypeTokens->coordSys))
{
    // Provide a callback to the _coordSysBindingCache so it can
    // convert USD paths to Hydra ID's.
    _coordSysBindingImplData.usdToHydraPath =
        std::bind( &UsdImagingDelegate::ConvertCachePathToIndexPath, this,
                   std::placeholders::_1 );

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

    _instancerPrimCachePaths.clear();
    _refineLevelMap.clear();
    _pickablesMap.clear();
    _hdPrimInfoMap.clear();
    _cachePaths.Clear();
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

    return _AdapterLookup(adapterKey);
}

UsdImagingDelegate::_AdapterSharedPtr const& 
UsdImagingDelegate::_AdapterLookup(TfToken const& adapterKey)
{
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

UsdImagingDelegate::_HdPrimInfo *
UsdImagingDelegate::_GetHdPrimInfo(const SdfPath &cachePath)
{
    _HdPrimInfoMap::iterator it = _hdPrimInfoMap.find(cachePath);
    if (it == _hdPrimInfoMap.end()) {
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

    void AddTask(UsdImagingDelegate* delegate, SdfPath const& cachePath) {
        _tasks.push_back(_Task(delegate, cachePath));
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
            SdfPath const& cachePath = it->path;

            _HdPrimInfo *primInfo = delegate->_GetHdPrimInfo(cachePath);
            if (TF_VERIFY(primInfo, "%s\n", cachePath.GetText())) {
                _AdapterSharedPtr const& adapter = primInfo->adapter;
                if (TF_VERIFY(adapter, "%s\n", cachePath.GetText())) {
                    adapter->TrackVariabilityPrep(primInfo->usdPrim, cachePath);
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
            SdfPath const& cachePath = _tasks[i].path;

            _HdPrimInfo *primInfo = delegate->_GetHdPrimInfo(cachePath);
            if (TF_VERIFY(primInfo, "%s\n", cachePath.GetText())) {
                _AdapterSharedPtr const& adapter = primInfo->adapter;
                if (TF_VERIFY(adapter, "%s\n", cachePath.GetText())) {
                    adapter->TrackVariability(primInfo->usdPrim,
                                              cachePath,
                                              &primInfo->timeVaryingBits);
                    if (primInfo->timeVaryingBits != HdChangeTracker::Clean) {
                        adapter->MarkDirty(primInfo->usdPrim,
                                           cachePath,
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
            SdfPath const& cachePath = _tasks[i].path;

            _HdPrimInfo *primInfo = delegate->_GetHdPrimInfo(cachePath);
            if (TF_VERIFY(primInfo, "%s\n", cachePath.GetText())) {
                _AdapterSharedPtr const& adapter = primInfo->adapter;
                if (TF_VERIFY(adapter, "%s\n", cachePath.GetText())) {
                    adapter->UpdateForTime(primInfo->usdPrim,
                                           cachePath,
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
    UsdImagingDelegate::_Worker *worker, SdfPath const& cachePath)
{
    worker->AddTask(this, cachePath);
}

// -------------------------------------------------------------------------- //
// Population & Update
// -------------------------------------------------------------------------- //

void
UsdImagingDelegate::SyncAll(bool includeUnvarying)
{
    UsdImagingDelegate::_Worker worker;

    TF_FOR_ALL(it, _hdPrimInfoMap) {
        const SdfPath &cachePath = it->first;
        _HdPrimInfo &primInfo    = it->second;

        if (includeUnvarying) {
            primInfo.dirtyBits |= HdChangeTracker::AllDirty;
        } else if (primInfo.dirtyBits == HdChangeTracker::Clean) {
            continue;
        }

        // In this case, the path is coming from our internal state, so it is
        // not prefixed with the delegate ID.
        _AdapterSharedPtr adapter = primInfo.adapter;

        if (TF_VERIFY(adapter, "%s\n", cachePath.GetText())) {
            TF_DEBUG(USDIMAGING_UPDATES).Msg(
                      "[Sync] PREP: <%s> dirtyFlags: 0x%x [%s]\n",
                      cachePath.GetText(), 
                      primInfo.dirtyBits,
                      HdChangeTracker::StringifyDirtyBits(
                                                   primInfo.dirtyBits).c_str());

            adapter->UpdateForTimePrep(primInfo.usdPrim,
                                       cachePath,
                                       _time,
                                       primInfo.dirtyBits);
            worker.AddTask(this, cachePath);
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
        // must translate it via ConvertIndexPathToCachePath.
        SdfPath cachePath = ConvertIndexPathToCachePath(request->IDs[i]);
        HdDirtyBits dirtyFlags = request->dirtyBits[i];

        _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
        if (!TF_VERIFY(primInfo != nullptr, "%s\n", cachePath.GetText())) {
            continue;
        }

        // Merge UsdImaging's own dirty flags with those coming from hydra.
        primInfo->dirtyBits |= dirtyFlags;

        _AdapterSharedPtr &adapter = primInfo->adapter;
        if (TF_VERIFY(adapter, "%s\n", cachePath.GetText())) {
            TF_DEBUG(USDIMAGING_UPDATES).Msg(
                    "[Sync] PREP: <%s> dirtyFlags: 0x%x [%s]\n",
                    cachePath.GetText(), 
                    primInfo->dirtyBits,
                    HdChangeTracker::StringifyDirtyBits(primInfo->dirtyBits).c_str());

            adapter->UpdateForTimePrep(primInfo->usdPrim,
                                       cachePath,
                                       _time,
                                       primInfo->dirtyBits);
            worker.AddTask(this, cachePath);
        }
    }

    // We always include instancers.
    for (SdfPath const& cachePath: _instancerPrimCachePaths) {
        _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
        if (!TF_VERIFY(primInfo != nullptr, "%s\n", cachePath.GetText())) {
            continue;
        }

        if (primInfo->dirtyBits == HdChangeTracker::Clean) {
            continue;
        }

        _AdapterSharedPtr &adapter = primInfo->adapter;
        if (TF_VERIFY(adapter, "%s\n", cachePath.GetText())) {
            TF_DEBUG(USDIMAGING_UPDATES).Msg(
                    "[Sync] PREP Instancer: <%s> dirtyFlags: 0x%x [%s]\n",
                    cachePath.GetText(),
                    primInfo->dirtyBits,
                    HdChangeTracker::StringifyDirtyBits(
                                              primInfo->dirtyBits).c_str());
            adapter->UpdateForTimePrep(primInfo->usdPrim,
                                       cachePath,
                                       _time,
                                       primInfo->dirtyBits);
            worker.AddTask(this, cachePath);
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
        TfNotice::Register(self, &This::_OnUsdObjectsChanged, _stage);
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

    SdfPathVector const& usdPathsToRepopulate =
        proxy->_GetUsdPathsToRepopulate();
    if (usdPathsToRepopulate.empty())
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
    leafPaths.reserve(usdPathsToRepopulate.size());

    for (SdfPath const& usdPath: usdPathsToRepopulate) {
        // Discover and insert all renderable prims into the worker for later
        // execution.
        TF_DEBUG(USDIMAGING_CHANGES).Msg("[Repopulate] Root path: <%s>\n",
                            usdPath.GetText());

        UsdPrimRange range(_GetUsdPrim(usdPath));
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
    TF_FOR_ALL(it, _hdPrimInfoMap) {
        const SdfPath &cachePath = it->first;
        _HdPrimInfo &primInfo    = it->second;

        if (primInfo.timeVaryingBits != HdChangeTracker::Clean) {
            primInfo.adapter->MarkDirty(primInfo.usdPrim,
                                        cachePath,
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
    if (_usdPathsToResync.empty() && _usdPathsToUpdate.empty()) {
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
    _coordSysBindingCache.Clear();
    _inheritedPrimvarCache.Clear();

    UsdImagingDelegate::_Worker worker;
    UsdImagingIndexProxy indexProxy(this, &worker);

    if (!_usdPathsToResync.empty()) {
        // Make a copy of _usdPathsToResync but uniqued with a
        // prefix-check, which removes all elements that are prefixed by
        // other elements.
        SdfPathVector usdPathsToResync;
        usdPathsToResync.reserve(_usdPathsToResync.size());
        std::sort(_usdPathsToResync.begin(), _usdPathsToResync.end());
        std::unique_copy(_usdPathsToResync.begin(), _usdPathsToResync.end(),
                         std::back_inserter(usdPathsToResync),
                         [](SdfPath const &l, SdfPath const &r) {
                             return r.HasPrefix(l);
                         });
        _usdPathsToResync.clear();

        for (SdfPath const& usdPath: usdPathsToResync) {
            if (usdPath.IsPropertyPath()) {
                _RefreshUsdObject(usdPath, TfTokenVector(), &indexProxy);
            } else if (usdPath.IsTargetPath()) {
                // TargetPaths are their own path type, when they change, resync
                // the relationship at which they're rooted; i.e. per-target
                // invalidation is not supported.
                _RefreshUsdObject(usdPath.GetParentPath(), TfTokenVector(),
                               &indexProxy);
            } else if (usdPath.IsAbsoluteRootOrPrimPath()) {
                _ResyncUsdPrim(usdPath, &indexProxy);
            } else {
                TF_WARN("Unexpected path type to resync: <%s>",
                        usdPath.GetText());
            }
        }
    }

    // Remove any objects that were queued for removal to ensure RefreshObject
    // doesn't apply changes to removed objects.
    indexProxy._ProcessRemovals();

    if (!_usdPathsToUpdate.empty()) {
        _PathsToUpdateMap usdPathsToUpdate;
        std::swap(usdPathsToUpdate, _usdPathsToUpdate);
        TF_FOR_ALL(pathIt, usdPathsToUpdate) {
            const SdfPath& usdPath = pathIt->first;
            const TfTokenVector& changedPrimInfoFields = pathIt->second;

            if (usdPath.IsPropertyPath() || usdPath.IsAbsoluteRootOrPrimPath()){
                // Note that changedPrimInfoFields will be empty if the
                // path is a property path.
                _RefreshUsdObject(usdPath, changedPrimInfoFields, &indexProxy);

                // If any objects were removed as a result of the refresh (if it
                // internally decided to resync), they must be ejected now,
                // before the next call to _RefreshObject.
                indexProxy._ProcessRemovals();
            } else {
                TF_RUNTIME_ERROR("Unexpected path type to update: <%s>",
                        usdPath.GetText());
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
UsdImagingDelegate::_OnUsdObjectsChanged(
    UsdNotice::ObjectsChanged const& notice,
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
    _usdPathsToResync.insert(_usdPathsToResync.end(), 
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
                TfTokenVector& changedPrimInfoFields = _usdPathsToUpdate[*it];
                changedPrimInfoFields.insert(
                    changedPrimInfoFields.end(),
                    changedFields.begin(), changedFields.end());
            }
        } else if (it->IsPropertyPath()) {
            _usdPathsToUpdate.emplace(*it, TfTokenVector());
        }
    }

    if (TfDebug::IsEnabled(USDIMAGING_CHANGES)) {
        TF_FOR_ALL(it, pathsToResync) {
            TF_DEBUG(USDIMAGING_CHANGES).Msg(" - Resync queued: %s\n",
                        it->GetText());
        }
        TF_FOR_ALL(it, pathsToUpdate) {
            // For diagonstic clarity, filter out paths we decided to ignore
            if (_usdPathsToUpdate.find(*it) != _usdPathsToUpdate.end()) {
                TF_DEBUG(USDIMAGING_CHANGES).Msg(" - Refresh queued: %s\n",
                                                 it->GetText());
            }
        }
    }
}

void 
UsdImagingDelegate::_ResyncUsdPrim(SdfPath const& usdPath, 
                                   UsdImagingIndexProxy* proxy,
                                   bool repopulateFromRoot) 
{
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: <%s>\n",
            usdPath.GetText());

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
    // below where instancerUsdPath is used.
    SdfPath instancerCachePath;

    //
    // Detect if the prim that is being resynced is in a sub-tree that was
    // pruned by an ancestral prim adapter.
    //
    UsdPrim prim = _stage->GetPrimAtPath(usdPath);
    if (!prim) {
        TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: "
                   "Root prim <%s> no longer exists\n",
                        usdPath.GetText());
    } else {
        if (prim.IsInMaster()) {
            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Resync Prim]: "
                   "Skipping root prim <%s>, is in master\n",
                        usdPath.GetText());
            return;
        }
        // First, search parents for pruning opinions.
        bool prunedByParent = false;
        for (UsdPrim curPrim = prim; curPrim; curPrim = curPrim.GetParent()) {
            // XXX: We're baking in the requirement that all instancer
            // prototypes must be nested below them here, would be nice to not
            // do this, however we would need to track yet another bit of state.
            //
            // See additional notes around instancerCachePath use below for why this
            // is needed.

            // XXX(UsdImagingPaths): Shouldn't we use a cachePath here?
            if (_instancerPrimCachePaths.find(curPrim.GetPath()) 
                    != _instancerPrimCachePaths.end()) {
                // XXX(UsdImagingPaths): Shouldn't we use a cachePath here?
                instancerCachePath = curPrim.GetPath();
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
                    usdPath.GetText(),
                    curPrim.GetPath().GetText(),
                    curPrim.GetTypeName().GetText());
                prunedByParent = true;
                break;
            }

            // XXX(UsdImagingPaths): Shouldn't we use a cachePath here?
            _HdPrimInfo *primInfo = _GetHdPrimInfo(curPrim.GetPath());

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
                                usdPath.GetText(),
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
                _HdPrimInfo *primInfo = _GetHdPrimInfo(usdPrim.GetPath());

                // Special case for adding UsdGeomSubset prims (which do not
                // get an adapter); resync the containing mesh.
                if (iter->IsA<UsdGeomSubset>()) {
                    UsdPrim parentPrim = iter->GetParent();
                    _HdPrimInfo *parentPrimInfo =
                        _GetHdPrimInfo(parentPrim.GetPath());
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

                // The prim wasn't in the _hdPrimInfoMap, this could happen
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
    SdfPathVector affectedCachePaths;
    HdPrimGather gather;

    // XXX(UsdImagingPaths): Shouldn't we use a cachePath here?
    gather.Subtree(_cachePaths.GetIds(), usdPath, &affectedCachePaths);

    if (affectedCachePaths.empty()) {
        // When we have no affected prims and all new prims were culled, the
        // instancer may still need to be notified that the child was resync'd,
        // in the event that a new prim came into existence under the root of an
        // existing prototype.
        //
        // TODO: proposes we expose an API on the adapter to query if the
        // path is of interest, which would allow the instancer (any ancestral
        // adapter) to hook in and get the event. We should do this in a future
        // change.
        if (instancerCachePath.IsEmpty()) {
            // We had no affected paths, which means the prim wasn't populated,
            // skip population below.
            return;
        } else {
            TF_DEBUG(USDIMAGING_CHANGES).Msg(
                    "  - affected instancer prim: <%s>\n",
                    instancerCachePath.GetText());

            _HdPrimInfo *primInfo = _GetHdPrimInfo(instancerCachePath);
            if (!TF_VERIFY(primInfo, "%s\n", instancerCachePath.GetText()) ||
                !TF_VERIFY(primInfo->adapter, "%s\n",
                           instancerCachePath.GetText())) {
                return;
            }

            // XXX(UsdImagingPaths): ProcessPrimResync takes a usdPath,
            // not a cachePath.
            primInfo->adapter->ProcessPrimResync(instancerCachePath, proxy);
            return;
        }
    }

    // Apply changes.
    for (SdfPath const& affectedCachePath: affectedCachePaths) {
        _HdPrimInfo *primInfo = _GetHdPrimInfo(affectedCachePath);

        TF_DEBUG(USDIMAGING_CHANGES).Msg("  - affected prim: <%s>\n",
                affectedCachePath.GetText());

        // We discovered these paths using the _hdPrimInfoMap above, this
        // method should never return a null adapter here.
        if (!TF_VERIFY(primInfo, "%s\n", affectedCachePath.GetText()) ||
            !TF_VERIFY(primInfo->adapter, "%s\n", affectedCachePath.GetText()))
            return;

        // PrimResync will:
        //  * Remove the rprim from the index, if it needs to be re-built
        //  * Schedule the prim to be repopulated
        // Note: primInfo may be invalid after this call

        // XXX(UsdImagingPaths): ProcessPrimRemoval and ProcessPrimResync
        // takes a usdPath, not a cachePath.
        if (repopulateFromRoot) {
            primInfo->adapter->ProcessPrimRemoval(affectedCachePath, proxy);
        } else {
            primInfo->adapter->ProcessPrimResync(affectedCachePath, proxy);
        }
    }

    if (repopulateFromRoot) {
        proxy->Repopulate(usdPath);
    }
}

void 
UsdImagingDelegate::_RefreshUsdObject(SdfPath const& usdPath, 
                                      TfTokenVector const& changedInfoFields,
                                      UsdImagingIndexProxy* proxy) 
{
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Refresh Object]: %s %s\n",
            usdPath.GetText(), TfStringify(changedInfoFields).c_str());

    SdfPathVector affectedCachePaths;

    // XXX(UsdImagingPaths): We need to map the usdPath to the cachePath
    // correctly here.
    SdfPath const& cachePathToRefresh = usdPath;

    if (usdPath.IsAbsoluteRootOrPrimPath()) {
        if (!_GetHdPrimInfo(cachePathToRefresh)) {
            return;
        }
        affectedCachePaths.push_back(cachePathToRefresh);
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
            TfStringStartsWith(attrName, UsdShadeTokens->materialBinding)) {
            _ResyncUsdPrim(usdPrimPath, proxy, true);
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
            // XXX(UsdImagingPaths): We need to use a cachePath here,
            // not a usdPrimPath.
            gather.Subtree(_cachePaths.GetIds(), usdPrimPath,
                           &affectedCachePaths);
        } else if (TfStringStartsWith(attrName, UsdTokens->collection)) {
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
            // No need to gather -- we know all cachePaths are affected.
            affectedCachePaths = _cachePaths.GetIds();
        } else if (TfStringStartsWith(attrName, UsdShadeTokens->coordSys)) {
            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Refresh Object]: "
                "HdCoordSys bindings affected for %s", usdPath.GetText());
            // Coordinate system bindings apply to all descendent gprims.
            _ResyncUsdPrim(usdPrimPath, proxy, true);
            return;
        } else {
            // Only include non-inherited properties for prims that we are
            // explicitly tracking in the render index.
            //
            // XXX(UsdImagingPaths): We need to use a cachePath here,
            // not a usdPrimPath.
            if (!_GetHdPrimInfo(usdPrimPath)) {
                return;
            }
            // XXX(UsdImagingPaths): We need to use a cachePath here,
            // not a usdPrimPath.
            affectedCachePaths.push_back(usdPrimPath);
        }
    }

    // PERFORMANCE: We could execute this in parallel, for large numbers of
    // prims.
    for (SdfPath const& affectedCachePath: affectedCachePaths) {

        _HdPrimInfo *primInfo = _GetHdPrimInfo(affectedCachePath);

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
            TF_VERIFY(primInfo->adapter, "%s", affectedCachePath.GetText())) {
            _AdapterSharedPtr &adapter = primInfo->adapter;

            // For the dirty bits that we've been told changed, go re-discover
            // variability and stage the associated data.
            HdDirtyBits dirtyBits = HdChangeTracker::Clean;
            if (usdPath.IsAbsoluteRootOrPrimPath()) {
                dirtyBits = adapter->ProcessPrimChange(
                    primInfo->usdPrim, affectedCachePath, changedInfoFields);
            } else if (usdPath.IsPropertyPath()) {
                dirtyBits = adapter->ProcessPropertyChange(
                    primInfo->usdPrim, affectedCachePath, usdPath.GetNameToken());
            } else {
                TF_VERIFY(false, "Unexpected path: <%s>", usdPath.GetText());
            }

            if (dirtyBits == HdChangeTracker::Clean) {
                // Do nothing
            } else if (dirtyBits != HdChangeTracker::AllDirty) {
                // Update Variability
                adapter->TrackVariabilityPrep(primInfo->usdPrim, affectedCachePath);
                adapter->TrackVariability(primInfo->usdPrim, affectedCachePath,
                                          &primInfo->timeVaryingBits);

                // Propagate the dirty bits back out to the change tracker.
                HdDirtyBits combinedBits =
                    dirtyBits | primInfo->timeVaryingBits;
                if (combinedBits != HdChangeTracker::Clean) {
                    adapter->MarkDirty(primInfo->usdPrim, affectedCachePath,
                                       combinedBits, proxy);
                }
            } else {
                // XXX(UsdImagingPaths): _ResyncUsdPrim takes a usdPath,
                // not a cachePath.
                _ResyncUsdPrim(affectedCachePath, proxy);
            }
        }
    }
}

// -------------------------------------------------------------------------- //
// Data Collection
// -------------------------------------------------------------------------- //
void
UsdImagingDelegate::_UpdateSingleValue(SdfPath const& cachePath,
                                       int requestBits)
{
    // XXX: potential race condition? UpdateSingleValue may be called from
    // multiple thread on a same path. we should probably need a guard here,
    // or in adapter
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo, "%s\n", cachePath.GetText()) &&
        TF_VERIFY(primInfo->adapter, "%s\n", cachePath.GetText())) {
        _AdapterSharedPtr &adapter = primInfo->adapter;
        adapter->UpdateForTimePrep(primInfo->usdPrim, cachePath,
                                   _time, requestBits);
        adapter->UpdateForTime(primInfo->usdPrim, cachePath,
                               _time, requestBits);
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
    // XXX(UsdImagingPaths): SetPickability takes a usdPath but we
    // use it directly as a cachePath here; should we route that through
    // a prim adapter?
    SdfPath const& cachePath = path;

    _pickablesMap[ConvertCachePathToIndexPath(cachePath)] = pickable;
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
    
    UsdImagingIndexProxy indexProxy(this, nullptr);

    // _displayGuides changes a prims render tag.
    // So we need to make sure all prims render tags get re-evaluated.
    // XXX: Should be smarted and only invalidate prims whose
    // purpose == UsdGeomTokens->guide.
    // Look at GetRenderTag for complixity with this.
    for (_HdPrimInfoMap::iterator it  = _hdPrimInfoMap.begin();
                                it != _hdPrimInfoMap.end();
                              ++it) {
        const SdfPath &cachePath = it->first;
        _HdPrimInfo &primInfo = it->second;

        if (TF_VERIFY(primInfo.adapter, "%s", cachePath.GetText())) {
            primInfo.adapter->MarkRenderTagDirty(primInfo.usdPrim,
                                                 cachePath,
                                                 &indexProxy);
        }
    }
}

void
UsdImagingDelegate::SetUsdDrawModesEnabled(bool enableUsdDrawModes)
{
    if (_enableUsdDrawModes != enableUsdDrawModes) {
        if (_hdPrimInfoMap.size() > 0) {
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
        TF_FOR_ALL(it, _hdPrimInfoMap) {
            const SdfPath &cachePath = it->first;
            _HdPrimInfo &primInfo = it->second;
            if (TF_VERIFY(primInfo.adapter, "%s", cachePath.GetText())) {
                primInfo.adapter->MarkMaterialDirty(primInfo.usdPrim,
                                                    cachePath, &indexProxy);
            }
        }
    }
}


/*virtual*/
TfToken
UsdImagingDelegate::GetRenderTag(SdfPath const& id)
{
    SdfPath cachePath = ConvertIndexPathToCachePath(id);

    // Check the purpose of the rpim
    TfToken purpose = UsdGeomTokens->default_;
    TF_VERIFY(_valueCache.FindPurpose(cachePath, &purpose), "%s", 
              cachePath.GetText());

    // If it is a property path then let's resolve it.
    // parent opinion wins if it is not default
    if (cachePath.IsPropertyPath()) {
        SdfPath cachePrimPath = cachePath.GetPrimPath();
        TfToken purposeParent = UsdGeomTokens->default_;
        TF_VERIFY(_valueCache.FindPurpose(cachePrimPath, &purposeParent), "%s", 
                  cachePrimPath.GetText());
        
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
                                cachePath.GetText(),
                                purpose.GetText());
    return purpose;
}

/*virtual*/
HdBasisCurvesTopology 
UsdImagingDelegate::GetBasisCurvesTopology(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    HdBasisCurvesTopology topology;
    VtValue tmp;

    if (_valueCache.ExtractTopology(cachePath, &tmp)) {
        return tmp.UncheckedGet<HdBasisCurvesTopology>();
    }
    _UpdateSingleValue(cachePath, HdChangeTracker::DirtyTopology);
    if (TF_VERIFY(_valueCache.ExtractTopology(cachePath, &tmp)))
        return tmp.UncheckedGet<HdBasisCurvesTopology>();

    return topology;
}

/*virtual*/
HdMeshTopology 
UsdImagingDelegate::GetMeshTopology(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    HdMeshTopology topology;
    VtValue tmp;

    if (_valueCache.ExtractTopology(cachePath, &tmp)) {
        return tmp.UncheckedGet<HdMeshTopology>();
    }
    _UpdateSingleValue(cachePath, HdChangeTracker::DirtyTopology);
    if (TF_VERIFY(_valueCache.ExtractTopology(cachePath, &tmp)))
        return tmp.UncheckedGet<HdMeshTopology>();
    
    return topology;
}

/*virtual*/
UsdImagingDelegate::SubdivTags 
UsdImagingDelegate::GetSubdivTags(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    SubdivTags tags;

    if (_valueCache.ExtractSubdivTags(cachePath, &tags)) {
        return tags;
    }
    _UpdateSingleValue(cachePath, HdChangeTracker::DirtySubdivTags);
    if (TF_VERIFY(_valueCache.ExtractSubdivTags(cachePath, &tags))) {
        return tags;
    }

    return tags;
}

/*virtual*/
GfRange3d 
UsdImagingDelegate::GetExtent(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    GfRange3d extent;
    if (_valueCache.ExtractExtent(cachePath, &extent)) {
        return extent;
    }
    // Slow path, we should not hit this.
    TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow extent fetch for %s\n", 
                               id.GetText());
    _UpdateSingleValue(cachePath, HdChangeTracker::DirtyExtent);
    TF_VERIFY(_valueCache.ExtractExtent(cachePath, &extent));
    return extent;
}

/*virtual*/ 
bool 
UsdImagingDelegate::GetDoubleSided(SdfPath const& id)
{
    bool doubleSided = false;
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    if (_valueCache.ExtractDoubleSided(cachePath, &doubleSided))
        return doubleSided; 

    _UpdateSingleValue(cachePath, HdChangeTracker::DirtyDoubleSided);
    TF_VERIFY(_valueCache.ExtractDoubleSided(cachePath, &doubleSided));
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
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    if (_valueCache.ExtractCullStyle(cachePath, &cullStyle)) {
        return cullStyle;
    }

    return _cullStyleFallback;
}

/*virtual*/ 
HdDisplayStyle 
UsdImagingDelegate::GetDisplayStyle(SdfPath const& id) 
{
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    int level = 0;
    if (TfMapLookup(_refineLevelMap, cachePath, &level))
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

    TF_FOR_ALL(it, _hdPrimInfoMap) {
        const SdfPath &cachePath = it->first;
        _HdPrimInfo &primInfo = it->second;

        // Dont mark prims with explicit refine levels as dirty.
        if (_refineLevelMap.find(cachePath) == _refineLevelMap.end()) {
            if (TF_VERIFY(primInfo.adapter, "%s", cachePath.GetText())) {
                primInfo.adapter->MarkRefineLevelDirty(primInfo.usdPrim,
                                                       cachePath,
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
        // XXX(UsdImagingPaths): _refineLevelMap is keyed by cachePath,
        // not usdPath.
        _refineLevelMap[usdPath] = level;
        // Avoid triggering changes if the new level is the same as the
        // fallback.
        if (level == _refineLevelFallback)
            return;
    }

    UsdImagingIndexProxy indexProxy(this, nullptr);

    // XXX(UsdImagingPaths): We use the usdPath directly as the cachePath
    // here, but we should consult the adapter for this.
    SdfPath const& cachePath = usdPath;
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo, "%s", cachePath.GetText()) &&
        TF_VERIFY(primInfo->adapter, "%s", cachePath.GetText())) {
        primInfo->adapter->MarkRefineLevelDirty(primInfo->usdPrim,
                                                cachePath, &indexProxy);
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

        // XXX(UsdImagingPaths): We use the usdPath directly as the cachePath
        // here, but we should consult the adapter for this.
        SdfPath const& cachePath = usdPath; // XXX ?!?
        _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
        if (TF_VERIFY(primInfo, "%s", cachePath.GetText()) &&
            TF_VERIFY(primInfo->adapter, "%s", cachePath.GetText())) {
            primInfo->adapter->MarkRefineLevelDirty(primInfo->usdPrim,
                                                    cachePath, &indexProxy);
        }
    }
}


bool
UsdImagingDelegate::IsRefined(SdfPath const& usdPath) const
{
    // XXX(UsdImagingPaths): We use the usdPath directly as the cachePath
    // here, but we should consult the adapter for this.
    SdfPath const& cachePath = usdPath; // XXX ?!?
    _RefineLevelMap::const_iterator it = _refineLevelMap.find(cachePath);
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

    TF_FOR_ALL(it, _hdPrimInfoMap) {
        const SdfPath &cachePath = it->first;
        _HdPrimInfo &primInfo = it->second;

        if (TF_VERIFY(primInfo.adapter, "%s", cachePath.GetText())) {
            primInfo.adapter->MarkReprDirty(primInfo.usdPrim,
                                            cachePath, &indexProxy);
        }
    }
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

    TF_FOR_ALL(it, _hdPrimInfoMap) {
        const SdfPath &cachePath = it->first;
        _HdPrimInfo &primInfo = it->second;

        if (TF_VERIFY(primInfo.adapter, "%s", cachePath.GetText())) {
            primInfo.adapter->MarkCullStyleDirty(primInfo.usdPrim,
                                                 cachePath, &indexProxy);
        }
    }
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
    TF_FOR_ALL(it, _hdPrimInfoMap) {
        const SdfPath &cachePath = it->first;
        _HdPrimInfo &primInfo = it->second;
        if (TF_VERIFY(primInfo.adapter, "%s", cachePath.GetText())) {
            primInfo.adapter->MarkTransformDirty(primInfo.usdPrim,
                                                 cachePath, &indexProxy);
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
        const SdfPath &usdSubtreeRoot = *changingInvisPathIt;
        const UsdPrim &usdPrim = _GetUsdPrim(usdSubtreeRoot);
        if (!usdPrim) {
            TF_CODING_ERROR("Could not find prim at path <%s>.", 
                            usdSubtreeRoot.GetText());
            continue;
        }

        TF_DEBUG(USDIMAGING_CHANGES).Msg("[Vis/Invis Prim] <%s>\n",
                                         usdSubtreeRoot.GetText());
        _MarkSubtreeVisibilityDirty(usdSubtreeRoot);
    }

    _invisedPrimPaths = invisedPaths;

    // process instance visibility.
    // this call is needed because we use _RefreshObject to repopulate
    // vis-ed/invis-ed instanced prims (accumulated in _usdPathsToUpdate)
    ApplyPendingUpdates();
}

void
UsdImagingDelegate::_MarkSubtreeVisibilityDirty(SdfPath const &usdSubtreeRoot)
{
    UsdImagingIndexProxy indexProxy(this, nullptr);

    // XXX(UsdImagingPaths): We use the usdPath directly as the cachePath
    // here, but we should do the correct mapping here instead.
    SdfPath const& cacheSubtreeRoot = usdSubtreeRoot;

    HdPrimGather gather;
    SdfPathVector affectedCachePaths;
    gather.Subtree(_cachePaths.GetIds(), cacheSubtreeRoot, &affectedCachePaths);

    // Propagate dirty bits to all descendents and outside dependent prims.
    for (SdfPath const& cachePath: affectedCachePaths) {
        _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
        if (primInfo == nullptr) {
            TF_CODING_ERROR("Prim in id list is not in prim info: %s",
                    cachePath.GetText());
            continue;
        }
        if (!TF_VERIFY(primInfo->adapter, "%s", cachePath.GetText())) {
            continue;
        }

        _AdapterSharedPtr const &adapter = primInfo->adapter;

        SdfPath instancerCachePath = adapter->GetInstancer(cachePath);
        if (!instancerCachePath.IsEmpty()) {
            // XXX: workaround for per-instance visibility in nested case.
            // testPxUsdGeomGLPopOut/test_*_5, test_*_6
            _usdPathsToResync.push_back(usdSubtreeRoot);
            return;
        } else if (_instancerPrimCachePaths.find(cachePath)
                   != _instancerPrimCachePaths.end()) {
            // XXX: workaround for per-instance visibility in nested case.
            // testPxUsdGeomGLPopOut/test_*_5, test_*_6
            _usdPathsToResync.push_back(usdSubtreeRoot);
            return;
        } else {
            adapter->MarkVisibilityDirty(primInfo->usdPrim,
                                         cachePath, &indexProxy);
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
        const UsdPrim &overridePrim = _GetUsdPrim(overridePath);

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
        UsdPrim overridePrim = _GetUsdPrim(it->first);
        overridesToRemove.push_back(overridePrim);
    }

    SdfPathVector dirtySubtreeRoots;
    _xformCache.UpdateValueOverrides(overridesToUpdate, overridesToRemove,
        &dirtySubtreeRoots);

    SdfPath::RemoveDescendentPaths(&dirtySubtreeRoots);

    // Mark dirty.
    TF_FOR_ALL(it, dirtySubtreeRoots) {
        const SdfPath &subtreeRoot = *it;
        const UsdPrim &usdPrim = _GetUsdPrim(subtreeRoot);
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

    // XXX(UsdImagingPaths): We use the usdPath directly as a cachePath
    // here.
    SdfPath const& subtreeCachePath = subtreeRoot;

    HdPrimGather gather;
    SdfPathVector affectedCachePaths;
    gather.Subtree(_cachePaths.GetIds(), subtreeCachePath, &affectedCachePaths);

    // Propagate dirty bits to all descendents and outside dependent prims.
    for (SdfPath const& cachePath: affectedCachePaths) {
        _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
        if (primInfo == nullptr) {
            TF_CODING_ERROR("Prim in id list is not in prim info: %s",
                            cachePath.GetText());
            continue;
        }
        if (!TF_VERIFY(primInfo->adapter, "%s", cachePath.GetText())) {
            continue;
        }

        _AdapterSharedPtr const &adapter = primInfo->adapter;

        SdfPath instancerCachePath = adapter->GetInstancer(cachePath);
        if (!instancerCachePath.IsEmpty()) {
            _HdPrimInfo *instancerInfo = _GetHdPrimInfo(instancerCachePath);
            if (!TF_VERIFY(instancerInfo, "%s", cachePath.GetText()) ||
                !TF_VERIFY(instancerInfo->adapter, "%s", cachePath.GetText())) {
                continue;
            }

            // redirect to native instancer.
            instancerInfo->adapter->MarkTransformDirty(instancerInfo->usdPrim,
                                                       instancerCachePath,
                                                       &indexProxy);

            // also communicate adapter to get the list of instanced proto prims
            // to be marked as dirty. for those are not in the namespace children
            // of the instancer (needed for NI-PI cases).
            SdfPathVector const &paths =
                adapter->GetDependPaths(instancerCachePath);
            TF_FOR_ALL (instIt, paths) {
                // recurse
                _MarkSubtreeTransformDirty(*instIt);
            }
        } else if (_instancerPrimCachePaths.find(cachePath)
                   != _instancerPrimCachePaths.end()) {

            // instancer itself
            adapter->MarkTransformDirty(primInfo->usdPrim,
                                        cachePath,
                                        &indexProxy);

            // also communicate adapter to get the list of instanced proto prims
            // to be marked as dirty. for those are not in the namespace children
            // of the instancer.
            SdfPathVector const &paths = adapter->GetDependPaths(cachePath);
            TF_FOR_ALL (instIt, paths) {
                // recurse
                _MarkSubtreeTransformDirty(*instIt);
            }
        } else {
            adapter->MarkTransformDirty(primInfo->usdPrim,
                                        cachePath,
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

    TF_FOR_ALL(it, _hdPrimInfoMap) {
        const SdfPath &cachePath = it->first;
        _HdPrimInfo &primInfo = it->second;

        if (TF_VERIFY(primInfo.adapter, "%s", cachePath.GetText())) {
            primInfo.adapter->MarkVisibilityDirty(primInfo.usdPrim,
                                                  cachePath,
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
    SdfPath cachePath = ConvertIndexPathToCachePath(protoPrimPath);

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "GetPathForInstanceIndex(%s, %d)\n",
        cachePath.GetText(), instanceIndex);

    // resolve all instancer hierarchy.
    int instanceCount = 0;
    int protoInstanceIndex = instanceIndex;
    int absIndex = ALL_INSTANCES; // PointInstancer may overwrite.
    SdfPathVector resolvedInstanceContext;
    SdfPath resolvedRprimPath;
    do {
        _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
        if (!TF_VERIFY(primInfo, "%s\n", cachePath.GetText()) ||
            !TF_VERIFY(primInfo->adapter, "%s\n", cachePath.GetText())) {
            return ConvertCachePathToIndexPath(cachePath);
        }

        _AdapterSharedPtr const& adapter = primInfo->adapter;
        cachePath = adapter->GetPathForInstanceIndex(
            cachePath, instanceIndex, &instanceCount, &absIndex, 
            &resolvedRprimPath, &resolvedInstanceContext);

        if (cachePath.IsEmpty()) {
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
        cachePath.GetText(), absIndex,
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

    return ConvertCachePathToIndexPath(cachePath);
}

bool
UsdImagingDelegate::PopulateSelection(
              HdSelection::HighlightMode const& highlightMode,
              SdfPath const &indexPath,
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
    SdfPath cachePath = ConvertIndexPathToCachePath(indexPath);
    // Since it is technically possible to call PopulateSelection() before
    // Populate(), we guard access to _stage.  Ideally this would be a TF_VERIFY
    // but some clients need to be fixed first.
    if (_stage) {
        // XXX(UsdImagingPaths): Using cachePath directly as USD path here;
        // we should do the correct mapping.
        SdfPath const& usdPath = cachePath;
        UsdPrim usdPrim = _stage->GetPrimAtPath(usdPath);

        // Should not need to check for pseudoroot since it can never be 
        // an instance proxy
        while (usdPrim && usdPrim.IsInstanceProxy()){
            usdPrim = usdPrim.GetParent();
        }
        if (usdPrim){
            // XXX(UsdImaging): We are using a usdPath directly as a cachePath
            // here; should we do a proper transformation?
            cachePath = usdPrim.GetPath();
        }
    }
    
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);

    bool added = false;

    // UsdImagingDelegate only supports top-most level per-instance highlighting
    VtIntArray instanceIndices;
    if (instanceIndex != ALL_INSTANCES) {
        instanceIndices.push_back(instanceIndex);
    }

    if (primInfo && TF_VERIFY(primInfo->adapter, "%s\n", cachePath.GetText())) {
        _AdapterSharedPtr const& adapter = primInfo->adapter;

        // Prim, or instancer
        return adapter->PopulateSelection(highlightMode, cachePath,
                                          instanceIndices, result);
    } else {

        // Select prims that are part of the path subtree. Exclude prototypes
        // since they are handled by their instancers' PopulateSelection calls.
        SdfPathVector affectedCachePaths;
        HdPrimGather gather;
        gather.Subtree(_cachePaths.GetIds(), cachePath, &affectedCachePaths);

        for (SdfPath const& cachePath: affectedCachePaths) {
            _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
            if (primInfo == nullptr) {
                TF_CODING_ERROR("Prim in usd ids is not in prim info: %s",
                                cachePath.GetText());
                continue;
            }
            if (!TF_VERIFY(primInfo->adapter, "%s\n", cachePath.GetText())) {
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
            if (cachePath.IsPropertyPath()) {
                continue;
            }
            added |= adapter->PopulateSelection(highlightMode, cachePath,
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

    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    GfMatrix4d ctm(1.0);
    if (_valueCache.ExtractTransform(cachePath, &ctm)) {
        return ctm;
    }
    // Slow path, we should not hit this.
    TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow transform fetch for %s\n", 
                               id.GetText());

    _UpdateSingleValue(cachePath, HdChangeTracker::DirtyTransform);
    TF_VERIFY(_valueCache.ExtractTransform(cachePath, &ctm));
    return ctm;
}

/*virtual*/
size_t
UsdImagingDelegate::SampleTransform(SdfPath const & id, size_t maxNumSamples,
                                    float *times, GfMatrix4d *samples)
{
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->SampleTransform(primInfo->usdPrim, cachePath,
                              _timeSampleOffsets, maxNumSamples,
                              times, samples);
    }
    return 0;
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

    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    // for instance protos (not IsPrimPath), visibility is
    // controlled by instanceIndices.
    if (cachePath.IsPrimPath() && IsInInvisedPaths(cachePath)) {
        return false;
    }

    bool vis = true;
    if (_valueCache.FindVisible(cachePath, &vis)) {
        return vis;
    }

    // Slow path, we should not hit this.
    TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow visible fetch for %s\n", 
                               id.GetText());

    _UpdateSingleValue(cachePath, HdChangeTracker::DirtyVisibility);
    if (TF_VERIFY(_valueCache.ExtractVisible(cachePath, &vis),
                "<%s>\n", cachePath.GetText())) {
        return vis;
    }
    return false;
}

/*virtual*/ 
VtValue 
UsdImagingDelegate::Get(SdfPath const& id, TfToken const& key)
{
    HD_TRACE_FUNCTION();

    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    VtValue value;

    if (!_valueCache.ExtractPrimvar(cachePath, key, &value)) {
        if (key == HdTokens->points) {
            _UpdateSingleValue(cachePath,HdChangeTracker::DirtyPoints);
            if (!TF_VERIFY(_valueCache.ExtractPoints(cachePath, &value))) {
                VtVec3fArray vec;
                value = VtValue(vec);
            }
        } else if (key == HdTokens->displayColor) {
            // XXX: Getting all primvars here when we only want color is wrong.
            _UpdateSingleValue(cachePath,HdChangeTracker::DirtyPrimvar);
            if (!TF_VERIFY(_valueCache.ExtractColor(cachePath, &value))){
                VtVec3fArray vec(1);
                vec.push_back(GfVec3f(.5,.5,.5));
                value = VtValue(vec);
            }
        } else if (key == HdTokens->displayOpacity) {
            // XXX: Getting all primvars here when we only want opacity is bad.
            _UpdateSingleValue(cachePath,HdChangeTracker::DirtyPrimvar);
            if (!TF_VERIFY(_valueCache.ExtractOpacity(cachePath, &value))){
                VtFloatArray vec(1);
                vec.push_back(1.0f);
                value = VtValue(vec);
            }
        } else if (key == HdTokens->widths) {
            _UpdateSingleValue(cachePath,HdChangeTracker::DirtyWidths);
            if (!TF_VERIFY(_valueCache.ExtractWidths(cachePath, &value))){
                VtFloatArray vec(1);
                vec.push_back(1.0f);
                value = VtValue(vec);
            }
        } else if (key == HdTokens->transform) {
            // XXX(UsdImaging): We use cachePath directly as usdPath here
            // but should do the proper transformation.  Maybe we can use
            // the primInfo.usdPrim?
            SdfPath const& usdPath = cachePath;
            value = VtValue(
                UsdImaging_XfStrategy::ComputeTransform( _GetUsdPrim(usdPath),
                    _rootPrimPath, GetTime(), _rigidXformOverrides) * _rootXf);
        } else if (UsdGeomPrimvar pv = UsdGeomGprim(_GetUsdPrim(cachePath))
                                                .GetPrimvar(key)) {
            // XXX(UsdImaging): We use cachePath directly as usdPath above,
            // but should do the proper transformation.  Maybe we can use
            // the primInfo.usdPrim?

            // Note here that Hydra requested "color" (e.g.) and we've converted
            // it to primvars:color automatically by virtue of UsdGeomPrimvar.
            TF_VERIFY(pv.ComputeFlattened(&value, _time), "%s, %s\n", 
                      id.GetText(), key.GetText());
        } else {
            // XXX: This does not work for point instancer child prims; while we
            // do not hit this code path given the current state of the
            // universe, we need to rethink UsdImagingDelegate::Get().
            //
            // XXX(UsdImaging): We use cachePath directly as usdPath here,
            // but should do the proper transformation.  Maybe we can use
            // the primInfo.usdPrim?
            TF_VERIFY(_GetUsdPrim(cachePath).GetAttribute(key)
                      .Get(&value, _time),
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

HdIdVectorSharedPtr
UsdImagingDelegate::GetCoordSysBindings(SdfPath const& id)
{
    if (!_coordSysEnabled) {
        return nullptr;
    }
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (!TF_VERIFY(primInfo) || !TF_VERIFY(primInfo->usdPrim)) {
        return nullptr;
    }
    return _coordSysBindingCache.GetValue(primInfo->usdPrim).idVecPtr;
}

/*virtual*/
size_t
UsdImagingDelegate::SamplePrimvar(SdfPath const& id, TfToken const& key,
                                  size_t maxNumSamples,
                                  float *times, VtValue *samples)
{
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->SamplePrimvar(primInfo->usdPrim, cachePath, key,
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
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    return _collectionCache.ComputeCollectionsContainingPath(cachePath);
}

/*virtual*/
std::vector<VtArray<TfToken>>
UsdImagingDelegate::GetInstanceCategories(SdfPath const &instancerId)
{
    SdfPath cachePath = ConvertIndexPathToCachePath(instancerId);
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->GetInstanceCategories(primInfo->usdPrim);
    }
    return std::vector<VtArray<TfToken>>();
}

// -------------------------------------------------------------------------- //
// Primvar Support Methods
// -------------------------------------------------------------------------- //

HdPrimvarDescriptorVector
UsdImagingDelegate::GetPrimvarDescriptors(SdfPath const& id,
                                          HdInterpolation interpolation)
{
    HD_TRACE_FUNCTION();
    SdfPath cachePath = ConvertIndexPathToCachePath(id);
    HdPrimvarDescriptorVector primvars;
    HdPrimvarDescriptorVector allPrimvars;
    // We expect to populate an entry always (i.e., we don't use a slow path
    // fetch)
    if (!TF_VERIFY(_valueCache.FindPrimvars(cachePath, &allPrimvars), 
                   "<%s> interpolation: %s", cachePath.GetText(),
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

    SdfPath cachePath = ConvertIndexPathToCachePath(prototypeId);
    VtValue indices;

    // TODO: it would be nice to only call Find on instancers and call Extract
    // otherwise, however we have no way of making that distinction currently.
    if (!_valueCache.FindInstanceIndices(cachePath, &indices)) {
        // Slow path, we should not hit this.
        TF_DEBUG(HD_SAFE_MODE).Msg(
                                "WARNING: Slow instance indices fetch for %s\n", 
                                prototypeId.GetText());
        _UpdateSingleValue(cachePath, HdChangeTracker::DirtyInstanceIndex);
        TF_VERIFY(_valueCache.FindInstanceIndices(cachePath, &indices));
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

    SdfPath cachePath = ConvertIndexPathToCachePath(instancerId);
    GfMatrix4d ctm(1.0);

    // same as GetInstanceIndices, the instancer transform may be
    // asked multiple times for all prototypes. use Find instead of Extract
    // to preserve the result for further lookup.

    if (!_valueCache.FindInstancerTransform(cachePath, &ctm)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow instancer transform fetch for %s\n", 
            instancerId.GetText());
        _UpdateSingleValue(cachePath, HdChangeTracker::DirtyTransform);
        TF_VERIFY(_valueCache.FindInstancerTransform(cachePath, &ctm));
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
    SdfPath cachePath = ConvertIndexPathToCachePath(instancerId);
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->SampleInstancerTransform(primInfo->usdPrim, cachePath,
                                       _time, _timeSampleOffsets,
                                       maxSampleCount, times, samples);
    }
    return 0;
}

/*virtual*/ 
SdfPath 
UsdImagingDelegate::GetMaterialId(SdfPath const &rprimId)
{
    SdfPath cachePath = ConvertIndexPathToCachePath(rprimId);
    SdfPath pathValue;
    if (!_valueCache.ExtractMaterialId(cachePath, &pathValue)) {
        _UpdateSingleValue(cachePath, HdChangeTracker::DirtyMaterialId);
        TF_VERIFY(_valueCache.ExtractMaterialId(cachePath, &pathValue));
    }
    return ConvertCachePathToIndexPath(pathValue);
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

    SdfPath cachePath = ConvertIndexPathToCachePath(materialId);
    std::string source;

    if (!_valueCache.ExtractSurfaceShaderSource(cachePath, &source)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow surface shader source fetch for %s\n",
            materialId.GetText());
        _UpdateSingleValue(cachePath, HdMaterial::DirtySurfaceShader);
        TF_VERIFY(_valueCache.ExtractSurfaceShaderSource(cachePath, &source));
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

    SdfPath cachePath = ConvertIndexPathToCachePath(materialId);
    std::string source;

    if (!_valueCache.ExtractDisplacementShaderSource(cachePath, &source)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow displacement shader source fetch for %s\n",
            materialId.GetText());
        _UpdateSingleValue(cachePath, HdMaterial::DirtySurfaceShader);
        TF_VERIFY(_valueCache.ExtractDisplacementShaderSource(
                  cachePath, &source));
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

    SdfPath cachePath = ConvertIndexPathToCachePath(materialId);
    VtValue param;

    // XXX: See comment in GetMaterialParams.
    TF_VERIFY(_valueCache.ExtractMaterialParam(cachePath, paramName, &param));

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


    SdfPath cachePath = ConvertIndexPathToCachePath(materialId);
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

    _UpdateSingleValue(cachePath, HdMaterial::DirtyParams);
    TF_VERIFY(_valueCache.FindMaterialParams(cachePath, &params));

    // Connections need to be represented as index paths...
    TF_FOR_ALL(paramIt, params) {
        if (paramIt->IsTexture()) {
            // Unfortunately, HdMaterialParam is immutable;
            // fortunately, it has relatively lightweight members.
            *paramIt = HdMaterialParam(
                HdMaterialParam::ParamTypeTexture,
                paramIt->GetName(),
                paramIt->GetFallbackValue(),
                ConvertCachePathToIndexPath(paramIt->GetConnection()),
                paramIt->GetSamplerCoordinates(),
                paramIt->GetTextureType());
        }
    }

    return params;
}

HdTextureResource::ID
UsdImagingDelegate::GetTextureResourceID(SdfPath const &textureId)
{
    SdfPath cachePath = ConvertIndexPathToCachePath(textureId);
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (primInfo) {
        return primInfo->adapter
            ->GetTextureResourceID(primInfo->usdPrim, cachePath, _time,
                                   (size_t) &GetRenderIndex() );
    } 

    // A bad asset can cause _GetHdPrimInfo() to fail. Hence, issue a warning and 
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
    SdfPath cachePath = ConvertIndexPathToCachePath(textureId);
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->GetTextureResource(primInfo->usdPrim, cachePath, _time);
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

    SdfPath cachePath = ConvertIndexPathToCachePath(id);

    // XXX(UsdImaging): We use the cachePath directly as a usdPath here
    // but should do the proper transformation.  Maybe we can use
    // the primInfo.usdPrim
    UsdPrim prim = _GetUsdPrim(cachePath);
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
    SdfPath cachePath = ConvertIndexPathToCachePath(volumeId);
    _HdPrimInfo *primInfo = _GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo)) {
        return primInfo->adapter
            ->GetVolumeFieldDescriptors(primInfo->usdPrim, cachePath, _time);
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

    SdfPath cachePath = ConvertIndexPathToCachePath(materialId);
    _UpdateSingleValue(cachePath, HdMaterial::DirtyResource);
    TF_VERIFY(_valueCache.FindMaterialResource(cachePath, &vtMatResource));
    return vtMatResource;
}

TfTokenVector 
UsdImagingDelegate::GetMaterialPrimvars(SdfPath const &materialId)
{
    if (!TF_VERIFY(materialId != SdfPath())) {
        return TfTokenVector();
    }

    SdfPath cachePath = ConvertIndexPathToCachePath(materialId);
    TfTokenVector materialPrimvars;
    _valueCache.FindMaterialPrimvars(cachePath, &materialPrimvars);

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

    SdfPath cachePath = ConvertIndexPathToCachePath(materialId);
    VtValue value;

    if (!_valueCache.ExtractMaterialMetadata(cachePath, &value)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow material metadata fetch for %s\n",
            materialId.GetText());
        // MaterialMetadata updates along with DirtySurfaceShader
        _UpdateSingleValue(cachePath, HdMaterial::DirtySurfaceShader);
        TF_VERIFY(_valueCache.ExtractMaterialMetadata(cachePath, &value));
    }

    return value.GetWithDefault<VtDictionary>();
}

TfTokenVector
UsdImagingDelegate::GetExtComputationSceneInputNames(
    SdfPath const& computationId)
{
    HD_TRACE_FUNCTION();

    SdfPath cachePath = ConvertIndexPathToCachePath(computationId);

    TfTokenVector inputNames;
    if (!_valueCache.ExtractExtComputationSceneInputNames(
            cachePath, &inputNames)) {

        TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow extComputation input "
                                   "descriptor fetch for %s\n", 
                                   computationId.GetText());
        
        _UpdateSingleValue(cachePath, HdExtComputation::DirtyInputDesc);
        TF_VERIFY(_valueCache.ExtractExtComputationSceneInputNames(
                      cachePath, &inputNames));
    }

    return inputNames;
}

HdExtComputationInputDescriptorVector
UsdImagingDelegate::GetExtComputationInputDescriptors(
    SdfPath const& computationId)
{
    HD_TRACE_FUNCTION();

    SdfPath cachePath = ConvertIndexPathToCachePath(computationId);

    HdExtComputationInputDescriptorVector inputs;
    if (!_valueCache.ExtractExtComputationInputs(cachePath, &inputs)) {

        TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow extComputation input "
                                   "descriptor fetch for %s\n", 
                                   computationId.GetText());
        
        _UpdateSingleValue(cachePath, HdExtComputation::DirtyInputDesc);
        TF_VERIFY(_valueCache.ExtractExtComputationInputs(cachePath, &inputs));
    }

    return inputs;
}

HdExtComputationOutputDescriptorVector
UsdImagingDelegate::GetExtComputationOutputDescriptors(
    SdfPath const& computationId)
{
    HD_TRACE_FUNCTION();

    SdfPath cachePath = ConvertIndexPathToCachePath(computationId);

    HdExtComputationOutputDescriptorVector outputs;
    if (!_valueCache.ExtractExtComputationOutputs(cachePath, &outputs)) {

        TF_DEBUG(HD_SAFE_MODE).Msg("WARNING: Slow extComputation output "
                                   "descriptor fetch for %s\n", 
                                   computationId.GetText());
        
        _UpdateSingleValue(cachePath, HdExtComputation::DirtyOutputDesc);
        TF_VERIFY(_valueCache.ExtractExtComputationOutputs(
            cachePath, &outputs));
    }

    return outputs;
}

HdExtComputationPrimvarDescriptorVector
UsdImagingDelegate::GetExtComputationPrimvarDescriptors(
    SdfPath const& computationId,
    HdInterpolation interpolation)
{
    HD_TRACE_FUNCTION();
    SdfPath cachePath = ConvertIndexPathToCachePath(computationId);

    HdExtComputationPrimvarDescriptorVector allPrimvars;
    // We don't require an entry to be populated.
    _valueCache.FindExtComputationPrimvars(cachePath, &allPrimvars);
    
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
    SdfPath cachePath = ConvertIndexPathToCachePath(computationId);
    VtValue value;

    if (!_valueCache.ExtractExtComputationInput(cachePath, input, &value)) {
        TF_DEBUG(HD_SAFE_MODE).Msg(
            "WARNING: Slow fetch for token %s for computation %s\n", 
                            input.GetText(), computationId.GetText());
        if (input == HdTokens->dispatchCount) {
            _UpdateSingleValue(cachePath, HdExtComputation::DirtyDispatchCount);
        } else if (input == HdTokens->elementCount) {
            _UpdateSingleValue(cachePath, HdExtComputation::DirtyElementCount);
        } else {
            _UpdateSingleValue(cachePath, HdExtComputation::DirtySceneInput);
        }

        TF_VERIFY(_valueCache.ExtractExtComputationInput(cachePath, input, 
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

        SdfPath cachePath = ConvertIndexPathToCachePath(computationId);
        if (!_valueCache.ExtractExtComputationKernel(cachePath, &kernel)) {
            TF_DEBUG(HD_SAFE_MODE).Msg(
                "WARNING: Slow extComputation kernel fetch for %s\n",
                computationId.GetText());
            _UpdateSingleValue(cachePath, HdExtComputation::DirtyKernel);
            TF_VERIFY(_valueCache.ExtractExtComputationKernel(
                          cachePath, &kernel));
        }
    }
    return kernel;
}

void
UsdImagingDelegate::InvokeExtComputation(SdfPath const& computationId,
                                         HdExtComputationContext *context)
{
    _HdPrimInfo *primInfo = _GetHdPrimInfo(computationId);

    _HdPrimInfoMap::iterator it = _hdPrimInfoMap.find(computationId);
    if (TF_VERIFY(primInfo, "%s\n", computationId.GetText()) &&
        TF_VERIFY(primInfo->adapter, "%s\n", computationId.GetText())) {
        primInfo->adapter->InvokeComputation(computationId, context);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

