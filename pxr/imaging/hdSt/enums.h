//
// Copyright 2024 Pixar
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
#ifndef PXR_IMAGING_HDST_ENUMS_H
#define PXR_IMAGING_HDST_ENUMS_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \enum HdStTextureType
///
/// Enumerates Storm's supported texture types.
///
/// Uv:    Sample the uv coordinates and accesses a single 2d texture.
///
/// Field: Transform coordinates by matrix before accessing a single 3d
///        texture.
///
/// Ptex:  Use the ptex connectivity information to sample a ptex texture.
///
/// Udim:  Remap the uv coordinates into udim coordinates using a maximum
///        tile width of 10 and sample all the udim tiles found in the
///        file system.
///
enum class HdStTextureType
{
    Uv,
    Field,
    Ptex,
    Udim
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDST_ENUMS_H
