//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/builtinStageComputations.h"

#include "pxr/exec/exec/builtinComputations.h"
#include "pxr/exec/exec/constantValueNode.h"
#include "pxr/exec/exec/definitionRegistry.h"
#include "pxr/exec/exec/program.h"
#include "pxr/exec/exec/privateBuiltinComputations.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/ef/timeInputNode.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// computeConstant
//

Exec_ComputeConstantComputationDefinition::
Exec_ComputeConstantComputationDefinition()
    : Exec_ComputationDefinition(
        TfType::GetUnknownType(),
        Exec_PrivateBuiltinComputations->computeConstant)
{
}

Exec_ComputeConstantComputationDefinition::
~Exec_ComputeConstantComputationDefinition() = default;

TfType
Exec_ComputeConstantComputationDefinition::GetResultType(
    const EsfObjectInterface &providerObject,
    const TfToken &disambiguatingId,
    EsfJournal *const journal) const
{
    return Exec_DefinitionRegistry::GetInstance().GetConstantValue(
        disambiguatingId).GetType();
}

TfType
Exec_ComputeConstantComputationDefinition::GetExtractionType(
    const EsfObjectInterface &providerObject) const
{
    TF_VERIFY(false, "Extracting constant values directly is not supported.");
    return TfType();
}

Exec_InputKeyVectorConstRefPtr
Exec_ComputeConstantComputationDefinition::GetInputKeys(
    const EsfObjectInterface &providerObject,
    EsfJournal *) const
{
    return Exec_InputKeyVector::GetEmptyVector();
}

VdfNode *
Exec_ComputeConstantComputationDefinition::CompileNode(
    const EsfObjectInterface &providerObject,
    const TfToken &disambiguatingId,
    EsfJournal *const nodeJournal,
    Exec_Program *const program) const
{
    if (!TF_VERIFY(nodeJournal, "Null nodeJournal")) {
        return nullptr;
    }
    if (!TF_VERIFY(program, "Null program")) {
        return nullptr;
    }

    const VtValue value =
        Exec_DefinitionRegistry::GetInstance().GetConstantValue(
            disambiguatingId);

    return program->CreateNode<Exec_ConstantValueNode>(*nodeJournal, value);
}

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
    const TfToken &,
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
