//
// Unlicensed 2022 benmalartre
//

#ifndef PXR_USD_USD_EXEC_TYPES_H
#define PXR_USD_USD_EXEC_TYPES_H

#include "pxr/pxr.h"
#include "pxr/usd/usdExec/api.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/base/tf/smallVector.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \enum UsdExecAttributeType
///
/// Specifies the type of a attribute.
///
enum class UsdExecAttributeType {
    Invalid,
    Input,
    Output,
};

/// \enum UsdExecConnectionModification
///
/// Choice when creating a single connection with the \p ConnectToSource method
/// for an attribute. The new connection can replace any existing
/// connections or be added to the list of existing connections. In which case
/// there is a choice between prepending and appending to said list, which will
/// be represented by Usd's list editing operations.
///
enum class UsdExecConnectionModification {
    Replace,
    Prepend,
    Append
};

/// \typedef UsdExecAttributeVector
///
/// For performance reasons we want to be extra careful when reporting
/// attributes. It is possible to have multiple connections for a shading
/// attribute, but by far the more common cases are one or no connection. So we
/// use a small vector that can be stack allocated that holds space for a single
/// attributes, but that can "spill" to the heap in the case of multiple
/// upstream attributes.
using UsdExecAttributeVector = TfSmallVector<UsdAttribute, 2>;

/// \typedef UsdExecSourceInfoVector
///
/// For performance reasons we want to be extra careful when reporting
/// connections. It is possible to have multiple connections for a shading
/// attribute, but by far the more common cases are one or no connection.
/// So we use a small vector that can be stack allocated that holds space
/// for a single source, but that can "spill" to the heap in the case
/// of a multi-connection.
///
/// /sa UsdExecConnectionSourceInfo in connectableAPI.h
struct UsdExecConnectionSourceInfo;
using UsdExecSourceInfoVector = TfSmallVector<UsdExecConnectionSourceInfo, 2>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_EXEC_TYPES_H
