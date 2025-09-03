//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/pageCache.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

Ef_PageCache::Ef_PageCache(_KeyFactoryFunction keyFactory) :
    _keyFactory(keyFactory)
{
}

Ef_PageCache::~Ef_PageCache()
{
    // Delete all output-to-value caches.
    TF_FOR_ALL (it, _cache) {
        delete it->second;
    }
}

Ef_OutputValueCache *
Ef_PageCache::Get(const VdfVector &key) const
{
    // Convert the given key into an Ef_VectorKey and look up the corresponding
    // page in the cache map.
    CacheMap::const_iterator it = _cache.find(_keyFactory(key));
    if (it != _cache.end()) {
        return it->second;
    }

    // There is no page for the given key.
    return NULL;
}

Ef_OutputValueCache *
Ef_PageCache::GetOrCreate(const VdfVector &key)
{
    // Look up the cache entry in the page cache. We construct an Ef_VectorKey
    // from the given key using the _keyFactory method.
    CacheMap::iterator it =
        _cache.insert(
            std::make_pair<Ef_VectorKey::StoredType, Ef_OutputValueCache *>(
                _keyFactory(key), NULL)).first;

    // If an entry does not exist for this key, instantiate a new cache.
    if (!it->second) {
        it->second = new Ef_OutputValueCache();
    }

    // Return the cache pointer.
    return it->second;
}

size_t
Ef_PageCache::Clear()
{
    if (_cache.empty()) {
        return 0;
    }

    TRACE_FUNCTION();

    // Keep track of the number of cleared bytes
    std::atomic<size_t> numBytesCleared{0};

    // Make sure to release the python lock on this thread, so that dropping
    // python objects does not result in a deadlock.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Clear all the output-to-value caches.
    WorkParallelForEach(
        _cache.begin(), _cache.end(),
        [&numBytesCleared](const CacheMap::value_type &value) {
            Ef_OutputValueCache::ExclusiveAccess cacheAccess(value.second);
            numBytesCleared.fetch_add(
                cacheAccess.Clear(),
                std::memory_order_relaxed);
        });

    return numBytesCleared.load(std::memory_order_relaxed);
}

PXR_NAMESPACE_CLOSE_SCOPE
