//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/privateBuiltinComputations.h"

#include "pxr/exec/exec/builtinComputationRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TfStaticData<Exec_PrivateBuiltinComputationTokens>
Exec_PrivateBuiltinComputations;

Exec_PrivateBuiltinComputationTokens::Exec_PrivateBuiltinComputationTokens()
    : Exec_PrivateBuiltinComputationTokens(
        Exec_BuiltinComputationRegistry::_GetInstanceForRegistration())
{}

Exec_PrivateBuiltinComputationTokens::Exec_PrivateBuiltinComputationTokens(
    Exec_BuiltinComputationRegistry &registry)
    : computeConstant(registry._RegisterBuiltinComputation("computeConstant"))
    , computeMetadata(registry._RegisterBuiltinComputation("computeMetadata"))
    , computeExpression(registry._RegisterBuiltinComputation(
        "computeExpression",
        Exec_BuiltinComputationTraits()
            .SetHasDefinition(false)
            .SetIsUserDefinable(true)
            .SetIsInputConsumable(false)))
{}

PXR_NAMESPACE_CLOSE_SCOPE
