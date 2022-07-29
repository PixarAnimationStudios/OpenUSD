//
// Copyright 2019 Pixar
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

#include "hdPrman/renderParam.h"
#include "hdPrman/coordSys.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/material.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/camera.h"
#include "hdPrman/cameraContext.h"
#include "hdPrman/framebuffer.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/pathUtils.h"  // Extract extension from tf token
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/imaging/hio/imageRegistry.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderThread.h"

// XXX: Statistics depend on a header that is only included in v24.4+
#if (_PRMANAPI_VERSION_MAJOR_ > 24 || (_PRMANAPI_VERSION_MAJOR_ == 24 && _PRMANAPI_VERSION_MINOR_ >= 4))
#define _ENABLE_STATS
#endif

#ifdef _ENABLE_STATS
#include "stats/Session.h"
#endif

#include "Riley.h"
#include "RiTypesHelper.h"
#include "RixRiCtl.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (PrimvarPass)
    (sourceName)
    (sourceType)
    (primvar)
);


TF_DEFINE_ENV_SETTING(HD_PRMAN_ENABLE_MOTIONBLUR, true,
                      "Enable motion blur in HdPrman");
TF_DEFINE_ENV_SETTING(HD_PRMAN_NTHREADS, 0,
                      "Override number of threads used by HdPrman");
TF_DEFINE_ENV_SETTING(HD_PRMAN_OSL_VERBOSE, 0,
                      "Override osl verbose in HdPrman");

extern TfEnvSetting<bool> HD_PRMAN_ENABLE_QUICKINTEGRATE;

static bool _enableQuickIntegrate =
    TfGetEnvSetting(HD_PRMAN_ENABLE_QUICKINTEGRATE);

TF_DEFINE_ENV_SETTING(HD_PRMAN_DISABLE_HIDER_JITTER, false,
                      "Disable hider jitter");

static bool _disableJitter =
    TfGetEnvSetting(HD_PRMAN_DISABLE_HIDER_JITTER);

TF_MAKE_STATIC_DATA(std::vector<HdPrman_RenderParam::IntegratorCameraCallback>,
                    _integratorCameraCallbacks)
{
    _integratorCameraCallbacks->clear();
}

HdPrman_RenderParam::HdPrman_RenderParam(const std::string &rileyVariant,
        const std::string &xpuVariant) :
    resolution(0),
    _rix(nullptr),
    _ri(nullptr),
    _mgr(nullptr),
    _statsSession(nullptr),
    _riley(nullptr),
    _sceneLightCount(0),
    _sampleFiltersId(riley::SampleFilterId::InvalidId()),
    _lastSettingsVersion(0)
{
    // Create the stats session
    _CreateStatsSession();

    // Setup to use the default GPU
    _xpuGpuConfig.push_back(0);

    TfRegistryManager::GetInstance().SubscribeTo<HdPrman_RenderParam>();
    _CreateRiley(rileyVariant, xpuVariant);
    
    // Register RenderMan display driver
    HdPrmanFramebuffer::Register(_rix);
}

HdPrman_RenderParam::~HdPrman_RenderParam()
{
    DeleteRenderThread();

    _DestroyRiley();

    _DestroyStatsSession();
}

void
HdPrman_RenderParam::IncrementLightLinkCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    ++_lightLinkRefs[name];
}

void 
HdPrman_RenderParam::DecrementLightLinkCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    if (--_lightLinkRefs[name] == 0) {
        _lightLinkRefs.erase(name);
    }
}

bool 
HdPrman_RenderParam::IsLightLinkUsed(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    return _lightLinkRefs.find(name) != _lightLinkRefs.end();
}

void
HdPrman_RenderParam::IncrementLightFilterCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    ++_lightFilterRefs[name];
}

void 
HdPrman_RenderParam::DecrementLightFilterCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    if (--_lightFilterRefs[name] == 0) {
        _lightFilterRefs.erase(name);
    }
}

bool 
HdPrman_RenderParam::IsLightFilterUsed(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    return _lightFilterRefs.find(name) != _lightFilterRefs.end();
}

static
size_t
_ConvertPointsPrimvar(HdSceneDelegate *sceneDelegate, SdfPath const &id,
                      RtPrimVarList& primvars, const size_t * npointsHint)
{
    HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> points;
    {
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedPoints;
        sceneDelegate->SamplePrimvar(id, HdTokens->points, &boxedPoints);
        if (!points.UnboxFrom(boxedPoints)) {
            TF_WARN("<%s> points did not have expected type vec3f[]",
                    id.GetText());
        }
    }

    size_t npoints = 0;
    if (npointsHint) {
        npoints = *npointsHint;
    } else {
        if (points.count > 0) {
            npoints = points.values[0].size();
        }
        primvars.SetDetail(
            1,        /* uniform */
            npoints,  /* vertex */
            npoints,  /* varying */
            npoints   /* faceVarying */);
    }
        
    primvars.SetTimes(points.count, &points.times[0]);
    for (size_t i=0; i < points.count; ++i) {
        if (points.values[i].size() == npoints) {
            primvars.SetPointDetail(
                RixStr.k_P, 
                (RtPoint3*) points.values[i].cdata(),
                RtDetailType::k_vertex, 
                i);
        } else {
            TF_WARN("<%s> primvar 'points' size (%zu) dod not match "
                    "expected (%zu)",
                    id.GetText(), 
                    points.values[i].size(),
                    npoints);
        }
    }

    return npoints;
}

void
HdPrman_ConvertPointsPrimvar(HdSceneDelegate *sceneDelegate, SdfPath const &id,
                             RtPrimVarList& primvars, const size_t npoints)

{
    _ConvertPointsPrimvar(sceneDelegate, id, primvars, &npoints);
}

size_t
HdPrman_ConvertPointsPrimvarForPoints(
    HdSceneDelegate *sceneDelegate, SdfPath const &id,
    RtPrimVarList& primvars)
{
    return _ConvertPointsPrimvar(sceneDelegate, id, primvars, nullptr);
}


inline static RtDetailType
_RixDetailForHdInterpolation(HdInterpolation interp)
{
    switch (interp) {
    case HdInterpolationInstance:
        // Instance-level primvars, aka attributes, must be constant.
        return RtDetailType::k_constant;
    case HdInterpolationConstant:
        return RtDetailType::k_constant;
    case HdInterpolationUniform:
        return RtDetailType::k_uniform;
    case HdInterpolationVertex:
        return RtDetailType::k_vertex;
    case HdInterpolationVarying:
        return RtDetailType::k_varying;
    case HdInterpolationFaceVarying:
        return RtDetailType::k_facevarying;
    default:
        TF_CODING_ERROR("Unknown HdInterpolation value");
        return RtDetailType::k_constant;
    }
}

enum _ParamType {
    _ParamTypePrimvar,
    _ParamTypeAttribute,
};

static bool
_SetParamValue(RtUString const& name,
               VtValue const& val,
               TfToken const& role,
               RtParamList& params)
{
    if (val.IsHolding<float>()) {
        float v = val.UncheckedGet<float>();
        params.SetFloat(name, v);
    } else if (val.IsHolding<double>()) {
        double v = val.UncheckedGet<double>();
        params.SetFloat(name, static_cast<float>(v));
    } else if (val.IsHolding<VtArray<float>>()) {
        const VtArray<float>& v = val.UncheckedGet<VtArray<float>>();
        params.SetFloatArray(name, v.cdata(), v.size());
    } else if (val.IsHolding<VtArray<double>>()) {
        const VtArray<double>& vd = val.UncheckedGet<VtArray<double>>();
        // Convert double->float
        VtArray<float> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = float(vd[i]);
        }
        params.SetFloatArray(name, v.cdata(), v.size());
    } else if (val.IsHolding<int>()) {
        int v = val.UncheckedGet<int>();
        params.SetInteger(name, v);
    } else if (val.IsHolding<VtArray<int>>()) {
        const VtArray<int>& v = val.UncheckedGet<VtArray<int>>();
        params.SetIntegerArray(name, v.cdata(), v.size());
    } else if (val.IsHolding<long>()) {
        long v = val.UncheckedGet<long>();
        params.SetInteger(name, (int)v);        
    } else if (val.IsHolding<GfVec2f>()) {
        GfVec2f v = val.UncheckedGet<GfVec2f>();
        params.SetFloatArray(
            name, reinterpret_cast<const float*>(&v), 2);
    } else if (val.IsHolding<VtArray<GfVec2f>>()) {
        const VtArray<GfVec2f>& v = val.UncheckedGet<VtArray<GfVec2f>>();
        params.SetFloatArray(name,
            reinterpret_cast<const float*>(v.cdata()), 2 * v.size());
    } else if (val.IsHolding<GfVec2d>()) {
        GfVec2d vd = val.UncheckedGet<GfVec2d>();
        float v[2] = {float(vd[0]), float(vd[1])};
        params.SetFloatArray(name, v, 2);
    } else if (val.IsHolding<VtArray<GfVec2d>>()) {
        const VtArray<GfVec2d>& vd = val.UncheckedGet<VtArray<GfVec2d>>();
        // Convert double->float
        VtArray<GfVec2f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec2f(vd[i]);
        }
        params.SetFloatArray(name,
            reinterpret_cast<const float*>(v.cdata()), 2 * v.size());
    } else if (val.IsHolding<GfVec3f>()) {
        GfVec3f v = val.UncheckedGet<GfVec3f>();
        if (role == HdPrimvarRoleTokens->color) {
            params.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->point) {
            params.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->normal) {
            params.SetNormal(name, RtNormal3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->vector) {
            params.SetVector(name, RtVector3(v[0], v[1], v[2]));
        } else {
            params.SetFloatArray(name,
                reinterpret_cast<const float*>(&v), 3);
        }
    } else if (val.IsHolding<VtArray<GfVec3f>>()) {
        const VtArray<GfVec3f>& v = val.UncheckedGet<VtArray<GfVec3f>>();
        if (role == HdPrimvarRoleTokens->color) {
            params.SetColorArray(
                name, reinterpret_cast<const RtColorRGB*>(v.cdata()), v.size());
        } else if (role == HdPrimvarRoleTokens->point) {
            params.SetPointArray(
                name, reinterpret_cast<const RtPoint3*>(v.cdata()), v.size());
        } else if (role == HdPrimvarRoleTokens->normal) {
            params.SetNormalArray(
                name, reinterpret_cast<const RtNormal3*>(v.cdata()), v.size());
        } else if (role == HdPrimvarRoleTokens->vector) {
            params.SetVectorArray(
                name, reinterpret_cast<const RtVector3*>(v.cdata()), v.size());
        } else {
            params.SetFloatArray(
                name, reinterpret_cast<const float*>(v.cdata()), 3 * v.size());
        }
    } else if (val.IsHolding<GfVec3d>()) {
        // double->float
        GfVec3f v(val.UncheckedGet<GfVec3d>());
        if (role == HdPrimvarRoleTokens->color) {
            params.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->point) {
            params.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->normal) {
            params.SetNormal(name, RtNormal3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->vector) {
            params.SetVector(name, RtVector3(v[0], v[1], v[2]));
        } else {
            params.SetFloatArray(name,
                reinterpret_cast<const float*>(&v), 3);
        }
    } else if (val.IsHolding<VtArray<GfVec3d>>()) {
        const VtArray<GfVec3d>& vd = val.UncheckedGet<VtArray<GfVec3d>>();
        // double->float
        VtArray<GfVec3f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec3f(vd[i]);
        }
        if (role == HdPrimvarRoleTokens->color) {
            params.SetColorArray(
                name, reinterpret_cast<const RtColorRGB*>(v.cdata()), v.size());
        } else if (role == HdPrimvarRoleTokens->point) {
            params.SetPointArray(
                name, reinterpret_cast<const RtPoint3*>(v.cdata()), v.size());
        } else if (role == HdPrimvarRoleTokens->normal) {
            params.SetNormalArray(
                name, reinterpret_cast<const RtNormal3*>(v.cdata()), v.size());
        } else if (role == HdPrimvarRoleTokens->vector) {
            params.SetVectorArray(
                name, reinterpret_cast<const RtVector3*>(v.cdata()), v.size());
        } else {
            params.SetFloatArray(
                name, reinterpret_cast<const float*>(v.cdata()), 3 * v.size());
        }
    } else if (val.IsHolding<GfVec4f>()) {
        GfVec4f v = val.UncheckedGet<GfVec4f>();
        params.SetFloatArray(
            name, reinterpret_cast<const float*>(&v), 4);
    } else if (val.IsHolding<VtArray<GfVec4f>>()) {
        const VtArray<GfVec4f>& v = val.UncheckedGet<VtArray<GfVec4f>>();
        params.SetFloatArray(
            name, reinterpret_cast<const float*>(v.cdata()), 4 * v.size());
    } else if (val.IsHolding<GfVec4d>()) {
        // double->float
        GfVec4f v(val.UncheckedGet<GfVec4d>());
        params.SetFloatArray(
            name, reinterpret_cast<const float*>(&v), 4);
    } else if (val.IsHolding<VtArray<GfVec4d>>()) {
        const VtArray<GfVec4d>& vd = val.UncheckedGet<VtArray<GfVec4d>>();
        // double->float
        VtArray<GfVec4f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec4f(vd[i]);
        }
        params.SetFloatArray(
            name, reinterpret_cast<const float*>(v.cdata()), 4 * v.size());
    } else if (val.IsHolding<GfMatrix4d>()) {
        GfMatrix4d v = val.UncheckedGet<GfMatrix4d>();
        params.SetMatrix(name, HdPrman_GfMatrixToRtMatrix(v));
    } else if (val.IsHolding<int>()) {
        int v = val.UncheckedGet<int>();
        params.SetInteger(name, v);
    } else if (val.IsHolding<VtArray<int>>()) {
        const VtArray<int>& v = val.UncheckedGet<VtArray<int>>();
        params.SetIntegerArray(
            name, reinterpret_cast<const int*>(v.cdata()), v.size());
    } else if (val.IsHolding<bool>()) {
        // bool->integer
        int v = val.UncheckedGet<bool>();
        params.SetInteger(name, v);
    } else if (val.IsHolding<VtArray<bool>>()) {
        const VtArray<bool>& vb = val.UncheckedGet<VtArray<bool>>();
        // bool->integer
        VtArray<int> v;
        v.resize(vb.size());
        for (size_t i=0,n=vb.size(); i<n; ++i) {
            v[i] = int(vb[i]);
        }
        params.SetIntegerArray(
            name, reinterpret_cast<const int*>(v.cdata()), v.size());
    } else if (val.IsHolding<TfToken>()) {
        TfToken v = val.UncheckedGet<TfToken>();
        params.SetString(name, RtUString(v.GetText()));
    } else if (val.IsHolding<std::string>()) {
        std::string v = val.UncheckedGet<std::string>();
        params.SetString(name, RtUString(v.c_str()));
    } else if (val.IsHolding<VtArray<std::string>>()) {
        // Convert to RtUString.
        const VtArray<std::string>& v =
            val.UncheckedGet<VtArray<std::string>>();
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (std::string const& s: v) {
            us.push_back(RtUString(s.c_str()));
        }
        params.SetStringArray(name, us.data(), us.size());
    } else if (val.IsHolding<SdfAssetPath>()) {
        SdfAssetPath asset = val.UncheckedGet<SdfAssetPath>();
        // Since we can't know how the texture will be consumed,
        // go with the default of flipping textures
        const bool flipTexture = true;
        RtUString v = HdPrman_ResolveAssetToRtUString(asset, flipTexture,
                                                   _tokens->primvar.GetText());
        params.SetString(name, v);
    } else if (val.IsHolding<VtArray<SdfAssetPath>>()) {
        // Convert to RtUString.
        const VtArray<SdfAssetPath>& v =
            val.UncheckedGet<VtArray<SdfAssetPath>>();
        // Since we can't know how the texture will be consumed,
        // go with the default of flipping textures
        const bool flipTexture = true;
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (SdfAssetPath const& asset: v) {
            us.push_back(HdPrman_ResolveAssetToRtUString(asset, flipTexture,
                                                   _tokens->primvar.GetText()));
        }
        params.SetStringArray(name, us.data(), us.size());
    } else if (val.IsHolding<VtArray<TfToken>>()) {
        // Convert to RtUString.
        const VtArray<TfToken>& v =
            val.UncheckedGet<VtArray<TfToken>>();
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (TfToken const& s: v) {
            us.push_back(RtUString(s.GetText()));
        }
        params.SetStringArray(name, us.data(), us.size());
    } else {
        // Unhandled type
        return false;
    }

    return true;
}

static bool
_SetPrimVarValue(RtUString const& name,
               VtValue const& val,
               RtDetailType const& detail,
               TfToken const& role,
               RtPrimVarList& params)
{
    if (val.IsHolding<float>()) {
        float v = val.UncheckedGet<float>();
        params.SetFloat(name, v);
    } else if (val.IsHolding<double>()) {
        double v = val.UncheckedGet<double>();
        params.SetFloat(name, static_cast<float>(v));
    } else if (val.IsHolding<VtArray<float>>()) {
        const VtArray<float>& v = val.UncheckedGet<VtArray<float>>();
        if (detail == RtDetailType::k_constant) {
            params.SetFloatArray(name, v.cdata(), v.size());
        } else {
            params.SetFloatDetail(name, v.cdata(), detail);
        }
    } else if (val.IsHolding<VtArray<double>>()) {
        const VtArray<double>& vd = val.UncheckedGet<VtArray<double>>();
        // Convert double->float
        VtArray<float> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = float(vd[i]);
        }
        if (detail == RtDetailType::k_constant) {
            params.SetFloatArray(name, v.cdata(), v.size());
        } else {
            params.SetFloatDetail(name, v.cdata(), detail);
        }
    } else if (val.IsHolding<int>()) {
        int v = val.UncheckedGet<int>();
        params.SetInteger(name, v);
    } else if (val.IsHolding<VtArray<int>>()) {
        const VtArray<int>& v = val.UncheckedGet<VtArray<int>>();
        if (detail == RtDetailType::k_constant) {
            params.SetIntegerArray(name, v.cdata(), v.size());
        } else {
            params.SetIntegerDetail(name, v.cdata(), detail);
        }
    } else if (val.IsHolding<long>()) {
        long v = val.UncheckedGet<long>();
        params.SetInteger(name, (int)v);
    } else if (val.IsHolding<long long>()) {
        long long v = val.UncheckedGet<long long>();
        params.SetInteger(name, (int)v);
    } else if (val.IsHolding<GfVec2i>()) {
        GfVec2i v = val.UncheckedGet<GfVec2i>();
        params.SetIntegerArray(
            name, reinterpret_cast<const int*>(&v), 2);        
    } else if (val.IsHolding<GfVec2f>()) {
        GfVec2f v = val.UncheckedGet<GfVec2f>();
        params.SetFloatArray(
            name, reinterpret_cast<const float*>(&v), 2);
    } else if (val.IsHolding<VtArray<GfVec2f>>()) {
        const VtArray<GfVec2f>& v = val.UncheckedGet<VtArray<GfVec2f>>();
        params.SetFloatArrayDetail(name,
            reinterpret_cast<const float*>(v.cdata()), 2, detail);
    } else if (val.IsHolding<GfVec2d>()) {
        GfVec2d vd = val.UncheckedGet<GfVec2d>();
        float v[2] = {float(vd[0]), float(vd[1])};
        params.SetFloatArray(name, v, 2);
    } else if (val.IsHolding<VtArray<GfVec2d>>()) {
        const VtArray<GfVec2d>& vd = val.UncheckedGet<VtArray<GfVec2d>>();
        // Convert double->float
        VtArray<GfVec2f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec2f(vd[i]);
        }
        params.SetFloatArrayDetail(name,
            reinterpret_cast<const float*>(v.cdata()), 2, detail);
    } else if (val.IsHolding<GfVec3f>()) {
        GfVec3f v = val.UncheckedGet<GfVec3f>();
        if (role == HdPrimvarRoleTokens->color) {
            params.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->point) {
            params.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->normal) {
            params.SetNormal(name, RtNormal3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->vector) {
            params.SetVector(name, RtVector3(v[0], v[1], v[2]));
        } else {
            params.SetFloatArray(name,
                reinterpret_cast<const float*>(&v), 3);
        }
    } else if (val.IsHolding<VtArray<GfVec3f>>()) {
        const VtArray<GfVec3f>& v = val.UncheckedGet<VtArray<GfVec3f>>();
        if (role == HdPrimvarRoleTokens->color) {
            params.SetColorDetail(
                name, reinterpret_cast<const RtColorRGB*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->point) {
            params.SetPointDetail(
                name, reinterpret_cast<const RtPoint3*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->normal) {
            params.SetNormalDetail(
                name, reinterpret_cast<const RtNormal3*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->vector) {
            params.SetVectorDetail(
                name, reinterpret_cast<const RtVector3*>(v.cdata()),
                detail);
        } else {
            params.SetFloatArrayDetail(
                name, reinterpret_cast<const float*>(v.cdata()),
                3, detail);
        }
    } else if (val.IsHolding<GfVec3d>()) {
        // double->float
        GfVec3f v(val.UncheckedGet<GfVec3d>());
        if (role == HdPrimvarRoleTokens->color) {
            params.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->point) {
            params.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->normal) {
            params.SetNormal(name, RtNormal3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->vector) {
            params.SetVector(name, RtVector3(v[0], v[1], v[2]));
        } else {
            params.SetFloatArray(name,
                reinterpret_cast<const float*>(&v), 3);
        }
    } else if (val.IsHolding<VtArray<GfVec3d>>()) {
        const VtArray<GfVec3d>& vd = val.UncheckedGet<VtArray<GfVec3d>>();
        // double->float
        VtArray<GfVec3f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec3f(vd[i]);
        }
        if (role == HdPrimvarRoleTokens->color) {
            params.SetColorDetail(
                name, reinterpret_cast<const RtColorRGB*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->point) {
            params.SetPointDetail(
                name, reinterpret_cast<const RtPoint3*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->normal) {
            params.SetNormalDetail(
                name, reinterpret_cast<const RtNormal3*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->vector) {
            params.SetVectorDetail(
                name, reinterpret_cast<const RtVector3*>(v.cdata()),
                detail);
        } else {
            params.SetFloatArrayDetail(
                name, reinterpret_cast<const float*>(v.cdata()),
                3, detail);
        }
    } else if (val.IsHolding<GfVec4f>()) {
        GfVec4f v = val.UncheckedGet<GfVec4f>();
        params.SetFloatArray(
            name, reinterpret_cast<const float*>(&v), 4);
    } else if (val.IsHolding<VtArray<GfVec4f>>()) {
        const VtArray<GfVec4f>& v = val.UncheckedGet<VtArray<GfVec4f>>();
        params.SetFloatArrayDetail(
            name, reinterpret_cast<const float*>(v.cdata()), 4, detail);
    } else if (val.IsHolding<GfVec4d>()) {
        // double->float
        GfVec4f v(val.UncheckedGet<GfVec4d>());
        params.SetFloatArray(
            name, reinterpret_cast<const float*>(&v), 4);
    } else if (val.IsHolding<VtArray<GfVec4d>>()) {
        const VtArray<GfVec4d>& vd = val.UncheckedGet<VtArray<GfVec4d>>();
        // double->float
        VtArray<GfVec4f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec4f(vd[i]);
        }
        params.SetFloatArrayDetail(
            name, reinterpret_cast<const float*>(v.cdata()), 4, detail);
    } else if (val.IsHolding<GfMatrix4d>()) {
        GfMatrix4d v = val.UncheckedGet<GfMatrix4d>();
        params.SetMatrix(name, HdPrman_GfMatrixToRtMatrix(v));
    } else if (val.IsHolding<int>()) {
        int v = val.UncheckedGet<int>();
        params.SetInteger(name, v);
    } else if (val.IsHolding<VtArray<int>>()) {
        const VtArray<int>& v = val.UncheckedGet<VtArray<int>>();
        params.SetIntegerArrayDetail(
            name, reinterpret_cast<const int*>(v.cdata()), 1, detail);
    } else if (val.IsHolding<bool>()) {
        // bool->integer
        int v = val.UncheckedGet<bool>();
        params.SetInteger(name, v);
    } else if (val.IsHolding<VtArray<bool>>()) {
        const VtArray<bool>& vb = val.UncheckedGet<VtArray<bool>>();
        // bool->integer
        VtArray<int> v;
        v.resize(vb.size());
        for (size_t i=0,n=vb.size(); i<n; ++i) {
            v[i] = int(vb[i]);
        }
        params.SetIntegerArrayDetail(
            name, reinterpret_cast<const int*>(v.cdata()), 1, detail);
    } else if (val.IsHolding<TfToken>()) {
        TfToken v = val.UncheckedGet<TfToken>();
        params.SetString(name, RtUString(v.GetText()));
    } else if (val.IsHolding<std::string>()) {
        std::string v = val.UncheckedGet<std::string>();
        params.SetString(name, RtUString(v.c_str()));
    } else if (val.IsHolding<SdfAssetPath>()) {
        // Since we can't know how the primvar will be consumed,
        // go with the default of flipping textures
        const bool flipTexture = true;
        SdfAssetPath asset = val.UncheckedGet<SdfAssetPath>();
        RtUString v = HdPrman_ResolveAssetToRtUString(asset, flipTexture,
                                                   _tokens->primvar.GetText());
        params.SetString(name, v);
    } else if (val.IsHolding<VtArray<std::string>>()) {
        // Convert to RtUString.
        const VtArray<std::string>& v =
            val.UncheckedGet<VtArray<std::string>>();
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (std::string const& s: v) {
            us.push_back(RtUString(s.c_str()));
        }
        if (detail == RtDetailType::k_constant) {
            params.SetStringArray(name, us.data(), us.size());
        } else {
            params.SetStringDetail(name, us.data(), detail);
        }
    } else if (val.IsHolding<VtArray<TfToken>>()) {
        // Convert to RtUString.
        const VtArray<TfToken>& v =
            val.UncheckedGet<VtArray<TfToken>>();
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (TfToken const& s: v) {
            us.push_back(RtUString(s.GetText()));
        }
        if (detail == RtDetailType::k_constant) {
            params.SetStringArray(name, us.data(), us.size());
        } else {
            params.SetStringDetail(name, us.data(), detail);
        }
    } else if (val.IsHolding<VtArray<SdfAssetPath>>()) {
        // Convert to RtUString.
        // Since we can't know how the primvar will be consumed,
        // go with the default of flipping textures
        const bool flipTexture = true;
        const VtArray<SdfAssetPath>& v =
            val.UncheckedGet<VtArray<SdfAssetPath>>();
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (SdfAssetPath const& asset: v) {
            us.push_back(HdPrman_ResolveAssetToRtUString(asset, flipTexture,
                                                   _tokens->primvar.GetText()));
        }
        if (detail == RtDetailType::k_constant) {
            params.SetStringArray(name, us.data(), us.size());
        } else {
            params.SetStringDetail(name, us.data(), detail);
        }
    } else {
        // Unhandled type
        return false;
    }

    return true;
}

static RtUString
_GetPrmanPrimvarName(TfToken const& hdPrimvarName,
                     RtDetailType const& /*detail*/)
{
    // Handle cases where Hydra built-in primvars map to Renderman
    // built-in primvars.
    if (hdPrimvarName == HdTokens->points) {
        return RixStr.k_P;
    } else if (hdPrimvarName == HdTokens->normals) {
        // Hydra "normals" becomes Renderman "N"
        return RixStr.k_N;
    } else if (hdPrimvarName == HdTokens->widths) {
        return RixStr.k_width;
    }

    return RtUString(hdPrimvarName.GetText());
}

static HdExtComputationPrimvarDescriptorVector
_GetComputedPrimvars(HdSceneDelegate* sceneDelegate,
                     SdfPath const& id,
                     HdInterpolation interp,
                     HdDirtyBits dirtyBits)
{
    HdExtComputationPrimvarDescriptorVector dirtyCompPrimvars;

    // Get all the dirty computed primvars
    HdExtComputationPrimvarDescriptorVector compPrimvars;
    compPrimvars = sceneDelegate->GetExtComputationPrimvarDescriptors
                                    (id,interp);
    for (auto const& pv: compPrimvars) {
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
            dirtyCompPrimvars.emplace_back(pv);
        }
    }

    return dirtyCompPrimvars;
}

static bool
_IsPrototypeAttribute(TfToken const& primvarName)
{
    // This is a list of names for uniform primvars/attributes that
    // affect the prototype geometry in Renderman. They need to be
    // emitted on the prototype as primvars to take effect, instead of
    // on geometry instances.
    //
    // This list was created based on this doc page:
    //   https://rmanwiki.pixar.com/display/REN23/Primitive+Variables
    typedef std::unordered_set<TfToken, TfToken::HashFunctor> TfTokenSet;
    static const TfTokenSet prototypeAttributes = {
        // Common
        TfToken("ri:attributes:identifier:object"),
        // Shading
        TfToken("ri:attributes:derivatives:extrapolate"),
        TfToken("ri:attributes:displacement:ignorereferenceinstance"),
        TfToken("ri:attributes:displacementbound:CoordinateSystem"),
        TfToken("ri:attributes:displacementbound:offscreen"),
        TfToken("ri:attributes:displacementbound:sphere"),
        TfToken("ri:attributes:Ri:Orientation"),
        TfToken("ri:attributes:trace:autobias"),
        TfToken("ri:attributes:trace:bias"),
        TfToken("ri:attributes:trace:sssautobias"),
        TfToken("ri:attributes:trace:sssbias"),
        TfToken("ri:attributes:trace:displacements"),
        // Dicing
        TfToken("ri:attributes:dice:micropolygonlength"),
        TfToken("ri:attributes:dice:offscreenstrategy"),
        TfToken("ri:attributes:dice:rasterorient"),
        TfToken("ri:attributes:dice:referencecamera"),
        TfToken("ri:attributes:dice:referenceinstance"),
        TfToken("ri:attributes:dice:strategy"),
        TfToken("ri:attributes:dice:worlddistancelength"),
        TfToken("ri:attributes:Ri:GeometricApproximationFocusFactor"),
        TfToken("ri:attributes:Ri:GeometricApproximationMotionFactor"),
        // Points
        TfToken("ri:attributes:falloffpower"),
        // Volume
        TfToken("ri:attributes:dice:minlength"),
        TfToken("ri:attributes:dice:minlengthspace"),
        TfToken("ri:attributes:Ri:Bound"),
        TfToken("ri:attributes:volume:dsominmax"),
        TfToken("ri:attributes:volume:aggregate"),
        // SubdivisionMesh
        TfToken("ri:attributes:dice:pretessellate"),
        TfToken("ri:attributes:dice:watertight"),
        TfToken("ri:attributes:shade:faceset"),
        TfToken("ri:attributes:stitchbound:CoordinateSystem"),
        TfToken("ri:attributes:stitchbound:sphere"),
        // NuPatch
        TfToken("ri:attributes:trimcurve:sense"),
        // PolygonMesh
        TfToken("ri:attributes:polygon:concave"),
        TfToken("ri:attributes:polygon:smoothdisplacement"),
        TfToken("ri:attributes:polygon:smoothnormals"),
        // Procedural
        TfToken("ri:attributes:procedural:immediatesubdivide"),
        TfToken("ri:attributes:procedural:reentrant")
    };

    return prototypeAttributes.count(primvarName) > 0;
}

static void
_Convert(HdSceneDelegate *sceneDelegate, SdfPath const& id,
         HdInterpolation hdInterp, RtPrimVarList& params,
         _ParamType paramType, int expectedSize)
{
    // XXX:TODO: To support array-valued types, we need more
    // shaping information.  Currently we assume arrays are
    // simply N scalar values, according to the detail.

    const char* label =
        (paramType == _ParamTypePrimvar) ? "primvar" : "attribute";

    const RtDetailType detail = _RixDetailForHdInterpolation(hdInterp);

    TF_DEBUG(HDPRMAN_PRIMVARS)
        .Msg("HdPrman: _Convert called -- <%s> %s %s\n",
             id.GetText(),
             TfEnum::GetName(hdInterp).c_str(),
             label);

    // Computed primvars
    if (paramType == _ParamTypePrimvar) {
        // XXX: Prman doesn't seem to check dirtyness before pulling a value.
        // Passing AllDirty until we plumb/respect change tracking.
        HdExtComputationPrimvarDescriptorVector  computedPrimvars =
            _GetComputedPrimvars(sceneDelegate, id, hdInterp, 
                                 HdChangeTracker::AllDirty);
        if (!computedPrimvars.empty()) {
            // Execute the computations
            HdExtComputationUtils::ValueStore valueStore
                = HdExtComputationUtils::GetComputedPrimvarValues(
                    computedPrimvars, sceneDelegate);
        
            for (auto const& compPrimvar : computedPrimvars) {
                auto const it = valueStore.find(compPrimvar.name);
                if (!TF_VERIFY(it != valueStore.end())) {
                    continue;
                }
                VtValue val = it->second;
                if (val.IsEmpty() ||
                    (val.IsArrayValued() && val.GetArraySize() == 0)) {
                    continue;
                }

                RtUString name = _GetPrmanPrimvarName(compPrimvar.name, detail);

                TF_DEBUG(HDPRMAN_PRIMVARS)
                    .Msg("HdPrman: <%s> %s %s "
                         "Computed Primvar \"%s\" (%s) = \"%s\"\n",
                         id.GetText(),
                         TfEnum::GetName(hdInterp).c_str(),
                         label,
                         compPrimvar.name.GetText(),
                         name.CStr(),
                         TfStringify(val).c_str());

                if (val.IsArrayValued() && 
                    val.GetArraySize() != static_cast<size_t>(expectedSize)) {
                    TF_WARN("<%s> %s '%s' size (%zu) did not match "
                            "expected (%d)", id.GetText(), label,
                            compPrimvar.name.GetText(), val.GetArraySize(),
                            expectedSize);
                    continue;
                }

                if (!_SetPrimVarValue(name, val, detail,
                                    compPrimvar.role, params)) {
                    TF_WARN("Ignoring unhandled %s of type %s for %s.%s\n",
                        label, val.GetTypeName().c_str(), id.GetText(),
                        compPrimvar.name.GetText());
                }
            }
        }
    }

    // Authored primvars
    for (HdPrimvarDescriptor const& primvar:
         sceneDelegate->GetPrimvarDescriptors(id, hdInterp))
    {
        TF_DEBUG(HDPRMAN_PRIMVARS)
            .Msg("HdPrman: authored id <%s> hdInterp %s label %s "
                 "primvar \"%s\"\n",
                 id.GetText(),
                 TfEnum::GetName(hdInterp).c_str(),
                 label,
                 primvar.name.GetText());

        // Skip params with special handling.
        if (primvar.name == HdTokens->points) {
            continue;
        }

        // Constant Hydra primvars become either Riley primvars or attributes,
        // depending on prefix and the name.
        // 1.) Constant primvars with the "ri:attributes:" or
        //     "primvars:ri:attributes:" prefixes have that
        //     prefix stripped and become primvars for geometry prototype
        //     "attributes" or attributes for geometry instances.
        // 2.) Constant primvars with the "user:" prefix become attributes.
        // 3.) Other constant primvars get set on prototype geometry as
        //     primvars.
        RtUString name;
        if (hdInterp == HdInterpolationConstant) {
            static const char *userAttrPrefix = "user:";
            static const char *riAttrPrefix = "ri:attributes:";
            static const char *primvarsPrefix = "primvars:";
            bool hasUserPrefix =
                TfStringStartsWith(primvar.name.GetString(), userAttrPrefix);
            bool hasRiAttributesPrefix =
                TfStringStartsWith(primvar.name.GetString(), riAttrPrefix);
            bool hasPrimvarRiAttributesPrefix =
                    TfStringStartsWith(primvar.name.GetString(),primvarsPrefix);

            // Strip "primvars:" from the name
            TfToken primvarName = primvar.name;
            if (hasPrimvarRiAttributesPrefix) {
                const char *strippedName = primvar.name.GetText();
                strippedName += strlen(primvarsPrefix);
                primvarName = TfToken(strippedName);
                hasRiAttributesPrefix =
                      TfStringStartsWith(primvarName.GetString(), riAttrPrefix);
            }

            bool skipPrimvar = false;
            if (paramType == _ParamTypeAttribute) {
                // When we're looking for attributes on geometry instances,
                // they need to have either 'user:' or 'ri:attributes:' as a
                // prefix.
                if (!hasUserPrefix && !hasRiAttributesPrefix) {
                    skipPrimvar = true;
                } else if (hasRiAttributesPrefix) {
                    // For 'ri:attributes' we check if the attribute is a
                    // prototype attribute and if so omit it, since it
                    // was included with the primvars.
                    if (_IsPrototypeAttribute(primvarName)) {
                        skipPrimvar = true;
                    }
                }
            } else {
                // When we're looking for actual primvars, we skip the ones with
                // the 'user:' or 'ri:attributes:' prefix. Except for a specific
                // set of attributes that affect tessellation and dicing of the
                // prototype geometry and so it becomes part of the primvars.
                if (hasUserPrefix) {
                    skipPrimvar = true;
                } else if (hasRiAttributesPrefix) {
                    // If this ri attribute does not affect the prototype
                    // we skip
                    if (!_IsPrototypeAttribute(primvarName)) {
                        skipPrimvar = true;
                    }
                }
            }

            if (skipPrimvar) {
                continue;
            }

            if (hasRiAttributesPrefix) {
                const char *strippedName = primvarName.GetText();
                strippedName += strlen(riAttrPrefix);
                name = _GetPrmanPrimvarName(TfToken(strippedName), detail);
            } else {
                name = _GetPrmanPrimvarName(primvarName, detail);
            }
        } else {
            name = _GetPrmanPrimvarName(primvar.name, detail);
        }
        // XXX HdPrman does not yet support time-sampled primvars,
        // but we want to exercise the SamplePrimvar() API, so use it
        // to request a single sample.
        const size_t maxNumTimeSamples = 1;
        float times[1];
        VtValue val;
        sceneDelegate->SamplePrimvar(id, primvar.name, maxNumTimeSamples,
                                     times, &val);
        TF_DEBUG(HDPRMAN_PRIMVARS)
            .Msg("HdPrman: <%s> %s %s \"%s\" (%s) = \"%s\"\n",
                 id.GetText(),
                 TfEnum::GetName(hdInterp).c_str(),
                 label,
                 primvar.name.GetText(),
                 name.CStr(),
                 TfStringify(val).c_str());

        if (val.IsEmpty() ||
            (val.IsArrayValued() && val.GetArraySize() == 0)) {
            continue;
        }

        if (val.IsArrayValued() && 
            val.GetArraySize() != static_cast<size_t>(expectedSize)) {
            TF_WARN("<%s> %s '%s' size (%zu) did not match "
                    "expected (%d)", id.GetText(), label,
                    primvar.name.GetText(), val.GetArraySize(), expectedSize);
            continue;
        }

        if (!_SetPrimVarValue(name, val, detail, primvar.role, params)) {
            TF_WARN("Ignoring unhandled %s of type %s for %s.%s\n",
                label, val.GetTypeName().c_str(), id.GetText(),
                primvar.name.GetText());
        }
    }
}

void
HdPrman_ConvertPrimvars(HdSceneDelegate *sceneDelegate, SdfPath const& id,
                        RtPrimVarList& primvars, int numUniform, int numVertex,
                        int numVarying, int numFaceVarying)
{
    const HdInterpolation hdInterpValues[] = {
        HdInterpolationConstant,
        HdInterpolationUniform,
        HdInterpolationVertex,
        HdInterpolationVarying,
        HdInterpolationFaceVarying,
    };
    // The expected size of each interpolation mode. -1 means any size is
    // acceptable.
    const int primvarSizes[] = {
        1,
        numUniform,
        numVertex,
        numVarying,
        numFaceVarying
    };
    const int modeCount = 5;
    for (size_t i = 0; i < modeCount; ++i) {
        _Convert(sceneDelegate, id, hdInterpValues[i], primvars,
                 _ParamTypePrimvar, primvarSizes[i]);
    }
}

void
HdPrman_TransferMaterialPrimvarOpinions(HdSceneDelegate *sceneDelegate,
                                        SdfPath const& materialId,
                                        RtPrimVarList& primvars)
{
    if (!materialId.IsEmpty()) {
        if (const HdSprim *sprim = sceneDelegate->GetRenderIndex().GetSprim(
            HdPrimTypeTokens->material, materialId)) {
            const HdPrmanMaterial *material =
                dynamic_cast<const HdPrmanMaterial*>(sprim);
            if (material && material->IsValid()) {
                const HdMaterialNetwork2 & matNetwork = 
                    material->GetMaterialNetwork();
                for (const auto & nodeIt : matNetwork.nodes) {
                    const HdMaterialNode2 & node = nodeIt.second;
                    if (node.nodeTypeId == _tokens->PrimvarPass) {
                        for (const auto &param : node.parameters) {
                            uint32_t paramId;
                            RtUString paramName = 
                                RtUString(param.first.GetText());
                            if (!primvars.GetParamId(paramName, paramId)) {
                                _SetPrimVarValue(paramName, param.second,
                                    RtDetailType::k_constant,
                                    /*role*/TfToken(), primvars);
                            }
                        }
                    }
                }
            }
        }
    }
}

RtParamList
HdPrman_RenderParam::ConvertAttributes(HdSceneDelegate *sceneDelegate,
                                   SdfPath const& id)
{
    RtPrimVarList attrs;

    // Convert Hydra instance-rate primvars, and "user:" prefixed
    // constant  primvars, to Riley attributes.
    const HdInterpolation hdInterpValues[] = {
        HdInterpolationConstant,
    };
    for (HdInterpolation hdInterp: hdInterpValues) {
        _Convert(sceneDelegate, id, hdInterp, attrs, _ParamTypeAttribute, 1);
    }

    // Hydra id -> Riley Rix::k_identifier_name
    attrs.SetString(RixStr.k_identifier_name, RtUString(id.GetText()));

    // Hydra visibility -> Riley Rix::k_visibility
    if (!sceneDelegate->GetVisible(id)) {
        attrs.SetInteger(RixStr.k_visibility_camera, 0);
        attrs.SetInteger(RixStr.k_visibility_indirect, 0);
        attrs.SetInteger(RixStr.k_visibility_transmission, 0);
    }

    // Hydra categories -> Riley k_grouping_membership
    VtArray<TfToken> categories = sceneDelegate->GetCategories(id);
    ConvertCategoriesToAttributes(id, categories, attrs);

    return attrs;
}

void
HdPrman_RenderParam::ConvertCategoriesToAttributes(
    SdfPath const& id,
    VtArray<TfToken> const& categories,
    RtParamList& attrs)
{
    if (categories.empty()) {
        attrs.SetString( RixStr.k_lightfilter_subset,
                         RtUString("") );
        attrs.SetString( RixStr.k_lighting_subset,
                         RtUString("default") );
        TF_DEBUG(HDPRMAN_LIGHT_LINKING)
            .Msg("HdPrman: <%s> no categories; lighting:subset = \"default\"\n",
                 id.GetText());
        return;
    }

    std::string membership;
    for (TfToken const& category: categories) {
        if (!membership.empty()) {
            membership += " ";
        }
        membership += category;
    }
    // Fetch incoming grouping:membership and tack it onto categories
    RtUString inputGrouping = RtUString("");
    attrs.GetString(RixStr.k_grouping_membership, inputGrouping);
    if (inputGrouping != RtUString("")) {
        std::string input = inputGrouping.CStr();
        membership += " " + input;
    }
    attrs.SetString( RixStr.k_grouping_membership,
                       RtUString(membership.c_str()) );
    TF_DEBUG(HDPRMAN_LIGHT_LINKING)
        .Msg("HdPrman: <%s> grouping:membership = \"%s\"\n",
             id.GetText(), membership.c_str());

    // Light linking:
    // Geometry subscribes to categories of lights illuminating it.
    // Take any categories used by a light as a lightLink param
    // and list as k_lighting_subset.
    std::string lightingSubset = "default";
    for (TfToken const& category: categories) {
        if (IsLightLinkUsed(category)) {
            if (!lightingSubset.empty()) {
                lightingSubset += " ";
            }
            lightingSubset += category;
        }
    }
    attrs.SetString( RixStr.k_lighting_subset,
                      RtUString(lightingSubset.c_str()) );
    TF_DEBUG(HDPRMAN_LIGHT_LINKING)
        .Msg("HdPrman: <%s> lighting:subset = \"%s\"\n",
             id.GetText(), lightingSubset.c_str());

    // Light filter linking:
    // Geometry subscribes to categories of light filters applied to it.
    // Take any categories used by a light filter as a lightFilterLink param
    // and list as k_lightfilter_subset.
    std::string lightFilterSubset = "default";
    for (TfToken const& category: categories) {
        if (IsLightFilterUsed(category)) {
            if (!lightFilterSubset.empty()) {
                lightFilterSubset += " ";
            }
            lightFilterSubset += category;
        }
    }
    attrs.SetString( RixStr.k_lightfilter_subset,
                      RtUString(lightFilterSubset.c_str()) );
    TF_DEBUG(HDPRMAN_LIGHT_LINKING)
        .Msg("HdPrman: <%s> lightFilter:subset = \"%s\"\n",
             id.GetText(), lightFilterSubset.c_str());
}

bool
HdPrman_ResolveMaterial(HdSceneDelegate *sceneDelegate,
                        SdfPath const& hdMaterialId,
                        riley::MaterialId *materialId,
                        riley::DisplacementId *dispId)
{
    if (hdMaterialId != SdfPath()) {
        if (const HdSprim *sprim = sceneDelegate->GetRenderIndex().GetSprim(
            HdPrimTypeTokens->material, hdMaterialId)) {
            const HdPrmanMaterial *material =
                dynamic_cast<const HdPrmanMaterial*>(sprim);
            if (material && material->IsValid()) {
                *materialId = material->GetMaterialId();
                *dispId = material->GetDisplacementId();
                return true;
            }
        }
    }
    return false;
}

inline static bool
HdPrman_IsNativeRenderManFormat(std::string const &path)
{
    std::string ext = ArGetResolver().GetExtension(path);
    return (ext == "tex") || (ext == "bkm") || (ext == "ptc") || (ext == "ies");
}

RtUString
HdPrman_ResolveAssetToRtUString(SdfAssetPath const &asset,
                                bool flipTexture,
                                char const *debugNodeType)
{

    static HioImageRegistry& imageRegistry =
        HioImageRegistry::GetInstance();

    std::string v = asset.GetResolvedPath();
    if (v.empty()) {
        v = asset.GetAssetPath();
    }

    // Use the RtxHioImage plugin for resolved paths that are not
    // native RenderMan formats, but which Hio can read.
    // Note: we cannot read tex files from USDZ until we add support
    // to RtxHioImage (or another Rtx plugin) for this.
    // FUTURE NOTE: When we want to support primvar substitutions with
    // the use of non-tex textures, the following clause can no longer
    // be an "else if" (because such paths won't ArResolve), and we may 
    // not be able to even do an extension check...
    else if (!HdPrman_IsNativeRenderManFormat(v) &&
             imageRegistry.IsSupportedImageFile(v)) {
        v = "rtxplugin:RtxHioImage" ARCH_LIBRARY_SUFFIX
            "?filename=" + v + (flipTexture ? "" : "&flipped=false");
    }

    TF_DEBUG(HDPRMAN_IMAGE_ASSET_RESOLVE)
        .Msg("Resolved %s asset path: %s\n", 
             debugNodeType ? debugNodeType : "image",
             v.c_str());

    return RtUString(v.c_str());
}


HdPrman_RenderParam::RileyCoordSysIdVecRefPtr
HdPrman_RenderParam::ConvertAndRetainCoordSysBindings(
    HdSceneDelegate *sceneDelegate,
    SdfPath const& id)
{
    // Query Hydra coordinate system bindings.
    HdIdVectorSharedPtr hdIdVecPtr =
        sceneDelegate->GetCoordSysBindings(id);
    if (!hdIdVecPtr) {
        return nullptr;
    }
    // We have bindings to convert.
    std::lock_guard<std::mutex> lock(_coordSysMutex);
    // Check for an existing converted binding vector.
    _HdToRileyCoordSysMap::const_iterator it =
        _hdToRileyCoordSysMap.find(hdIdVecPtr);
    if (it != _hdToRileyCoordSysMap.end()) {
        // Found an existing conversion.
        // Record an additioanl use, on this geometry.
        _geomToHdCoordSysMap[id] = hdIdVecPtr;
        return it->second;
    }
    // Convert Hd ids to Riley id's.
    RileyCoordSysIdVec rileyIdVec;
    rileyIdVec.reserve(hdIdVecPtr->size());
    for (SdfPath const& hdId: *hdIdVecPtr) {
        // Look up sprim for binding.
        const HdSprim *sprim =
            sceneDelegate->GetRenderIndex()
            .GetSprim(HdPrimTypeTokens->coordSys, hdId);
        // Expect there to be an sprim with this id.
        if (TF_VERIFY(sprim)) {
            // Expect it to be an HdPrmanCoordSys.
            const HdPrmanCoordSys *prmanSprim =
                dynamic_cast<const HdPrmanCoordSys*>(sprim);
            if (TF_VERIFY(prmanSprim) && prmanSprim->IsValid()) {
                // Use the assigned Riley ID.
                rileyIdVec.push_back(prmanSprim->GetCoordSysId());
            }
        }
    }
    // Establish a cache entry.
    RileyCoordSysIdVecRefPtr rileyIdVecPtr =
        std::make_shared<RileyCoordSysIdVec>(rileyIdVec);
    _hdToRileyCoordSysMap[hdIdVecPtr] = rileyIdVecPtr;
    _geomToHdCoordSysMap[id] = hdIdVecPtr;
    return rileyIdVecPtr;
}

void
HdPrman_RenderParam::ReleaseCoordSysBindings(SdfPath const& id)
{
    std::lock_guard<std::mutex> lock(_coordSysMutex);
    _GeomToHdCoordSysMap::iterator geomIt = _geomToHdCoordSysMap.find(id);
    if (geomIt == _geomToHdCoordSysMap.end()) {
        // No cached bindings to release.
        return;
    }
    if (TF_VERIFY(geomIt->second) && geomIt->second.use_count() == 1) {
        // If this is the last geometry using this Riley vector,
        // we can release the cache entry.  This will free the vector.
        // (Note that the Riley coordinate system object lifetime
        // is managed by the HdPrmanCoordSys sprim.)
        _hdToRileyCoordSysMap.erase(geomIt->second);
    }
    _geomToHdCoordSysMap.erase(geomIt);
}

void
HdPrman_RenderParam::SetOptionsFromRenderSettings(
    HdPrmanRenderDelegate *renderDelegate, 
    RtParamList& options)
{
    HdRenderSettingsMap renderSettings = renderDelegate->GetRenderSettingsMap();

    for (auto const& entry : renderSettings) {
        TfToken token = entry.first;
        VtValue val = entry.second;
        
        bool hasRiPrefix = TfStringStartsWith(token.GetText(), "ri:");
        if (hasRiPrefix) {
            bool hasIntegratorPrefix =
                TfStringStartsWith(token.GetText(), "ri:integrator");
            if (hasIntegratorPrefix)
            {
                // This is an integrator setting. Skip.
                continue;
            }
            
            // Strip "ri:" namespace from USD.
            RtUString riName;
            riName = RtUString(token.GetText()+3);

            // XXX there is currently no way to distinguish the type of a 
            // float3 setting (color, point, vector).  All float3 settings are
            // treated as float[3] until we have a way to determine the type. 
            _SetParamValue(riName, val, TfToken(), options);
        } else {
            // map usd renderSetting to ri option
            if (token == HdPrmanRenderSettingsTokens->pixelAspectRatio) {
                options.SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, 
                                 val.UncheckedGet<float>());

            } else if (token == HdPrmanRenderSettingsTokens->resolution ) {
                const GfVec2i& res = val.UncheckedGet<GfVec2i>();
                options.SetIntegerArray(RixStr.k_Ri_FormatResolution, 
                                        res.data(), 2);

            }            

            // TODO: Unhandled settings from schema
            // rel camera
            // token includedPurposes
            // token materialBindingPurposes
            // rel products
            // token aspectRatioConformPolicy (ScreenWindow?)

        }
    }
}

void
HdPrman_RenderParam::SetIntegratorParamsFromRenderSettings(
    HdPrmanRenderDelegate *renderDelegate,
    const std::string& integratorName,
    RtParamList& params)
{
    HdRenderSettingsMap renderSettings = renderDelegate->GetRenderSettingsMap();

    TfToken preFix(std::string("ri:integrator:") + integratorName);
    for (auto const& entry : renderSettings) {
        TfToken token = entry.first;
        VtValue val = entry.second;
        
        bool hasRiPrefix = TfStringStartsWith(token.GetText(),
                                              preFix.GetText());
        if (hasRiPrefix) {
            // Strip namespace from USD.
            RtUString riName;
            riName = RtUString(token.GetText()+preFix.size()+1);

            _SetParamValue(riName, val, TfToken(), params);
        }
    }        
}

void
HdPrman_UpdateSearchPathsFromEnvironment(RtParamList& options)
{
    // searchpath:shader contains OSL (.oso)
    std::string shaderpath = TfGetenv("RMAN_SHADERPATH");
    if (!shaderpath.empty()) {
        // RenderMan expects ':' as path separator, regardless of platform
        NdrStringVec paths = TfStringSplit(shaderpath, ARCH_PATH_LIST_SEP);
        shaderpath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_shader,
                            RtUString(shaderpath.c_str()) );
    } else {
        NdrStringVec paths;
        // Default RenderMan installation under '$RMANTREE/lib/shaders'
        std::string rmantree = TfGetenv("RMANTREE");
        if (!rmantree.empty()) {
            paths.push_back(TfStringCatPaths(rmantree, "lib/shaders"));
        }
        // Default hdPrman installation under 'plugins/usd/resources/shaders'
        PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginWithName("hdPrmanLoader");
        if (plugin)
        {
            std::string path = TfGetPathName(plugin->GetPath());
            if (!path.empty()) {
                paths.push_back(TfStringCatPaths(path, "resources/shaders"));
            }
        }
        shaderpath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_shader,
                            RtUString(shaderpath.c_str()) );
    }

    // searchpath:rixplugin contains C++ (.so) plugins
    std::string rixpluginpath = TfGetenv("RMAN_RIXPLUGINPATH");
    if (!rixpluginpath.empty()) {
        // RenderMan expects ':' as path separator, regardless of platform
        NdrStringVec paths = TfStringSplit(rixpluginpath, ARCH_PATH_LIST_SEP);
        rixpluginpath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_rixplugin,
                            RtUString(rixpluginpath.c_str()) );
    } else {
        NdrStringVec paths;
        // Default RenderMan installation under '$RMANTREE/lib/plugins'
        std::string rmantree = TfGetenv("RMANTREE");
        if (!rmantree.empty()) {
            paths.push_back(TfStringCatPaths(rmantree, "lib/plugins"));
        }
        rixpluginpath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_rixplugin,
                            RtUString(rixpluginpath.c_str()) );
    }

    // searchpath:texture contains textures (.tex) and Rtx plugins (.so)
    std::string texturepath = TfGetenv("RMAN_TEXTUREPATH");
    if (!texturepath.empty()) {
        // RenderMan expects ':' as path separator, regardless of platform
        NdrStringVec paths = TfStringSplit(texturepath, ARCH_PATH_LIST_SEP);
        texturepath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_texture,
                            RtUString(texturepath.c_str()) );
    } else {
        NdrStringVec paths;
        // Default RenderMan installation under '$RMANTREE/lib/textures'
        // and '$RMANTREE/lib/plugins'
        std::string rmantree = TfGetenv("RMANTREE");
        if (!rmantree.empty()) {
            paths.push_back(TfStringCatPaths(rmantree, "lib/textures"));
            paths.push_back(TfStringCatPaths(rmantree, "lib/plugins"));
        }
        // Default hdPrman installation under 'plugins/usd'
        // We need the path to RtxHioImage and we assume that it lives in the
        // same directory as hdPrmanLoader
        PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginWithName("hdPrmanLoader");
        if (plugin)
        {
            std::string path = TfGetPathName(plugin->GetPath());
            if (!path.empty()) {
                paths.push_back(path);
            }
        }
        texturepath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_texture,
                            RtUString(texturepath.c_str()) );
    }

    std::string proceduralpath = TfGetenv("RMAN_PROCEDURALPATH");
    if (!proceduralpath.empty()) {
        // RenderMan expects ':' as path separator, regardless of platform
        NdrStringVec paths = TfStringSplit(proceduralpath, ARCH_PATH_LIST_SEP);
        proceduralpath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_procedural,
                            RtUString(proceduralpath.c_str()) );
    }
}

void
HdPrman_RenderParam::SetIntegratorParamsFromCamera(
    HdPrmanRenderDelegate *renderDelegate,
    const HdPrmanCamera *camera,
    std::string const& integratorName,
    RtParamList &integratorParams)
{
    for(auto const& cb: *_integratorCameraCallbacks) {
        cb(renderDelegate, camera, integratorName, integratorParams);
    }
}

void 
HdPrman_RenderParam::RegisterIntegratorCallbackForCamera(
    IntegratorCameraCallback const& callback)
{
   _integratorCameraCallbacks->push_back(callback);
}

bool
HdPrman_RenderParam::SetParamFromVtValue(
    RtUString const& name,
    VtValue const& val,
    TfToken const& role,
    RtParamList& params)
{
    return _SetParamValue(name, val, role, params);
}

void
HdPrman_RenderParam::_CreateStatsSession(void)
{
#ifdef _ENABLE_STATS
    // Set log level for diagnostics relating to initialization. If we succeed in loading a
    // config file then the log level specified in the config file will take precedence.
    stats::Logger::LogLevel statsDebugLevel = stats::GlobalLogger()->DefaultLogLevel();
    stats::SetGlobalLogLevel(statsDebugLevel);
    stats::SetGlobalLogLevel(stats::Logger::k_debug);

    // Build default listener plugin search path
    std::string listenerPath(".");
    char* rmanTreePath = getenv("RMANTREE");
    if (rmanTreePath)
    {
        listenerPath += ":";
        listenerPath += rmanTreePath;
        listenerPath += "/lib/plugins/listeners";
    }

    stats::SetListenerPluginSearchPath(listenerPath);

    // Create our stats Session config.
    std::string configFilename("stats.ini");
    std::string configSearchPathStr;
    char* configSearchPathOverride = getenv("RMAN_STATS_CONFIG_PATH");
    if (configSearchPathOverride)
    {
        configSearchPathStr = std::string(configSearchPathOverride);
    }

    // This could eventually come from a GUI so we go through
    // the motion of checking to see if we have a filename.
    stats::SessionConfig sessionConfig("HDPRman Stats Session");
    if (!configFilename.empty() && !configSearchPathStr.empty())
    {
        // Try to resolve the file in the given path and load the
        // configuration data. If it fails to find the config
        // file we'll just fall back onto the defaults.
        sessionConfig.LoadConfigFile(configSearchPathStr, configFilename);
    }

    // Instantiate a stats Session from config object.
    _statsSession = &stats::AddSession(sessionConfig);

    // Validate and inform
    _statsSession->LogInfo("HDPRMan", "Created Roz stats session '" +
                                      _statsSession->GetName() + "'.");
#endif
}

void
HdPrman_RenderParam::_CreateRiley(const std::string &rileyVariant,
    const std::string &xpuDevices)
{
    _rix = RixGetContext();
    if (!_rix) {
        TF_RUNTIME_ERROR("Could not initialize Rix API.");
        return;
    }
    _ri = (RixRiCtl*)_rix->GetRixInterface(k_RixRiCtl);
    if (!_ri) {
        TF_RUNTIME_ERROR("Could not initialize Ri API.");
        return;
    }

    // Must invoke PRManBegin() before we start using Riley.
    char arg0[] = "hdPrman";
    // Turning off unwanted statistics warnings
    // TODO: Fix incorrect tear-down handling of these statistics in 
    // interactive contexts as described in PRMAN-2353
    char arg1[] = "-woff";
    char woffs[] = "R56008,R56009";

#ifdef _ENABLE_STATS
    std::string statsSession("-statssession");
    std::string statsSessionName(_statsSession->GetName());

    char* argv[] = { arg0, arg1, woffs, &statsSession[0], &statsSessionName[0]};
    _ri->PRManBegin(5, argv);
#else
    char* argv[] = { arg0, arg1, woffs};
    _ri->PRManBegin(3, argv);
#endif

    // Register an Xcpt handler
    RixXcpt* rix_xcpt = (RixXcpt*)_rix->GetRixInterface(k_RixXcpt);
    rix_xcpt->Register(&_xcpt);

    // Populate RixStr struct
    RixSymbolResolver* sym = (RixSymbolResolver*)_rix->GetRixInterface(
        k_RixSymbolResolver);
    sym->ResolvePredefinedStrings(RixStr);

    // Sanity check symbol resolution with a canary symbol, shutterTime.
    // This can catch accidental linking with incompatible versions.
    TF_VERIFY(RixStr.k_shutterOpenTime == RtUString("shutterOpenTime"),
              "Renderman API tokens do not match expected values.  "
              "There may be a compile/link version mismatch.");

    // Acquire Riley instance.
    _mgr = (RixRileyManager*)_rix->GetRixInterface(k_RixRileyManager);

    _xpu = (!rileyVariant.empty() ||
            (rileyVariant.find("xpu") != std::string::npos));

    // Decide whether to use the CPU, GPU, or both
    RtParamList paramList;
    if (_xpu && !xpuDevices.empty()) {
        static const RtUString cpuConfig("xpu:cpuconfig");
        static const RtUString gpuConfig("xpu:gpuconfig");

        const bool useCpu = xpuDevices.find("cpu") != std::string::npos;
        paramList.SetInteger(cpuConfig, useCpu ? 1 : 0);

        const bool useGpu = xpuDevices.find("gpu") != std::string::npos;
        if (useGpu) {
            paramList.SetIntegerArray(gpuConfig, 
                                _xpuGpuConfig.data(), _xpuGpuConfig.size());
        }
    }

    _riley = _mgr->CreateRiley(RtUString(rileyVariant.c_str()), paramList);

    if(!_riley) {
        TF_RUNTIME_ERROR("Could not initialize riley API.");
        return;
    }
}


static
riley::RenderOutputType
_ToRenderOutputType(const TfToken &t)
{
    if (t == TfToken("color3f")) {
        return riley::RenderOutputType::k_Color;
    } else if (t == TfToken("float")) {
        return riley::RenderOutputType::k_Float;
    } else if (t == TfToken("int")) {
        return riley::RenderOutputType::k_Integer;
    } else {
        TF_RUNTIME_ERROR("Unimplemented renderVar dataType '%s'; "
                         "skipping", t.GetText());
        return riley::RenderOutputType::k_Integer;
    }
}

// Helper to convert a dictionary of Hydra settings to Riley params.
static
RtParamList
_ToRtParamList(VtDictionary const& dict)
{
    RtParamList params;

    for (auto const& entry: dict) {
        RtUString riName(entry.first.c_str());

        if (entry.second.IsHolding<int>()) {
            params.SetInteger(riName, entry.second.UncheckedGet<int>());
        } else if (entry.second.IsHolding<float>()) {
            params.SetFloat(riName, entry.second.UncheckedGet<float>());
        } else if (entry.second.IsHolding<std::string>()) {
            params.SetString(riName,
                RtUString(entry.second.UncheckedGet<std::string>().c_str()));
        } else if (entry.second.IsHolding<VtArray<int>>()) {
            auto const& array = entry.second.UncheckedGet<VtArray<int>>();
            params.SetIntegerArray(riName, array.data(), array.size());
        } else if (entry.second.IsHolding<VtArray<float>>()) {
            auto const& array = entry.second.UncheckedGet<VtArray<float>>();
            params.SetFloatArray(riName, array.data(), array.size());
        } else {
            TF_CODING_ERROR("Unimplemented setting %s of type %s\n",
                            entry.first.c_str(),
                            entry.second.GetTypeName().c_str());
        }
    }

    return params;
}

static
HdPrman_RenderViewDesc
_ComputeRenderViewDesc(
    const VtDictionary &renderSpec,
    const riley::CameraId cameraId,
    const riley::IntegratorId integratorId,
    const riley::SampleFilterList &sampleFilterList,
    const GfVec2i &resolution)
{
    HdPrman_RenderViewDesc renderViewDesc;

    renderViewDesc.cameraId = cameraId;
    renderViewDesc.integratorId = integratorId;
    renderViewDesc.resolution = resolution;
    renderViewDesc.sampleFilterList = sampleFilterList;

    const std::vector<VtValue> &renderVars =
        VtDictionaryGet<std::vector<VtValue>>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->renderVars);
    
    for (const VtValue &renderVarVal : renderVars) {
        const VtDictionary renderVar = renderVarVal.Get<VtDictionary>();
        const std::string &nameStr =
            VtDictionaryGet<std::string>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->name);
        const RtUString name(nameStr.c_str());

        HdPrman_RenderViewDesc::RenderOutputDesc renderOutputDesc;
        renderOutputDesc.name = name;
        renderOutputDesc.type = _ToRenderOutputType(
            TfToken(
                VtDictionaryGet<std::string>(
                    renderVar,
                    HdPrmanExperimentalRenderSpecTokens->type)));
        renderOutputDesc.sourceName = name;
        renderOutputDesc.rule = RixStr.k_filter;
        renderOutputDesc.params = _ToRtParamList(
            VtDictionaryGet<VtDictionary>(
                renderVar,
                HdPrmanExperimentalRenderSpecTokens->params,
                VtDefault = VtDictionary()));
        renderViewDesc.renderOutputDescs.push_back(renderOutputDesc);
    }
    
    const std::vector<VtValue> & renderProducts =
        VtDictionaryGet<std::vector<VtValue>>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->renderProducts);

    for (const VtValue &renderProductVal : renderProducts) {

        const VtDictionary &renderProduct = renderProductVal.Get<VtDictionary>();

        HdPrman_RenderViewDesc::DisplayDesc displayDesc;

        const TfToken name(
            VtDictionaryGet<std::string>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->name));

        displayDesc.name = RtUString(name.GetText());
        
        // get output display driver type
        // TODO this is not a robust solution
        static const std::map<std::string,TfToken> extToDisplayDriver{
            { std::string("exr"),  TfToken("openexr") },
            { std::string("tif"),  TfToken("tiff") },
            { std::string("tiff"), TfToken("tiff") },
            { std::string("png"),  TfToken("png") }
        };
        
        const std::string outputExt = TfGetExtension(name.GetString());
        const TfToken dspyFormat = extToDisplayDriver.at(outputExt);
        displayDesc.driver = RtUString(dspyFormat.GetText());

        displayDesc.params = _ToRtParamList(
            VtDictionaryGet<VtDictionary>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->params,
                VtDefault = VtDictionary()));

        const VtIntArray &renderVarIndices =
            VtDictionaryGet<VtIntArray>(
                renderProduct,
                HdPrmanExperimentalRenderSpecTokens->renderVarIndices);
        for (const int renderVarIndex : renderVarIndices) {
            displayDesc.renderOutputIndices.push_back(renderVarIndex);
        }
        renderViewDesc.displayDescs.push_back(displayDesc);
    }

    return renderViewDesc;
}

void
HdPrman_RenderParam::CreateRenderViewFromSpec(
    const VtDictionary &renderSpec)
{
    const HdPrman_RenderViewDesc renderViewDesc =
        _ComputeRenderViewDesc(
            renderSpec,
            GetCameraContext().GetCameraId(),
            GetActiveIntegratorId(),
            GetSampleFilterList(),
            GfVec2i(512, 512));

    GetRenderViewContext().CreateRenderView(
        renderViewDesc, AcquireRiley());
}

void
HdPrman_RenderParam::_DestroyRiley()
{
    if (_mgr) {
        if(_riley) {
            _mgr->DestroyRiley(_riley);
        }
        _mgr = nullptr;
    }

    _riley = nullptr;

    if (_rix) {
        RixXcpt* rix_xcpt = (RixXcpt*)_rix->GetRixInterface(k_RixXcpt);
        rix_xcpt->Unregister(&_xcpt);
    }

    if (_ri) {
        _ri->PRManEnd();
        _ri = nullptr;
    }
}

void
HdPrman_RenderParam::_DestroyStatsSession(void)
{
#ifdef _ENABLE_STATS
    if (_statsSession)
    {
        stats::RemoveSession(*_statsSession);
        _statsSession = nullptr;
    }
#endif
}

static
RtParamList
_ComputeVolumeNodeParams()
{
    static const RtUString us_densityFloatPrimVar("densityFloatPrimVar");
    static const RtUString us_density("density");
    static const RtUString us_diffuseColor("diffuseColor");

    RtParamList result;
    result.SetString(
        us_densityFloatPrimVar, us_density);
    // 18% albedo chosen to match Storm's fallback volume shader.
    result.SetColor(
        us_diffuseColor, RtColorRGB(0.18, 0.18, 0.18));
    return result;
}

void
HdPrman_RenderParam::_CreateFallbackMaterials()
{
    // Default material
    {
        std::vector<riley::ShadingNode> materialNodes;
        HdPrman_ConvertHdMaterialNetwork2ToRmanNodes(
            HdPrmanMaterial_GetFallbackSurfaceMaterialNetwork(),
            SdfPath("/PxrSurface"), // We assume this terminal name here
            &materialNodes
        );
        _fallbackMaterialId = _riley->CreateMaterial(
            riley::UserId(stats::AddDataLocation(materialNodes[0].name.CStr()).GetValue()),
            {static_cast<uint32_t>(materialNodes.size()), materialNodes.data()},
            RtParamList());
    }

    // Volume default material
    {
        static const RtUString us_PxrVolume("PxrVolume");
        static const RtUString us_simpleVolume("simpleVolume");

        const std::vector<riley::ShadingNode> materialNodes{
            riley::ShadingNode{
                riley::ShadingNode::Type::k_Bxdf,
                us_PxrVolume,
                us_simpleVolume,
                _ComputeVolumeNodeParams()}};
        _fallbackVolumeMaterialId = _riley->CreateMaterial(
            riley::UserId(stats::AddDataLocation(materialNodes[0].name.CStr()).GetValue()),
            {static_cast<uint32_t>(materialNodes.size()), materialNodes.data()},
            RtParamList());
    }
}

void
HdPrman_RenderParam::SetLastSettingsVersion(const int version)
{
    _lastSettingsVersion = version;
}

void
HdPrman_RenderParam::InvalidateTexture(const std::string &path)
{
    AcquireRiley();

    _ri->InvalidateTexture(RtUString(path.c_str()));
}

riley::ShadingNode
HdPrman_RenderParam::_ComputeIntegratorNode(
    HdRenderDelegate * const renderDelegate,
    const HdPrmanCamera * const cam)
{
    const std::string &integratorName =
        renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->integratorName,
            HdPrmanIntegratorTokens->PxrPathTracer.GetString());

    const RtUString rtIntegratorName(integratorName.c_str());

    SetIntegratorParamsFromRenderSettings(
        static_cast<HdPrmanRenderDelegate*>(renderDelegate),
        integratorName,
        _integratorParams);

    if (cam) {
        SetIntegratorParamsFromCamera(
            static_cast<HdPrmanRenderDelegate*>(renderDelegate),
            cam,
            integratorName,
            _integratorParams);
    }

    return riley::ShadingNode{
        riley::ShadingNode::Type::k_Integrator,
        rtIntegratorName,
        rtIntegratorName,
        _integratorParams};
}

void
HdPrman_RenderParam::_CreateIntegrator(HdRenderDelegate * const renderDelegate)
{
    // Called when there isn't even a render index yet, so we ignore
    // integrator opinions coming from the camera here. They will be
    // consumed in UpdateIntegrator.
    static const HdPrmanCamera * const camera = nullptr;

    riley::ShadingNode integratorNode(_ComputeIntegratorNode(renderDelegate, camera));
    _integratorId = _riley->CreateIntegrator(
        riley::UserId(stats::AddDataLocation(integratorNode.name.CStr()).GetValue()),
        integratorNode);
}

void
HdPrman_RenderParam::UpdateIntegrator(
    const HdRenderIndex * const renderIndex)
{
    const riley::ShadingNode node = _ComputeIntegratorNode(
        renderIndex->GetRenderDelegate(),
        _cameraContext.GetCamera(renderIndex));

    AcquireRiley()->ModifyIntegrator(
        _integratorId, &node);
}

void 
HdPrman_RenderParam::_RenderThreadCallback()
{
    static RtUString const US_RENDERMODE = RtUString("renderMode");
    static RtUString const US_INTERACTIVE = RtUString("interactive");

    // Note: this is currently hard-coded because hdprman only ever 
    // create a single camera. When this changes, we will need to make sure 
    // the correct name is used here.
    // Note: why not use us_main_cam defined earlier in the same file?
    static RtUString const defaultReferenceCamera = RtUString("main_cam");

    RtParamList renderOptions;
    renderOptions.SetString(US_RENDERMODE, US_INTERACTIVE);
    renderOptions.SetString(
        RixStr.k_dice_referencecamera, 
        defaultReferenceCamera);

    bool renderComplete = false;
    while (!renderComplete) {
        while (_renderThread->IsPauseRequested()) {
            if (_renderThread->IsStopRequested()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (_renderThread->IsStopRequested()) {
            break;
        }

        HdPrman_RenderViewContext &ctx = GetRenderViewContext();

        const riley::RenderViewId renderViewIds[] = { ctx.GetRenderViewId() };

        _riley->Render(
            { static_cast<uint32_t>(TfArraySize(renderViewIds)),
              renderViewIds },
            renderOptions);

        // If a pause was requested, we may have stopped early
        renderComplete = !_renderThread->IsPauseDirty();
    }
}

bool 
HdPrman_RenderParam::IsValid() const
{
    return _riley;
}

void 
HdPrman_RenderParam::Begin(HdPrmanRenderDelegate *renderDelegate)
{
    //////////////////////////////////////////////////////////////////////// 
    //
    // Riley setup
    //
    static const RtUString us_circle("circle");

    // XXX Shutter settings from studio katana defaults:
    // - /root.renderSettings.shutter{Open,Close}
    float shutterInterval[2] = { 0.0f, 0.5f };
    if (!TfGetEnvSetting(HD_PRMAN_ENABLE_MOTIONBLUR)) {
        shutterInterval[1] = 0.0;
    }

    // Options
    {
        RtParamList &options = GetOptions();

        // Set thread limit for Renderman. Leave a few threads for app.
        static const unsigned appThreads = 4;
        unsigned nThreads = std::max(WorkGetConcurrencyLimit()-appThreads, 1u);
        // Check the environment
        unsigned nThreadsEnv = TfGetEnvSetting(HD_PRMAN_NTHREADS);
        if (nThreadsEnv > 0) {
            nThreads = nThreadsEnv;
        } else {
            // Otherwise check for a render setting
            VtValue vtThreads = renderDelegate->GetRenderSetting(
                HdRenderSettingsTokens->threadLimit).Cast<int>();
            if (!vtThreads.IsEmpty()) {
                nThreads = vtThreads.UncheckedGet<int>();
            }
        }
        options.SetInteger(RixStr.k_limits_threads, nThreads);

        // Set resolution from render settings
        const VtValue resolutionVal = renderDelegate->GetRenderSetting(
            HdPrmanRenderSettingsTokens->resolution);

        if (resolutionVal.IsHolding<GfVec2i>()) {
            resolution = resolutionVal.UncheckedGet<GfVec2i>();
            options.SetIntegerArray(RixStr.k_Ri_FormatResolution,
                                    resolution.data(), 2);
        }

        // Read the maxSamples out of settings (if it exists).
        // Use a low value to default to a non-expensive render.
        VtValue vtMaxSamples = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedSamplesPerPixel).Cast<int>();
        int maxSamples = TF_VERIFY(!vtMaxSamples.IsEmpty()) ?
            vtMaxSamples.UncheckedGet<int>() : 16;
        options.SetInteger(RixStr.k_hider_minsamples, 1);
        options.SetInteger(RixStr.k_hider_maxsamples, maxSamples);

        // Read the variance threshold out of settings (if it exists). Use a
        // default of 0.001.
        VtValue vtPixelVariance = renderDelegate->GetRenderSetting(
            HdRenderSettingsTokens->convergedVariance).Cast<float>();
        float pixelVariance = TF_VERIFY(!vtPixelVariance.IsEmpty()) ?
            vtPixelVariance.UncheckedGet<float>() : 0.001f;
        options.SetFloat(RixStr.k_Ri_PixelVariance, pixelVariance);

        HdPrman_UpdateSearchPathsFromEnvironment(options);

        // Path tracer config.
        options.SetInteger(RixStr.k_hider_incremental, 1);
        options.SetInteger(RixStr.k_hider_jitter, !_disableJitter);
        options.SetInteger(RixStr.k_trace_maxdepth, 10);
        options.SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, 1.0f);
        options.SetString(RixStr.k_bucket_order, us_circle);

        // Camera lens
        options.SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2);

        // OSL verbose
        int oslVerbose = TfGetEnvSetting(HD_PRMAN_OSL_VERBOSE);
        if (oslVerbose > 0)
            options.SetInteger(RtUString("user:osl:verbose"), oslVerbose);

        // Searchpaths (TEXTUREPATH, etc)
        HdPrman_UpdateSearchPathsFromEnvironment(options);
        
        // Set Options from RenderSettings schema
        SetOptionsFromRenderSettings(
            static_cast<HdPrmanRenderDelegate*>(renderDelegate), options);
        
        _riley->SetOptions(_GetDeprecatedOptionsPrunedList());
    }

    GetCameraContext().Begin(_riley);

    _CreateIntegrator(renderDelegate);
    _CreateQuickIntegrator(renderDelegate);
    _activeIntegratorId = GetIntegratorId();

    _CreateFallbackMaterials();

    // Set the camera path before the first sync so that
    // HdPrmanCamera::Sync can detect whether it is syncing the
    // current camera and needs to set the riley shutter interval
    // which needs to be set before any time-sampled primvars are
    // synced.
    const VtDictionary &renderSpec =
        renderDelegate->GetRenderSetting<VtDictionary>(
            HdPrmanRenderSettingsTokens->experimentalRenderSpec,
            VtDictionary());
    const SdfPath &cameraPath =
        VtDictionaryGet<SdfPath>(
            renderSpec,
            HdPrmanExperimentalRenderSpecTokens->camera,
            VtDefault = SdfPath());
    GetCameraContext().SetCameraPath(cameraPath);
}

void 
HdPrman_RenderParam::SetActiveIntegratorId(
    const riley::IntegratorId id)
{
    _activeIntegratorId = id;

    riley::Riley * riley = AcquireRiley();

    GetRenderViewContext().SetIntegratorId(id, riley);
}

void 
HdPrman_RenderParam::StartRender()
{
    // Last chance to set Ri options before starting riley!
    // Called from HdPrman_RenderPass::_Execute

    // Prepare Riley state for rendering.
    // Pass a valid riley callback pointer during IPR

    if (!_renderThread) {
        _renderThread = std::make_unique<HdRenderThread>();
        _renderThread->SetRenderCallback(
            std::bind(
                &HdPrman_RenderParam::_RenderThreadCallback, this));
        _renderThread->StartThread();
    }

#ifdef _ENABLE_STATS
    // Clear out old stats values
    if (_statsSession)
    {
        _statsSession->RemoveOldMetricData();
    }
#endif

    _renderThread->StartRender();
}

void
HdPrman_RenderParam::StopRender(bool blocking)
{
    if (!_renderThread || !_renderThread->IsRendering()) {
        return;
    }

    if (!blocking) {
        _riley->RequestUpdate();
        return;
    }

    // Note: if we were rendering, when the flag goes low we'll be back in
    // render thread idle until another StartRender comes in, so we don't need
    // to manually call renderThread->StopRender. Theoretically
    // riley->Stop() is blocking, but we need the loop here because:
    // 1. It's possible that IsRendering() is true because we're in the preamble
    //    of the render loop, before calling into riley. In that case, Stop()
    //    is a no-op and we need to call it again after we call into Riley.
    // 2. We've occassionally seen cases where Stop() returns successfully,
    //    but the riley threadpools don't shut down right away.
    while (_renderThread->IsRendering()) {
        _riley->Stop();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100us);
    }

#ifdef _ENABLE_STATS
    // Clear out old stats values. TODO: should we be calling this here? 
    if (_statsSession)
    {
        _statsSession->RemoveOldMetricData();
    }
#endif
}

bool
HdPrman_RenderParam::IsRendering()
{
    return _renderThread && _renderThread->IsRendering();
}

bool
HdPrman_RenderParam::IsPauseRequested()
{
    return _renderThread && _renderThread->IsPauseRequested();
}

void
HdPrman_RenderParam::DeleteRenderThread()
{
    if (_renderThread) {
        _renderThread->StopThread();
        _renderThread.reset();
    }
}

bool
HdPrman_RenderParam::_UpdateFramebufferClearValues(
    const HdRenderPassAovBindingVector& aovBindings)
{
    if(_framebuffer->aovBuffers.size() != aovBindings.size()) {
        // Number of AOVs changed, can't update framebuffer clear values.
        return false;
    }

    for(size_t aov = 0; aov < aovBindings.size(); ++aov) {
        const HdRenderPassAovBinding &aovBinding =
            aovBindings[aov];
        HdPrmanFramebuffer::AovDesc &aovDesc =
            _framebuffer->aovBuffers[aov].desc;
        if (aovBinding.aovName != aovDesc.name) {
            // Different AOV, can't update framebuffer clear value.
            return false;
        }
        
        if(aovBinding.aovName == HdAovTokens->color ||
           aovBinding.aovName == HdAovTokens->depth) {
            if (aovDesc.clearValue != aovBinding.clearValue) {
                // Request a framebuffer clear if the clear value in the aov
                // has changed from the framebuffer clear value.
                // We do this before StartRender() to avoid race conditions
                // where some buckets may get discarded or cleared with
                // the wrong value.
                
                // Stops render and increases sceneVersion to trigger restart.
                AcquireRiley();
                
                _framebuffer->pendingClear = true;
                aovDesc.clearValue = aovBinding.clearValue;
            }
        }
    }

    return true;
}    

static
void
_ComputeRenderOutputAndAovDescs(
    const HdRenderPassAovBindingVector& aovBindings,
    bool isXpu,
    std::vector<HdPrman_RenderViewDesc::RenderOutputDesc> * renderOutputDescs,
    HdPrmanFramebuffer::AovDescVector * aovDescs)
{
    static const RtUString us_ci("ci");
    static const RtUString us_st("__st");
    static const RtUString us_primvars_st("primvars:st");

    std::unordered_map<TfToken, RtUString, TfToken::HashFunctor> sourceNames;

    for (const HdRenderPassAovBinding &aovBinding : aovBindings) {
        std::string dataType;
        std::string sourceType;
        RtUString aovName(aovBinding.aovName.GetText());
        RtUString sourceName;
        riley::RenderOutputType rt = riley::RenderOutputType::k_Float;
        RtUString rule = RixStr.k_filter;

        HdFormat aovFormat = aovBinding.renderBuffer->GetFormat();

        // Prman always renders colors as float, so for types with 3 or 4
        // components, always set the format in our framebuffer to float.
        // Conversion will take place in the Blit method of renderBuffer.cpp
        // when it notices that the aovBinding's buffer format doesn't match
        // our framebuffer's format.
        const int componentCount = HdGetComponentCount(aovFormat);
        if(componentCount == 3)
        {
            aovFormat = HdFormatFloat32Vec3;
        }
        else if(componentCount == 4)
        {
            aovFormat = HdFormatFloat32Vec4;
        }

        // Prman only supports float, color, and integer
        if(aovFormat == HdFormatFloat32)
        {
            rt = riley::RenderOutputType::k_Float;
        }
        else if(aovFormat == HdFormatFloat32Vec4 ||
                aovFormat == HdFormatFloat32Vec3)
        {
            rt = riley::RenderOutputType::k_Color;
        }
        else if(aovFormat == HdFormatInt32)
        {
            rt = riley::RenderOutputType::k_Integer;
        }

        {
            // Look at the aovSettings to see if there is
            // information about the source.  In prman
            // an aov can have an arbitrary name, while its source
            // might be an lpe or a standard aov name.
            // When no source is specified, we'll assume the aov name
            // is standard and also use that as the source.
            auto it = aovBinding.aovSettings.find(_tokens->sourceName);
            if (it != aovBinding.aovSettings.end()) {
                const VtValue &val = it->second;
                if (val.IsHolding<TfToken>()) {
                    sourceName =
                        RtUString(
                            val.UncheckedGet<TfToken>().GetText());
                }
            }
        }

        {
            // Look at the aovSettings to see if there is
            // information about the source.  In prman
            // an aov can have an arbitrary name, while its source
            // might be an lpe or a standard aov name.
            // When no source is specified, we'll assume the aov name
            // is standard and also use that as the source.
            auto it = aovBinding.aovSettings.find(_tokens->sourceType);
            if (it != aovBinding.aovSettings.end()) {
                const VtValue &val = it->second;
                if (val.IsHolding<TfToken>()) {
                    sourceType = val.UncheckedGet<TfToken>().GetString();
                }
            }
        }

        // If the sourceType hints that the source is an lpe, make sure
        // it starts with "lpe:" as required by prman.
        if(sourceType == RixStr.k_lpe.CStr())
        {
            std::string sn = sourceName.CStr();
            if(sn.find(RixStr.k_lpe.CStr()) == std::string::npos)
                sn = "lpe:" + sn;
            sourceName = RtUString(sn.c_str());
        }

        // Map some standard hydra aov names to their equivalent prman names
        if(aovBinding.aovName == HdAovTokens->color ||
           aovBinding.aovName.GetString() == us_ci.CStr())
        {
            aovName = RixStr.k_Ci;
            sourceName = RixStr.k_Ci;
        }
        else if(aovBinding.aovName == HdAovTokens->depth)
        {
            sourceName = RixStr.k_z;
        }
        else if(aovBinding.aovName == HdAovTokens->normal)
        {
            sourceName= RixStr.k_Nn;
        }
        else if(aovBinding.aovName == HdAovTokens->primId)
        {
            aovName = RixStr.k_id;
            sourceName = RixStr.k_id;
        }
        else if(aovBinding.aovName == HdAovTokens->instanceId)
        {
            aovName = RixStr.k_id2;
            sourceName = RixStr.k_id2;
        }
        else if(aovBinding.aovName == HdAovTokens->elementId)
        {
            aovName = RixStr.k_faceindex;
            sourceName = RixStr.k_faceindex;
        }
        else if(aovName == us_primvars_st)
        {
            sourceName = us_st;
        }

        // If no sourceName is specified, assume name is a standard prman aov
        if(sourceName.Empty())
        {
            sourceName = aovName;
        }

        // XPU is picky about AOV names, it wants only standard names
        if(isXpu)
        {
            aovName = sourceName;
        }

        // z and integer types require zmin filter
        if(sourceName == RixStr.k_id || sourceName == RixStr.k_id2 ||
           sourceName == RixStr.k_z || rt == riley::RenderOutputType::k_Integer)
        {
            rule = RixStr.k_zmin;
        }

        if(!sourceName.Empty())
        {
            // This is a workaround for an issue where we get an
            // unexpected duplicate in the aovBindings sometimes,
            // where the second entry lacks a sourceName.
            // Can't just skip it because the caller expects
            // a result in the buffer
            sourceNames[aovBinding.aovName] = sourceName;
        } else {
            auto it = sourceNames.find(aovBinding.aovName);
            if(it != sourceNames.end())
            {
                sourceName = it->second;
            }
        }

        {
            HdPrman_RenderViewDesc::RenderOutputDesc renderOutputDesc;
            renderOutputDesc.name = aovName;
            renderOutputDesc.type = rt;
            renderOutputDesc.sourceName = sourceName;
            renderOutputDesc.rule = rule;
            
            renderOutputDescs->push_back(
                std::move(renderOutputDesc));
        }

        // When a float4 color is requested, assume we require alpha as well.
        // This assumption is reflected in framebuffer.cpp HydraDspyData
        if(rt == riley::RenderOutputType::k_Color && componentCount == 4)
        {
            HdPrman_RenderViewDesc::RenderOutputDesc renderOutputDesc;
            renderOutputDesc.name = RixStr.k_a;
            renderOutputDesc.type = riley::RenderOutputType::k_Float;
            renderOutputDesc.sourceName = RixStr.k_a;
            renderOutputDesc.rule = RixStr.k_filter;

            renderOutputDescs->push_back(
                std::move(renderOutputDesc));
        }
        
        {
            HdPrmanFramebuffer::AovDesc aovDesc;
            aovDesc.name = aovBinding.aovName;
            aovDesc.format = aovFormat;
            aovDesc.clearValue = aovBinding.clearValue;
            aovDesc.rule = HdPrmanFramebuffer::ToAccumulationRule(rule);
            
            aovDescs->push_back(std::move(aovDesc));
        }
    }
}

void
HdPrman_RenderParam::CreateFramebufferAndRenderViewFromAovs(
    const HdRenderPassAovBindingVector& aovBindings)
{
    if (!_framebuffer) {
        _framebuffer = std::make_unique<HdPrmanFramebuffer>();
    }

    if(_UpdateFramebufferClearValues(aovBindings)) {
        // AOVs are the same and updating the clear values succeeded,
        // nothing more to do.
        return;
    }

    // Proceed with creating displays if the number has changed
    // or the display names don't match what we have.

    // Stop render and crease sceneVersion to trigger restart.
    riley::Riley * riley = AcquireRiley();

    std::lock_guard<std::mutex> lock(_framebuffer->mutex);

    // Displays & Display Channels
    HdPrman_RenderViewDesc renderViewDesc;
    HdPrmanFramebuffer::AovDescVector aovDescs;

    _ComputeRenderOutputAndAovDescs(
        aovBindings,
        IsXpu(),
        &renderViewDesc.renderOutputDescs,
        &aovDescs);

    _framebuffer->CreateAovBuffers(aovDescs);

    renderViewDesc.resolution = resolution;

    static const RtUString us_bufferID("bufferID");
    static const RtUString us_hydra("hydra");

    RtUString driver(us_hydra);
    RtParamList displayParams;

    if(IsXpu())
    {
        // XPU loads hdPrman as the display plug-in
        PlugPluginPtr const plugin =
            PlugRegistry::GetInstance().GetPluginWithName("hdPrman");
        assert(plugin);
        std::string hdPrmanPath;
        if (plugin) {
            const std::string path = TfGetPathName(plugin->GetPath());
            if (!path.empty()) {
                hdPrmanPath =
                    TfStringCatPaths(path, "hdPrman");
            }
        }

        driver = RtUString(hdPrmanPath.c_str());

        displayParams.SetString(
            RixStr.k_Ri_name,
            RixStr.k_framebuffer);
        displayParams.SetString(
            RixStr.k_Ri_type,
            driver);
        displayParams.SetInteger(
            us_bufferID,
            _framebuffer->id);
    }

    {
        HdPrman_RenderViewDesc::DisplayDesc displayDesc;
        displayDesc.name = RixStr.k_framebuffer;
        displayDesc.driver = driver;
        displayDesc.params = displayParams;
        for (size_t i = 0; i < renderViewDesc.renderOutputDescs.size(); i++) {
            displayDesc.renderOutputIndices.push_back(i);
        }
        
        renderViewDesc.displayDescs.push_back(std::move(displayDesc));
    }

    renderViewDesc.cameraId = GetCameraContext().GetCameraId();
    renderViewDesc.integratorId = GetActiveIntegratorId();
    renderViewDesc.sampleFilterList = GetSampleFilterList();

    GetRenderViewContext().CreateRenderView(
        renderViewDesc, riley);
}

bool
HdPrman_RenderParam::DeleteFramebuffer()
{
    if (_framebuffer) {
        _framebuffer.reset();
        return true;
    }
    return false;
}

riley::IntegratorId
HdPrman_RenderParam::GetActiveIntegratorId()
{
    return _activeIntegratorId;
}

RtParamList 
HdPrman_RenderParam::_GetDeprecatedOptionsPrunedList()
{
    // The following should not be given to Riley::SetOptions() anymore.
    static std::vector<RtUString> const _deprecatedRileyOptions = {
        RixStr.k_Ri_PixelFilterName, 
        RixStr.k_hider_pixelfiltermode, 
        RixStr.k_Ri_PixelFilterWidth,
        RixStr.k_Ri_ScreenWindow};

    RtParamList prunedOptions = GetOptions();
    uint32_t paramId;
    for (auto name : _deprecatedRileyOptions) {
        if (prunedOptions.GetParamId(name, paramId)) {
            prunedOptions.Remove(paramId);
        }
    }

    return prunedOptions;
}

riley::Riley *
HdPrman_RenderParam::AcquireRiley()
{
    StopRender();
    sceneVersion++;

    return _riley;
}

riley::ShadingNode
HdPrman_RenderParam::_ComputeQuickIntegratorNode(
    HdRenderDelegate * const renderDelegate,
    const HdPrmanCamera * const cam)
{
    const std::string &integratorName =
        renderDelegate->GetRenderSetting<std::string>(
            HdPrmanRenderSettingsTokens->interactiveIntegrator,
            HdPrmanIntegratorTokens->PxrDirectLighting.GetString());

    const RtUString rtIntegratorName(integratorName.c_str());

    SetIntegratorParamsFromRenderSettings(
        static_cast<HdPrmanRenderDelegate*>(renderDelegate),
        integratorName,
        _quickIntegratorParams);

    if (cam) {
        SetIntegratorParamsFromCamera(
            static_cast<HdPrmanRenderDelegate*>(renderDelegate),
            cam,
            integratorName,
            _quickIntegratorParams);
    }

    static const RtUString numLightSamples("numLightSamples");
    static const RtUString numBxdfSamples("numBxdfSamples");

    _quickIntegratorParams.SetInteger(numLightSamples, 1);
    _quickIntegratorParams.SetInteger(numBxdfSamples, 1);        

    return riley::ShadingNode{
        riley::ShadingNode::Type::k_Integrator,
        rtIntegratorName,
        rtIntegratorName,
        _quickIntegratorParams};
}

void
HdPrman_RenderParam::_CreateQuickIntegrator(
    HdRenderDelegate * const renderDelegate)
{
    // See comment in _CreateIntegrator.
    static const HdPrmanCamera * const camera = nullptr;

    if (_enableQuickIntegrate) {
      riley::ShadingNode integratorNode(
          _ComputeQuickIntegratorNode(renderDelegate, camera));
      _quickIntegratorId = _riley->CreateIntegrator(
          riley::UserId(
              stats::AddDataLocation(integratorNode.name.CStr()).GetValue()),
          integratorNode);
    }
}

void
HdPrman_RenderParam::UpdateQuickIntegrator(
    const HdRenderIndex * const renderIndex)
{
    if (_enableQuickIntegrate) {
        const riley::ShadingNode node =
            _ComputeQuickIntegratorNode(
                renderIndex->GetRenderDelegate(),
                _cameraContext.GetCamera(renderIndex));
        
        AcquireRiley()->ModifyIntegrator(
            _quickIntegratorId,
            &node);
    }
}

// Note that we only support motion blur with the correct shutter
// interval if the the camera path and disableMotionBlur value
// have been set to the desired values before any syncing or rendering
// has happened. We don't update the riley shutter interval in
// response to setting these render settings. The only callee of
// UpdateRileyShutterInterval is HdPrmanCamera::Sync.
//
// This limitation is due to Riley's limitation: the shutter interval
// option has to be set before any sampled prim vars or transforms are
// given to Riley. It might be possible to circumvent this limitation
// by forcing a sync of all rprim's and the camera transform (through
// the render index'es change tracker) when the shutter interval changes.
//
void
HdPrman_RenderParam::UpdateRileyShutterInterval(
    const HdRenderIndex * const renderIndex)
{
    // Fallback shutter interval.
    float shutterInterval[2] = { 0.0f, 0.5f };
    
    // Try to get shutter interval from camera.
    if (const HdCamera * const camera =
            _cameraContext.GetCamera(renderIndex)) {
        shutterInterval[0] = camera->GetShutterOpen();
        shutterInterval[1] = camera->GetShutterClose();
    }

    // Deprecated.
    const bool instantaneousShutter =
        renderIndex->GetRenderDelegate()->GetRenderSetting<bool>(
            HdPrmanRenderSettingsTokens->instantaneousShutter, false);
    if (instantaneousShutter) {
        // Disable motion blur by making the interval a single point.
        shutterInterval[1] = shutterInterval[0];
    }

    const bool disableMotionBlur =
        renderIndex->GetRenderDelegate()->GetRenderSetting<bool>(
            HdPrmanRenderSettingsTokens->disableMotionBlur, false);
    if (disableMotionBlur) {
        // Disable motion blur by sampling at current frame only.
        shutterInterval[0] = 0.0f;
        shutterInterval[1] = 0.0f;
    }
    
    RtParamList &options = GetOptions();
    options.SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2); 
    
    riley::Riley * riley = AcquireRiley();
    riley->SetOptions(_GetDeprecatedOptionsPrunedList());
}

void
HdPrman_RenderParam::SetConnectedSampleFilterPaths(
    HdSceneDelegate *sceneDelegate,
    SdfPathVector const &connectedSampleFilterPaths)
{
    if (_connectedSampleFilterPaths != connectedSampleFilterPaths) {
        // Reset the Filter Shading Nodes and update the Connected Paths
        _sampleFilterNodes.clear();
        _connectedSampleFilterPaths = connectedSampleFilterPaths;

        if (! HdRenderIndex::IsSceneIndexEmulationEnabled()) {
            // Mark the SampleFilter Prims Dirty
            for (const SdfPath &path : connectedSampleFilterPaths) {
                sceneDelegate->GetRenderIndex().GetChangeTracker()
                    .MarkSprimDirty(path, HdChangeTracker::DirtyParams);
            }
        }
    }

    // If there are no connected SampleFilters, delete the riley SampleFilter
    if (_connectedSampleFilterPaths.size() == 0) {
        if (_sampleFiltersId != riley::SampleFilterId::InvalidId()) {
            AcquireRiley()->DeleteSampleFilter(_sampleFiltersId);
            _sampleFiltersId = riley::SampleFilterId::InvalidId();
        }
    }
}

void
HdPrman_RenderParam::CreateSampleFilterNetwork(HdSceneDelegate *sceneDelegate)
{
    std::vector<riley::ShadingNode> shadingNodes;
    std::vector<RtUString> filterRefs;

    // Gather shading nodes and reference paths (for combiner) for all connected
    // and visible SampleFilters. The filterRefs order needs to match the order
    // of SampleFilters specified in the RenderSettings connection.
    for (const auto& path : _connectedSampleFilterPaths) {
        if (sceneDelegate->GetVisible(path)) {
            const auto it = _sampleFilterNodes.find(path);
            if (!TF_VERIFY(it != _sampleFilterNodes.end())) {
                continue;
            }
            if (it->second.name) {
                shadingNodes.push_back(it->second);
                filterRefs.push_back(RtUString(path.GetText()));
            }
        }
    }

    // If we have multiple SampleFilters, create a SampleFilter Combiner Node
    if (shadingNodes.size() > 1) {
        static RtUString filterArrayName("filter");
        static RtUString pxrSampleFilterCombiner("PxrSampleFilterCombiner");

        riley::ShadingNode combinerNode;
        combinerNode.type = riley::ShadingNode::Type::k_SampleFilter;
        combinerNode.handle = pxrSampleFilterCombiner;
        combinerNode.name = pxrSampleFilterCombiner;
        combinerNode.params.SetSampleFilterReferenceArray(
            filterArrayName, filterRefs.data(), filterRefs.size());
        shadingNodes.push_back(combinerNode);
    }
    
    // Create or update the Riley SampleFilters
    riley::ShadingNetwork const sampleFilterNetwork = {
        static_cast<uint32_t>(shadingNodes.size()), &shadingNodes[0] };
    
    if (_sampleFiltersId == riley::SampleFilterId::InvalidId()) {
        _sampleFiltersId = AcquireRiley()->CreateSampleFilter(
            riley::UserId(stats::AddDataLocation("/sampleFilters").
                              GetValue()),
            sampleFilterNetwork, 
            RtParamList());
    }
    else {
        AcquireRiley()->ModifySampleFilter(
            _sampleFiltersId, &sampleFilterNetwork, nullptr);
    }

    if (_sampleFiltersId == riley::SampleFilterId::InvalidId()) {
        TF_WARN("Failed to create the Sample Filter(s)\n");
    }
}

void
HdPrman_RenderParam::AddSampleFilter(
    HdSceneDelegate *sceneDelegate,
    SdfPath const& path,
    riley::ShadingNode const& node)
{
    // Update or Add the SampleFilter Shading node
    const auto filterIt = _sampleFilterNodes.insert( {path, node} );
    if (!filterIt.second) {
        filterIt.first->second = node;
    }

    // If we have all the Shading Nodes, create the SampleFilters in Riley
    if (_sampleFilterNodes.size() == _connectedSampleFilterPaths.size()) {
        CreateSampleFilterNetwork(sceneDelegate);
    }
}

riley::SampleFilterList
HdPrman_RenderParam::GetSampleFilterList()
{
    return (_sampleFiltersId == riley::SampleFilterId::InvalidId()) 
        ? riley::SampleFilterList({ 0, nullptr })
        : riley::SampleFilterList({ 1, &_sampleFiltersId });
}

PXR_NAMESPACE_CLOSE_SCOPE
