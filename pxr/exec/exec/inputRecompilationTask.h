//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_INPUT_RECOMPILATION_TASK_H
#define PXR_EXEC_EXEC_INPUT_RECOMPILATION_TASK_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/compilationTask.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/vdf/maskedOutput.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfInput;

/// Task that begins compilation from a VdfInput that was disconnected by
/// uncompilation.
///
/// The task re-resolves the input, compiles its source outputs, then reconnects
/// those outputs to the input. The input may be for a leaf node, or any other
/// intermediate node in the network.
///
class Exec_InputRecompilationTask : public Exec_CompilationTask
{
public:
    Exec_InputRecompilationTask(
        Exec_CompilationState &compilationState,
        VdfInput *const input)
        : Exec_CompilationTask(compilationState)
        , _input(input)
    {}

private:
    void _Compile(
        Exec_CompilationState &compilationState,
        TaskPhases &taskPhases) override;

    void _Interrupt(Exec_CompilationState &compilationState) override;

private:
    // The input to be recompiled.
    VdfInput *const _input;

    // The task uses these journals to resolve the input, one for each input
    // key.
    TfSmallVector<EsfJournal, 1> _journalPerInputKey;

    // The new source outputs for the input, one set for each input key.
    TfSmallVector<
        TfSmallVector<VdfMaskedOutput, 1>, 1> _resultOutputsPerInputKey;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif