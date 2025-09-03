//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/inputRecompilationTask.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/inputResolvingCompilationTask.h"
#include "pxr/exec/exec/nodeRecompilationInfo.h"
#include "pxr/exec/exec/program.h"

#include "pxr/base/trace/trace.h"
#include "pxr/exec/ef/leafNode.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_InputRecompilationTask::_Compile(
    Exec_CompilationState &compilationState,
    TaskPhases &taskPhases)
{
    taskPhases.Invoke(
    [this, &compilationState](TaskDependencies &taskDeps) {
        TRACE_FUNCTION_SCOPE("recompile input");

        // Fetch recompilation info for the input's node.
        const Exec_NodeRecompilationInfo *const nodeRecompilationInfo =
            compilationState.GetProgram()->GetNodeRecompilationInfo(
                &_input->GetNode());
        if (!TF_VERIFY(
            nodeRecompilationInfo,
            "Unable to recompile input '%s' because no recompilation info was "
            "found for the node.",
            _input->GetDebugName().c_str())) {
            return;
        }

        // Fetch recompilation info specific to this input.
        const EsfObject &originObject = nodeRecompilationInfo->GetProvider();
        const TfSmallVector<const Exec_InputKey *, 1> inputKeys =
            nodeRecompilationInfo->GetInputKeys(*_input);
        if (!TF_VERIFY(
            !inputKeys.empty(),
            "Unable to recompile input '%s' because no input keys were found.",
            _input->GetDebugName().c_str())) {
            return;
        }

        // Each input key needs its own output vector and journal for
        // input resolution.
        const size_t numInputKeys = inputKeys.size();
        _resultOutputsPerInputKey.resize(numInputKeys);
        _journalPerInputKey.resize(numInputKeys);

        // Re-resolve and recompile the input's dependencies.
        for (size_t i = 0; i < numInputKeys; ++i) {
            taskDeps.NewSubtask<Exec_InputResolvingCompilationTask>(
                compilationState,
                *inputKeys[i],
                originObject,
                nodeRecompilationInfo->GetDispatchingSchemaKey(),
                &_resultOutputsPerInputKey[i],
                &_journalPerInputKey[i]);
        }
    },

    [this, &compilationState](TaskDependencies &taskDeps) {
        TRACE_FUNCTION_SCOPE("reconnect input");

        size_t totalOutputs = 0;
        for (const auto &sourceOutputs : _resultOutputsPerInputKey) {
            totalOutputs += sourceOutputs.size();
        }

        // If the input belonged to a leaf node, then we require exactly one
        // source output.
        if (!TF_VERIFY(
            (_resultOutputsPerInputKey.size() == 1 &&
             _resultOutputsPerInputKey[0].size() == 1) ||
            !EfLeafNode::IsALeafNode(_input->GetNode()),
            "Recompilation of leaf node input '%s' expected exactly 1 output "
            "from 1 input key; got %zu outputs from %zu input keys.",
            _input->GetDebugName().c_str(),
            totalOutputs,
            _resultOutputsPerInputKey.size())) {
            return;
        }

        // Connect the recompiled outputs to this input.
        const size_t numInputKeys = _resultOutputsPerInputKey.size();
        for (size_t i = 0; i < numInputKeys; ++i) {
            compilationState.GetProgram()->Connect(
                _journalPerInputKey[i], 
                _resultOutputsPerInputKey[i], 
                &_input->GetNode(),
                _input->GetName());
        }
    }
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
