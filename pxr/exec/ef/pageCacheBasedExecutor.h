//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_PAGE_CACHE_BASED_EXECUTOR_H
#define PXR_EXEC_EF_PAGE_CACHE_BASED_EXECUTOR_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/outputValueCache.h"
#include "pxr/exec/ef/pageCacheStorage.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/dataManagerBasedExecutor.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/types.h"

#include <tbb/concurrent_hash_map.h>

#include <atomic>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorErrorLogger;

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfPageCacheBasedExecutor
///
/// \brief Executes a VdfNetwork to compute a requested set of values. Caches
///        the computed data in a EfPageCacheStorage container and recalls
///        existing data using a page specified via the currently set value
///        on the key output.
///
template <
    template <typename> class EngineType,
    typename DataManagerType>
class EfPageCacheBasedExecutor : 
    public VdfDataManagerBasedExecutor<DataManagerType, VdfExecutorInterface>
{
    typedef
        VdfDataManagerBasedExecutor<DataManagerType, VdfExecutorInterface>
        Base;

public:
    /// Constructor.
    ///
    EfPageCacheBasedExecutor(
        EfPageCacheStorage *cacheStorage) :
        _engine(*this, &this->_dataManager),
        _cacheStorage(cacheStorage),
        _currentCache(NULL),
        _numLoftedNodeRefs(0)
    {
        // The pointer to the page cache storage container must always be valid.
        TF_VERIFY(_cacheStorage);
    }

    /// Destructor.
    ///
    virtual ~EfPageCacheBasedExecutor() {}

    /// Set an output value.
    ///
    /// Changes the currently selected page in the page cache, if \p output
    /// is a key output in the EfPageCacheStorage container.
    ///
    virtual void SetOutputValue(
        const VdfOutput &output,
        const VdfVector &value,
        const VdfMask &mask);

    /// Removes any internal references to \p node upon deleting the
    /// node from the VdfNetwork.
    ///
    void WillDeleteNode(const VdfNode &node);

protected:
    // Returns a value for the cache that flows across \p connection.
    virtual const VdfVector *_GetInputValue(
        const VdfConnection &connection,
        const VdfMask &mask) const override;

    // Returns an output value for reading.
    virtual const VdfVector *_GetOutputValueForReading(
        const VdfOutput &output,
        const VdfMask &mask) const override;
    
    // Clear all data in the local data manager.
    virtual void _ClearData();

private:
    // Run the specified schedule.
    virtual void _Run(
        const VdfSchedule &schedule,
        const VdfRequest &computeRequest,
        VdfExecutorErrorLogger *errorLogger);

    // Returns \c true if the values resulting from evaluation should be stored
    // away in the page cache.
    bool _IsCaching() const;

    // Query the page cache for a value.
    const VdfVector *_GetPageCacheValue(
        const VdfOutput &output,
        const VdfMask &mask) const;

    // Add an output to the set of lofted outputs. Returns \c true if this
    // operation succeeds.
    bool _AddLoftedOutput(
        const VdfOutput *output,
        const VdfMask &mask) const;

    // Remove an output from the set of lofted outputs.
    void _RemoveLoftedOutput(
        VdfId outputId,
        VdfId nodeId,
        const VdfMask &mask);

    // Clear the set of lofted outputs.
    void _ClearLoftedOutputs();

    // Resize the atomic array _loftedNodeRefs, which grows to accommodate 
    // the maximum capacity of the network.
    void _ResizeLoftedReferences(const VdfNetwork &network);

    // Executor data invalidation
    bool _InvalidateOutput(
        const VdfOutput &output,
        const VdfMask &invalidationMask) override;

    // Pre-process executor invalidation by augmenting the invalidation
    // request to also invalidate any lofted outputs.
    virtual bool _PreProcessInvalidation(
        const VdfMaskedOutputVector &invalidationRequest,
        VdfMaskedOutputVector *processedRequest);

    // Clear data at a specified output.
    void _ClearDataForOutput(
        VdfId outputId, VdfId nodeId) override;

private:
    // The executor engine.
    EngineType<DataManagerType> _engine;

    // The page cache storage container.
    EfPageCacheStorage *_cacheStorage;

    // The output value cache for the currently selected page.
    Ef_OutputValueCache *_currentCache;

    // A set of outputs, which had their values sourced from the page cache
    // during evaluation (or getting of output values.) We need to keep track
    // of these outputs in order to allows us to later properly invalidate them.
    using _LoftedOutputsMap = tbb::concurrent_hash_map<VdfId, VdfMask>;
    mutable _LoftedOutputsMap _loftedOutputs;

    // An array of node references used to accelerate lookups into the
    // _loftedOutputs map.
    mutable std::unique_ptr<std::atomic<uint32_t>[]> _loftedNodeRefs;

    // The size of _loftedNodeRefs, which grows to accommodate the network's 
    // maximum capacity. 
    mutable size_t _numLoftedNodeRefs;

};

///////////////////////////////////////////////////////////////////////////////

template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::SetOutputValue(
    const VdfOutput &output,
    const VdfVector &value,
    const VdfMask &mask)
{
    // Call through to the base class, to set the output value in the
    // local data manager.
    Base::SetOutputValue(output, value, mask);

    // If the output we are setting a new value on is the key output, then
    // also make sure to set the currently selected page in the page cache
    // storage container.
    if (_cacheStorage->_IsKeyOutput(output, mask)) {
        _currentCache = _cacheStorage->_GetOrCreateCache(value);
    }
}

template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::WillDeleteNode(
    const VdfNode &node)
{
    // If there are no lofted outputs, bail out right away.
    if (_loftedOutputs.empty()) {
        return;
    }

    // If the node is not referenced, bail out right away.
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());
    if (_numLoftedNodeRefs <= nodeIndex || !_loftedNodeRefs[nodeIndex]) {
        return;
    }

    TRACE_FUNCTION_SCOPE("removing lofted outputs");

    // Iterate over all outputs on the node, and remove them
    // from the set of lofted outputs.
    for (const std::pair<TfToken, VdfOutput *> &i : node.GetOutputsIterator()) {
        _loftedOutputs.erase(i.second->GetId());
    }

    // Reset the node reference count to 0 to indicate that no
    // outputs have been lofted from this node.
    _loftedNodeRefs[nodeIndex] = 0;
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
const VdfVector *
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_GetInputValue(
    const VdfConnection &connection,
    const VdfMask &mask) const
{
    // Note, this method will be called concurrently, if the engine type is
    // a parallel engine.

    // First, look for the value in the local data manager.
    if (const VdfVector *value =
            Base::_dataManager.GetInputValue(connection, mask)) {
        return value;
    }

    // Then, query the page cache storage.
    return _GetPageCacheValue(connection.GetSourceOutput(), mask);
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
const VdfVector *
EfPageCacheBasedExecutor<EngineType, DataManagerType>::
    _GetOutputValueForReading(
        const VdfOutput &output,
        const VdfMask &mask) const
{
    // Note, this method will be called concurrently, if the engine type is
    // a parallel engine.

    // First, look for the value in the local data manager.
    if (const VdfVector *value =
            Base::_dataManager.GetOutputValueForReading(
                Base::_dataManager.GetDataHandle(output.GetId()), mask)) {
        return value;
    }

    // Then, query the page cache storage.
    return _GetPageCacheValue(output, mask);
}

template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_Run(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    VdfExecutorErrorLogger *errorLogger)
{    
    // If nothing has been requested, bail out early.
    if (computeRequest.IsEmpty()) {
        return;
    }

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Ef", "EfPageCacheBasedExecutor::Run");

    // Make sure the lofted node references array is sufficiently large.
    _ResizeLoftedReferences(*schedule.GetNetwork());

    // If caching is enabled, run the schedule with a callback that writes each
    // computed output value to the page cache.
    if (TF_VERIFY(_currentCache) && _IsCaching()) {
        const DataManagerType &dataManager = Base::_dataManager;
        const VdfRequest &cacheableRequest =
            _cacheStorage->GetCacheableRequest(computeRequest);
        VdfRequest::IndexedView cacheableView(cacheableRequest);
        Ef_OutputValueCache *cache = _currentCache;
        EfPageCacheStorage *storage = _cacheStorage;

        _engine.RunSchedule(
            schedule, computeRequest, errorLogger,
            [&dataManager, &cacheableView, cache, storage]
            (const VdfMaskedOutput &mo, size_t requestedIndex) {
                // Lookup the value in the local data manager. This is the
                // value to store away in the page cache. Bail out if no such
                // value exists, or if the output is not cacheable.
                const VdfVector *value = dataManager.GetOutputValueForReading(
                    dataManager.GetDataHandle(mo.GetOutput()->GetId()),
                    mo.GetMask());
                if (!value || !cacheableView.Get(requestedIndex)) {
                    return;
                }

                // If the output has already been cached, bail out.
                {
                    Ef_OutputValueCache::SharedAccess access(cache);
                    if (access.GetValue(*mo.GetOutput(), mo.GetMask())) {
                        return;
                    }
                }

                // Attempt to cache the value in the page cache.
                Ef_OutputValueCache::ExclusiveAccess access(cache);
                storage->_Commit(mo, *value, &access);
            });
    }

    // If caching is not enabled, run the schedule without a callback.
    else {
        _engine.RunSchedule(schedule, computeRequest, errorLogger);
    }
}

template <template <typename> class EngineType, typename DataManagerType>
bool
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_IsCaching() const
{
    return
        _cacheStorage->IsEnabled() &&
        !EfPageCacheStorage::HasReachedMemoryLimit();
}

template <template <typename> class EngineType, typename DataManagerType>
const VdfVector *
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_GetPageCacheValue(
    const VdfOutput &output,
    const VdfMask &mask) const
{
    // Note, this method will be called concurrently, if the engine type is
    // a parallel engine.

    // We only do this when the executor is running, because we never want
    // external clients to receive pointers to data in the page cache, in order
    // to avoid data races.
    if (!_currentCache) {
        return nullptr;
    }

    // Obtain shared read access to the current cache.
    Ef_OutputValueCache::SharedAccess cacheAccess(_currentCache);

    // Lookup the value in the current cache.
    if (const VdfVector *cachedValue = cacheAccess.GetValue(output, mask)) {
        // Mark this output as having been lofted into the data manager. We
        // cannot return a cache hit if this fails.
        if (_AddLoftedOutput(&output, mask)) {
            // Touch the output, so that invalidation will be able to
            // propagate down in the network.
            Base::_TouchOutput(output);

            // Return the cached value.
            return cachedValue;
        }
    }

    // No value found.
    return nullptr;
}

template <template <typename> class EngineType, typename DataManagerType>
bool
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_AddLoftedOutput(
    const VdfOutput *output,
    const VdfMask &mask) const
{
    // Note, this method will be called concurrently, if the engine type is
    // a parallel engine.
    
    // First make sure that the output can be lofted. _Run() is responsible for
    // resizing the _loftedNodeRefs array, but we may end up here before having
    // called run (e.g. client calling GetOutputValue() on this executor.)
    // Note, we could dynamically resize _loftedNodeRefs here as long as that
    // operation is thread safe. We are not currently doing that for performance
    // reasons.
    const VdfNode &node = output->GetNode();
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());
    if (_numLoftedNodeRefs <= nodeIndex) {
        return false;
    }

    // Try to insert the output and mask into the set of lofted outputs.
    {
        _LoftedOutputsMap::accessor accessor;

        // If the output had previously been inserted, simply append the mask
        // and we are done here.
        if (!_loftedOutputs.insert(accessor, { output->GetId(), mask })) {
            accessor->second.SetOrAppend(mask);
            return true;
        }
    }

    // If this is the first time this output is being inserted into the map,
    // make sure to also increment the node reference count for the node that
    // owns the output.

    // Increment the reference count.
    ++_loftedNodeRefs[nodeIndex];

    // Success
    return true;
}

template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_RemoveLoftedOutput(
    const VdfId outputId,
    const VdfId nodeId,
    const VdfMask &mask)
{
    // If the map of lofted outputs remains empty, we can bail out right away.
    if (_loftedOutputs.empty()) {
        return;
    }

    // Look at the reference count for the node owning this output. If the
    // node is not referenced, we can bail out without even looking at the map.
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(nodeId);
    if (_numLoftedNodeRefs <= nodeIndex || !_loftedNodeRefs[nodeIndex]) {
        return;
    }

    // Lookup the output in the map.
    _LoftedOutputsMap::accessor accessor;
    if (!_loftedOutputs.find(accessor, outputId)) {
        return;
    }

    // If the entire mask is being removed, simply drop the output from the map
    // and decrement the reference count for the owning node.
    if (accessor->second == mask || mask.IsEmpty()) {
        _loftedOutputs.erase(accessor);
        --_loftedNodeRefs[nodeIndex];
    }

    // If some subset of the mask is being removed, merely update the
    // stored mask.
    else {
        accessor->second -= mask;

        // If at this point the mask is all zeros, we can drop the output
        // entirely, and also decrement the reference count for the
        // node owning the output.
        if (accessor->second.IsAllZeros()) {
            _loftedOutputs.erase(accessor);
            --_loftedNodeRefs[nodeIndex];
        }
    }
}

template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_ClearLoftedOutputs()
{
    if (_loftedOutputs.empty() && _numLoftedNodeRefs == 0) {
        return;
    }

    TRACE_FUNCTION();

    _loftedOutputs.clear();

    WorkParallelForN(_numLoftedNodeRefs, 
        [&loftedNodeRefs = _loftedNodeRefs] (size_t begin, size_t end) {
            for (size_t i = begin; i != end; ++i) {
                loftedNodeRefs[i].store(0, std::memory_order_relaxed);
            }
        });
}

template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_ResizeLoftedReferences(
    const VdfNetwork &network)
{
    // This array is over-allocated to accommodate the maximum network 
    // capacity.
    const size_t newSize = network.GetNodeCapacity();
    if (newSize > _numLoftedNodeRefs) {
        auto * const newReferences = new std::atomic<uint32_t>[newSize];

        // Copy all the existing entries into the new array.
        for (size_t i = 0; i < _numLoftedNodeRefs; ++i) {
            newReferences[i].store(
                _loftedNodeRefs[i].load(
                    std::memory_order_relaxed),
                std::memory_order_relaxed);        
        }

        // Initialize the tail values in the new array. 
        for (size_t i = _numLoftedNodeRefs; i < newSize; ++i) {
            newReferences[i].store(0, std::memory_order_relaxed);
        }

        _loftedNodeRefs.reset(newReferences);
    }
    _numLoftedNodeRefs = newSize;
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
bool
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_InvalidateOutput(
    const VdfOutput &output,
    const VdfMask &invalidationMask)
{
    // Call into the base class for output invalidation.
    if (Base::_InvalidateOutput(output, invalidationMask)) {
        // If some data has been invalidated, make sure to also remove the
        // bits from the lofted output.
        _RemoveLoftedOutput(
            output.GetId(), output.GetNode().GetId(), invalidationMask);

        // Some data has been invalidated.
        return true;
    }

    // Nothing has been invalidated.
    return false;
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
bool
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_PreProcessInvalidation(
    const VdfMaskedOutputVector &invalidationRequest,
    VdfMaskedOutputVector *processedRequest)
{
    // Bail out, if there are no lofted outputs. Return false to indicate
    // that the originally supplied invalidationRequest shall be used.
    if (_loftedOutputs.empty()) {
        return false;
    }

    TRACE_FUNCTION();

    // Find all outputs depenending on the originally supplied request.
    const VdfOutputToMaskMap &deps =
        _cacheStorage->_FindDependencies(invalidationRequest);

    // If there are no dependent outputs, bail out.
    if (deps.empty()) {
        return false;
    }

    // Resize the new invalidation request to the maximum number of elements it
    // could possibly contain. We will populate the vector in parallel.
    const size_t maxNumRequest =
        std::min(_loftedOutputs.size(), deps.size()) +
        invalidationRequest.size();
    processedRequest->resize(maxNumRequest);

    // Keep track of how many entries in the new invalidation request have been
    // populated in parallel. We will use this to later trim the tail of the
    // vector.
    std::atomic<size_t> numRequest(0);

    // For each dependent output, determine if it has been lofted, and if so
    // add it to the invalidation request.
    const std::unique_ptr<std::atomic<uint32_t>[]> &loftedNodeRefs = _loftedNodeRefs;
    const size_t numLoftedNodeRefs = _numLoftedNodeRefs;
    const _LoftedOutputsMap &loftedOutputs = _loftedOutputs;

    WorkParallelForN(deps.bucket_count(),
        [&deps, &loftedNodeRefs, numLoftedNodeRefs, 
            &loftedOutputs, &numRequest, processedRequest]
        (size_t b, size_t e) {
            for (size_t i = b; i != e; ++i) {
                VdfOutputToMaskMap::const_local_iterator it = deps.begin(i);
                VdfOutputToMaskMap::const_local_iterator end = deps.end(i);
                for (; it != end; ++it) {
                    const VdfOutput *output = it->first;

                    // Is this output's node even referenced in the set of
                    // lofted outputs?
                    const VdfIndex idx = VdfNode::GetIndexFromId(
                        output->GetNode().GetId());
                    if (numLoftedNodeRefs <= idx || !loftedNodeRefs[idx]) {
                        continue;
                    }

                    // If this output has been lofted, add it to the
                    // invalidation request.
                    _LoftedOutputsMap::const_accessor accessor;
                    if (loftedOutputs.find(accessor, output->GetId())) {
                        (*processedRequest)[numRequest.fetch_add(1)] =
                            VdfMaskedOutput(
                                const_cast<VdfOutput*>(output),
                                it->second & accessor->second);
                    }
                }
            }
        });

    // If the invalidation request has not been augmented with lofted outputs,
    // we can simply bail out an instead use the originally supplied
    // invalidationRequest.
    const size_t num = numRequest.load(std::memory_order_relaxed);
    if (num == 0) {
        return false;
    }

    // Otherwise, add all the outputs from the original invalidationRequest
    // to the new invalidation request, which now also contains all the
    // dependent lofted outputs. Then, resize the new request in order to drop
    // the tail that has not been populated with elements.
    std::copy(
        invalidationRequest.begin(), invalidationRequest.end(),
        processedRequest->begin() + num);
    processedRequest->resize(num + invalidationRequest.size());

    // The invalidation request has been modified. Return true to tell
    // executor invalidation to use the processed request instead of the
    // originally supplied request.
    return true;
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_ClearDataForOutput(
    const VdfId outputId, const VdfId nodeId)
{
    Base::_ClearDataForOutput(outputId, nodeId);
    _RemoveLoftedOutput(outputId, nodeId, VdfMask());
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_ClearData()
{
    _ClearLoftedOutputs();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
