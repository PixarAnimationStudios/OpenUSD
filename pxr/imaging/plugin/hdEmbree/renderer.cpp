//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/renderer.h"

#include "pxr/imaging/plugin/hdEmbree/config.h"
#include "pxr/imaging/plugin/hdEmbree/light.h"
#include "pxr/imaging/plugin/hdEmbree/mesh.h"
#include "pxr/imaging/plugin/hdEmbree/renderBuffer.h"

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/gf/color.h"
#include "pxr/base/gf/colorSpace.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/range1f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/work/loops.h"

#include "pxr/base/tf/hash.h"

#include <embree3/rtcore_common.h>
#include <embree3/rtcore_geometry.h>
#include <embree3/rtcore_scene.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <stdint.h>
#include <thread>

namespace {

PXR_NAMESPACE_USING_DIRECTIVE

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

template <typename T>
constexpr T _pi = static_cast<T>(M_PI);

constexpr float _rayHitContinueBias = 0.001f;

constexpr float _minLuminanceCutoff = 1e-9f;

constexpr GfVec3f _invalidColor = GfVec3f(-std::numeric_limits<float>::infinity());

// -------------------------------------------------------------------------
// General Math Utilities
// -------------------------------------------------------------------------

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
// General Ray Utilities
// -------------------------------------------------------------------------

inline GfVec3f
_CalculateHitPosition(RTCRayHit const& rayHit)
{
    return GfVec3f(rayHit.ray.org_x + rayHit.ray.tfar * rayHit.ray.dir_x,
                   rayHit.ray.org_y + rayHit.ray.tfar * rayHit.ray.dir_y,
                   rayHit.ray.org_z + rayHit.ray.tfar * rayHit.ray.dir_z);
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

struct _LightSample {
    GfVec3f Li;
    GfVec3f wI;
    float dist;
    float invPdfW;
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
    GfVec3f Le = light.color * light.intensity * powf(2.0f, light.exposure);
    if (light.enableColorTemperature) {
        Le = GfCompMult(Le,
            _BlackbodyTemperatureAsRgb(light.colorTemperature));
    }
    return Le;
}

_LightSample
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

    return _LightSample {
        Le,
        wI,
        dist,
        invPdfW
    };
}

class _LightSampler {
public:
    static _LightSample GetLightSample(HdEmbree_LightData const& lightData,
                                       GfVec3f const& hitPosition,
                                       GfVec3f const& normal,
                                       float u1,
                                       float u2)
    {
        _LightSampler lightSampler(lightData, hitPosition, normal, u1, u2);
        return std::visit(lightSampler, lightData.lightVariant);
    }

    // callables to be used with std::visit
    _LightSample operator()(HdEmbree_UnknownLight const& rect) {
        // Could warn, but we should have already warned when lightVariant
        // first created / set to HdEmbree_UnknownLight... and warning here
        // could result in a LOT of spam
        return _LightSample {
            GfVec3f(0.0f),
            GfVec3f(0.0f),
            0.0f,
            0.0f,
        };
    }

    _LightSample operator()(HdEmbree_Rect const& rect) {
        _ShapeSample shapeSample = _SampleRect(
            _lightData.xformLightToWorld,
            _lightData.normalXformLightToWorld,
            rect.width,
            rect.height,
            _u1,
            _u2);
        return _EvalAreaLight(_lightData, shapeSample, _hitPosition);
    }

    _LightSample operator()(HdEmbree_Sphere const& sphere) {
        _ShapeSample shapeSample = _SampleSphere(
            _lightData.xformLightToWorld,
            _lightData.normalXformLightToWorld,
            sphere.radius,
            _u1,
            _u2);
        return _EvalAreaLight(_lightData, shapeSample, _hitPosition);
    }

    _LightSample operator()(HdEmbree_Disk const& disk) {
        _ShapeSample shapeSample = _SampleDisk(
            _lightData.xformLightToWorld,
            _lightData.normalXformLightToWorld,
            disk.radius,
            _u1,
            _u2);
        return _EvalAreaLight(_lightData, shapeSample, _hitPosition);
    }

    _LightSample operator()(HdEmbree_Cylinder const& cylinder) {
        _ShapeSample shapeSample = _SampleCylinder(
            _lightData.xformLightToWorld,
            _lightData.normalXformLightToWorld,
            cylinder.radius,
            cylinder.length,
            _u1,
            _u2);
        return _EvalAreaLight(_lightData, shapeSample, _hitPosition);
    }

private:
    _LightSampler(HdEmbree_LightData const& lightData,
                 GfVec3f const& hitPosition,
                 GfVec3f const& normal,
                 float u1,
                 float u2) :
        _lightData(lightData),
        _hitPosition(hitPosition),
        _normal(normal),
        _u1(u1),
        _u2(u2)
    {}

    HdEmbree_LightData const& _lightData;
    GfVec3f const& _hitPosition;
    GfVec3f const& _normal;
    float _u1;
    float _u2;
};

}  // anonymous namespace

PXR_NAMESPACE_OPEN_SCOPE

HdEmbreeRenderer::HdEmbreeRenderer()
    : _aovBindings()
    , _aovNames()
    , _aovBindingsNeedValidation(false)
    , _aovBindingsValid(false)
    , _width(0)
    , _height(0)
    , _viewMatrix(1.0f) // == identity
    , _projMatrix(1.0f) // == identity
    , _inverseViewMatrix(1.0f) // == identity
    , _inverseProjMatrix(1.0f) // == identity
    , _scene(nullptr)
    , _samplesToConvergence(0)
    , _ambientOcclusionSamples(0)
    , _enableSceneColors(false)
    , _completedSamples(0)
{
}

HdEmbreeRenderer::~HdEmbreeRenderer() = default;

void
HdEmbreeRenderer::SetScene(RTCScene scene)
{
    _scene = scene;
}

void
HdEmbreeRenderer::SetSamplesToConvergence(int samplesToConvergence)
{
    _samplesToConvergence = samplesToConvergence;
}

void
HdEmbreeRenderer::SetAmbientOcclusionSamples(int ambientOcclusionSamples)
{
    _ambientOcclusionSamples = ambientOcclusionSamples;
}

void
HdEmbreeRenderer::SetEnableSceneColors(bool enableSceneColors)
{
    _enableSceneColors = enableSceneColors;
}

void
HdEmbreeRenderer::SetRandomNumberSeed(int randomNumberSeed)
{
    _randomNumberSeed = randomNumberSeed;
}

void
HdEmbreeRenderer::SetDataWindow(const GfRect2i &dataWindow)
{
    _dataWindow = dataWindow;

    // Here for clients that do not use camera framing but the
    // viewport.
    //
    // Re-validate the attachments, since attachment viewport and
    // render viewport need to match.
    _aovBindingsNeedValidation = true;
}

void
HdEmbreeRenderer::SetCamera(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projMatrix)
{
    _viewMatrix = viewMatrix;
    _projMatrix = projMatrix;
    _inverseViewMatrix = viewMatrix.GetInverse();
    _inverseProjMatrix = projMatrix.GetInverse();
}

void
HdEmbreeRenderer::SetAovBindings(
    HdRenderPassAovBindingVector const &aovBindings)
{
    _aovBindings = aovBindings;
    _aovNames.resize(_aovBindings.size());
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        _aovNames[i] = HdParsedAovToken(_aovBindings[i].aovName);
    }

    // Re-validate the attachments.
    _aovBindingsNeedValidation = true;
}


void
HdEmbreeRenderer::AddLight(SdfPath const& lightPath,
                           HdEmbree_Light* light)
{
    ScopedLock lightsWriteLock(_lightsWriteMutex);
    _lightMap[lightPath] = light;
}

void
HdEmbreeRenderer::RemoveLight(SdfPath const& lightPath, HdEmbree_Light* light)
{
    ScopedLock lightsWriteLock(_lightsWriteMutex);
    _lightMap.erase(lightPath);
}

bool
HdEmbreeRenderer::_ValidateAovBindings()
{
    if (!_aovBindingsNeedValidation) {
        return _aovBindingsValid;
    }

    _aovBindingsNeedValidation = false;
    _aovBindingsValid = true;

    for (size_t i = 0; i < _aovBindings.size(); ++i) {

        // By the time the attachment gets here, there should be a bound
        // output buffer.
        if (_aovBindings[i].renderBuffer == nullptr) {
            TF_WARN("Aov '%s' doesn't have any renderbuffer bound",
                    _aovNames[i].name.GetText());
            _aovBindingsValid = false;
            continue;
        }

        if (_aovNames[i].name != HdAovTokens->color &&
            _aovNames[i].name != HdAovTokens->cameraDepth &&
            _aovNames[i].name != HdAovTokens->depth &&
            _aovNames[i].name != HdAovTokens->primId &&
            _aovNames[i].name != HdAovTokens->instanceId &&
            _aovNames[i].name != HdAovTokens->elementId &&
            _aovNames[i].name != HdAovTokens->Neye &&
            _aovNames[i].name != HdAovTokens->normal &&
            !_aovNames[i].isPrimvar) {
            TF_WARN("Unsupported attachment with Aov '%s' won't be rendered to",
                    _aovNames[i].name.GetText());
        }

        HdFormat format = _aovBindings[i].renderBuffer->GetFormat();

        // depth is only supported for float32 attachments
        if ((_aovNames[i].name == HdAovTokens->cameraDepth ||
             _aovNames[i].name == HdAovTokens->depth) &&
            format != HdFormatFloat32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // ids are only supported for int32 attachments
        if ((_aovNames[i].name == HdAovTokens->primId ||
             _aovNames[i].name == HdAovTokens->instanceId ||
             _aovNames[i].name == HdAovTokens->elementId) &&
            format != HdFormatInt32) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // Normal is only supported for vec3 attachments of float.
        if ((_aovNames[i].name == HdAovTokens->Neye ||
             _aovNames[i].name == HdAovTokens->normal) &&
            format != HdFormatFloat32Vec3) {
            TF_WARN("Aov '%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // Primvars support vec3 output (though some channels may not be used).
        if (_aovNames[i].isPrimvar &&
            format != HdFormatFloat32Vec3) {
            TF_WARN("Aov 'primvars:%s' has unsupported format '%s'",
                    _aovNames[i].name.GetText(),
                    TfEnum::GetName(format).c_str());
            _aovBindingsValid = false;
        }

        // color is only supported for vec3/vec4 attachments of float,
        // unorm, or snorm.
        if (_aovNames[i].name == HdAovTokens->color) {
            switch(format) {
                case HdFormatUNorm8Vec4:
                case HdFormatUNorm8Vec3:
                case HdFormatSNorm8Vec4:
                case HdFormatSNorm8Vec3:
                case HdFormatFloat32Vec4:
                case HdFormatFloat32Vec3:
                    break;
                default:
                    TF_WARN("Aov '%s' has unsupported format '%s'",
                        _aovNames[i].name.GetText(),
                        TfEnum::GetName(format).c_str());
                    _aovBindingsValid = false;
                    break;
            }
        }

        // make sure the clear value is reasonable for the format of the
        // attached buffer.
        if (!_aovBindings[i].clearValue.IsEmpty()) {
            HdTupleType clearType =
                HdGetValueTupleType(_aovBindings[i].clearValue);

            // array-valued clear types aren't supported.
            if (clearType.count != 1) {
                TF_WARN("Aov '%s' clear value type '%s' is an array",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str());
                _aovBindingsValid = false;
            }

            // color only supports float/double vec3/4
            if (_aovNames[i].name == HdAovTokens->color &&
                clearType.type != HdTypeFloatVec3 &&
                clearType.type != HdTypeFloatVec4 &&
                clearType.type != HdTypeDoubleVec3 &&
                clearType.type != HdTypeDoubleVec4) {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str());
                _aovBindingsValid = false;
            }

            // only clear float formats with float, int with int, float3 with
            // float3.
            if ((format == HdFormatFloat32 && clearType.type != HdTypeFloat) ||
                (format == HdFormatInt32 && clearType.type != HdTypeInt32) ||
                (format == HdFormatFloat32Vec3 &&
                 clearType.type != HdTypeFloatVec3)) {
                TF_WARN("Aov '%s' clear value type '%s' isn't compatible with"
                        " format %s",
                        _aovNames[i].name.GetText(),
                        _aovBindings[i].clearValue.GetTypeName().c_str(),
                        TfEnum::GetName(format).c_str());
                _aovBindingsValid = false;
            }
        }
    }

    return _aovBindingsValid;
}

/* static */
GfVec4f
HdEmbreeRenderer::_GetClearColor(VtValue const& clearValue)
{
    HdTupleType type = HdGetValueTupleType(clearValue);
    if (type.count != 1) {
        return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }

    switch(type.type) {
        case HdTypeFloatVec3:
        {
            GfVec3f f =
                *(static_cast<const GfVec3f*>(HdGetValueData(clearValue)));
            return GfVec4f(f[0], f[1], f[2], 1.0f);
        }
        case HdTypeFloatVec4:
        {
            GfVec4f f =
                *(static_cast<const GfVec4f*>(HdGetValueData(clearValue)));
            return f;
        }
        case HdTypeDoubleVec3:
        {
            GfVec3d f =
                *(static_cast<const GfVec3d*>(HdGetValueData(clearValue)));
            return GfVec4f(f[0], f[1], f[2], 1.0f);
        }
        case HdTypeDoubleVec4:
        {
            GfVec4d f =
                *(static_cast<const GfVec4d*>(HdGetValueData(clearValue)));
            return GfVec4f(f);
        }
        default:
            return GfVec4f(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

void
HdEmbreeRenderer::Clear()
{
    if (!_ValidateAovBindings()) {
        return;
    }

    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        if (_aovBindings[i].clearValue.IsEmpty()) {
            continue;
        }

        HdEmbreeRenderBuffer *rb = 
            static_cast<HdEmbreeRenderBuffer*>(_aovBindings[i].renderBuffer);

        rb->Map();
        if (_aovNames[i].name == HdAovTokens->color) {
            GfVec4f clearColor = _GetClearColor(_aovBindings[i].clearValue);
            rb->Clear(4, clearColor.data());
        } else if (rb->GetFormat() == HdFormatInt32) {
            int32_t clearValue = _aovBindings[i].clearValue.Get<int32_t>();
            rb->Clear(1, &clearValue);
        } else if (rb->GetFormat() == HdFormatFloat32) {
            float clearValue = _aovBindings[i].clearValue.Get<float>();
            rb->Clear(1, &clearValue);
        } else if (rb->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f clearValue = _aovBindings[i].clearValue.Get<GfVec3f>();
            rb->Clear(3, clearValue.data());
        } // else, _ValidateAovBindings would have already warned.

        rb->Unmap();
        rb->SetConverged(false);
    }
}

void
HdEmbreeRenderer::MarkAovBuffersUnconverged()
{
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdEmbreeRenderBuffer *rb =
            static_cast<HdEmbreeRenderBuffer*>(_aovBindings[i].renderBuffer);
        rb->SetConverged(false);
    }
}

int
HdEmbreeRenderer::GetCompletedSamples() const
{
    return _completedSamples.load();
}

static
bool
_IsContained(const GfRect2i &rect, int width, int height)
{
    return
        rect.GetMinX() >= 0 && rect.GetMaxX() < width &&
        rect.GetMinY() >= 0 && rect.GetMaxY() < height;
}

void
HdEmbreeRenderer::_PreRenderSetup()
{
    _completedSamples.store(0);

    // Commit any pending changes to the scene.
    rtcCommitScene(_scene);

    if (!_ValidateAovBindings()) {
        // We aren't going to render anything. Just mark all AOVs as converged
        // so that we will stop rendering.
        for (size_t i = 0; i < _aovBindings.size(); ++i) {
            HdEmbreeRenderBuffer *rb = static_cast<HdEmbreeRenderBuffer*>(
                _aovBindings[i].renderBuffer);
            rb->SetConverged(true);
        }
        // XXX:validation
        TF_WARN("Could not validate Aovs. Render will not complete");
        return;
    }

    _width  = 0;
    _height = 0;

    // Map all of the attachments.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        //
        // XXX
        //
        // A scene delegate might specify the path to a
        // render buffer instead of a pointer to the
        // render buffer.
        //
        static_cast<HdEmbreeRenderBuffer*>(
            _aovBindings[i].renderBuffer)->Map();

        if (i == 0) {
            _width  = _aovBindings[i].renderBuffer->GetWidth();
            _height = _aovBindings[i].renderBuffer->GetHeight();
        } else {
            if ( _width  != _aovBindings[i].renderBuffer->GetWidth() ||
                 _height != _aovBindings[i].renderBuffer->GetHeight()) {
                TF_CODING_ERROR(
                    "Embree render buffers have inconsistent sizes");
            }
        }
    }

    if (_width > 0 || _height > 0) {
        if (!_IsContained(_dataWindow, _width, _height)) {
            TF_CODING_ERROR(
                "dataWindow is larger than render buffer");
        }
    }
}

void
HdEmbreeRenderer::Render(HdRenderThread *renderThread)
{
    _PreRenderSetup();

    // Render the image. Each pass through the loop adds a sample per pixel
    // (with jittered ray direction); the longer the loop runs, the less noisy
    // the image becomes. We add a cancellation point once per loop.
    //
    // We consider the image converged after N samples, which is a convenient
    // and simple heuristic.
    for (int i = 0; i < _samplesToConvergence; ++i) {
        // Pause point.
        while (renderThread->IsPauseRequested()) {
            if (renderThread->IsStopRequested()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        // Cancellation point.
        if (renderThread->IsStopRequested()) {
            break;
        }

        const unsigned int tileSize = HdEmbreeConfig::GetInstance().tileSize;
        const unsigned int numTilesX =
            (_dataWindow.GetWidth() + tileSize-1) / tileSize;
        const unsigned int numTilesY =
            (_dataWindow.GetHeight() + tileSize-1) / tileSize;

        // Render by scheduling square tiles of the sample buffer in a parallel
        // for loop.
        // Always pass the renderThread to _RenderTiles to allow the first frame
        // to be interrupted.
        WorkParallelForN(numTilesX*numTilesY,
            std::bind(&HdEmbreeRenderer::_RenderTiles, this,
                renderThread, i, std::placeholders::_1, std::placeholders::_2));

        // After the first pass, mark the single-sampled attachments as
        // converged and unmap them. If there are no multisampled attachments,
        // we are done.
        if (i == 0) {
            bool moreWork = false;
            for (size_t i = 0; i < _aovBindings.size(); ++i) {
                HdEmbreeRenderBuffer *rb = static_cast<HdEmbreeRenderBuffer*>(
                    _aovBindings[i].renderBuffer);
                if (rb->IsMultiSampled()) {
                    moreWork = true;
                }
            }
            if (!moreWork) {
                _completedSamples.store(i+1);
                break;
            }
        }

        // Track the number of completed samples for external consumption.
        _completedSamples.store(i+1);

        // Cancellation point.
        if (renderThread->IsStopRequested()) {
            break;
        }
    }

    // Mark the multisampled attachments as converged and unmap all buffers.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdEmbreeRenderBuffer *rb = static_cast<HdEmbreeRenderBuffer*>(
            _aovBindings[i].renderBuffer);
        rb->Unmap();
        rb->SetConverged(true);
    }
}

void
HdEmbreeRenderer::_RenderTiles(HdRenderThread *renderThread, int sampleNum,
                               size_t tileStart, size_t tileEnd)
{
    const unsigned int minX = _dataWindow.GetMinX();
    unsigned int minY = _dataWindow.GetMinY();
    const unsigned int maxX = _dataWindow.GetMaxX() + 1;
    unsigned int maxY = _dataWindow.GetMaxY() + 1;

    // If a client does not use AOVs and we have no render buffers,
    // _height is 0 and we shouldn't use it to flip the data window.
    if (_height > 0) {
        // The data window is y-Down but the image line order
        // is from bottom to top, so we need to flip it.
        std::swap(minY, maxY);
        minY = _height - minY;
        maxY = _height - maxY;
    }

    const unsigned int tileSize =
        HdEmbreeConfig::GetInstance().tileSize;
    const unsigned int numTilesX =
        (_dataWindow.GetWidth() + tileSize-1) / tileSize;

    // Initialize the RNG for this tile (each tile creates one as
    // a lazy way to do thread-local RNGs).
    size_t seed;
    if (_randomNumberSeed == -1) {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
    } else {
        seed = static_cast<size_t>(_randomNumberSeed);
    }
    seed = TfHash::Combine(seed, tileStart);
    seed = TfHash::Combine(seed, sampleNum);
    std::default_random_engine random(seed);

    // Create a uniform distribution for jitter calculations.
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    auto uniform_float = [&random, &uniform_dist]() {
        return uniform_dist(random);
    };

    // _RenderTiles gets a range of tiles; iterate through them.
    for (unsigned int tile = tileStart; tile < tileEnd; ++tile) {

        // Cancellation point.
        if (renderThread && renderThread->IsStopRequested()) {
            break;
        }

        // Compute the pixel location of tile boundaries.
        const unsigned int tileY = tile / numTilesX;
        const unsigned int tileX = tile - tileY * numTilesX; 
        // (Above is equivalent to: tileX = tile % numTilesX)
        const unsigned int x0 = tileX * tileSize + minX;
        const unsigned int y0 = tileY * tileSize + minY;
        // Clamp to data window, in case tileSize doesn't
        // neatly divide its with and height.
        const unsigned int x1 = std::min(x0+tileSize, maxX);
        const unsigned int y1 = std::min(y0+tileSize, maxY);

        // Loop over pixels casting rays.
        for (unsigned int y = y0; y < y1; ++y) {
            for (unsigned int x = x0; x < x1; ++x) {
                // Jitter the camera ray direction.
                GfVec2f jitter(0.0f, 0.0f);
                if (HdEmbreeConfig::GetInstance().jitterCamera) {
                    jitter = GfVec2f(uniform_float(), uniform_float());
                }

                // Un-transform the pixel's NDC coordinates through the
                // projection matrix to get the trace of the camera ray in the
                // near plane.
                const float w(_dataWindow.GetWidth());
                const float h(_dataWindow.GetHeight());

                const GfVec3f ndc(
                    2.0f * ((x + jitter[0] - minX) / w) - 1.0f,
                    2.0f * ((y + jitter[1] - minY) / h) - 1.0f,
                    -1.0f);
                const GfVec3f nearPlaneTrace(_inverseProjMatrix.Transform(ndc));

                GfVec3f origin;
                GfVec3f dir;

                const bool isOrthographic = round(_projMatrix[3][3]) == 1.0;
                if (isOrthographic) {
                    // During orthographic projection: trace parallel rays
                    // from the near plane trace.
                    origin = nearPlaneTrace;
                    dir = GfVec3f(0.0f, 0.0f, -1.0f);
                } else {
                    // Otherwise, assume this is a perspective projection;
                    // project from the camera origin through the
                    // near plane trace.
                    origin = GfVec3f(0.0f, 0.0f, 0.0f);
                    dir = nearPlaneTrace;
                }
                // Transform camera rays to world space.
                origin = GfVec3f(_inverseViewMatrix.Transform(origin));
                dir = GfVec3f(
                    _inverseViewMatrix.TransformDir(dir)).GetNormalized();

                // Trace the ray.
                _TraceRay(x, y, origin, dir, random);
            }
        }
    }
}

/// Fill in an RTCRay structure from the given parameters.
static void
_PopulateRay(RTCRay *ray, GfVec3f const& origin, 
             GfVec3f const& dir, float nearest,
             float furthest = std::numeric_limits<float>::infinity(),
             HdEmbree_RayMask mask = HdEmbree_RayMask::All)
{
    ray->org_x = origin[0];
    ray->org_y = origin[1];
    ray->org_z = origin[2];
    ray->tnear = nearest;

    ray->dir_x = dir[0];
    ray->dir_y = dir[1];
    ray->dir_z = dir[2];
    ray->time = 0.0f;

    ray->tfar = furthest;
    ray->mask = static_cast<uint32_t>(mask);
}

/// Fill in an RTCRayHit structure from the given parameters.
// note this containts a Ray and a RayHit
static void
_PopulateRayHit(RTCRayHit* rayHit, GfVec3f const& origin,
             GfVec3f const& dir, float nearest,
             float furthest = std::numeric_limits<float>::infinity(),
             HdEmbree_RayMask mask = HdEmbree_RayMask::All)
{
    // Fill in defaults for the ray
    _PopulateRay(&rayHit->ray, origin, dir, nearest, furthest, mask);

    // Fill in defaults for the hit
    rayHit->hit.primID = RTC_INVALID_GEOMETRY_ID;
    rayHit->hit.geomID = RTC_INVALID_GEOMETRY_ID;
}

/// Generate a random cosine-weighted direction ray (in the hemisphere
/// around <0,0,1>).  The input is a pair of uniformly distributed random
/// numbers in the range [0,1].
///
/// The algorithm here is to generate a random point on the disk, and project
/// that point to the unit hemisphere.
static GfVec3f
_CosineWeightedDirection(GfVec2f const& uniform_float)
{
    GfVec3f dir;
    float theta = 2.0f * _pi<float> * uniform_float[0];
    float eta = uniform_float[1];
    float sqrteta = sqrtf(eta);
    dir[0] = cosf(theta) * sqrteta;
    dir[1] = sinf(theta) * sqrteta;
    dir[2] = sqrtf(1.0f-eta);
    return dir;
}

void
HdEmbreeRenderer::_TraceRay(unsigned int x, unsigned int y,
                            GfVec3f const &origin, GfVec3f const &dir,
                            std::default_random_engine &random)
{
    // Intersect the camera ray.
    RTCRayHit rayHit; // EMBREE_FIXME: use RTCRay for occlusion rays
    rayHit.ray.flags = 0;
    _PopulateRayHit(&rayHit, origin, dir, 0.0f,
                    std::numeric_limits<float>::max(),
                    HdEmbree_RayMask::Camera);
    {
      RTCIntersectContext context;
      rtcInitIntersectContext(&context);
      rtcIntersect1(_scene, &context, &rayHit);
      //
      // there is something odd about how this is used in Embree. Is it reversed
      // here and then when it it used in
      //      _ComputeNormal
      //      _ComputeColor
      // but not when it is used in
      //      _EmbreeCullFacess
      // this should probably all made to be consistent. What would make most
      // sense would be to remove this reversal, and then just change the test
      // in _EmbreeCullFaces. this would be the most performant solution.
      //
      rayHit.hit.Ng_x = -rayHit.hit.Ng_x;
      rayHit.hit.Ng_y = -rayHit.hit.Ng_y;
      rayHit.hit.Ng_z = -rayHit.hit.Ng_z;
    }

    // Write AOVs to attachments that aren't converged.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        HdEmbreeRenderBuffer *renderBuffer =
            static_cast<HdEmbreeRenderBuffer*>(_aovBindings[i].renderBuffer);

        if (renderBuffer->IsConverged()) {
            continue;
        }

        if (_aovNames[i].name == HdAovTokens->color) {
            GfVec4f clearColor = _GetClearColor(_aovBindings[i].clearValue);
            GfVec4f sample = _ComputeColor(rayHit, random, clearColor);
            renderBuffer->Write(GfVec3i(x,y,1), 4, sample.data());
        } else if ((_aovNames[i].name == HdAovTokens->cameraDepth ||
                    _aovNames[i].name == HdAovTokens->depth) &&
                   renderBuffer->GetFormat() == HdFormatFloat32) {
            float depth;
            bool clip = (_aovNames[i].name == HdAovTokens->depth);
            if(_ComputeDepth(rayHit, &depth, clip)) {
                renderBuffer->Write(GfVec3i(x,y,1), 1, &depth);
            }
        } else if ((_aovNames[i].name == HdAovTokens->primId ||
                    _aovNames[i].name == HdAovTokens->elementId ||
                    _aovNames[i].name == HdAovTokens->instanceId) &&
                   renderBuffer->GetFormat() == HdFormatInt32) {
            int32_t id;
            if (_ComputeId(rayHit, _aovNames[i].name, &id)) {
                renderBuffer->Write(GfVec3i(x,y,1), 1, &id);
            }
        } else if ((_aovNames[i].name == HdAovTokens->Neye ||
                    _aovNames[i].name == HdAovTokens->normal) &&
                   renderBuffer->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f normal;
            bool eye = (_aovNames[i].name == HdAovTokens->Neye);
            if (_ComputeNormal(rayHit, &normal, eye)) {
                renderBuffer->Write(GfVec3i(x,y,1), 3, normal.data());
            }
        } else if (_aovNames[i].isPrimvar &&
                   renderBuffer->GetFormat() == HdFormatFloat32Vec3) {
            GfVec3f value;
            if (_ComputePrimvar(rayHit, _aovNames[i].name, &value)) {
                renderBuffer->Write(GfVec3i(x,y,1), 3, value.data());
            }
        }
    }
}

bool
HdEmbreeRenderer::_ComputeId(RTCRayHit const& rayHit, TfToken const& idType,
                             int32_t *id)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    // Get the instance and prototype context structures for the hit prim.
    // We don't use embree's multi-level instancing; we
    // flatten everything in hydra. So instID[0] should always be correct.
    const HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
            rtcGetGeometryUserData(rtcGetGeometry(_scene,
                                                  rayHit.hit.instID[0])));

    const HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
            rtcGetGeometryUserData(rtcGetGeometry(instanceContext->rootScene,
                                                  rayHit.hit.geomID)));

    if (idType == HdAovTokens->primId) {
        *id = prototypeContext->rprim->GetPrimId();
    } else if (idType == HdAovTokens->elementId) {
        if (prototypeContext->primitiveParams.empty()) {
            *id = rayHit.hit.primID;
        } else {
            *id = HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(
                prototypeContext->primitiveParams[rayHit.hit.primID]);
        }
    } else if (idType == HdAovTokens->instanceId) {
        *id = instanceContext->instanceId;
    } else {
        return false;
    }

    return true;
}

bool
HdEmbreeRenderer::_ComputeDepth(RTCRayHit const& rayHit,
                                float *depth,
                                bool clip)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    if (clip) {
        GfVec3f hitPos = _CalculateHitPosition(rayHit);

        hitPos = GfVec3f(_viewMatrix.Transform(hitPos));
        hitPos = GfVec3f(_projMatrix.Transform(hitPos));

        // For the depth range transform, we assume [0,1].
        *depth = (hitPos[2] + 1.0f) / 2.0f;
    } else {
        *depth = rayHit.ray.tfar;
    }
    return true;
}

bool
HdEmbreeRenderer::_ComputeNormal(RTCRayHit const& rayHit,
                                 GfVec3f *normal,
                                 bool eye)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    // We don't use embree's multi-level instancing; we
    // flatten everything in hydra. So instID[0] should always be correct.
    const HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(_scene,
                                                      rayHit.hit.instID[0])));

    const HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetGeometryUserData(
                    rtcGetGeometry(instanceContext->rootScene,
                                   rayHit.hit.geomID)));

    GfVec3f n = -GfVec3f(rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z);
    auto it = prototypeContext->primvarMap.find(HdTokens->normals);
    if (it != prototypeContext->primvarMap.end()) {
        it->second->Sample(rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, &n);
    }

    n = instanceContext->objectToWorldMatrix.TransformDir(n);
    if (eye) {
        n = GfVec3f(_viewMatrix.TransformDir(n));
    }
    n.Normalize();

    *normal = n;
    return true;
}

bool
HdEmbreeRenderer::_ComputePrimvar(RTCRayHit const& rayHit,
                                  TfToken const& primvar,
                                  GfVec3f *value)
{
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }

    // We don't use embree's multi-level instancing; we
    // flatten everything in hydra. So instID[0] should always be correct.
    const HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(_scene,
                                                      rayHit.hit.instID[0])));

    const HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetGeometryUserData(
                    rtcGetGeometry(instanceContext->rootScene,
                                   rayHit.hit.geomID)));

    // XXX: This is a little clunky, although sample will early out if the
    // types don't match.
    auto it = prototypeContext->primvarMap.find(primvar);
    if (it != prototypeContext->primvarMap.end()) {
        const HdEmbreePrimvarSampler *sampler = it->second;
        if (sampler->Sample(rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
                            value)) {
            return true;
        }
        GfVec2f v2;
        if (sampler->Sample(rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
                            &v2)) {
            value->Set(v2[0], v2[1], 0.0f);
            return true;
        }
        float v1;
        if (sampler->Sample(rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
                            &v1)) {
            value->Set(v1, 0.0f, 0.0f);
            return true;
        }
    }
    return false;
}

float
HdEmbreeRenderer::_Visibility(
    GfVec3f const& position, GfVec3f const& direction, float dist) const
{
    RTCRay shadow;
    shadow.flags = 0;
    _PopulateRay(&shadow, position, direction, 0.001f, dist,
                 HdEmbree_RayMask::Shadow);
    {
        RTCIntersectContext context;
        rtcInitIntersectContext(&context);
        rtcOccluded1(_scene,&context,&shadow);
    }
    // XXX: what do we do about shadow visibility (continuation) here?
    // probably need to use rtcIntersect instead of rtcOccluded

    // occluded sets tfar < 0 if the ray hit anything
    return shadow.tfar > 0.0f;
}

GfVec4f
HdEmbreeRenderer::_ComputeColor(RTCRayHit const& rayHit,
                                std::default_random_engine &random,
                                GfVec4f const& clearColor)
{
    // Get the instance and prototype context structures for the hit prim.
    // We don't use embree's multi-level instancing; we
    // flatten everything in hydra. So instID[0] should always be correct.
    const HdEmbreeInstanceContext *instanceContext =
        static_cast<HdEmbreeInstanceContext*>(
                rtcGetGeometryUserData(rtcGetGeometry(_scene,
                                                      rayHit.hit.instID[0])));

    const HdEmbreePrototypeContext *prototypeContext =
        static_cast<HdEmbreePrototypeContext*>(
                rtcGetGeometryUserData(
                    rtcGetGeometry(instanceContext->rootScene,
                                   rayHit.hit.geomID)));

    // Compute the worldspace location of the rayHit hit.
    GfVec3f hitPos = _CalculateHitPosition(rayHit);

    // If a normal primvar is present (e.g. from smooth shading), use that
    // for shading; otherwise use the flat face normal.
    GfVec3f normal = -GfVec3f(rayHit.hit.Ng_x, rayHit.hit.Ng_y,
                              rayHit.hit.Ng_z);
    auto it = prototypeContext->primvarMap.find(HdTokens->normals);
    if (it != prototypeContext->primvarMap.end()) {
        it->second->Sample(
            rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, &normal);
    }

    // If a color primvar is present, use that as diffuse color; otherwise,
    // use flat grey.
    GfVec3f materialColor = _invalidColor;
    if (_enableSceneColors) {
        auto it = prototypeContext->primvarMap.find(HdTokens->displayColor);
        if (it != prototypeContext->primvarMap.end()) {
            it->second->Sample(
                rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v, &materialColor);
        }
    }

    // Transform the normal from object space to world space.
    normal = instanceContext->objectToWorldMatrix.TransformDir(normal);

    // Make sure the normal is unit-length.
    normal.Normalize();

    GfVec3f lightingColor(0.0f);

    // If there are no lights, then keep the existing camera light + AO path to
    // be able to inspect the scene
    if (_lightMap.empty())
    {
        // For ambient occlusion, default material is flat 50% gray
        if (materialColor == _invalidColor) {
            materialColor = GfVec3f(.5f);
        }

        // Lighting model: (camera dot normal), i.e. diffuse-only point light
        // centered on the camera.
        GfVec3f dir = GfVec3f(rayHit.ray.dir_x, rayHit.ray.dir_y,
                              rayHit.ray.dir_z);
        float diffuseLight = fabs(GfDot(-dir, normal)) *
            HdEmbreeConfig::GetInstance().cameraLightIntensity;

        // Lighting gets modulated by an ambient occlusion term.
        float aoLightIntensity =
            _ComputeAmbientOcclusion(hitPos, normal, random);

        // XXX: We should support opacity here...

        lightingColor = GfVec3f(diffuseLight * aoLightIntensity);
    }
    else
    {
        // For lighting, default material is 100% white
        if (materialColor == _invalidColor) {
            materialColor = GfVec3f(1.0f);
        }

        lightingColor = _ComputeLighting(
            hitPos, normal,random, prototypeContext);
    }
    const GfVec3f finalColor = GfCompMult(materialColor, lightingColor);

    // Clamp colors to > 0
    GfVec4f output;
    output[0] = std::max(0.0f, finalColor[0]);
    output[1] = std::max(0.0f, finalColor[1]);
    output[2] = std::max(0.0f, finalColor[2]);
    output[3] = 1.0f;
    return output;
}

float
HdEmbreeRenderer::_ComputeAmbientOcclusion(GfVec3f const& position,
                                            GfVec3f const& normal,
                                            std::default_random_engine &random)
{
    // Create a uniform random distribution for AO calculations.
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    auto uniform_float = [&random, &uniform_dist]() {
        return uniform_dist(random);
    };

    // 0 ambient occlusion samples means disable the ambient occlusion term.
    if (_ambientOcclusionSamples < 1) {
        return 1.0f;
    }

    float occlusionFactor = 0.0f;

    // For hemisphere sampling we need to choose a coordinate frame at this
    // point. For the purposes of _CosineWeightedDirection, the normal needs
    // to map to (0,0,1), but since the distribution is radially symmetric
    // we don't care about the other axes.
    GfMatrix3f basis(1.0f);
    GfVec3f xAxis;
    if (fabsf(GfDot(normal, GfVec3f(0.0f,0.0f,1.0f))) < 0.9f) {
        xAxis = GfCross(normal, GfVec3f(0.0f,0.0f,1.0f));
    } else {
        xAxis = GfCross(normal, GfVec3f(0.0f,1.0f,0.0f));
    }
    GfVec3f yAxis = GfCross(normal, xAxis);
    basis.SetColumn(0, xAxis.GetNormalized());
    basis.SetColumn(1, yAxis.GetNormalized());
    basis.SetColumn(2, normal);

    // Generate random samples, stratified with Latin Hypercube Sampling.
    // https://en.wikipedia.org/wiki/Latin_hypercube_sampling
    // Stratified sampling means we don't get all of our random samples
    // bunched in the far corner of the hemisphere, but instead have some
    // equal spacing guarantees.
    std::vector<GfVec2f> samples;
    samples.resize(_ambientOcclusionSamples);
    for (int i = 0; i < _ambientOcclusionSamples; ++i) {
        samples[i][0] = (float(i) + uniform_float()) / _ambientOcclusionSamples;
    }
    std::shuffle(samples.begin(), samples.end(), random);
    for (int i = 0; i < _ambientOcclusionSamples; ++i) {
        samples[i][1] = (float(i) + uniform_float()) / _ambientOcclusionSamples;
    }

    // Trace ambient occlusion rays. The occlusion factor is the fraction of
    // the hemisphere that's occluded when rays are traced to infinity,
    // computed by random sampling over the hemisphere.
    for (int i = 0; i < _ambientOcclusionSamples; i++)
    {
        // Sample in the hemisphere centered on the face normal. Use
        // cosine-weighted hemisphere sampling to bias towards samples which
        // will have a bigger effect on the occlusion term.
        GfVec3f shadowDir = basis * _CosineWeightedDirection(samples[i]);

        // Trace shadow ray, using the fast interface (rtcOccluded) since
        // we only care about intersection status, not intersection id.
        RTCRay shadow;
        shadow.flags = 0;
        _PopulateRay(&shadow, position, shadowDir, 0.001f);
        {
          RTCIntersectContext context;
          rtcInitIntersectContext(&context);
          rtcOccluded1(_scene,&context,&shadow);
        }

        // Record this AO ray's contribution to the occlusion factor: a
        // boolean [In shadow/Not in shadow].
        // shadow is occluded when shadow.ray.tfar < 0.0f
        // notice this is reversed since "it's a visibility ray, and
        // the occlusionFactor is really an ambientLightFactor."
        if (shadow.tfar > 0.0f)
            occlusionFactor += GfDot(shadowDir, normal);
    }
    // Compute the average of the occlusion samples.
    occlusionFactor /= _ambientOcclusionSamples;

    return occlusionFactor;
}

GfVec3f
HdEmbreeRenderer::_ComputeLighting(
    GfVec3f const& position,
    GfVec3f const& normal,
    std::default_random_engine &random,
    HdEmbreePrototypeContext const* prototypeContext) const
{
    std::uniform_real_distribution<float> uniform_dist(0.0f, 1.0f);
    auto uniform_float = [&random, &uniform_dist]() {
        return uniform_dist(random);
    };

    GfVec3f finalColor(0.0f);
    // For now just a 100% reflective diffuse BRDF
    float brdf = 1.0f / _pi<float>;

    // For now just iterate over all lights
    /// XXX: simple uniform sampling may be better here
    for (auto const& it : _lightMap)
    {
        auto const& light = it.second->LightData();
        // Skip light if it's hidden
        if (!light.visible)
        {
            continue;
        }

        // Sample the light
        _LightSample ls = _LightSampler::GetLightSample(
            light, position, normal, uniform_float(), uniform_float());
        if (GfIsClose(ls.Li, GfVec3f(0.0f), _minLuminanceCutoff)) {
            continue;
        }

        // Trace shadow
        float vis = _Visibility(position, ls.wI, ls.dist * 0.99f);

        // Add exitant luminance
        finalColor += ls.Li
            * _DotZeroClip(ls.wI, normal)
            * brdf
            * vis
            * ls.invPdfW;
    }
    return finalColor;
}

PXR_NAMESPACE_CLOSE_SCOPE
