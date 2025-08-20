//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_VALUE_KEY_H
#define PXR_EXEC_EXEC_USD_VALUE_KEY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/api.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"

#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

class ExecValueKey;

/// Represents expired value keys.
///
/// Records the path and computation for debugging purposes.
///
struct ExecUsd_ExpiredValueKey
{
    SdfPath path;
    TfToken computation;
};

/// Represents attribute value keys.
struct ExecUsd_AttributeValueKey
{
    UsdAttribute provider;
    TfToken computation;
};

/// Represents prim computation value keys.
struct ExecUsd_PrimComputationValueKey
{
    UsdPrim provider;
    TfToken computation;
};

/// Specifies a computed value.
///
/// Clients identify computations to evaluate using a UsdObject that provides
/// computations and the name of the computation.
/// 
class ExecUsdValueKey
{
public:
    /// Constructs a value key representing an attribute value.
    EXECUSD_API
    explicit ExecUsdValueKey(const UsdAttribute &provider);

    /// Constructs a value key representing a prim computation.
    EXECUSD_API
    ExecUsdValueKey(const UsdPrim &provider, const TfToken &computation);

    EXECUSD_API
    ~ExecUsdValueKey();

private:
    template <typename Visitor>
    friend auto ExecUsd_VisitValueKey(Visitor &&, const ExecUsdValueKey &);

    // Expires a value key by replacing it with an ExecUsd_ExpiredValueKey.
    friend void
    ExecUsd_ExpireValueKey(ExecUsdValueKey *);

    std::variant<
        ExecUsd_ExpiredValueKey,
        ExecUsd_AttributeValueKey,
        ExecUsd_PrimComputationValueKey
    > _key;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
