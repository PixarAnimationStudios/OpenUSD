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

#include "pxr/exec/ef/loftedOutputSet.h"
#include "pxr/exec/ef/outputValueCache.h"
#include "pxr/exec/ef/pageCacheStorage.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/dataManagerBasedExecutor.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/output.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/types.h"

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
        _currentCache(NULL)
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
    mutable Ef_LoftedOutputSet _loftedOutputs;
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
    _loftedOutputs.RemoveAllOutputsForNode(node);
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
    _loftedOutputs.Resize(*schedule.GetNetwork());

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
        if (_loftedOutputs.Add(output, mask)) {
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
        _loftedOutputs.Remove(
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
    const size_t numLoftedOutputs = _loftedOutputs.GetSize();
    if (numLoftedOutputs == 0) {
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

    // Reserve space to guarantee that all outputs will fit without needing to
    // reallocate.
    processedRequest->reserve(
        invalidationRequest.size() +
        std::min(numLoftedOutputs, deps.size()));

    // If the invalidation request has not been augmented with lofted outputs,
    // we can simply bail out an instead use the originally supplied
    // invalidationRequest.
    _loftedOutputs.CollectLoftedDependencies(deps, processedRequest);
    if (processedRequest->empty()) {
        return false;
    }

    // Otherwise, add all the outputs from the original invalidationRequest
    // to the new invalidation request, which now also contains all the
    // dependent lofted outputs.
    processedRequest->insert(
        processedRequest->end(),
        invalidationRequest.begin(), invalidationRequest.end());

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
    _loftedOutputs.Remove(outputId, nodeId, VdfMask());
}

/* virtual */
template <template <typename> class EngineType, typename DataManagerType>
void
EfPageCacheBasedExecutor<EngineType, DataManagerType>::_ClearData()
{
    _loftedOutputs.Clear();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
