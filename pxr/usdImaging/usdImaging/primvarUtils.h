//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_PRIMVAR_UTILS_H
#define PXR_USD_IMAGING_USD_IMAGING_PRIMVAR_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/enums.h"

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;

/// Converts from \c usdRole to the corresponding Hd role.
USDIMAGING_API
TfToken UsdImagingUsdToHdRole(TfToken const& usdRole);

/// Converts from \c usdInterp to the corresponding \c HdInterpolation
USDIMAGING_API
HdInterpolation UsdImagingUsdToHdInterpolation(TfToken const& usdInterp);

/// Converts from \c usdInterp to the token for the corresponding
/// \c HdInterpolation.
USDIMAGING_API
TfToken UsdImagingUsdToHdInterpolationToken(TfToken const& usdInterp);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
