//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/cachingSceneIndex.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX Ideally, this is disabled by default. See comment in _PrimsDirtied.
TF_DEFINE_ENV_SETTING(HD_CACHING_SCENE_INDEX_USE_CONVERVATIVE_EVICTION, true,
    "Evict cache entry for a prim when *any* locator on the prim is dirty.");

static bool
_IsEnabledConvervativeEviction()
{
    static bool enabled =
        TfGetEnvSetting(HD_CACHING_SCENE_INDEX_USE_CONVERVATIVE_EVICTION);
    return enabled;
}

HdCachingSceneIndex::HdCachingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
{
}

HdCachingSceneIndex::~HdCachingSceneIndex() = default;

HdSceneIndexPrim
HdCachingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    // Check the hierarchy cache
    const auto i = _prims.find(primPath);
    // SdfPathTable will default-construct entries for ancestors
    // as needed to represent hierarchy, so double-check the
    // dataSource to confirm presence os a cached prim
    if (i != _prims.end() && i->second) {
        return *i->second;
    }

    using Accessor = _RecentPrimTable::const_accessor;
    
    // Check the recent prims cache
    {
        // Use a scope to minimize lifetime of tbb accessor
        // for maximum concurrency
        Accessor accessor;
        if (_recentPrims.find(accessor, primPath)) {
            return accessor->second;
        }
    }

    // No cache entry found; query input scene
    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Store in the recent prims cache
    if (!_recentPrims.insert({primPath, prim})) {
        // Another thread inserted this entry.  Since dataSources
        // are stateful, return that one.
        Accessor accessor;
        if (TF_VERIFY(_recentPrims.find(accessor, primPath))) {
            return accessor->second;
        }
    }
    return prim;
}

SdfPathVector
HdCachingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    const auto i = _childPaths.find(primPath);
    if (i != _childPaths.end() && i->second) {
        return *i->second;
    }

    using Accessor = _RecentChildPathsTable::const_accessor;
    
    {
        Accessor accessor;
        if (_recentChildPaths.find(accessor, primPath)) {
            return accessor->second;
        }
    }

    const SdfPathVector childPrimPaths =
        _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    if (!_recentChildPaths.insert({primPath, childPrimPaths})) {
        Accessor accessor;
        if (TF_VERIFY(_recentChildPaths.find(accessor, primPath))) {
            return accessor->second;
        }
    }
    
    return childPrimPaths;
}

void
HdCachingSceneIndex::_PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _ConsolidateRecent();

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        const auto i = _prims.find(entry.primPath);
        if (i != _prims.end() && i->second) {
            WorkSwapDestroyAsync(i->second->dataSource);
            i->second = std::nullopt;
        }

        _childPaths[SdfPath::AbsoluteRootPath()] = std::nullopt;
        if (!entry.primPath.IsAbsoluteRootPath()) {
            for (const SdfPath &prefix : entry.primPath.GetPrefixes()) {
                const auto it = _childPaths.find(prefix);
                if (it == _childPaths.end()) {
                    break;
                }
                if (!it->second) {
                    continue;
                }
                WorkSwapDestroyAsync(*it->second);
                it->second = std::nullopt;
            }
        }
    }

    _SendPrimsAdded(entries);
}

void
HdCachingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _ConsolidateRecent();

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        if (entry.primPath.IsAbsoluteRootPath()) {
            // Special case removing the whole scene, since this is a common
            // shutdown operation.
            _prims.ClearInParallel();
            TfReset(_prims);
            _childPaths.ClearInParallel();
            TfReset(_childPaths);
            break;
        } else {
            {
                const auto startEndIt =
                    _prims.FindSubtreeRange(entry.primPath);
                for (auto it = startEndIt.first;
                     it != startEndIt.second;
                     ++it) {
                    if (it->second) {
                        WorkSwapDestroyAsync(it->second->dataSource);
                    }
                }
                if (startEndIt.first != startEndIt.second) {
                    _prims.erase(startEndIt.first);
                }
            }
            {
                const auto it =
                    _childPaths.find(entry.primPath.GetParentPath());
                if (it != _childPaths.end()) {
                    if (it->second) {
                        WorkSwapDestroyAsync(*it->second);
                        it->second = std::nullopt;
                    }
                    const auto startEndIt =
                        _childPaths.FindSubtreeRange(entry.primPath);
                    for (auto it = startEndIt.first;
                         it != startEndIt.second;
                         ++it) {
                        if (it->second) {
                            WorkSwapDestroyAsync(*it->second);
                        }
                    }
                }
            }
        }
    }
    _SendPrimsRemoved(entries);
}


void
HdCachingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // Note that calling _ConsolidateRecentPrims() here is sufficient.
    // Though that also means that subsequent
    // calls to GetPrim() would not have the benefit
    // of needing to do look-ups in only one
    // table.
    _ConsolidateRecent();

    if (_IsEnabledConvervativeEviction()) {
        // Below, we evict the cache entry for the primPath for *any*
        // locator. Ideally, we should evict the entry only if the
        // default prim-level locator is dirty. However, we have a handful
        // of scene indices that either optionally wrap the prim container,
        // or override the prim container in a non-lazy manner.
        // In those cases, we need to evict the prim entry for any locator
        // to provide the correct result when it is requeried.
        //
        for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
            const _PrimTable::iterator i = _prims.find(entry.primPath);
            if (i != _prims.end() && i->second) {
                WorkSwapDestroyAsync(i->second->dataSource);
                i->second = std::nullopt;
            }
        }
    } else {
        // Evict the cache entry only if the default prim-level locator is
        // dirty.
        //
        for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
            if (entry.dirtyLocators.Contains(
                    HdDataSourceLocator::EmptyLocator())) {

                const _PrimTable::iterator i = _prims.find(entry.primPath);
                if (i != _prims.end() && i->second) {
                    WorkSwapDestroyAsync(i->second->dataSource);
                    i->second = std::nullopt;
                }
            }
        }
    }

    _SendPrimsDirtied(entries);
}

void
HdCachingSceneIndex::_ConsolidateRecentPrims()
{
    TRACE_FUNCTION();

    for (auto &entry: _recentPrims) {
        _prims[entry.first] = std::move(entry.second);
    }
    _recentPrims.clear();
}

void
HdCachingSceneIndex::_ConsolidateRecentChildPaths()
{
    TRACE_FUNCTION();

    for (auto &entry: _recentChildPaths) {
        _childPaths[entry.first] = std::move(entry.second);
    }
    _recentChildPaths.clear();
}

void
HdCachingSceneIndex::_ConsolidateRecent()
{
    _ConsolidateRecentPrims();
    _ConsolidateRecentChildPaths();
}

PXR_NAMESPACE_CLOSE_SCOPE
