//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/skelData.h"

#include "pxr/usdImaging/usdSkelImaging/animationSchema.h"
#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"
#include "pxr/usdImaging/usdSkelImaging/skeletonSchema.h"

#include "pxr/usd/usdSkel/utils.h"

#include "pxr/base/trace/trace.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

// Wrapper for HdSceneIndexBase::GetPrim to have a TRACE_FUNCTION.
//
// We could cache some data in the UsdSkelImaging filtering scene indices
// if we see much time spend here.
//
static
HdSceneIndexPrim
_GetPrim(HdSceneIndexBaseRefPtr const &sceneIndex,
         const SdfPath &primPath)
{
    TRACE_FUNCTION();

    return sceneIndex->GetPrim(primPath);
}

static
void
_Convert(const VtArray<GfMatrix4d> &matrices,
         VtArray<GfMatrix4f> * const inverses)
{
    TRACE_FUNCTION();

    inverses->resize(
        matrices.size(),
        [&matrices](
            GfMatrix4f * const begin, GfMatrix4f * const end) {
            const GfMatrix4d * inData = matrices.data();
            for (GfMatrix4f * outData = begin; outData < end; ++outData) {
                new (outData) GfMatrix4f(*inData);
                ++inData;
            }});
}

static
void
_Invert(const VtArray<GfMatrix4f> &matrices,
        VtArray<GfMatrix4f> * const inverses)
{
    TRACE_FUNCTION();

    inverses->resize(
        matrices.size(),
        [&matrices](
            GfMatrix4f * const begin, GfMatrix4f * const end) {
            const GfMatrix4f * inData = matrices.data();
            for (GfMatrix4f * outData = begin; outData < end; ++outData) {
                new (outData) GfMatrix4f(inData->GetInverse());
                ++inData;
            }});
}

UsdSkelImagingSkelData
UsdSkelImagingComputeSkelData(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath)
{
    TRACE_FUNCTION();

    UsdSkelImagingSkelData data;

    data.primPath = primPath;

    const HdSceneIndexPrim prim = _GetPrim(sceneIndex, primPath);

    const UsdSkelImagingSkeletonSchema skeletonSchema =
        UsdSkelImagingSkeletonSchema::GetFromParent(prim.dataSource);

    data.topology =
        UsdSkelTopology(
            UsdSkelImagingGetTypedValue(skeletonSchema.GetJoints()));

    _Convert(
        UsdSkelImagingGetTypedValue(skeletonSchema.GetBindTransforms()),
        &data.bindTransforms);
    
    _Invert(
        data.bindTransforms,
        &data.inverseBindTransforms);

    const UsdSkelImagingBindingSchema bindingSchema =
        UsdSkelImagingBindingSchema::GetFromParent(prim.dataSource);

    data.animationSource =
        UsdSkelImagingGetTypedValue(bindingSchema.GetAnimationSource());

    if (!data.animationSource.IsEmpty()) {
        const HdSceneIndexPrim animPrim =
            _GetPrim(sceneIndex, data.animationSource);
        const UsdSkelImagingAnimationSchema animSchema =
            UsdSkelImagingAnimationSchema::GetFromParent(animPrim.dataSource);
        if (animSchema) {
            data.animMapper =
                UsdSkelAnimMapper(
                    UsdSkelImagingGetTypedValue(animSchema.GetJoints()),
                    UsdSkelImagingGetTypedValue(skeletonSchema.GetJoints()));
        }
    }

    return data;
}

static
bool
_MultiplyInPlace(const VtArray<GfMatrix4f> &matrices,
                 VtArray<GfMatrix4f> * const result)
{
    const size_t n = result->size();
    if (matrices.size() != n) {
        return false;
    }

    GfMatrix4f * data = result->data();
    GfMatrix4f * end = data + n;
    const GfMatrix4f * matrixData = matrices.cdata();
    while(data < end) {
        *data = *matrixData * *data;
        ++data;
        ++matrixData;
    }

    return true;
}

static
VtArray<GfMatrix4f>
_ComputeJointLocalTransforms(
    const UsdSkelImagingSkelData &data,
    HdMatrix4fArrayDataSourceHandle const &restTransforms,
    const VtArray<GfVec3f> &translations,
    const VtArray<GfQuatf> &rotations,
    const VtArray<GfVec3h> &scales)
{
    if (data.animMapper.IsNull()) {
        // No skelAnimation, simply return the restTransforms.
        return UsdSkelImagingGetTypedValue(restTransforms);
    }

    VtArray<GfMatrix4f> animTransforms(translations.size());

    if (!UsdSkelMakeTransforms(translations, rotations, scales,
                               animTransforms)) {
        TF_WARN("Could not compute transforms for skelAnimation %s.\n",
                data.animationSource.GetText());
        return UsdSkelImagingGetTypedValue(restTransforms);
    }

    VtArray<GfMatrix4f> result;

    if (data.animMapper.IsSparse()) {
        result = UsdSkelImagingGetTypedValue(restTransforms);
    } else {
        result.resize(data.topology.GetNumJoints());
    }

    if (!data.animMapper.RemapTransforms(animTransforms, &result)) {
        TF_WARN("Could not remap transforms from skelAnimation %s for "
                "skeleton %s.\n",
                data.animationSource.GetText(),
                data.primPath.GetText());
        return UsdSkelImagingGetTypedValue(restTransforms);
    }
    return result;
}

static
VtArray<GfMatrix4f>
_ConcatJointTransforms(
    const UsdSkelTopology &topology,
    const VtArray<GfMatrix4f> &localTransforms,
    const SdfPath &primPath)
{
    VtArray<GfMatrix4f> result(topology.size());
    if (!UsdSkelConcatJointTransforms(topology, localTransforms, result)) {
        TF_WARN("Could not concat local joint transforms for skeleton %s.\n",
                primPath.GetText());
    }
    return result;
}

VtArray<GfMatrix4f>
UsdSkelImagingComputeSkinningTransforms(
    const UsdSkelImagingSkelData &data,
    HdMatrix4fArrayDataSourceHandle const &restTransforms,
    const VtArray<GfVec3f> &translations,
    const VtArray<GfQuatf> &rotations,
    const VtArray<GfVec3h> &scales)
{

    VtArray<GfMatrix4f> result =
        _ConcatJointTransforms(
            data.topology,
            _ComputeJointLocalTransforms(
                data,
                restTransforms,
                translations, rotations, scales),
            data.primPath);

    if (!_MultiplyInPlace(data.inverseBindTransforms, &result)) {
        TF_WARN(
            "Length (%zu) of bind transforms does not match number (%zu) of "
            "joints.\n",
            data.inverseBindTransforms.size(), result.size());
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
