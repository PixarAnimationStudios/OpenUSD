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

// Set the color from a CIEXY coordinate in the chromaticity chart.
void GfColor::SetFromChromaticity(const GfVec2f& xy)
{
    NcYxy c = { 1.f, xy[0], xy[1] };
    NcRGB rgb = NcYxyToRGB(_colorSpace._data->colorSpace, c);
    _rgb = GfVec3f(rgb.r, rgb.g, rgb.b);
}

// Set the color from a NcXYZ coordinate, in the existing color space.
void GfColor::SetFromXYZ(const GfVec3f& xyz)
{
    NcXYZ ncxyz = { xyz[0], xyz[1], xyz[2] };
    NcRGB dst = NcXYZToRGB(_colorSpace._data->colorSpace, ncxyz);
    _rgb = GfVec3f(dst.r, dst.g, dst.b);
}

// Set the color from blackbody temperature in Kelvin, 
// converting to the existing color space.
void GfColor::SetFromBlackbodyKelvin(float kelvin, float lumimance)
{
    NcYxy c = NcKelvinToYxy(kelvin, lumimance);
    NcRGB rgb = NcYxyToRGB(_colorSpace._data->colorSpace, c);
    _rgb = GfVec3f(rgb.r, rgb.g, rgb.b);
}

// Get the XYZ coordinate of the color.
GfVec3f GfColor::GetXYZ() const
{
    NcRGB src = {_rgb[0], _rgb[1], _rgb[2]};
    NcXYZ dst = NcRGBToXYZ(_colorSpace._data->colorSpace, src);
    return GfVec3f(dst.x, dst.y, dst.z);
}

// Get the CIEXY coordinate of the color in the chromaticity chart.
GF_API
GfVec2f GfColor::GetChromaticity() const
{
    NcRGB src = {_rgb[0], _rgb[1], _rgb[2]};
    NcXYZ rgb = NcRGBToXYZ(_colorSpace._data->colorSpace, src);
    NcYxy chroma = NcXYZToYxy(rgb);
    return GfVec2f(chroma.x, chroma.y);
}

PXR_NAMESPACE_CLOSE_SCOPE
