//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/lightSamplers.h"

#include "pxr/base/gf/color.h"
#include "pxr/base/gf/colorSpace.h"

#include "pxr/base/gf/range1f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"

namespace {

PXR_NAMESPACE_USING_DIRECTIVE

// -------------------------------------------------------------------------
// General Math Utilities
// -------------------------------------------------------------------------

template <typename T>
constexpr T _pi = static_cast<T>(M_PI);

inline float
_Sqr(float x)
{
    return x*x;
}

// The latitudinal polar coordinate of v, in the range [0, pi]
inline float
_Theta(GfVec3f const& v)
{
    return acosf(GfClamp(v[2], -1.0f, 1.0f));
}

// The longitudinal polar coordinate of v, in the range [0, 2*pi)
inline float
_Phi(GfVec3f const& v)
{
    float p = atan2f(v[1], v[0]);
    return p < 0.0f ? (p + 2.0f * _pi<float>) : p;
}

// Dot product, but set to 0 if less than 0 - ie, 0 for backward-facing rays
inline float
_DotZeroClip(GfVec3f const& a, GfVec3f const& b)
{
    return std::max(0.0f, GfDot(a, b));
}

float
_Smoothstep(float t, GfRange1f range)
{
    const float length = range.GetSize();
    if (length == 0) {
        if (t <= range.GetMin()) {
            // Note that in the case of t == range.GetMin(), we have a
            // degenerate case where there's no clear answer what the "right"
            // thing to do is.

            // I arbitrarily chose 0.0 to return in this case, so at least we
            // have consistent / well defined behavior; could have also done 1.0
            // or 0.5...
            return 0.0;
        }
        return 1.0;
    }
    t = GfClamp((t - range.GetMin())/length, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float
_AreaRect(GfMatrix4f const& xf, float width, float height)
{
    const GfVec3f U = xf.TransformDir(GfVec3f{width, 0.0f, 0.0f});
    const GfVec3f V = xf.TransformDir(GfVec3f{0.0f, height, 0.0f});
    return GfCross(U, V).GetLength();
}

float
_AreaSphere(GfMatrix4f const& xf, float radius)
{
    // Area of the ellipsoid
    const float a = xf.TransformDir(GfVec3f{radius, 0.0f, 0.0f}).GetLength();
    const float b = xf.TransformDir(GfVec3f{0.0f, radius, 0.0f}).GetLength();
    const float c = xf.TransformDir(GfVec3f{0.0f, 0.0f, radius}).GetLength();
    const float ab = powf(a*b, 1.6f);
    const float ac = powf(a*c, 1.6f);
    const float bc = powf(b*c, 1.6f);
    return powf((ab + ac + bc) / 3.0f, 1.0f / 1.6f) * 4.0f * _pi<float>;
}

float
_AreaDisk(GfMatrix4f const& xf, float radius)
{
    // Calculate surface area of the ellipse
    const float a = xf.TransformDir(GfVec3f{radius, 0.0f, 0.0f}).GetLength();
    const float b = xf.TransformDir(GfVec3f{0.0f, radius, 0.0f}).GetLength();
    return _pi<float> * a * b;
}

float
_AreaCylinder(GfMatrix4f const& xf, float radius, float length)
{
    const float c = xf.TransformDir(GfVec3f{length, 0.0f, 0.0f}).GetLength();
    const float a = xf.TransformDir(GfVec3f{0.0f, radius, 0.0f}).GetLength();
    const float b = xf.TransformDir(GfVec3f{0.0f, 0.0f, radius}).GetLength();
    // Ramanujan's approximation to perimeter of ellipse
    const float e =
        _pi<float> * (3.0f * (a + b) - sqrtf((3.0f * a + b) * (a + 3.0f * b)));
    return e * c;
}

// -------------------------------------------------------------------------
// Color utilities
// -------------------------------------------------------------------------

const GfColorSpace _linRec709(GfColorSpaceNames->LinearRec709);
const GfColorSpace _xyzColorSpace(GfColorSpaceNames->CIEXYZ);

// Ideally, we could could move this to GfColor::GetLuminance()
inline float
_GetLuminance(GfColor const& color)
{
    GfColor xyzColor(color, _xyzColorSpace);
    // The "Y" component in XYZ space is luminance
    return xyzColor.GetRGB()[1];
}

const GfVec3f _rec709LuminanceComponents(
    _GetLuminance(GfColor(GfVec3f::XAxis(), _linRec709)),
    _GetLuminance(GfColor(GfVec3f::YAxis(), _linRec709)),
    _GetLuminance(GfColor(GfVec3f::ZAxis(), _linRec709)));


// Recreates UsdLuxBlackbodyTemperatureAsRgb in "pxr/usd/usdLux/blackbody.h"...
/// But uses new GfColor functionality, since we shouldn't import usd into
// imaging

// Perhaps UsdLuxBlackbodyTemperatureAsRgb should be deprecated, and this made
// a new utility function somewhere, for use by other HdRenderDelegates?
// (Maybe in gf/color.h?)
inline GfVec3f
_BlackbodyTemperatureAsRgb(float kelvinColorTemp)
{
    auto tempColor = GfColor(_linRec709);
    // Get color in Rec709 with luminance 1.0
    tempColor.SetFromPlanckianLocus(kelvinColorTemp, 1.0f);
    // We normalize to the luminance of (1,1,1) in Rec709
    GfVec3f tempColorRGB = tempColor.GetRGB();
    float rec709Luminance = GfDot(tempColorRGB, _rec709LuminanceComponents);
    return tempColorRGB / rec709Luminance;
}

// -------------------------------------------------------------------------
// Light sampling structures / utilities
// -------------------------------------------------------------------------

struct _ShapeSample {
    GfVec3f pWorld;
    GfVec3f nWorld;
    GfVec2f uv;
    float invPdfA;
};

GfVec3f
_SampleLightTexture(HdEmbree_LightTexture const& texture, float s, float t)
{
    if (texture.pixels.empty()) {
        return GfVec3f(0.0f);
    }

    int x = float(texture.width) * s;
    int y = float(texture.height) * t;

    return texture.pixels.at(y*texture.width + x);
}

_ShapeSample
_SampleRect(GfMatrix4f const& xf, GfMatrix3f const& normalXform, float width,
            float height, float u1, float u2)
{
    // Sample rectangle in object space
    const GfVec3f pLight(
      (u1 - 0.5f) * width,
      (u2 - 0.5f) * height,
      0.0f
    );
    const GfVec3f nLight(0.0f, 0.0f, -1.0f);
    const GfVec2f uv(u1, u2);

    // Transform to world space
    const GfVec3f pWorld = xf.Transform(pLight);
    const GfVec3f nWorld = (nLight * normalXform).GetNormalized();

    const float area = _AreaRect(xf, width, height);

    return _ShapeSample {
        pWorld,
        nWorld,
        uv,
        area
    };
}

_ShapeSample
_SampleSphere(GfMatrix4f const& xf, GfMatrix3f const& normalXform, float radius,
              float u1, float u2)
{
    // Sample sphere in light space
    const float z = 1.0 - 2.0 * u1;
    const float r = sqrtf(std::max(0.0f, 1.0f - z*z));
    const float phi = 2.0f * _pi<float> * u2;
    GfVec3f pLight{r * std::cos(phi), r * std::sin(phi), z};
    const GfVec3f nLight = pLight;
    pLight *= radius;
    const GfVec2f uv(u2, z);

    // Transform to world space
    const GfVec3f pWorld = xf.Transform(pLight);
    const GfVec3f nWorld = (nLight * normalXform).GetNormalized();

    const float area = _AreaSphere(xf, radius);

    return _ShapeSample {
        pWorld,
        nWorld,
        uv,
        area
    };
}

GfVec3f
_SampleDiskPolar(float u1, float u2)
{
    const float r = sqrtf(u1);
    const float theta = 2.0f * _pi<float> * u2;
    return GfVec3f(r * cosf(theta), r * sinf(theta), 0.0f);
}

_ShapeSample
_SampleDisk(GfMatrix4f const& xf, GfMatrix3f const& normalXform, float radius,
            float u1, float u2)
{
    // Sample disk in light space
    GfVec3f pLight = _SampleDiskPolar(u1, u2);
    const GfVec3f nLight(0.0f, 0.0f, -1.0f);
    const GfVec2f uv(pLight[0], pLight[1]);
    pLight *= radius;

    // Transform to world space
    const GfVec3f pWorld = xf.Transform(pLight);
    const GfVec3f nWorld = (nLight * normalXform).GetNormalized();

    const float area = _AreaDisk(xf, radius);

    return _ShapeSample {
        pWorld,
        nWorld,
        uv,
        area
    };
}

_ShapeSample
_SampleCylinder(GfMatrix4f const& xf, GfMatrix3f const& normalXform,
                float radius,float length, float u1, float u2) {
    float z = GfLerp(u1, -length/2.0f, length/2.0f);
    float phi = u2 * 2.0f * _pi<float>;
    // Compute cylinder sample position _pi_ and normal _n_ from $z$ and $\phi$
    GfVec3f pLight = GfVec3f(z, radius * cosf(phi), radius * sinf(phi));
    // Reproject _pObj_ to cylinder surface and compute _pObjError_
    float hitRad = sqrtf(_Sqr(pLight[1]) + _Sqr(pLight[2]));
    pLight[1] *= radius / hitRad;
    pLight[2] *= radius / hitRad;

    GfVec3f nLight(0.0f, pLight[1], pLight[2]);
    nLight.Normalize();

    // Transform to world space
    const GfVec3f pWorld = xf.Transform(pLight);
    const GfVec3f nWorld = (nLight * normalXform).GetNormalized();

    const float area = _AreaCylinder(xf, radius, length);

    return _ShapeSample {
        pWorld,
        nWorld,
        GfVec2f(u2, u1),
        area
    };
}

GfVec3f
_EvalLightBasic(HdEmbree_LightData const& light)
{
    // Our current material model is always 100% diffuse, so diffuse parameter
    // is a straight multiplier
    GfVec3f Le = light.color * light.intensity * light.diffuse
        * powf(2.0f, light.exposure);
    if (light.enableColorTemperature) {
        Le = GfCompMult(Le,
            _BlackbodyTemperatureAsRgb(light.colorTemperature));
    }
    return Le;
}

HdEmbreeLightSampler::LightSample
_EvalAreaLight(HdEmbree_LightData const& light, _ShapeSample const& ss,
               GfVec3f const& position)
{
    // Transform PDF from area measure to solid angle measure. We use the
    // inverse PDF here to avoid division by zero when the surface point is
    // behind the light
    GfVec3f wI = ss.pWorld - position;
    const float dist = wI.GetLength();
    wI /= dist;
    const float cosThetaOffNormal = _DotZeroClip(-wI, ss.nWorld);
    float invPdfW = cosThetaOffNormal / _Sqr(dist) * ss.invPdfA;
    GfVec3f lightNegZ = -light.xformLightToWorld.GetRow3(2).GetNormalized();
    const float cosThetaOffZ = GfDot(-wI, lightNegZ);

    // Combine the brightness parameters to get initial emission luminance
    // (nits)
    GfVec3f Le = cosThetaOffNormal > 0.0f ?
        _EvalLightBasic(light)
        : GfVec3f(0.0f);

    // Multiply by the texture, if there is one
    if (!light.texture.pixels.empty()) {
        Le = GfCompMult(Le, _SampleLightTexture(light.texture, ss.uv[0],
                                                1.0f - ss.uv[1]));
    }

    // If normalize is enabled, we need to divide the luminance by the surface
    // area of the light, which for an area light is equivalent to multiplying
    // by the area pdf, which is itself the reciprocal of the surface area
    if (light.normalize && ss.invPdfA != 0) {
        Le /= ss.invPdfA;
    }

    // Apply focus shaping
    if (light.shaping.focus > 0.0f) {
        const float ff = powf(GfAbs(cosThetaOffZ), light.shaping.focus);
        const GfVec3f focusTint = GfLerp(ff, light.shaping.focusTint,
                                         GfVec3f(1.0f));
        Le = GfCompMult(Le, focusTint);
    }

    // Apply cone shaping
    const float thetaCone = GfDegreesToRadians(light.shaping.coneAngle);
    const float thetaSoft = GfLerp(light.shaping.coneSoftness, thetaCone, 0.0f);
    const float thetaOffZ = acosf(cosThetaOffZ);
    Le *= 1.0f - _Smoothstep(thetaOffZ, GfRange1f(thetaSoft, thetaCone));

    return HdEmbreeLightSampler::LightSample {
        Le,
        wI,
        dist,
        invPdfW
    };
}

HdEmbreeLightSampler::LightSample
_SampleDomeLight(HdEmbree_LightData const& light, GfVec3f const& direction)
{
    GfVec3f localDirection = light.xformWorldToLight.TransformDir(direction).GetNormalized();
    float t = acosf(localDirection[1]) / _pi<float>;
    float s = atan2f(localDirection[0], localDirection[2]) / (2.0f * _pi<float>);
    s = 1.0 - fmodf(s+0.5f, 1.0f);

    GfVec3f Li = light.texture.pixels.empty() ?
        GfVec3f(1.0f)
        : _SampleLightTexture(light.texture, s, t);

    return HdEmbreeLightSampler::LightSample {
        Li,
        direction,
        std::numeric_limits<float>::max(),
        4.0f * _pi<float>
    };
}

HdEmbreeLightSampler::LightSample
_EvalDomeLight(HdEmbree_LightData const& light, GfVec3f const& W,
               float u1, float u2)
{
    GfVec3f U, V;
    GfBuildOrthonormalFrame(W, &U, &V);

    float z = u1;
    float r = sqrtf(std::max(0.0f, 1.0f - _Sqr(z)));
    float phi = 2.0f * _pi<float> * u2;

    const GfVec3f wI =
        (W * z + r * cosf(phi) * U + r * sinf(phi) * V).GetNormalized();

    HdEmbreeLightSampler::LightSample ls = _SampleDomeLight(light, wI);
    ls.invPdfW = 2.0f * _pi<float>; // We only picked from the hemisphere

    return ls;
}

} // namespace ""

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeLightSampler::LightSample HdEmbreeLightSampler::GetLightSample(
        HdEmbree_LightData const& lightData,
        GfVec3f const& hitPosition,
        GfVec3f const& normal,
        float u1,
        float u2)
{
    HdEmbreeLightSampler lightSampler(lightData, hitPosition, normal, u1, u2);
    return std::visit(lightSampler, lightData.lightVariant);
}

HdEmbreeLightSampler::LightSample HdEmbreeLightSampler::operator()(
        HdEmbree_UnknownLight const& unk) {
    // Could warn, but we should have already warned when lightVariant
    // first created / set to HdEmbree_UnknownLight... and warning here
    // could result in a LOT of spam
    return HdEmbreeLightSampler::LightSample {
        GfVec3f(0.0f),
        GfVec3f(0.0f),
        0.0f,
        0.0f,
    };
}

HdEmbreeLightSampler::LightSample HdEmbreeLightSampler::operator()(
        HdEmbree_Rect const& rect) {
    _ShapeSample shapeSample = _SampleRect(
            _lightData.xformLightToWorld,
            _lightData.normalXformLightToWorld,
            rect.width,
            rect.height,
            _u1,
            _u2);
    return _EvalAreaLight(_lightData, shapeSample, _hitPosition);
}

HdEmbreeLightSampler::LightSample HdEmbreeLightSampler::operator()(
        HdEmbree_Sphere const& sphere) {
    _ShapeSample shapeSample = _SampleSphere(
            _lightData.xformLightToWorld,
            _lightData.normalXformLightToWorld,
            sphere.radius,
            _u1,
            _u2);
    return _EvalAreaLight(_lightData, shapeSample, _hitPosition);
}

HdEmbreeLightSampler::LightSample HdEmbreeLightSampler::operator()(
        HdEmbree_Disk const& disk) {
    _ShapeSample shapeSample = _SampleDisk(
            _lightData.xformLightToWorld,
            _lightData.normalXformLightToWorld,
            disk.radius,
            _u1,
            _u2);
    return _EvalAreaLight(_lightData, shapeSample, _hitPosition);
}

HdEmbreeLightSampler::LightSample HdEmbreeLightSampler::operator()(
        HdEmbree_Cylinder const& cylinder) {
    _ShapeSample shapeSample = _SampleCylinder(
            _lightData.xformLightToWorld,
            _lightData.normalXformLightToWorld,
            cylinder.radius,
            cylinder.length,
            _u1,
            _u2);
    return _EvalAreaLight(_lightData, shapeSample, _hitPosition);
}

HdEmbreeLightSampler::LightSample HdEmbreeLightSampler::operator()(
        HdEmbree_Dome const& dome) {
    return _EvalDomeLight(_lightData, _normal, _u1, _u2);
}

PXR_NAMESPACE_CLOSE_SCOPE
