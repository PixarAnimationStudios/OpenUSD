//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/runtime.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/ef/pageCacheExecutor.h"
#include "pxr/exec/ef/pageCacheStorage.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/ef/timeInputNode.h"
#include "pxr/exec/vdf/dataManagerVector.h"
#include "pxr/exec/vdf/executorErrorLogger.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/parallelDataManagerVector.h"
#include "pxr/exec/vdf/parallelExecutorEngine.h"
#include "pxr/exec/vdf/parallelSpeculationExecutorEngine.h"
#include "pxr/exec/vdf/pullBasedExecutorEngine.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/typedVector.h"
#include "pxr/exec/vdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_Runtime::Exec_Runtime(
    EfTimeInputNode &timeNode,
    EfLeafNodeCache &leafNodeCache)
    : _executorTopologicalStateVersion(0)
{
    // Create a cache for time-varying computed values, indexed by time.
    _cacheStorage.reset(
        EfPageCacheStorage::New<EfTime>(
            VdfMaskedOutput(timeNode.GetOutput(), VdfMask::AllOnes(1)),
            &leafNodeCache));

    // Create a multi-threaded main executor, if parallel evaluation is
    // enabled.
    if (VdfIsParallelEvaluationEnabled()) {
        _executor = std::make_unique<
            EfPageCacheExecutor<
                VdfParallelExecutorEngine,
                VdfParallelDataManagerVector>>(
                    _cacheStorage.get());
    } 

    // Create a single-threaded main executor, if parallel evaluation is
    // disabled.
    else {
        _executor = std::make_unique<
            EfPageCacheExecutor<
                VdfPullBasedExecutorEngine,
                VdfDataManagerVector<
                    VdfDataManagerDeallocationMode::Background>>>(
                        _cacheStorage.get());
    }
}

Exec_Runtime::~Exec_Runtime() = default;

const VdfDataManagerFacade
Exec_Runtime::GetDataManager()
{
    return VdfDataManagerFacade(*_executor.get());
}

std::tuple<bool, EfTime>
Exec_Runtime::SetTime(const EfTimeInputNode &timeNode, const EfTime &time)
{
    const VdfOutput &timeOutput = *timeNode.GetOutput();
    const VdfMask timeMask = VdfMask::AllOnes(1);

    // Retrieve the old time vector from the executor data manager.
    // 
    // If there isn't already a time value stored in the executor data manager,
    // perform first time initialization and return. In this case, we don't
    // consider time as having changed.
    const VdfVector *const oldTimeVector = _executor->GetOutputValue(
        timeOutput, timeMask);
    if (!oldTimeVector) {
        _executor->SetOutputValue(
            timeOutput, VdfTypedVector<EfTime>(time), timeMask);
        return {false, EfTime()};
    }

    // Get the old time value from the vector. If there is no change in time,
    // we can return without setting the new time value.
    const EfTime oldTime = oldTimeVector->GetReadAccessor<EfTime>()[0];
    if (oldTime == time) {
        return {false, oldTime};
    }

    // Set the new time value and return.
    _executor->SetOutputValue(
        timeOutput, VdfTypedVector<EfTime>(time), timeMask);
    return {true, oldTime};
}

void
Exec_Runtime::InvalidateTopologicalState()
{
    _executor->InvalidateTopologicalState();
}

void
Exec_Runtime::InvalidateExecutor(
    const VdfMaskedOutputVector &invalidationRequest)
{
    if (invalidationRequest.empty()) {
        return;
    }

    // Get the current network version.
    const VdfNetwork &network =
        invalidationRequest.front().GetOutput()->GetNode().GetNetwork();
    const size_t networkVersion = network.GetVersion();

    // If the last recorded network version is different from the current
    // network version, we need to make sure to invalidate the main executor's
    // topological state before invalidating values.
    if (networkVersion != _executorTopologicalStateVersion) {
        _executor->InvalidateTopologicalState();
        _executorTopologicalStateVersion = networkVersion;
    }

    // Invalidate values on the main executor.
    _executor->InvalidateValues(invalidationRequest);
}

void
Exec_Runtime::InvalidatePageCache(
    const VdfMaskedOutputVector &invalidationRequest,
    const EfTimeInterval &timeInterval)
{
    _cacheStorage->Invalidate(
        [&timeInterval](const VdfVector &cacheKey){
            const EfTime &time = cacheKey.GetReadAccessor<EfTime>()[0];
            return timeInterval.Contains(time);
        },
        invalidationRequest);
}

void
Exec_Runtime::DeleteData(const VdfNode &node)
{
    for (const auto &[name, output] : node.GetOutputsIterator()) {
        _executor->ClearDataForOutput(output->GetId(), node.GetId());
    }

    _cacheStorage->WillDeleteNode(node);
}

void
Exec_Runtime::ComputeValues(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest)
{
    // Make sure that the cache storage is large enough to hold all possible
    // computed values in the network.
    _cacheStorage->Resize(*schedule.GetNetwork());

    // Run the executor to compute the values.
    VdfExecutorErrorLogger errorLogger;
    _executor->Run(schedule, computeRequest, &errorLogger);

    // Increment the executor's invalidation timestamp after each run. All
    // executor invalidation after this call will pick up the new timestamp,
    // ensuring that mung-buffer locking will take hold at invalidation edges.
    // 
    // Note, that all sub-executors must inherit the invalidation timestamp
    // (c.f., VdfExecutorInterface::InheritInvalidationTimestamp()) from their
    // parent executor for mung-buffer locking to function on sub-executors.
    _executor->IncrementExecutorInvalidationTimestamp();

    // Report any errors or warnings surfaced during this executor run.
    _ReportExecutorErrors(errorLogger);
}

void
Exec_Runtime::_ReportExecutorErrors(
    const VdfExecutorErrorLogger &errorLogger) const
{
    const VdfExecutorErrorLogger::NodeToStringMap &warnings =
        errorLogger.GetWarnings();
    if (warnings.empty()) {
        return;
    }

    TRACE_FUNCTION();

    for (auto &[node, error] : warnings) {
        if (!TF_VERIFY(node)) {
            continue;
        }

        TF_WARN("Node: '%s'. Exec Warning: %s",
            node->GetDebugName().c_str(),
            error.c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
