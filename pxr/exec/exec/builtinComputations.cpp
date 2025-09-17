//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/builtinComputations.h"

PXR_NAMESPACE_OPEN_SCOPE

TfStaticData<Exec_BuiltinComputations> ExecBuiltinComputations;

// A vector of all registered builtin computation tokens.
static TfStaticData<std::vector<TfToken>> _builtinComputationNames;

static TfToken
_RegisterBuiltin(const std::string &name)
{
    const TfToken computationNameToken(
        Exec_BuiltinComputations::builtinComputationNamePrefix + name);
    _builtinComputationNames->push_back(computationNameToken);
    return computationNameToken;
}

Exec_BuiltinComputations::Exec_BuiltinComputations()
    : computeTime(_RegisterBuiltin("computeTime"))
    , computeValue(_RegisterBuiltin("computeValue"))
{
}

const std::vector<TfToken> &
Exec_BuiltinComputations::GetComputationTokens()
{
    return *_builtinComputationNames;
}

PXR_NAMESPACE_CLOSE_SCOPE
