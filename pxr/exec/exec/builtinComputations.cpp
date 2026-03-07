//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/builtinComputations.h"

#include "pxr/exec/exec/builtinComputationRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TfStaticData<Exec_BuiltinComputationTokens> ExecBuiltinComputations;

Exec_BuiltinComputationTokens::Exec_BuiltinComputationTokens()
    : Exec_BuiltinComputationTokens(
        Exec_BuiltinComputationRegistry::_GetInstanceForRegistration())
{}

Exec_BuiltinComputationTokens::Exec_BuiltinComputationTokens(
    Exec_BuiltinComputationRegistry &registry)
    : computeTime(registry._RegisterBuiltinComputation("computeTime"))
    , computeValue(registry._RegisterBuiltinComputation(
        "computeValue",
        Exec_BuiltinComputationTraits()
            .SetHasDefinition(false)))
    , computeResolvedValue(
        registry._RegisterBuiltinComputation("computeResolvedValue"))
    , computePath(
        registry._RegisterBuiltinComputation("computePath"))

{}

PXR_NAMESPACE_CLOSE_SCOPE
