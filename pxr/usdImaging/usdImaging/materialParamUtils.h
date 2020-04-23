//
// Copyright 2020 Pixar
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
#ifndef PXR_USD_IMAGING_MATERIAL_PARAM_UTILS_H
#define PXR_USD_IMAGING_MATERIAL_PARAM_UTILS_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdAttribute;
class UsdTimeCode;
class VtValue;

/// Get the value from the usd attribute at given time. If it is an
/// SdfAssetPath containing a UDIM pattern, e.g., //SHOW/myImage.<UDIM>.exr,
/// the resolved path of the SdfAssetPath will be updated to a file path
/// with a UDIM pattern, e.g., /filePath/myImage.<UDIM>.exr.
/// There might be support for different patterns, e.g., myImage._MAPID_.exr,
/// but this function always normalizes it to myImage.<UDIM>.exr.
///
/// The function assumes that the correct ArResolverContext is bound.
///
VtValue
UsdImaging_ResolveMaterialParamValue(
    const UsdAttribute& attr, const UsdTimeCode& time);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_MATERIAL_PARAM_UTILS_H
