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
#ifndef _GUSD_USD_PRIMCACHE_H_
#define _GUSD_USD_PRIMCACHE_H_


#include "gusd/USD_Proxy.h"
#include "gusd/USD_Utils.h"

#include <pxr/pxr.h>
#include "pxr/usd/usd/stageCache.h"

#include <UT/UT_StringHolder.h>

PXR_NAMESPACE_OPEN_SCOPE

class GusdUSD_DataCache;
class GusdUSD_StageCache;


/** Singleton cache for stage proxies.
    This adds additional caching on top of UsdStageCache to support
    thread-safe stage mutation (necessary to properly support dynamic
    variant switching and deferred prim loading) and dirtying of    
    dependencies for reloading.

    This additionally serves as an entry point for 'data caches',
    wherein caches to data for a stage (bounding box caches, etc.),
    can be dirtied as the stages are reloaded.

    Users of the cache must access stages via 'accessors'. These
    acquire locks on the stage, and must only be held temporarily.

    @section Variants
    
    Within Houdini, users may be simultaneously working with different
    variant combinations for the same prim. They may also be jumping   
    back and forward between viewing different variant selections
    (eg., by toggling display flags).
    Since variant switching involves costly composition, it is not
    performant to switch variants on-demand. Threads may be requesting
    different variant combinations of the same prim at the same time.
    It is also not appropriate to create a separate stage for each
    unique variant selection being made, as the stages the variants
    are being set on may be large and expensive to compose.

    The stage cache deals with this problem by attempting to create
    a minimal set of stages to allow all of the users'
    requested variant combinations to exist simultaneously,
    without setting variant opinions that conflict.

    If the user requests a primitive without specifying any variant
    selections, the primitive is pulled from a single, common stage.

    If the user requests a primitive with variant selections, and
    that primitive+variant combination does not yet exist, the
    cache will attempt to find an existing 'variant stage' that the
    selection can be made on without conflicting with selections
    already requested.
    For example, if a user requests </Foo/{modelingVariant=tall}> at
    one point in time, and then later requests 
    </Foo/{modelingVariant=tall}Bar{shadingVariant=red}, the latter
    variant selection is seen to be in conflict with the previous
    selection, so a new variant stage will be created to hold the
    selection. However, neight of those selections conflicts with
    </Bar{lod=high}, so that selection may coexist with either  
    of the other variant selections.

    @section Primitive Encapsulation

    Prims are accessed with an assumption of encapsulation.

    I.e., if prim </Foo/Bar> is requested from the cache, it is assumed
    that the caller will only be working with the contents of </Foo/Bar>.
    If a relationship of </Foo/Bar> references </Other/Scope>, which
    is behind an alternate payload, that reference may not be loaded.

    This assumption is made for the sake of performant stage management.
    Without a requirement of encapsulation, every single relationship
    would have to be taken into consideration for deferred loading.
    In models that hold shaders, this would mean walking thousands
    of relationship targets to resolve a single model.

    Similarly, the task of sharing stages for variant selections becomes
    far more complicated and expensive to manage without encapsulation,
    as all relationships must be taken into account when determining
    whether or not two variant selections cause conflict with each other.*/
class GusdUSD_StageCache
{
public:
    using StageProxy = GusdUSD_StageProxy;
    using StageProxyHandle = GusdUSD_StageProxyHandle;
    using StageKeyHandle = GusdUSD_StageProxy::KeyHandle;

    /** Access a common cache.*/
    static GusdUSD_StageCache&  GetInstance();

    GusdUSD_StageCache();

    ~GusdUSD_StageCache();

    UsdStageCache&          GetCache()          { return _cache; }
    const UsdStageCache&    GetCache() const    { return _cache; }

    /** Find or create proxies.
        This ensures the underlying cache proxies exist,
        but does *not* cause the stages to be loaded.
        Stages are loaded from proxies using proxy accessors.
        @{ */
    StageProxyHandle        FindProxy(const StageKeyHandle& key,
                                      const SdfPath& variants=SdfPath());

    StageProxyHandle        FindOrCreateProxy(
                                const StageKeyHandle& key,
                                const SdfPath& variants=SdfPath());
    /** @} */

    /** Create a layer from a string.
        Since these become session layers, and different session
        layers lead to different stages, the results are cached.*/
    static SdfLayerRefPtr   LayerFromString(UT_StringHolder& contents,
                                            GusdUT_ErrorContext* err=NULL);

    /** Clear all data caches attched to this stage cache.*/
    void                    ClearDataCaches();

    /** Clear data cache entries associated with @a proxy.*/ 
    int64                   ClearDataCaches(const GusdUSD_StageProxy& proxy);

    /** Clear data cache entries for all stages using path @a path.*/
    int64                   ClearDataCaches(const std::string& path);

    void                    AddDataCache(GusdUSD_DataCache* cache);
    void                    RemoveDataCache(GusdUSD_DataCache* cache);

    /** Unload all stages matching @a path.*/
    void                    Unload(const std::string& path);

private:
    UsdStageCache   _cache;

    struct _Impl;
    _Impl* const    _impl;
};


/** Context for cache loading.
    The cache context determines what resolver is used as well
    as the initial load set the stage is loaded with.*/
class GusdUSD_StageCacheContext
{
public:
    using PrimIdentifier = GusdUSD_Utils::PrimIdentifier;
    using StageProxy = GusdUSD_StageProxy;
    using StageProxyHandle = GusdUSD_StageProxyHandle;
    using StageKeyHandle = GusdUSD_StageProxy::KeyHandle;

    GusdUSD_StageCacheContext(
        UsdStage::InitialLoadSet loadSet=UsdStage::LoadNone);

    GusdUSD_StageCacheContext(
        GusdUSD_StageCache& cache,
        const ArResolverContext& resolverCtx,
        UsdStage::InitialLoadSet loadSet=UsdStage::LoadNone)
        : _cache(cache), _resolverCtx(resolverCtx), _loadSet(loadSet) {}

    GusdUSD_StageCache&         operator*()         { return _cache; }
    GusdUSD_StageCache*         operator->()        { return &_cache; }
    
    const ArResolverContext&        GetResolver() const { return _resolverCtx; }
    const UsdStage::InitialLoadSet& GetLoadSet() const  { return _loadSet; }

    /** Create a stage key from strings. */
    StageKeyHandle      CreateStageKey(const TfToken& path);

    /** Find or create a proxy from strings.*/
    StageProxyHandle    FindOrCreateProxy(const TfToken& path,
                                          const SdfPath& variants=SdfPath());

    /** Find or create multiple proxies.
        The input arrays must all be the same size.*/
    bool                FindOrCreateProxies(UT_Array<StageProxyHandle>& proxies,
                                            const UT_Array<TfToken>& paths,
                                            const UT_Array<SdfPath>& variants);

    /** Bind an accessor to a stage.
        The stage may contain unloaded prims.*/
    bool                Bind(StageProxy::Accessor& accessor,
                             const TfToken& path,
                             GusdUT_ErrorContext* err=NULL);

    /** Bind an accessor for @a prim in USD file at @a path.*/
    bool                Bind(StageProxy::Accessor& accessor,
                             const TfToken& path,
                             const PrimIdentifier& prim,
                             GusdUT_ErrorContext* err=NULL);

    /** Bind an accessor for @a prim in @a proxy.
        Variants must be specified when extract a proxy from the cache,
        so any variants set on @a prim are ignored here.*/
    bool                Bind(StageProxy::Accessor& accessor,
                             const StageProxyHandle& proxy,
                             const PrimIdentifier& prim,
                             GusdUT_ErrorContext* err=NULL);

    /** Fetch a prim in USD file at @a path.*/
    UsdPrim             GetPrim(StageProxy::Accessor& accessor,
                                const TfToken& path,
                                const PrimIdentifier& prim,
                                GusdUT_ErrorContext* err=NULL);

    /** Fetch a prim from @a proxy.
        Variants must be specified when extract a proxy from the cache,
        so any variants set on @a prim are ignored here.*/
    UsdPrim             GetPrim(StageProxy::Accessor& accessor,
                                const StageProxyHandle& proxy,
                                const PrimIdentifier& prim,
                                GusdUT_ErrorContext* err=NULL);

    /** Given arrays of @a proxies with corresponding @a paths, 
        bind an accessor for all of the proxies and retrieve the stages.
        All prims returned will be loaded together with their descendants.*/
    bool                GetPrims(StageProxy::MultiAccessor& accessor,
                                 const UT_Array<StageProxyHandle>& proxies,
                                 const UT_Array<SdfPath>& paths,
                                 UT_Array<UsdPrim>& prims,
                                 GusdUT_ErrorContext* err=NULL);
    /**@ } */

    /** Find or open a stage.
        Note that the stage may hold inactive prims.*/
    UsdStageRefPtr      GetStage(StageProxy::Accessor& accessor,
                                 const TfToken& path,
                                 GusdUT_ErrorContext* err=NULL);


    /** Deprecated helpers for maintaining backwards compatibility
        with old code.
        These methods use an older style of error reporting,
        and should not be used in future code.
        @{ */
    UsdPrim             Deprecated_GetPrim(StageProxy::Accessor& accessor,
                                           const TfToken& path,
                                           const UT_StringRef& primPath,
                                           const UT_StringRef& variants,
                                           std::string* err=NULL);

    UsdStageRefPtr      Deprecated_GetStage(StageProxy::Accessor& accessor,
                                            const TfToken& path,
                                            std::string* err=NULL);
    /** @} */

private:
    GusdUSD_StageCache&             _cache;
    const ArResolverContext         _resolverCtx;
    const UsdStage::InitialLoadSet  _loadSet;
};


inline UsdPrim
GusdUSD_StageCacheContext::GetPrim(StageProxy::Accessor& accessor,
                                   const TfToken& path,
                                   const PrimIdentifier& prim,
                                   GusdUT_ErrorContext* err)
{
    if(Bind(accessor, path, prim, err))
        return accessor.GetPrimAtPath(prim.GetPrimPath(), err);
    return UsdPrim();
}


inline UsdPrim
GusdUSD_StageCacheContext::GetPrim(StageProxy::Accessor& accessor,
                                   const StageProxyHandle& proxy,
                                   const PrimIdentifier& prim,
                                   GusdUT_ErrorContext* err)
{
    if(Bind(accessor, proxy, prim, err))
        return accessor.GetPrimAtPath(prim.GetPrimPath(), err);
    return UsdPrim();
}


inline UsdStageRefPtr
GusdUSD_StageCacheContext::GetStage(StageProxy::Accessor& accessor,
                                    const TfToken& path,
                                    GusdUT_ErrorContext* err)
{
    return Bind(accessor, path, err) ? accessor.GetStage() : nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /*_GUSD_USD_PRIMCACHE_H_*/
