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
#include "gusd/USD_Proxy.h"

#include "gusd/USD_StageCache.h"
#include "gusd/UT_Error.h"

#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/base/tf/pathUtils.h"

#include <SYS/SYS_AtomicInt.h>
#include <UT/UT_ConcurrentHashMap.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_Map.h>
#include <UT/UT_ParallelUtil.h>

#include <tbb/concurrent_unordered_set.h>

PXR_NAMESPACE_OPEN_SCOPE

/** Helper for tracking loaded prims.
    This currently only supports loading, not unloading.

    The set of loaded prims is tracked with a concurrent set.
    The point of this is, to the extent possible, to avoid lock contention
    when testing whether or not a prim is loaded.
    
    Note that when testing whether or not a prim is loaded, it is
    *not* sufficient to simply query UsdPrim::IsLoaded(); when we
    refer to a prim being loaded, we mean that both the prim
    *and all of its descendants* are loaded.*/
struct GusdUSD_StageProxy::_PrimLoader
{
    void    Clear() { _loaded.clear(); }

    /** Check if prims are already loaded.
        Must already have a read lock.
        @{ */
    bool    IsLoaded(const SdfPath& path)
            { return _loaded.find(path) != _loaded.end(); }
    
    void    GetUnloaded(const UnorderedPathSet& paths, SdfPathSet& unloaded)
            {
                for(const auto& path : paths) {
                    if(!IsLoaded(path))
                        unloaded.insert(path);
                }
            }

    /** @} */

    /** Load prims if they are not already loaded.
        Returns true if the lock was upgraded to a write lock.
        The @a lock should already be read-locked.
        @{ */
    bool    LoadIfNeeded(ScopedLock& lock, const SdfPath& path,
                         const UsdStageRefPtr& stage, bool haveLock)
            {
                UT_ASSERT(stage);

                if(IsLoaded(path))
                    return false;

                if(!haveLock)
                    lock.UpgradeToWriter();

                Load(path, stage);
                return true;
            }

    bool    LoadIfNeeded(ScopedLock& lock, const UnorderedPathSet& paths,
                         const UsdStageRefPtr& stage, bool haveLock)
            {
                UT_ASSERT(stage);

                SdfPathSet unloaded;
                GetUnloaded(paths, unloaded);

                if(unloaded.size() == 0)
                    return false;

                if(!haveLock)
                    lock.UpgradeToWriter();
                
                Load(unloaded, stage);
                return true;
            }
    /** @} */

    /** Load prims. Must already have a write lock.
        @{ */
    void    Load(const SdfPath& path, const UsdStageRefPtr& stage)
            {
                stage->Load(path);
                _SetLoaded(path, stage);
            }

    void    Load(const SdfPathSet& paths, const UsdStageRefPtr& stage)
            {
                stage->LoadAndUnload(paths, SdfPathSet());
                _SetLoaded(paths, stage);
            }

private:
    void    _SetLoaded(const SdfPath& path, const UsdStageRefPtr& stage)
            {
                /* Mark descendants, to reduce future lock contention.
                   TODO: This will also have the effect of increasing the
                   size of the set, which may slow down lookups.
                   Test how this affects prim load checks on larger stages
                   (i.e., millions of prims) .*/
                if(UsdPrim prim = stage->GetPrimAtPath(path)) {
                    for(UsdPrim p: UsdPrimRange::AllPrims(prim))
                        _loaded.insert(p.GetPath());
                }
            }

    void    _SetLoaded(const SdfPathSet& paths, const UsdStageRefPtr& stage)
            {
                for(const auto& path : paths)
                    _SetLoaded(path, stage);
            }


private:
    tbb::concurrent_unordered_set<SdfPath,SdfPath::Hash>    _loaded;
};


GusdUSD_StageProxy::GusdUSD_StageProxy(GusdUSD_StageCache& cache,
                                       const KeyHandle& key)
    : _key(KeyConstHandle(key.get())),
      _cache(cache),
      _lock(new GusdUSD_StageLock),
      _loadSet(UsdStage::LoadAll), _primLoader(NULL)
{
    UT_ASSERT(key);
}


GusdUSD_StageProxy::~GusdUSD_StageProxy()
{
    GusdUSD_StageCache::GetInstance().ClearDataCaches(*this);

    if(_primLoader)
        delete _primLoader;
}


bool
GusdUSD_StageProxy::_Load(ScopedLock& lock,
                          UsdStage::InitialLoadSet loadSet,
                          const UnorderedPathSet* pathsToLoad,
                          GusdUT_ErrorContext* err)
{
    if(!_microNode.requiresUpdate(0)) {
        /* XXX: Errors copied will currently only include errors,
           not warning. This is because TfErrorMark, which is being used to
           capture USD errors, is currently not able to capture warnings.
           
           This means that in the event that the stage is valid, very little
           work will be done. This is expected to change in Tf eventually,
           in which case every stage lookup may involve an excess amount of
           warning copying on every lookup, possibly impacting performance.
           May need to revisit this approach of copying all errors when Tf
           starts allowing warnings to be captured.*/
        if(err)
            _CopyErrors(*err);

        if(pathsToLoad && _primLoader) {
            if(_primLoader->LoadIfNeeded(lock, *pathsToLoad, _stage,
                                         /*have lock*/ false))
                lock.DowngradeToReader();
        }
        return _stage;
    }
    if(lock.UpgradeToWriter() || _microNode.requiresUpdate(0)) {
        /* Mark the proxy clean, so that we don't attempt to load
           again even if loading has failed. To attempt to reload,
           the node should be dirtied with MarkDirty() prior to
           the next load attempt.*/
        _microNode.update(0);

        _errors.clearAndDestroyErrors();

        GusdUT_ErrorManager errMgr(_errors);
        GusdUT_TfErrorScope scope(errMgr);
        
        if(_stage) {
            /* Asking to load when we already have a stage means
               we should reload the stage.*/
            _Reload(_stage);
            
            lock.DowngradeToReader();

            // XXX: Can reloading fail?
            return true;
        }

        if(SdfLayerRefPtr rootLyr =
           SdfLayer::FindOrOpen(_key->path.GetString())) {
            
            // Load the stage from the cache.
            UsdStageCacheContext ctx(_cache.GetCache());
            if(UsdStageRefPtr stage = UsdStage::Open(
                   rootLyr, _key->sessionLyr, _key->resolverCtx, loadSet)) {
                
                _realPath = TfToken(TfRealPath( _key->path));
                struct stat attrib;
                if( stat(_realPath.GetText(), &attrib) == 0 ) {
                    _mtime = attrib.st_mtime;
                }

                UT_ASSERT(_cache.GetCache().Contains(stage));
                _stage = stage;

                _InitLoadSet(loadSet);
                _stageData.Update(stage);

                if(pathsToLoad && _primLoader) {
                    _primLoader->Load(SdfPathSet(pathsToLoad->begin(),
                                                 pathsToLoad->end()), stage);
                }
            }
        } else {
            /* Sdf doesn't throw errors here, so we need
               to report the failure ourselves.*/
            UT_WorkBuffer buf;
            buf.sprintf("Failed to open layer: %s",
                        _key->path.GetString().c_str());
            GusdUT_LogGenericError(_errors, buf.buffer());
        }
    }
    if(err) 
        _CopyErrors(*err);
    lock.DowngradeToReader();
    return _stage;
}


void
GusdUSD_StageProxy::_Reload(const UsdStageRefPtr& stage)
{
    UT_ASSERT_P(stage);

    stage->Reload();

    _stageData.Update(stage);

    // Stage contents may have changed, so caches need to be flushed.
    _cache.ClearDataCaches(*this);
}


void
GusdUSD_StageProxy::Unload()
{
    ScopedLock lock(_lock, /*write*/ true);

    // Clear the load set (frees some memory)
    _loadSet = UsdStage::LoadAll;
    if( _primLoader ) {
        _primLoader->Clear();
    }
    _stage = nullptr;

    _errors.clearAndDestroyErrors();

    _microNode.setDirty(true);
}

void
GusdUSD_StageProxy::MarkDirtyIfFileChanged()
{
    std::string path = TfRealPath(_key->path.GetString());
    if ( _realPath == path ) {
        struct stat attrib;
        if( stat(path.c_str(), &attrib) == 0 ) {
            if( attrib.st_mtime == _mtime ) {
                return;		// file has not changed
            }
        }
    }
    MarkDirty();
}

void
GusdUSD_StageProxy::_CopyErrors(GusdUT_ErrorContext& dst)
{
    if(dst)
        GusdUT_ErrorManager::Accessor(*dst.GetErrorManager()).CopyErrors(
            _errors, UT_ERROR_NONE, dst.GetLogSeverity());
}


void
GusdUSD_StageProxy::_InitLoadSet(UsdStage::InitialLoadSet loadSet)
{
    _loadSet = loadSet;
    if(loadSet == UsdStage::LoadAll) {
        // Everything is loaded, so don't need a prim loader.
        if(_primLoader) {
            delete _primLoader;
            _primLoader = NULL;
        }
    } else {
        if(_primLoader)
            _primLoader->Clear();
        else
            _primLoader = new _PrimLoader;
    }
}


GusdUSD_StageProxy::_StageData::_StageData()
  : startTimeCode(0), endTimeCode(0)
{}


void
GusdUSD_StageProxy::_StageData::Update(const UsdStageRefPtr& stage)
{
    UT_ASSERT(stage);

    double preroll = 0.0;
    double postroll = 0.0;

    if (const auto& pseudoRoot = stage->GetPseudoRoot()) {

        const TfToken prerollName("shot:preroll");
        const TfToken postrollName("shot:postroll");

        static const Usd_PrimFlagsPredicate pred(UsdPrimIsActive and
                                                 UsdPrimIsDefined and
                                                 not UsdPrimIsAbstract);
        for(const auto& prim : pseudoRoot.GetFilteredChildren(pred)) {

            VtValue val;
            double dval;

            if (auto prerollAttr = prim.GetAttribute(prerollName)) {
                prerollAttr.Get(&val);

                dval = 0.0;
                if (val.IsHolding<int>()) {
                    dval = double(val.UncheckedGet<int>());
                } else if (val.IsHolding<double>()) {
                    dval = val.UncheckedGet<double>();
                }
                preroll = std::max(preroll, dval);
            }

            if (auto postrollAttr = prim.GetAttribute(postrollName)) {
                postrollAttr.Get(&val);

                dval = 0.0;
                if (val.IsHolding<int>()) {
                    dval = double(val.UncheckedGet<int>());
                } else if (val.IsHolding<double>()) {
                    dval = val.UncheckedGet<double>();
                }
                postroll = std::max(postroll, dval);
            }
        }
    }

    const auto& lyr = stage->GetRootLayer();
    
    startTimeCode = lyr->GetStartTimeCode() - preroll;
    endTimeCode = lyr->GetEndTimeCode() + postroll;
}


bool
GusdUSD_StageProxy::Accessor::Bind(const GusdUSD_StageProxyHandle& proxy,
                                   UsdStage::InitialLoadSet loadSet,
                                   const SdfPath& pathToLoad,
                                   GusdUT_ErrorContext* err)
{
    if(!pathToLoad.IsEmpty()) {
        UnorderedPathSet pathsToLoad;
        pathsToLoad.insert(pathToLoad);
        return Bind(proxy, loadSet, &pathsToLoad, err);
    } else {
        return Bind(proxy, loadSet, NULL, err);
    }
}


bool
GusdUSD_StageProxy::Accessor::Bind(const GusdUSD_StageProxyHandle& proxy,
                                   UsdStage::InitialLoadSet loadSet,
                                   const UnorderedPathSet* pathsToLoad,
                                   GusdUT_ErrorContext* err)
{
    Release();
    if(_proxy = proxy) {
        _lock.Acquire(_proxy->_lock, /*write*/ false);

        if(_proxy->_Load(_lock, loadSet, pathsToLoad, err))
            return true;
        _proxy.reset();
        _lock.Release();
    }
    return false;
}


UsdPrim
GusdUSD_StageProxy::Accessor::GetPrimAtPath(const SdfPath& path,
                                            GusdUT_ErrorContext* err) const
{
    UT_ASSERT_P(_proxy);

    if(UsdPrim prim = GetStage()->GetPrimAtPath(path))
        return prim;
    if(err) {
        UT_WorkBuffer buf;
        buf.sprintf("Invalid prim <%s>", path.GetString().c_str());
        err->AddError(buf.buffer());
    }
    return UsdPrim();
}


void
GusdUSD_StageProxy::Accessor::_Load(const SdfPathSet& paths)
{
    UT_ASSERT_P(_proxy);
 
    if(_proxy->_primLoader) {
        _lock.UpgradeToWriter();
        _proxy->_primLoader->Load(paths, _proxy->_stage);
        _lock.DowngradeToReader();
    }
}


void
GusdUSD_StageProxy::MultiAccessor::Release()
{
    if(_accessors) {
        delete[] _accessors;
        _accessors = NULL;
        _size = 0;
        _numAccessors = 0;
    }
}


namespace {


struct _BindAccessorsFn
{
   typedef GusdUSD_StageProxy::UnorderedPathSet UnorderedPathSet;

    _BindAccessorsFn(GusdUSD_StageProxy::Accessor* accessors,
                     const UT_Array<GusdUSD_StageProxyHandle>& proxies,
                     UsdStage::InitialLoadSet loadSet,
                     GusdUT_ErrorContext* err,
                     std::atomic_bool& workerInterrupt)
        : _accessors(accessors), _proxies(proxies),
          _loadSet(loadSet), _err(err), _workerInterrupt(workerInterrupt) {}

    void    operator()(const UT_BlockedRange<size_t>& r) const
            {
                auto* boss = UTgetInterrupt();

                for(size_t i = r.begin(); i < r.end(); ++i) {
                    if(boss->opInterrupt() || _workerInterrupt)
                        return;

                    auto& accessor = _accessors[i];
                    if(!accessor.Bind(_proxies(i), _loadSet, NULL, _err) &&
                       (!_err || (*_err)() >= UT_ERROR_ABORT)) {
                        _workerInterrupt = true;
                        return;
                    }
                }
            }
private:
    GusdUSD_StageProxy::Accessor* const         _accessors;
    const UT_Array<GusdUSD_StageProxyHandle>&   _proxies;
    const UsdStage::InitialLoadSet              _loadSet;
    GusdUT_ErrorContext*                        _err;
    std::atomic_bool&                           _workerInterrupt;
};


/** Functor for computing an array of unique proxies from a proxy
    array, as well as an index->uniqueIndex mapping of the original
    proxy elements into that array.*/
struct _ComputeProxyIndexMapFn
{
    typedef UT_ConcurrentHashMap<GusdUSD_StageProxy*,exint>  ProxyIndexMap;

    _ComputeProxyIndexMapFn(ProxyIndexMap& map,
                            UT_Array<exint>& indexMap,
                            const UT_Array<GusdUSD_StageProxyHandle>& proxies,
                            SYS_AtomicInt32& counter)
        : _map(map), _indexMap(indexMap),
          _proxies(proxies), _counter(counter) {}

    void    operator()(const UT_BlockedRange<size_t>& r) const
            {
                for(size_t i = r.begin(); i < r.end(); ++i) {

                    if(auto* proxy = _proxies(i).get()) {

                        {
                            ProxyIndexMap::const_accessor a;
                            if(_map.find(a, proxy)) {
                                _indexMap(i) = a->second;
                                continue;
                            }
                        }

                        ProxyIndexMap::accessor a;
                        if(_map.insert(a, proxy))
                            a->second = _counter.exchangeAdd(1);
                        _indexMap(i) = a->second;

                    } else {
                        _indexMap(i) = -1;
                    }
                }
            }

private:
    ProxyIndexMap&                              _map;
    UT_Array<exint>&                            _indexMap;
    const UT_Array<GusdUSD_StageProxyHandle>&   _proxies;
    SYS_AtomicInt32&                            _counter;
};


/** Compute an array containing the unique set of proxies in @a proxies.
    This also provides a index (from source array)->id mapping.*/ 
bool
_ComputeUniqueProxies(UT_Array<GusdUSD_StageProxyHandle>& uniqueProxies,
                      UT_Array<exint>& indexMap,
                      const UT_Array<GusdUSD_StageProxyHandle>& proxies)
{
    indexMap.setSize(proxies.size());

    /* XXX: When processing large numbers of primitives, almost half of
       binding time went into this operation, hence why we are threading.*/
    _ComputeProxyIndexMapFn::ProxyIndexMap map;
    SYS_AtomicInt32 counter;
    UTparallelFor(UT_BlockedRange<size_t>(0, proxies.size()),
                  _ComputeProxyIndexMapFn(map, indexMap, proxies, counter));
    if(UTgetInterrupt()->opInterrupt())
        return false;

    uniqueProxies.setSize(map.size());
        
    for(const auto& pair : map)
        uniqueProxies(pair.second) = GusdUSD_StageProxyHandle(pair.first);

    return true;
}


} /*namespace*/


bool
GusdUSD_StageProxy::MultiAccessor::Bind(
    const UT_Array<GusdUSD_StageProxyHandle>& proxies,
    const UT_Array<SdfPath>& paths,
    UT_Array<UsdPrim>& prims,
    UsdStage::InitialLoadSet loadSet,
    GusdUT_ErrorContext* err)
{
    Release();

    UT_ASSERT(paths.isEmpty() || paths.size() == proxies.size());

    if(proxies.size() == 0)
        return true;

    /* We have an input arrays of proxies and paths.
       Many of the paths will be associated with the same proxy,
       but they may also point at different proxies.
       
       To avoid having to lock every individual prim, we want to compute
       a mapping of indices from those input arrays into indices in
       an array containing just the unique set of proxies.*/

    UT_Array<GusdUSD_StageProxyHandle> uniqueProxies;
    if(!_ComputeUniqueProxies(uniqueProxies, _indexMap, proxies))
        return false;

    _numAccessors = uniqueProxies.size();
    _accessors = new Accessor[_numAccessors];
    _size = proxies.size();

    /* Now the unique set of proxies is known, so acquire accessors
       (I.e., lock and load stages) */
    std::atomic_bool workerInterrupt(false);
    UTparallelForHeavyItems(UT_BlockedRange<size_t>(0, _numAccessors),
                            _BindAccessorsFn(_accessors, uniqueProxies,
                                             loadSet, err, workerInterrupt));
    if(UTgetInterrupt()->opInterrupt() || workerInterrupt)
        return false;

    /* Any entries referencing proxies that couldn't be bound 
       should have an invalid index; no point in accessing invalid accessors.*/
    for(exint i = 0; i < _indexMap.size(); ++i) {
        int idx = _indexMap(i);
        if(idx >= 0) {
            if(!_accessors[idx])
                _indexMap(i) = -1;
        }
    }

    return _GetPrims(paths, prims, err);
}


bool
GusdUSD_StageProxy::MultiAccessor::_Load(const UT_Array<SdfPath>& paths)
{
    if(paths.isEmpty())
        return true;

    UT_ASSERT_P(paths.size() == _size);

    typedef tbb::concurrent_unordered_set<
        SdfPath,SdfPath::Hash> ConcurrentPathSet;

    struct _ComputeUnloadedPrimsFn
    {
        _ComputeUnloadedPrimsFn(const UT_Array<SdfPath>& paths,
                                const UT_Array<exint>& indexMap,
                                const UT_Array<_PrimLoader*>& loaders,
                                UT_Array<ConcurrentPathSet*>& pathSets)
            : _paths(paths), _indexMap(indexMap),
              _loaders(loaders), _pathSets(pathSets) {}

        void    operator()(const UT_BlockedRange<size_t>& r) const
                {
                    auto* boss = UTgetInterrupt();
                    char bcnt = 0;

                    for(size_t i = r.begin(); i < r.end(); ++i) {
                        if(BOOST_UNLIKELY(!++bcnt && boss->opInterrupt()))
                            return;
                        
                        exint idx = _indexMap(i);
                        if(idx >= 0) {
                            if(auto* loader = _loaders(idx)) {
                                const SdfPath& path = _paths(i);
                                if(!path.IsEmpty() && !loader->IsLoaded(path))
                                    _pathSets(idx)->insert(path);
                            }
                        }
                    }
                }

    private:
        const UT_Array<SdfPath>&        _paths;
        const UT_Array<exint>&          _indexMap;
        const UT_Array<_PrimLoader*>&   _loaders;
        UT_Array<ConcurrentPathSet*>    _pathSets;
    };

    /* Compute sets of unloaded prims.
       This constitutes the bulk of binding time, so do this parallel.*/
    UT_Array<_PrimLoader*> loaders(_numAccessors, _numAccessors);
    UT_Array<ConcurrentPathSet*> pathSets(_numAccessors, _numAccessors);
    for(exint i = 0; i < _numAccessors; ++i) {
        if ( _accessors[i] ) {
            auto* loader = _accessors[i].GetProxy()->_primLoader;
            if(loader) {
                loaders(i) = loader;
                pathSets(i) = new ConcurrentPathSet;
            }
        }
    }

    UTparallelFor(UT_BlockedRange<size_t>(0, _size),
                  _ComputeUnloadedPrimsFn(paths, _indexMap, loaders, pathSets));

    if(UTgetInterrupt()->opInterrupt())
        return false;

    /* Load the actual prims.
       This could be done in parallel, but is probably not worth it since
       there's only work to perform the first time a prim load is requested.*/
    for(exint i = 0; i < _numAccessors; ++i) {
        if(auto* pathSet = pathSets(i)) {
            SdfPathSet pathsToLoad(pathSet->begin(), pathSet->end());
            _accessors[i]._Load(pathsToLoad);
            
            delete pathSet;
        }
    }

    return true;
}


namespace {


struct _GetPrimsFn
{
    _GetPrimsFn(GusdUSD_StageProxy::MultiAccessor& accessor,
                const UT_Array<SdfPath>& paths,
                UT_Array<UsdPrim>& prims,
                GusdUT_ErrorContext* err,
                std::atomic_bool& workerInterrupt)
        : _accessor(accessor), _paths(paths), _prims(prims),
          _err(err), _workerInterrupt(workerInterrupt) {}

    void    operator()(const UT_BlockedRange<size_t>& r) const
    {
        auto* boss = UTgetInterrupt();
        char bcnt = 0;

        for(size_t i = r.begin(); i < r.end(); ++i) {
            if(BOOST_UNLIKELY(!++bcnt &&
                              (boss->opInterrupt() || _workerInterrupt)))
                return;

            const SdfPath& path = _paths(i);
            if(!path.IsEmpty()) {
                if(auto* accessor = _accessor(i)) {
                    if(UsdPrim prim = accessor->GetPrimAtPath(path, _err)) {
                        _prims(i) = prim;
                    } else if(!_err || (*_err)() >= UT_ERROR_ABORT) {
                        _workerInterrupt = true;
                        return;
                    }
                }
            }
        }
    }
private:
    GusdUSD_StageProxy::MultiAccessor&  _accessor;
    const UT_Array<SdfPath>&            _paths;
    UT_Array<UsdPrim>&                  _prims;
    GusdUT_ErrorContext*                _err;
    std::atomic_bool&                   _workerInterrupt;
};


} /*namespace*/


bool
GusdUSD_StageProxy::MultiAccessor::_GetPrims(
    const UT_Array<SdfPath>& primPaths,
    UT_Array<UsdPrim>& prims,
    GusdUT_ErrorContext* err)
{
    if(!_Load(primPaths))
        return false;

    prims.setSize(primPaths.size());

    std::atomic_bool workerInterrupt(false);
    UTparallelFor(UT_BlockedRange<size_t>(0, primPaths.size()),
                  _GetPrimsFn(*this, primPaths, prims, err, workerInterrupt));
    return !UTgetInterrupt()->opInterrupt() and !workerInterrupt;
}


bool
GusdUSD_StageProxy::MultiAccessor::ClampTimes(
    UT_Array<UsdTimeCode>& times) const
{
    UT_ASSERT(times.size() == _size);

    for(exint i = 0; i < times.size(); ++i) {
        if(const auto* accessor = (*this)(i))
            times(i) = accessor->ClampTime(times(i));
    }
    return true;
}


bool
GusdUSD_StageProxy::MultiAccessor::ClampTimes(
    GusdUSD_Utils::PrimTimeMap& timeMap) const
{
    if(!timeMap.HasPerPrimTimes()) {
        timeMap.times.setSizeNoInit(_size);
        timeMap.times.constant(timeMap.defaultTime);
    }
    return ClampTimes(timeMap.times);
}

PXR_NAMESPACE_CLOSE_SCOPE

