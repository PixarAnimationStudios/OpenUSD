//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executorInterface.h"

#include "pxr/exec/vdf/executorInvalidator.h"
#include "pxr/exec/vdf/executorObserver.h"
#include "pxr/exec/vdf/schedule.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfExecutorInterface::VdfExecutorInterface() :
    _stats(NULL),
    _executorInvalidationTimestamp(0),
    _parentExecutor(NULL),
    _interruptionFlag(NULL)
{
}

VdfExecutorInterface::VdfExecutorInterface(
    const VdfExecutorInterface *parentExecutor) :
    VdfExecutorInterface()
{
    SetParentExecutor(parentExecutor);
}

VdfExecutorInterface::~VdfExecutorInterface()
{
    std::lock_guard<std::recursive_mutex> lock(_observersLock);

    _Observers::iterator it = _observers.begin();
    while(it != _observers.end()) {
        _Observers::iterator prev = it;
        ++it;

        (*prev)->_OnExecutorDelete(this);
    }
}

void
VdfExecutorInterface::Run(
    const VdfSchedule &schedule, 
    VdfExecutorErrorLogger *errorLogger)
{
    _Run(schedule, schedule.GetRequest(), errorLogger);
}

void
VdfExecutorInterface::Run(
    const VdfSchedule &schedule,
    const VdfRequest &request,
    VdfExecutorErrorLogger *errorLogger)
{
    _Run(schedule, request, errorLogger);
}

void
VdfExecutorInterface::RegisterObserver(
    const VdfExecutorObserver *observer) const
{
    TRACE_FUNCTION();

    std::lock_guard<std::recursive_mutex> lock(_observersLock);

    // Only register the observer if it has not been registered before
    if (TF_VERIFY(_observers.count(observer) <= 0)) {
        _observers.insert(observer);
    }
}

void
VdfExecutorInterface::UnregisterObserver(
    const VdfExecutorObserver *observer) const
{
    TRACE_FUNCTION();

    std::lock_guard<std::recursive_mutex> lock(_observersLock);

    // Only unregister the observer if has not already been unregistered
    if (TF_VERIFY(_observers.count(observer) > 0)) {
        _observers.erase(observer);
    }

}

void 
VdfExecutorInterface::SetParentExecutor(
    const VdfExecutorInterface *parentExecutor)
{
    // Assign the parent executor
    _parentExecutor = parentExecutor;

    // Inherit the execution stats from the parent executor, if this executor
    // does not already have execution stats.
    if (parentExecutor && !GetExecutionStats()) {
        SetExecutionStats(parentExecutor->GetExecutionStats());
    }
}

void
VdfExecutorInterface::InvalidateValues(
    const VdfMaskedOutputVector &invalidationRequest)
{
    // Bail out if the executor is still empty.
    if (IsEmpty()) {
        return;
    }

    TRACE_FUNCTION();

    // Pre-processing can create a new invalidation request to override
    // the supplied invalidationRequest.
    VdfMaskedOutputVector processedInvalidationRequest;

    // Run invalidation pre-processing, which will return true if a new
    // invalidation request has been created.
    // Note, that we use the return value here, to decide whether to use
    // the processedInvalidationRequest, because we want to avoid copying
    // the entire invalidationRequest, if we don't absolutely have to.
    const bool didPreProcess =
        _PreProcessInvalidation(
            invalidationRequest,
            &processedInvalidationRequest);

    // If pre-processing overrode the invalidation request, but the
    // processed request is empty, we can bail out without doing any
    // invalidation.
    if (didPreProcess && processedInvalidationRequest.empty()) {
        return;
    }

    // Right before doing any invalidation traversal, update the current
    // invalidation timestamp, which will be written for every output that
    // we visit during this round of invalidation traversal.  This timestamp
    // will identify the outputs we touched in this most recent round of
    // invalidation.
    _UpdateInvalidationTimestamp();

    // Construct a new invalidator if there isn't already one. Some executors
    // never do any invalidation, so we do not want to pay for the cost of
    // constructing an invalidator ahead of time.
    if (!_invalidator) {
        _invalidator.reset(new VdfExecutorInvalidator(this));
    }

    // Push through the actual invalidation.
    _invalidator->Invalidate(didPreProcess
        ? processedInvalidationRequest
        : invalidationRequest);
}

void
VdfExecutorInterface::InvalidateTopologicalState()
{
    TRACE_FUNCTION();

    // Reset the invalidator, if there is one.
    if (_invalidator) {
        _invalidator->Reset();
    }
}

void
VdfExecutorInterface::ClearData()
{
    TRACE_FUNCTION();

    {
        std::lock_guard<std::recursive_mutex> lock(_observersLock);
    
        _Observers::iterator it = _observers.begin();
        while(it != _observers.end()) {
            _Observers::iterator prev = it;
            ++it;
    
            (*prev)->_OnExecutorClearData(this);
        }
    }

    _ClearData();
}

void
VdfExecutorInterface::_ClearData()
{
}

void
VdfExecutorInterface::ClearDataForOutput(
    const VdfId outputId,
    const VdfId nodeId)
{
    _ClearDataForOutput(outputId, nodeId);
}

void
VdfExecutorInterface::_ClearDataForOutput(
    const VdfId outputId,
    const VdfId nodeId)
{
}

PXR_NAMESPACE_CLOSE_SCOPE
