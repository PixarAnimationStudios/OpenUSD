//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execUsd/valueKey.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/exec/exec/builtinComputations.h"

#include <typeinfo>
#include <utility>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

ExecUsdValueKey::ExecUsdValueKey(const UsdAttribute &provider)
    : _key(ExecUsd_AttributeValueKey{
            provider, ExecBuiltinComputations->computeValue})
{}

ExecUsdValueKey::ExecUsdValueKey(
    const UsdPrim &provider, const TfToken &computation)
    : _key(ExecUsd_PrimComputationValueKey{provider, computation})
{}

ExecUsdValueKey::~ExecUsdValueKey() = default;

PXR_NAMESPACE_CLOSE_SCOPE
