//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

//  Lookup table from:
//  Colour Rendering of Spectra
//  by John Walker
//  https://www.fourmilab.ch/documents/specrend/specrend.c
//
//  Covers range from 1000k to 10000k in 500k steps
//  assuming Rec709 / sRGB colorspace chromaticity.
//
// NOTE: 6500K doesn't give a pure white because the D65
//       illuminant used by Rec. 709 doesn't lie on the
//       Planckian Locus. We would need to compute the
//       Correlated Colour Temperature (CCT) using Ohno's
//       method to get pure white. Maybe one day.
//
// Note that the beginning and ending knots are repeated to simplify
// boundary behavior.  The last 4 knots represent the segment starting
// at 1.0.
//
static GfVec3f const _blackbodyRGB[] = {
    GfVec3f(1.000000f, 0.027490f, 0.000000f), //  1000 K (Approximation)
    GfVec3f(1.000000f, 0.027490f, 0.000000f), //  1000 K (Approximation)
    GfVec3f(1.000000f, 0.149664f, 0.000000f), //  1500 K (Approximation)
    GfVec3f(1.000000f, 0.256644f, 0.008095f), //  2000 K
    GfVec3f(1.000000f, 0.372033f, 0.067450f), //  2500 K
    GfVec3f(1.000000f, 0.476725f, 0.153601f), //  3000 K
    GfVec3f(1.000000f, 0.570376f, 0.259196f), //  3500 K
    GfVec3f(1.000000f, 0.653480f, 0.377155f), //  4000 K
    GfVec3f(1.000000f, 0.726878f, 0.501606f), //  4500 K
    GfVec3f(1.000000f, 0.791543f, 0.628050f), //  5000 K
    GfVec3f(1.000000f, 0.848462f, 0.753228f), //  5500 K
    GfVec3f(1.000000f, 0.898581f, 0.874905f), //  6000 K
    GfVec3f(1.000000f, 0.942771f, 0.991642f), //  6500 K
    GfVec3f(0.906947f, 0.890456f, 1.000000f), //  7000 K
    GfVec3f(0.828247f, 0.841838f, 1.000000f), //  7500 K
    GfVec3f(0.765791f, 0.801896f, 1.000000f), //  8000 K
    GfVec3f(0.715255f, 0.768579f, 1.000000f), //  8500 K
    GfVec3f(0.673683f, 0.740423f, 1.000000f), //  9000 K
    GfVec3f(0.638992f, 0.716359f, 1.000000f), //  9500 K
    GfVec3f(0.609681f, 0.695588f, 1.000000f), // 10000 K
    GfVec3f(0.609681f, 0.695588f, 1.000000f), // 10000 K
    GfVec3f(0.609681f, 0.695588f, 1.000000f)  // 10000 K
};

// Catmull-Rom basis
static const float _basis[4][4] = {
    {-0.5f,  1.5f, -1.5f,  0.5f},
    { 1.f,  -2.5f,  2.0f, -0.5f},
    {-0.5f,  0.0f,  0.5f,  0.0f},
    { 0.f,   1.0f,  0.0f,  0.0f}
};

static inline float _Rec709RgbToLuma(const GfVec3f &rgb)
{
    return GfDot(rgb, GfVec3f(0.2126f, 0.7152f, 0.0722f));
}

static GfVec3f _BlackbodyTemperatureAsRgb(float temp)
{
    // Catmull-Rom interpolation of _blackbodyRGB
    constexpr int numKnots = sizeof(_blackbodyRGB) / sizeof(_blackbodyRGB[0]);
    // Parametric distance along spline
    const float u_spline = GfClamp((temp - 1000.0f) / 9000.0f, 0.0f, 1.0f);
    // Last 4 knots represent a trailing segment starting at u_spline==1.0,
    // to simplify boundary behavior
    constexpr int numSegs = (numKnots-4);
    const float x = u_spline * numSegs;
    const int seg = int(floor(x));
    const float u_seg = x-seg; // Parameter within segment
    // Knot values for this segment
    GfVec3f k0 = _blackbodyRGB[seg+0];
    GfVec3f k1 = _blackbodyRGB[seg+1];
    GfVec3f k2 = _blackbodyRGB[seg+2];
    GfVec3f k3 = _blackbodyRGB[seg+3];
    // Compute cubic coefficients.  Could fold constants (zero, one) here
    // if speed is a concern.
    GfVec3f a=_basis[0][0]*k0+_basis[0][1]*k1+_basis[0][2]*k2+_basis[0][3]*k3;
    GfVec3f b=_basis[1][0]*k0+_basis[1][1]*k1+_basis[1][2]*k2+_basis[1][3]*k3;
    GfVec3f c=_basis[2][0]*k0+_basis[2][1]*k1+_basis[2][2]*k2+_basis[2][3]*k3;
    GfVec3f d=_basis[3][0]*k0+_basis[3][1]*k1+_basis[3][2]*k2+_basis[3][3]*k3;
    // Eval cubic polynomial.
    GfVec3f rgb = ((a*u_seg+b)*u_seg+c)*u_seg+d;
    // Normalize to the same luminance as (1,1,1)
    rgb /= _Rec709RgbToLuma(rgb);
    // Clamp at zero, since the spline can produce small negative values,
    // e.g. in the blue component at 1300k.
    rgb[0] = GfMax(rgb[0], 0.f);
    rgb[1] = GfMax(rgb[1], 0.f);
    rgb[2] = GfMax(rgb[2], 0.f);
    return rgb;
}

HdStLight::HdStLight(SdfPath const &id, TfToken const &lightType)
    : HdLight(id),
    _lightType(lightType)
{
}

HdStLight::~HdStLight() = default;

GlfSimpleLight
HdStLight::_ApproximateAreaLight(SdfPath const &id, 
                                 HdSceneDelegate *sceneDelegate)
{
    if (!sceneDelegate->GetVisible(id)) {
        GlfSimpleLight l;
        l.SetAmbient(GfVec4f(0.0f));
        l.SetDiffuse(GfVec4f(0.0f));
        l.SetSpecular(GfVec4f(0.0f));
        l.SetHasIntensity(false);
        return l;
    }

    // Get the color of the light
    GfVec3f hdc = sceneDelegate->GetLightParamValue(id, HdLightTokens->color)
            .Get<GfVec3f>();

    // Color temperature
    VtValue enableColorTemperatureVal = sceneDelegate->GetLightParamValue(id,
            HdLightTokens->enableColorTemperature);
    if (enableColorTemperatureVal.GetWithDefault<bool>(false)) {
        VtValue colorTemperatureVal = sceneDelegate->GetLightParamValue(id,
                HdLightTokens->colorTemperature);
        if (colorTemperatureVal.IsHolding<float>()) {
            float colorTemperature = colorTemperatureVal.Get<float>();
            hdc = GfCompMult(hdc,
                _BlackbodyTemperatureAsRgb(colorTemperature));
        }
    }

    // Intensity
    float intensity = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->intensity)
            .Get<float>();

    // Exposure
    float exposure = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->exposure)
            .Get<float>();
    intensity *= powf(2.0f, GfClamp(exposure, -50.0f, 50.0f));

    // Dimensions
    // (If we are normalizing for area, there's nothing to do here, since
    // we are already approximating the light as an area-less point source)
    VtValue normalizeVal = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->normalize);
    if (!normalizeVal.GetWithDefault<bool>(false)) {
        // Compute area of the maximum possible facing profile.
        float area = 1.0f;
        if (_lightType == HdPrimTypeTokens->diskLight ||
            _lightType == HdPrimTypeTokens->sphereLight) {
            VtValue radiusVal = 
                sceneDelegate->GetLightParamValue(id, HdLightTokens->radius);
            if (radiusVal.IsHolding<float>()) {
                float radius = radiusVal.Get<float>();
                area = radius * radius * M_PI;
            }
        } else if (_lightType == HdPrimTypeTokens->rectLight) {
            VtValue widthVal = 
                sceneDelegate->GetLightParamValue(id, HdLightTokens->width);
            if (widthVal.IsHolding<float>()) {
                area *= widthVal.Get<float>();
            }
            VtValue heightVal = 
                sceneDelegate->GetLightParamValue(id, HdLightTokens->height);
            if (heightVal.IsHolding<float>()) {
                area *= heightVal.Get<float>();
            }
        } else if (_lightType == HdPrimTypeTokens->cylinderLight) {
            VtValue lengthVal = 
                sceneDelegate->GetLightParamValue(id, HdLightTokens->length);
            if (lengthVal.IsHolding<float>()) {
                area *= lengthVal.Get<float>();
            }
            VtValue radiusVal = 
                sceneDelegate->GetLightParamValue(id, HdLightTokens->radius);
            if (radiusVal.IsHolding<float>()) {
                float radius = radiusVal.Get<float>();
                area *= radius;
            }
        } else if (_lightType == HdPrimTypeTokens->distantLight) {
            VtValue angleDegVal = 
                sceneDelegate->GetLightParamValue(id, HdLightTokens->angle);
            if (angleDegVal.IsHolding<float>()) {
                // Convert from cone apex angle to solid angle
                float angleRadians = angleDegVal.Get<float>() / 180.0 * M_PI;
                float solidAngleSteradians = 2*M_PI*(1.0-cos(angleRadians/2.0));
                area = solidAngleSteradians;
            }
        }
        intensity *= area;
    }

    // Calculate the final color of the light
    GfVec4f c(hdc[0]*intensity, hdc[1]*intensity, hdc[2]*intensity, 1.0f); 

    // Ambient, Diffuse & Specular multipliers
    float ambientMultiplier = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->ambient)
            .GetWithDefault<float>(0.0f);
    float diffuseMultiplier = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->diffuse)
            .GetWithDefault<float>(1.0f);
    float specularMultiplier = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->specular)
            .GetWithDefault<float>(1.0f);

    // Directional emission shaping
    float shapingConeAngle =
        sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingConeAngle)
            .GetWithDefault<float>(90.0f);
    float shapingFocus =
        sceneDelegate->GetLightParamValue(id, HdLightTokens->shapingFocus)
            .GetWithDefault<float>(0.0f);

    // Create the Glf Simple Light object that will be used by the rest
    // of the pipeline.
    GlfSimpleLight l;
    l.SetHasIntensity(intensity != 0.0f);
    l.SetAmbient(ambientMultiplier * c);
    l.SetDiffuse(diffuseMultiplier * c);
    l.SetSpecular(specularMultiplier * c);
    l.SetHasShadow(
        sceneDelegate->GetLightParamValue(id, HdLightTokens->hasShadow)
            .GetWithDefault<bool>(false));

    if (_lightType == HdPrimTypeTokens->rectLight ||
        _lightType == HdPrimTypeTokens->diskLight) {
        l.SetSpotCutoff(shapingConeAngle);
        l.SetSpotFalloff(GfMax(0.0f, shapingFocus));
    }

    // See glf/shaders/simpleLighting.glslfx for attenuation math
    if (_lightType == HdPrimTypeTokens->distantLight) {
        l.SetAttenuation(GfVec3f(0.0f, 0.0f, 0.0f)); // none
    } else {
        l.SetAttenuation(GfVec3f(0.0f, 0.0f, 1.0f)); // distance^-2
    }

    return l;
}

GlfSimpleLight
HdStLight::_PrepareDomeLight(
    SdfPath const &id, 
    HdSceneDelegate * const sceneDelegate)
{
    // Create the Glf Simple Light object that will be used by the rest
    // of the pipeline. No support for shadows for dome light.
    GlfSimpleLight l;
    l.SetHasShadow(false);
    l.SetIsDomeLight(true);
    l.SetAttenuation(GfVec3f(0.0f, 0.0f, 0.0f));

    // The intensity value is set to 0 if light is not visible
    if (!sceneDelegate->GetVisible(id) ||
        (sceneDelegate->GetLightParamValue(id, HdLightTokens->intensity)
            .Get<float>() == 0.0)) {
        l.SetHasIntensity(false);
        return l;
    }
    
    // Get the color of the light
    GfVec3f hdc = sceneDelegate->GetLightParamValue(id, HdLightTokens->color)
        .Get<GfVec3f>();

    // Color temperature
    VtValue enableColorTemperatureVal = 
        sceneDelegate->GetLightParamValue(id,
            HdLightTokens->enableColorTemperature);
    if (enableColorTemperatureVal.GetWithDefault<bool>(false)) {
        VtValue colorTemperatureVal = 
            sceneDelegate->GetLightParamValue(id,
                HdLightTokens->colorTemperature);
        if (colorTemperatureVal.IsHolding<float>()) {
            float colorTemperature = colorTemperatureVal.Get<float>();
            hdc = GfCompMult(hdc,
                _BlackbodyTemperatureAsRgb(colorTemperature));
        }
    }

    // Intensity
    float intensity = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->intensity)
            .Get<float>();

    // Exposure
    float exposure = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->exposure)
            .Get<float>();
    intensity *= powf(2.0f, GfClamp(exposure, -50.0f, 50.0f));

    // Calculate the final color of the light
    GfVec4f c(hdc[0]*intensity, hdc[1]*intensity, hdc[2]*intensity, 1.0f); 

    // Diffuse & Specular multiplier
    float diffuseMultiplier = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->diffuse)
            .GetWithDefault<float>(1.0f);
    float specularMultiplier = 
        sceneDelegate->GetLightParamValue(id, HdLightTokens->specular)
            .GetWithDefault<float>(1.0f);

    l.SetHasIntensity(intensity != 0.0f);
    l.SetDiffuse(diffuseMultiplier * c);
    l.SetSpecular(specularMultiplier * c);

    // Dome light texture
    {
        const VtValue v = sceneDelegate->GetLightParamValue(
                id, HdLightTokens->textureFile);
        if (!v.IsEmpty()) {
            if (v.IsHolding<SdfAssetPath>()) {
                l.SetDomeLightTextureFile(v.UncheckedGet<SdfAssetPath>());
            } else {
                TF_CODING_ERROR("Dome light texture file not an asset path.");
            }
        }
    }
    return l;
}

/* virtual */
void
HdStLight::Sync(HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    SdfPath const &id = GetId();

    // HdStLight communicates to the scene graph and caches all interesting
    // values within this class. Later on Get() is called from 
    // TaskState (RenderPass) to perform aggregation/pre-computation, 
    // in order to make the shader execution efficient.

    // Change tracking
    HdDirtyBits bits = *dirtyBits;

    // Transform
    if (bits & DirtyTransform) {
        _params[HdTokens->transform] = sceneDelegate->GetTransform(id);
    }

    // Lighting Params
    if (bits & DirtyParams) {
        HdChangeTracker& changeTracker =
            sceneDelegate->GetRenderIndex().GetChangeTracker();

        // Remove old dependencies
        VtValue val = Get(HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            SdfPathVector lightFilterPaths = val.UncheckedGet<SdfPathVector>();
            for (SdfPath const & filterPath : lightFilterPaths) {
                changeTracker.RemoveSprimSprimDependency(filterPath, id);
            }
        }

        if (_lightType == HdPrimTypeTokens->simpleLight) {
            _params[HdLightTokens->params] =
                sceneDelegate->Get(id, HdLightTokens->params);
        }
        else if (_lightType == HdPrimTypeTokens->domeLight) {
            _params[HdLightTokens->params] = 
                _PrepareDomeLight(id, sceneDelegate);
        }
        // If it is an area light we will extract the parameters and convert
        // them to a GlfSimpleLight that approximates the light source.
        else {
            _params[HdLightTokens->params] =
                _ApproximateAreaLight(id, sceneDelegate);
        }

        // Add new dependencies
        val = Get(HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            SdfPathVector lightFilterPaths = val.UncheckedGet<SdfPathVector>();
            for (SdfPath const & filterPath : lightFilterPaths) {
                changeTracker.AddSprimSprimDependency(filterPath, id);
            }
        }
    }

    if (bits & (DirtyTransform | DirtyParams)) {
        GfMatrix4d transform =
            Get(HdTokens->transform).GetWithDefault<GfMatrix4d>();
        // Update cached light objects.  Note that simpleLight ignores
        // scene-delegate transform, in favor of the transform passed in by
        // params...
        if (_lightType == HdPrimTypeTokens->domeLight) {
            GlfSimpleLight light =
                Get(HdLightTokens->params).GetWithDefault<GlfSimpleLight>();
            light.SetTransform(transform);
            _params[HdLightTokens->params] = light;
        } else if (_lightType != HdPrimTypeTokens->simpleLight) {
            // e.g. area light
            GlfSimpleLight light =
                Get(HdLightTokens->params).GetWithDefault<GlfSimpleLight>();
            GfVec3d p = transform.ExtractTranslation();
            GfVec4f pos(p[0], p[1], p[2], 1.0f);
            // Convention is to emit light along -Z
            GfVec4d zDir = transform.GetRow(2);
            if (_lightType == HdPrimTypeTokens->rectLight ||
                _lightType == HdPrimTypeTokens->diskLight) {
                light.SetSpotDirection(GfVec3f(-zDir[0], -zDir[1], -zDir[2]));
            } else if (_lightType == HdPrimTypeTokens->distantLight) {
                // For a distant light, translate to +Z homogeneous limit
                // See simpleLighting.glslfx : integrateLightsDefault.
                pos = GfVec4f(zDir[0], zDir[1], zDir[2], 0.0f);
            }
            light.SetPosition(pos);
            _params[HdLightTokens->params] = light;
        }
    }

    // Shadow Params
    if (bits & DirtyShadowParams) {
        _params[HdLightTokens->shadowParams] =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->shadowParams);
    }

    // Shadow Collection
    if (bits & DirtyCollection) {
        VtValue vtShadowCollection =
            sceneDelegate->GetLightParamValue(id, HdLightTokens->shadowCollection);

        // Optional
        if (vtShadowCollection.IsHolding<HdRprimCollection>()) {
            HdRprimCollection newCollection =
                vtShadowCollection.UncheckedGet<HdRprimCollection>();

            if (_params[HdLightTokens->shadowCollection] != newCollection) {
                _params[HdLightTokens->shadowCollection] = newCollection;

                HdChangeTracker& changeTracker =
                             sceneDelegate->GetRenderIndex().GetChangeTracker();

                changeTracker.MarkCollectionDirty(newCollection.GetName());
            }

        } else {
            _params[HdLightTokens->shadowCollection] = HdRprimCollection();
        }
    }

    *dirtyBits = Clean;
}

/* virtual */
void
HdStLight::Finalize(HdRenderParam *renderParam)
{
    // HdStLight doesn't own resources directly, so we don't need to
    // flag garbage collection.
}

VtValue
HdStLight::Get(TfToken const &token) const
{
    VtValue val;
    TfMapLookup(_params, token, &val);
    return val;
}

/* virtual */
HdDirtyBits
HdStLight::GetInitialDirtyBitsMask() const
{
    // In the case of simple and distant lights we want to sync all dirty bits,
    // but for area lights coming from the scenegraph we just want to extract
    // the Transform and Params for now.
    if (_lightType == HdPrimTypeTokens->simpleLight || 
        _lightType == HdPrimTypeTokens->distantLight) {
        return AllDirty;
    } else {
        return (DirtyParams | DirtyTransform);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
