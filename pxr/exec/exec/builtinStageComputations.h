//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_BUILTIN_STAGE_COMPUTATIONS_H
#define PXR_EXEC_EXEC_BUILTIN_STAGE_COMPUTATIONS_H

/// \file
///
/// This file defines builtin computations that are provided by the stage, i.e.,
/// by the pseudo-root prim.
///

#include "pxr/exec/exec/computationDefinition.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A computation that yields a constant value.
class Exec_ComputeConstantComputationDefinition final
    : public Exec_ComputationDefinition
{
public:
    Exec_ComputeConstantComputationDefinition();

    ~Exec_ComputeConstantComputationDefinition() override;

    TfType GetResultType(
        const EsfObjectInterface &providerObject,
        const TfToken &disambiguatingId,
        EsfJournal *journal) const override;

    TfType GetExtractionType(
        const EsfObjectInterface &providerObject) const override;

    Exec_InputKeyVectorConstRefPtr GetInputKeys(
        const EsfObjectInterface &providerObject,
        EsfJournal *journal) const override;

    VdfNode *CompileNode(
        const EsfObjectInterface &providerObject,
        const TfToken &disambiguatingId,
        EsfJournal *nodeJournal,
        Exec_Program *program) const override;
};

/// A computation that yields the current evaluation time.
class Exec_TimeComputationDefinition final
    : public Exec_ComputationDefinition
{
public:
    Exec_TimeComputationDefinition();
    
    ~Exec_TimeComputationDefinition() override;

    Exec_InputKeyVectorConstRefPtr GetInputKeys(
        const EsfObjectInterface &providerObject,
        EsfJournal *journal) const override;

    VdfNode *CompileNode(
        const EsfObjectInterface &providerObject,
        const TfToken &disambiguatingId,
        EsfJournal *nodeJournal,
        Exec_Program *program) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
