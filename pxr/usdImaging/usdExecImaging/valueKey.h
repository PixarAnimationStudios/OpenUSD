//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_VALUE_KEY_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_VALUE_KEY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Identifies a value that is computed by exec.
///
/// UsdExecImaging uses its own value key type distinct from ExecUsdValueKey
/// in order to prevent a public dependency on exec. This value key type
/// identifies providers by their SdfPath so the value key can be formed
/// in response to HdSceneIndexBase::GetPrim, which only provides an SdfPath.
///
class UsdExecImagingValueKey
{
public:
    /// Path to the object that provides the named computation.
    ///
    /// The path may refer to a prim or a property.
    ///
    SdfPath providerPath;

    /// The name of the computation.
    ///
    /// This can the name of a plugin-defined computation, or may be one of
    /// the pre-defined ExecBuiltinComputations.
    ///
    TfToken computationName;

    UsdExecImagingValueKey(SdfPath providerPath_, TfToken computationName_)
        : providerPath(std::move(providerPath_))
        , computationName(std::move(computationName_))
    {}

    bool operator==(const UsdExecImagingValueKey &other) const {
        return providerPath == other.providerPath &&
            computationName == other.computationName;
    }
};

template <class HashState>
void TfHashAppend(HashState &h, const UsdExecImagingValueKey &valueKey) {
    h.Append(valueKey.providerPath, valueKey.computationName);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif