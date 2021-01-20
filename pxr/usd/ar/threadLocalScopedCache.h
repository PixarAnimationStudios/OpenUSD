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
#ifndef PXR_USD_AR_THREAD_LOCAL_SCOPED_CACHE_H
#define PXR_USD_AR_THREAD_LOCAL_SCOPED_CACHE_H

#include "pxr/pxr.h"
#include "pxr/usd/ar/api.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/diagnostic.h"

#include <tbb/enumerable_thread_specific.h>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class ArThreadLocalScopedCache
///
/// Utility class for custom resolver implementations. This class wraps up
/// a common pattern for implementing thread-local scoped caches for
/// ArResolver::BeginCacheScope and ArResolver::EndCacheScope.
///
/// \code{.cpp}
/// class MyResolver : public ArResolver {
///     using ResolveCache = ArThreadLocalScopedCache<...>;
///     ResolveCache _cache;
///     
///     void BeginCacheScope(VtValue* data) { _cache.BeginCacheScope(data); }
///     void EndCacheScope(VtValue* data) { _cache.EndCacheScope(data); }
///     void Resolve(...) {
///         // If caching is active in this thread, retrieve the current
///         // cache and use it to lookup/store values.
///         if (ResolveCache::CachePtr cache = _cache.GetCurrentCache()) {
///             // ...
///         }
///         // Otherwise, caching is not active
///         // ...
///    }
/// };
/// \endcode
///
/// \see \ref ArResolver_scopedCache "Scoped Resolution Cache"
template <class CachedType>
class ArThreadLocalScopedCache
{
public:
    using CachePtr = std::shared_ptr<CachedType>;

    void BeginCacheScope(VtValue* cacheScopeData)
    {
        // Since this is intended to be used by ArResolver implementations,
        // we expect cacheScopeData to never be NULL and to either be empty
        // or holding a cache pointer that we've filled in previously.
        if (!cacheScopeData ||
            (!cacheScopeData->IsEmpty() &&
             !cacheScopeData->IsHolding<CachePtr>())) {
            TF_CODING_ERROR("Unexpected cache scope data");
            return;
        }

        _CachePtrStack& cacheStack = _threadCacheStack.local();
        if (cacheScopeData->IsHolding<CachePtr>()) {
            cacheStack.push_back(cacheScopeData->UncheckedGet<CachePtr>());
        }
        else {
            if (cacheStack.empty()) {
                cacheStack.push_back(std::make_shared<CachedType>());
            }
            else {
                cacheStack.push_back(cacheStack.back());
            }
        }
        *cacheScopeData = cacheStack.back();
    }

    void EndCacheScope(VtValue* cacheScopeData)
    {
        _CachePtrStack& cacheStack = _threadCacheStack.local();
        if (TF_VERIFY(!cacheStack.empty())) {
            cacheStack.pop_back();
        }
    }

    CachePtr GetCurrentCache()
    {
        _CachePtrStack& cacheStack = _threadCacheStack.local();
        return (cacheStack.empty() ? CachePtr() : cacheStack.back());
    }

private:
    using _CachePtrStack = std::vector<CachePtr>;
    using _ThreadLocalCachePtrStack = 
        tbb::enumerable_thread_specific<_CachePtrStack>;
    _ThreadLocalCachePtrStack _threadCacheStack;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_AR_THREAD_LOCAL_SCOPED_CACHE_H
