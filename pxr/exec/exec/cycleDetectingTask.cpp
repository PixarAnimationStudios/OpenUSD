//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/cycleDetectingTask.h"

#include "pxr/exec/exec/program.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/node.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_CycleDetectingTask::_Compile(
    Exec_CompilationState &compilationState,
    TaskPhases &taskPhases)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    taskPhases.Invoke(
    [this, &compilationState](TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("search for potential cycles");
        
        // If this node is a VdfSpeculationNode, traversal should not proceed
        // upwards from here, because VdfSpeculationNodes are specifically
        // created to break topological cycles, and there is no need to
        // re-detect those cycles.
        if (_node->IsSpeculationNode()) {
            return;
        }

        const std::unordered_set<VdfInput *> &inputsRequiringRecompilation =
            compilationState.GetProgram()->GetInputsRequiringRecompilation();

        for (const auto &entry : _node->GetInputsIterator()) {
            VdfInput *const input = entry.second;
            if (inputsRequiringRecompilation.find(input) !=
                inputsRequiringRecompilation.end()) {
                // This input is being recompiled. This task cannot complete
                // until the input has finished recompilation. If recompilation
                // of the input depends on the current Exec_CycleDetectingTask,
                // then the dependency added here introduces a task cycle, which
                // will be caught by the Exec_TaskCycleDetector.
                deps.WaitOnInputRecompilationTask(input);
                continue;
            }

            // The input is not being recompiled. Recursively spawn more cycle
            // detecting tasks for all nodes upstream of this input.
            for (const VdfConnection *const connection :
                input->GetConnections()) {
                const VdfNode *const sourceNode = &connection->GetSourceNode();
                if (deps.ClaimCycleDetectingTask(sourceNode)) {
                    deps.NewSubtask<Exec_CycleDetectingTask>(
                        compilationState,
                        sourceNode);
                }
            }
        }
    },
    [this](TaskDependencies &deps) {
        // Any upstream inputs undergoing recompilation have completed.
        deps.MarkDoneCycleDetectingTask(_node);
    }
    );
}

void
Exec_CycleDetectingTask::_Interrupt(Exec_CompilationState &)
{
    // Cycle detecting tasks have nothing to contribute to the interrupt state.
    // An empty implementation must be provided because _Interrupt is a
    // pure-virtual method.
}

PXR_NAMESPACE_CLOSE_SCOPE
