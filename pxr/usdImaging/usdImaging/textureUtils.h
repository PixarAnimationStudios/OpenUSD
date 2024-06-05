//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXTURE_UTILS_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXTURE_UTILS_H

#include "pxr/pxr.h"

#include "pxr/base/tf/token.h"

#include "pxr/usd/sdf/layer.h"

#include "pxr/usdImaging/usdImaging/api.h"

#include <tuple>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \deprecated HdSceneDelegate no longer implements GetTextureResource.
///
/// The functions below are used by the old texture system where the
/// scene delegates creates the texture resource in
/// GetTextureResource.
///
/// Note: these functions are also not binding the Usd stage's
/// resolver context and thus don't handle some cases (e.g., model
/// search paths) correctly.
///
/// The corresponding functions for the new texture system are in
/// usdImaging/materialParamUtils.cpp and HdStUdimTextureObject.
///
USDIMAGING_API
std::vector<std::tuple<int, TfToken>>
UsdImaging_GetUdimTiles(
    std::string const& basePath,
    int tileLimit,
    SdfLayerHandle const& layerHandle = SdfLayerHandle());

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXTURE_UTILS_H
