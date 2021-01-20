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
#include "hdPrman/instancer.h"
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

#include "RtParamList.h"

PXR_NAMESPACE_OPEN_SCOPE


HdPrmanInstancer::HdPrmanInstancer(HdSceneDelegate* delegate,
                                     SdfPath const& id)
    : HdInstancer(delegate, id)
{
}

HdPrmanInstancer::~HdPrmanInstancer()
{
}

void
HdPrmanInstancer::Sync(HdSceneDelegate* delegate,
                       HdRenderParam* renderParam,
                       HdDirtyBits* dirtyBits)
{
    _UpdateInstancer(delegate, dirtyBits);

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, GetId())) {
        _SyncPrimvars(delegate, *dirtyBits);
    }
}

void
HdPrmanInstancer::_SyncPrimvars(HdSceneDelegate *delegate,
                                HdDirtyBits dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    SdfPath const& id = GetId();

    // Get the list of primvar names and then cache each one.
    for (HdPrimvarDescriptor const& primvar:
            delegate->GetPrimvarDescriptors(id, HdInterpolationInstance)) {
        // Skip primvars that have special handling elsewhere.
        // The transform primvars are all handled in
        // SampleInstanceTransform.
        if (primvar.name == HdInstancerTokens->instanceTransform ||
                primvar.name == HdInstancerTokens->rotate ||
                primvar.name == HdInstancerTokens->scale ||
                primvar.name == HdInstancerTokens->translate) {
            continue;
        }
        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, primvar.name)) {
            VtValue value = delegate->Get(id, primvar.name);
            if (!value.IsEmpty()) {
                _PrimvarValue &entry = _primvarMap[primvar.name];
                entry.desc = primvar;
                std::swap(entry.value, value);
            }
        }
    }
}

// Helper to accumulate sample times from the largest set of
// samples seen, up to maxNumSamples.
template <typename T1, typename T2, unsigned int C>
static void
_AccumulateSampleTimes(HdTimeSampleArray<T1,C> const& in,
                       HdTimeSampleArray<T2,C> *out)
{
    if (in.count > out->count) {
        out->Resize(in.count);
        out->times = in.times;
    }
}

void
HdPrmanInstancer::SampleInstanceTransforms(
    SdfPath const& prototypeId,
    VtIntArray const& instanceIndices,
    HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> *sa)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSceneDelegate *delegate = GetDelegate();
    const SdfPath &instancerId = GetId();

    // Sample the inputs
    HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> instancerXform;
    HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedInstanceXforms;
    HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedTranslates;
    HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedRotates;
    HdTimeSampleArray<VtValue, HDPRMAN_MAX_TIME_SAMPLES> boxedScales;
    delegate->SampleInstancerTransform(instancerId, &instancerXform);
    delegate->SamplePrimvar(instancerId, HdInstancerTokens->instanceTransform,
                            &boxedInstanceXforms);
    delegate->SamplePrimvar(instancerId, HdInstancerTokens->translate,
                            &boxedTranslates);
    delegate->SamplePrimvar(instancerId, HdInstancerTokens->scale,
                            &boxedScales);
    delegate->SamplePrimvar(instancerId, HdInstancerTokens->rotate,
                            &boxedRotates);

    // Unbox samples held as VtValues
    HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> instanceXforms;
    HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> translates;
    HdTimeSampleArray<VtQuathArray, HDPRMAN_MAX_TIME_SAMPLES> rotates;
    HdTimeSampleArray<VtVec3fArray, HDPRMAN_MAX_TIME_SAMPLES> scales;
    instanceXforms.UnboxFrom(boxedInstanceXforms);
    translates.UnboxFrom(boxedTranslates);
    rotates.UnboxFrom(boxedRotates);
    scales.UnboxFrom(boxedScales);

    // As a simple resampling strategy, find the input with the max #
    // of samples and use its sample placement.  In practice we expect
    // them to all be the same, i.e. to not require resampling.
    sa->Resize(0);
    _AccumulateSampleTimes(instancerXform, sa);
    _AccumulateSampleTimes(instanceXforms, sa);
    _AccumulateSampleTimes(translates, sa);
    _AccumulateSampleTimes(scales, sa);
    _AccumulateSampleTimes(rotates, sa);

    // Resample inputs and concatenate transformations.
    //
    // XXX:PERFORMANCE: This currently samples the transform arrays for
    // all indices.  We should only do this work for the instances
    // indicated in the instanceIndices array.
    //
    for (size_t i=0; i < sa->count; ++i) {
        const float t = sa->times[i];
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

        // Concatenate transformations and filter to just the instanceIndices.
        VtMatrix4dArray &ma = sa->values[i];
        ma.resize(instanceIndices.size());
        for (size_t j=0; j < instanceIndices.size(); ++j) {
            ma[j] = xf;
            size_t instanceIndex = instanceIndices[j];
            if (trans.size() > instanceIndex) {
                GfMatrix4d t(1);
                t.SetTranslate(GfVec3d(trans[instanceIndex]));
                ma[j] = t * ma[j];
            }
            if (rot.size() > instanceIndex) {
                GfMatrix4d r(1);
                r.SetRotate(GfRotation(rot[instanceIndex]));
                ma[j] = r * ma[j];
            }
            if (scale.size() > instanceIndex) {
                GfMatrix4d s(1);
                s.SetScale(GfVec3d(scale[instanceIndex]));
                ma[j] = s * ma[j];
            }
            if (ixf.size() > instanceIndex) {
                ma[j] = ixf[instanceIndex] * ma[j];
            }
        }
    }

    // If there is a parent instancer, continue to unroll
    // the child instances across the parent; otherwise we're done.
    if (GetParentId().IsEmpty()) {
        return;
    }

    HdInstancer *parentInstancer =
        GetDelegate()->GetRenderIndex().GetInstancer(GetParentId());
    if (!TF_VERIFY(parentInstancer)) {
        return;
    }
    HdPrmanInstancer *hdPrmanParentInstancer =
        static_cast<HdPrmanInstancer*>(parentInstancer);

    // Multiply the instance samples against the parent instancer samples.
    // The transforms taking nesting into account are computed by:
    // parentTransforms = parentInstancer->ComputeInstanceTransforms(GetId())
    // foreach (parentXf : parentTransforms, xf : transforms) {
    //     parentXf * xf
    // }
    HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> parentXf;
    VtIntArray instanceIndicesParent =
        GetDelegate()->GetInstanceIndices(GetParentId(), GetId());
    hdPrmanParentInstancer->
        SampleInstanceTransforms(GetId(), instanceIndicesParent, &parentXf);
    if (parentXf.count == 0 || parentXf.values[0].empty()) {
        // No samples for parent instancer.
        return;
    }
    // Move aside previously computed child xform samples to childXf.
    HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> childXf(*sa);
    // Merge sample times, taking the densest sampling.
    _AccumulateSampleTimes(parentXf, sa);
    // Apply parent xforms to the children.
    for (size_t i=0; i < sa->count; ++i) {
        const float t = sa->times[i];
        // Resample transforms at the same time.
        VtMatrix4dArray curParentXf = parentXf.Resample(t);
        VtMatrix4dArray curChildXf = childXf.Resample(t);
        // Multiply out each combination.
        VtMatrix4dArray &result = sa->values[i];
        result.resize(curParentXf.size() * curChildXf.size());
        for (size_t j = 0; j < curParentXf.size(); ++j) {
            for (size_t k = 0; k < curChildXf.size(); ++k) {
                result[j * curChildXf.size() + k] =
                    curChildXf[k] * curParentXf[j];
            }
        }
    }
}

void
HdPrmanInstancer::GetInstancePrimvars(
    SdfPath const& prototypeId,
    size_t instanceIndex,
    RtParamList& attrs)
{
    for (auto entry: _primvarMap) {
        HdPrimvarDescriptor const& primvar = entry.second.desc;
        // Skip non-instance-rate primvars.
        if (primvar.interpolation != HdInterpolationInstance) {
            continue;
        }
        // Confirm that instance-rate primvars are array-valued
        // and have sufficient dimensions.
        VtValue const& val = entry.second.value;
        if (instanceIndex >= val.GetArraySize()) {
            TF_WARN("HdPrman: Instance-rate primvar has array size %zu; "
                    "cannot provide a value for instance index %zu\n",
                    val.GetArraySize(), instanceIndex);
            continue;
        }

        // Instance primvars with the "ri:attributes:" prefix correspond to
        // renderman-namespace attributes and have that prefix stripped.
        // All other primvars are in the "user:" namespace, so if they don't
        // have that prefix we need to add it.
        RtUString name;
        static const char *userPrefix = "user:";
        static const char *riAttrPrefix = "ri:attributes:";
        if (!strncmp(entry.first.GetText(), userPrefix, strlen(userPrefix))) {
            name = RtUString(entry.first.GetText());
        } else if (!strncmp(entry.first.GetText(), riAttrPrefix,
                            strlen(riAttrPrefix))) {
            const char *strippedName = entry.first.GetText();
            strippedName += strlen(riAttrPrefix);
            name = RtUString(strippedName);
        } else {
            std::string mangled =
                TfStringPrintf("user:%s", entry.first.GetText());
            name = RtUString(mangled.c_str());
        }

        if (val.IsHolding<VtArray<float>>()) {
            const VtArray<float>& v = val.UncheckedGet<VtArray<float>>();
            attrs.SetFloat(name, v[instanceIndex]);
        } else if (val.IsHolding<VtArray<int>>()) {
            const VtArray<int>& v = val.UncheckedGet<VtArray<int>>();
            attrs.SetInteger(name, v[instanceIndex]);
        } else if (val.IsHolding<VtArray<GfVec2f>>()) {
            const VtArray<GfVec2f>& v = val.UncheckedGet<VtArray<GfVec2f>>();
            attrs.SetFloatArray(
                name, reinterpret_cast<const float*>(v.cdata()), 2);
        } else if (val.IsHolding<VtArray<GfVec3f>>()) {
            const GfVec3f& v =
                val.UncheckedGet<VtArray<GfVec3f>>()[instanceIndex];
            if (primvar.role == HdPrimvarRoleTokens->color) {
                attrs.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
            } else if (primvar.role == HdPrimvarRoleTokens->point) {
                attrs.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
            } else if (primvar.role == HdPrimvarRoleTokens->normal) {
                attrs.SetPoint(name, RtNormal3(v[0], v[1], v[2]));
            } else {
                attrs.SetVector(name, RtVector3(v[0], v[1], v[2]));
            }
        } else if (val.IsHolding<VtArray<GfVec4f>>()) {
            const VtArray<GfVec4f>& v = val.UncheckedGet<VtArray<GfVec4f>>();
            attrs.SetFloatArray(
                name, reinterpret_cast<const float*>(v.cdata()), 4);
        } else if (val.IsHolding<VtArray<GfMatrix4d>>()) {
            const VtArray<GfMatrix4d>& v =
                val.UncheckedGet<VtArray<GfMatrix4d>>();
            attrs.SetMatrix(name,
                HdPrman_GfMatrixToRtMatrix(v[instanceIndex]));
        } else if (val.IsHolding<VtArray<std::string>>()) {
            const VtArray<std::string>& v =
                val.UncheckedGet<VtArray<std::string>>();
            attrs.SetString(name, RtUString(v[instanceIndex].c_str()));
        } else if (val.IsHolding<VtArray<TfToken>>()) {
            const VtArray<TfToken>& v = val.UncheckedGet<VtArray<TfToken>>();
            attrs.SetString(name, RtUString(v[instanceIndex].GetText()));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

