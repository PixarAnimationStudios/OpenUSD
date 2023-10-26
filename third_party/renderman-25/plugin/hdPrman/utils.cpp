//
// Copyright 2023 Pixar
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

#include "hdPrman/utils.h"
#include "hdPrman/debugCodes.h"
#if PXR_VERSION < 2211 // HdPrman_RenderParam::SetParamFromVtValue()
#include "hdPrman/renderParam.h"
#endif

#include "pxr/base/arch/env.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/pathUtils.h"  // Extract extension from tf token
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#if PXR_VERSION >= 2211 // VtVisitValue
#include "pxr/base/vt/visitValue.h"
#endif
#include "pxr/base/vt/value.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/sdf/assetPath.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hio/imageRegistry.h"

#include "hdPrman/rixStrings.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (primvar)
);

extern TfEnvSetting<bool> HD_PRMAN_DISABLE_HIDER_JITTER;
extern TfEnvSetting<bool> HD_PRMAN_ENABLE_MOTIONBLUR;
extern TfEnvSetting<int> HD_PRMAN_NTHREADS;
extern TfEnvSetting<int> HD_PRMAN_OSL_VERBOSE;

namespace {

#if PXR_VERSION < 2211
// This duplicates what's in renderParam.cpp prior to 22.11 but never exported.
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
        params.SetMatrix(name, HdPrman_Utils::GfMatrixToRtMatrix(v));
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
        RtUString v = HdPrman_Utils::ResolveAssetToRtUString(
            asset, flipTexture, _tokens->primvar.GetText());
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
            us.push_back(HdPrman_Utils::ResolveAssetToRtUString(
                asset, flipTexture, _tokens->primvar.GetText()));
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
#else
// _VtValueToRtParamList is a helper used with VtVisitValue to handle
// type dispatch for converting VtValues to RtParamList entries.
struct _VtValueToRtParamList
{
    RtUString const& name;
    TfToken const& role;
    RtParamList *params;

    //
    // Scalars
    //
    bool operator()(const int &v) {
        return params->SetInteger(name, v);
    }
    bool operator()(const float &v) {
        return params->SetFloat(name, v);
    }
    bool operator()(const double &v) {
        return params->SetFloat(name, static_cast<float>(v));
    }

    //
    // Gf types
    //
    bool operator()(const GfVec2i &v) {
        return params->SetIntegerArray(name, v.data(), 2); 
    }
    bool operator()(const GfVec2f &v) {
        return params->SetFloatArray(name, v.data(), 2); 
    }
    bool operator()(const GfVec2d &vd) {
        return (*this)(GfVec2f(vd));
    }
    bool operator()(const GfVec3f &v) {
        if (role == HdPrimvarRoleTokens->color) {
            return params->SetColor(name, RtColorRGB(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->point) {
            return params->SetPoint(name, RtPoint3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->normal) {
            return params->SetNormal(name, RtNormal3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->vector) {
            return params->SetVector(name, RtVector3(v[0], v[1], v[2]));
        } else {
            return params->SetFloatArray(name, v.data(), 3);
        }
    }
    bool operator()(const GfVec3d &vd) {
        return (*this)(GfVec3f(vd));
    }
    bool operator()(const GfVec4f &v) {
        return params->SetFloatArray(name, v.data(), 4);
    }
    bool operator()(const GfVec4d &vd) {
        return (*this)(GfVec4f(vd));
    }
    bool operator()(const GfMatrix4d &v) {
        return params->SetMatrix(name,   HdPrman_Utils::GfMatrixToRtMatrix(v));
    }

    //
    // Arrays of scalars
    //
    bool operator()(const VtArray<bool> &vb) {
        // bool->integer
        VtArray<int> v;
        v.resize(vb.size());
        for (size_t i=0,n=vb.size(); i<n; ++i) {
            v[i] = int(vb[i]);
        }
        return (*this)(v);
    }
    bool operator()(const VtArray<int> &v) {
        return params->SetIntegerArray(name, v.cdata(), v.size());
    }
    bool operator()(const VtArray<float> &v) {
        return params->SetFloatArray(name, v.cdata(), v.size());
    }
    bool operator()(const VtArray<double> &vd) {
        // Convert double->float
        VtArray<float> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = float(vd[i]);
        }
        return (*this)(v);
    }

    //
    // Arrays of Gf types
    //
    bool operator()(const VtArray<GfVec2f> &v) {
        return params->SetFloatArray(name,   
            reinterpret_cast<const float*>(v.cdata()), 2*v.size());
    }
    bool operator()(const VtArray<GfVec2d> &vd) {
        // Convert double->float
        VtArray<GfVec2f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec2f(vd[i]);
        }
        return (*this)(v);
    }
    bool operator()(const VtArray<GfVec3f> &v) {
        if (role == HdPrimvarRoleTokens->color) {
            return params->SetColorArray(
                name, reinterpret_cast<const RtColorRGB*>(v.cdata()),
                v.size());
        } else if (role == HdPrimvarRoleTokens->point) {
            return params->SetPointArray(
                name, reinterpret_cast<const RtPoint3*>(v.cdata()),
                v.size());
        } else if (role == HdPrimvarRoleTokens->normal) {
            return params->SetNormalArray(
                name, reinterpret_cast<const RtNormal3*>(v.cdata()),
                v.size());
        } else if (role == HdPrimvarRoleTokens->vector) {
            return params->SetVectorArray(
                name, reinterpret_cast<const RtVector3*>(v.cdata()),
                v.size());
        } else {
            return params->SetFloatArray(
                name, reinterpret_cast<const float*>(v.cdata()),
                3*v.size());
        }
    }
    bool operator()(const VtArray<GfVec3d> &vd) {
        // double->float
        VtArray<GfVec3f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec3f(vd[i]);
        }
        return (*this)(v);
    }
    bool operator()(const VtArray<GfVec4f> &v) {
        return params->SetFloatArray(
            name, reinterpret_cast<const float*>(v.cdata()), 4*v.size());
    }
    bool operator()(const VtArray<GfVec4d> &vd) {
        // double->float
        VtArray<GfVec4f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec4f(vd[i]);
        }
        return (*this)(v);
    }

    //
    // String-like types
    //
    bool operator()(const TfToken &v) {
        return params->SetString(name, RtUString(v.GetText()));
    }
    bool operator()(const std::string &v) {
        return params->SetString(name, RtUString(v.c_str()));
    }
    bool operator()(const SdfAssetPath &assetPath) {
        // Since we can't know how the texture will be consumed,
        // go with the default of flipping textures
        const bool flipTexture = true;
        RtUString v = HdPrman_Utils::ResolveAssetToRtUString(
            assetPath, flipTexture, _tokens->primvar.GetText());
        return params->SetString(name, v);
    }
    
    //
    // Arrays of string-like types
    //
    bool operator()(const std::vector<RtUString> &us) {
        return params->SetStringArray(name, us.data(), us.size());
    }
    bool operator()(const VtArray<TfToken> &v) {
        // Convert to RtUString.
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (TfToken const& s: v) {
            us.push_back(RtUString(s.GetText()));
        }
        return (*this)(us);
    }
    bool operator()(const VtArray<std::string> &v) {
        // Convert to RtUString.
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (std::string const& s: v) {
            us.push_back(RtUString(s.c_str()));
        }
        return (*this)(us);
    }
    bool operator()(const VtArray<SdfAssetPath> &v) {
        // Convert to RtUString.
        // Since we can't know how the texture will be consumed,
        // go with the default of flipping textures
        const bool flipTexture = true;
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (SdfAssetPath const& asset: v) {
            us.push_back(HdPrman_Utils::ResolveAssetToRtUString(asset, flipTexture,
                                                   _tokens->primvar.GetText()));
        }
        return (*this)(us);
    }

    bool operator()(const VtValue &val) {
        // Dispatch for types that are not part of VT_VALUE_TYPES
        // because they are defined downstream of Vt.
        if (val.IsHolding<SdfAssetPath>()) {
            return (*this)(val.UncheckedGet<SdfAssetPath>());
        } else if (val.IsHolding<VtArray<SdfAssetPath>>()) {
            return (*this)(val.UncheckedGet<VtArray<SdfAssetPath>>());
        } else if (val.IsHolding<std::vector<RtUString>>()) {
            return (*this)(val.UncheckedGet<std::vector<RtUString>>());
        } else {
            TF_CODING_ERROR("Cannot handle type %s\n", val.GetTypeName().c_str());
            return false;
        }
    }
};


// _VtValueToRtPrimVar derives from _VtValueToRtParamList, corresponding
// to how RtPrimVarList derives from RtParamList.  In both cases, the
// key addition is the concept of 'detail', which specifies how array
// values should be handled across topology.
struct _VtValueToRtPrimVar : _VtValueToRtParamList
{
    RtDetailType detail;
    RtPrimVarList *primvars;

    _VtValueToRtPrimVar(RtUString const& name_,
                        RtDetailType detail_,
                        TfToken const& role_,
                        RtPrimVarList *primvars_)
        : _VtValueToRtParamList{name_, role_, primvars_}
        , detail(detail_)
        , primvars(primvars_)
    {}

    // Make operator() for the underlying _VtValueToRtPrimVar
    // visible here.
    using _VtValueToRtParamList::operator();

    //
    // Arrays of scalars
    //
    bool operator()(const VtArray<bool> &vb) {
        // bool->integer
        VtArray<int> v;
        v.resize(vb.size());
        for (size_t i=0,n=vb.size(); i<n; ++i) {
            v[i] = int(vb[i]);
        }
        return (*this)(v);
    }
    bool operator()(const VtArray<int> &v) {
        if (detail == RtDetailType::k_constant) {
            return primvars->SetIntegerArray(name, v.cdata(), v.size());
        } else {
            return primvars->SetIntegerDetail(name, v.cdata(), detail);
        }
    }
    bool operator()(const VtArray<float> &v) {
        if (detail == RtDetailType::k_constant) {
            return primvars->SetFloatArray(name, v.cdata(), v.size());
        } else {
            return primvars->SetFloatDetail(name, v.cdata(), detail);
        }
    }
    bool operator()(const VtArray<double> &vd) {
        // Convert double->float
        VtArray<float> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = float(vd[i]);
        }
        return (*this)(v);
    }

    //
    // Arrays of Gf types
    //
    bool operator()(const VtArray<GfVec2f> &v) {
        return primvars->SetFloatArrayDetail(name,
            reinterpret_cast<const float*>(v.cdata()), 2, detail);
    }
    bool operator()(const VtArray<GfVec2d> &vd) {
        // Convert double->float
        VtArray<GfVec2f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec2f(vd[i]);
        }
        return (*this)(v);
    }
    bool operator()(const VtArray<GfVec3f> &v) {
        if (role == HdPrimvarRoleTokens->color) {
            return primvars->SetColorDetail(
                name, reinterpret_cast<const RtColorRGB*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->point) {
            return primvars->SetPointDetail(
                name, reinterpret_cast<const RtPoint3*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->normal) {
            return primvars->SetNormalDetail(
                name, reinterpret_cast<const RtNormal3*>(v.cdata()),
                detail);
        } else if (role == HdPrimvarRoleTokens->vector) {
            return primvars->SetVectorDetail(
                name, reinterpret_cast<const RtVector3*>(v.cdata()),
                detail);
        } else {
            return primvars->SetFloatArrayDetail(
                name, reinterpret_cast<const float*>(v.cdata()),
                3, detail);
        }
    }
    bool operator()(const VtArray<GfVec3d> &vd) {
        // double->float
        VtArray<GfVec3f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec3f(vd[i]);
        }
        return (*this)(v);
    }
    bool operator()(const VtArray<GfVec4f> &v) {
        return primvars->SetFloatArrayDetail(
            name, reinterpret_cast<const float*>(v.cdata()), 4, detail);
    }
    bool operator()(const VtArray<GfVec4d> &vd) {
        // double->float
        VtArray<GfVec4f> v;
        v.resize(vd.size());
        for (size_t i=0,n=vd.size(); i<n; ++i) {
            v[i] = GfVec4f(vd[i]);
        }
        return (*this)(v);
    }

    //
    // Arrays of string-like types
    //
    bool operator()(const std::vector<RtUString> &us) {
        if (detail == RtDetailType::k_constant) {
            return primvars->SetStringArray(name, us.data(), us.size());
        } else {
            return primvars->SetStringDetail(name, us.data(), detail);
        }
    }
    bool operator()(const VtArray<TfToken> &v) {
        // Convert to RtUString.
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (TfToken const& s: v) {
            us.push_back(RtUString(s.GetText()));
        }
        return (*this)(us);
    }
    bool operator()(const VtArray<std::string> &v) {
        // Convert to RtUString.
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (std::string const& s: v) {
            us.push_back(RtUString(s.c_str()));
        }
        return (*this)(us);
    }
    bool operator()(const VtArray<SdfAssetPath> &v) {
        // Convert to RtUString.
        // Since we can't know how the texture will be consumed,
        // go with the default of flipping textures
        const bool flipTexture = true;
        std::vector<RtUString> us;
        us.reserve(v.size());
        for (SdfAssetPath const& asset: v) {
            us.push_back(HdPrman_Utils::ResolveAssetToRtUString(asset, flipTexture,
                                                   _tokens->primvar.GetText()));
        }
        return (*this)(us);
    }

    bool operator()(const VtValue &val) {
        // Dispatch for types that are not part of VT_VALUE_TYPES
        // because they are defined downstream of Vt.
        if (val.IsHolding<SdfAssetPath>()) {
            return (*this)(val.UncheckedGet<SdfAssetPath>());
        } else if (val.IsHolding<VtArray<SdfAssetPath>>()) {
            return (*this)(val.UncheckedGet<VtArray<SdfAssetPath>>());
        } else if (val.IsHolding<std::vector<RtUString>>()) {
            return (*this)(val.UncheckedGet<std::vector<RtUString>>());
        } else {
            TF_CODING_ERROR("Cannot handle type %s\n", val.GetTypeName().c_str());
            return false;
        }
    }
};
#endif

bool
_IsNativeRenderManFormat(std::string const &path)
{
    const std::string ext = ArGetResolver().GetExtension(path);
    return (ext == "tex") || (ext == "bkm") || (ext == "ptc") || (ext == "ies");
}

/// Update the supplied list of options using searchpaths
/// pulled from envrionment variables:
///
/// - RMAN_SHADERPATH
/// - RMAN_TEXTUREPATH
/// - RMAN_RIXPLUGINPATH
/// - RMAN_PROCEDURALPATH
///
void
_UpdateSearchPathsFromEnvironment(RtParamList& options)
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

}

// -----------------------------------------------------------------------------

namespace HdPrman_Utils {

bool
SetParamFromVtValue(
    RtUString const& name,
    VtValue const& val,
    TfToken const& role,
    RtParamList *params)
{
    if (ARCH_UNLIKELY(!params)) {
        return false;
    }
#if PXR_VERSION < 2211
    return HdPrman_RenderParam::SetParamFromVtValue(name, val, role, *params);
#else
    return VtVisitValue(val, _VtValueToRtParamList{name, role, params});
#endif
}

RtParamList
ParamsFromDataSource(
    HdContainerDataSourceHandle const &containerDs)
{
    RtParamList result;
    if (!containerDs) {
        return result;
    }
    for (const TfToken &name : containerDs->GetNames()) {
        if (HdSampledDataSourceHandle const ds =
                HdSampledDataSource::Cast(containerDs->Get(name))) {
            SetParamFromVtValue(
                RtUString(name.GetText()),
                ds->GetValue(0.0f),
                TfToken(),
                &result);
        }
    }

    return result;
}

bool
SetPrimVarFromVtValue(
    RtUString const& name,
    VtValue const& val,
    RtDetailType const& detail,
    TfToken const& role,
    RtPrimVarList *params)
{
    if (ARCH_UNLIKELY(!params)) {
        return false;
    }
#if PXR_VERSION < 2211
    return _SetPrimVarValue(name, val, detail, role, *params);
#else
    return VtVisitValue(val, _VtValueToRtPrimVar(
        name, detail, role, params));
#endif
}

RtUString
ResolveAssetToRtUString(
    SdfAssetPath const &asset,
    bool flipTexture,
    char const *debugNodeType /* = nullptr*/)
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
    else if (!_IsNativeRenderManFormat(v) &&
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

RtParamList
PruneDeprecatedOptions(
    const RtParamList &options)
{
    // The following should not be given to Riley::SetOptions() anymore.
    static std::vector<RtUString> const _deprecatedRileyOptions = {
        RixStr.k_Ri_PixelFilterName, 
        RixStr.k_hider_pixelfiltermode, 
        RixStr.k_Ri_PixelFilterWidth,
        RixStr.k_Ri_ScreenWindow};

    RtParamList prunedOptions = options;
    for (auto name : _deprecatedRileyOptions) {
        uint32_t paramId;
        if (prunedOptions.GetParamId(name, paramId)) {
            prunedOptions.Remove(paramId);
        }
    }

    return prunedOptions;
}

RtParamList
GetDefaultRileyOptions()
{
    RtParamList options;

    // Set default thread limit for Renderman. Leave a few threads for app.
    {
        const unsigned appThreads = 4;
        const unsigned nThreads =
            std::max(WorkGetConcurrencyLimit()-appThreads, 1u);
        options.SetInteger(RixStr.k_limits_threads, nThreads);
    }

    // Path tracer default configuration. Values below may be overriden by
    // those in the legacy render settings map and/or prim.
    options.SetInteger(RixStr.k_hider_minsamples, 1);
    options.SetInteger(RixStr.k_hider_maxsamples, 16);
    options.SetInteger(RixStr.k_hider_incremental, 1);
    options.SetInteger(RixStr.k_trace_maxdepth, 10);
    options.SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, 1.0f);
    options.SetFloat(RixStr.k_Ri_PixelVariance, 0.001f);
    options.SetString(RixStr.k_bucket_order, RtUString("circle"));
    
     // Default shutter settings from studio katana defaults:
    // - /root.renderSettings.shutter{Open,Close}
    float shutterInterval[2] = { 0.0f, 0.5f };
    options.SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2);

    return options;
}

RtParamList
GetRileyOptionsFromEnvironment()
{
    RtParamList options;

    const unsigned nThreadsEnv = TfGetEnvSetting(HD_PRMAN_NTHREADS);
    if (nThreadsEnv > 0) {
        options.SetInteger(RixStr.k_limits_threads, nThreadsEnv);
    }

    if (!TfGetEnvSetting(HD_PRMAN_ENABLE_MOTIONBLUR)) {
        float shutterInterval[2] = { 0.0f, 0.0f };
        options.SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2);
    }

    // OSL verbose
    const int oslVerbose = TfGetEnvSetting(HD_PRMAN_OSL_VERBOSE);
    if (oslVerbose > 0) {
        options.SetInteger(RtUString("user:osl:verbose"), oslVerbose);
    }

    const bool disableJitter = TfGetEnvSetting(HD_PRMAN_DISABLE_HIDER_JITTER);
    options.SetInteger(RixStr.k_hider_jitter, !disableJitter);

    if (ArchHasEnv("HD_PRMAN_MAX_SAMPLES")) {
        const int maxSamples = TfGetenvInt("HD_PRMAN_MAX_SAMPLES", 64);
        options.SetInteger(RixStr.k_hider_maxsamples, maxSamples);
    }

    // Searchpaths (TEXTUREPATH, etc)
    _UpdateSearchPathsFromEnvironment(options);

    return options;
}

RtParamList
Compose(
    RtParamList const &a,
    RtParamList const &b)
{
    if (b.GetNumParams() == 0) {
        return a;
    }
    if (a.GetNumParams() == 0) {
        return b;
    }

    RtParamList result = b;
    result.Update(a);
    return result;
}

}

PXR_NAMESPACE_CLOSE_SCOPE
