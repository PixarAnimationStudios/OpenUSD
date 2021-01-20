//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
