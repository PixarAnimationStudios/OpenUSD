//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/computationDefinition.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_ComputationDefinition::Exec_ComputationDefinition(
    TfType resultType,
    const TfToken &computationName)
    : _resultType(resultType)
    , _computationName(computationName)
{        
}

Exec_ComputationDefinition::~Exec_ComputationDefinition() = default;

TfType
Exec_ComputationDefinition::GetResultType(
    const EsfObjectInterface &,
    const TfToken &,
    EsfJournal *) const
{
    return _resultType;
}

TfType
Exec_ComputationDefinition::GetExtractionType(
    const EsfObjectInterface &) const
{
    // TODO: Currently, a computation cannot specify an extraction type that
    // differs from its result type.  Extend the definition language to allow
    // authors to specify whether the extracted value should be a scalar or
    // array.
    return _resultType;
}

bool
Exec_ComputationDefinition::IsDispatched() const
{
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
