//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SHADE_TYPES_H
#define PXR_USD_USD_SHADE_TYPES_H

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/base/tf/smallVector.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \enum UsdShadeAttributeType
///
/// Specifies the type of a shading attribute.
///
enum class UsdShadeAttributeType {
    Invalid,
    Input,
    Output,
};

/// \enum UsdShadeConnectionModification
///
/// Choice when creating a single connection with the \p ConnectToSource method
/// for a shading attribute. The new connection can replace any existing
/// connections or be added to the list of existing connections. In which case
/// there is a choice between prepending and appending to said list, which will
/// be represented by Usd's list editing operations.
///
enum class UsdShadeConnectionModification {
    Replace,
    Prepend,
    Append
};

/// \typedef UsdShadeAttributeVector
///
/// For performance reasons we want to be extra careful when reporting
/// attributes. It is possible to have multiple connections for a shading
/// attribute, but by far the more common cases are one or no connection. So we
/// use a small vector that can be stack allocated that holds space for a single
/// attributes, but that can "spill" to the heap in the case of multiple
/// upstream attributes.
using UsdShadeAttributeVector = TfSmallVector<UsdAttribute, 1>;

/// \typedef UsdShadeSourceInfoVector
///
/// For performance reasons we want to be extra careful when reporting
/// connections. It is possible to have multiple connections for a shading
/// attribute, but by far the more common cases are one or no connection.
/// So we use a small vector that can be stack allocated that holds space
/// for a single source, but that can "spill" to the heap in the case
/// of a multi-connection.
///
/// /sa UsdShadeConnectionSourceInfo in connectableAPI.h
struct UsdShadeConnectionSourceInfo;
using UsdShadeSourceInfoVector = TfSmallVector<UsdShadeConnectionSourceInfo, 1>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
