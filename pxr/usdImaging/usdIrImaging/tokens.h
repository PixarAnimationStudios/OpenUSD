//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IR_IMAGING_TOKENS_H
#define PXR_USD_IMAGING_USD_IR_IMAGING_TOKENS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdIrImaging/api.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define USDIRIMAGING_TOKENS                                                     \
    (baseSphere)                                                                \
    (zAxisCone)

TF_DECLARE_PUBLIC_TOKENS(
    UsdIrImagingTokens,
    USDIRIMAGING_API,
    USDIRIMAGING_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
