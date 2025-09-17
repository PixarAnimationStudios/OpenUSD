//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_PRIVATE_BUILTIN_COMPUTATIONS_H
#define PXR_EXEC_EXEC_PRIVATE_BUILTIN_COMPUTATIONS_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"

#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Tokens representing built-in computations that are used internally by the
/// execution system.
///
struct Exec_PrivateBuiltinComputationsStruct
{
    EXEC_API
    Exec_PrivateBuiltinComputationsStruct();

    // Computes a constant value.
    //
    // The computation provider must be the stage.
    //
    const TfToken computeConstant;

    // Computes a given metadatum on the computation provider.
    //
    // NOTE: The computation provider can be any type of scene object *except*
    // the pseudo-root (which represents the stage).
    // 
    const TfToken computeMetadata;

    /// Returns all builtin computation tokens.
    const std::vector<TfToken> &GetComputationTokens();

    /// The prefix that begins all builtin computation names.
    static constexpr char builtinComputationNamePrefix[] = "__";
};

// Used to publicly access builtin computation tokens.
EXEC_API
extern TfStaticData<Exec_PrivateBuiltinComputationsStruct>
Exec_PrivateBuiltinComputations;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
