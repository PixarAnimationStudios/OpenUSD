//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_REQUEST_BUILDER_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_REQUEST_BUILDER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/requestBuilderInterface.h"
#include "pxr/usdImaging/usdExecImaging/valueKey.h"
#include "pxr/usdImaging/usdExecImaging/valueKeyMap.h"

#include "pxr/exec/execUsd/valueKey.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdExecImagingPrimAdapterInterface;

/// This class implements UsdExecImagingRequestBuilderInterface by building up a
/// set of ExecUsdValueKeys and an associated mapping of value keys back to
/// indices.
///
/// The UsdExecImaging_Request constructs an instance of this object when it
/// (re)builds the exec request. For each discovered prim that has an adapter,
/// the request sets the current prim and adapter on this object, then passes
/// the builder to the adapter for it to add its value keys. Those value keys
/// are accumulated in the builder as well as metadata for each value key
/// (stored as an instance of UsdExecImaging_ValueKeyMap).
///
class UsdExecImaging_RequestBuilder
    : public UsdExecImagingRequestBuilderInterface
{
public:
    /// \name UsdExecImagingRequestBuilder implementation
    ///
    /// @{

    void AddValueKey(
        const UsdPrim &providerPrim,
        const TfToken &computationName) override;

    void AddValueKey(
        const UsdAttribute &providerAttribute) override;
    
    /// @}

    /// Records that subsequent calls to AddValueKey are from \p primAdapter
    /// which is adapting the given \p prim.
    ///
    void SetAdaptedPrim(
        const UsdPrim &prim,
        const UsdExecImagingPrimAdapterInterface &primAdapter);

    /// Moves the vector of ExecUsdValueKeys out of this object.
    std::vector<ExecUsdValueKey> TakeValueKeys();

    /// Moves the value key map out of this object.
    UsdExecImaging_ValueKeyMap TakeValueKeyMap();

private:
    void _AddValueKey(UsdExecImagingValueKey valueKey);

private:
    // Path to the prim currently being adapted.
    SdfPath _adaptedPrimPath;

    // The current prim adapter that is adding value keys.
    const UsdExecImagingPrimAdapterInterface *_primAdapter;

    // The accumulated vector of value keys.
    std::vector<ExecUsdValueKey> _valueKeys;

    // Additional information for each added value key, including which prim
    // adapter added which value key.
    UsdExecImaging_ValueKeyMap _valueKeyMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif