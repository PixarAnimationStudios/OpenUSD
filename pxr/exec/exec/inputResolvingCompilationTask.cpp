//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/inputResolvingCompilationTask.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/inputResolver.h"
#include "pxr/exec/exec/outputProvidingCompilationTask.h"
#include "pxr/exec/exec/program.h"

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"
#include "pxr/exec/esf/attribute.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/esf/object.h"
#include "pxr/exec/esf/prim.h"
#include "pxr/exec/esf/stage.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Exec_InputResolvingCompilationTask::_Compile(
    Exec_CompilationState &compilationState,
    TaskPhases &taskPhases)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    taskPhases.Invoke(
    // Generate the output key (or multiple output keys) to compile from the
    // input key, and create new subtasks for any outputs that still need to be
    // compiled.
    [this, &compilationState](TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("compile sources");

        // Generate all the output keys for this input.
        _outputKeys = Exec_ResolveInput(
            compilationState.GetStage(),
            _originObject,
            // We use the schema config key of the dispatching prim for
            // computation lookup if this input requests dispatched
            // computations.
            (_inputKey.fallsBackToDispatched
             ? _dispatchingSchemaKey : EsfSchemaConfigKey()),
            _inputKey,
            _journal);
         _resultOutputs->resize(_outputKeys.size());

        // For every output key, make sure it's either already available or
        // a task has been kicked off to produce it.
        for (size_t i = 0; i < _outputKeys.size(); ++i) {
            const Exec_OutputKey &outputKey = _outputKeys[i];
            VdfMaskedOutput *const resultOutput = &(*_resultOutputs)[i];

            const Exec_OutputKey::Identity outputKeyIdentity =
                outputKey.MakeIdentity();
            const auto &[output, hasOutput] =
                compilationState.GetProgram()->GetCompiledOutput(
                    outputKeyIdentity);
            if (hasOutput) {
                *resultOutput = output;
                continue;
            }

            // Claim the task for producing the missing output.
            const Exec_CompilerTaskSync::ClaimResult claimResult =
                deps.ClaimSubtask(outputKeyIdentity);
            if (claimResult == Exec_CompilerTaskSync::ClaimResult::Claimed) {
                // Run the task that produces the output.
                deps.NewSubtask<Exec_OutputProvidingCompilationTask>(
                    compilationState,
                    outputKey,
                    resultOutput);
            }
        }
    },

    // Compiled outputs are now all available and can be retrieved from the
    // compiled outputs cache.
    [this, &compilationState](TaskDependencies &deps) {
        TRACE_FUNCTION_SCOPE("populate result");

        // For every output key, check if we still don't have a result and if
        // so retrieve it from the compiled output. All the task dependencies
        // should have fulfilled at this point.
        for (size_t i = 0; i < _outputKeys.size(); ++i) {
            const Exec_OutputKey &outputKey = _outputKeys[i];
            VdfMaskedOutput *const resultOutput = &(*_resultOutputs)[i];

            if (*resultOutput) {
                continue;
            }

            const auto &[output, hasOutput] =
                compilationState.GetProgram()->GetCompiledOutput(
                    outputKey.MakeIdentity());
            if (!output) {
                TF_VERIFY(_inputKey.optional);
                continue;
            }

            *resultOutput = output;
        }
    }
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
