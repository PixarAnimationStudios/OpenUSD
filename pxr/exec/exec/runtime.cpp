//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/runtime.h"

#include "pxr/exec/exec/typeRegistry.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/ef/maskedSubExecutor.h"
#include "pxr/exec/ef/pageCacheExecutor.h"
#include "pxr/exec/ef/pageCacheStorage.h"
#include "pxr/exec/ef/subExecutor.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/ef/timeInputNode.h"
#include "pxr/exec/vdf/dataManagerHashTable.h"
#include "pxr/exec/vdf/dataManagerVector.h"
#include "pxr/exec/vdf/executorErrorLogger.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/parallelDataManagerVector.h"
#include "pxr/exec/vdf/parallelExecutorEngine.h"
#include "pxr/exec/vdf/parallelSpeculationExecutorEngine.h"
#include "pxr/exec/vdf/pullBasedExecutorEngine.h"
#include "pxr/exec/vdf/schedule.h"
#include "pxr/exec/vdf/typedVector.h"
#include "pxr/exec/vdf/types.h"

#include <memory>

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
    // If the schedule does not have a network, then we cannot compute anything.
    // This happens when we compute an ExecRequest that only contains empty
    // leaf outputs.
    if (!schedule.GetNetwork()) {
        return;
    }

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

std::unique_ptr<VdfExecutorInterface>
Exec_Runtime::ComputeWithOverrides(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest,
    const VdfMaskedOutputVector &overriddenOutputs,
    const std::vector<VtValue> &overriddenValues)
{
    TRACE_FUNCTION();

    // This function requires that each overridden output has a corresponding
    // override value.
    if (!TF_VERIFY(overriddenOutputs.size() == overriddenValues.size())) {
        return nullptr;
    }

    // Create a masked subexecutor that will be used for every call to
    // ComputeWithOverrides. This dataless executor maintains a mask of outputs
    // that are invalid (due to the overrides) without affecting the main
    // executor. This is a parent of the temporary executor created for only
    // this call to ComputeWithOverrides, and since that executor is returned to
    // the caller, we persist the parent executor so that its lifetime extends
    // beyond this function call.
    if (!_overridesExecutor) {
        _overridesExecutor =
            std::make_unique<EfMaskedSubExecutor>(_executor.get());
    }
    else {
        _overridesExecutor->ClearData();
    }

    // Create a temporary executor only used for this call to
    // ComputeWithOverrides. Overridden values, and computation results that
    // depend on overridden values are stored in this subexecutor, as to avoid
    // overwriting values in the main executor. This subexecutor will be
    // returned to the caller, so that computed values can be extracted. Use a
    // parallel executor engine if paralellism is enabled, but only if the
    // schedule is sufficiently large. Small schedules are evaluated more
    // efficiently on a single-threaded executor engine.
    std::unique_ptr<VdfExecutorInterface> subExecutor;
    if (VdfIsParallelEvaluationEnabled() && !schedule.IsSmallSchedule()) {
        using ExecutorType = EfSubExecutor<
            VdfParallelExecutorEngine, VdfParallelDataManagerVector>;
        subExecutor = std::make_unique<ExecutorType>(_overridesExecutor.get());
    }
    else {
        using ExecutorType = EfSubExecutor<
            VdfPullBasedExecutorEngine, VdfDataManagerHashTable>;
        subExecutor = std::make_unique<ExecutorType>(_overridesExecutor.get());
    }

    // Apply the overridden values.
    for (size_t i = 0; i < overriddenOutputs.size(); ++i) {
        const VdfMaskedOutput &maskedOutput = overriddenOutputs[i];
        if (!TF_VERIFY(maskedOutput)) {
            continue;
        }

        // Set the override value in the subexecutor.
        subExecutor->SetOutputValue(
            *maskedOutput.GetOutput(),
            ExecTypeRegistry::GetInstance().CreateVector(overriddenValues[i]),
            maskedOutput.GetMask());
    }

    // Invalidate all overridden outputs, and outputs dependent on overridden
    // outputs. Note that invalidation is handled by the masked overrides
    // executor, not the subexecutor.
    //
    // TODO: Currently, this may invalidate outputs that are not present in the
    // schedule that we are about to compute. For example, some of the overrides
    // may have no bearing on the request; or an overridden output may feed into
    // a large subnetwork of nodes that the request does not care about. In the
    // future, we can use a different executor type that knows to only
    // invalidate outputs belonging to a specific schedule, so that we avoid
    // the cost of invalidating outputs that are irrelevant to the request.
    _overridesExecutor->InvalidateValues(overriddenOutputs);

    // Compute the requested values on the subexecutor.
    VdfExecutorErrorLogger errorLogger;
    subExecutor->Run(schedule, computeRequest, &errorLogger);
    _ReportExecutorErrors(errorLogger);

    return subExecutor;
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
