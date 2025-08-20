//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_VISIT_VALUE_KEY_H
#define PXR_EXEC_EXEC_USD_VISIT_VALUE_KEY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/api.h"
#include "pxr/exec/execUsd/valueKey.h"

#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

/// Apply a visitor to the held value key variant.
template <typename Visitor>
auto
ExecUsd_VisitValueKey(Visitor &&visitor, const ExecUsdValueKey &uvk)
{
    return std::visit(std::forward<Visitor>(visitor), uvk._key);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
