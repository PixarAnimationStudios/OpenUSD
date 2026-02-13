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
#include "pxr/base/work/loops.h"

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

UsdSkelImagingSkelData::UsdSkelImagingSkelData(const SdfPath& path, 
    const UsdSkelImagingSkeletonSchema& schema):
    primPath(path), skeletonSchema(schema), 
    topology(UsdSkelTopology(UsdSkelImagingGetTypedValue(schema.GetJoints()))) 
{
}

UsdSkelImagingSkelData
UsdSkelImagingComputeSkelData(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath)
{
    TRACE_FUNCTION();

    const HdSceneIndexPrim prim = _GetPrim(sceneIndex, primPath);
    UsdSkelImagingSkelData data(primPath, 
        UsdSkelImagingSkeletonSchema::GetFromParent(prim.dataSource));

    _Convert(UsdSkelImagingGetTypedValue(
        data.skeletonSchema.GetBindTransforms()), &data.bindTransforms);

    _Invert(data.bindTransforms, &data.inverseBindTransforms);

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
    const UsdSkelImagingSkelData& data,
    const UsdSkelImagingAnimationSchema& animationSchema,
    const SdfPath& animationSource,
    HdMatrix4fArrayDataSourceHandle const& restTransforms,
    const HdSampledDataSource::Time& shutterOffset)
{
    // Remapping of skelAnimation's data to skeleton's hierarchy.
    UsdSkelAnimMapper animMapper = UsdSkelAnimMapper(
        UsdSkelImagingGetTypedValue(animationSchema.GetJoints()),
        UsdSkelImagingGetTypedValue(data.skeletonSchema.GetJoints()));
    if (animMapper.IsNull()) {
        // No skelAnimation, simply return the restTransforms.
        return UsdSkelImagingGetTypedValue(restTransforms, shutterOffset);
    }

    const VtArray<GfVec3f>& translations = UsdSkelImagingGetTypedValue(
        animationSchema.GetTranslations(), shutterOffset);
    const VtArray<GfQuatf>& rotations = UsdSkelImagingGetTypedValue(
        animationSchema.GetRotations(), shutterOffset);
    const VtArray<GfVec3h>& scales = UsdSkelImagingGetTypedValue(
        animationSchema.GetScales(), shutterOffset);

    VtArray<GfMatrix4f> animTransforms(translations.size());

    if (!UsdSkelMakeTransforms(translations, rotations, scales,
                               animTransforms)) {
        TF_WARN("Could not compute transforms for skelAnimation %s.\n",
                animationSource.GetText());
        return UsdSkelImagingGetTypedValue(restTransforms, shutterOffset);
    }

    // If all transform components were empty, that could mean:
    // - the attributes were never authored
    // - the attributes were blocked
    // - the attributes were authored with empty arrays (possibly intentionally)
    // In many of these cases, we should expect the animation to be silently
    // ignored, so throw no warning.
    if (animTransforms.empty()) {
        return UsdSkelImagingGetTypedValue(restTransforms);
    }

    if (animMapper.IsIdentity()) {
        return animTransforms;
    }

    VtArray<GfMatrix4f> result;

    if (animMapper.IsSparse()) {
        result = UsdSkelImagingGetTypedValue(restTransforms, shutterOffset);
    } else {
        result.resize(data.topology.GetNumJoints());
    }

    if (!animMapper.RemapTransforms(animTransforms, &result)) {
        TF_WARN("Could not remap transforms from skelAnimation %s for "
                "skeleton %s.\n",
                animationSource.GetText(),
                data.primPath.GetText());
        return UsdSkelImagingGetTypedValue(restTransforms, shutterOffset);
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
    const VtArray<UsdSkelImagingAnimationSchema>& animationSchemas,
    const VtArray<SdfPath>& animationSources,
    const HdSampledDataSource::Time& shutterOffset)
{
    TRACE_FUNCTION();

    // Compute and fill the concatenated skinning transforms in parallel.
    // Since all the animationSources/Schemas share the same resolved skeleton
    // that calls this method, they should all have the same size which is the 
    // number of joints in the topology.
    const size_t numJoints = data.topology.size();
    VtArray<GfMatrix4f> result;
    result.resize(
        animationSchemas.size() * numJoints,
        [&] (GfMatrix4f* start, GfMatrix4f* end)
        {
            WorkParallelForN(
                animationSchemas.size(),
                [&] (size_t animStart, size_t animEnd)
                {
                    TRACE_FUNCTION_SCOPE(
                        "ComputeSkinningTransforms inner loop");

                    for (size_t i = animStart; i < animEnd; ++i) {
                        VtArray<GfMatrix4f> xforms = _ConcatJointTransforms(
                            data.topology,
                            _ComputeJointLocalTransforms(
                                data, animationSchemas[i], animationSources[i],
                                restTransforms, shutterOffset),
                            data.primPath);

                        if (!_MultiplyInPlace(
                            data.inverseBindTransforms, &xforms)) {
                            TF_WARN("Length (%zu) of bind transforms does not "
                                "match number (%zu) of joints for "
                                "skelAnimation %s.\n",
                                data.inverseBindTransforms.size(), numJoints, 
                                animationSources[i].GetText());
                        }
                        // XXX
                        // xforms.size() should equal to numJoints
                        // do we need a TF_VERIFY here?
                        for (size_t j = 0; j < numJoints; ++j) {
                            new (start + i * numJoints + j) 
                                GfMatrix4f(xforms[j]);
                        }
                    }
                });
        });

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
