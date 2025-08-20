//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/pageCacheCommitRequest.h"

#include "pxr/exec/ef/outputValueCache.h"
#include "pxr/exec/ef/pageCacheStorage.h"

#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/request.h"

PXR_NAMESPACE_OPEN_SCOPE

EfPageCacheCommitRequest::EfPageCacheCommitRequest(
    const EfInputValueBlock &inputs,
    EfPageCacheStorage *storage) :
    _inputs(inputs),
    _cache(NULL),
    _storage(storage)
{
    if (TF_VERIFY(_storage)) {
        // We only support input value blocks with one entry.
        TF_VERIFY(inputs.GetSize() == 1);

        // Make sure that this one entry is the key output.
        EfInputValueBlock::const_iterator firstInput = inputs.begin();
        TF_VERIFY(
            _storage->_IsKeyOutput(
                *firstInput->first.GetOutput(), 
                firstInput->first.GetMask()));

        // Get the key value.
        const VdfVector *keyValue = firstInput->second;
        TF_VERIFY(keyValue);

        // Make sure a cache page exists for the given key and retain
        // a pointer to the output-to-value cache to later commit data
        // to.
        _cache = _storage->_GetOrCreateCache(*keyValue);
        TF_VERIFY(_cache);
    }
}

EfPageCacheCommitRequest::~EfPageCacheCommitRequest()
{

}

bool
EfPageCacheCommitRequest::IsUncached(
    const VdfRequest &request) const
{
    // Requires a valid pointer to the output-to-value cache.
    if (!TF_VERIFY(_cache)) {
        return true;
    }

    // Gain protected access to the output-to-value cache.
    Ef_OutputValueCache::ExclusiveAccess cacheAccess(_cache);

    // Check if any output is still not cached.
    return cacheAccess.IsUncached(request);
}

VdfRequest
EfPageCacheCommitRequest::GetUncached(
    const VdfRequest &request) const
{
    // Requires a valid pointer to the output-to-value cache.
    if (!TF_VERIFY(_cache)) {
        return request;
    }

    // Gain protected access to the output-to-value cache.
    Ef_OutputValueCache::ExclusiveAccess cacheAccess(_cache);

    // Get the subset of the request that is still not cached.
    return cacheAccess.GetUncached(request);
}

bool
EfPageCacheCommitRequest::Commit(
    const VdfExecutorInterface &executor,
    const VdfRequest &request,
    size_t *bytesCommitted)
{
    *bytesCommitted = 0;

    // Requires a valid pointer to the output-to-value cache
    // and page cache storage.
    if (!TF_VERIFY(_cache && _storage)) {
        return false;
    }

    // If there is nothing to cache, bail out right away.
    if (request.IsEmpty()) {
        return true;
    }

    // If caching is disabled, or the memory limit has been reached, or no data
    // is available on the executor, bail out.
    if (!_storage->IsEnabled() ||
        _storage->HasReachedMemoryLimit() ||
        executor.IsEmpty()) {
        return false;
    }

    // Gain protected access to the output-to-value cache.
    Ef_OutputValueCache::ExclusiveAccess cacheAccess(_cache);

    // Commit data to the output-to-value cache.
    *bytesCommitted = _storage->_Commit(executor, request, &cacheAccess);

    // All the data has been successfully committed.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
