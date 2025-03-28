//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_VOL_IMAGING_TOKENS_H
#define PXR_USD_IMAGING_USD_VOL_IMAGING_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdVolImaging/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USDVOLIMAGING_TOKENS \
    (field3dAsset)           \
    (openvdbAsset)

TF_DECLARE_PUBLIC_TOKENS(UsdVolImagingTokens, USDVOLIMAGING_API, USDVOLIMAGING_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_VOL_IMAGING_TOKENS_H
