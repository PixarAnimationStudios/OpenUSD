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
#include "gusd/USD_StageCache.h"

#include "gusd/UT_Assert.h"
#include "gusd/USD_DataCache.h"
#include "gusd/UT_Error.h"
#include "gusd/UT_Usd.h"

#include <UT/UT_ConcurrentHashMap.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Map.h>
#include <UT/UT_StringHolder.h>
#include <UT/UT_WorkArgs.h>

#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {


using rwlock = tbb::spin_rw_mutex;

using _StageKey = GusdUSD_StageProxy::Key;
using _StageKeyHandle = GusdUSD_StageProxy::KeyHandle;


struct _StageKeyHashCmp
{
    static size_t   hash(const _StageKeyHandle& key)
                    { return hash_value(*key); }

    static bool     equal(const _StageKeyHandle& a,
                          const _StageKeyHandle& b)
                    { return *a == *b; }
};


enum _PrimState
{
    _PRIMSTATE_DESCENDENT_HAS_EDITS, /*! Variant sel edits exist somewhere
                                         beneath this prim.*/
    _PRIMSTATE_HAS_EDITS             /*! This prim or one of its parents
                                         has variant sel edits.*/
};


/** A stage on which variants are being dynamically authored.*/
struct _VariantStage
{
    _VariantStage(GusdUSD_StageCache& cache, const _StageKeyHandle& key)
        : stageProxy(new GusdUSD_StageProxy(cache, key)),
          _cache(cache) {}
        
    bool    VariantsConflict(const SdfPath& variants,
                             const SdfPath& firstVariant) const;

    void    SetVariants(const SdfPath& variants,
                        const SdfPath& firstVariant);

    GusdUSD_StageProxyHandle    stageProxy;
    
private:
    GusdUSD_StageCache&         _cache;
    UT_Map<SdfPath,_PrimState>  _affectedPaths;
};


bool
_VariantStage::VariantsConflict(const SdfPath& variants,
                                const SdfPath& firstVariant) const
{
    /* Test at the first variant. If there's any entry at all on the map,
       someone has set an opinion at a lower path that would be in conflict.*/
    auto it = _affectedPaths.find(firstVariant);
    if(it != _affectedPaths.end())
        return true;

    /* Walk the path above the first edit.
       If any edits exist, they would conflict.*/
    for(SdfPath p = firstVariant.GetParentPath();
        !p.IsEmpty(); p = p.GetParentPath()) {
        auto it = _affectedPaths.find(p);
        if(it != _affectedPaths.end() && it->second == _PRIMSTATE_HAS_EDITS) {
            return true;
        }
    }
    return false;
}


void
_VariantStage::SetVariants(const SdfPath& variants,
                           const SdfPath& firstVariant)
{
    GusdUSD_StageLock::ScopedLock lock(stageProxy->GetLock(),
                                       /*writer*/ true);

    SdfLayerRefPtr lyr = stageProxy->GetKey()->sessionLyr;

    for(SdfPath p = variants; p != firstVariant.GetParentPath();
        p = p.GetParentPath()) {
        if(p.IsPrimVariantSelectionPath()) {
            auto sel = p.GetVariantSelection();
            SdfPrimSpecHandle spec =
                SdfCreatePrimInLayer(lyr, p.GetPrimPath());
            spec->SetVariantSelection(sel.first, sel.second);
        }
        // Mark path as being edited.
        UT_ASSERT_P(_affectedPaths.find(p) == _affectedPaths.end());
        _affectedPaths[p] = _PRIMSTATE_HAS_EDITS;
    }
    /* Walk paths above the first edit, marking them false
       (indicating that they we now depend on them not being edited)*/
    for(SdfPath p = firstVariant.GetParentPath(); !p.IsEmpty();
        p = p.GetParentPath()) {

        UT_ASSERT_P(_affectedPaths.find(p) == _affectedPaths.end() ||
                    _affectedPaths[p] == _PRIMSTATE_DESCENDENT_HAS_EDITS);
        _affectedPaths[p] = _PRIMSTATE_DESCENDENT_HAS_EDITS;
    }
}


/** Data for a particulate stage key.
    Since we support deferred variant applications, this may
    encompass a range of stages.*/
struct _StageData
{
    _StageData(GusdUSD_StageCache& cache, const _StageKeyHandle& key)
        : stageProxy(new GusdUSD_StageProxy(cache, key)), _cache(cache) {}

    ~_StageData()
    {
        for(auto* variantStage : _variantStages)
            delete GusdUTverify_ptr(variantStage);
    }


    GusdUSD_StageProxyHandle    FindVariantsStage(
                                    const SdfPath& variants) const;

    /** Find or create a stage to hold the given variants,
        without conflicting with other selections we've made.*/
    GusdUSD_StageProxyHandle    FindOrCreateVariantsStage(
                                    const _StageKeyHandle& key,
                                    const SdfPath& variants);

    GusdUSD_StageProxyHandle    stageProxy;     /*! Plain stage without
                                                    any modifications.*/

    /** Unload all stages within this StageData.*/
    void                        Unload();

private:
    typedef UT_Map<SdfPath,GusdUSD_StageProxyHandle>        _PathProxyMap;

    _PathProxyMap               _variantsMap; /*! Maps pathWithVariants->proxy,
                                                to accelerate variant access.*/
    UT_Array<_VariantStage*>    _variantStages;
    mutable rwlock              _variantsLock;
    GusdUSD_StageCache&         _cache;
};


GusdUSD_StageProxyHandle
_StageData::FindVariantsStage(const SdfPath& variants) const
{
    rwlock::scoped_lock lock(_variantsLock, /*write*/ false);
    auto it = _variantsMap.find(variants);
    return it != _variantsMap.end() ? it->second : GusdUSD_StageProxyHandle();
}


GusdUSD_StageProxyHandle
_StageData::FindOrCreateVariantsStage(const _StageKeyHandle& key,
                                      const SdfPath& variants)
{
    rwlock::scoped_lock lock(_variantsLock, /*write*/ false);

    auto it = _variantsMap.find(variants);
    if(it != _variantsMap.end())
        return it->second;

    // acquire write access
    if(!lock.upgrade_to_writer()) {
        // other thread might have beat us to it.
        it = _variantsMap.find(variants);
        if(it != _variantsMap.end())
            return it->second;
    }

    // Find the parent path right above the first variant in the path.
    SdfPath firstVariant;
    {
        for(SdfPath p = variants; !p.IsEmpty(); p = p.GetParentPath()) {
            if(p.IsPrimVariantSelectionPath()) {
                firstVariant = p;
            }
        }
        if(firstVariant.IsEmpty()) {
            /* We didn't find any variant sels, so okay to use the main proxy.
               Mark the path so we don't have to lock on it again.*/
            _variantsMap[variants] = stageProxy;
            return stageProxy;
        }
        firstVariant = firstVariant.GetPrimPath();
        UT_ASSERT_P(!firstVariant.IsEmpty());
    }

    // Find a variant-holding stage that isn't in conflict with these variants.
    for(auto* variantStage : _variantStages) {
        if(!variantStage->VariantsConflict(variants, firstVariant)) {
            variantStage->SetVariants(variants, firstVariant);
            // Mark the path to speed up future lookups.
            _variantsMap[variants] = variantStage->stageProxy;
            return variantStage->stageProxy;
        }
    }
    
    // Couldn't find a non-conflicting stage. Need to create one.

    // Always need a new session layer for setting variants.
    static std::string identifier(".usda");
    SdfLayerRefPtr sessionLyr = SdfLayer::CreateAnonymous(identifier);
    // Copy the contents of the existing session, if any.
    if(key->sessionLyr)
        sessionLyr->TransferContent(key->sessionLyr);

    _StageKeyHandle variantStageKey(
        new _StageKey(key->path, sessionLyr, key->resolverCtx));
    
    auto* variantStage = new _VariantStage(_cache, variantStageKey);
    variantStage->SetVariants(variants, firstVariant);
    // Mark the path to speed up future lookups.
    _variantsMap[variants] = variantStage->stageProxy;
    _variantStages.append(variantStage);
    return variantStage->stageProxy;
}


void
_StageData::Unload()
{
    rwlock::scoped_lock lock(_variantsLock, /*write*/ true);

    // Primary stage first.
    stageProxy->Unload();
    // Unload all stages representing variants.
    for(const auto& pair : _variantsMap)
        pair.second->Unload();
}


} /*namespace*/


struct GusdUSD_StageCache::_Impl
{
    _Impl(GusdUSD_StageCache& cache) : _cache(cache) {}
    
    ~_Impl();

    typedef UT_ConcurrentHashMap<_StageKeyHandle,
                                 _StageData*,
                                 _StageKeyHashCmp> _StageDataMap;

    GusdUSD_StageProxyHandle    Find(const _StageKeyHandle& key,
                                     const SdfPath& variants) const;

    GusdUSD_StageProxyHandle    FindOrCreate(const _StageKeyHandle& key,
                                             const SdfPath& variants=SdfPath());

    void                        ClearDataCaches();
    int64                       ClearDataCaches(const GusdUSD_StageProxy& proxy);
    int64                       ClearDataCaches(const std::string& path);

    void                        AddDataCache(GusdUSD_DataCache* cache);
    void                        RemoveDataCache(GusdUSD_DataCache* cache);

    void                        Unload(const std::string& path);

private:

    GusdUSD_StageCache&             _cache;
    _StageDataMap                   _map;
    UT_Array<GusdUSD_DataCache*>    _dataCaches;
    UT_Lock                         _dataCacheLock;

    friend class GusdUSD_StageCache;
};


GusdUSD_StageCache::_Impl::~_Impl()
{
    for(auto& pair : _map)
        delete GusdUTverify_ptr(pair.second);
}


GusdUSD_StageProxyHandle
GusdUSD_StageCache::_Impl::Find(const _StageKeyHandle& key,
                                const SdfPath& variants=SdfPath()) const
{
    _StageDataMap::const_accessor a;
    if(_map.find(a, key)) {
        if(variants.IsEmpty())
            return a->second->stageProxy;
        return a->second->FindVariantsStage(variants);
    }
    return GusdUSD_StageProxyHandle();
}


GusdUSD_StageProxyHandle
GusdUSD_StageCache::_Impl::FindOrCreate(const _StageKeyHandle& key,
                                        const SdfPath& variants)
{
    {
        _StageDataMap::const_accessor a;
        if(_map.find(a, key)) {
            if(!variants.ContainsPrimVariantSelection())
                return a->second->stageProxy;
            return a->second->FindOrCreateVariantsStage(key, variants);
        }
    }

    _StageDataMap::accessor a;
    if(_map.insert(a, key))
        a->second = new _StageData(_cache, key);
    if(!variants.ContainsPrimVariantSelection())
        return a->second->stageProxy;
    return a->second->FindOrCreateVariantsStage(key, variants);
}


void
GusdUSD_StageCache::_Impl::ClearDataCaches()
{
    UT_AutoLock lock(_dataCacheLock);
    
    for(auto* cache : _dataCaches)
        cache->Clear();
}


int64
GusdUSD_StageCache::_Impl::ClearDataCaches(const GusdUSD_StageProxy& proxy)
{
    return ClearDataCaches(proxy.GetKey()->GetPath().GetString());
}


int64
GusdUSD_StageCache::_Impl::ClearDataCaches(const std::string& stagePath)
{
    UT_AutoLock lock(_dataCacheLock);

    UT_Set<std::string> stagePaths(stagePath);

    int64 freed = 0;
    for(auto* cache : _dataCaches)
        freed += cache->Clear(stagePaths);
    return freed;
}


void
GusdUSD_StageCache::_Impl::AddDataCache(GusdUSD_DataCache* cache)
{
    UT_AutoLock lock(_dataCacheLock);
    _dataCaches.append(cache);
}


void
GusdUSD_StageCache::_Impl::RemoveDataCache(GusdUSD_DataCache* cache)
{
    UT_AutoLock lock(_dataCacheLock);
    exint idx = _dataCaches.find(cache);
    if(idx >= 0)
        _dataCaches.removeIndex(idx);
}


void
GusdUSD_StageCache::_Impl::Unload(const std::string& path)
{
    for(const auto& pair : _map) {
        if(pair.first->path.GetString() == path)
            pair.second->Unload();
    }

    // Also clear entries from the source UsdStageCache.
    if(const auto lyr = SdfLayer::Find(path))
        _cache.GetCache().EraseAll(lyr);

    ClearDataCaches(path);
}


GusdUSD_StageCache::GusdUSD_StageCache()
    : _impl(new _Impl(*this))
{}


GusdUSD_StageCache::~GusdUSD_StageCache()
{
    delete _impl;
}


GusdUSD_StageCache&
GusdUSD_StageCache::GetInstance()
{
    static GusdUSD_StageCache cache;
    return cache;
}


GusdUSD_StageProxyHandle
GusdUSD_StageCache::FindProxy(const StageKeyHandle& key,
                              const SdfPath& variants)
{
    return _impl->Find(key, variants);
}


GusdUSD_StageProxyHandle
GusdUSD_StageCache::FindOrCreateProxy(const StageKeyHandle& key,
                                      const SdfPath& variants)
{
    return _impl->FindOrCreate(key, variants);
}


SdfLayerRefPtr
GusdUSD_StageCache::LayerFromString(UT_StringHolder& contents,
                                    GusdUT_ErrorContext* err)
{
    if(!contents.isstring())
        return nullptr;

    typedef UT_ConcurrentHashMap<UT_StringHolder,SdfLayerRefPtr> _Map;
    static _Map map;

    {
        _Map::const_accessor a;
        if(map.find(a, contents))
            return a->second;
    }
    _Map::accessor a;
    if(map.insert(a, contents)) {
        GusdUT_TfErrorScope scope(err);
        static std::string identifier(".usd");
        a->second = SdfLayer::CreateAnonymous(identifier);
        if(!a->second->ImportFromString(contents.toStdString())) {
            // Drop the entry.
            map.erase(a);
            return nullptr;
        }
    }
    return a->second;
}


void
GusdUSD_StageCache::ClearDataCaches()
{
    return _impl->ClearDataCaches();
}


int64
GusdUSD_StageCache::ClearDataCaches(const GusdUSD_StageProxy& proxy)
{
    return _impl->ClearDataCaches(proxy);
}


int64
GusdUSD_StageCache::ClearDataCaches(const std::string& path)
{
    return _impl->ClearDataCaches(path);
}


void
GusdUSD_StageCache::AddDataCache(GusdUSD_DataCache* cache)
{
    _impl->AddDataCache(cache);
}


void
GusdUSD_StageCache::RemoveDataCache(GusdUSD_DataCache* cache)
{
    _impl->RemoveDataCache(cache);
}


void
GusdUSD_StageCache::Unload(const std::string& path)
{
    _impl->Unload(path);
}


GusdUSD_StageCacheContext::GusdUSD_StageCacheContext(
    UsdStage::InitialLoadSet loadSet)
    : GusdUSD_StageCacheContext(GusdUSD_StageCache::GetInstance(),
                                ArGetResolver().GetCurrentContext(), loadSet)
{}


_StageKeyHandle
GusdUSD_StageCacheContext::CreateStageKey(const TfToken& path)
{
    if(!path.IsEmpty())
        return StageProxy::Key::New(path, SdfLayerRefPtr(), _resolverCtx);
    return _StageKeyHandle();
}


GusdUSD_StageProxyHandle
GusdUSD_StageCacheContext::FindOrCreateProxy(const TfToken& path,
                                             const SdfPath& variants)
{
    if(auto key = CreateStageKey(path))
        return _cache.FindOrCreateProxy(key, variants);
    return StageProxyHandle();
}


bool
GusdUSD_StageCacheContext::FindOrCreateProxies(
    UT_Array<StageProxyHandle>& proxies,
    const UT_Array<TfToken>& paths,
    const UT_Array<SdfPath>& variants)
{
    UT_ASSERT(paths.size() == variants.size());

    /* Resolving variants is potentially contentous,
       so just do this in serial for now.*/

    char bcnt = 0;
    UT_Interrupt* boss = UTgetInterrupt();
    
    proxies.setSize(paths.size());
    for(exint i = 0; i < paths.size(); ++i) {
        if(BOOST_UNLIKELY(!++bcnt && boss->opInterrupt()))
            return false;
        auto key = StageProxy::Key::New(paths(i), SdfLayerRefPtr(),
                                        _resolverCtx);
        proxies(i) = _cache.FindOrCreateProxy(key, variants(i));
    }
    return true;
}


bool
GusdUSD_StageCacheContext::Bind(StageProxy::Accessor& accessor,
                                const TfToken& path,
                                GusdUT_ErrorContext* err)
{
    if(!path.IsEmpty()) {
        if(auto proxy = FindOrCreateProxy(path))
            return accessor.Bind(proxy, _loadSet, SdfPath(), err);
    }
    return false;
}


bool
GusdUSD_StageCacheContext::Bind(StageProxy::Accessor& accessor,
                                const TfToken& path,
                                const PrimIdentifier& prim,
                                GusdUT_ErrorContext* err)
{
    if(!path.IsEmpty() && prim) {
        if(auto proxy = FindOrCreateProxy(path, prim.GetVariants()))
            return Bind(accessor, proxy, prim, err);
    }
    return false;
}


bool
GusdUSD_StageCacheContext::Bind(StageProxy::Accessor& accessor,
                                const StageProxyHandle& proxy,
                                const PrimIdentifier& prim,
                                GusdUT_ErrorContext* err)
{
    UT_ASSERT_P(proxy);
    return prim ? accessor.Bind(proxy, _loadSet, *prim, err) : false;
}



bool
GusdUSD_StageCacheContext::GetPrims(StageProxy::MultiAccessor& accessor,
                                    const UT_Array<StageProxyHandle>& proxies,
                                    const UT_Array<SdfPath>& paths,
                                    UT_Array<UsdPrim>& prims,
                                    GusdUT_ErrorContext* err)
{
    return accessor.Bind(proxies, paths, prims, _loadSet, err);
}


UsdPrim
GusdUSD_StageCacheContext::Deprecated_GetPrim(StageProxy::Accessor& accessor,
                                              const TfToken& path,
                                              const UT_StringRef& primPath,
                                              const UT_StringRef& variants,
                                              std::string* err)
{
    GusdUT_StrErrorScope scope(err);
    GusdUT_ErrorContext errCtx(scope);
    return GetPrim(accessor, path, PrimIdentifier(
                       primPath, variants, &errCtx), &errCtx);
}


UsdStageRefPtr
GusdUSD_StageCacheContext::Deprecated_GetStage(StageProxy::Accessor& accessor,
                                               const TfToken& path,
                                               std::string* err)
{
    GusdUT_StrErrorScope scope(err);
    GusdUT_ErrorContext errCtx(scope);
    return GetStage(accessor, path, &errCtx);
}

PXR_NAMESPACE_CLOSE_SCOPE
