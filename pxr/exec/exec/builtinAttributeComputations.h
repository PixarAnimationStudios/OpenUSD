//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_BUILTIN_ATTRIBUTE_COMPUTATIONS_H
#define PXR_EXEC_EXEC_BUILTIN_ATTRIBUTE_COMPUTATIONS_H

/// \file
///
/// This file defines builtin computations that are provided by attributes.
///

#include "pxr/exec/exec/computationDefinition.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A computation that yields the resolved value of an attribute.
///
class Exec_ComputeResolvedValueComputationDefinition final
    : public Exec_ComputationDefinition
{
public:
    Exec_ComputeResolvedValueComputationDefinition();

    ~Exec_ComputeResolvedValueComputationDefinition() override;

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

private:
    static Exec_InputKeyVectorConstRefPtr _MakeInputKeys();

private:
    const Exec_InputKeyVectorConstRefPtr _inputKeys;
};

/// A computation that yields the value received across a single connection 
/// that is owned by the provider attribute and targets another attribute of 
/// the same type that is valid, i.e. it is active, loaded, defined, and 
/// non-abstract (see EsfAttribute::IsValid()).
///
/// This computation does not currently support providers that own multiple 
/// connections or a single connection that targets any object other than a 
/// valid attribute. 
///
class Exec_ComputeConnectedValueComputationDefinition final
    : public Exec_ComputationDefinition
{
public:
    Exec_ComputeConnectedValueComputationDefinition();

    ~Exec_ComputeConnectedValueComputationDefinition() override;

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
