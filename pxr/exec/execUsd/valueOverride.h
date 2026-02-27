//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_VALUE_OVERRIDE_H
#define PXR_EXEC_EXEC_USD_VALUE_OVERRIDE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/vt/value.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Specifies a computed value that should be temporarily overridden with a
/// different value.
///
/// \see ExecUsdSystem::ComputeWithOverrides
///
struct ExecUsdValueOverride
{
    /// Which computed value should be overridden.
    ExecUsdValueKey valueKey;

    /// When exec computes the above value key, it should produce this value.
    VtValue overrideValue;
};

using ExecUsdValueOverrideVector = std::vector<ExecUsdValueOverride>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif