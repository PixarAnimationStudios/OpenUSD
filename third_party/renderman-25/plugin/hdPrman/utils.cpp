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

#include "pxr/base/arch/library.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/visitValue.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/assetPath.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hio/imageRegistry.h"

#include "hdPrman/rixStrings.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (primvar)
);

namespace {

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

bool
_IsNativeRenderManFormat(std::string const &path)
{
    const std::string ext = ArGetResolver().GetExtension(path);
    return (ext == "tex") || (ext == "bkm") || (ext == "ptc") || (ext == "ies");
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
    return VtVisitValue(val, _VtValueToRtParamList{name, role, params});
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
    return VtVisitValue(val, _VtValueToRtPrimVar(name, detail, role, params));
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

}

PXR_NAMESPACE_CLOSE_SCOPE