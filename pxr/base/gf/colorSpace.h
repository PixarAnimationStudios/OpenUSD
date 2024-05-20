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
#ifndef PXR_BASE_GF_COLORSPACE_H
#define PXR_BASE_GF_COLORSPACE_H

/// \file gf/color.h
/// \ingroup group_gf_Color

#include "pxr/pxr.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/api.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \enum GfColorSpace
/// \ingroup group_gf_Color
///
/// Color spaces natively supported by Gf.
/// The token names correspond to the canonical names defined
/// by the OpenColorIO Nanocolor project
///
/// In general, the names have the form <curve>_<name> where <curve>
/// is the transfer curve, and <name> is a common name for the color space.
/// If the <curve> portion is ommitted, then it is the same as the <name>.
///
/// Identity, Raw:    No transformation occurs with these.
/// ACEScg:           The Academy Color Encoding System, a color space designed
///                     for cinematic content creation and exchange, with AP1 primaries.
///                     ACEScg also specifies a display transform.
/// AP0:              The ACES AP0 primaries and white point.
/// AP1:              The ACES AP1 primaries and white point.
/// Rec709:           Primaries and whitepoint and EOTF per the Rec. 709 specification.
/// AdobeRGB:         A color space developed by Adobe Systems. It has a wider gamut
///                     than sRGB and is suitable for photography and printing
/// DisplayP3:        DisplayP3 gamut, and linear gamma. Commonly used by wide
///                     gamut HDR monitors.
/// SRGB:             sRGB primaries and whitepoint and EOTF transform function.
/// Texture:          A synonym for identity.
/// sRGB:             A synonym for sRGB_Texture.
/// Rec2020:          Rec2020 primaries, white point, and EOTF.
/// CIEXYZ:           The CIE 1931 XYZ color space, a standard color space used
///                     in color science.
///
/// Colorspaces outside of the set may be defined through explicit construction.
///
#define GF_COLORSPACE_NAME_TOKENS  \
    ((Identity, "identity"))                 \
    ((Raw, "raw"))                           \
    ((ACEScg, "acescg"))                     \
    ((AdobeRGB, "adobergb"))                 \
    ((LinearAdobeRGB, "lin_adobergb"))       \
    ((CIEXYZ, "CIEXYZ"))                     \
    ((LinearAP0, "lin_ap0"))                 \
    ((LinearAP1, "lin_ap1"))                 \
    ((G18AP1, "g18_ap1"))                    \
    ((G22AP1, "g22_ap1"))                    \
    ((LinearRec2020, "lin_rec2020"))         \
    ((LinearRec709, "lin_rec709"))           \
    ((G18Rec709, "g18_rec709"))              \
    ((G22Rec709, "g22_rec709"))              \
    ((LinearDisplayP3, "lin_displayp3"))     \
    ((LinearSRGB, "lin_srgb"))               \
    ((SRGBTexture, "srgb_texture"))          \
    ((SRGB, "sRGB"))                         \
    ((SRGBDisplayP3, "srgb_displayp3"))      \

TF_DECLARE_PUBLIC_TOKENS(GfColorSpaceNames, GF_API, 
                         GF_COLORSPACE_NAME_TOKENS);

class GfColor;

/// \class GfColorSpace
/// \ingroup group_gf_Color
///
/// Basic type: ColorSpace
///
/// This class represents a colorspace. Color spaces may be created by
/// name, parameterization, or by a 3x3 matrix and a gamma operator.
///
/// The parameters used to construct the color space are not available for
/// introspection ~ the color space object is intended for color conversion
/// operations on a GfColor.
///
/// The color spaces natively recognized by GfColorSpace are listed in 
/// GfColorSpaceNames.

class GfColorSpace {
    friend class GfColor;
public:    
    /// Construct a GfColorSpace from a name token.
    ///
    /// \param name The name token of the color space.
    GF_API 
    explicit GfColorSpace(const TfToken& name);

    /// Construct a custom color space from raw values.
    ///
    /// \param name The name token of the color space.
    /// \param redChroma The red chromaticity coordinates.
    /// \param greenChroma The green chromaticity coordinates.
    /// \param blueChroma The blue chromaticity coordinates.
    /// \param whitePoint The white point chromaticity coordinates.
    /// \param gamma The gamma value of the log section.
    /// \param linearBias The linear bias of the log section.
    /// \param K0 The linear break point.
    /// \param phi The slope of the linear section.
    GF_API 
    explicit GfColorSpace(const TfToken& name,
                          const GfVec2f &redChroma,
                          const GfVec2f &greenChroma,
                          const GfVec2f &blueChroma,
                          const GfVec2f &whitePoint,
                          float gamma,
                          float linearBias);

    /// Construct a color space from a 3x3 matrix and linearization parameters.
    ///
    /// \param name The name token of the color space.
    /// \param rgbToXYZ The RGB to XYZ conversion matrix.
    /// \param gamma The gamma value of the log section.
    /// \param linearBias The linear bias of the log section.
    /// \param K0 The linear break point.
    /// \param phi The slope of the linear section.
    GF_API 
    explicit GfColorSpace(const TfToken& name,
                          const GfMatrix3f &rgbToXYZ,
                          float gamma,
                          float linearBias);
    
    /// Get the name of the color space.
    ///
    /// \return The name of the color space.
    GF_API 
    TfToken GetName() const;

    /// Check if two color spaces are equal.
    ///
    /// \param lh The left-hand side color space.
    /// \return True if the color spaces are equal, false otherwise.
    GF_API 
    bool operator ==(const GfColorSpace &rh) const;

    /// Check if two color spaces are not equal.
    ///
    /// \param rh The rigt-hand side color space.
    /// \return True if the color spaces are not equal, false otherwise.
    bool operator !=(const GfColorSpace &rh) const { return !(*this == rh); }

    /// Convert in place a packed array of RGB values from one color space to "this" one.
    ///
    /// \param to The target color space.
    /// \param rgb The packed array of RGB values to convert.
    GF_API 
    void ConvertRGBSpan(const GfColorSpace& srcColorSpace, TfSpan<float> rgb) const;

    /// Convert in place a packed array of RGBA values from one color space to "this one.
    ///
    /// \param to The target color space.
    /// \param rgba The packed array of RGBA values to convert.
    GF_API 
    void ConvertRGBASpan(const GfColorSpace& srcColorSpace, TfSpan<float> rgba) const;

    /// Convert a rgb triplet in a certain color space to "this" color space.
    GF_API
    GfColor Convert(const GfColorSpace& srcColorSpace, const GfVec3f& rgb) const;

    /// Get the RGB to XYZ conversion matrix.
    ///
    /// \return The RGB to XYZ conversion matrix.
    GF_API 
    GfMatrix3f GetRGBToXYZ() const;

    /// Get the gamma value of the color space.
    ///
    /// \return The gamma value of the color space.
    GF_API 
    float GetGamma() const;

    /// Get the linear bias of the color space.
    ///
    /// \return The linear bias of the color space.
    GF_API 
    float GetLinearBias() const;

    /// Get the computed K0 and Phi values for use in the transfer function.
    ///
    GF_API 
    std::pair<float, float> GetTransferFunctionParams() const;

    /// Get the chromaticity coordinates and white point if the color space
    /// was constructed from primaries. The primaries and white points will
    /// be in the order red, green, blue, white. The values will be 
    /// valid if the color space was constructed from primaries or a well formed
    /// primary matrix.
    ///
    /// \return The chromaticity coordinates and white point; 
    /// an empty optional if the color space was not constructed from primaries.
    GF_API 
    std::tuple<GfVec2f, GfVec2f, GfVec2f, GfVec2f>
        GetPrimariesAndWhitePoint() const;

private:
    struct _Data;
    std::shared_ptr<_Data> _data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_COLORSPACE_H
