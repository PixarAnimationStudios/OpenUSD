//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/interruptState.h"

#include "pxr/exec/exec/compilationState.h"

#include <atomic>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

Exec_InterruptState::Exec_InterruptState(
    Exec_CompilationState *const compilationState)
    : _compilationState(compilationState)
    , _wasInterrupted(false)
{}

void
Exec_InterruptState::Interrupt()
{
    // Sets the flag to indicate that the current round of compilation has been
    // interrupted. This should only happen once.
    const bool wasAlreadyInterrupted =
        _wasInterrupted.exchange(true, std::memory_order_release);
    if (!TF_VERIFY(!wasAlreadyInterrupted)) {
        return;
    }

    // We mark done every key in every task sync while holding a busy scope.
    // It's possible that an unblocked task runs to completion and detects
    // a cycle among the remaining tasks, only because we haven't got around to
    // unblocking them yet. By creating a busy scope, we ensure that no cycles
    // can be detected until we finish unblocking ALL the waiting tasks.
    const auto busyScope =
        _compilationState->GetTaskCycleDetector().NewBusyScope();

    // Mark done every key in every Exec_CompilerTaskSync<T>. If any tasks are
    // stuck in a task cycle, this will break those cycles, allowing those tasks
    // to complete.
    Exec_CompilationState::TaskSyncAccess::_GetOutputProvidingTaskSync(
        _compilationState).MarkAllDone();
    Exec_CompilationState::TaskSyncAccess::_GetInputRecompilationTaskSync(
        _compilationState).MarkAllDone();
    Exec_CompilationState::TaskSyncAccess::_GetCycleDetectingTaskSync(
        _compilationState).MarkAllDone();
}

void
Exec_InterruptState::GetPotentiallyIsolatedNodes(
    std::unordered_set<VdfNode *> *const out) const
{
    out->insert(
        _potentiallyIsolatedNodes.begin(),
        _potentiallyIsolatedNodes.end());
}

void
Exec_InterruptState::GetInputsRequiringRecompilation(
    std::unordered_set<VdfInput *> *const out) const
{
    out->insert(
        _inputsRequiringRecompilation.begin(),
        _inputsRequiringRecompilation.end());
}

void
Exec_InterruptState::AddPotentiallyIsolatedNode(VdfNode *const node)
{
    _potentiallyIsolatedNodes.push_back(node);
}

void
Exec_InterruptState::AddInputRequiringRecompilation(VdfInput *const input)
{
    _inputsRequiringRecompilation.push_back(input);
}

PXR_NAMESPACE_CLOSE_SCOPE
