//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/locatorCachingSceneIndex.h"

#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dependencyForwardingSceneIndex.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/base/tf/denseHashMap.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSceneIndexBaseRefPtr
HdsiLocatorCachingSceneIndex::AddDependencyForwardingAndCache(
    HdSceneIndexBaseRefPtr const& inputScene,
    HdDataSourceLocator const& locatorToCache,
    TfToken const& primTypeToCache)
{
    return HdsiLocatorCachingSceneIndex::New(
        HdDependencyForwardingSceneIndex::New(inputScene),
        locatorToCache, primTypeToCache);
}

HdsiLocatorCachingSceneIndexRefPtr
HdsiLocatorCachingSceneIndex::New(
    HdSceneIndexBaseRefPtr const& inputScene,
    HdDataSourceLocator const& locatorToCache,
    TfToken const& primTypeToCache)
{
    if (locatorToCache.IsEmpty()) {
        // This scene index is not intended for prim-level caching.
        TF_CODING_ERROR("HdsiLocatorCachingSceneIndex requires "
                        "a non-empty locator to cache.");
        return nullptr;
    }
    return TfCreateRefPtr(
        new HdsiLocatorCachingSceneIndex(
            inputScene, locatorToCache, primTypeToCache));
}

HdsiLocatorCachingSceneIndex::_DataSource::_DataSource(
    HdsiLocatorCachingSceneIndex const* cachingSceneIndex,
    SdfPath const& primPath,
    HdDataSourceLocator const& locator,
    HdContainerDataSourceHandle const& inputDataSource)
    : _cachingSceneIndex(*cachingSceneIndex)
    , _primPath(primPath)
    , _locator(locator)
    , _inputDataSource(inputDataSource)
{
}

TfTokenVector
HdsiLocatorCachingSceneIndex::_DataSource::GetNames()
{
    // XXX In the future, we could consider caching names.  However,
    // this is generally less important than data sources, since
    // scene index observers are already fully informed about all
    // prims via PrimsAdded notification.  Some scene indexes do invoke
    // GetNames() as part of a data source computation, but the
    // result of that computation can be cached instead.
    return _inputDataSource->GetNames();
}

HdDataSourceBaseHandle
HdsiLocatorCachingSceneIndex::_DataSource::Get(const TfToken &name)
{
    return _cachingSceneIndex._GetWithCache(
        _primPath, _locator.Append(name), _inputDataSource);
}

HdDataSourceBaseHandle
HdsiLocatorCachingSceneIndex::_GetWithCache(
    SdfPath const& primPath,
    HdDataSourceLocator const& locator,
    HdContainerDataSourceHandle const& inputDataSource) const
{
    // Require at least one locator entry.
    TF_VERIFY(locator.GetElementCount() > 0);
    TfToken const& name = locator.GetLastElement();

    // Are we caching this locator?
    if (locator.HasPrefix(_locatorToCache)) {
        // Yes.  Check the cache.
        {
            std::lock_guard<std::mutex> lock(_cacheMutex);
            _PrimCache::const_iterator it = _primDsCache.find(primPath);
            if (it != _primDsCache.end()) {
                // Found cached data for this prim.
                _LocatorDataSourceMap::const_iterator it2 =
                    it->second.find(locator);
                if (it2 != it->second.end() && it2->second) {
                    // Found cached data for this locator.
                    return it2->second;
                }
            }
        }

        // Tag cache allocations.
        HF_MALLOC_TAG_FUNCTION();

        // No cached data source found, so pull from input.
        // This is the potentially expensive or repeated operation
        // that we are intending to cache.
        HdDataSourceBaseHandle ds = inputDataSource->Get(name);
        // Recursively wrap containers.
        if (HdContainerDataSourceHandle containerDs =
            HdContainerDataSource::Cast(ds)) {
            ds = _DataSource::New(this, primPath, locator, containerDs);
        }
        // Store result in cache.
        if (ds) {
            // Note that we did not hold the lock while calling Get() above.
            // This means it is possible to race another thread to populate
            // this cache, however the cached entries are equivalent.
            std::lock_guard<std::mutex> lock(_cacheMutex);
            _primDsCache[primPath][locator] = ds;
        }
        // Return result.
        return ds;
    } else if (_locatorToCache.HasPrefix(locator)) {
        // Not caching this locator, but we do cache a descendant. 
        // Therefore, we need to recursively wrap containers.
        HdDataSourceBaseHandle ds = inputDataSource->Get(name);
        if (HdContainerDataSourceHandle containerDs =
            HdContainerDataSource::Cast(ds)) {
            ds = _DataSource::New(this, primPath, locator, containerDs);
        }
        return ds;
    } else {
        // No caching of this locator or any descendants.
        return inputDataSource->Get(name);
    }
}

HdsiLocatorCachingSceneIndex::HdsiLocatorCachingSceneIndex(
        HdSceneIndexBaseRefPtr const& inputScene,
        HdDataSourceLocator const& locatorToCache,
        TfToken const& primTypeToCache)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
  , _locatorToCache(locatorToCache)
  , _primTypeToCache(primTypeToCache)
{
    // Convenience name for recognizing caches in Hydra Scene Debugger.
    std::string displayName =
        std::string("HdsiLocatorCachingSceneIndex for ") + TfStringify(locatorToCache);
    if (!primTypeToCache.IsEmpty()) {
        displayName += std::string(" (primType ")
            + primTypeToCache.GetString() + ")";
    }
    SetDisplayName(displayName);
}

HdsiLocatorCachingSceneIndex::~HdsiLocatorCachingSceneIndex() = default;

HdSceneIndexPrim
HdsiLocatorCachingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.dataSource
        && (prim.primType == _primTypeToCache || _primTypeToCache.IsEmpty())) {
        // Returned prim containers delegate back to this cache.
        prim.dataSource = 
            _DataSource::New(this, primPath,
                HdDataSourceLocator::EmptyLocator(), prim.dataSource);
    }

    return prim;
}

SdfPathVector
HdsiLocatorCachingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiLocatorCachingSceneIndex::_PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    // An added prim can change prim contents arbitarily,
    // so all drop cached results for added prims.
    //
    // We do not need to lock _cacheMutex because Hydra requires
    // that other threads not access this scene index while it is
    // processing change notification.
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        const auto i = _primDsCache.find(entry.primPath);
        if (i != _primDsCache.end()) {
            i->second.clear();
        }
    }

    _SendPrimsAdded(entries);
}

void
HdsiLocatorCachingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    // Drop cached results for removed prims.
    //
    // We do not need to lock _cacheMutex because Hydra requires
    // that other threads not access this scene index while it is
    // processing change notification.
    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        // SdfPathTable::erase() drops all descendant cache entries.
        _primDsCache.erase(entry.primPath);
    }

    _SendPrimsRemoved(entries);
}


void
HdsiLocatorCachingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // Check for cache entries to invalidate.
    //
    // We do not need to lock _cacheMutex because Hydra requires
    // that other threads not access this scene index while it is
    // processing change notification.
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        // Fast path rejection: ignore dirtied locators we do not cache.
        if (!entry.dirtyLocators.Intersects(_locatorToCache)) {
            continue;
        }
        const auto primDsCacheIt = _primDsCache.find(entry.primPath);
        if (primDsCacheIt == _primDsCache.end()) {
            // No data cached for this prim.
            continue;
        }
        // Sweep over locators with cached data sources.
        for (auto& dsCacheEntry: primDsCacheIt->second) {
            if (!dsCacheEntry.second) {
                // No data source cached for this locator; ignore.
                continue;
            }
            if (entry.dirtyLocators.Intersects(dsCacheEntry.first)) {
                // Drop cached data source.
                // We do not remove the hashmap entry since we expect
                // it to soon be re-populated by observers.
                dsCacheEntry.second.reset();
            }
        }
    }

    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
