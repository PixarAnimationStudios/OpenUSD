//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/color.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "nc/nanocolor.h"
#include "colorSpace_data.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfColor>();
}

std::ostream& 
operator<<(std::ostream &out, GfColor const &v)
{
    GfVec3f rgb = v.GetRGB();
    return out << '(' 
        << Gf_OstreamHelperP(rgb[0]) << ", " 
        << Gf_OstreamHelperP(rgb[1]) << ", " 
        << Gf_OstreamHelperP(rgb[2]) << ", "
        << Gf_OstreamHelperP(v.GetColorSpace().GetName().GetString()) << ')';
}

// The default constructor creates black, in the "lin_rec709" space.
GfColor::GfColor()
: GfColor(GfColorSpace(GfColorSpaceNames->LinearRec709))
{
}

// Construct from a colorspace.
GfColor::GfColor(const GfColorSpace& colorSpace)
: _colorSpace(colorSpace)
, _rgb(0, 0, 0)
{
}

// Construct from an rgb tuple and colorspace.
GfColor::GfColor(const GfVec3f &rgb, const GfColorSpace& colorSpace)
: _colorSpace(colorSpace)
, _rgb(rgb)
{
}

// Construct a color from another color into the specified color space.
GfColor::GfColor(const GfColor &srcColor, const GfColorSpace& dstColorSpace)
: _colorSpace(dstColorSpace)
{
    const NcColorSpace* src = srcColor._colorSpace._data->colorSpace;
    const NcColorSpace* dst = dstColorSpace._data->colorSpace;
    NcRGB srcRGB = { srcColor._rgb[0], srcColor._rgb[1], srcColor._rgb[2] };
    NcRGB dstRGB = NcTransformColor(dst, src, srcRGB);
    _rgb = GfVec3f(dstRGB.r, dstRGB.g, dstRGB.b);
}

// Set the color from the Planckian locus (blackbody radiation) temperature
// in Kelvin, in the existing color space.
// Values are computed for temperatures between 1000K and 15000K.
// Note that temperatures below 1900K are out of gamut for Rec709.
void GfColor::SetFromPlanckianLocus(float kelvin, float lumimance)
{
    NcYxy c = NcKelvinToYxy(kelvin, lumimance);
    NcRGB rgb = NcYxyToRGB(_colorSpace._data->colorSpace, c);
    _rgb = GfVec3f(rgb.r, rgb.g, rgb.b);
}

// Get the CIEXY coordinate of the color in the chromaticity chart,
// For use in testing.
GF_API
GfVec2f GfColor::_GetChromaticity() const {
    NcRGB src = {_rgb[0], _rgb[1], _rgb[2]};
    NcXYZ rgb = NcRGBToXYZ(_colorSpace._data->colorSpace, src);
    NcYxy chroma = NcXYZToYxy(rgb);
    return GfVec2f(chroma.x, chroma.y);
}

// Set the color from a CIEXY coordinate in the chromaticity chart,
// normalized such that the max RGB value is 1.0.
// For use in testing.
GF_API
void GfColor::_SetFromChromaticity(const GfVec2f& xy) {
    // Arbitrarily initialize the luminance to 1.
    NcYxy c = { 1.f, xy[0], xy[1] };
    NcRGB rgb = NcYxyToRGB(_colorSpace._data->colorSpace, c);

    // The expectation is that the max RGB of the result is 1.
    NcRGB magRgb = {
        fabsf(rgb.r),
        fabsf(rgb.g),
        fabsf(rgb.b) };
    const float maxc = std::max({ magRgb.r, magRgb.g, magRgb.b });
    if (maxc == 0.f) {
        _rgb = GfVec3f(0, 0, 0);
        return;
    }
    NcRGB normRgb = NcRGB {
        rgb.r / maxc,
        rgb.g / maxc,
        rgb.b / maxc };

    _rgb = GfVec3f(normRgb.r, normRgb.g, normRgb.b);
}

PXR_NAMESPACE_CLOSE_SCOPE
