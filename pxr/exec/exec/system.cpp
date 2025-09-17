//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/system.h"

#include "pxr/exec/exec/compiler.h"
#include "pxr/exec/exec/invalidationResult.h"
#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/requestImpl.h"
#include "pxr/exec/exec/requestTracker.h"
#include "pxr/exec/exec/runtime.h"
#include "pxr/exec/exec/timeChangeInvalidationResult.h"

#include "pxr/base/tf/functionRef.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/exec/ef/time.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

ExecSystem::ExecSystem(EsfStage &&stage)
    : _stage(std::move(stage))
    , _program(std::make_unique<Exec_Program>())
    , _runtime(std::make_unique<Exec_Runtime>(
        _program->GetTimeInputNode(),
        _program->GetLeafNodeCache()))
    , _requestTracker(std::make_unique<Exec_RequestTracker>())
{
    _ChangeTime(EfTime());
}

ExecSystem::~ExecSystem() = default;

void
ExecSystem::_ChangeTime(const EfTime &newTime)
{
    const auto [timeChanged, oldTime] =
        _runtime->SetTime(_program->GetTimeInputNode(), newTime);
    if (!timeChanged) {
        return;
    }

    TRACE_FUNCTION();

    // Invalidate time on the program.
    const Exec_TimeChangeInvalidationResult invalidationResult =
        _program->InvalidateTime(oldTime, newTime);

    // Invalidate the executor and send request invalidation notification.
    WorkWithScopedDispatcher(
        [&runtime = _runtime, &invalidationResult,
         &requestTracker = _requestTracker]
        (WorkDispatcher &dispatcher){
        // Invalidate values on the executor.
        dispatcher.Run([&](){
            runtime->InvalidateExecutor(invalidationResult.invalidationRequest);
        });

        // Notify all the requests of the time change. Not all the requests will
        // contain all the leaf nodes affected by the time change, and the
        // request impls are responsible for filtering the provided information.
        if (!invalidationResult.invalidLeafNodes.empty()) {
            dispatcher.Run([&] {
                requestTracker->DidChangeTime(invalidationResult);
            });
        }
    });
}

void
ExecSystem::_Compute(
    const VdfSchedule &schedule,
    const VdfRequest &computeRequest)
{
    TRACE_FUNCTION();

    // Reset the accumulated input nodes requiring invalidation on the program,
    // and retain the invalidation request for executor invalidation below.
    const VdfMaskedOutputVector invalidationRequest =
        _program->ResetInputNodesRequiringInvalidation();

    // Make sure that the executor data manager is properly invalidated for any
    // input nodes that were just initialized.
    _runtime->InvalidateExecutor(invalidationRequest);

    // Run the executor to compute the values.
    _runtime->ComputeValues(schedule, computeRequest);
}

void
ExecSystem::_ParallelForEachRequest(
    TfFunctionRef<void(Exec_RequestImpl&)> f) const
{
    _requestTracker->ParallelForEachRequest(f);
}

std::vector<VdfMaskedOutput>
ExecSystem::_Compile(TfSpan<const ExecValueKey> valueKeys)
{
    Exec_Compiler compiler(_stage, _program.get(), _runtime.get());
    return compiler.Compile(valueKeys);
}

bool
ExecSystem::_HasPendingRecompilation() const
{
    return !_program->GetInputsRequiringRecompilation().empty();
}

void
ExecSystem::_InvalidateAll()
{
    TRACE_FUNCTION();

    // Reset data structures in reverse order of construction.
    _requestTracker.reset();
    _runtime.reset();
    _program.reset();

    // Reconstruct the relevant data structures.
    _program = std::make_unique<Exec_Program>();
    _runtime = std::make_unique<Exec_Runtime>(
        _program->GetTimeInputNode(),
        _program->GetLeafNodeCache());
    _requestTracker = std::make_unique<Exec_RequestTracker>();

    // Initialize time with the default time.
    _ChangeTime(EfTime());
}

void
ExecSystem::_InvalidateDisconnectedInputs()
{
    TRACE_FUNCTION();

    Exec_DisconnectedInputsInvalidationResult invalidationResult =
        _program->InvalidateDisconnectedInputs();

    // Invalidate the executor and send request invalidation.
    WorkWithScopedDispatcher(
        [&runtime = _runtime, &invalidationResult,
         &requestTracker = _requestTracker]
        (WorkDispatcher &dispatcher){
        // Invalidate the executor data manager.
        dispatcher.Run([&](){
            runtime->InvalidateExecutor(
                invalidationResult.invalidationRequest);
        });

        // Invalidate values in the page cache.
        dispatcher.Run([&](){
            runtime->InvalidatePageCache(
                invalidationResult.invalidationRequest,
                EfTimeInterval::GetFullInterval());
        });

        // Notify all the requests of computed value invalidation. Not all the
        // requests will contain all the invalid leaf nodes, and the request
        // impls are responsible for filtering the provided information.
        dispatcher.Run([&] {
            requestTracker->DidInvalidateComputedValues(invalidationResult);
        });

    });
}

void
ExecSystem::_InvalidateAttributeValues(TfSpan<const SdfPath> invalidAttributes)
{
    TRACE_FUNCTION();

    const Exec_AttributeValueInvalidationResult invalidationResult =
        _program->InvalidateAttributeAuthoredValues(invalidAttributes);

    // Invalidate the executor and send request invalidation.
    WorkWithScopedDispatcher(
        [&runtime = _runtime, &invalidationResult,
         &requestTracker = _requestTracker]
        (WorkDispatcher &dispatcher){
        // If any of the inputs to exec changed to be time dependent when
        // previously they were not (or vice versa), we need to invalidate the
        // main executor's topological state, such that invalidation traversals
        // pick up the new time dependency.
        if (invalidationResult.isTimeDependencyChange) {
            dispatcher.Run([&](){
                runtime->InvalidateTopologicalState();
            });
        }

        // Invalidate values in the page cache.
        dispatcher.Run([&](){
            runtime->InvalidatePageCache(
                invalidationResult.invalidationRequest,
                invalidationResult.invalidInterval);
        });

        // Notify all the requests of computed value invalidation. Not all the
        // requests will contain all the invalid leaf nodes or invalid
        // attributes, and the request impls are responsible for filtering the
        // provided information.
        requestTracker->DidInvalidateComputedValues(invalidationResult);
    });
}

void
ExecSystem::_InvalidateMetadataValues(
    TfSpan<const std::pair<SdfPath, TfToken>> invalidObjects)
{
    TRACE_FUNCTION();

    const Exec_MetadataInvalidationResult invalidationResult =
        _program->InvalidateMetadataValues(invalidObjects);

    const EfTimeInterval fullTimeInterval = EfTimeInterval::GetFullInterval();

    // Invalidate the executor and send request invalidation.
    WorkWithScopedDispatcher(
        [&runtime = _runtime, &invalidationResult,
         &requestTracker = _requestTracker,
         &fullTimeInterval]
        (WorkDispatcher &dispatcher){
        // Invalidate values in the page cache.
        dispatcher.Run([&](){
            runtime->InvalidatePageCache(
                invalidationResult.invalidationRequest,
                fullTimeInterval);
        });

        // Notify all the requests of computed value invalidation. Not all the
        // requests will contain all the invalid leaf nodes, and the request
        // impls are responsible for filtering the provided information.
        requestTracker->DidInvalidateComputedValues(invalidationResult);
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
