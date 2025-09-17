//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_PAGE_CACHE_STORAGE_H
#define PXR_EXEC_EF_PAGE_CACHE_STORAGE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"
#include "pxr/exec/ef/pageCache.h"
#include "pxr/exec/ef/pageCacheCommitRequest.h"

#include "pxr/exec/vdf/lruCache.h"
#include "pxr/exec/vdf/request.h"
#include "pxr/exec/vdf/types.h"

#include <tbb/concurrent_vector.h>

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class EfLeafNodeCache;
class VdfExecutorInterface;
class VdfMask;
class VdfMaskedOutput;

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfPageCacheStorage
///
/// \brief Manages a page cache and provides methods for invalidation of 
///        cached values.
///
class EfPageCacheStorage
{
    // The executor needs direct access to the page cache
    template <template <typename> class E, typename D>
    friend class EfPageCacheBasedExecutor;

    // The cache commit request needs direct access to the page cache
    friend class EfPageCacheCommitRequest;

    // Predicate type used for invalidation. The predicate returns
    // \c true if the page indexed by the specified key value shall
    // receive invalidation.
    using _CacheIteratorPredicateFunction =
        std::function<bool (const VdfVector &)>;

public:
    /// Destructor.
    ///
    EF_API
    ~EfPageCacheStorage();

    /// Constructor helper.
    ///
    /// Use this to construct heap allocated instances of this class, with the
    /// given \p leafNodeCache.
    ///
    template < typename T >
    static EfPageCacheStorage *New(
        const VdfMaskedOutput &keyMaskedOutput,
        EfLeafNodeCache *leafNodeCache);

    /// Returns the amount of memory currently used for cache storage, in bytes.
    ///
    EF_API
    static size_t GetNumBytesUsed();

    /// Returns the upper cache storage memory limit, in bytes.
    ///
    EF_API
    static size_t GetNumBytesLimit();

    /// Returns \c true, if the upper memory limit has been reached, and the
    /// object is no longer allowed to allocate additional storage to cache
    /// new values.
    ///
    EF_API
    static bool HasReachedMemoryLimit();

    /// Sets the upper memory limit, denoting how much memory this object is
    /// allowed to allocate.
    ///
    EF_API
    static void SetMemoryUsageLimit(size_t bytes);

    /// Returns \c true if the storage is enabled, i.e. output values can be
    /// committed and retrieved from the cache.
    ///
    EF_API
    bool IsEnabled() const;

    /// Enables / disables the storage.
    ///
    EF_API
    void SetEnabled(bool enable);

    /// Given any request, returns another \p request containing the outputs,
    /// which are dependent on the key output, and thus can be committed to the
    /// page cache.
    ///
    EF_API
    const VdfRequest &GetCacheableRequest(const VdfRequest &request) const;

    /// Returns the set of keys that have been cached in the pages selected
    /// by the \p predicate, as determined by the set of outputs contained in
    /// the \p request.
    /// Returns \c false if the \p request does not contain any cacheable
    /// outputs.
    ///
    EF_API
    bool GetCachedKeys(
        const _CacheIteratorPredicateFunction &predicate,
        const VdfRequest &request,
        std::vector<const VdfVector *> *cachedKeys) const;

    /// Invalidate the page cache by clearing the entire cache on the pages
    /// determined by the invalidation \p predicate.
    ///
    EF_API
    void Invalidate(
        const _CacheIteratorPredicateFunction &predicate);

    /// Invalidate the page cache by clearing the output values dependent on
    /// the \p invalidationRequest, on the pages determined by the invalidation
    /// \p predicate.
    ///
    EF_API
    void Invalidate(
        const _CacheIteratorPredicateFunction &predicate,
        const VdfMaskedOutputVector &invalidationRequest);

    /// Clear the entire cache on all pages.
    ///
    EF_API
    void Clear();

    /// Clears the output values associated with all the given \p nodes in the
    /// provided \p network.
    ///
    EF_API
    void ClearNodes(
        const VdfNetwork &network,
        const tbb::concurrent_vector<VdfIndex> &nodes);

    /// Resizes the internal structures of the page cache to be able to
    /// accommodate output values for the provided \p network.
    /// 
    /// \note
    /// It's not thread-safe to Resize() while the page cache storage is
    /// concurrently being accessed.
    ///
    EF_API
    void Resize(const VdfNetwork &network);

    /// Call this to notify the page cache storage of nodes that have been
    /// deleted from the network.
    ///
    EF_API
    void WillDeleteNode(const VdfNode &node);

private:
    // Constructor
    EF_API
    EfPageCacheStorage(
        const VdfMaskedOutput &keyMaskedOutput,
        EfLeafNodeCache *leafNodeCache,
        Ef_PageCache *newPageCache);

    // Returns \c true of the given \p output is a key output.
    EF_API
    bool _IsKeyOutput(
        const VdfOutput &output,
        const VdfMask &mask) const;

    // Returns a pointer to an existing, or newly created cache at the
    // page indexed by \p key.
    EF_API
    Ef_OutputValueCache *_GetOrCreateCache(const VdfVector &key);

    // Returns a set of all outputs dependent on the specified request.
    EF_API
    const VdfOutputToMaskMap &_FindDependencies(
        const VdfMaskedOutputVector &request) const;

    // Commits data to an output value cache, returning the size of the
    // committed data, in bytes.
    EF_API
    size_t _Commit(
        const VdfExecutorInterface &executor,
        const VdfRequest &request,
        Ef_OutputValueCache::ExclusiveAccess *cacheAccess);

    // Commits data for a single output to an output value cache, returning the
    // size of the committed data, in bytes.
    EF_API
    size_t _Commit(
        const VdfMaskedOutput &maskedOutput,
        const VdfVector &value,
        Ef_OutputValueCache::ExclusiveAccess *cacheAccess);

private:
    // The key masked output.
    VdfMaskedOutput _keyMaskedOutput;

    // The leaf node cache.
    EfLeafNodeCache *_leafNodeCache;

    // Pointer to the page cache managed by this class.
    std::unique_ptr<Ef_PageCache> _pageCache;

    // An entry in the cacheableRequests cache. The entry becomes invalid on
    // changes to the leaf node cache, so we store the leaf node cache version
    // along with the cached request.
    struct _CacheableRequestEntry {
        _CacheableRequestEntry() : version(0) {}
        size_t version;
        VdfRequest request;
    };

    // An LRU cache with cacheable requests.
    using _CacheableRequests =
        VdfLRUCache<VdfRequest, _CacheableRequestEntry, VdfRequest::Hash>;
    mutable _CacheableRequests _cacheableRequests;

    // Flags nodes that have had at least one output value stored in at
    // least one page. Once added, node references will not be removed
    // until the node is being deleted from the network. This serves as
    // an acceleration structure, which limits the set of nodes that could
    // possibly have output values stored in the page cache.
    std::unique_ptr<std::atomic<bool>[]> _nodeRefs;
    size_t _numNodeRefs;

    // The number of bytes currently used.
    static std::atomic<size_t> _numBytesUsed;

    // The upper memory limit in bytes.
    static std::atomic<size_t> _numBytesLimit;

    // Is this storage enabled?
    bool _enabled;

};

template < typename T >
EfPageCacheStorage *
EfPageCacheStorage::New(
    const VdfMaskedOutput &keyMaskedOutput,
    EfLeafNodeCache *leafNodeCache)
{
    return new EfPageCacheStorage(
        keyMaskedOutput, leafNodeCache, Ef_PageCache::New<T>());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
