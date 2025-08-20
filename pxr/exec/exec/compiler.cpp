//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compiler.h"

#include "pxr/exec/exec/compilationState.h"
#include "pxr/exec/exec/inputRecompilationTask.h"
#include "pxr/exec/exec/leafCompilationTask.h"
#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/runtime.h"
#include "pxr/exec/exec/valueKey.h"

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/exec/vdf/isolatedSubnetwork.h"
#include "pxr/exec/vdf/maskedOutput.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

Exec_Compiler::Exec_Compiler(
    const EsfStage &stage,
    Exec_Program *program,
    Exec_Runtime *runtime)
    : _stage(stage)
    , _program(program)
    , _runtime(runtime)
{}

std::vector<VdfMaskedOutput>
Exec_Compiler::Compile(TfSpan<const ExecValueKey> valueKeys)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    // Note that the returned vector should always have the same size as
    // valueKeys.  Any key that failed to compile should yield a null masked
    // output at the corresponding index in the result.
    std::vector<VdfMaskedOutput> leafOutputs(valueKeys.size());

    // Process requested value keys in parallel and spawn compilation tasks.
    WorkWithScopedDispatcher(
        [valueKeys, &leafOutputs, &stage = _stage, &program = _program]
        (WorkDispatcher &dispatcher) {

        // Compiler state shared between all compilation tasks.
        Exec_CompilationState state(dispatcher, stage, program);

        WorkParallelForN(valueKeys.size(),
            [&state, valueKeys, &leafOutputs](size_t b, size_t e) {
            for (size_t i = b; i != e; ++i) {
                Exec_CompilationState::NewTask<Exec_LeafCompilationTask>(
                    state, valueKeys[i], &leafOutputs[i]);
            }
        });

        // These VdfInputs have been disconnected by previous rounds of
        // uncompilation and need to be recompiled.
        const std::unordered_set<VdfInput *> &inputsRequiringRecompilation =
            program->GetInputsRequiringRecompilation();
            
        WorkParallelForN(inputsRequiringRecompilation.bucket_count(),
            [&state, &inputsRequiringRecompilation](size_t b, size_t e) {
                for (size_t bucketIdx = b; bucketIdx != e; ++bucketIdx) {
                    auto bucketIter =
                        inputsRequiringRecompilation.cbegin(bucketIdx);
                    const auto bucketEnd =
                        inputsRequiringRecompilation.cend(bucketIdx);
                    for (; bucketIter != bucketEnd; ++bucketIter) {
                        Exec_CompilationState::NewTask<
                            Exec_InputRecompilationTask>(state, *bucketIter);
                    }
                }
        });

        {
            TRACE_FUNCTION_SCOPE("waiting for tasks");
            dispatcher.Wait();
        }
    });

    // All inputs requiring recompilation have been recompiled.
    _program->ClearInputsRequiringRecompilation();

    {
        TRACE_FUNCTION_SCOPE("uncompiling isolated subnetwork");

        // We hold onto the isolated subnetwork object until we are done
        // clearing node output data, because the subnetwork object's destructor
        // deletes the isolated nodes.
        const std::unique_ptr<VdfIsolatedSubnetwork> subnetwork =
            _program->CreateIsolatedSubnetwork();

        WorkWithScopedDispatcher(
            [&subnetwork, runtime = _runtime]
            (WorkDispatcher &dispatcher) {

                dispatcher.Run([&subnetwork]{
                    TRACE_FUNCTION_SCOPE("removing isolated objects");
                    subnetwork->RemoveIsolatedObjectsFromNetwork();
                });

                {
                    TRACE_FUNCTION_SCOPE("clearing data");
                    for (VdfNode *const node : subnetwork->GetIsolatedNodes()) {
                        runtime->DeleteData(*node);
                    }
                }
            });
    }

    return leafOutputs;
}

PXR_NAMESPACE_CLOSE_SCOPE
