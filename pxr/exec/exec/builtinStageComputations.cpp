//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/builtinStageComputations.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/inputKey.h"
#include "pxr/exec/exec/program.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/ef/timeInputNode.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// computeTime
//

Exec_TimeComputationDefinition::Exec_TimeComputationDefinition()
    : Exec_ComputationDefinition(
        TfType::Find<EfTime>(),
        ExecBuiltinComputations->computeTime)
{
}

Exec_TimeComputationDefinition::~Exec_TimeComputationDefinition() = default;

Exec_InputKeyVectorConstRefPtr
Exec_TimeComputationDefinition::GetInputKeys(
    const EsfObjectInterface &,
    EsfJournal *) const
{
    return Exec_InputKeyVector::GetEmptyVector();
}

VdfNode *
Exec_TimeComputationDefinition::CompileNode(
    const EsfObjectInterface &,
    EsfJournal *const nodeJournal,
    Exec_Program *const program) const
{
    if (!TF_VERIFY(nodeJournal, "Null nodeJournal")) {
        return nullptr;
    }
    if (!TF_VERIFY(program, "Null program")) {
        return nullptr;
    }

    return &program->GetTimeInputNode();
}

PXR_NAMESPACE_CLOSE_SCOPE
