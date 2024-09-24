//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/color.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"
#include "nc/nanocolor.h"
#include "colorSpace_data.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfColorSpace>();
}

TF_DEFINE_PUBLIC_TOKENS(GfColorSpaceNames, GF_COLORSPACE_NAME_TOKENS);

bool GfColorSpace::IsValid(const TfToken& name) {
    // Retrieve the color space by name, if it exists in the built-in table.
    auto colorSpace = NcGetNamedColorSpace(name.GetString().c_str());
    return colorSpace != nullptr;
}

GfColorSpace::GfColorSpace(const TfToken& name)
: _data(new _Data())
{
    _data->colorSpace = NcGetNamedColorSpace(name.GetString().c_str());
    if (!_data->colorSpace) {
        // A color space constructed with a name that is not a registered name
        // should function like an identity color space; the only reason to do
        // this is to have a sentinel color space meant for comparison and
        // hashing.
        NcColorSpaceM33Descriptor identity;
        identity.name = name.GetString().c_str();
        identity.rgbToXYZ = { 1.0f, 0.0f, 0.0f,
                              0.0f, 1.0f, 0.0f,
                              0.0f, 0.0f, 1.0f };
        identity.gamma = 1.0f;
        identity.linearBias = 0.0f;
        _data->colorSpace = NcCreateColorSpaceM33(&identity, nullptr);
    }
}

// construct a custom colorspace from raw values
GfColorSpace::GfColorSpace(const TfToken& name,
                           const GfVec2f &redChroma,
                           const GfVec2f &greenChroma,
                           const GfVec2f &blueChroma,
                           const GfVec2f &whitePoint,
                           float gamma,
                           float linearBias)
: _data(new _Data())
{
    NcColorSpaceDescriptor desc;
    desc.name = name.GetString().c_str();
    desc.redPrimary.x = redChroma[0];
    desc.redPrimary.y = redChroma[1];
    desc.greenPrimary.x = greenChroma[0];
    desc.greenPrimary.y = greenChroma[1];
    desc.bluePrimary.x = blueChroma[0];
    desc.bluePrimary.y = blueChroma[1];
    desc.whitePoint.x = whitePoint[0];
    desc.whitePoint.y = whitePoint[1];
    desc.gamma = gamma;
    desc.linearBias = linearBias;
    _data->colorSpace = NcCreateColorSpace(&desc);
}

// construct a custom colorspace from a 3x3 matrix and linearization parameters
GfColorSpace::GfColorSpace(const TfToken& name,
                           const GfMatrix3f &rgbToXYZ,
                           float gamma,
                           float linearBias)
: _data(new _Data())
{
    NcColorSpaceM33Descriptor desc;
    desc.name = name.GetString().c_str();
    desc.rgbToXYZ.m[0] = rgbToXYZ[0][0];
    desc.rgbToXYZ.m[1] = rgbToXYZ[0][1];
    desc.rgbToXYZ.m[2] = rgbToXYZ[0][2];
    desc.rgbToXYZ.m[3] = rgbToXYZ[1][0];
    desc.rgbToXYZ.m[4] = rgbToXYZ[1][1];
    desc.rgbToXYZ.m[5] = rgbToXYZ[1][2];
    desc.rgbToXYZ.m[6] = rgbToXYZ[2][0];
    desc.rgbToXYZ.m[7] = rgbToXYZ[2][1];
    desc.rgbToXYZ.m[8] = rgbToXYZ[2][2];
    desc.gamma = gamma;
    desc.linearBias = linearBias;
    _data->colorSpace = NcCreateColorSpaceM33(&desc, nullptr);
}

bool GfColorSpace::operator==(const GfColorSpace &lh) const
{
    return NcColorSpaceEqual(_data->colorSpace, lh._data->colorSpace);
}

/// Convert a packed array of RGB values from one color space to another
void GfColorSpace::ConvertRGBSpan(const GfColorSpace& to, TfSpan<float> rgb) const
{
    // Convert the RGB values in place
    size_t count = rgb.size() / 3;
    if (!count || (count * 3 != rgb.size())) {
        TF_CODING_ERROR("RGB array size must be a multiple of 3");
        return;
    }
    NcTransformColors(to._data->colorSpace, _data->colorSpace, 
                      (NcRGB*) rgb.data(), (int) count);
}

/// Convert a packed array of RGBA values from one color space to another
void GfColorSpace::ConvertRGBASpan(const GfColorSpace& to, TfSpan<float> rgba) const
{
    // Convert the RGBA values in place
    size_t count = rgba.size() / 4;
    if (!count || (count * 4 != rgba.size())) {
        TF_CODING_ERROR("RGBA array size must be a multiple of 4");
        return;
    }
    NcTransformColorsWithAlpha(to._data->colorSpace, _data->colorSpace, 
                               rgba.data(), (int) count);
}

GfColor GfColorSpace::Convert(const GfColorSpace& srcColorSpace, const GfVec3f& rgb) const
{
    GfColor c(rgb, srcColorSpace);
    return GfColor(c, *this);
}

TfToken GfColorSpace::GetName() const
{
    NcColorSpaceM33Descriptor desc;
    if (!NcGetColorSpaceM33Descriptor(_data->colorSpace, &desc)) {
        return TfToken();
    }
    return TfToken(desc.name);
}

GfMatrix3f GfColorSpace::GetRGBToXYZ() const
{
    NcColorSpaceM33Descriptor desc;
    if (!NcGetColorSpaceM33Descriptor(_data->colorSpace, &desc)) {
        return GfMatrix3f(1.0f);
    }
    float* m = desc.rgbToXYZ.m;
    return GfMatrix3f(m[0], m[1], m[2],
                      m[3], m[4], m[5],
                      m[6], m[7], m[8]);
}

float GfColorSpace::GetLinearBias() const
{
    NcColorSpaceM33Descriptor desc;
    if (!NcGetColorSpaceM33Descriptor(_data->colorSpace, &desc)) {
        return 0.0f;
    }
    return desc.linearBias;
}

float GfColorSpace::GetGamma() const
{
    NcColorSpaceM33Descriptor desc;
    if (!NcGetColorSpaceM33Descriptor(_data->colorSpace, &desc)) {
        return 1.0f;
    }
    return desc.gamma;
}

std::pair<float, float> GfColorSpace::GetTransferFunctionParams() const
{
    float K0, phi;
    NcGetK0Phi(_data->colorSpace, &K0, &phi);
    return std::make_pair(K0, phi);
}

std::tuple<GfVec2f, GfVec2f, GfVec2f, GfVec2f>
    GfColorSpace::GetPrimariesAndWhitePoint() const
{
    NcColorSpaceDescriptor desc;
    if (!NcGetColorSpaceDescriptor(_data->colorSpace, &desc)) {
        return std::make_tuple(GfVec2f(0.0f), GfVec2f(0.0f), GfVec2f(0.0f), GfVec2f(0.0f));
    }
    return std::make_tuple(GfVec2f(desc.redPrimary.x, desc.redPrimary.y),
                           GfVec2f(desc.greenPrimary.x, desc.greenPrimary.y),
                           GfVec2f(desc.bluePrimary.x, desc.bluePrimary.y),
                           GfVec2f(desc.whitePoint.x, desc.whitePoint.y));
}


PXR_NAMESPACE_CLOSE_SCOPE
