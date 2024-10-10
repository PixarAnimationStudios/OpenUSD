//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/camera.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/cameraContext.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/version.h"

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

#if HD_API_VERSION < 52
TF_DEFINE_PRIVATE_TOKENS(
    _lensDistortionTokens,
    ((k1,     "lensDistortion:k1"))
    ((k2,     "lensDistortion:k2"))
    ((center, "lensDistortion:center"))
    ((anaSq,  "lensDistortion:anaSq"))
    ((asym,   "lensDistortion:asym"))
    ((scale,  "lensDistortion:scale"))
);
#endif

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((shutterOpenTime,    "ri:shutterOpenTime"))
    ((shutterCloseTime,   "ri:shutterCloseTime"))
    ((shutteropening,     "ri:shutteropening"))
    ((apertureAngle,      "ri:apertureAngle"))
    ((apertureDensity,    "ri:apertureDensity"))
    ((apertureNSides,     "ri:apertureNSides"))
    ((apertureRoundness,  "ri:apertureRoundness"))
    ((projection_dofMult, "ri:projection:dofMult"))
);

TF_DEFINE_PRIVATE_TOKENS(
    _tokensLegacy,
    ((orthowidth,                     "ri:camera:orthowidth"))
    ((window,                         "ri:camera:window"))
    ((dofAspect,                      "ri:camera:dofaspect"))
    ((extremeOffset,                  "ri:camera:extremeoffset"))
    ((apertureNSides,                 "ri:camera:aperturensides"))
    ((apertureAngle,                  "ri:camera:apertureangle"))
    ((apertureRoundness,              "ri:camera:apertureroundness"))
    ((apertureDensity,                "ri:camera:aperturedensity"))
    ((shutteropening1,                "ri:camera:shutteropening1"))
    ((shutteropening2,                "ri:camera:shutteropening2"))
    ((shutterOpenTime,                "ri:camera:shutterOpenTime"))
    ((shutterCloseTime,               "ri:camera:shutterCloseTime"))
);

template <class T>
static const T*
_GetDictItem(const VtDictionary& dict, const TfToken& key)
{
    const VtValue* v = TfMapLookupPtr(dict, key.GetString());
    return v && v->IsHolding<T>() ? &v->UncheckedGet<T>() : nullptr;
}

static
std::optional<std::array<float, 8>>
_ToOptionalFloat8(const VtValue &value)
{
    if (!value.IsHolding<VtArray<float>>()) {
        return std::nullopt;
    }
    const VtArray<float> array = value.UncheckedGet<VtArray<float>>();
    if (array.size() != 8) {
        return std::nullopt;
    }
    std::array<float, 8> result;
    for (size_t i = 0; i < 8; i++) {
        result[i] = array[i];
    }
    return result;
}

HdPrmanCamera::HdPrmanCamera(SdfPath const& id)
  : HdCamera(id)
#if HD_API_VERSION < 52
  , _lensDistortionK1(0.0f)
  , _lensDistortionK2(0.0f)
  , _lensDistortionCenter(0.0f)
  , _lensDistortionAnaSq(1.0f)
  , _lensDistortionAsym(0.0f)
  , _lensDistortionScale(1.0f)
#endif
  , _apertureAngle(0.0f)
  , _apertureDensity(0.0f)
  , _apertureNSides(0)
  , _apertureRoundness(1.0f)
  , _dofMult(1.0f)
{
}

HdPrmanCamera::~HdPrmanCamera() = default;

/* virtual */
void
HdPrmanCamera::Sync(HdSceneDelegate *sceneDelegate,
                    HdRenderParam   *renderParam,
                    HdDirtyBits     *dirtyBits)
{  
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!TF_VERIFY(sceneDelegate)) {
        return;
    }

    HdPrman_RenderParam * const param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    SdfPath const &id = GetId();
    // Save state of dirtyBits before HdCamera::Sync clears them.
    const HdDirtyBits bits = *dirtyBits;

    if (bits & AllDirty) {
        param->GetCameraContext().MarkCameraInvalid(id);
    }

    // These are legacy tokens for solaris that aren't updated
    // by HdCamera::Sync
    if (bits & DirtyParams) {
        TfToken params[] = {
            _tokensLegacy->orthowidth,
            _tokensLegacy->window,
            _tokensLegacy->dofAspect,
            _tokensLegacy->extremeOffset,
            _tokensLegacy->apertureNSides,
            _tokensLegacy->apertureAngle,
            _tokensLegacy->apertureRoundness,
            _tokensLegacy->apertureDensity,
            _tokensLegacy->shutteropening1,
            _tokensLegacy->shutteropening2,
            _tokensLegacy->shutterOpenTime,
            _tokensLegacy->shutterCloseTime,
        };

        for (TfToken const& param : params) {
            VtValue val = sceneDelegate->GetCameraParamValue(id, param);
            if (!val.IsEmpty()) {
                _params[param] = val;
            }
        }
    }

    HdCamera::Sync(sceneDelegate, renderParam, dirtyBits);

    if (bits & DirtyParams) {
#if HD_API_VERSION < 52
        _lensDistortionK1 =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->k1)
                .GetWithDefault<float>(0.0f);
        _lensDistortionK2 =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->k2)
                .GetWithDefault<float>(0.0f);
        _lensDistortionCenter =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->center)
                .GetWithDefault<GfVec2f>(GfVec2f(0.0f));
        _lensDistortionAnaSq =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->anaSq)
                .GetWithDefault<float>(1.0f);
        _lensDistortionAsym =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->asym)
                .GetWithDefault<GfVec2f>(GfVec2f(0.0f));
        _lensDistortionScale =
            sceneDelegate
                ->GetCameraParamValue(id, _lensDistortionTokens->scale)
                .GetWithDefault<float>(1.0f);
#endif

        const VtValue vShutterOpenTime =
            sceneDelegate->GetCameraParamValue(id, _tokens->shutterOpenTime);
        if (vShutterOpenTime.IsHolding<float>()) {
            _shutterCurve.shutterOpenTime =
                vShutterOpenTime.UncheckedGet<float>();
        } else {
            _shutterCurve.shutterOpenTime = std::nullopt;
        }
        const VtValue vShutterCloseTime =
            sceneDelegate->GetCameraParamValue(id, _tokens->shutterCloseTime);
        if (vShutterCloseTime.IsHolding<float>()) {
            _shutterCurve.shutterCloseTime =
                vShutterCloseTime.UncheckedGet<float>();
        } else {
            _shutterCurve.shutterCloseTime = std::nullopt;
        }
        const VtValue vShutteropening =
            sceneDelegate->GetCameraParamValue(id, _tokens->shutteropening);
        _shutterCurve.shutteropening = _ToOptionalFloat8(vShutteropening);

        _apertureAngle =
            sceneDelegate->GetCameraParamValue(id, _tokens->apertureAngle)
                         .GetWithDefault<float>(0.0f);
        _apertureDensity =
            sceneDelegate->GetCameraParamValue(id, _tokens->apertureDensity)
                         .GetWithDefault<float>(0.0f);
        _apertureNSides =
            sceneDelegate->GetCameraParamValue(id, _tokens->apertureNSides)
                         .GetWithDefault<int>(0);
        _apertureRoundness =
            sceneDelegate->GetCameraParamValue(id, _tokens->apertureRoundness)
                         .GetWithDefault<float>(1.0f);
        _dofMult =
            sceneDelegate->GetCameraParamValue(id, _tokens->projection_dofMult)
                         .GetWithDefault<float>(1.0f);
        if (id == param->GetCameraContext().GetCameraPath()) {
            // Motion blur in Riley only works correctly if the
            // shutter interval is set before any rprims are synced
            // (and the transform of the riley camera is updated).
            //
            // See SetRileyShutterIntervalFromCameraContextCameraPath
            // for additional context.
            //
            param->SetRileyShutterIntervalFromCameraContextCameraPath(
                &sceneDelegate->GetRenderIndex());
        }
    }

    if (bits & DirtyTransform) {
        // Do SampleTranform last.
        //
        // This is because it needs the shutter interval which is computed above.
        //
        sceneDelegate->SampleTransform(
            id,
#if HD_API_VERSION >= 68
            param->GetShutterInterval()[0],
            param->GetShutterInterval()[1],
#endif
            &_sampleXforms);
    }

    // XXX: Should we flip the proj matrix (RHS vs LHS) as well here?

    // We don't need to clear the dirty bits since HdCamera::Sync always clears
    // all the dirty bits.
}

void HdPrmanCamera::setFov(RtParamList& projParams) const
{
    const float horizontalAperture = GetHorizontalAperture();
    const float verticalAperture = GetVerticalAperture();
    const float focalLength = GetFocalLength();

    float filmAspect = horizontalAperture / verticalAperture;
    float aperture = (filmAspect < 1) ? horizontalAperture : verticalAperture;

    float focal = focalLength;
    float fov_rad = 2.f * atan((0.5 * aperture) / focal);

    float fov_deg = fov_rad / M_PI * 180.0;
    projParams.SetFloat(RixStr.k_fov, fov_deg);
}

void HdPrmanCamera::setScreenWindow(RtParamList& camParams, bool isPerspective) const
{
    const float horizontalAperture = GetHorizontalAperture();
    const float horizontalApertureOffset = GetHorizontalApertureOffset();
    const float verticalAperture = GetVerticalAperture();
    const float verticalApertureOffset = GetVerticalApertureOffset();

    float const *orthowidth =
        _GetDictItem<float>(_params, _tokensLegacy->orthowidth);

    GfVec4f const *window =
        _GetDictItem<GfVec4f>(_params, _tokensLegacy->window);

    float screenWindow[4] = {0.f, 0.f, 0.f, 0.f};

    float filmAspect = horizontalAperture / verticalAperture;
    if (window) // user defined
    {
        float const* win = window->GetArray();
        screenWindow[0] = win[0];
        screenWindow[1] = win[1];
        screenWindow[2] = win[2];
        screenWindow[3] = win[3];
    }
    else if (!isPerspective) {
        float wOver2, vOver2;
        float owidth = (orthowidth) ? *orthowidth : 2.f;
        if (filmAspect < 1)
        {
            wOver2 = 0.5f * owidth;
            vOver2 = wOver2 / filmAspect;
        }
        else
        {
            vOver2 = 0.5f * owidth / filmAspect;
            wOver2 = vOver2 * filmAspect;
        }
        screenWindow[0] = -wOver2;
        screenWindow[1] = wOver2;
        screenWindow[2] = -vOver2;
        screenWindow[3] = vOver2;
    }
    else if (filmAspect < 1) {
        screenWindow[0] = -1.f;
        screenWindow[1] = 1.f;
        screenWindow[2] = -1.f/filmAspect;
        screenWindow[3] = 1.f/filmAspect;
    }
    else {
        screenWindow[0] = -filmAspect;
        screenWindow[1] = filmAspect;
        screenWindow[2] = -1.f;
        screenWindow[3] = 1.f;
    }

    // aperture offset has same units as aperture
    float hOffsetScale = (screenWindow[1] - screenWindow[0]) / horizontalAperture;
    screenWindow[0] += horizontalApertureOffset * hOffsetScale;
    screenWindow[1] += horizontalApertureOffset * hOffsetScale;

    // aperture offset has same units as aperture
    float vOffsetScale = (screenWindow[3] - screenWindow[2]) / verticalAperture;
    screenWindow[2] += verticalApertureOffset * vOffsetScale;
    screenWindow[3] += verticalApertureOffset * vOffsetScale;

    camParams.SetFloatArray(RixStr.k_Ri_ScreenWindow, screenWindow, 4);
}

// Some of this method has moved to
// cameraContext.cpp SetCameraAndCameraNodeParams
// where newer camera APIs are used.
// Leaving this here to still be called for backward compatibility
// and some features not supported by the studio's hdprman.
void
HdPrmanCamera::SetRileyCameraParams(RtParamList& camParams,
                                    RtParamList& camParamsOverride,
                                    RtParamList& projParams) const
{
    float const *dofAspect =
        _GetDictItem<float>(_params, _tokensLegacy->dofAspect);
    if (dofAspect) {
        camParamsOverride.SetFloat(RixStr.k_dofaspect, *dofAspect);
    }

    float const *extremeOffset =
        _GetDictItem<float>(_params, _tokensLegacy->extremeOffset);
    if (extremeOffset) {
        camParamsOverride.SetFloat(RixStr.k_extrememoffset, *extremeOffset);
    }

    int const *apertureNSides =
        _GetDictItem<int>(_params, _tokensLegacy->apertureNSides);
    if (apertureNSides) {
        camParamsOverride.SetInteger(RixStr.k_apertureNSides, *apertureNSides);
    }

    float const *apertureAngle =
        _GetDictItem<float>(_params, _tokensLegacy->apertureAngle);
    if (apertureAngle) {
        camParamsOverride.SetFloat(RixStr.k_apertureAngle, *apertureAngle);
    }

    float const *apertureRoundness =
        _GetDictItem<float>(_params, _tokensLegacy->apertureRoundness);
    if (apertureRoundness) {
        camParamsOverride.SetFloat(RixStr.k_apertureRoundness, *apertureRoundness);
    }

    float const *apertureDensity =
        _GetDictItem<float>(_params, _tokensLegacy->apertureDensity);
    if (apertureDensity) {
        camParamsOverride.SetFloat(RixStr.k_apertureDensity, *apertureDensity);
    }

    float const *shutterOpenTime =
        _GetDictItem<float>(_params, _tokensLegacy->shutterOpenTime);
    if (shutterOpenTime) {
        camParamsOverride.SetFloat(RixStr.k_shutterOpenTime, *shutterOpenTime);
    }

    float const *shutterCloseTime =
        _GetDictItem<float>(_params, _tokensLegacy->shutterCloseTime);
    if (shutterCloseTime) {
        camParamsOverride.SetFloat(RixStr.k_shutterCloseTime, *shutterCloseTime);
    }

    GfVec4f const *shutteropening1 =
        _GetDictItem<GfVec4f>(_params, _tokensLegacy->shutteropening1);
    GfVec4f const *shutteropening2 =
        _GetDictItem<GfVec4f>(_params, _tokensLegacy->shutteropening2);
    if (shutteropening1 && shutteropening2) {
        float shutteropening[8];
        float const* so1 = shutteropening1->GetArray();
        float const* so2 = shutteropening2->GetArray();
        shutteropening[0] = so1[0];
        shutteropening[1] = so1[1];
        shutteropening[2] = so1[2];
        shutteropening[3] = so1[3];
        shutteropening[4] = so2[0];
        shutteropening[5] = so2[1];
        shutteropening[6] = so2[2];
        shutteropening[7] = so2[3];
        camParamsOverride.SetFloatArray(RixStr.k_shutteropening, shutteropening, 8);
    }

    // Following parameters are currently set on the Riley camera:
    // 'nearClip' (float): near clipping distance
    // 'farClip' (float): near clipping distance
    // 'shutterOpenTime' (float): beginning of normalized shutter interval
    // 'shutterCloseTime' (float): end of normalized shutter interval

    // Parameter that is handled during Riley camera creation:
    // Rix::k_shutteropening (float[8] [c1 c2 d1 d2 e1 e2 f1 f2): additional
    // control points
    // Do not use clipping range if scene delegate did not provide one.
    // Note that we do a sanity check slightly stronger than
    // GfRange1f::IsEmpty() in that we do not allow the range to contain
    // only exactly one point.

    GfMatrix4d const proj = ComputeProjectionMatrix();
    bool isPerspective = round(proj[3][3]) != 1 || proj == GfMatrix4d(1);
    if((TfMapLookupPtr(_params, _tokensLegacy->window) != nullptr)) {
        setScreenWindow(camParamsOverride, isPerspective);
    } else {
        setScreenWindow(camParams, isPerspective);
    }
    if (isPerspective)
    {
       setFov(projParams);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

