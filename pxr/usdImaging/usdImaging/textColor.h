//
// Copyright 2023 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_COLOR_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_COLOR_H

/// \file usdImaging/textColor.h

#include "pxr/pxr.h"
#include "pxr/base/gf/math.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \struct UsdImagingTextColor
///
/// The color of text.
///
struct UsdImagingTextColor
{
public:
    /// The red channel.
    float red = 0.0f;

    /// The green channel.
    float green = 0.0f;

    /// The blue channel.
    float blue = 0.0f;

    /// The alpha channel.
    float alpha = 1.0f;

    /// The default constructor
    UsdImagingTextColor() = default;

    /// The default destructor
    ~UsdImagingTextColor() = default;
    
    /// The copy constructor
    UsdImagingTextColor(const UsdImagingTextColor& other) = default;

    /// operator ==.
    bool operator==(const UsdImagingTextColor& other) const
    {
        static const float epsilon = 1e-10f;
        return GfIsClose(red, other.red, epsilon) && GfIsClose(green, other.green, epsilon)
            && GfIsClose(blue, other.blue, epsilon) && GfIsClose(alpha, other.alpha, epsilon);
    }

    /// operator !=.
    bool operator!=(const UsdImagingTextColor& other) const
    { 
        return !(*this == other);
    }
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_TEXT_COLOR_H
