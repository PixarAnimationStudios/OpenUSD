//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_LEAF_COMPILATION_TASK_H
#define PXR_EXEC_EXEC_LEAF_COMPILATION_TASK_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/exec/exec/compilationTask.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/valueKey.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/exec/esf/journal.h"
#include "pxr/exec/vdf/maskedOutput.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class Exec_CompilationState;

/// Leaf compilation task for compiling requested outputs.
///
/// This is the main entry point into the compilation task graph for outputs
/// that have been requested via an ExecRequest and therefore need leaf nodes
/// compiled and connected to them.
/// 
class Exec_LeafCompilationTask : public Exec_CompilationTask
{
public:
    Exec_LeafCompilationTask(
        Exec_CompilationState &compilationState,
        const ExecValueKey &valueKey,
        VdfMaskedOutput *leafOutput)
        : Exec_CompilationTask(compilationState)
        , _valueKey(valueKey)
        , _leafOutput(leafOutput)
    {}

private:
    void _Compile(
        Exec_CompilationState &compilationState,
        TaskPhases &taskPhases) override;

    // The value key for the requested output.
    const ExecValueKey _valueKey;

    // The origin object on which input resolution is performed. EsfObjects are
    // not default-constructible, but construction must be deferred until
    // _Compile. Therefore, the EsfObject is held by a std::optional.
    std::optional<EsfObject> _originObject;

    // The input keys that resolve to the leaf outputs. This only contains a
    // single input key, but Exec_Program::SetRecompilationInfo requires input
    // keys be specified by an Exec_InputKeyVectorConstRefPtr.
    Exec_InputKeyVectorConstRefPtr _inputKeys;

    // The array of outputs populated by the input resolving task.
    TfSmallVector<VdfMaskedOutput, 1> _resultOutputs;

    // The journal used while resolving the input to the leaf node.
    EsfJournal _journal;

    // Pointer to the leaf output to be populated by this task.
    VdfMaskedOutput *const _leafOutput;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif