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

#include "hdPrman/context.h"
#include "hdPrman/coordSys.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/material.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/rixStrings.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/debug.h"
#include "pxr/usd/sdr/registry.h"

#include "Riley.h"
#include "RtParamList.h"
#include "RixRiCtl.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

void
HdPrman_Context::IncrementLightLinkCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    ++_lightLinkRefs[name];
}

void 
HdPrman_Context::DecrementLightLinkCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    if (--_lightLinkRefs[name] == 0) {
        _lightLinkRefs.erase(name);
    }
}

bool 
HdPrman_Context::IsLightLinkUsed(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightLinkMutex);
    return _lightLinkRefs.find(name) != _lightLinkRefs.end();
}

bool
HdPrman_Context::IsInteractive() const
{
    return _isInteractive;
}

void
HdPrman_Context::SetIsInteractive(bool isInteractive)
{
    _isInteractive = isInteractive;
}

bool 
HdPrman_Context::IsShutterInstantaneous() const
{
    return _instantaneousShutter;
}

void
HdPrman_Context::SetInstantaneousShutter(bool instantaneousShutter)
{
    _instantaneousShutter = instantaneousShutter;
}

void
HdPrman_Context::IncrementLightFilterCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    ++_lightFilterRefs[name];
}

void 
HdPrman_Context::DecrementLightFilterCount(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    if (--_lightFilterRefs[name] == 0) {
        _lightFilterRefs.erase(name);
    }
}

bool 
HdPrman_Context::IsLightFilterUsed(TfToken const& name)
{
    std::lock_guard<std::mutex> lock(_lightFilterMutex);
    return _lightFilterRefs.find(name) != _lightFilterRefs.end();
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
               RtDetailType const& detail,
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
            params.SetPoint(name, RtNormal3(v[0], v[1], v[2]));
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
            params.SetPoint(name, RtNormal3(v[0], v[1], v[2]));
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
            params.SetStringArray(name, &us[0], us.size());
        } else {
            params.SetStringDetail(name, &us[0], detail);
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
            params.SetStringArray(name, &us[0], us.size());
        } else {
            params.SetStringDetail(name, &us[0], detail);
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

static void
_Convert(HdSceneDelegate *sceneDelegate, SdfPath const& id,
         HdInterpolation hdInterp, RtParamList& params,
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

                if (!_SetParamValue(name, val, detail,
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
            .Msg("HdPrman: authored id <%s> hdInterp %s label %s primvar \"%s\"\n",
                 id.GetText(),
                 TfEnum::GetName(hdInterp).c_str(),
                 label,
                 primvar.name.GetText());

        // Skip params with special handling.
        if (primvar.name == HdTokens->points) {
            continue;
        }

        // Constant Hydra primvars become either Riley primvars or attributes,
        // depending on prefix.
        // 1.) Constant primvars with the "ri:attributes:"
        //     prefix have that prefix stripped and become attributes.
        // 2.) Constant primvars with the "user:" prefix become attributes.
        // 3.) Other constant primvars get set on master,
        //     e.g. displacementbounds.
        RtUString name;
        if (hdInterp == HdInterpolationConstant) {
            bool hasUserPrefix =
                TfStringStartsWith(primvar.name.GetString(), "user:");
            bool hasRiAttributesPrefix =
                TfStringStartsWith(primvar.name.GetString(), "ri:attributes:");
            if ((paramType == _ParamTypeAttribute) ^
                (hasUserPrefix || hasRiAttributesPrefix)) {
                continue;
            }
            const char *strippedName = primvar.name.GetText();
            static const char *riAttrPrefix = "ri:attributes:";
            if (!strncmp(strippedName, riAttrPrefix, strlen(riAttrPrefix))) {
                strippedName += strlen(riAttrPrefix);
            }
            name = _GetPrmanPrimvarName(TfToken(strippedName), detail);
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

        if (!_SetParamValue(name, val, detail, primvar.role, params)) {
            TF_WARN("Ignoring unhandled %s of type %s for %s.%s\n",
                label, val.GetTypeName().c_str(), id.GetText(),
                primvar.name.GetText());
        }
    }
}

void
HdPrman_ConvertPrimvars(HdSceneDelegate *sceneDelegate, SdfPath const& id,
                        RtParamList& primvars, int numUniform, int numVertex,
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

RtParamList
HdPrman_Context::ConvertAttributes(HdSceneDelegate *sceneDelegate,
                                   SdfPath const& id)
{
    RtParamList attrs;

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

    return std::move(attrs);
}

void
HdPrman_Context::ConvertCategoriesToAttributes(
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

HdPrman_Context::RileyCoordSysIdVecRefPtr
HdPrman_Context::ConvertAndRetainCoordSysBindings(
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
HdPrman_Context::ReleaseCoordSysBindings(SdfPath const& id)
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

static void
_GetShutterInterval(
    HdRenderSettingsMap& renderSettings,
    float* shutterOpen,
    float* shutterClose)
{
    auto const& shutterOpenEntry = 
        renderSettings.find(HdPrmanRenderSettingsTokens->shutterOpen);
    auto const& shutterCloseEntry = 
        renderSettings.find(HdPrmanRenderSettingsTokens->shutterClose);

    if (shutterOpenEntry != renderSettings.end() &&
        shutterCloseEntry != renderSettings.end())
    {
        VtValue shutterOpenVal = shutterOpenEntry->second; 
        VtValue shutterCloseVal = shutterCloseEntry->second; 

        if (shutterOpenVal.IsHolding<float>()) {
            *shutterOpen = shutterOpenVal.UncheckedGet<float>();
        } else if (shutterOpenVal.IsHolding<double>()) {
            double v = shutterOpenVal.UncheckedGet<double>();
            *shutterOpen = static_cast<float>(v);
        }

        if (shutterCloseVal.IsHolding<float>()) {
            *shutterOpen = shutterCloseVal.UncheckedGet<float>();
        } else if (shutterCloseVal.IsHolding<double>()) {
            double v = shutterCloseVal.UncheckedGet<double>();
            *shutterClose = static_cast<float>(v);
        }
    }
}

void
HdPrman_Context::SetOptionsFromRenderSettings(
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
            _SetParamValue(riName, val, RtDetailType::k_constant,
                           TfToken(), options);
        } else {
            // map usd renderSetting to ri option
            if (token == HdPrmanRenderSettingsTokens->pixelAspectRatio) {
                options.SetFloat(RixStr.k_Ri_FormatPixelAspectRatio, 
                                 val.UncheckedGet<float>());

            } else if (token == HdPrmanRenderSettingsTokens->resolution ) {
                auto const& res = val.UncheckedGet<GfVec2i>();
                options.SetIntegerArray(RixStr.k_Ri_FormatResolution, 
                                        &res[0], 2);

            } else if (token == 
                       HdPrmanRenderSettingsTokens->instantaneousShutter ) {
                _instantaneousShutter = val.UncheckedGet<bool>();
            }            

            // TODO: Unhandled settings from schema
            // rel camera
            // token includedPurposes
            // token materialBindingPurposes
            // rel products
            // token aspectRatioConformPolicy (ScreenWindow?)

        }
    }

    float shutterOpen = 0.0f;
    float shutterClose = 0.0f;
    _GetShutterInterval(renderSettings, &shutterOpen, &shutterClose);
    float shutterInterval[2] = { shutterOpen, shutterOpen };

    if (!_instantaneousShutter)
    {
        shutterInterval[1] = shutterClose;
    }
    options.SetFloatArray(RixStr.k_Ri_Shutter, shutterInterval, 2); 
}

void
HdPrman_Context::SetIntegratorParamsFromRenderSettings(
    HdPrmanRenderDelegate *renderDelegate,
    std::string& integratorName,
    RtParamList& params)
{
    static const RtUString us_PxrPathTracer("PxrPathTracer");

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

            _SetParamValue(riName, val, RtDetailType::k_constant,
                           TfToken(), params);
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
        // We need the path to RtxGlfImage and we assume that it lives in the
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

PXR_NAMESPACE_CLOSE_SCOPE
