//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_REQUEST_BUILDER_INTERFACE_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_REQUEST_BUILDER_INTERFACE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdAttribute;
class UsdPrim;
class TfToken;

/// Interface for building up an exec request within an implementation of
/// UsdExecImagingPrimAdapterInterface.
///
class UsdExecImagingRequestBuilderInterface
{
public:
    USDEXECIMAGING_API
    virtual ~UsdExecImagingRequestBuilderInterface();

    /// The calling UsdExecImagingPrimAdapterInterface requires the computation of
    /// \p computationName provided by the prim \p providerPrim.
    ///
    virtual void AddValueKey(
        const UsdPrim &providerPrim,
        const TfToken &computationName) = 0;

    /// The calling UsdExecImagingPrimAdapterInterface requires the computed value of
    /// \p providerAttribute.
    ///
    virtual void AddValueKey(
        const UsdAttribute &providerAttribute) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif