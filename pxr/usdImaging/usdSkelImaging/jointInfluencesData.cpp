//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/jointInfluencesData.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"
#include "pxr/usdImaging/usdSkelImaging/skeletonSchema.h"

#include "pxr/usd/usdSkel/utils.h"

#include "pxr/imaging/hd/primvarsSchema.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// The primvar value of an indexed primvar is a flattening data source that
// is a plain HdSampledDataSource (not an HdTypedSampledDataSource<T>), so
// we cannot use Cast<> and need to go through the VtValue.
template<typename T>
VtArray<T>
_GetPrimvarArray(const HdPrimvarSchema &primvar)
{
    if (HdSampledDataSourceHandle const ds = primvar.GetPrimvarValue()) {
        const VtValue value = ds->GetValue(0.0f);
        if (value.IsHolding<VtArray<T>>()) {
            return value.UncheckedGet<VtArray<T>>();
        }
    }
    return {};
}

}

UsdSkelImagingJointInfluencesData
UsdSkelImagingComputeJointInfluencesData(
    HdContainerDataSourceHandle const &primSource,
    HdContainerDataSourceHandle const &skeletonPrimSource)
{
    UsdSkelImagingJointInfluencesData data{};

    const HdPrimvarsSchema primvars =
        HdPrimvarsSchema::GetFromParent(primSource);

    // joint indices primvar. Also determines hasConstantInterpolation
    // through primvar interpolation.
    const HdPrimvarSchema jointIndicesPrimvar =
        primvars.GetPrimvar(
            UsdSkelImagingBindingSchemaTokens->jointIndicesPrimvar);

    const TfToken interpolation =
        UsdSkelImagingGetTypedValue(jointIndicesPrimvar.GetInterpolation());

    data.hasConstantInfluences =
        interpolation == HdPrimvarSchemaTokens->constant;

    const VtIntArray jointIndices =
        _GetPrimvarArray<int>(jointIndicesPrimvar);
    if (jointIndices.empty()) {
        return data;
    }

    // joint weights primvar. Also determines numInfluencesPerComponent
    // through element size.
    const HdPrimvarSchema jointWeightsPrimvar =
        primvars.GetPrimvar(
            UsdSkelImagingBindingSchemaTokens->jointWeightsPrimvar);

    const VtFloatArray jointWeights =
        _GetPrimvarArray<float>(jointWeightsPrimvar);
    if (jointWeights.empty()) {
        return data;
    }

    if (HdIntDataSourceHandle const ds = jointWeightsPrimvar.GetElementSize()) {
        data.numInfluencesPerComponent = ds->GetTypedValue(0.0f);
    } else {
        data.numInfluencesPerComponent = 1;
    }

    data.influences.resize(jointIndices.size());
    UsdSkelInterleaveInfluences(jointIndices, jointWeights, data.influences);

    // Compute jointMapper.
    const VtArray<TfToken> joints =
        UsdSkelImagingGetTypedValue(
            UsdSkelImagingBindingSchema::GetFromParent(primSource)
                .GetJoints());
    if (!joints.empty()) {
        data.jointMapper =
            UsdSkelAnimMapper(
                UsdSkelImagingGetTypedValue(
                    UsdSkelImagingSkeletonSchema::GetFromParent(
                            skeletonPrimSource)
                        .GetJoints()),
                joints);
    }

    return data;
}

PXR_NAMESPACE_CLOSE_SCOPE
