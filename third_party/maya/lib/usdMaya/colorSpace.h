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
#ifndef PXRUSDMAYA_COLORSPACE_H
#define PXRUSDMAYA_COLORSPACE_H

/// \file colorSpace.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "pxr/base/gf/gamma.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Helper functions for dealing with colors stored in Maya.
///
/// Technically, this doesn't need to be tied to Usd.
namespace PxrUsdMayaColorSpace
{

/// Returns true if we treat colors from Maya as linear colors.
///
/// Before color management (viewport 1.0), all Maya colors were stored with
/// gamma correction.  When we have a mix of shapes we need to draw, some shaded
/// via native Maya and others with our custom shapes, we need to know if the
/// Maya colors are considered linear or not.  If things are color correct, our
/// shape needs to write linear colors to the framebuffer and we leave the final
/// correction up to Maya.  Otherwise, we want to draw things as if they were
/// modeled in Maya.  While this may not be "correct" in all situations, at
/// least it is consistent with native Maya shading.
///
/// Currently, this value is controlled via an environment variable:
///
/// PIXMAYA_LINEAR_COLORS
///
/// You should only be setting that if you've more or less fully switched to
/// Viewport 2.0 (as proper color management is only supported there).
///
PXRUSDMAYA_API
bool
IsColorManaged();

/// Converts a linear color into the appropriate Maya color space as determined by
/// the above \c IsColorManaged.
template <typename T>
T
ConvertLinearToMaya(const T& linearColor)
{
    return IsColorManaged() ? linearColor : GfConvertLinearToDisplay(linearColor);
}

/// Converts a Maya color space into a linear color.
template <typename T>
T
ConvertMayaToLinear(const T& mayaColor)
{
    return IsColorManaged() ? mayaColor : GfConvertDisplayToLinear(mayaColor);
}

}; // PxrUsdMayaColorSpace

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_COLORSPACE_H

