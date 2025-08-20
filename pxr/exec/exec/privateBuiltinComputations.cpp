//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/privateBuiltinComputations.h"

PXR_NAMESPACE_OPEN_SCOPE

TfStaticData<Exec_PrivateBuiltinComputationsStruct>
Exec_PrivateBuiltinComputations;

// A vector of all registered builtin computation tokens.
static TfStaticData<std::vector<TfToken>> _builtinComputationNames;

static TfToken
_RegisterBuiltin(const std::string &name)
{
    const TfToken computationNameToken(
        Exec_PrivateBuiltinComputationsStruct::builtinComputationNamePrefix +
        name);
    _builtinComputationNames->push_back(computationNameToken);
    return computationNameToken;
}

Exec_PrivateBuiltinComputationsStruct::Exec_PrivateBuiltinComputationsStruct()
    : computeMetadata(_RegisterBuiltin("computeMetadata"))
{
}

const std::vector<TfToken> &
Exec_PrivateBuiltinComputationsStruct::GetComputationTokens()
{
    return *_builtinComputationNames;
}

PXR_NAMESPACE_CLOSE_SCOPE
