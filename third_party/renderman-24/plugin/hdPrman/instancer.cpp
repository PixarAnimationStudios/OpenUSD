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
#include "hdPrman/instancer.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/debugUtil.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/quatf.h"
#include "pxr/base/gf/quath.h"
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/envSetting.h"

#include "RiTypesHelper.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_PRMAN_DISABLE_NESTED_INSTANCING, false,
    "disable riley nested instancing in hdprman");

const int HDPRMAN_MAX_SUPPORTED_NESTING_DEPTH = 4;

// **********************************************
// **        Internal helper functions         **
// **********************************************

template <typename T1, typename T2, unsigned int C>
static void _AccumulateSampleTimes(
    const HdTimeSampleArray<T1,C>& in,
    HdTimeSampleArray<T2,C>& out)
{
    // XXX: This is just a straight copy that works fine in situations where
    // out's sample range is within in's. But if out's sample range begins
    // before in's (out.times[0] < in.times[0]) or ends after in's
    // (out.times[-1] > in.times[-1]), we're gonna lose part of the range.
    if (in.count > out.count) {
        out.Resize(in.count);
        out.times = in.times;
    }
}

void _BuildStatsId(
    const SdfPath& instancerId, 
    const int index, 
    const SdfPath& protoId, 
    RtParamList& params)
{
    RtUString val;
    if (params.HasParam(RixStr.k_stats_identifier)) {
        params.GetString(RixStr.k_stats_identifier, val);
        std::string valStr(val.CStr());
        valStr = TfStringReplace(
            valStr, 
            instancerId.GetString(), 
            TfStringPrintf("%s[%i]", instancerId.GetText(), index));
        val = RtUString(valStr.c_str());
    } else {
        std::string valStr = TfStringPrintf(
            "%s[%i]{%s}", 
            instancerId.GetText(), 
            index, 
            protoId.GetName().c_str());
        val = RtUString(valStr.c_str());
    }
    params.SetString(RixStr.k_stats_identifier, val);
}

RtUString
_FixupPrimvarName(const TfToken& name) {
    // Instance primvars with the "ri:attributes:" and
    // "primvars:ri:attributes:" prefixes correspond to renderman-namespace
    // attributes and have that prefix stripped.
    // All other primvars are in the "user:" namespace, so if they don't
    // have that prefix we need to add it.
    static const std::string userPrefix("user:");
    static const std::string riAttrPrefix("ri:attributes:");
    static const std::string primvarsRiAttrPrefix("primvars:ri:attributes:");
    RtUString rtname;
    if (TfStringStartsWith(name.GetString(), userPrefix)) {
        rtname = RtUString(name.GetText());
    } else if (TfStringStartsWith(name.GetString(), riAttrPrefix)) {
        rtname = RtUString(name.GetString()
            .substr(riAttrPrefix.length()).c_str());
    } else if (TfStringStartsWith(name.GetString(), primvarsRiAttrPrefix)) {
        rtname = RtUString(name.GetString()
            .substr(primvarsRiAttrPrefix.length()).c_str());
    } else {
        rtname = RtUString(TfStringPrintf("user:%s", name.GetText()).c_str());
    }
    return rtname;
}

bool
_SetPrimvarValue(
    const RtUString& name,
    const VtValue& val,
    const TfToken& role,
    const bool isConstantRate,
    const size_t instanceIndex,
    RtParamList& dest)
{
    if (val.IsHolding<VtArray<float>>()) {
        const VtArray<float>& v = val.UncheckedGet<VtArray<float>>();
        if (isConstantRate) {
            dest.SetFloatArray(name, v.data(), v.size());
        } else {
            dest.SetFloat(name, v[instanceIndex]);
        }
    } else if (val.IsHolding<float>()) {
        const float v = val.UncheckedGet<float>();
        dest.SetFloat(name, v);
    } else if (val.IsHolding<VtArray<int>>()) {
        const VtArray<int>& v = val.UncheckedGet<VtArray<int>>();
        if (isConstantRate) {
            dest.SetIntegerArray(name, v.data(), v.size());
        } else {
            dest.SetInteger(name, v[instanceIndex]);
        }
    } else if (val.IsHolding<int>()) {
        const int v = val.UncheckedGet<int>();
        dest.SetInteger(name, v);
    } else if (val.IsHolding<VtArray<GfVec2f>>()) {
        const VtArray<GfVec2f>& v = val.UncheckedGet<VtArray<GfVec2f>>();
        if (isConstantRate) {
            dest.SetFloatArray(name, 
                reinterpret_cast<const float*>(v.data()), 2 * v.size());
        } else {
            dest.SetFloatArray(name, 
                reinterpret_cast<const float*>(v[instanceIndex].data()), 2);
        }
    } else if (val.IsHolding<GfVec2f>()) {
        const GfVec2f& v = val.UncheckedGet<GfVec2f>();
        dest.SetFloatArray(name,
            reinterpret_cast<const float*>(v.data()), 2);
    } else if (val.IsHolding<VtArray<GfVec3f>>()) {
        if (isConstantRate) {
            const VtArray<GfVec3f>& v = val.UncheckedGet<VtArray<GfVec3f>>();
            if (role == HdPrimvarRoleTokens->color) {
                dest.SetColorArray(name,
                    reinterpret_cast<const RtColorRGB*>(v.data()), v.size());
            } else if (role == HdPrimvarRoleTokens->point) {
                dest.SetPointArray(name,
                    reinterpret_cast<const RtPoint3*>(v.data()), v.size());
            } else if (role == HdPrimvarRoleTokens->normal) {
                dest.SetNormalArray(name,
                    reinterpret_cast<const RtNormal3*>(v.data()), v.size());
            } else if (role == HdPrimvarRoleTokens->vector) {
                dest.SetVectorArray(name,
                    reinterpret_cast<const RtVector3*>(v.data()), v.size());
            } else {
                dest.SetFloatArray(name,
                    reinterpret_cast<const float*>(v.data()), 3 * v.size());
            }
        } else {
            const GfVec3f& v =
                val.UncheckedGet<VtArray<GfVec3f>>()[instanceIndex];
            if (role == HdPrimvarRoleTokens->color) {
                dest.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
            } else if (role == HdPrimvarRoleTokens->point) {
                dest.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
            } else if (role == HdPrimvarRoleTokens->normal) {
                dest.SetNormal(name, RtNormal3(v[0], v[1], v[2]));
            } else if (role == HdPrimvarRoleTokens->vector) {
                dest.SetVector(name, RtVector3(v[0], v[1], v[2]));
            } else {
                dest.SetFloatArray(
                    name, reinterpret_cast<const float*>(v.data()), 3);
            }
        }
    } else if (val.IsHolding<GfVec3f>()) {
        const GfVec3f& v = val.UncheckedGet<GfVec3f>();
        if (role == HdPrimvarRoleTokens->color) {
            dest.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->point) {
            dest.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->normal) {
            dest.SetNormal(name, RtNormal3(v[0], v[1], v[2]));
        } else if (role == HdPrimvarRoleTokens->vector) {
            dest.SetVector(name, RtVector3(v[0], v[1], v[2]));
        } else {
            dest.SetFloatArray(
                name, reinterpret_cast<const float*>(v.data()), 3);
        }
    } else if (val.IsHolding<VtArray<GfVec4f>>()) {
        const VtArray<GfVec4f>& v = val.UncheckedGet<VtArray<GfVec4f>>();
        if (isConstantRate) {
            dest.SetFloatArray(name,
                reinterpret_cast<const float*>(v.data()), 4 * v.size());
        } else {
            dest.SetFloatArray(name, 
                reinterpret_cast<const float*>(v[instanceIndex].data()), 4);
        }
    } else if (val.IsHolding<GfVec4f>()) {
        const GfVec4f& v = val.UncheckedGet<GfVec4f>();
        dest.SetFloatArray(name,
            reinterpret_cast<const float*>(v.data()), 4);
    } else if (val.IsHolding<VtArray<GfMatrix4d>>()) {
        const VtArray<GfMatrix4d>& v =
            val.UncheckedGet<VtArray<GfMatrix4d>>();
        if (isConstantRate) {
            VtArray<RtMatrix4x4> values;
            values.reserve(v.size());
            std::transform(v.begin(), v.end(), std::back_inserter(values),
            [](const GfMatrix4d& mat){ 
                return HdPrman_GfMatrixToRtMatrix(mat);
            });
            dest.SetMatrixArray(name, values.data(), values.size());
        } else {
            dest.SetMatrix(name,
                HdPrman_GfMatrixToRtMatrix(v[instanceIndex]));
        }
    } else if (val.IsHolding<GfMatrix4d>()) {
        const GfMatrix4d& v = val.UncheckedGet<GfMatrix4d>();
        dest.SetMatrix(name, HdPrman_GfMatrixToRtMatrix(v));
    } else if (val.IsHolding<VtArray<std::string>>()) {
        const VtArray<std::string>& v =
            val.UncheckedGet<VtArray<std::string>>();
        if (isConstantRate) {
            VtArray<RtUString> values;
            values.reserve(v.size());
            std::transform(v.begin(), v.end(), std::back_inserter(values),
            [](const std::string& str){
                return RtUString(str.c_str());
            });
            dest.SetStringArray(name, values.data(), values.size());
        } else {
            dest.SetString(name, RtUString(v[instanceIndex].c_str()));
        }
    } else if (val.IsHolding<std::string>()) {
        const std::string& v = val.UncheckedGet<std::string>();
        dest.SetString(name, RtUString(v.c_str()));
    } else if (val.IsHolding<VtArray<TfToken>>()) {
        const VtArray<TfToken>& v = val.UncheckedGet<VtArray<TfToken>>();
        if (isConstantRate) {
            VtArray<RtUString> values;
            values.reserve(v.size());
            std::transform(v.begin(), v.end(), std::back_inserter(values),
            [](const TfToken& tok) {
                return RtUString(tok.GetText());
            });
            dest.SetStringArray(name, values.data(), values.size());
        } else {
            dest.SetString(name, RtUString(v[instanceIndex].GetText()));
        }
    } else if (val.IsHolding<TfToken>()) {
        const TfToken& v = val.UncheckedGet<TfToken>();
        dest.SetString(name, RtUString(v.GetText()));
    } else {
        return false;
    }
    return true;
}

// **********************************************
// **              Public methods              **
// **********************************************

HdPrmanInstancer::HdPrmanInstancer(
    HdSceneDelegate* delegate,
    SdfPath const& id)
    : HdInstancer(delegate, id)
{ }

HdPrmanInstancer::~HdPrmanInstancer() = default;

HdDirtyBits 
HdPrmanInstancer::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::DirtyTransform |
        HdChangeTracker::DirtyPrimvar |
        HdChangeTracker::DirtyVisibility |
        HdChangeTracker::DirtyInstancer |
        HdChangeTracker::DirtyCategories |
        HdChangeTracker::DirtyInstanceIndex;
};

void
HdPrmanInstancer::Sync(
    HdSceneDelegate* delegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    const SdfPath& id = GetId();

    if (TfDebug::IsEnabled(HDPRMAN_INSTANCERS)) {
        using namespace HdPrmanDebugUtil;

        const std::string clr = GetCallerAsString(TF_CALL_CONTEXT);
        const std::string dbs = HdChangeTracker::StringifyDirtyBits(*dirtyBits);
        const std::string pro = SdfPathVecToString(delegate->
            GetInstancerPrototypes(id));
        
        std::string dps;
        if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
            for (HdInterpolation i = HdInterpolationConstant;
                i != HdInterpolationCount; i = HdInterpolation(i+1)){
                for (const HdPrimvarDescriptor& primvar : 
                    delegate->GetPrimvarDescriptors(id, i)) {
                    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, 
                        primvar.name)) {
                        if (dps.empty()) {
                            dps += "    dirty primvars    : ";
                        } else {
                            dps += "                      : ";
                        }
                        const VtValue& val = delegate->Get(id, primvar.name);
                        dps += TfStringPrintf("(%s) (%s) %s\n",
                            TfEnum::GetName(i).c_str(), 
                            val.GetTypeName().c_str(),
                            primvar.name.GetText());
                    }
                }
            }
        }

        std::string msg;
        msg += TfStringPrintf("*** Sync called on <%s>\n", id.GetText());
        msg += TfStringPrintf("    dirtyBits         : %s\n", dbs.c_str());
        if (!dps.empty()) {
            msg += dps;
        }
        msg += TfStringPrintf("    prototypes        : [%s]\n", pro.c_str());
        msg += TfStringPrintf("    caller            : %s\n", clr.c_str());
        TF_DEBUG(HDPRMAN_INSTANCERS).Msg("%s\n", msg.c_str());
    }

    _UpdateInstancer(delegate, dirtyBits);

    HdPrman_RenderParam *param = static_cast<HdPrman_RenderParam*>(renderParam);

    // Convert (and cache) instancer coordinate systems.
    if (HdPrman_RenderParam::RileyCoordSysIdVecRefPtr convertedCoordSys =
        param->ConvertAndRetainCoordSysBindings(delegate, id)) {
        _coordSysList.count = convertedCoordSys->size();
        _coordSysList.ids = convertedCoordSys->data();
    }

    // cache instance primvars
    _SyncPrimvars(dirtyBits);

    // cache the instancer and instance transforms
    _SyncTransforms(dirtyBits);
    
    // cache the instancer and instance categories
    _SyncCategories(dirtyBits);

    // cache the instancer visibility
    _SyncVisibility(dirtyBits);

    // If anything has changed, internally flag all previously-populated
    // instances as dirty. Since instances are grouped by prototype prim id
    // and Populate gets called one prototype prim at a time, we set a dirty
    // flag for each known prototype prim id. Each gets cleared once Populate
    // has been called with the corresponding prototype prim. This helps avoid
    // unnecessary updates to riley instances in Populate.
    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id) ||
        HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
        *dirtyBits & HdChangeTracker::DirtyCategories ||
        HdChangeTracker::IsVisibilityDirty(*dirtyBits, id) ||
        HdChangeTracker::IsInstanceIndexDirty(*dirtyBits, id)) {
        _SetPrototypesDirty();
    }
}

void
HdPrmanInstancer::Finalize(HdRenderParam *renderParam)
{
    TF_DEBUG(HDPRMAN_INSTANCERS).Msg("*** Finalize called on <%s>\n\n",
        GetId().GetText());
    HdPrman_RenderParam *param = static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley *riley = param->AcquireRiley();

    // Release retained conversions of coordSys bindings.
    param->ReleaseCoordSysBindings(GetId());
    
    // Delete all my riley instances
    _protoMap.iterate([riley](const SdfPath& path, _ProtoMapEntry& entry){
        for (auto rp : entry.map) {
            const _InstanceIdVec& ids = rp.second;
            for (const _RileyInstanceId& ri : ids) {
                if (ri.instanceId != riley::GeometryInstanceId::InvalidId()) {
                    riley->DeleteGeometryInstance(ri.groupId, ri.instanceId);
                }
            }
        }
    });

    // Clear my proto map
    _protoMap.clear();

    // Delete my groups
    _groupMap.iterate([&](const _FlattenData fd, riley::GeometryPrototypeId& gp) {
        if (gp != riley::GeometryPrototypeId::InvalidId()) {
            riley->DeleteGeometryPrototype(gp);
            gp = riley::GeometryPrototypeId::InvalidId();
        }
    });
    
    // Clear my group map
    _groupMap.clear();
}

void HdPrmanInstancer::Populate(
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    const SdfPath& hydraPrototypeId,
    const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
    const riley::CoordinateSystemList& coordSysList,
    const int32_t rileyPrimId,
    const std::vector<riley::MaterialId>& rileyMaterialIds,
    const SdfPathVector& prototypePaths)
{
    // This public Populate signature does not accept the last two arguments
    // that the private _PopulateInstances does; those are only available to 
    // HdPrmanInstancer. This lets us keep their messy types private.
    
    _PopulateInstances(
        renderParam,
        dirtyBits,
        hydraPrototypeId,
        hydraPrototypeId,
        rileyPrototypeIds,
        coordSysList,
        rileyPrimId,
        rileyMaterialIds,
        prototypePaths,
        { },
        { });
}

// **********************************************
// **    Private methods called during Sync    **
// **********************************************

void HdPrmanInstancer::_SyncPrimvars(
    HdDirtyBits* dirtyBits)
{

    // XXX: This method syncs primvars authored on the instancer so they may
    // be applied to the instances. Under Hydra 1.0, only instance-rate primvars
    // are available, and any authored as "varying", "vertex", or "faceVarying"
    // are converted to instance-rate. However, in Hydra 2.0, all interpolation
    // types are available, and none are converted from "varying", "vertex", or
    // "faceVarying" to "instance". So we have to query for each interpolation
    // type to be sure to capture all primvars that should be applied per-
    // instance.
    //
    // The exclusion here of constant and uniform primvars is an open point of
    // controversy insofar as point instancers are concerned. In theory, those
    // should be inherited by the prototypes, which are descendants of the point
    // instancer, and we make an explicit attempt to capture them elsewhere.
    // However, some users have requested the ability to use constant primvars
    // authored on the point instancer to control the renderer-specific behavior
    // of the point instancer itself. Such an approach would violate assumptions
    // about inheritance in USD. The issue remains in discussion both internally
    // and in Github issues.

    // XXX: Primvars authored on native instances are currently missing under
    // Hydra 2.0 and are not captured here or anywhere else.

    HdSceneDelegate* delegate = GetDelegate();
    SdfPath const& id = GetId();

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id)) {
        // Get list of primvar names for each interp mode and cache each one.
        for (HdInterpolation i = HdInterpolationVarying;
            i != HdInterpolationCount; i = HdInterpolation(i+1)) {
            for (const HdPrimvarDescriptor& primvar :
                delegate->GetPrimvarDescriptors(id, i)) {
                // Skip primvars that have special handling elsewhere.
                // The transform primvars are all handled in
                // _SyncTransforms.
                if (primvar.name == HdInstancerTokens->instanceTransform ||
                    primvar.name == HdInstancerTokens->rotate ||
                    primvar.name == HdInstancerTokens->scale ||
                    primvar.name == HdInstancerTokens->translate) {
                    continue;
                }
                if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, primvar.name)) {
                    VtValue value = delegate->Get(id, primvar.name);
                    if (!value.IsEmpty()) {
                        _PrimvarValue& entry = _primvarMap[primvar.name];
                        entry.desc = primvar;
                        std::swap(entry.value, value);
                    }
                }
            }
        }
    }
}

void
HdPrmanInstancer::_SyncTransforms(
    HdDirtyBits* dirtyBits)
{
    HdSceneDelegate* delegate = GetDelegate();
    const SdfPath& id = GetId();

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
            HdInstancerTokens->instanceTransform) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
            HdInstancerTokens->translate) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
            HdInstancerTokens->rotate) ||
        HdChangeTracker::IsPrimvarDirty(*dirtyBits, id,
            HdInstancerTokens->scale)) {

        HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> instancerXform;
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedInstanceXforms;
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedTranslates;
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedRotates;
        HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedScales;
        delegate->SampleInstancerTransform(id, &instancerXform);
        delegate->SamplePrimvar(id, HdInstancerTokens->instanceTransform,
                                &boxedInstanceXforms);
        delegate->SamplePrimvar(id, HdInstancerTokens->translate,
                                &boxedTranslates);
        delegate->SamplePrimvar(id, HdInstancerTokens->scale,
                                &boxedScales);
        delegate->SamplePrimvar(id, HdInstancerTokens->rotate,
                                &boxedRotates);

        // Unbox samples held as VtValues
        HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> instanceXforms;
        HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> translates;
        HdTimeSampleArray<VtQuathArray, HDPRMAN_MAX_TIME_SAMPLES> rotates;
        HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> scales;
        if (!instanceXforms.UnboxFrom(boxedInstanceXforms)) {
            TF_WARN("<%s> instanceTransform did not have expected type matrix4d[]",
                    id.GetText());
        }
        if (!translates.UnboxFrom(boxedTranslates)) {
            TF_WARN("<%s> translate did not have expected type vec3f[]",
                    id.GetText());
        }
        if (!rotates.UnboxFrom(boxedRotates)) {
            TF_WARN("<%s> rotate did not have expected type quath[]",
                    id.GetText());
        }
        if (!scales.UnboxFrom(boxedScales)) {
            TF_WARN("<%s> scale did not have expected type vec3f[]",
                    id.GetText());
        }

        // As a simple resampling strategy, find the input with the max #
        // of samples and use its sample placement.  In practice we expect
        // them to all be the same, i.e. to not require resampling.
        _sa.Resize(0);
        _AccumulateSampleTimes(instancerXform, _sa);
        _AccumulateSampleTimes(instanceXforms, _sa);
        _AccumulateSampleTimes(translates, _sa);
        _AccumulateSampleTimes(scales, _sa);
        _AccumulateSampleTimes(rotates, _sa);

        // Resample inputs and concatenate transformations.
        for (size_t i=0; i < _sa.count; ++i) {
            const float t = _sa.times[i];
            GfMatrix4d xf(1);
            if (instancerXform.count > 0) {
                xf = instancerXform.Resample(t);
            }
            VtMatrix4dArray ixf;
            if (instanceXforms.count > 0) {
                ixf = instanceXforms.Resample(t);
            }
            VtVec3fArray trans;
            if (translates.count > 0) {
                trans = translates.Resample(t);
            }
            VtQuathArray rot;
            if (rotates.count > 0) {
                rot = rotates.Resample(t);
            }
            VtVec3fArray scale;
            if (scales.count > 0) {
                scale = scales.Resample(t);
            }

            size_t size = std::max({ 
                ixf.size(), 
                trans.size(), 
                rot.size(), 
                scale.size()
            });

            // Concatenate transformations.
            VtMatrix4dArray &ma = _sa.values[i];
            ma.resize(size);
            for (size_t j=0; j < size; ++j) {
                ma[j] = xf;
                if (trans.size() > j) {
                    GfMatrix4d t(1);
                    t.SetTranslate(GfVec3d(trans[j]));
                    ma[j] = t * ma[j];
                }
                if (rot.size() > j) {
                    GfMatrix4d r(1);
                    r.SetRotate(GfQuatd(rot[j]));
                    ma[j] = r * ma[j];
                }
                if (scale.size() > j) {
                    GfMatrix4d s(1);
                    s.SetScale(GfVec3d(scale[j]));
                    ma[j] = s * ma[j];
                }
                if (ixf.size() > j) {
                    ma[j] = ixf[j] * ma[j];
                }
            }
        }
    }
}

void
HdPrmanInstancer::_SyncCategories(HdDirtyBits* dirtyBits)
{
    // XXX: Instance categories only exist under native instancing, because
    // point instancer instances are not path-addressable. For point instancers,
    // we want the instances to take the categories of the instancer itself.
    // Ideally, this difference would have been smoothed over for us by the
    // scene delegate, and we would get instance categories for either kind of
    // instancing using GetInstanceCategories(). For point instancers, the
    // delegate would give us an appropriately sized vector of identical
    // category lists pulled from the instancer.
    //
    // Unfortunately, GetInstanceCategories() does not handle point instancing
    // this way. It instead returns an empty vector, leaving it to us to notice
    // and call GetCategories() for the instancer ourselves, something we
    // wouldn't otherwise want to do.
    // 
    // Under point instancing, once we've called GetCategories(), we don't
    // bother copying those categories into _instanceCategories, since we don't
    // really know at this point how many instances we will have; we store them
    // separately in the instancer-level flatten data instead.
    //
    // Under point instancing, _instanceCategories will be an empty vector.
    // Therefore, all indexing into _instanceCategories must be bounds-checked!
    //
    // When we *do* have instance categories (as under native instancing), we
    // make a little optimization by finding any categories common to all
    // instances and moving them to the instancer-level flatten data.

    HdSceneDelegate* delegate = GetDelegate();
    const SdfPath& id = GetId();

    if (*dirtyBits & HdChangeTracker::DirtyCategories) {
        _instancerFlat.categories.clear();
        _instanceCategories = delegate->GetInstanceCategories(id);
        if (_instanceCategories.size() == 0) {
            // Point instancing; use instancer's categories
            VtTokenArray cats = delegate->GetCategories(id);
            _instancerFlat.categories.insert(cats.begin(), cats.end());
        } else {
            // Native instancing; move common categories to instancer
            VtTokenArray intersection;
            for (size_t i = 0; i < _instanceCategories.size(); ++i) {
                VtTokenArray& instCats = _instanceCategories[i];
                // if any instance has no categories there can be no intersection
                if (instCats.size() == 0) {
                    intersection.clear();
                    break;
                }
                std::sort(instCats.begin(), instCats.end());
                VtTokenArray newIntersection;
                if (i == 0) {
                    newIntersection = instCats;
                } else {
                    std::set_intersection(
                        intersection.begin(), intersection.end(),
                        instCats.begin(), instCats.end(),
                        std::back_inserter(newIntersection));
                }
                if (newIntersection.size() == 0) {
                    intersection.clear();
                    break;
                }
                intersection = newIntersection;
            }
            if (intersection.size() > 0) {
                for (size_t i = 0; i < _instanceCategories.size(); ++i) {
                    VtTokenArray instCats = _instanceCategories[i];
                    // already sorted above
                    VtTokenArray newCats;
                    std::set_difference(
                        instCats.begin(), instCats.end(),
                        intersection.begin(), intersection.end(),
                        std::back_inserter(newCats));
                    instCats = newCats;
                }
                _instancerFlat.categories.insert(intersection.begin(), 
                    intersection.end());
            }
        }
    }
}

void
HdPrmanInstancer::_SyncVisibility(HdDirtyBits* dirtyBits)
{
    HdSceneDelegate* delegate = GetDelegate();
    const SdfPath& id = GetId();

    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        if (delegate->GetVisible(id)) {
            _instancerFlat.visibility = RtParamList();
        } else {
            for (const RtUString& attr : _GetVisAttrs()) {
                _instancerFlat.visibility.SetInteger(attr, 0);
            }
        }
    }
}

void
HdPrmanInstancer::_SetPrototypesDirty()
{
    HdPrmanInstancer* parent = _GetParentInstancer();
    if (parent && _Depth() > HDPRMAN_MAX_SUPPORTED_NESTING_DEPTH) {
        parent->_SetPrototypesDirty();
    } else {
        _protoMap.iterate([&](const SdfPath& pp, _ProtoMapEntry& entry) {
            entry.dirty = true;
        });
    }
}

// **********************************************
// **  Private methods called during Populate  **
// **********************************************

void
HdPrmanInstancer::_PopulateInstances(
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    const SdfPath& hydraPrototypeId,
    const SdfPath& prototypePrimPath,
    const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
    const riley::CoordinateSystemList& coordSysList,
    const int32_t rileyPrimId,
    const std::vector<riley::MaterialId>& rileyMaterialIds,
    const SdfPathVector& prototypePaths,
    const std::vector<_InstanceData>& subInstances,
    const std::vector<_FlattenData>& prototypeFlats
)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // This is our main workhorse. This method instructs the instancer to
    // generate riley instances of a given prototype. When coming from the
    // public Populate method, that prototype is a hydra prototype whose riley
    // prototypes have already been constructed (see gprim.h). When coming from
    // a child instancer, that prototype will usually be the child instancer
    // itself (along with its associated riley geometry prototype groups).
    // However, if the child instancer is too deep for nested instancing, it
    // will instead provide the same prototype it was given along with a list
    // of InstanceData structures, which contain all the information about the
    // instances the child would have made if it could have. This instancer must
    // then multiply that list by its own instances, and either create them or
    // pass them along to *its* parent if it is also too deep.
    //
    // Further complicating issues, this method may be called concurrently from
    // multiple threads, so some actions must be gated behind mutex locks.

#if PXR_VERSION <= 2011
    // Sync hydra instancer primvars
    // XXX: Does this still go here?
    SyncPrimvars();
#endif

    HdPrman_RenderParam* param = static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley* riley = param->AcquireRiley();
    HdSceneDelegate* delegate = GetDelegate();
    HdRenderIndex& renderIndex = delegate->GetRenderIndex();
    const SdfPath& instancerId = GetId();
    HdPrmanInstancer* parentInstancer = _GetParentInstancer();
    bool instancesNeedUpdate = _protoMap.get(prototypePrimPath).dirty;
    bool anyGroupIdChanged = false;

    if (TfDebug::IsEnabled(HDPRMAN_INSTANCERS)) {
        using namespace HdPrmanDebugUtil;

        std::string pid = " -none- ";
        if (parentInstancer) {
            pid = parentInstancer->GetId().GetAsString();
        }

        const VtIntArray instanceIndices = delegate->GetInstanceIndices(
            instancerId, hydraPrototypeId);

        std::string ins = TfStringPrintf("%lu instances ", instanceIndices.size());
        size_t total = instanceIndices.size();
        if (subInstances.size() > 0) {
            ins += TfStringPrintf("of %lu subInstances ", subInstances.size());
            total *= subInstances.size();
        }
        ins += TfStringPrintf("of %lu prototypes ", rileyPrototypeIds.size());
        total *= rileyPrototypeIds.size();
        ins += TfStringPrintf("= %lu Riley instances", total);
    
        const HdDirtyBits instDirtyBits = renderIndex.GetChangeTracker()
            .GetInstancerDirtyBits(instancerId);
        const std::string dbs = HdChangeTracker::StringifyDirtyBits(*dirtyBits);
        const std::string idb = HdChangeTracker::StringifyDirtyBits(instDirtyBits);
        const std::string pro = RileyIdVecToString(rileyPrototypeIds);
        const std::string mid = RileyIdVecToString(rileyMaterialIds);
        const std::string pps = SdfPathVecToString(prototypePaths);
        const std::string clr = GetCallerAsString(TF_CALL_CONTEXT);

        std::string str;
        str += TfStringPrintf("*** Populate called on <%s>\n", instancerId.GetText());
        str += TfStringPrintf("    dirtyBits         : %s\n", dbs.c_str());
        str += TfStringPrintf("    instDirtyBits     : %s\n", idb.c_str());
        str += TfStringPrintf("    hydraPrototypeId  : <%s>\n", hydraPrototypeId.GetText());
        str += TfStringPrintf("    prototypePrimPath : <%s>\n", prototypePrimPath.GetText());
        str += TfStringPrintf("    rileyPrototypeIds : (%s)\n", pro.c_str());
        str += TfStringPrintf("    rileyMaterialIds  : (%s)\n", mid.c_str());
        str += TfStringPrintf("    prototypePaths    : [%s]\n", pps.c_str());
        str += TfStringPrintf("    instances         : %lu\n", instanceIndices.size());
        str += TfStringPrintf("    subInstances      : %lu\n", subInstances.size());
        str += TfStringPrintf("    total instances   : %s\n", ins.c_str());
        str += TfStringPrintf("    parentInstancer   : <%s>\n", pid.c_str());
        str += TfStringPrintf("    depth             : %i\n", _Depth());
        str += TfStringPrintf("    caller            : %s\n", clr.c_str());
        TF_DEBUG(HDPRMAN_INSTANCERS).Msg("%s\n", str.c_str());
    }

    TF_VERIFY(
        rileyMaterialIds.size() == rileyPrototypeIds.size(), 
        "rileyMaterialIds size mismatch: %lu != %lu", 
        rileyMaterialIds.size(), rileyPrototypeIds.size());
    TF_VERIFY(
        prototypePaths.size() == rileyPrototypeIds.size(),
        "prototypePaths size mismatch: %lu != %lu",
        prototypePaths.size(), rileyPrototypeIds.size());
    TF_VERIFY(
        prototypeFlats.size() == 0 || 
        prototypeFlats.size() == rileyPrototypeIds.size(),
        "prototypeFlats size mismatch: %lu != %lu",
        prototypeFlats.size(), rileyPrototypeIds.size());

    instancesNeedUpdate = _RemoveDeadInstances(riley, prototypePrimPath,
        rileyPrototypeIds) || instancesNeedUpdate;

    std::vector<_InstanceData> instances;

    // hydraPrototypeId corresponds to the hydra prototype, which might be a
    // child instancer. prototypePrimPath corresponds to the prim represented by
    // the rileyPrototypeIds, which could be a child instancer or prototype
    // geometry. In most cases, they are equal:
    //  1. Both paths are the same, and point to a geometry prim that is one of
    //     this instancer's prototypes and has already been represented in riley
    //     during GPrim::Sync(), resulting in the ids in rileyPrototypeIds which
    //     are all of a non-group primType; or
    //  2. Both paths are the same, and point to a child instancer prim that is
    //     this instancer's prototype and has already been represented in riley
    //     by the child instancer, resulting in the ids in rileyPrototypeIds
    //     which are all of primType "Group".
    // They differ only when the prototype is an instancer and that instancer
    // was too deep to put its riley instances into a group. In that case, 
    // hydraPrototypeId will be the instancer below, while prototypePrimPath 
    // will be the prototype prim path originally given to Populate, because
    // that's the prim represented by the rileyPrototypeIds we've been given.
    // We need the former to retrieve instance indices, attributes, and
    // transforms for the nested instancer prototype, while we use the latter to
    // track our riley instances and their prototype ids in _protoMap and
    // retrieve primvars affecting the prototype.
    
    // We might receive no instances from the too-deep nested instacer, because
    // the nested instancer no longer has any instances of this prototype. In
    // that case, we should not make any instances of this prototype here, and
    // delete any we already have.

    if (hydraPrototypeId == prototypePrimPath || subInstances.size() > 0) {
        _ComposeInstances(hydraPrototypeId, rileyPrimId, subInstances, instances);
    }

    // TODO: if depth *decreases*, how will no-longer-too-deep-child signal its
    // parent to release the flattened instances?

    // Check for full flattening based on env setting or depth
    if (parentInstancer &&
        (TfGetEnvSetting(HD_PRMAN_DISABLE_NESTED_INSTANCING) ||
            _Depth() > HDPRMAN_MAX_SUPPORTED_NESTING_DEPTH)) {
        
        // Ensure the protoMap is clear of instances (perhaps depth increased?)
        _ResizeProtoMap(riley, prototypePrimPath, rileyPrototypeIds, 0);

        // Send allInstances up to the parent to populate
        parentInstancer->_PopulateInstances(
            renderParam,
            dirtyBits,
            instancerId,
            prototypePrimPath,
            rileyPrototypeIds,
            coordSysList, 
            0,
            rileyMaterialIds,
            prototypePaths,
            instances,
            { }
        );
        return;
    }

    if (instancesNeedUpdate) {
        // Allocate the protoMap; this deletes instances if instances is empty
        _ResizeProtoMap(riley, prototypePrimPath, rileyPrototypeIds, instances.size());
    }

    if (instancesNeedUpdate && instances.size() > 0) {

        std::vector<RtParamList> protoAttrs;
        std::vector<_FlattenData> protoFlats;
        _GfMatrixSA protoXform;

        _ComposePrototypeData(
            param,
            prototypePrimPath,
            rileyPrototypeIds, 
            prototypePaths,
            prototypeFlats,
            protoAttrs, protoFlats, protoXform);

        for (size_t i = 0; i < instances.size(); ++i) {
            const _InstanceData& instance = instances[i];
            _RtMatrixSA xform;
            _MultiplyTransforms(protoXform, instance.transform, xform);
            const riley::Transform rileyXform = {
                unsigned(xform.count),
                xform.values.data(),
                xform.times.data()
            };

            for (size_t j = 0; j < rileyPrototypeIds.size(); ++j) {
                const riley::GeometryPrototypeId& protoId = 
                    rileyPrototypeIds[j];
                riley::MaterialId matId = rileyMaterialIds[j];
                RtParamList attrs = instance.params;
                attrs.Update(protoAttrs[j]);
                
                // append subset name to stats:identifier
                if (prototypePaths[j] != hydraPrototypeId) {
                    std::string protoName = TfStringPrintf(
                        "{%s}", prototypePaths[j].GetName().c_str());
                    RtUString sid;
                    if (attrs.GetString(RixStr.k_stats_identifier, sid)) {
                        std::string sidStr(sid.CStr());
                        if (sidStr.find(protoName) == sidStr.npos) {
                            sid = RtUString(
                                (std::string(sid.CStr()) + protoName).c_str());
                            attrs.SetString(RixStr.k_stats_identifier, sid);
                        }
                    }
                }

                // compose the final flats
                _FlattenData flats = instance.flattenData;
                flats.Inherit(protoFlats[j]);

                // acquire the group id
                riley::GeometryPrototypeId groupId;
                anyGroupIdChanged |= _AcquireGroupId(param, flats, groupId);

                // fold the flats back into attrs if no parent instancer
                if (!parentInstancer) {
                    // we use Update because these should not exist in attrs yet
                    attrs.Update(flats.visibility);
                    param->ConvertCategoriesToAttributes(
                        instancerId,
                        { flats.categories.begin(), flats.categories.end() },
                        attrs);
                }
                
                _RileyInstanceId& instIds = _protoMap.get(prototypePrimPath)
                    .map[protoId][i];
                if (instIds.instanceId != riley::GeometryInstanceId::InvalidId()
                    && instIds.groupId != groupId) {
                    // the instanceId is valid but the groupId is not; delete it
                    riley->DeleteGeometryInstance(
                        instIds.groupId, instIds.instanceId);
                    instIds.instanceId = riley::GeometryInstanceId::InvalidId();
                }

                instIds.groupId = groupId;

                if (instIds.instanceId == riley::GeometryInstanceId::InvalidId()) {
                    RtUString name;
                    attrs.GetString(RixStr.k_identifier_name, name);
                    riley::UserId userId = riley::UserId(
                        stats::AddDataLocation(name.CStr()).GetValue());
                    instIds.instanceId = riley->CreateGeometryInstance(
                        userId,
                        instIds.groupId, 
                        protoId, 
                        matId, 
                        coordSysList, 
                        rileyXform, 
                        attrs);
                } else if (*dirtyBits) {
                    riley->ModifyGeometryInstance(
                        instIds.groupId,
                        instIds.instanceId,
                        &matId,
                        &coordSysList,
                        &rileyXform,
                        &attrs);
                }
            }
        }
        _protoMap.get(prototypePrimPath).dirty = false;
    }

    // clean up disused prototype groups
    anyGroupIdChanged |= _CleanDisusedGroupIds(param);

    if (parentInstancer && (anyGroupIdChanged || 
        HdChangeTracker::IsInstancerDirty(*dirtyBits, instancerId))) {
        // tell parent to make instances of my groups
        // (my groups, my groups, my lovely proto groups)

        std::vector<_FlattenData> flats;
        std::vector<riley::GeometryPrototypeId> ids;
        std::vector<riley::MaterialId> mats;
        SdfPathVector paths;
        _groupMap.iterate([&](const _FlattenData& fd, riley::GeometryPrototypeId& gp) {
            flats.push_back(fd);
            ids.push_back(gp);
            paths.push_back(instancerId);
            mats.push_back(riley::MaterialId::InvalidId());
        });
        parentInstancer->_PopulateInstances(
            renderParam,
            dirtyBits,
            instancerId,
            instancerId,
            ids,
            coordSysList,
            0,
            mats,
            paths,
            { },
            flats);
    }
}

void
HdPrmanInstancer::_ComposeInstances(
    const SdfPath& protoId,
    const int primId,
    const std::vector<_InstanceData> subInstances,
    std::vector<_InstanceData>& instances)
{
    // XXX: Using riley nested instancing breaks selection. Selection depends on
    // enumerating every instance of a given hydra geometry prototype prim with
    // a unique id and setting that id in riley as identifier:id2. When using
    // riley prototype groups, there is no longer a 1:1 correspondence between
    // hydra instances of a given prototype and riley instances. If instance
    // picking and selection are required, users should disable riley nested
    // instancing by setting HD_PRMAN_DISABLE_NESTED_INSTANCING=1. In future,
    // we may consider adding an instancer id AOV to the picking and selection
    // flow to support precise instance disambiguation.

    HdSceneDelegate* delegate = GetDelegate();
    const SdfPath& id = GetId();
    const VtIntArray indices = delegate->GetInstanceIndices(id, protoId);
    instances.clear();
    if (subInstances.empty()) {
        instances.resize(indices.size());
        for (size_t i = 0; i < indices.size(); ++i) {
            const int index = indices[i];
            _InstanceData& instance = instances[i];
            _GetInstancePrimvars(index, instance.params);
            if (primId > 0) {
                instance.params.SetInteger(RixStr.k_identifier_id, primId);
            }
            instance.params.SetInteger(RixStr.k_identifier_id2, int(i));
            _BuildStatsId(id, index, protoId, instance.params);
            _ComposeInstanceFlattenData(index, instance.params, instance.flattenData);
            _GetInstanceTransform(index, instance.transform);
        }
    } else {
        instances.resize(indices.size() * subInstances.size());
        // XXX: Iteration order is critical to selection. identifier:id2 must
        // increment in subInstance-major order. So we slow-iterate through
        // this level's instances and fast-iterate through the subInstances.
        for (size_t i = 0; i < indices.size(); ++i) {
            const int index = indices[i];
            for (size_t si = 0; si < subInstances.size(); ++si) {
                const _InstanceData& subInstance  = subInstances[si];
                const int ii = i * subInstances.size() + si;
                _InstanceData& instance = instances[ii];
                _GetInstancePrimvars(index, instance.params);
                instance.params.Update(subInstance.params);
                instance.params.SetInteger(RixStr.k_identifier_id2, int(ii));
                _BuildStatsId(id, index, protoId, instance.params);
                _ComposeInstanceFlattenData(
                    index, 
                    instance.params, 
                    instance.flattenData, 
                    subInstance.flattenData);
                _GetInstanceTransform(
                    index,
                    instance.transform,
                    subInstance.transform);
            }
        }
    }
}

void
HdPrmanInstancer::_ComposeInstanceFlattenData(
    const size_t instanceId,
    RtParamList& instanceParams,
    _FlattenData& fd,
    const _FlattenData& fromBelow)
{
    _FlattenData instance;
    if (instanceId < _instanceCategories.size()) {
        instance = _FlattenData(_instanceCategories[instanceId]);
    }

    // Capture fine-grained visibility that may have been authored on the point
    // instancer as instance-varying primvars or the native instance; remove
    // these from instanceParams if they exist.
    for (const RtUString& visAttr : _GetVisAttrs()) {
        if (instanceParams.HasParam(visAttr)) {
            int val;
            instanceParams.GetInteger(visAttr, val);
            instance.visibility.SetInteger(visAttr, val);
            instanceParams.Remove(visAttr);
        }
    }
    
    fd.Update(_instancerFlat);
    fd.Update(instance);
    fd.Update(fromBelow);
}

bool
HdPrmanInstancer::_RemoveDeadInstances(
    riley::Riley* riley,
    const SdfPath& prototypePrimPath,
    const std::vector<riley::GeometryPrototypeId>& protoIds
)
{
    using ProtoMapPair = std::pair<riley::GeometryPrototypeId, _InstanceIdVec>;
    _ProtoInstMap& protoMap = _protoMap.get(prototypePrimPath).map;
    std::vector<riley::GeometryPrototypeId> oldProtoIds;
    std::transform(
        protoMap.begin(), protoMap.end(),
        std::back_inserter(oldProtoIds),
        [](const ProtoMapPair& pair) { return pair.first; });
    std::vector<riley::GeometryPrototypeId> newProtoIds(
            protoIds.begin(), protoIds.end());

    std::sort(oldProtoIds.begin(), oldProtoIds.end());
    std::sort(newProtoIds.begin(), newProtoIds.end());

    std::vector<riley::GeometryPrototypeId> toRemove;
    std::set_difference(
        oldProtoIds.begin(), oldProtoIds.end(),
        protoIds.begin(), protoIds.end(),
        std::back_inserter(toRemove));
    if (toRemove.size() > 0) {
        _ResizeProtoMap(riley, prototypePrimPath, toRemove, 0);
    }

    // returns true if there are new geometry prototype ids for this prototype
    std::vector<riley::GeometryPrototypeId> toAdd;
    std::set_difference(
        protoIds.begin(), protoIds.end(),
        oldProtoIds.begin(), oldProtoIds.end(),
        std::back_inserter(toAdd));
    return toAdd.size() > 0;
}

void
HdPrmanInstancer::_ComposePrototypeData(
    HdPrman_RenderParam* param,
    const SdfPath& protoPath,
    const std::vector<riley::GeometryPrototypeId>& protoIds,
    const SdfPathVector& subProtoPaths,
    const std::vector<_FlattenData>& subProtoFlats,
    std::vector<RtParamList>& protoAttrs,
    std::vector<_FlattenData>& protoFlats,
    _GfMatrixSA& protoXform
)
{
    HdSceneDelegate* delegate = GetDelegate();
    HdRenderIndex& renderIndex = delegate->GetRenderIndex();
    bool isGeometry = !!(renderIndex.GetRprim(protoPath));

    protoAttrs.resize(protoIds.size());
    protoFlats.resize(protoIds.size());
    delegate->SampleTransform(protoPath, &protoXform);

    auto SetProtoAttrs = [&](
        const SdfPath& protoPath,
        RtParamList& attrs,
        _FlattenData& flats)
    {
        attrs = param->ConvertAttributes(delegate, protoPath, isGeometry);

        // get any constant or uniform primvars on or inherited by the prototype
        _GetPrototypePrimvars(protoPath, attrs);
        
        const VtTokenArray& cats = delegate->GetCategories(protoPath);
        flats.categories.insert(cats.begin(), cats.end());
        for (const RtUString& attr : _GetLightLinkAttrs()) {
            attrs.Remove(attr);
        }
        int val;
        for (const RtUString& attr : _GetVisAttrs()) {
            if (attrs.GetInteger(attr, val)) {
                if (val == 0) {
                    flats.visibility.SetInteger(attr, val);
                }
                attrs.Remove(attr);
            }
        }
    };

    for (size_t i = 0; i < protoIds.size(); ++i) {
        RtParamList& attrs = protoAttrs[i];
        _FlattenData& flats = protoFlats[i];

        SetProtoAttrs(protoPath, attrs, flats);

        // If prototype is a subset, also get the subset attrs. While geom
        // subsets should not have primvars on them, they may be the targets
        // of light linking.
        if (i < subProtoPaths.size() && subProtoPaths[i] != protoPath) {
            RtParamList subsetAttrs;
            _FlattenData subsetFlats;
            SetProtoAttrs(subProtoPaths[i], subsetAttrs, subsetFlats);
            attrs.Update(subsetAttrs);
            flats.Update(subsetFlats);
        }

        // Combine any flats received from below for this prototype.
        if (i < subProtoFlats.size()) {
            flats.Update(subProtoFlats[i]);
        }
    }
}

void
HdPrmanInstancer::_ResizeProtoMap(
    riley::Riley* riley,
    const SdfPath& prototypePrimPath, 
    const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds, 
    const size_t newSize
)
{
    _ProtoInstMap& protoInstMap = _protoMap.get(prototypePrimPath).map;
    for (riley::GeometryPrototypeId protoId : rileyPrototypeIds) {
        _InstanceIdVec& instIdVec = protoInstMap[protoId];
        const size_t oldSize = instIdVec.size();
        for (size_t i = newSize; i < oldSize; ++i) {
            const _RileyInstanceId& ids = instIdVec[i];
            if (ids.instanceId != riley::GeometryInstanceId::InvalidId()) {
                riley->DeleteGeometryInstance(ids.groupId, ids.instanceId);
            }
        }
        if (oldSize != newSize) {
            instIdVec.resize(newSize);
        }
        if (newSize == 0) {
            protoInstMap.erase(protoId);
        }
    }
    if (protoInstMap.size() == 0) {
        _protoMap.erase(prototypePrimPath);
    }
}

bool
HdPrmanInstancer::_CleanDisusedGroupIds(HdPrman_RenderParam* param)
{
    std::lock_guard<std::mutex> lock(_groupIdAcquisitionLock);
    riley::Riley* riley = param->AcquireRiley();
    std::unordered_set<riley::GeometryPrototypeId, _ProtoIdHash> active;
    _protoMap.iterate([&](const SdfPath& hp, _ProtoMapEntry& pi){
        for (const auto& rp : pi.map) {
            for (const auto& i : rp.second) {
                if (i.groupId != riley::GeometryPrototypeId::InvalidId()) {
                    active.insert(i.groupId);
                }
            }
        }
    });
    
    std::vector<_FlattenData> removed;
    _groupMap.iterate([&](const _FlattenData& fd, riley::GeometryPrototypeId& gp){
        if (active.find(gp) == active.end()) {
            // there are no longer any instances of any prototypes that use
            // this set of flatten data; kill the group.
            if (gp != riley::GeometryPrototypeId::InvalidId()) {
                riley->DeleteGeometryPrototype(gp);
                gp = riley::GeometryPrototypeId::InvalidId();
            }
            removed.push_back(fd);
        }
    });
    for (const auto& fl : removed) {
        _groupMap.erase(fl);
    }
    // return true if any were deleted
    return removed.size() > 0;
}

bool
HdPrmanInstancer::_AcquireGroupId(
    HdPrman_RenderParam* param,
    const _FlattenData& flattenGroup,
    riley::GeometryPrototypeId& groupId
)
{
    // This lock prevents simultaneous calls to Populate from creating separate
    // riley groups for the same set of flatten data.
    std::lock_guard<std::mutex> lock(_groupIdAcquisitionLock);
    bool changed = false;
    if (_GetParentInstancer() && _Depth() < 5) {
        
        // We use the flatten data to look up whether this instancer has
        // a riley group that it will use for all instances across all
        // prototypes that will share that flatten data. When this instancer
        // tells its parent instancer to make instances of its riley prototype
        // groups, it will also tell the parent to set the params that make up
        // the flatten data on those instances (or pass them along to *its*
        // parent if it is not the outermost instancer). Using the hashable
        // FlattenData structure as the key to identify a group id allows us to
        // take maximal advantage of prman's support for nested instancing while
        // still preserving any incompatable params we may encounter.
        //
        // In short, If the instancer detects variability in the flatten data
        // across its instances, it will put them into separate buckets.

        riley::GeometryPrototypeId& id = _groupMap.get(flattenGroup);
        if (id == riley::GeometryPrototypeId::InvalidId()) {
            RtPrimVarList groupPrimvars;
            groupPrimvars.SetString(RixStr.k_stats_prototypeIdentifier,
                RtUString(GetId().GetText()));
            id = param->AcquireRiley()->CreateGeometryPrototype(
                riley::UserId(stats::AddDataLocation(GetId().GetText()).GetValue()),
                RixStr.k_Ri_Group,
                riley::DisplacementId::InvalidId(),
                groupPrimvars
            );
            _groupMap.set(flattenGroup, id);
            changed = true;
        }
        groupId = id;
    }
    return changed;
}

HdPrmanInstancer*
HdPrmanInstancer::_GetParentInstancer()
{
    // XXX: There is no way of knowing at this stage whether a native instancer
    // is part of a prototype of another instancer, and thus no way to access
    // the parent instancer for a native instancing-backed HdInstancer. This
    // will always return nullptr under native instancing, so native instancing
    // always produces full flattening in riley and takes no advantage of
    // nesting.
    //
    // Note that it is possible for instancers to have multiple parent
    // instancers! UsdImaging currently hides this behind instancer id munging
    // for point instancers, while native instancers do not propagate parent
    // data to hydra at all, so for now we assume only a single parent.

    HdSceneDelegate* delegate = GetDelegate();
    HdRenderIndex& renderIndex = delegate->GetRenderIndex();
    SdfPath parentId = GetParentId();
    while (!parentId.IsEmpty() && !parentId.IsAbsoluteRootPath()) {
        HdPrmanInstancer* instancer = static_cast<HdPrmanInstancer*>(
            renderIndex.GetInstancer(parentId));
        if (instancer) {
            return instancer;
        } else {
            parentId = parentId.GetParentPath();
        }
    }
    return nullptr;
}

int
HdPrmanInstancer::_Depth()
{
    // XXX: Since there is no way to tell if a native instancer has any parent
    // instancers, this will always return depth 0 for native instancing. Also
    // note that while it is technically possible for an instancer to have
    // multiple parent instancers, and multple depths along various paths to
    // its outermost ancestor instancers, assuming a single depth works for now
    // thanks to limitations imposed by UsdImaging.

    int depth = 0;
    HdPrmanInstancer* parent = _GetParentInstancer();
    while (parent) {
        depth++;
        parent = parent->_GetParentInstancer();
    }
    return depth;
}

void
HdPrmanInstancer::_GetInstancePrimvars(
    const size_t instanceIndex,
    RtParamList& attrs)
{
    for (auto entry: _primvarMap) {
        const HdPrimvarDescriptor& primvar = entry.second.desc;
        
        // 'constant' and 'uniform' primvars are inherited in toto by instances,
        // while 'varying', 'vertex', and 'faceVarying' primvars (and those
        // marked as instance-rate by hydra) are inherited per instance by
        // indexing into the value array. See https://tinyurl.com/hdxya2yk.

        bool isConstantRate = primvar.interpolation == HdInterpolationConstant
            || primvar.interpolation == HdInterpolationUniform;

        // Confirm that instance-rate primvars are array-valued
        // and have sufficient dimensions.
        const VtValue& val = entry.second.value;
        if ((!isConstantRate) && (instanceIndex >= val.GetArraySize())) {
            TF_WARN("HdPrman: Instance-rate primvar has array size %zu; "
                    "cannot provide a value for instance index %zu\n",
                    val.GetArraySize(), instanceIndex);
            continue;
        }

        // Instance primvars with the "ri:attributes:" and
        // "primvars:ri:attributes:" prefixes correspond to renderman-namespace
        // attributes and have that prefix stripped.
        // All other primvars are in the "user:" namespace, so if they don't
        // have that prefix we need to add it.
        const RtUString name = _FixupPrimvarName(entry.first);
        
        // ri:attributes and primvars:ri:attributes primvars end up having
        // the same name, potentially causing collisions in the primvar list.
        // When both ri:attributes and primvar:ri:attributes versions of 
        // the same primvars exist, the primvar:ri:attributes version should
        // win out.
        if (TfStringStartsWith(entry.first.GetString(), "ri:attributes:") &&
            attrs.HasParam(name)) {
            continue;
        }

        if (!_SetPrimvarValue(name, val, primvar.role, isConstantRate,
            instanceIndex, attrs)) {
            TF_WARN("Unrecognized primvar value type at %s.%s", 
                GetId().GetText(), entry.first.GetText());
        }
    }
}

void
HdPrmanInstancer::_GetPrototypePrimvars(
    const SdfPath& protoPath,
    RtParamList& attrs)
{
    // XXX: With the scene index enabled (Hydra 2.0), this fails to find
    // constant inherited primvars, but picks up displayColor and
    // displayOpacity, even when those are not authored anywhere on or
    // above the target prototype.

    HdSceneDelegate* delegate = GetDelegate();
    // Only get constant and uniform primvars
    for (HdInterpolation i = HdInterpolationConstant;
        i < HdInterpolationVarying; i = HdInterpolation(i+1)) {
        for (const HdPrimvarDescriptor& primvar :
            delegate->GetPrimvarDescriptors(protoPath, i)) {
            const RtUString name = _FixupPrimvarName(primvar.name);
            if (TfStringStartsWith(primvar.name.GetString(), "ri:attributes") &&
                attrs.HasParam(name)) {
                continue;
            }
            const VtValue& val = delegate->Get(protoPath, primvar.name);
            if (!_SetPrimvarValue(name, val, primvar.role, true, 0, attrs)) {
                TF_WARN("Unrecognized primvar value type at %s.%s",
                    protoPath.GetText(), primvar.name.GetText());
            }
        }
    }
}

void
HdPrmanInstancer::_GetInstanceTransform(
    const size_t instanceIndex,
    _GfMatrixSA& xform,
    const _GfMatrixSA& left)
{
    if (_sa.count > 0 && instanceIndex < _sa.values[0].size()) {
        if (left.count > 0) {
            _GfMatrixSA right;
            _AccumulateSampleTimes(_sa, right);
            for (size_t i = 0; i < right.count; ++i) {
                right.values[i] = _sa.values[i][instanceIndex];
            }
            _AccumulateSampleTimes(left, xform);
            _AccumulateSampleTimes(right, xform);
            for (size_t i = 0; i < xform.count; ++i) {
                const float t = xform.times[i];
                xform.values[i] = left.Resample(t) * right.Resample(t);
            }
        } else {
            _AccumulateSampleTimes(_sa, xform);
            for (size_t i = 0; i < xform.count; ++i) {
                xform.values[i] = _sa.values[i][instanceIndex];
            }
        }
    }
}

/* static */
void
HdPrmanInstancer::_MultiplyTransforms(
    const _GfMatrixSA& lhs,
    const _GfMatrixSA& rhs,
    _RtMatrixSA& dest)
{
    _AccumulateSampleTimes(lhs, dest);
    _AccumulateSampleTimes(rhs, dest);
    if (lhs.count == 0 || (lhs.count == 1 && lhs.values[0] == GfMatrix4d(1))) {
        for (size_t j = 0; j < dest.count; ++j) {
            dest.values[j] = HdPrman_GfMatrixToRtMatrix(rhs.Resample(dest.times[j]));
        }
    } else if (rhs.count == 0 || (rhs.count == 1 && rhs.values[0] == GfMatrix4d(1))) {
        for (size_t j = 0; j < dest.count; ++j) {
            dest.values[j] = HdPrman_GfMatrixToRtMatrix(lhs.Resample(dest.times[j]));
        }
    } else {
        for (size_t j = 0; j < dest.count; ++j) {
            GfMatrix4d lhj = lhs.Resample(dest.times[j]);
            GfMatrix4d rhj = rhs.Resample(dest.times[j]);
            dest.values[j] = HdPrman_GfMatrixToRtMatrix(lhj * rhj);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
