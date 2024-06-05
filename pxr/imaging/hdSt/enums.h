//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
