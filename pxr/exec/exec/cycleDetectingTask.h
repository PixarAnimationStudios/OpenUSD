//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_CYCLE_DETECTING_TASK_H
#define PXR_EXEC_EXEC_CYCLE_DETECTING_TASK_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/compilationTask.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

/// Used to detect cycles that pass through one or more recompiled inputs.
///
/// When recompiling an input, it's possible that the new dependencies are
/// already downstream of the input. Connecting the new dependencies to the
/// input would introduce a cycle.
///
/// Exec_CycleDetectingTasks traverse the network in the upstream direction
/// searching for inputs currently being recompiled. If found, the
/// Exec_CycleDetectingTask waits on the completion of the corresponding
/// Exec_InputRecompilationTasks for those inputs.
///
/// Exec_InputRecompilationTasks spawn Exec_CycleDetectingTasks for their new
/// upstream nodes before connecting them to the recompiled input. If the new
/// upstream nodes are also downstream of the recompiled input, then this will
/// introduce a task cycle to be handled by the Exec_TaskCycleDetector.
///
class Exec_CycleDetectingTask : public Exec_CompilationTask
{
public:
    Exec_CycleDetectingTask(
        Exec_CompilationState &compilationState,
        const VdfNode *const node)
        : Exec_CompilationTask(compilationState)
        , _node(node)
    {}

private:
    void _Compile(
        Exec_CompilationState &compilationState,
        TaskPhases &taskPhases) override;

private:
    // The task begins its search from this node.
    const VdfNode *const _node;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif