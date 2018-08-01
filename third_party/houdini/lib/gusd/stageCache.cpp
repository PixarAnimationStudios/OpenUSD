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
#include <OP/OP_Director.h>
#include <UT/UT_ConcurrentHashMap.h>
#include <UT/UT_Exit.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Lock.h>
#include <UT/UT_ParallelUtil.h>
#include <UT/UT_RWLock.h>
#include <UT/UT_StringHolder.h>
#include <UT/UT_StringSet.h>
#include <UT/UT_Thread.h>
#include <UT/UT_WorkBuffer.h>

#include "gusd/debugCodes.h"
#include "gusd/error.h"
#include "gusd/USD_DataCache.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/notice.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stagePopulationMask.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(GUSD_STAGEMASK_EXPANDRELS, true,
                      "Expand stage masks to include targets of relationships. "
                      "It may be possible to disable this option, which may "
                      "provide performance gains, but correctness cannot be "
                      "guaranteed when doing so.");


TF_DEFINE_ENV_SETTING(GUSD_STAGEMASK_ENABLE, true,
                      "Enable use of stage masks when accessing prims from "
                      "the cache. Note that disabling this feature may "
                      "be very detrimental to performance when separately "
                      "querying many prims with variant selections "
                      "(or other types of stage edits).");


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

            TF_DEBUG(GUSD_STAGECACHE).Msg(
                "[GusdStageCache] Propagating dirty state for stage %s\n",
                _identifier.c_str());

            OP_Node* node = OPgetDirector();
            node->propagateDirtyMicroNode(*this, OP_INPUT_CHANGED,
                                          /*data*/ nullptr, 
                                          /*send_root_event*/ false);
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
        TF_DEBUG(GUSD_STAGECACHE).Msg(
            "[GusdStageCache] StageContentsChanged notice for stage "
            "%s: dirtying state.\n", _identifier.c_str());

        SetDirty();
    }

private:
    TfNotice::Key   _noticeKey;
    std::string     _identifier;
};


} /*namespace*/


void
GusdStageCache::ReloadStages(const UT_Set<UsdStagePtr>& stages)
{
    if(!UT_Thread::isMainThread()) {
        TF_WARN("Reloading USD stages on a secondary thread. "
                "Beware that stage reloading is not thread-safe, and reloading "
                "a stage may affect other stages, including stages for which a "
                "reload request was not made! To ensure safety of reload "
                "operations, stages should only be reloaded from within "
                "Houdini's main thread.");
    }
    for(const auto& stage : stages)
        stage->Reload();
}


void
GusdStageCache::ReloadLayers(const UT_Set<SdfLayerHandle>& layers)
{
    if(!UT_Thread::isMainThread()) {
        TF_WARN("Reloading USD layers on a secondary thread. "
                "Beware that layer reloading is not thread-safe, and reloading "
                "a layer may affect any USD stages that reference that layer! "
                "To ensure safety of reload operations, stages should only be "
                "reloaded from within Houdini's main thread.");
    }
    for(const auto& layer : layers)
        layer->Reload();
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


} /*namespace*/


/// Cache holding stages for different sets of masked prims.
/// These caches are created for a common set of stage options.
class GusdStageCache::_MaskedStageCache
{
public:
    _MaskedStageCache(GusdStageCache::_Impl& stageCache, const _StageKey& key)
        : _stageCache(stageCache), _stageKey(key) {}

    UsdStageRefPtr  FindStage(const SdfPath& primPath);

    UsdStageRefPtr  FindOrOpenStage(const SdfPath& primPath,
                                    UT_ErrorSeverity sev=UT_ERROR_ABORT);

    void            Clear() { _map.clear(); }
    
    /// Append all stages held by this cache to @a stages.
    void            GetStages(UT_Set<UsdStageRefPtr>& stages) const
                    {
                        for(const auto& pair : _map)
                            stages.insert(pair.second);
                    }

    /// Load a range of [start,end) prims from this cache. The range corresponds
    /// to a *subset* of the prims in \p primPaths.
    /// The \p rangeFn functor must implement `operator()(exint)` which, given
    /// an index of an element in the [start,end) range, returns the index in
    /// \p primPaths identifying which primitive should be loaded.
    /// The resulting UsdPrim is written into \p prims at the same index.
    template <typename PrimRangeFn>
    bool            LoadPrimRange(const PrimRangeFn& rangeFn,
                                  exint start, exint end,
                                  const UT_Array<SdfPath>& primPaths,
                                  UsdPrim* prims,
                                  UT_ErrorSeverity sev=UT_ERROR_ABORT);

private:
    /// Open a new stage with the given mask.
    /// The \p invokingPrimPath is the path at which FindOrOpenStage() was
    /// called to begin the stage opening procedure. This may be set to an
    /// empty path for other loading scenarios.
    UsdStageRefPtr  _OpenStage(const UsdStagePopulationMask& mask,
                               const SdfPath& invokingPrimPath,
                               UT_ErrorSeverity sev=UT_ERROR_ABORT);

private:
    using _StageMap = UT_ConcurrentHashMap<SdfPath,UsdStageRefPtr,
                                           _SdfPathHashCmp>;

    GusdStageCache::_Impl&  _stageCache;
    _StageMap               _map;
    const _StageKey         _stageKey;
};


/// Primary internal cache implementation.
class GusdStageCache::_Impl
{
public:
    ~_Impl();

    UT_RWLock&      GetMapLock()    { return _mapLock; }

    /// Methods accessible to GusdStageCacheReader.
    /// These require only a shared lock to the stage.

    UsdStageRefPtr  OpenNewStage(const UT_StringRef& path,
                                 const GusdStageOpts& opts,
                                 const GusdStageEditPtr& edit,
                                 const UsdStagePopulationMask* mask,
                                 UT_ErrorSeverity sev=UT_ERROR_ABORT);

    SdfLayerRefPtr  CreateSessionLayer(const GusdStageEdit& edit,
                                       UT_ErrorSeverity sev=UT_ERROR_ABORT);

    SdfLayerRefPtr  FindOrOpenLayer(const UT_StringRef& path,
                                    UT_ErrorSeverity sev=UT_ERROR_ABORT);

    UsdStageRefPtr  FindStage(const UT_StringRef& path,
                              const GusdStageOpts& opts,
                              const GusdStageEditPtr& edit) const;

    UsdStageRefPtr  FindOrOpenStage(const UT_StringRef& path,
                                    const GusdStageOpts& opts,
                                    const GusdStageEditPtr& edit,
                                    UT_ErrorSeverity sev=UT_ERROR_ABORT);

    UsdStageRefPtr  FindMaskedStage(const UT_StringRef& path,
                                    const GusdStageOpts& opts,
                                    const GusdStageEditPtr& edit,
                                    const SdfPath& primPath);

    UsdStageRefPtr  FindOrOpenMaskedStage(const UT_StringRef& path,
                                          const GusdStageOpts& opts,
                                          const GusdStageEditPtr& edit,
                                          const SdfPath& primPath,
                                          UT_ErrorSeverity sev=UT_ERROR_ABORT);

    /// Open a new stage with an explicit mask.
    /// This is used when externally requesting a set of prims, so that
    /// prims may still be loaded behind masks, but in a batch that allows
    /// them to share the same stage.
    UsdStageRefPtr  OpenMaskedStage(const UT_StringRef& path,
                                    const GusdStageOpts& opts,
                                    const GusdStageEditPtr& edit,
                                    const UsdStagePopulationMask& mask,
                                    UT_ErrorSeverity sev=UT_ERROR_ABORT);

    /// Load each prim from \p primPaths from the cache, writing resulting
    /// UsdPrim instances to \p prim. The \p paths and \p edits arrays are
    /// indexed at the same element from \p primPaths being loaded.
    /// Although the cache attempts to batch prims together when it's possible
    /// for them to share the same stage, there are no guarantees that prims
    /// returned by this method will be sharing the same stage.
    bool            LoadPrims(const GusdDefaultArray<UT_StringHolder>& paths,
                              const UT_Array<SdfPath>& primPaths,
                              const GusdDefaultArray<GusdStageEditPtr>& edits,
                              UsdPrim* prims,
                              const GusdStageOpts& opts,
                              UT_ErrorSeverity sev=UT_ERROR_ABORT);

    /// Variant of the above method when a range of prims is being pulled from
    /// a common stage configuration.
    bool            LoadPrims(const UT_StringHolder& path,
                              const GusdStageOpts& opts,
                              const GusdStageEditPtr& edit,
                              const UT_Array<SdfPath>& primPaths,
                              UsdPrim* prims,
                              UT_ErrorSeverity sev=UT_ERROR_ABORT);

    DEP_MicroNode*  FindStageMicroNode(const UsdStagePtr& stage);

    DEP_MicroNode*  GetStageMicroNode(const UsdStagePtr& stage);


    /// Methods accessible to GusdStageCacheWriter.
    /// These require an exclusive lock to the stage.

    void            Clear(bool propagateDirty=false);
    void            Clear(const UT_StringSet& paths, bool propagateDirty=false);

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

    /// Load a range of [start,end) prims from the cache. The range corresponds
    /// to a *subset* of the prims in \p primPaths.
    /// The \p rangeFn functor must implement `operator()(exint)` which, given
    /// an index of an element in the range [start,end), returns the index in
    /// \p primPaths identifying which primitive should be loaded.
    /// The resulting UsdPrim is written into \p prims at the same index.
    ///
    /// If \p sev is less than UT_ERROR_ABORT, prim loading will continue even
    /// after load errors have occurred.
    template <typename PrimRangeFn>
    bool            LoadPrimRange(const PrimRangeFn& rangeFn,
                                  exint start, exint end,
                                  const UT_StringHolder& path,
                                  const GusdStageOpts& opts,
                                  const GusdStageEditPtr& edit,
                                  const UT_Array<SdfPath>& primPaths,
                                  UsdPrim* prims,
                                  UT_ErrorSeverity sev=UT_ERROR_ABORT);

protected:
    /// Expand the set of masked prims on a stage.
    void            _ExpandStageMask(UsdStageRefPtr& stage);

    /// Get a range of prims from \p stage, using the same range
    /// encoding as LoadPrimRange.
    template <typename PrimRangeFn>
    bool            _GetPrimsInRange(const PrimRangeFn& rangeFn,
                                     exint start, exint end,
                                     const UsdStageRefPtr& stage,
                                     const UT_Array<SdfPath>& primPaths,
                                     UsdPrim* prims,
                                     UT_ErrorSeverity sev=UT_ERROR_ABORT);

private:
    using _StageMap = UT_ConcurrentHashMap<_StageKey,UsdStageRefPtr,
                                           _StageKeyHashCmp>;

    using _MaskedStageCacheMap =
        UT_ConcurrentHashMap<_StageKey,
                             GusdStageCache::_MaskedStageCache*,
                             _StageKeyHashCmp>;

    struct _StageHashCmp
    {
        static bool equal(const UsdStagePtr& a, const UsdStagePtr& b)
                    { return a == b; }

        static bool hash(const UsdStagePtr& stage)
                    { return SYShash(stage); }
    };


    using _MicroNodeMap =
        UT_ConcurrentHashMap<UsdStagePtr,
                             std::shared_ptr<_StageChangeMicroNode>,
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


GusdStageCache::_Impl::~_Impl()
{
    // Clear entries, but don't propagate dirty states, as we
    // cannot guarantee that state propagation is safe.
    Clear(/*propagateDirty*/ false);
}


UsdStageRefPtr
GusdStageCache::_Impl::OpenNewStage(const UT_StringRef& path,
                                    const GusdStageOpts& opts,
                                    const GusdStageEditPtr& edit,
                                    const UsdStagePopulationMask* mask,
                                    UT_ErrorSeverity sev)
{
    // Catch Tf errors.
    GusdTfErrorScope errorScope(sev);

    // TODO: Should consider including the context as a member of the
    // stage opts, so that it can be reconfigured across different hip files.
    ArResolverContext resolverContext = ArGetResolver().GetCurrentContext();
    ArResolverContextBinder binder(resolverContext);

    // The root layer is shared, and not modified.
    if(SdfLayerRefPtr rootLayer = FindOrOpenLayer(path, sev)) {

        // Need a unique session layer on which to apply any edits.
        SdfLayerRefPtr sessionLayer;
        if(edit) {
            sessionLayer = CreateSessionLayer(*edit);
            if(!sessionLayer)
                return TfNullPtr;
        }

        UsdStageRefPtr stage =
            mask ? UsdStage::OpenMasked(rootLayer, sessionLayer,
                                        resolverContext,
                                        *mask, opts.GetLoadSet())
            : UsdStage::Open(rootLayer, sessionLayer,
                             resolverContext, opts.GetLoadSet());

        if(stage) {
            if(edit) {
                // Edits must apply on the session layer.
                stage->SetEditTarget(UsdEditTarget(sessionLayer));

                if(!edit->Apply(stage, sev)) {
                    return TfNullPtr;
                }

                stage->SetEditTarget(UsdEditTarget(rootLayer));
            }

            if(mask)
                _ExpandStageMask(stage);
            return stage;
        } else {
            GUSD_GENERIC_ERR(sev).Msg(
                "Failed opening stage @%s@", path.c_str());
        }
    }
    return TfNullPtr;
}


SdfLayerRefPtr
GusdStageCache::_Impl::CreateSessionLayer(const GusdStageEdit& edit,
                                          UT_ErrorSeverity sev)
{
    static const std::string layerTag("GusdStageCache_SessionLayer.usda");

    if(SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(layerTag))
        return edit.Apply(layer, sev) ? layer : TfNullPtr;

    GUSD_GENERIC_ERR(sev).Msg("Internal error creating session layer.");
    return TfNullPtr;
}


void
GusdStageCache::_Impl::_ExpandStageMask(UsdStageRefPtr& stage)
{
    UT_ASSERT_P(stage);
    UT_ASSERT_P(!stage->GetPopulationMask().IsEmpty());

    // Expand the population mask to contain any existing prims of the
    // given kind. This is done to limit the number of masked stages that
    // we create. For instance, if the user passes in leaf prim paths, we
    // might otherwise end up creating a new masked stage per leaf-prim.
    // The kind used for this search is not meant to be exposed to users.
    const TfToken& expandAtKind = KindTokens->component;

    if(!expandAtKind.IsEmpty()) {
        auto popMask = stage->GetPopulationMask();
        bool foundAncestorToExpand = false;

        const auto modelSearchPredicate =
            UsdPrimIsDefined && UsdPrimIsModel
            && UsdPrimIsActive && !UsdPrimIsAbstract;

        // Iterate over ancestor prims at each masked path,
        // looking for possible points at which to expand the mask.
        UsdPrimRange range = stage->Traverse(modelSearchPredicate);
        for(auto it = range.begin(); it != range.end(); ++it) {

            if(popMask.IncludesSubtree(it->GetPath())) {
                // Don't traverse beneath the masking points, because
                // masking guarantees that subtrees of the masking points
                // are fully expanded and present.
                it.PruneChildren();
                continue;
            }

            TfToken kind;
            if(UsdModelAPI(*it).GetKind(&kind) &&
               KindRegistry::IsA(kind, expandAtKind)) {

                popMask.Add(it->GetPath());

                foundAncestorToExpand = true;
                it.PruneChildren();
            }
        }
        if(foundAncestorToExpand) {
            stage->SetPopulationMask(popMask);
        } else  if(!foundAncestorToExpand) {
            // Couldn't find a reasonable enclosing model to expand to.
            // This might mean that the kinds of prims we want to expand to are
            // descendants of the masking point. Find out if that's the case.
            // (Note that unlike the previous traversal, this traversal
            // iterates *beneath* the masking sites)
            bool havePrimsWithExpansionKind = false;
            for(const auto& prim : stage->Traverse(modelSearchPredicate)) {
                TfToken kind;
                if(UsdModelAPI(prim).GetKind(&kind) &&
                   KindRegistry::IsA(kind, expandAtKind)) {
                    havePrimsWithExpansionKind = true;
                    break;
                }
            }

            if(!havePrimsWithExpansionKind) {
                // No prims matching the target expandAtKind were found.
                // This can happen if a stage isn't encoding appropriate
                // kinds in its model hierarchy, or if a stage is using a
                // non-standard kind hierarchy.
                // Rather than risking creating a stage per leaf-prim queried
                // from the cache, it's better to just expand to include the
                // full stage.
                stage->SetPopulationMask(UsdStagePopulationMask::All());
                return;
            }
        }
    }

    if(TfGetEnvSetting(GUSD_STAGEMASK_EXPANDRELS)) {
        // Expand the population mask to include relationship targets.
        // TODO: This currently will test all relationships, and may be very
        // expensive. For performance, it may be necessary to limit the set
        // of relationships that are searched (skipping, say, shaders).
        stage->ExpandPopulationMask();
    }
}


SdfLayerRefPtr
GusdStageCache::_Impl::FindOrOpenLayer(const UT_StringRef& path,
                                       UT_ErrorSeverity sev)
{
    // Catch Tf errors.
    GusdTfErrorScope errorScope(sev);
     
    SdfLayerRefPtr layer = SdfLayer::FindOrOpen(path.toStdString());

    TF_DEBUG(GUSD_STAGECACHE).Msg(
        "[GusdStageCache::FindOrOpenLayer] Returning layer %s for @%s@\n",
        (layer ? layer->GetIdentifier().c_str() : "(null)"), path.c_str());

    if(!layer)
        GUSD_GENERIC_ERR(sev).Msg("Failed opening layer @%s@", path.c_str());

    return layer;
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
                                       UT_ErrorSeverity sev)
{
    if(UsdStageRefPtr stage = FindStage(path, opts, edit)) {

        TF_DEBUG(GUSD_STAGECACHE).Msg(
            "[GusdStageCache::FindOrOpenStage] Returning %s for @%s@\n",
            UsdDescribe(stage).c_str(), path.c_str());

        return stage;
    }

    TF_DEBUG(GUSD_STAGECACHE).Msg(
        "[GusdStageCache::FindOrOpenStage] Cache miss for @%s@\n",
        path.c_str());

    _StageMap::accessor a;
    if(_stageMap.insert(a, _StageKey(path, opts, edit))) {

        a->second = OpenNewStage(path, opts, edit, /*mask*/ nullptr, sev);

        TF_DEBUG(GUSD_STAGECACHE).Msg(
            "[GusdStageCache::FindOrOpenStage] Returning %s for @%s@\n",
            UsdDescribe(a->second).c_str(), path.c_str());

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
                                             UT_ErrorSeverity sev)
{
    // XXX: empty paths and invalid prim paths should be caught earlier.
    UT_ASSERT_P(path);
    UT_ASSERT_P(primPath.IsAbsolutePath());
    UT_ASSERT_P(primPath.IsAbsoluteRootOrPrimPath());

    if(primPath == SdfPath::AbsoluteRootPath() ||
       !TfGetEnvSetting(GUSD_STAGEMASK_ENABLE)) {

        TF_DEBUG(GUSD_STAGECACHE).Msg(
            "[GusdStageCache] Load a complete stage for @%s@\n", path.c_str());

        // Access full stages.
        return FindOrOpenStage(path, opts, edit, sev);
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

            TF_DEBUG(GUSD_STAGECACHE).Msg(
                "[GusdStageCache] Found existing masked stage cache "
                "for @%s@<%s>\n", path.c_str(), primPath.GetText());

            return a->second->FindOrOpenStage(primPath, sev);
        }
    }

    // Make a new sub cache to hold the masked stages   
    // for this stage configuration.
    _MaskedStageCacheMap::accessor a;
    _StageKey newKey(path, opts, edit);
    if(_maskedCacheMap.insert(a, newKey)) {
        TF_DEBUG(GUSD_STAGECACHE).Msg(
            "[GusdStageCache] No existing masked stage cache "
            "for @%s@<%s>. Creating a new subcache.\n", 
            path.c_str(), primPath.GetText());

        a->second = new GusdStageCache::_MaskedStageCache(*this, newKey);
    }
    UT_ASSERT_P(a->second);
    return a->second->FindOrOpenStage(primPath, sev);
}


template <typename PrimRangeFn>
bool
GusdStageCache::_Impl::_GetPrimsInRange(const PrimRangeFn& rangeFn,
                                        exint start, exint end,
                                        const UsdStageRefPtr& stage,
                                        const UT_Array<SdfPath>& primPaths,
                                        UsdPrim* prims,
                                        UT_ErrorSeverity sev)
{
    // XXX: Could do this in parallel, but profiling suggests it's not worth it.

    UT_AutoInterrupt task("Get prims from stage");

    char bcnt = 0;

    for(exint i = start; i < end; ++i) {
        if(BOOST_UNLIKELY(!++bcnt && task.wasInterrupted()))
            return false;

        exint primIndex = rangeFn(i);
        UT_ASSERT_P(primIndex >= 0 && primIndex < primPaths.size());

        const SdfPath& primPath = primPaths(primIndex);
        if(!primPath.IsEmpty()) {
            prims[primIndex] =
                GusdUSD_Utils::GetPrimFromStage(stage, primPath, sev);
            if(!prims[primIndex] && sev >= UT_ERROR_ABORT) {
                return false;
            }
        }
    }
    return !task.wasInterrupted();
}


template <typename PrimRangeFn>
bool
GusdStageCache::_Impl::LoadPrimRange(const PrimRangeFn& rangeFn,
                                     exint start, exint end,
                                     const UT_StringHolder& path,
                                     const GusdStageOpts& opts,
                                     const GusdStageEditPtr& edit,
                                     const UT_Array<SdfPath>& primPaths,
                                     UsdPrim* prims,
                                     UT_ErrorSeverity sev)
{
    // XXX: Empty paths should be caught earlier.
    UT_ASSERT(path);

    if(start == end)
        return true;

    bool useFullStage = !TfGetEnvSetting(GUSD_STAGEMASK_ENABLE);
    if(!useFullStage) {
        // Check if any of the prims in the range are the absolute root;
        // If so, we should load a complete stage.
        for(exint i = start; i < end; ++i) {
            if(primPaths(rangeFn(i)) == SdfPath::AbsoluteRootPath()) {
                useFullStage = true;
                break;
            }
        }
    }

    if(useFullStage) {
        if(UsdStageRefPtr stage = FindOrOpenStage(path, opts, edit, sev)) {
            return _GetPrimsInRange(rangeFn, start, end,
                                    stage, primPaths, prims, sev);
        } else {
            // Whether or not this is an error depends on the
            // reporting severity.
            return sev < UT_ERROR_ABORT;
        }
    }

    {
        // Find an existing _MaskedStageCache for this configuration.

        _MaskedStageCacheMap::const_accessor a;
        if(_maskedCacheMap.find(
               a, _StageKey(UTmakeUnsafeRef(path), opts, edit))) {
            UT_ASSERT_P(a->second);
            return a->second->LoadPrimRange(rangeFn, start, end,
                                            primPaths, prims, sev);
        }
    }

    // Make a new sub cache to hold the masked stages   
    // for this stage configuration.
    _MaskedStageCacheMap::accessor a;
    _StageKey newKey(path, opts, edit);
    if(_maskedCacheMap.insert(a, newKey))
        a->second = new GusdStageCache::_MaskedStageCache(*this, newKey);
    UT_ASSERT_P(a->second);
    return a->second->LoadPrimRange(rangeFn, start, end, primPaths, prims, sev);
}


namespace {


/// Key used in batched prim loading.
/// This identifies the stage for a prim, as well the index that the
/// entry maps into inside of a range during batched loads.
struct _PrimLoadKey
{
    _PrimLoadKey() = default;

    _PrimLoadKey(const UT_StringHolder& path,
                 const GusdStageEditPtr& edit,
                 exint primIndex)
        : path(path), edit(edit), primIndex(primIndex) {}

    /// Check if a prim loaded with this key can be loaded on the same
    /// stage as a prim loaded for key \p o.
    bool    CanShareStage(const _PrimLoadKey& o) const
            { return edit == o.edit && path == o.path; }
    
    bool    operator<(const _PrimLoadKey& o) const
            {
                UT_ASSERT_P(path);
                UT_ASSERT_P(o.path);
                return path < o.path ||
                       (path == o.path &&
                        (edit < o.edit ||
                         edit == o.edit && primIndex < o.primIndex));
            }

    UT_StringHolder     path;
    GusdStageEditPtr    edit;
    exint               primIndex;
};


/// Object holding a set of prim load keys.
/// This object is compatible as a range functor on the LoadPrimRange()
/// methods of the cache.
struct _PrimLoadRange
{
    exint   operator()(exint i) const   { return keys(i).primIndex; }

    /// Sort the load keys. This is done in order to
    /// produce contiguous load sets.
    void    Sort();

    /// Compute ranges of elements in the array that share the same stage.
    void    ComputeSharedStageRanges(
                UT_Array<std::pair<exint,exint> >& ranges) const;
    
    UT_Array<_PrimLoadKey> keys;
};


void
_PrimLoadRange::Sort()
{
    UTparallelSort<UT_Array<_PrimLoadKey>::iterator>(
        keys.begin(), keys.end());
}


void
_PrimLoadRange::ComputeSharedStageRanges(
    UT_Array<std::pair<exint,exint> >& ranges) const
{
    if(keys.size() == 0)
        return;

    exint start = 0;
    auto prev = keys(0);

    for(exint i = 1; i < keys.size(); ++i) {
        if(!keys(i).CanShareStage(prev)) {
            ranges.emplace_back(start, i);
            prev = keys(i);
            start = i;
        }
    }
    // Handle the last entry.
    ranges.emplace_back(start, keys.size());
}


} // namespace


bool
GusdStageCache::_Impl::LoadPrims(
    const GusdDefaultArray<UT_StringHolder>& paths,
    const UT_Array<SdfPath>& primPaths,
    const GusdDefaultArray<GusdStageEditPtr>& edits,
    UsdPrim* prims,
    const GusdStageOpts& opts,
    UT_ErrorSeverity sev)
{
    const exint count = primPaths.size();
    if(count == 0) {
        return true;
    }

    UT_AutoInterrupt task("Load USD prims");

    if(paths.IsConstant() && !paths.GetDefault()) {
        // No file paths, so will get back only invalid prims.
        return true;
    }

    if(paths.IsConstant() && edits.IsConstant()) {
        // Optimization: all file paths and edits are the same,
        // so prims can be pulled from the same stage.
        return LoadPrims(paths.GetDefault(), opts,
                         edits.GetDefault(), primPaths, prims, sev);
    }

    UT_ASSERT(edits.IsConstant() || edits.size() == count);
    UT_ASSERT(paths.IsConstant() || paths.size() == count);

    // Build up keys for loading.

    _PrimLoadRange primRange;
    primRange.keys.setCapacity(count);
    for(exint i = 0; i < count; ++i) {
        // Only include valid entries.
        if(paths(i) && primPaths(i).IsAbsoluteRootOrPrimPath() &&
           primPaths(i).IsAbsolutePath()) {
            primRange.keys.emplace_back(paths(i), edits(i), i);
        }
    }

    // Sort the entries. This means that all entries that should reference
    // the same stage -- I.e., the same (path,edit) pair -- will be
    // contiguous on the array.
    primRange.Sort();

    // Identify the ranges of prims that may be able to share the same stage.
    UT_Array<std::pair<exint,exint> > ranges;
    primRange.ComputeSharedStageRanges(ranges);

    // We now have contiguous ranges of prims, identifying which
    // prims can be loaded on the same stage.
    // Dispatch across these ranges to load prims.

    std::atomic_bool workerInterrupt(false);

    GusdErrorTransport errTransport;

    UTparallelFor(
        UT_BlockedRange<size_t>(0, ranges.size()),
        [&](const UT_BlockedRange<size_t>& r)
        {
            GusdAutoErrorTransport autoErrTransport(errTransport);

            auto* boss = UTgetInterrupt();
            
            for(size_t i = r.begin(); i < r.end(); ++i) {
                if(BOOST_UNLIKELY(boss->opInterrupt() || workerInterrupt)) {
                    return;
                }

                const auto& range = ranges(i);
                
                // Can get the file/edit from the first key in the range.
                const auto& key = primRange.keys(range.first);

                if(!LoadPrimRange(primRange, range.first, range.second,
                                  key.path, opts, key.edit,
                                  primPaths, prims, sev)) {
                    // Interrupt the other worker threads.
                    workerInterrupt = true;
                    break;
                }
            }
        });
    
    return !task.wasInterrupted() && !workerInterrupt;
}


namespace {


struct _IdentityPrimRangeFn
{
    exint operator()(exint i) const { return i; }
};


} /*namespace*/


bool
GusdStageCache::_Impl::LoadPrims(const UT_StringHolder& path,
                                 const GusdStageOpts& opts,
                                 const GusdStageEditPtr& edit,
                                 const UT_Array<SdfPath>& primPaths,
                                 UsdPrim* prims,
                                 UT_ErrorSeverity sev)
{
    if(!path) {
        // Not an error: no prims are loaded.
        return true;
    }

    // Optimization:
    // May already have a full stage loaded that we can reference.
    if(UsdStageRefPtr stage = FindStage(path, opts, edit)) {
        return _GetPrimsInRange(_IdentityPrimRangeFn(), 0, primPaths.size(),
                                stage, primPaths, prims, sev);
    }

    return LoadPrimRange(_IdentityPrimRangeFn(), 0, primPaths.size(),
                         path, opts, edit, primPaths, prims, sev);
}


DEP_MicroNode*
GusdStageCache::_Impl::GetStageMicroNode(const UsdStagePtr& stage)
{
    if(!stage)
        return nullptr;

    {
        _MicroNodeMap::const_accessor a;
        if(_microNodeMap.find(a, stage))
            return a->second.get();
    }

    _MicroNodeMap::accessor a;
    if(_microNodeMap.insert(a, stage))
        a->second.reset(new _StageChangeMicroNode(stage));
    return a->second.get();
}


void
GusdStageCache::_Impl::Clear(bool propagateDirty)
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

    if(propagateDirty) {
        for(auto& pair : _microNodeMap) {
            pair.second->SetDirty();
        }
    }
    _microNodeMap.clear();
}


void
GusdStageCache::_Impl::Clear(const UT_StringSet& paths, bool propagateDirty)
{
    // XXX: Caller should have an exclusive map lock!

    UT_Array<_StageKey> keysToRemove;
    UT_Set<UsdStageRefPtr> stagesBeingRemoved;

    for(const auto& pair : _stageMap) {
        if(paths.contains(pair.first.GetPath())) {
            keysToRemove.append(pair.first);
            stagesBeingRemoved.insert(pair.second);
        }
    }
    for(const auto& key : keysToRemove)
        _stageMap.erase(key);

    keysToRemove.clear();
    for(auto& pair : _maskedCacheMap) {
        if(paths.contains(pair.first.GetPath())) {
            keysToRemove.append(pair.first);
            pair.second->GetStages(stagesBeingRemoved);
            delete pair.second;
        }
    }
    for(const auto& key : keysToRemove)
        _maskedCacheMap.erase(key);

    // Update and clear micro nodes.
    for(const UsdStageRefPtr& stage : stagesBeingRemoved) {

        if(propagateDirty) {
            _MicroNodeMap::accessor a;
            if(_microNodeMap.find(a, stage)) {
                a->second->SetDirty();
            }
        }
        _microNodeMap.erase(stage);
    }


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
GusdStageCache::_MaskedStageCache::FindStage(const SdfPath& primPath)
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
GusdStageCache::_MaskedStageCache::FindOrOpenStage(const SdfPath& primPath,
                                                   UT_ErrorSeverity sev)
{
    if(UsdStageRefPtr stage = FindStage(primPath)) {

        TF_DEBUG(GUSD_STAGECACHE).Msg(
            "[GusdStageCache::_MaskedStageCache::FindOrOpenStage] Returning "
            "%s for <%s>\n", UsdDescribe(stage).c_str(), primPath.GetText());

        return stage;
    }

    TF_DEBUG(GUSD_STAGECACHE).Msg(
        "[GusdStageCache::_MaskedStageCache::FindOrOpenStage] "
        "Cache miss for <%s>\n", primPath.GetText());

    _StageMap::accessor a;
    if(_map.insert(a, primPath)) {

        UsdStagePopulationMask mask(std::vector<SdfPath>({primPath}));

        a->second = _OpenStage(mask, primPath, sev);

        TF_DEBUG(GUSD_STAGECACHE).Msg(
            "[GusdStageCache::_MaskedStageCache::FindOrOpenStage] "
            "Returning %s for <%s>\n", UsdDescribe(a->second).c_str(),
            primPath.GetText());

        if(!a->second) {
            _map.erase(a);
            return TfNullPtr;
        }
    }
    return a->second;
}


UsdStageRefPtr
GusdStageCache::_MaskedStageCache::_OpenStage(const UsdStagePopulationMask& mask,
                                              const SdfPath& invokingPrimPath,
                                              UT_ErrorSeverity sev)
{
    UsdStageRefPtr stage =
        _stageCache.OpenNewStage(_stageKey.GetPath(), _stageKey.GetOpts(),
                                 _stageKey.GetEdit(), &mask, sev);

    TF_DEBUG(GUSD_STAGECACHE).Msg(
        "[GusdStageCache::_MaskedStageCache::_OpenStage] "
        "%p -- Opened stage %s\n", this, UsdDescribe(stage).c_str());

    if(stage) {
        // Make sure that all paths included in the mask are
        // mapped on the cache.
        // Pull the stage mask from the stage itself when doing this,
        // since the mask may have been expanded to include more prims
        // than our initial mask.
        for(const auto& maskedPath : stage->GetPopulationMask().GetPaths()) {

            // If the stage is being opened via FindOrOpenStage(), we already
            // have an accessor with exclusive write access at the
            // invokingPrimPath. The mutex on element access in not recursive,
            // so be careful to avoid attempting to re-lock that same prim,
            // or else we'll hit a deadlock.

            if(maskedPath != invokingPrimPath) {
                _StageMap::accessor other;
                if(_map.insert(other, maskedPath)) {

                    TF_DEBUG(GUSD_STAGECACHE).Msg(
                        "[GusdStageCache::_MaskedStageCache::_OpenStage] "
                        "%p -- Mapping prim <%s> to stage %s\n",  
                        this, maskedPath.GetText(), UsdDescribe(stage).c_str());

                    other->second = stage;
                }
            }
        }
    }
    return stage;
}


template <typename PrimRangeFn>
bool
GusdStageCache::_MaskedStageCache::LoadPrimRange(
    const PrimRangeFn& rangeFn,
    exint start, exint end,
    const UT_Array<SdfPath>& primPaths,
    UsdPrim* prims,
    UT_ErrorSeverity sev)
{
    if(start == end)
        return true;
    
    UT_ASSERT_P(end > start);
    UT_ASSERT_P(prims);

    // Extract prims that can be found on existing stages.
    // If the prims can't be found, append them to arrays for batched loading.
    UT_Array<exint> primIndicesForBatchedLoad;
    std::vector<SdfPath> primPathsForBatchedLoad;

    for(exint i = start; i < end; ++i) {
        exint primIndex = rangeFn(i);
        UT_ASSERT_P(primIndex >= 0 && primIndex < primPaths.size());
        
        const SdfPath& primPath = primPaths(primIndex);
        if(!primPath.IsEmpty()) {
            if(UsdStageRefPtr stage = FindStage(primPath)) {
                prims[primIndex] =
                    GusdUSD_Utils::GetPrimFromStage(stage, primPath, sev);
                if(!prims[primIndex] && sev >= UT_ERROR_ABORT)
                    return false;
            } else {
                // No existing stage may contain this prim.
                // Append to the mask for batched loading.
                primIndicesForBatchedLoad.append(primIndex);
                primPathsForBatchedLoad.emplace_back(primPath);
            }
        }
    }

    if(primPathsForBatchedLoad.size() > 0) {
        UT_ASSERT_P(primIndicesForBatchedLoad.size() ==
                    primPathsForBatchedLoad.size());
        
        // Open a stage with a mask holding all currently unloaded prims.
        if(UsdStageRefPtr stage =
           _OpenStage(UsdStagePopulationMask(
                          std::move(primPathsForBatchedLoad)),
                      SdfPath(), sev)) {

            // Get all prims in the range.
            for(exint i = 0; i < primIndicesForBatchedLoad.size(); ++i) {
                exint primIndex = primIndicesForBatchedLoad(i);
                const SdfPath& primPath = primPaths(primIndex);

                UT_ASSERT_P(!primPath.IsEmpty());

                prims[primIndex] =
                    GusdUSD_Utils::GetPrimFromStage(stage, primPath, sev);

                if(!prims[primIndex] && sev >= UT_ERROR_ABORT)
                    return false;

                // Map this prim onto the cache so that future prim lookups will
                // return this stage. This is also needed in order for the cache
                // to take ownership of the stage.
                _StageMap::accessor acc;
                _map.insert(acc, primPath);
                acc->second = stage;
            }
        } else {
            if(sev >= UT_ERROR_ABORT)
                return false;
        }
    }
    return true;
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
                           const GusdStageEditPtr& edit) const
{
    return path ? _cache._impl->FindStage(path, opts, edit) : TfNullPtr;
}


UsdStageRefPtr
GusdStageCacheReader::FindOrOpen(const UT_StringRef& path,
                                 const GusdStageOpts& opts,
                                 const GusdStageEditPtr& edit,
                                 UT_ErrorSeverity sev)
{
    return path ? _cache._impl->FindOrOpenStage(
        path, opts, edit, sev) : TfNullPtr;
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
                              UT_ErrorSeverity sev)
{
    PrimStagePair pair;
    if(path && primPath.IsAbsolutePath() &&
       primPath.IsAbsoluteRootOrPrimPath()) {

        if((pair.second = _cache._impl->FindOrOpenMaskedStage(
               path, opts, edit, primPath, sev))) {

            pair.first =
                GusdUSD_Utils::GetPrimFromStage(pair.second, primPath, sev);
        }

    }
    return pair;
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrimWithVariants(const UT_StringRef& path,
                                          const SdfPath& primPath,
                                          const GusdStageOpts& opts,
                                          UT_ErrorSeverity sev)
{
    GusdStageBasicEditPtr edit;
    SdfPath primPathWithoutVariants;
    GusdStageBasicEdit::GetPrimPathAndEditFromVariantsPath(
        primPath, primPathWithoutVariants, edit);
    return GetPrim(path, primPathWithoutVariants, edit, opts, sev);
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrimWithVariants(const UT_StringRef& path,
                                          const UT_StringRef& primPath,
                                          const GusdStageOpts& opts,
                                          UT_ErrorSeverity sev)
{
    if(primPath) {
        SdfPath usdPrimPath;
        if(GusdUSD_Utils::CreateSdfPath(primPath, usdPrimPath, sev))
            return GetPrimWithVariants(path, usdPrimPath, opts, sev);
    }
    return PrimStagePair();
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrimWithVariants(const UT_StringRef& path,
                                          const SdfPath& primPath,
                                          const SdfPath& variants,
                                          const GusdStageOpts& opts,
                                          UT_ErrorSeverity sev)
{
    GusdStageBasicEditPtr edit;
    if(variants.ContainsPrimVariantSelection()) {
        edit.reset(new GusdStageBasicEdit);
        edit->GetVariants().append(variants);
    }
    return GetPrim(path, primPath, edit, opts, sev);
}


GusdStageCacheReader::PrimStagePair
GusdStageCacheReader::GetPrimWithVariants(const UT_StringRef& path,
                                          const UT_StringRef& primPath,
                                          const UT_StringRef& variants,
                                          const GusdStageOpts& opts,
                                          UT_ErrorSeverity sev)
{
    if(primPath) {
        SdfPath sdfPrimPath, sdfVariants;
        if(GusdUSD_Utils::CreateSdfPath(primPath, sdfPrimPath, sev) &&
           GusdUSD_Utils::CreateSdfPath(variants, sdfVariants, sev)) {
            return GetPrimWithVariants(path, sdfPrimPath,
                                       sdfVariants, opts, sev);
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
    const GusdStageOpts& opts,
    UT_ErrorSeverity sev)
{
    return _cache._impl->LoadPrims(filePaths, primPaths,
                                   edits, prims, opts, sev);
}


GusdStageCacheWriter::GusdStageCacheWriter(GusdStageCache& cache)
    : GusdStageCacheReader(cache, /*writer*/ true)
{}


void
GusdStageCacheWriter::Clear()
{
    _cache._impl->Clear(/*propagateDirty*/ true);
}


void
GusdStageCacheWriter::Clear(const UT_StringSet& paths)
{
    _cache._impl->Clear(paths, /*propagateDirty*/ true);
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
