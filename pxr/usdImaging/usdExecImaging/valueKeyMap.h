//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_VALUE_KEY_MAP_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_VALUE_KEY_MAP_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/valueKey.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/usd/sdf/path.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdExecImagingPrimAdapterInterface;

/// UsdExecImaging_ValueKeyMap stores additional information about the value
/// keys in a UsdExecImaging_Request.
///
/// This information is used to extract values from the request, to notify prim
/// adapters of invalidation, and to discover prims whose adapters have changed.
///
struct UsdExecImaging_ValueKeyMap
{
    /// Information stored for each value key.
    struct ValueKeyInfo
    {
        /// The original value key.
        UsdExecImagingValueKey valueKey;

        /// Path to the prim whose adapter added this value key.
        SdfPath adaptedPrimPath;

        /// The adapter that added the value key.
        const UsdExecImagingPrimAdapterInterface *primAdapter;
    };

    /// The ValueKeyInfo at index i provides information about the value key
    /// at request index i.
    ///
    std::vector<ValueKeyInfo> indexToValueKeyInfo;

    /// Maps value keys to their corresponding positions in the request.
    using ValueKeyToIndexMap =
        pxr_tsl::robin_map<UsdExecImagingValueKey, int, TfHash>;
    ValueKeyToIndexMap valueKeyToIndexMap;

    /// Maps each adapted prim to its adapter.
    using PrimToAdapterMap = pxr_tsl::robin_map<
        SdfPath, const UsdExecImagingPrimAdapterInterface *, TfHash>;
    PrimToAdapterMap primToAdapterMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif