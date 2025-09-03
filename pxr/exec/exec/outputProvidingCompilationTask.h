//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_OUTPUT_PROVIDING_COMPILATION_TASK_H
#define PXR_EXEC_EXEC_OUTPUT_PROVIDING_COMPILATION_TASK_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/exec/compilationTask.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/outputKey.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/vdf/maskedOutput.h"

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompilationState;

/// Compilation task that returns a VdfMaskedOutput for the provided
/// output key.
/// 
/// This task finds the compiled VdfMaskedOutput in the output cache and returns
/// it to the successor task. If there is no compiled output for the given
/// output key, this task will create a new node with the relevant
/// VdfMaskedOutput, and connect all its inputs by kicking off
/// Exec_InputResolvingCompilationTasks for each input.
/// 
class Exec_OutputProvidingCompilationTask : public Exec_CompilationTask
{
public:
    Exec_OutputProvidingCompilationTask(
        Exec_CompilationState &compilationState,
        const Exec_OutputKey &outputKey,
        VdfMaskedOutput *resultOutput)
        : Exec_CompilationTask(compilationState)
        , _outputKey(outputKey)
        , _resultOutput(resultOutput)
    {}

private:
    void _Compile(
        Exec_CompilationState &compilationState,
        TaskPhases &taskStages) override;

    // The output key indicating which output and node must be compiled.
    const Exec_OutputKey _outputKey;

    // The input keys, which are recorded during the first stage of this task,
    // and queried during the second stage.
    Exec_InputKeyVectorConstRefPtr _inputKeys;

    // Some nodes only have one input, and many inputs only source from one
    // output, hence the choice of TfSmallVector.
    using _SourceOutputs = TfSmallVector<VdfMaskedOutput, 1>;
    using _InputSources = TfSmallVector<_SourceOutputs, 1>;

    // If we end up compiling a new node, these are the source outputs populated
    // from the input resolving tasks. Each entry in this array corresponds
    // to one input on the new node, and each input can source data from any
    // number of source outputs, each resulting in one input connection.
    _InputSources _inputSources;
    
    // Input resolving tasks created by this task record their resolution
    // traversals into these journals. One journal is created for each input.
    TfSmallVector<EsfJournal, 1> _inputJournals;

    // This journal records the changes that should cause uncompilation of the
    // node.
    EsfJournal _nodeJournal;

    // Pointer to the resulting masked output to be populated by this task.
    VdfMaskedOutput *const _resultOutput;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
