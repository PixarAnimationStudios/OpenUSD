//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILER_H
#define PXR_EXEC_EXEC_COMPILER_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class EsfStage;
class Exec_CompiledOutputCache;
class Exec_Program;
class Exec_Runtime;
class ExecValueKey;
template <typename> class TfSpan;
class VdfMaskedOutput;

/// This class is responsible for compiling the data flow network for requested
/// value keys.
///
class Exec_Compiler
{
public:
    Exec_Compiler(
        const EsfStage &stage,
        Exec_Program *program,
        Exec_Runtime *runtime);

    /// Returns a vector of leaf masked outputs whose entries correspond to
    /// the value key at the same index in \p valueKeys.
    /// 
    std::vector<VdfMaskedOutput> Compile(TfSpan<const ExecValueKey> valueKeys);

private:
    const EsfStage &_stage;
    Exec_Program *_program;
    Exec_Runtime *_runtime;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
