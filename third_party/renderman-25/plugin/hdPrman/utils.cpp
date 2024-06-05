//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/utils.h"

#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h" // HDPRMAN_SHUTTER{OPEN,CLOSE}_DEFAULT
#include "hdPrman/rixStrings.h"
#include "hdPrman/tokens.h"

#include "pxr/base/arch/env.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/pathUtils.h"  // ARCH_PATH_LIST_SEP
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/visitValue.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hio/imageRegistry.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/sdf/assetPath.h"

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
    bool operator()(const long &v) {
        return params->SetInteger(name, static_cast<int>(v));
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
    bool operator()(const GfVec3i &v) {
        return params->SetIntegerArray(name, v.data(), 3); 
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
    bool operator()(const GfVec4i &v) {
        return params->SetIntegerArray(name, v.data(), 4); 
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
    bool operator()(const VtArray<long> &vl) {
        // convert long->int
        VtArray<int> v;
        v.resize(vl.size());
        for (size_t i=0,n=vl.size(); i<n; ++i) {
            v[i] = int(vl[i]);
        }
        return (*this)(v);
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
        if (role == HdPrmanRileyAdditionalRoleTokens->colorReference) {
            return params->SetColorReference(name, RtUString(v.GetText()));
        } else if (role == HdPrmanRileyAdditionalRoleTokens->floatReference) {
            return params->SetFloatReference(name, RtUString(v.GetText()));
        } else {
            return params->SetString(name, RtUString(v.GetText()));
        }
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
    bool operator()(const VtArray<long> &vl) {
        // Convert double->int
        VtArray<int> v;
        v.resize(vl.size());
        for (size_t i=0,n=vl.size(); i<n; ++i) {
            v[i] = int(vl[i]);
        }
        return (*this)(v);
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

/// Update the supplied list of options using searchpaths
/// pulled from envrionment variables:
///
/// - RMAN_SHADERPATH
/// - RMAN_TEXTUREPATH
/// - RMAN_RIXPLUGINPATH
/// - RMAN_PROCEDURALPATH
/// - RMAN_DISPLAYPATH
///
void
_UpdateSearchPathsFromEnvironment(RtParamList& options)
{
    // Default RenderMan installation under '$RMANTREE/lib/shaders'
    std::string rmantree = TfGetenv("RMANTREE");
    // Default hdPrman installation under 'plugins/usd/resources/shaders'
    PlugPluginPtr plugin =
        PlugRegistry::GetInstance().GetPluginWithName("hdPrmanLoader");

    {
        // searchpath:shader contains OSL (.oso)
        std::string shaderpath = TfGetenv("RMAN_SHADERPATH");
        NdrStringVec paths;
        if (!shaderpath.empty()) {
            // RenderMan expects ':' as path separator, regardless of platform
            for (auto path : TfStringSplit(shaderpath, ARCH_PATH_LIST_SEP))
            {
                paths.push_back(path);
            }
        }
        if (!rmantree.empty()) {
            paths.push_back(TfStringCatPaths(rmantree, "lib/shaders"));
        }
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

    {
        // searchpath:rixplugin contains C++ (.so) plugins
        std::string rixpluginpath = TfGetenv("RMAN_RIXPLUGINPATH");
        NdrStringVec paths;
        if (!rixpluginpath.empty()) {
            // RenderMan expects ':' as path separator, regardless of platform
            for (auto path : TfStringSplit(rixpluginpath, ARCH_PATH_LIST_SEP))
            {
                paths.push_back(path);
            }
        }
        // Default RenderMan installation under '$RMANTREE/lib/plugins'
        if (!rmantree.empty()) {
            paths.push_back(TfStringCatPaths(rmantree, "lib/plugins"));
        }
        rixpluginpath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_rixplugin,
                           RtUString(rixpluginpath.c_str()) );
    }

    {
        // searchpath:texture contains textures (.tex) and Rtx plugins (.so)
        std::string texturepath = TfGetenv("RMAN_TEXTUREPATH");
        NdrStringVec paths;
        if (!texturepath.empty()) {
            // RenderMan expects ':' as path separator, regardless of platform
            for (auto path : TfStringSplit(texturepath, ARCH_PATH_LIST_SEP))
            {
                paths.push_back(path);
            }
        }
        // Default RenderMan installation under '$RMANTREE/lib/textures'
        // and '$RMANTREE/lib/plugins'
        if (!rmantree.empty()) {
            paths.push_back(TfStringCatPaths(rmantree, "lib/textures"));
            paths.push_back(TfStringCatPaths(rmantree, "lib/plugins"));
        }
        // Default hdPrman installation under 'plugins/usd'
        // We need the path to RtxHioImage and we assume that it lives in the
        // same directory as hdPrmanLoader
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

    {
        std::string proceduralpath = TfGetenv("RMAN_PROCEDURALPATH");
        NdrStringVec paths;
        if (!proceduralpath.empty()) {
            // RenderMan expects ':' as path separator, regardless of platform
            for (std::string const& path : TfStringSplit(proceduralpath,
                                                         ARCH_PATH_LIST_SEP))
            {
                paths.push_back(path);
            }
        }

        // Default RenderMan installation under '$RMANTREE/lib/plugins'
        if (!rmantree.empty()) {
            paths.push_back(TfStringCatPaths(rmantree, "lib/plugins"));
        }
        proceduralpath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_procedural,
                           RtUString(proceduralpath.c_str()) );
    }

    {
        std::string displaypath = TfGetenv("RMAN_DISPLAYPATH");
        NdrStringVec paths;
        if (!displaypath.empty()) {
            // RenderMan expects ':' as path separator, regardless of platform
            for (std::string const& path : TfStringSplit(displaypath,
                                                         ARCH_PATH_LIST_SEP))
            {
                paths.push_back(path);
            }
        }

        // Default RenderMan installation under '$RMANTREE/lib/plugins'
        if (!rmantree.empty()) {
            paths.push_back(TfStringCatPaths(rmantree, "lib/plugins"));
        }
        displaypath = TfStringJoin(paths, ":");
        options.SetString( RixStr.k_searchpath_display,
                           RtUString(displaypath.c_str()) );
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
    return VtVisitValue(val, _VtValueToRtPrimVar(
        name, detail, role, params));
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
PruneBatchOnlyOptions(
    const RtParamList &options)
{
    // The following should not be given to Riley::SetOptions()
    // when doing an interactive render.
    //
    // XXX We use an explicit list here, but would it be better
    // to do a prefix-check instead?
    static std::vector<RtUString> const _batchOnlyRileyOptions = {
        RixStr.k_checkpoint,
        RixStr.k_checkpoint_asfinal,
        RixStr.k_checkpoint_command,
        RixStr.k_checkpoint_exitat,
        RixStr.k_checkpoint_interval,
        RixStr.k_checkpoint_keepfiles,
        RixStr.k_exitat,
        RixStr.k_statistics,
        RixStr.k_statistics_displaceratios,
        RixStr.k_statistics_endofframe,
        RixStr.k_statistics_filename,
        RixStr.k_statistics_level,
        RixStr.k_statistics_maxdispwarnings,
        RixStr.k_statistics_shaderprofile,
        RixStr.k_statistics_stylesheet,
        RixStr.k_statistics_texturestatslevel,
        RixStr.k_statistics_xmlfilename
        };

    RtParamList prunedOptions = options;
    for (auto name : _batchOnlyRileyOptions) {
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
    
    float shutterInterval[2] = {
        HDPRMAN_SHUTTEROPEN_DEFAULT,
        HDPRMAN_SHUTTERCLOSE_DEFAULT
    };
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

} // namespace HdPrman_Utils

PXR_NAMESPACE_CLOSE_SCOPE
