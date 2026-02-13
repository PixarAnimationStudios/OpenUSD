//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/outputProvidingCompilationTask.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/computationDefinition.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/inputResolvingCompilationTask.h"
#include "pxr/exec/exec/program.h"

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/esf/editReason.h"
#include "pxr/exec/esf/journal.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_OutputProvidingCompilationTask::_Compile(
    Exec_CompilationState &compilationState,
    TaskPhases &taskPhases)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    const Exec_ComputationDefinition *const computationDefinition =
        _outputKey.GetComputationDefinition();

    taskPhases.Invoke(
    // Make sure input dependencies are fulfilled
    [this, &compilationState, computationDefinition](TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("input tasks");

        // TODO: The node to be compiled by this task should be uncompiled when
        // the provider object is resynced. Ideally, this dependency would be
        // automatically added by looking up the computation definition, but
        // that already happened in the input resolving task. Therefore, we need
        // to explicitly add the resync entry to the node's journal.
        _nodeJournal.Add(
            _outputKey.GetProviderObject()->GetPath(nullptr),
            EsfEditReason::ResyncedObject);

        _inputKeys =
            computationDefinition->GetInputKeys(
                *_outputKey.GetProviderObject(),
                &_nodeJournal);

        const size_t numInputKeys = _inputKeys->Get().size();
        _inputSources.resize(numInputKeys);
        _inputJournals.resize(numInputKeys);
        for (size_t i = 0; i < numInputKeys; ++i) {
            deps.NewSubtask<Exec_InputResolvingCompilationTask>(
                compilationState,
                _inputKeys->Get()[i],
                _outputKey.GetProviderObject(),
                _outputKey.GetDispatchingSchemaKey(),
                &_inputSources[i],
                &_inputJournals[i]);
        }
    },

    // Compile and connect the node
    [this, &compilationState, computationDefinition](
        TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("node creation");

        VdfNode *const node = computationDefinition->CompileNode(
            *_outputKey.GetProviderObject(),
            _outputKey.GetDisambiguatingId(),
            &_nodeJournal,
            compilationState.GetProgram());

        if (!TF_VERIFY(node)) {
            return;
        }

        const Exec_OutputKey::Identity keyIdentity = _outputKey.MakeIdentity();
        node->SetDebugNameCallback([keyIdentity]{
            return keyIdentity.GetDebugName();
        });

        compilationState.GetProgram()->SetNodeRecompilationInfo(
            node,
            _outputKey.GetProviderObject(),
            _outputKey.GetDispatchingSchemaKey(),
            Exec_InputKeyVectorConstRefPtr(_inputKeys));

        for (size_t i = 0; i < _inputSources.size(); ++i) {
            compilationState.GetProgram()->Connect(
                _inputJournals[i],
                _inputSources[i],
                node,
                _inputKeys->Get()[i].inputName);
        }

        // Return the compiled output to the calling task.
        VdfMaskedOutput compiledOutput(node->GetOutput(), VdfMask::AllOnes(1));
        *_resultOutput = compiledOutput;

        // Then publish it to the compiled outputs cache.
        TF_VERIFY(compilationState.GetProgram()->SetCompiledOutput(
            _outputKey.MakeIdentity(), compiledOutput));

        // Then indicate that the task identified by _outputKey is done. This
        // notifies all other tasks with a dependency on this _outputKey.
        deps.MarkDoneOutputProvidingTask(_outputKey.MakeIdentity());
    }
    );
}

void
Exec_OutputProvidingCompilationTask::_Interrupt(
    Exec_CompilationState &compilationState)
{
    // The upstream nodes that would have connected to this node are now
    // potentially isolated.
    for (const auto &sourceOutputs : _inputSources) {
        for (const auto &maskedOutput : sourceOutputs) {
            if (VdfOutput *const output = maskedOutput.GetOutput()) {
                compilationState.GetInterruptState()
                    .AddPotentiallyIsolatedNode(&output->GetNode());
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
