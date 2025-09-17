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
/// GfColorSpace defines color spaces natively supported by Gf to define scene
/// referred color values. The descriptive names are those published by the 
/// Color Interop Forum.
///
/// The short names take the form of three components joined by underscores. 
/// 
/// The first component is the encoding of the RGB tuple; either `lin` for 
/// linear encoding, `srgb` for IEC 61966-2-1:1999 encoding, or gNN where NN 
/// indicates a gamma value. 
///
/// The second component designates the color primaries and white point, and
/// are a short name of a corresponding CIF color space.
///
/// Finally, the third component names the image state, as named in ISO 22028-1,
/// drawing a distinction between scene-referred and display-referred color 
/// spaces. Scene referenced color spaces are used to describe the color 
/// appearance of objects in the real world, while display-referred color spaces
/// are used to describe color as emitted by projectors or monitor screen.
///
/// ACEScg: lin_ap1_scene
/// ACES2065-1: lin_ap0_scene
/// Linear Rec.709 (sRGB): lin_rec709_scene
/// Linear P3-D65: lin_p3d65_scene
/// Linear Rec.2020: lin_rec2020_scene
/// Linear AdobeRGB: lin_adobergb_scene
/// CIE XYZ-D65 - Scene-referred: lin_ciexyzd65_scene
/// sRGB Encoded Rec.709 (sRGB): srgb_rec709_scene
/// Gamma 2.2 Encoded Rec.709: g22_rec709_scene
/// Gamma 1.8 Encoded Rec.709: g18_rec709_scene
/// sRGB Encoded AP1: srgb_ap1_scene
/// Gamma 2.2 Encoded AP1: g22_ap1_scene
/// sRGB Encoded P3-D65: srgb_p3d65_scene
/// Gamma 2.2 Encoded AdobeRGB: g22_adobergb_scene
/// Data: data
/// Unknown: unknown
///
/// `lin_ciexyzd65_scene` bears some additional explanation. The `d65` component
/// in the name is meant to indicate that values transformed to this color space
/// should be adapted to the D65 white point.
///
/// In addition the `data` and `unknown` color space names, `raw` and `identity`
/// are provided for compatibility with existing production assets. The CIEXYZ
/// and LinearDisplayP3 names are deprecated and will be removed in a future 
/// release.
///
/// User defined color spaces outside of this set may be defined through
/// explicit construction.
///
#define GF_COLORSPACE_NAME_TOKENS                \
    ((LinearAP1, "lin_ap1_scene"))               \
    ((LinearAP0, "lin_ap0_scene"))               \
    ((LinearRec709, "lin_rec709_scene"))         \
    ((LinearP3D65, "lin_p3d65_scene"))           \
    ((LinearRec2020, "lin_rec2020_scene"))       \
    ((LinearAdobeRGB, "lin_adobergb_scene"))     \
    ((LinearCIEXYZD65, "lin_ciexyzd65_scene"))   \
    ((SRGBRec709, "srgb_rec709_scene"))          \
    ((G22Rec709, "g22_rec709_scene"))            \
    ((G18Rec709, "g18_rec709_scene"))            \
    ((SRGBAP1, "srgb_ap1_scene"))                \
    ((G22AP1, "g22_ap1_scene"))                  \
    ((SRGBP3D65, "srgb_p3d65_scene"))            \
    ((G22AdobeRGB, "g22_adobergb_scene"))        \
    ((Identity, "identity"))                     \
    ((Data, "data"))                             \
    ((Raw, "raw"))                               \
    ((Unknown, "unknown"))                       \
    ((CIEXYZ, "lin_ciexyzd65_scene"))            \
    ((LinearDisplayP3, "lin_p3d65_scene")) 

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

    /// Get the RGB to RGB conversion matrix from srcColorSpace to "this" color space.
    /// \param srcColorSpace The source color space.
    /// \return The RGB to RGB conversion matrix.
    GF_API 
    GfMatrix3f GetRGBToRGB(const GfColorSpace& srcColorSpace) const;

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
