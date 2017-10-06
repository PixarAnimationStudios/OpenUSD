//
// Copyright 2017 Pixar
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
#include "gusd/stageCache.h"

#include <DEP/DEP_MicroNode.h>
#include <UI/UI_Object.h>
#include <UT/UT_ConcurrentHashMap.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Lock.h>
#include <UT/UT_ParallelUtil.h>
#include <UT/UT_RWLock.h>
#include <UT/UT_StringHolder.h>
#include <UT/UT_StringSet.h>
#include <UT/UT_Thread.h>
#include <UT/UT_WorkBuffer.h>

#include "gusd/USD_DataCache.h"
#include "gusd/UT_Error.h"

#include "pxr/base/tf/notice.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stagePopulationMask.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

namespace {


/// Micro node that dirties itself based on Tf
/// change notifications on a USD stage.
class _StageChangeMicroNode final : public DEP_MicroNode,
                                    public TfWeakBase // Required for TfNotice
{
public:

    _StageChangeMicroNode(const UsdStagePtr& stage)
        : DEP_MicroNode(), TfWeakBase(),
        _identifier(stage->GetRootLayer()->GetIdentifier())
    {
        _noticeKey = TfNotice::Register(
            TfCreateWeakPtr(this),
            &_StageChangeMicroNode::_HandleStageDidChange, stage);
    }

    virtual ~_StageChangeMicroNode()
    {
        TfNotice::Revoke(_noticeKey);
    }

    /// Propagate dirty state to outputs.
    /// This is unsafe outside of the main event queue.
    void SetDirty()
    {
        // Dirty propagation is not thread safe.
        // This should only occur on the main event queue, as happens
        // with stage reloads on the GusdStageCache.
        if(UT_Thread::isMainThread()) {
            propagateDirty([](const DEP_MicroNode& node,
                              const DEP_MicroNode& src) {});
        } else {
            TF_WARN("Change notification received for stage @%s@ outside of "
                    "the main event queue. This may indicate unsafe mutation "
                    "of stages owned by the GusdUsdStageCache.",
                    _identifier.c_str());
        }
    }

private:

    void
    _HandleStageDidChange(const UsdNotice::StageContentsChanged& n)
    {
        SetDirty();
    }

private:
    TfNotice::Key   _noticeKey;
    std::string     _identifier;
};


/// UI object that serves as an event queue for stage changes that are unsafe
/// outside of the main event queue.
class _EventQueue final : public UI_Object
{
public:
    static _EventQueue&  GetInstance();

    _EventQueue() : UI_Object() {}
    virtual ~_EventQueue();

    /// Event handler, called on the event queue.
    /// This does the actual work of processing changes.
    virtual void        handleEvent(UI_Event* event) override;

    virtual const char* className() const override
                        { return "GusdStageCache::_EventQueue"; }

    /// Add stages to the reload queue.
    void                ReloadStages(const UT_Set<UsdStagePtr>& stages);

    /// Add layers to the reload queue.
    void                ReloadLayers(const UT_Set<SdfLayerHandle>& layers);
    
    /// Add a micro node that was evicted from the cache.
    /// The event queue takes ownership of the node.
    void                AppendEvictedNode(_StageChangeMicroNode* node);

private:
    /// Mark the state dirty, setting an event on the event queue to
    /// process reloads.
    void                _MarkDirty();

private:
    bool                                _dirty;
    UT_Lock                             _lock;
    std::set<UsdStagePtr>               _stagesToReload;
    std::set<SdfLayerHandle>            _layersToReload;
    std::set<_StageChangeMicroNode*>    _evictedNodes;
};


_EventQueue::~_EventQueue()
{
    for(auto* microNode : _evictedNodes)
        delete microNode;
}


_EventQueue&
_EventQueue::GetInstance()
{
    static _EventQueue queue;
    return queue;
}


void
_EventQueue::_MarkDirty()
{
    if(!_dirty) {
        // Haven't queued up an event yet. Send one.
        generateEvent(UI_EVENT_REFRESH, this);
    }
    _dirty = true;
}


void
_EventQueue::handleEvent(UI_Event* event)
{
    UT_AutoLock lock(_lock);

    if(_dirty) {
        {
            // Use a change block so that layer change
            // notifications will be batch-processed by stages.
            SdfChangeBlock changeBlock;
            for(const auto& layer : _layersToReload) {
                if(layer)
                    layer->Reload();
            }
            _layersToReload.clear();
        }
        for(const auto& stage : _stagesToReload) {
            if(stage)
                stage->Reload();
        }
        _stagesToReload.clear();

        for(auto* microNode : _evictedNodes) {
            UT_ASSERT_P(microNode);
            microNode->SetDirty();

            // The event queue takes ownership of the micro node. Kill it.
            delete microNode;
        }
        _evictedNodes.clear();

        _dirty = false;
    }
}


void
_EventQueue::ReloadLayers(const UT_Set<SdfLayerHandle>& layers)
{
    if(layers.size() > 0) {
        UT_AutoLock lock(_lock);
        _layersToReload.insert(layers.begin(), layers.end());
        _MarkDirty();
    }
}


void
_EventQueue::ReloadStages(const UT_Set<UsdStagePtr>& stages)
{
    if(stages.size() > 0) {
        UT_AutoLock lock(_lock);
        _stagesToReload.insert(stages.begin(), stages.end());
        _MarkDirty();
    }
}


void
_EventQueue::AppendEvictedNode(_StageChangeMicroNode* node)
{
    UT_ASSERT_P(node);
    UT_AutoLock lock(_lock);
    _evictedNodes.insert(node);
    _MarkDirty();
}


} /*namespace*/


void
GusdStageCache::ReloadStages(const UT_Set<UsdStagePtr>& stages)
{
    _EventQueue::GetInstance().ReloadStages(stages);
}


void
GusdStageCache::ReloadLayers(const UT_Set<SdfLayerHandle>& layers)
{
    _EventQueue::GetInstance().ReloadLayers(layers);
}


namespace {


template <typename T>
bool
_PointerTypesMatch(const T& a, const T& b)
{
    if(a == b)
        return true;
    if(a && b)
        return *a == *b;
    return false;
}


/// Key for a looking up a stage for a set of layers and load opts.
struct _StageKey
{
    _StageKey() = default;
    _StageKey(const UT_StringHolder& path,
              const GusdStageOpts& opts,
              const GusdStageEditPtr& edit)
        : _path(path), _opts(opts), _edit(edit) {}

    const UT_StringHolder&  GetPath() const { return _path; }
    const GusdStageOpts&    GetOpts() const { return _opts; }
    const GusdStageEditPtr& GetEdit() const { return _edit; }

private:
    UT_StringHolder     _path;
    GusdStageOpts       _opts;
    GusdStageEditPtr    _edit;
};


struct _StageKeyHashCmp
{
    static bool equal(const _StageKey& a, const _StageKey& b)
                {
                    return a.GetPath() == b.GetPath() &&
                           a.GetOpts() == b.GetOpts() &&
                           _PointerTypesMatch(a.GetEdit(), b.GetEdit());
                }

    static bool hash(const _StageKey& key)
                {
                    size_t hash = SYShash(key.GetPath());
                    SYShashCombine(hash, key.GetOpts().GetHash());
                    if(key.GetEdit())
                        SYShashCombine(hash, key.GetEdit()->GetHash());
                    return hash;
                }
};


struct _SdfPathHashCmp
{
    static bool     equal(const SdfPath& a, const SdfPath& b)
                    { return a == b; }

    static size_t   hash(const SdfPath& path)
                    { return path.GetHash(); }
};


/// Cache holding stages for different sets of masked prims.
/// These caches are created for a common set of stage options.
class _MaskedStageCache
{
public:
    _MaskedStageCache(GusdStageCache::_Impl& stageCache, const _StageKey& key)
        : _stageCache(stageCache), _stageKey(key) {}

    UsdStageRefPtr  FindStage(const SdfPath& primPath);

    UsdStageRefPtr  FindOrOpenStage(const SdfPath& primPath,
                                    GusdUT_ErrorContext* err=nullptr);

    void            Clear() { _map.clear(); }
    
    /// Append all stages held by this cache to @a stages.
    void            GetStages(UT_Set<UsdStageRefPtr>& stages) const
                    {
                        for(const auto& pair : _map)
                            stages.insert(pair.second);
                    }

private:
    using _StageMap = UT_ConcurrentHashMap<SdfPath,UsdStageRefPtr,
                                           _SdfPathHashCmp>;

    GusdStageCache::_Impl&  _stageCache;
    _StageMap               _map;
    const _StageKey         _stageKey;
};


} /*namespace*/


/// Primary internal cache implementation.
class GusdStageCache::_Impl
{
public:
    ~_Impl() { Clear(); }

    UT_RWLock&      GetMapLock()    { return _mapLock; }

    /// Methods accessible to GusdStageCacheReader.
    /// These require only a shared lock to the stage.

    UsdStageRefPtr  OpenNewStage(const UT_StringRef& path,
                                 const GusdStageOpts& opts,
                                 const GusdStageEditPtr& edit,
                                 const UsdStagePopulationMask* mask,
                                 GusdUT_ErrorContext* err=nullptr);

    SdfLayerRefPtr  CreateSessionLayer(const GusdStageEdit& edit,
                                       GusdUT_ErrorContext* err=nullptr);

    SdfLayerRefPtr  FindOrOpenLayer(const UT_StringRef& path,
                                    GusdUT_ErrorContext* err=nullptr);

    UsdStageRefPtr  FindStage(const UT_StringRef& path,
                              const GusdStageOpts& opts,
                              const GusdStageEditPtr& edit) const;

    UsdStageRefPtr  FindOrOpenStage(const UT_StringRef& path,
                                    const GusdStageOpts& opts,
                                    const GusdStageEditPtr& edit,
                                    GusdUT_ErrorContext* err=nullptr);

    UsdStageRefPtr  FindMaskedStage(const UT_StringRef& path,
                                    const GusdStageOpts& opts,
                                    const GusdStageEditPtr& edit,
                                    const SdfPath& primPath);

    UsdStageRefPtr  FindOrOpenMaskedStage(const UT_StringRef& path,
                                          const GusdStageOpts& opts,
                                          const GusdStageEditPtr& edit,
                                          const SdfPath& primPath,
                                          GusdUT_ErrorContext* err=nullptr);

    DEP_MicroNode*  FindStageMicroNode(const UsdStagePtr& stage);

    DEP_MicroNode*  GetStageMicroNode(const UsdStagePtr& stage);


    /// Methods accessible to GusdStageCacheWriter.
    /// These require an exclusive lock to the stage.

    void            Clear();
    void            Clear(const UT_StringSet& paths);

    void            AddDataCache(GusdUSD_DataCache& cache)
                    {
                        UT_AutoLock lock(_dataCacheLock);
                        _dataCaches.append(&cache);
                    }
                        
    void            RemoveDataCache(GusdUSD_DataCache& cache)   
                    {
                        UT_AutoLock lock(_dataCacheLock);
                        exint idx = _dataCaches.find(&cache);
                        if(idx >= 0)
                            _dataCaches.removeIndex(idx);
                    }

    void            FindStages(const UT_StringSet& paths,
                               UT_Set<UsdStageRefPtr>& stages) const;

protected:
    /// Expand the set of masked prims on a stage.
    void            _ExpandStageMask(UsdStageRefPtr& stage);

private:
    using _StageMap = UT_ConcurrentHashMap<_StageKey,UsdStageRefPtr,
                                           _StageKeyHashCmp>;

    using _MaskedStageCacheMap = UT_ConcurrentHashMap<_StageKey,
                                                      _MaskedStageCache*,
                                                      _StageKeyHashCmp>;

    struct _StageHashCmp
    {
        static bool equal(const UsdStagePtr& a, const UsdStagePtr& b)
                    { return a == b; }

        static bool hash(const UsdStagePtr& stage)
                    { return SYShash(stage); }
    };


    using _MicroNodeMap = UT_ConcurrentHashMap<UsdStagePtr,
                                               _StageChangeMicroNode*,
                                               _StageHashCmp>;

    /// Mutex around the concurrent maps.
    /// An exclusive lock must be acquired when iterating over the maps.
    UT_RWLock   _mapLock;

    /// Data cache mutex.
    /// Must be acquired when accessing data caches in any way.  
    UT_Lock     _dataCacheLock;

    /// Cache of stages without any masks.
    _StageMap   _stageMap;
    /// Cache of sub-caches for masked stages.
    _MaskedStageCacheMap _maskedCacheMap;

    /// Cache of micro nodes for layers (created on request only).
    _MicroNodeMap _microNodeMap;
    
    UT_Array<GusdUSD_DataCache*> _dataCaches;
};


UsdStageRefPtr
GusdStageCache::_Impl::OpenNewStage(const UT_StringRef& path,
                                    const GusdStageOpts& opts,
                                    const GusdStageEditPtr& edit,
                                    const UsdStagePopulationMask* mask, 
                                    GusdUT_ErrorContext* err)
{
    // Catch Tf errors.
    GusdUT_TfErrorScope errorScope(err);

    // The root layer is shared, and not modified.
    if(SdfLayerRefPtr rootLayer = FindOrOpenLayer(path, err)) {

        // Need a unique session layer on which to apply any edits.
        SdfLayerRefPtr sessionLayer;
        if(edit) {
            sessionLayer = CreateSessionLayer(*edit);
            if(!sessionLayer)
                return TfNullPtr;
        }

        UsdStageRefPtr stage =
            mask ? UsdStage::OpenMasked(rootLayer, sessionLayer,
                                        *mask, opts.GetLoadSet())
            : UsdStage::Open(rootLayer, sessionLayer, opts.GetLoadSet());

        if(stage) {
            if(edit) {
                // Edits must apply on the session layer.
                stage->SetEditTarget(UsdEditTarget(sessionLayer));

                if(!edit->Apply(stage, err)) {
                    return TfNullPtr;
                }

                stage->SetEditTarget(UsdEditTarget(rootLayer));
            }

            if(mask)
                _ExpandStageMask(stage);
            return stage;
        } else {
            if(err) {
                UT_WorkBuffer buf;
                buf.sprintf("Failed opening stage @%s@", path.c_str());
                err->AddError(buf.buffer());
            }
        }
    }
    return TfNullPtr;
}


SdfLayerRefPtr
GusdStageCache::_Impl::CreateSessionLayer(const GusdStageEdit& edit,
                                          GusdUT_ErrorContext* err)
{
    static const std::string layerTag("GusdStageCache_SessionLayer.usda");

    if(SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(layerTag))
        return edit.Apply(layer, err) ? layer : TfNullPtr;
    if(err)
        err->AddError("Internal error creating session layer.");
    return TfNullPtr;
}


void
GusdStageCache::_Impl::_ExpandStageMask(UsdStageRefPtr& stage)
{
    UT_ASSERT_P(stage);
    UT_ASSERT_P(!stage->GetPopulationMask().IsEmpty());

    // Expand the population mask to include relationship targets.
    // TODO: This currently will test all relationships, and may be very
    // expensive. For performance, it may be necessary to limit the set
    // of relationships that are searched (skipping, say, shaders).
    // TODO: This currently does not consider attribute connections.
    stage->ExpandPopulationMask();

    // Expand the population mask to contain any existing prims of the
    // given kind. This is done to limit the number of masked stages that
    // we create. For instance, if the user passes in leaf prim paths, we
    // might otherwise end up creating a new masked stage per leaf-prim.
    // The kind used for this search is not meant to be exposed to users.
    const TfToken& expandAtKind = KindTokens->component;

    if(!expandAtKind.IsEmpty()) {
        auto popMask = stage->GetPopulationMask();
        bool changed = false;

        UsdPrimRange range = stage->TraverseAll();
        for(auto it = range.begin(); it != range.end(); ++it) {
            if(popMask.IncludesSubtree(it->GetPath())) {
                it.PruneChildren();
                continue;
            }

            TfToken kind;
            if(UsdModelAPI(*it).GetKind(&kind) &&
               KindRegistry::IsA(kind, expandAtKind)) {

                popMask.Add(it->GetPath());
                changed = true;
                it.PruneChildren();
            }
        }
        if(changed)
            stage->SetPopulationMask(popMask);
    }
}


SdfLayerRefPtr
GusdStageCache::_Impl::FindOrOpenLayer(const UT_StringRef& path,
                                       GusdUT_ErrorContext* err)
{
    // Catch Tf errors.
    GusdUT_TfErrorScope errorScope(err);
     
    if(SdfLayerRefPtr layer = SdfLayer::FindOrOpen(path.toStdString()))
        return layer;

    if(err) {
        UT_WorkBuffer buf;
        buf.sprintf("Failed opening layer @%s@", path.c_str());
        err->AddError(buf.buffer());
    }
    return TfNullPtr;
}


UsdStageRefPtr
GusdStageCache::_Impl::FindStage(const UT_StringRef& path,
                                 const GusdStageOpts& opts,
                                 const GusdStageEditPtr& edit) const
{
    // XXX: empty paths should be caught earlier.
    UT_ASSERT_P(path);

    _StageMap::const_accessor a;
    if(_stageMap.find(a, _StageKey(UTmakeUnsafeRef(path), opts, edit)))
        return a->second;
    return TfNullPtr;
}


UsdStageRefPtr
GusdStageCache::_Impl::FindOrOpenStage(const UT_StringRef& path,
                                       const GusdStageOpts& opts,
                                       const GusdStageEditPtr& edit,
                                       GusdUT_ErrorContext* err)
{
    if(UsdStageRefPtr stage = FindStage(path, opts, edit))
        return stage;

    _StageMap::accessor a;
    if(_stageMap.insert(a, _StageKey(path, opts, edit))) {
        a->second = OpenNewStage(path, opts, edit, /*mask*/ nullptr, err);
        if(!a->second) {
            _stageMap.erase(a);
            return TfNullPtr;
        }
    }
    return a->second;
}


UsdStageRefPtr
GusdStageCache::_Impl::FindOrOpenMaskedStage(const UT_StringRef& path,
                                             const GusdStageOpts& opts,
                                             const GusdStageEditPtr& edit,
                                             const SdfPath& primPath,
                                             GusdUT_ErrorContext* err)
{
    // XXX: empty paths and invalid prim paths should be caught earlier.
    UT_ASSERT_P(path);
    UT_ASSERT_P(primPath.IsAbsolutePath());
    UT_ASSERT_P(primPath.IsAbsoluteRootOrPrimPath());

    if(primPath == SdfPath::AbsoluteRootPath()) {
        // Take the cheap path!
        return FindOrOpenStage(path, opts, edit, err);
    }

    // May have an unmasked stage that matches our criteria.
    // If so, no need to create a masked stage, as the unmasked
    // stage will contain everything we need.

    if(UsdStageRefPtr stage = FindStage(path, opts, edit))
        return stage;

    // Look for an existing masked stage.
    {
        _MaskedStageCacheMap::const_accessor a;
        if(_maskedCacheMap.find(
               a, _StageKey(UTmakeUnsafeRef(path), opts, edit))) {
            UT_ASSERT_P(a->second);
            return a->second->FindOrOpenStage(primPath, err);
        }
    }

    // Make a new sub cache to hold the masked stages   
    // for this stage configuration.
    _MaskedStageCacheMap::accessor a;
    _StageKey newKey(path, opts, edit);
    if(_maskedCacheMap.insert(a, newKey))
        a->second = new _MaskedStageCache(*this, newKey);
    UT_ASSERT_P(a->second);
    return a->second->FindOrOpenStage(primPath, err);
}


DEP_MicroNode*
GusdStageCache::_Impl::GetStageMicroNode(const UsdStagePtr& stage)
{
    if(!stage)
        return nullptr;

    {
        _MicroNodeMap::const_accessor a;
        if(_microNodeMap.find(a, stage))
            return a->second;
    }

    _MicroNodeMap::accessor a;
    if(_microNodeMap.insert(a, stage))
        a->second = new _StageChangeMicroNode(stage);
    return a->second;
}


void
GusdStageCache::_Impl::Clear()
{
    // XXX: Caller should have an exclusive map lock!
    
    _stageMap.clear();

    for(auto& pair : _maskedCacheMap)
        delete pair.second;
    _maskedCacheMap.clear();

    {
        UT_AutoLock lock(_dataCacheLock);
        for(auto* cache : _dataCaches) {
            UT_ASSERT_P(cache);
            cache->Clear();
        }
        _dataCaches.clear();
    }

    for(auto& pair : _microNodeMap) {
        // Micro nodes must be deleted on the event queue.
        _EventQueue::GetInstance().AppendEvictedNode(pair.second);
    }
    _microNodeMap.clear();
}


void
GusdStageCache::_Impl::Clear(const UT_StringSet& paths)
{
    // XXX: Caller should have an exclusive map lock!

    UT_Array<_StageKey> keysToRemove;
    for(const auto& pair : _stageMap) {
        if(paths.contains(pair.first.GetPath())) {
            keysToRemove.append(pair.first);
            _microNodeMap.erase(pair.second);
        }
    }
    for(const auto& key : keysToRemove)
        _stageMap.erase(key);

    keysToRemove.clear();
    for(auto& pair : _maskedCacheMap) {
        if(paths.contains(pair.first.GetPath())) {
            delete pair.second;

            // TODO: Iterate over the masked stages,
            // erase the corresponding micro nodes.

            keysToRemove.append(pair.first);
        }
    }
    for(const auto& key : keysToRemove)
        _maskedCacheMap.erase(key);

    {
        UT_AutoLock lock(_dataCacheLock);
        for(auto* cache : _dataCaches) {
            UT_ASSERT_P(cache);
            cache->Clear(paths);
        }
    }
}


void
GusdStageCache::_Impl::FindStages(const UT_StringSet& paths,
                                  UT_Set<UsdStageRefPtr>& stages) const
{
    // Unmasked stages.
    for(const auto& pair : _stageMap) {
        if(paths.contains(pair.first.GetPath())) {
            stages.insert(pair.second);
        }
    }
    // Masked stages.
    for(const auto& pair : _maskedCacheMap) {
        if(paths.contains(pair.first.GetPath())) {
            UT_ASSERT_P(pair.second);
            pair.second->GetStages(stages);
        }
    }
}


UsdStageRefPtr
_MaskedStageCache::FindStage(const SdfPath& primPath)
{
    UT_ASSERT_P(primPath.IsAbsolutePath());
    UT_ASSERT_P(primPath.IsAbsoluteRootOrPrimPath());

    _StageMap::const_accessor ancestorAcc; 
    if(_map.find(ancestorAcc, primPath))
        return ancestorAcc->second;

    // The cache holds a map of primPath->stage. When a prim is loaded
    // with masking, all of its descendant prims are fully loaded.
    // So, to find a stage that has our prim, we only need to find an
    // existing stage that has one of our ancestors.

    int distanceToMatchingAncestor = 1;
    for(SdfPath ancestorPath = primPath.GetParentPath();
        ancestorPath != SdfPath::AbsoluteRootPath();
        ancestorPath = ancestorPath.GetParentPath(),
        ++distanceToMatchingAncestor) {

        if(_map.find(ancestorAcc, ancestorPath)) {
            // Insert an entry on the cache for this prim if we traversed
            // further than we would like to find a loaded prim,
            // in order to speed up future lookups.
            // We don't always store a new entry because that might
            // flood the cache, harming rather than improving lookups.   

            const int maxSearchDistance = 4; // Non-scientific guess.
            if(distanceToMatchingAncestor > maxSearchDistance) {
                _StageMap::accessor primAcc;
                if(_map.insert(primAcc, primPath))
                    primAcc->second = ancestorAcc->second;
                return primAcc->second;
            }
            return ancestorAcc->second;
        }
    }
    return TfNullPtr;
}


UsdStageRefPtr
_MaskedStageCache::FindOrOpenStage(const SdfPath& primPath,
                                   GusdUT_ErrorContext* err)
{
    if(UsdStageRefPtr stage = FindStage(primPath))
        return stage;

    _StageMap::accessor a;
    if(_map.insert(a, primPath)) {

        UsdStagePopulationMask mask(std::vector<SdfPath>({primPath}));
        a->second = _stageCache.OpenNewStage(_stageKey.GetPath(),
                                             _stageKey.GetOpts(),
                                             _stageKey.GetEdit(),
                                             &mask, err);
        if(!a->second) {
            _map.erase(a);
            return TfNullPtr;
        }

        // The mask may have been expanded so that the stage
        // includes additional prims. Make sure all such paths
        // are mapped on the cache.
        for(const auto& maskedPath :
                a->second->GetPopulationMask().GetPaths()) {
            if(maskedPath != primPath) {
                _StageMap::accessor other;
                if(_map.insert(other, maskedPath))
                    other->second = a->second;
            }
        }
    }
    return a->second;
}


GusdStageCache&
GusdStageCache::GetInstance()
{
    static GusdStageCache cache;
    return cache;
}


GusdStageCache::GusdStageCache()
    : _impl(new _Impl)
{}


GusdStageCache::~GusdStageCache()
{
    delete _impl;
}


void
GusdStageCache::AddDataCache(GusdUSD_DataCache& cache)
{
    _impl->AddDataCache(cache);
}


void
GusdStageCache::RemoveDataCache(GusdUSD_DataCache& cache)
{
    _impl->RemoveDataCache(cache);
}


GusdStageCacheReader::GusdStageCacheReader(GusdStageCache& cache, bool writer)
    : _cache(cache), _writer(writer)
{
    if(writer)
        _cache._impl->GetMapLock().writeLock();
    else
        _cache._impl->GetMapLock().readLock();
}


GusdStageCacheReader::~GusdStageCacheReader()
{
    if(_writer)
        _cache._impl->GetMapLock().writeUnlock();
    else
        _cache._impl->GetMapLock().readUnlock();
}


UsdStageRefPtr
GusdStageCacheReader::Find(const UT_StringRef& path,
                           const GusdStageOpts& opts,
                           const GusdStageEditPtr& edit)
{
    return path ? _cache._impl->FindStage(path, opts, edit) : TfNullPtr;
}


UsdStageRefPtr
GusdStageCacheReader::FindOrOpen(const UT_StringRef& path,
                                 const GusdStageOpts& opts,
                                 const GusdStageEditPtr& edit,
                                 GusdUT_ErrorContext* err)
{
    return path ? _cache._impl->FindOrOpenStage(
        path, opts, edit, err) : TfNullPtr;
}


DEP_MicroNode*
GusdStageCacheReader::GetStageMicroNode(const UsdStagePtr& stage)
{
    return _cache._impl->GetStageMicroNode(stage);
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrim(const UT_StringRef& path,
                              const SdfPath& primPath,
                              const GusdStageEditPtr& edit,
                              const GusdStageOpts& opts,
                              GusdUT_ErrorContext* err)
{
    PrimStagePair pair;
    if(path && primPath.IsAbsolutePath() &&
       primPath.IsAbsoluteRootOrPrimPath()) {

        if(pair.second = _cache._impl->FindOrOpenMaskedStage(
               path, opts, edit, primPath, err)) {

            pair.first =
                GusdUSD_Utils::GetPrimFromStage(pair.second, primPath, err);
        }

    }
    return pair;
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrimWithVariants(const UT_StringRef& path,
                                          const SdfPath& primPath,
                                          const GusdStageOpts& opts,
                                          GusdUT_ErrorContext* err)
{
    GusdStageBasicEditPtr edit;
    SdfPath primPathWithoutVariants;
    GusdStageBasicEdit::GetPrimPathAndEditFromVariantsPath(
        primPath, primPathWithoutVariants, edit);
    return GetPrim(path, primPathWithoutVariants, edit, opts, err);
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrimWithVariants(const UT_StringRef& path,
                                          const UT_StringRef& primPath,
                                          const GusdStageOpts& opts,
                                          GusdUT_ErrorContext* err)
{
    if(primPath) {
        SdfPath usdPrimPath;
        if(GusdUSD_Utils::CreateSdfPath(primPath, usdPrimPath, err))
            return GetPrimWithVariants(path, usdPrimPath, opts, err);
    }
    return PrimStagePair();
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrimWithVariants(const UT_StringRef& path,
                                          const SdfPath& primPath,
                                          const SdfPath& variants,
                                          const GusdStageOpts& opts,
                                          GusdUT_ErrorContext* err)
{
    GusdStageBasicEditPtr edit;
    if(variants.ContainsPrimVariantSelection()) {
        edit.reset(new GusdStageBasicEdit);
        edit->GetVariants().append(variants);
    }
    return GetPrim(path, primPath, edit, opts, err);
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrimWithVariants(const UT_StringRef& path,
                                          const UT_StringRef& primPath,
                                          const UT_StringRef& variants,
                                          const GusdStageOpts& opts,
                                          GusdUT_ErrorContext* err)
{
    if(primPath) {
        SdfPath sdfPrimPath, sdfVariants;
        if(GusdUSD_Utils::CreateSdfPath(primPath, sdfPrimPath, err) &&
           GusdUSD_Utils::CreateSdfPath(variants, sdfVariants, err)) {
            return GetPrimWithVariants(path, sdfPrimPath,
                                       sdfVariants, opts, err);
        }
    }
    return PrimStagePair();
}


bool
GusdStageCacheReader::GetPrims(
    const GusdDefaultArray<UT_StringHolder>& filePaths,
    const UT_Array<SdfPath>& primPaths,
    const GusdDefaultArray<GusdStageEditPtr>& edits,
    UsdPrim* prims,
    const GusdStageOpts opts,
    GusdUT_ErrorContext* err)
{
    exint count = primPaths.size();
    if(count == 0)
        return true;

    UT_ASSERT_P(prims);

    UT_AutoInterrupt task("Load USD prims");

    if(filePaths.IsConstant() && !filePaths.GetDefault()) {
        // no file paths, so will get back null prims.
        return true;
    }

    if(filePaths.IsConstant() && edits.IsConstant()) {
        // Optimization:
        // May be possible to find a full stage satisfying our needs.
        if(UsdStageRefPtr stage = Find(filePaths.GetDefault(), opts,
                                       edits.GetDefault())) {
            
            for(exint i = 0; i < count; ++i) {
                prims[i] =
                    GusdUSD_Utils::GetPrimFromStage(stage, primPaths(i), err);
                if(!prims[i]) {
                    if(!err || (*err)() >= UT_ERROR_ABORT) {
                        return false;
                    }
                }
            }
            return true;
        }
    }

    UT_ASSERT(edits.IsConstant() || edits.size() == count);
    UT_ASSERT(filePaths.IsConstant() || filePaths.size() == count);

    // TODO: Stage composition supports threading, but if a small number
    //       of stages are going to be created, we may miss out on threading
    //       opportunities, because threads will be blocked waiting for
    //       other threads to return cached stages.
    //       If this becomes a problem for performance it may be better to
    //       break this into separate Find/FindOrOpen stages, with stage
    //       requests being grouped to reduce contention.

    std::atomic_bool workerInterrupt(false);

    UTparallelForHeavyItems(
        UT_BlockedRange<size_t>(0, count),
        [&](const UT_BlockedRange<size_t>& r)
    {
        auto* boss = UTgetInterrupt();
        char bcnt;

        for(size_t i = r.begin(); i < r.end(); ++i) {
            if(BOOST_UNLIKELY(!++bcnt && (boss->opInterrupt() || 
                                          workerInterrupt))) {
                return;
            }

            prims[i] = GetPrim(filePaths(i), primPaths(i),
                               edits(i), opts, err).first;
            if(!prims[i]) {
                if(!err || (*err)() >= UT_ERROR_ABORT) {
                    // Interrupt the other worker threads.
                    workerInterrupt = true;
                    break;
                }
            }
        }
    });
    return !task.wasInterrupted() && !workerInterrupt;
}


GusdStageCacheWriter::GusdStageCacheWriter(GusdStageCache& cache)
    : GusdStageCacheReader(cache, /*writer*/ true)
{}


void
GusdStageCacheWriter::Clear()
{
    _cache._impl->Clear();
}


void
GusdStageCacheWriter::Clear(const UT_StringSet& paths)
{
    _cache._impl->Clear(paths);
}


void
GusdStageCacheWriter::FindStages(const UT_StringSet& paths,
                                 UT_Set<UsdStageRefPtr>& stages)
{
    _cache._impl->FindStages(paths, stages);
}


void
GusdStageCacheWriter::ReloadStages(const UT_StringSet& paths)
{
    UT_Set<UsdStageRefPtr> stages;
    FindStages(paths, stages);

    UT_Set<UsdStagePtr> stagePtrs;
    for(const auto& refPtr : stages)
        stagePtrs.insert(UsdStagePtr(refPtr));

    GusdStageCache::ReloadStages(stagePtrs);
}

PXR_NAMESPACE_CLOSE_SCOPE
