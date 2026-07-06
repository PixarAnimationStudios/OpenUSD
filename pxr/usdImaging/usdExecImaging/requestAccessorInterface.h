//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_REQUEST_ACCESSOR_INTERFACE_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_REQUEST_ACCESSOR_INTERFACE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/api.h"

#include "pxr/base/vt/value.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class UsdExecImagingValueKey;

using UsdExecImagingRequestAccessorInterfaceSharedPtr =
    std::shared_ptr<class UsdExecImagingRequestAccessorInterface>;

/// Interface for reading exec-computed values within an implementation of
/// UsdExecImagingPrimAdapterInterface.
///
class UsdExecImagingRequestAccessorInterface
{
public:
    USDEXECIMAGING_API
    virtual ~UsdExecImagingRequestAccessorInterface();

    /// Reads the computed value identified by \p valueKey.
    ///
    /// The calling UsdExecImagingAdapter must have added the equivalent value
    /// key in its implementation of BuildRequest, or else this method may
    /// return an empty value. If the requested \p valueKey was not previously
    /// added to the exec request, this emits a TF_CODING_ERROR.
    ///
    virtual VtValue GetComputedValue(
        const UsdExecImagingValueKey &valueKey) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif