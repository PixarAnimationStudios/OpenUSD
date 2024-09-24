//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// Color spaces natively supported by Gf to define scene referred color values.
/// The token names correspond to the canonical names defined
/// by the OpenColorIO Nanocolor project
///
/// The names have the form <Curve><Name> where <Curve> is the transfer curve, 
/// and <Name> is the common name for the color space.
///
/// The curves include:
///
/// Linear:     Linear transfer function.
/// G18:        Gamma 1.8 transfer function.
/// G22:        Gamma 2.2 transfer function.
/// SRGB:       sRGB transfer function, comprised of a linear and gamma segment.
///
/// The named color spaces refer to a set of primaries and a white point.
///
/// AP0:        The ACES2065-1 AP0 primaries and corresponding D60 white point.
/// AP1:        The ACEScg AP1 primaries and corresponding D60 white point.
/// Rec2020:    Rec2020 primaries and corresponding D65 white point.
/// Rec709:     Rec7709 primaries and corresponding D65 white point.
/// AdobeRGB:   A color space developed by Adobe Systems. It's primaries define
///             a wider gamut than sRGB. It has a D65 white point.
/// DisplayP3:  P3 primaries, a D65 whitepoint. Commonly used 
///             by wide gamut HDR monitors.
/// CIEXYZ:     The CIE 1931 XYZ color space.
/// Data, Raw,
/// Unknown:    No transformation occurs with these.
///
/// User defined color spaces outside of this set may be defined through
/// explicit construction.
///
#define GF_COLORSPACE_NAME_TOKENS                \
    ((CIEXYZ, "lin_ciexyzd65_scene"))            \
    ((Data, "data"))                             \
    ((Raw, "raw"))                               \
    ((Unknown, "unknown"))                       \
    ((LinearAdobeRGB, "lin_adobergb_scene"))     \
    ((LinearAP0, "lin_ap0_scene"))               \
    ((LinearAP1, "lin_ap1_scene"))               \
    ((LinearDisplayP3, "lin_displayp3_scene"))   \
    ((LinearRec2020, "lin_rec2020_scene"))       \
    ((LinearRec709, "lin_rec709_scene"))         \
    ((G18Rec709, "g18_rec709_scene"))            \
    ((G22AdobeRGB, "g22_adobergb_scene"))        \
    ((G22AP1, "g22_ap1_scene"))                  \
    ((G22Rec709, "g22_rec709_scene"))            \
    ((SRGBP3D65, "srgb_p3d65_scene"))            \
    ((SRGBRec709, "srgb_rec709_scene"))          \
    ((SRGBAP1, "srgb_ap1_scene"))

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

    /// Check if a color space name is valid for constructing
    /// a GfColorSpace by name.
    GF_API
    static bool IsValid(const TfToken& name);

    /// Construct a custom color space from raw values.
    ///
    /// \param name The name token of the color space.
    /// \param redChroma The red chromaticity coordinates.
    /// \param greenChroma The green chromaticity coordinates.
    /// \param blueChroma The blue chromaticity coordinates.
    /// \param whitePoint The white point chromaticity coordinates.
    /// \param gamma The gamma value of the log section.
    /// \param linearBias The linear bias of the log section.
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
