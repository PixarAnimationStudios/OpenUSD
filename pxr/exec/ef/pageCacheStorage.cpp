//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/pageCacheStorage.h"

#include "pxr/exec/ef/pageCache.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/exec/ef/leafNodeCache.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

// The number of bytes used by page cache storage
std::atomic<size_t> EfPageCacheStorage::_numBytesUsed(0);

// The upper memory limit for page cache storage. 0 denotes no limit.
std::atomic<size_t> EfPageCacheStorage::_numBytesLimit(0);

EfPageCacheStorage::EfPageCacheStorage(
    const VdfMaskedOutput &keyMaskedOutput,
    EfLeafNodeCache *leafNodeCache,
    Ef_PageCache *newPageCache) :
    _keyMaskedOutput(keyMaskedOutput),
    _leafNodeCache(leafNodeCache),
    _pageCache(newPageCache),
    _cacheableRequests(16),
    _numNodeRefs(0),
    _enabled(true)
{
    // A page cache must be available
    TF_VERIFY(_pageCache);

    // We only support 1x1 masks for the key output.
    TF_VERIFY(_keyMaskedOutput && _keyMaskedOutput.GetMask().GetSize() == 1);
}

EfPageCacheStorage::~EfPageCacheStorage() = default;

size_t
EfPageCacheStorage::GetNumBytesUsed()
{
    return _numBytesUsed.load(std::memory_order_relaxed);
}

size_t
EfPageCacheStorage::GetNumBytesLimit()
{
    return _numBytesLimit.load(std::memory_order_relaxed);
}

bool
EfPageCacheStorage::HasReachedMemoryLimit()
{
    // Since we atomically write to these fields, we technically have to make
    // sure that all writes have been retired at this point, by issuing a 
    // memory fence.
    // However, for the sake of performance we don't issue the synchronization
    // barrier here. It's okay if we slightly exceed the memory limit because
    // not all writes had been retired just yet.

    const size_t numBytesLimit = _numBytesLimit.load(std::memory_order_relaxed);
    const size_t numBytesUsed = _numBytesUsed.load(std::memory_order_relaxed);
    return numBytesLimit > 0 && numBytesUsed >= numBytesLimit;
}

void
EfPageCacheStorage::SetMemoryUsageLimit(size_t bytes)
{
    _numBytesLimit = bytes;
}

bool
EfPageCacheStorage::IsEnabled() const
{
    return _enabled;
}

void
EfPageCacheStorage::SetEnabled(bool enable)
{
    // Is this a state change?
    if (_enabled != enable) {
        // Always clear the cache after toggling the enabled flag. The cache
        // will no longer be maintained if the storage is disabled, and we
        // also don't want to get any cache hits.
        Clear();

        // Toggle the enabled flag.
        _enabled = enable;
    }
}

void
EfPageCacheStorage::Invalidate(
    const _CacheIteratorPredicateFunction &predicate)
{
    TRACE_FUNCTION();

    // Keep track of the number of bytes that have been invalidated.
    size_t bytesInvalidated = 0;

    // Invalidate only the pages determined by the predicate functor, by
    // clearing the corresponding output-to-value caches.
    TF_FOR_ALL (it, *_pageCache) {
        Ef_OutputValueCache::ExclusiveAccess cacheAccess(it->second);
        if (!cacheAccess.IsEmpty() && predicate(it->first->GetValue())) {
            bytesInvalidated += cacheAccess.Clear();
        }
    }

    // Account for the memory that has been deallocated.
    _numBytesUsed -= bytesInvalidated;
}

class EfPageCacheStorage_InvalidateWorker
{
public:
    typedef Ef_OutputValueCache *Work;
    typedef std::vector<Work> WorkVector;

    EfPageCacheStorage_InvalidateWorker(
        const WorkVector &work,
        const VdfMaskedOutputVector &invalidOutputs,
        std::atomic<size_t> *bytesInvalidated) :
        _work(work),
        _invalidOutputs(invalidOutputs),
        _bytesInvalidated(bytesInvalidated)
    {}

    void operator() (size_t begin, size_t end) {
        size_t bytesInvalidated = 0;

        // Iterate over the subset of work units.
        for (size_t i = begin; i != end; ++i) {

            // Gain exclusive access to the output-to-value cache.
            Ef_OutputValueCache::ExclusiveAccess cacheAccess(_work[i]);

            // Invalidate each of the invalid outputs, keeping track of
            // how many bytes of memory have been free'd along the way.
            // Note that _bytesInvalidated is shared among the workers, so
            // we must update it atomically.
            bytesInvalidated += cacheAccess.Invalidate(_invalidOutputs);
        }

        // Update the atomic just once. Doing so repeatedly is expensive.
        *_bytesInvalidated += bytesInvalidated;
    }

private:
    const WorkVector &_work;
    const VdfMaskedOutputVector &_invalidOutputs;
    std::atomic<size_t> *_bytesInvalidated;
};

void
EfPageCacheStorage::Invalidate(
    const _CacheIteratorPredicateFunction &predicate,
    const VdfMaskedOutputVector &invalidationRequest)
{
    if (invalidationRequest.empty()) {
        return;
    }

    TRACE_FUNCTION();

    // Find all the outputs above leaf nodes, which are dependent
    // on the invalidation request.
    const VdfOutputToMaskMap &deps = _FindDependencies(invalidationRequest);

    // If there are no such dependencies, there is no work to do here.
    if (deps.empty()) {
        return;
    }
    
    // Create a vector of work, by finding each affected page as determined
    // by the iteration predicate. Empty output-to-value caches need not be
    // considered.
    EfPageCacheStorage_InvalidateWorker::WorkVector work;
    TF_FOR_ALL (it, *_pageCache) {
        Ef_OutputValueCache::ExclusiveAccess cacheAccess(it->second);
        if (!cacheAccess.IsEmpty() && predicate(it->first->GetValue())) {
            work.push_back(it->second);
        }
    }

    // Bail out if there is no work to do.
    if (work.empty()) {
        return;
    }

    // Transform the map of outputs affected by the invalidation into a 
    // request, which is a more tightly packed structure and allows faster
    // iteration.
    VdfMaskedOutputVector invalidOutputs;
    invalidOutputs.reserve(deps.size());
    TF_FOR_ALL (it, deps) {
        // If the mask is empty, we weren't able to determine the size of the
        // mask on this output when traversing. But the only time we aren't able
        // to infer a mask for an output is when nothing is connected to it.
        // Because this is used only to find things that are reachable from a
        // leaf node, we expect to never have disconnected outputs in our cache.
        const VdfMask &mask = it->second;
        if (!TF_VERIFY(!mask.IsEmpty())) {
            continue;
        }
        invalidOutputs.push_back(VdfMaskedOutput(
            const_cast<VdfOutput *>(it->first), mask));
    }

    // Keep track of the number of bytes that have been invalidated.
    std::atomic<size_t> bytesInvalidated(0);

    // Make sure to release the python lock on this thread, so that dropping
    // python objects does not result in a deadlock.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Do the actual work of invalidating the individual pages.
    WorkParallelForN(
        work.size(),
        EfPageCacheStorage_InvalidateWorker(
            work, invalidOutputs, &bytesInvalidated));

    // Account for the free'd memory
    _numBytesUsed -= bytesInvalidated;
}

const VdfRequest &
EfPageCacheStorage::GetCacheableRequest(const VdfRequest &request) const
{
    TRACE_FUNCTION();

    // The current version of the leaf node cache. This covers any topological
    // edits, along with property time dependency changes.
    const size_t currentVersion = _leafNodeCache->GetVersion();

    // Lookup the entry in the cache. If the cached network version is
    // out-of-date we treat the lookup as a cache miss. Otherwise return the
    // cached entry.
    _CacheableRequestEntry *entry = nullptr;
    if (_cacheableRequests.Lookup(request, &entry) &&
            entry->version == currentVersion) {
        return entry->request;
    }

    TRACE_FUNCTION_SCOPE("cache miss");

    // Update the leaf node cache version.
    entry->version = currentVersion;

    // Add all the entries from the specified request to the cached request. It
    // is important that the specified request, and the cached request use the
    // same index space.
    VdfRequest *result = &entry->request;
    *result = request;
    result->AddAll();

    // Now iterate over all the entries in the request, and determine whether
    // the output is dependent on the key output. Only outputs that are
    // dependent on the key output are considered cacheable. We will remove all
    // other outputs from the request.
    const VdfOutputToMaskMap &deps =
        _FindDependencies(VdfMaskedOutputVector(1, _keyMaskedOutput));
    VdfRequest::const_iterator it = result->begin();
    const VdfRequest::const_iterator end = result->end();
    for (; it != end; ++it) {
        if (!deps.count(it->GetOutput())) {
            result->Remove(it);
        }
    }

    // Return the updated, cached request.
    return *result;
}

class EfPageCacheStorage_GetCachedWorker
{
public:
    struct WorkUnit {
        WorkUnit(const VdfVector *key, Ef_OutputValueCache *cache) :
            key(key), cache(cache), isCached(true)
        {}

        // The page key.
        const VdfVector *key;

        // The output value cache on this page.
        Ef_OutputValueCache *cache;

        // The cache status.
        bool isCached;
    };

    typedef std::vector<WorkUnit> WorkVector;

    EfPageCacheStorage_GetCachedWorker(
        const VdfRequest &cacheableRequest,
        WorkVector *work) :
        _cacheableRequest(cacheableRequest),
        _work(work)
    {}

    void operator()(size_t begin, size_t end) {
        // Iterate over the subset of work units.
        for (size_t i = begin; i != end; ++i) {

            // Gain exclusive access to the output-to-value cache.
            Ef_OutputValueCache::ExclusiveAccess cacheAccess((*_work)[i].cache);

            // For each output in the cacheable request, determine if it has
            // been cached.
            TF_FOR_ALL (it, _cacheableRequest) {

                // If the output has not been cached, reset the cache status
                // and bail out early.
                // By default, all pages in the work vector are considered
                // cached.
                if (!cacheAccess.GetValue(*it->GetOutput(), it->GetMask())) {
                    (*_work)[i].isCached = false;
                    break;
                }
            }
        }
    }

private:
    const VdfRequest &_cacheableRequest;
    WorkVector *_work;
};

bool
EfPageCacheStorage::GetCachedKeys(
    const _CacheIteratorPredicateFunction &predicate,
    const VdfRequest &request,
    std::vector<const VdfVector *> *cachedKeys) const
{
    if (request.IsEmpty()) {
        return false;
    }

    TRACE_FUNCTION();

    // Filter the specified request by the set of cacheable outputs.
    const VdfRequest &cacheableRequest = GetCacheableRequest(request);

    // If there are no outputs to cache, return false.
    if (cacheableRequest.IsEmpty()) {
        return false;
    }

    // Determine the units of work, by iterating over all pages, and extracting
    // the page key and output-value-cache pointer for only those pages that
    // have been selected by the predicate.
    EfPageCacheStorage_GetCachedWorker::WorkVector work;
    TF_FOR_ALL (it, *_pageCache) {
        Ef_OutputValueCache::ExclusiveAccess cacheAccess(it->second);

        if (!cacheAccess.IsEmpty() && predicate(it->first->GetValue())) {
            work.push_back(
                EfPageCacheStorage_GetCachedWorker::WorkUnit(
                    &it->first->GetValue(), it->second));
        }        
    }

    // Do the work of determining which pages are cached entirely.
    WorkParallelForN(
        work.size(),
        EfPageCacheStorage_GetCachedWorker(cacheableRequest, &work));

    // Iterate over the completed units of work and if the page has been
    // cached, add it's key to the result set.
    TF_FOR_ALL (it, work) {
        if (it->isCached) {
            cachedKeys->push_back(it->key);
        } 
    }

    // Return true to indicate that there are indeed some outputs to cache.
    return true;
}

void
EfPageCacheStorage::Clear()
{
    // Clear the page cache and keep track of the free'd memory.
    _numBytesUsed -= _pageCache->Clear();

    // Clear the node references.
    for (size_t i = 0; i < _numNodeRefs; ++i) {
        _nodeRefs[i].store(false, std::memory_order_relaxed);
    }

    // Make sure that all writes (to _numBytesUsed) have been retired.
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

void
EfPageCacheStorage::ClearNodes(
    const VdfNetwork &network,
    const tbb::concurrent_vector<VdfIndex> &nodes)
{
    TRACE_FUNCTION();

    // Make sure to release the python lock on this thread, so that dropping
    // python objects does not result in a deadlock.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // First invert the node indices to node pointers, but only for the nodes
    // that actually have output values stored in this cache. The resulting
    // container should be much smaller than the number of nodes provided.
    std::vector<const VdfNode *> referenced;
    for (const VdfIndex nodeIndex : nodes) {
        if (nodeIndex < _numNodeRefs &&
            _nodeRefs[nodeIndex].load(std::memory_order_acquire)) {
            referenced.push_back(network.GetNode(nodeIndex));
            _nodeRefs[nodeIndex].store(false, std::memory_order_release);
        }
    }
    
    // Invalidate all the output values for all the referenced nodes.
    std::atomic<size_t> *numBytesUsed = &_numBytesUsed;
    WorkParallelForEach(_pageCache->begin(), _pageCache->end(),
        [numBytesUsed, &referenced] (Ef_PageCache::CacheMap::value_type &page) {
            Ef_OutputValueCache::ExclusiveAccess cacheAccess(page.second);
            if (!cacheAccess.IsEmpty()) {
                for (const VdfNode *node : referenced) {
                    // Invalidate all outputs on this node.
                    size_t bytesInvalidated = 0;
                    for (const std::pair<TfToken, VdfOutput *> &o :
                            node->GetOutputsIterator()) {
                        bytesInvalidated += cacheAccess.Invalidate(*o.second);
                    }

                    // Account for the free'd memory. We try to write to the
                    // atomic as infrequently as possible, in an effort to avoid
                    // costly ping-pong'ing of the associated cache line.
                    if (bytesInvalidated) {
                        numBytesUsed->fetch_sub(
                            bytesInvalidated,
                            std::memory_order_relaxed);
                    }
                }
            }
        });
}

void
EfPageCacheStorage::Resize(const VdfNetwork &network)
{
    const size_t numNodes = network.GetNodeCapacity();

    // Whenever the network is resized, make sure that the _nodeRefs array is
    // still appropriately sized. Note, that we only do this from the main
    // thread, and when all background threads are stopped. Otherwise, the
    // _nodeRefs array could be in use by a background thread.
    if (numNodes < _numNodeRefs) {
        return;
    }

    // TODO: We can convert this to using a tbb::concurrent_vector, and
    // dynamically resize _nodeRefs on access, rather than pre-size it here.
    // We should revisit this once std::atomic_ref becomes available to us, so
    // that we can work around the issue with concurrent_vector not
    // synchronizing on element construction.

    // Allocate a new array. Over-allocate, so not to re-allocate
    // the array every time a few nodes are added to an existing network, e.g.,
    // small incremental compilation after major first-time compilation.
    // However, the constant below is just a guess at what might be sufficient
    // over-allocation without going overboard.
    const size_t newSize = numNodes + 1000;
    std::unique_ptr<std::atomic<bool>[]> newNodeRefs(
        new std::atomic<bool>[newSize]);

    // Copy all the existing values into the new array.
    for (size_t i = 0; i < _numNodeRefs; ++i) {
        newNodeRefs[i].store(
            _nodeRefs[i].load(std::memory_order_relaxed),
            std::memory_order_relaxed);
    }

    // Set all the tail in the new array to the initial values.
    for (size_t i = _numNodeRefs; i < newSize; ++i) {
        newNodeRefs[i].store(false, std::memory_order_relaxed);
    }

    // Swap the array, and set the new size.
    _nodeRefs.swap(newNodeRefs);
    _numNodeRefs = newSize;
}

void
EfPageCacheStorage::WillDeleteNode(const VdfNode &node)
{
    // Retrieve the node index.
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());

    // If this node has not been referenced in the cache, we can bail out
    // right away. Note, that this is an acceleration structure. We add
    // node references, but never remove them unless the node was deleted.
    // However, this prunes about 99% of the nodes in the network.
    if (nodeIndex >= _numNodeRefs ||
        !_nodeRefs[nodeIndex].load(std::memory_order_acquire)) {
        return;
    }

    TRACE_FUNCTION();

    // Make sure to release the python lock on this thread, so that dropping
    // python objects does not result in a deadlock.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Invalidate all the data stored for the deleted node.
    std::atomic<size_t> *numBytesUsed = &_numBytesUsed;
    WorkParallelForEach(_pageCache->begin(), _pageCache->end(),
        [&node, numBytesUsed](Ef_PageCache::CacheMap::value_type &page){
            Ef_OutputValueCache::ExclusiveAccess cacheAccess(page.second);
            if (!cacheAccess.IsEmpty()) {
                // Invalidate all outputs on this node.
                size_t bytesInvalidated = 0;
                for (const std::pair<TfToken, VdfOutput *> &o :
                        node.GetOutputsIterator()) {
                    bytesInvalidated += cacheAccess.Invalidate(*o.second);
                }

                // Account for the free'd memory. We try to write to the atomic
                // as infrequently as possible, in an effort to avoid costly
                // ping-pong'ing of the associated cache line.
                if (bytesInvalidated) {
                    numBytesUsed->fetch_sub(
                        bytesInvalidated,
                        std::memory_order_relaxed);
                }
            }
        });

    // Remove the node reference.
    _nodeRefs[nodeIndex].store(false, std::memory_order_release);
}

bool
EfPageCacheStorage::_IsKeyOutput(
    const VdfOutput &output,
    const VdfMask &mask) const
{
    return
        _keyMaskedOutput.GetOutput() == &output &&
        _keyMaskedOutput.GetMask() == mask;
}

Ef_OutputValueCache *
EfPageCacheStorage::_GetOrCreateCache(const VdfVector &key)
{
    return _pageCache->GetOrCreate(key);
}

const VdfOutputToMaskMap &
EfPageCacheStorage::_FindDependencies(
    const VdfMaskedOutputVector &outputs) const
{
    // If the request is for the key dependencies, dispatch to the
    // incrementally updated dependency cache.
    if (outputs.size() == 1 && outputs[0] == _keyMaskedOutput) {
        return _leafNodeCache->FindOutputs(
            outputs, /* updateIncrementally = */ true);
    }

    return _leafNodeCache->FindOutputs(
        outputs, /* updateIncrementally = */ false);
}

size_t
EfPageCacheStorage::_Commit(
    const VdfExecutorInterface &executor,
    const VdfRequest &request,
    Ef_OutputValueCache::ExclusiveAccess *cacheAccess)
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Ef", "EfPageCacheStorage::_Commit (vectorized)");

    // Keep track of the number of bytes stored.
    size_t bytesStored = 0;

    // Iterate over the request and commit all relevant data to the cache
    TF_FOR_ALL (it, request) {
        const VdfOutput *output = it->GetOutput();

        // Get the output value from the specified executor, store it in
        // the output-to-value cache.
        const VdfMask &mask = it->GetMask();
        if (const VdfVector *value =
            executor.GetOutputValue(*output, mask)) {
            // The node references vector is expected to be appropriately sized.
            // Note, this will be called from multiple threads, so the vector
            // cannot be dynamically re-sized here.
            const VdfIndex nodeIndex =
                VdfNode::GetIndexFromId(output->GetNode().GetId());
            if (!TF_VERIFY(nodeIndex < _numNodeRefs)) {
                continue;
            }

            // Note that we store the requested output value in its
            // entirety, rather than merely the time dependent bits in the
            // mask.
            // We do this because in order for lookups from the executor to get
            // a cache hit, all the data requested at an output must be
            // available.
            bytesStored += cacheAccess->SetValue(*output, *value, mask);

            // Mark the owning node as referenced.
            _nodeRefs[nodeIndex].store(true, std::memory_order_release);
        }
    }

    // Account for the additional memory used
    _numBytesUsed += bytesStored;

    return bytesStored;
}

size_t
EfPageCacheStorage::_Commit(
    const VdfMaskedOutput &maskedOutput,
    const VdfVector &value,
    Ef_OutputValueCache::ExclusiveAccess *cacheAccess)
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Ef", "EfPageCacheStorage::_Commit");

    const VdfOutput *output = maskedOutput.GetOutput();
    const VdfMask &mask = maskedOutput.GetMask();

    // The node references vector is expected to be appropriately sized.
    // Note, this will be called from multiple threads, so the vector cannot be
    // dynamically re-sized here.
    const VdfIndex nodeIndex =
        VdfNode::GetIndexFromId(output->GetNode().GetId());
    if (!TF_VERIFY(nodeIndex < _numNodeRefs)) {
        return 0;
    } 

    // Note that we store the requested output value in its entirety, rather
    // than merely the time dependent bits in the mask. We do this because in
    // order for lookups from the executor to get a cache hit, all the data
    // requested at an output must be available.
    const size_t bytesStored = cacheAccess->SetValue(*output, value, mask);

    // Keep track of the number of bytes stored, if any. Avoid any redundant
    // writes to _numByesUsed, because it is an atomic variable.
    if (bytesStored) {
        _numBytesUsed += bytesStored;
    }

    // Mark the owning node as referenced.
    _nodeRefs[nodeIndex].store(true, std::memory_order_release);

    return bytesStored;
}

PXR_NAMESPACE_CLOSE_SCOPE
