//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/builtinComputationRegistry.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

static const char *const _builtinComputationPrefix = "__";

TF_INSTANTIATE_SINGLETON(Exec_BuiltinComputationRegistry);

Exec_BuiltinComputationRegistry::Exec_BuiltinComputationRegistry() = default;

const Exec_BuiltinComputationRegistry &
Exec_BuiltinComputationRegistry::GetInstance()
{
    return TfSingleton<Exec_BuiltinComputationRegistry>::GetInstance();
}

Exec_BuiltinComputationRegistry &
Exec_BuiltinComputationRegistry::_GetInstanceForRegistration()
{
    return TfSingleton<Exec_BuiltinComputationRegistry>::GetInstance();
}

bool
Exec_BuiltinComputationRegistry::IsReservedName(const TfToken &computationName)
{
    return TfStringStartsWith(
        computationName.GetString(),
        _builtinComputationPrefix);
}

const char *
Exec_BuiltinComputationRegistry::GetReservedNamePrefix()
{
    return _builtinComputationPrefix;
}

const Exec_BuiltinComputationTraits *
Exec_BuiltinComputationRegistry::GetTraits(
    const TfToken &computationName) const
{
    const auto it = _traits.find(computationName);
    return it == _traits.end() ? nullptr : &it->second;
}

TfToken
Exec_BuiltinComputationRegistry::_RegisterBuiltinComputation(
    const std::string &computationName,
    Exec_BuiltinComputationTraits computationTraits)
{
    // This cannot be const because it would prevent moving out of the function
    // as a return value.
    TfToken computationNameToken(
        _builtinComputationPrefix + computationName);

    const bool emplaced =
        _traits.emplace(computationNameToken, computationTraits).second;

    if (!TF_VERIFY(emplaced)) {
        return TfToken();
    }

    if (computationTraits.HasDefinition()) {
        _numComputationsWithDefinitions++;
    }

    return computationNameToken;
}

PXR_NAMESPACE_CLOSE_SCOPE
