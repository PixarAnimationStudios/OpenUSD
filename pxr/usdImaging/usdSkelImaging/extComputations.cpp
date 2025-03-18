//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdSkelImaging/extComputations.h"

#include "pxr/usdImaging/usdSkelImaging/package.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

#include "pxr/usd/usdSkel/tokens.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/imaging/hd/extComputationContext.h"
#include "pxr/imaging/hd/extComputationCpuCallback.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (skinPointsLBSKernel)
    (skinPointsDQSKernel)
);

TF_DEFINE_ENV_SETTING(USDSKELIMAGING_FORCE_CPU_COMPUTE, false,
                      "Use Hydra ExtCPU computations for skinning.");

///////////////////////////////////////////////////////////////////////////////
/// UsdSkelImagingInvokeExtComputation

static
void
_TransformPoints(TfSpan<GfVec3f> points, const GfMatrix4d& xform)
{
    WorkParallelForN(
        points.size(),
        [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                points[i] = GfVec3f(xform.Transform(points[i]));
            }
        }, /*grainSize*/ 1000);
}

static
void
_ApplyPackedBlendShapes(const TfSpan<const GfVec4f>& offsets,
                        const TfSpan<const GfVec2i>& ranges,
                        const TfSpan<const float>& weights,
                        TfSpan<GfVec3f> points)
{
    const size_t end = std::min(ranges.size(), points.size());
    for (size_t i = 0; i < end; ++i) {
        const GfVec2i range = ranges[i];

        GfVec3f p = points[i];
        for (int j = range[0]; j < range[1]; ++j) {
            const GfVec4f offset = offsets[j];
            const int shapeIndex = static_cast<int>(offset[3]);
            const float weight = weights[shapeIndex];
            p += GfVec3f(offset[0], offset[1], offset[2])*weight;
        }
        points[i] = p;
    }
}

void
UsdSkelImagingInvokeExtComputation(
    const TfToken &skinningMethod,
    HdExtComputationContext * const ctx)
{
    TRACE_FUNCTION();

    const VtValue restPointsValue =
        ctx->GetInputValue(
            UsdSkelImagingExtAggregatorComputationInputNameTokens
                ->restPoints);
    const VtValue geomBindXformValue =
        ctx->GetInputValue(
            UsdSkelImagingExtAggregatorComputationInputNameTokens
                ->geomBindXform);
    const VtValue influencesValue =
        ctx->GetInputValue(
            UsdSkelImagingExtAggregatorComputationInputNameTokens
                ->influences);
    const VtValue numInfluencesPerComponentValue =
        ctx->GetInputValue(
            UsdSkelImagingExtAggregatorComputationInputNameTokens
                ->numInfluencesPerComponent);
    const VtValue hasConstantInfluencesValue =
        ctx->GetInputValue(
            UsdSkelImagingExtAggregatorComputationInputNameTokens
                ->hasConstantInfluences);
    const VtValue primWorldToLocalValue =
        ctx->GetInputValue(
            UsdSkelImagingExtComputationInputNameTokens
                ->primWorldToLocal);
    const VtValue blendShapeOffsetsValue =
        ctx->GetInputValue(
            UsdSkelImagingExtAggregatorComputationInputNameTokens
                ->blendShapeOffsets);
    const VtValue blendShapeOffsetRangesValue =
        ctx->GetInputValue(
            UsdSkelImagingExtAggregatorComputationInputNameTokens
                ->blendShapeOffsetRanges);
    const VtValue blendShapeWeightsValue =
        ctx->GetInputValue(
            UsdSkelImagingExtComputationInputNameTokens
                ->blendShapeWeights);
    const VtValue skinningXformsValue =
        ctx->GetInputValue(
            UsdSkelImagingExtComputationInputNameTokens
                ->skinningXforms);
    const VtValue skelLocalToWorldValue =
        ctx->GetInputValue(
            UsdSkelImagingExtComputationInputNameTokens
                ->skelLocalToWorld);

    // Ensure inputs are holding the right value types.
    if (!restPointsValue.IsHolding<VtVec3fArray>() ||
        !geomBindXformValue.IsHolding<GfMatrix4f>() ||
        !influencesValue.IsHolding<VtVec2fArray>() ||
        !numInfluencesPerComponentValue.IsHolding<int>() ||
        !hasConstantInfluencesValue.IsHolding<bool>() ||
        !primWorldToLocalValue.IsHolding<GfMatrix4d>() ||
        !blendShapeOffsetsValue.IsHolding<VtVec4fArray>() ||
        !blendShapeOffsetRangesValue.IsHolding<VtVec2iArray>() ||
        !blendShapeWeightsValue.IsHolding<VtFloatArray>() ||
        !skinningXformsValue.IsHolding<VtMatrix4fArray>() ||
        !skelLocalToWorldValue.IsHolding<GfMatrix4d>()) {
        ctx->RaiseComputationError();
        return;
    }

    VtVec3fArray skinnedPoints =
        restPointsValue.UncheckedGet<VtVec3fArray>();

    _ApplyPackedBlendShapes(
        blendShapeOffsetsValue.UncheckedGet<VtVec4fArray>(),
        blendShapeOffsetRangesValue.UncheckedGet<VtVec2iArray>(),
        blendShapeWeightsValue.UncheckedGet<VtFloatArray>(),
        skinnedPoints);

    const int numInfluencesPerComponent =
        numInfluencesPerComponentValue.UncheckedGet<int>();

    if (numInfluencesPerComponent <= 0) {
        ctx->SetOutputValue(
            UsdSkelImagingExtComputationOutputNameTokens->skinnedPoints,
            VtValue(skinnedPoints));
        return;
    }

    if (hasConstantInfluencesValue.UncheckedGet<bool>()) {
        // Have constant influences. Compute a rigid deformation.
        GfMatrix4f skinnedTransform;
        if (UsdSkelSkinTransform(
                skinningMethod,
                geomBindXformValue.UncheckedGet<GfMatrix4f>(),
                skinningXformsValue.UncheckedGet<VtMatrix4fArray>(),
                influencesValue.UncheckedGet<VtVec2fArray>(),
                &skinnedTransform)) {

            // The computed skinnedTransform is the transform which, when
            // applied to the points of the skinned prim, results in skinned
            // points in *skel* space, and need to be xformed to prim
            // local space.

            const GfMatrix4d restToPrimLocalSkinnedXf =
                GfMatrix4d(skinnedTransform)*
                skelLocalToWorldValue.UncheckedGet<GfMatrix4d>()*
                primWorldToLocalValue.UncheckedGet<GfMatrix4d>();

            // XXX: Ideally we would modify the xform of the skinned prim,
            // rather than its underlying points (which is particularly
            // important if we want to preserve instancing!).
            // For now, bake the rigid deformation into the points.
            _TransformPoints(skinnedPoints, restToPrimLocalSkinnedXf);

        } else {
            // Nothing to do. We initialized skinnedPoints to the restPoints,
            // so just return that.
        }
    } else {
        UsdSkelSkinPoints(
            skinningMethod,
            geomBindXformValue.UncheckedGet<GfMatrix4f>(),
            skinningXformsValue.UncheckedGet<VtMatrix4fArray>(),
            influencesValue.UncheckedGet<VtVec2fArray>(),
            numInfluencesPerComponent,
            skinnedPoints);

        // The points returned above are in skel space, and need to be
        // transformed to prim local space.
        const GfMatrix4d skelToPrimLocal =
            skelLocalToWorldValue.UncheckedGet<GfMatrix4d>() *
            primWorldToLocalValue.UncheckedGet<GfMatrix4d>();

        _TransformPoints(skinnedPoints, skelToPrimLocal);

    }

    ctx->SetOutputValue(
        UsdSkelImagingExtComputationOutputNameTokens->skinnedPoints,
        VtValue(skinnedPoints));
}

///////////////////////////////////////////////////////////////////////////////
/// UsdSkelImagingExtComputationCpuCallback

namespace {

class _SkinningComputationCpuCallback : public HdExtComputationCpuCallback
{
public:
    _SkinningComputationCpuCallback(const TfToken &skinningMethod)
     : _skinningMethod(skinningMethod)
    {
    }

    void Compute(HdExtComputationContext * const ctx) override {
        UsdSkelImagingInvokeExtComputation(_skinningMethod, ctx);
    }

private:
    const TfToken _skinningMethod;
};

}

static
HdExtComputationCpuCallbackDataSourceHandle
_ExtComputationCpuCallbackDataSource(const TfToken &skinningMethod)
{
    return
        HdRetainedTypedSampledDataSource<
            HdExtComputationCpuCallbackSharedPtr>::New(
                std::make_shared<_SkinningComputationCpuCallback>(
                    skinningMethod));
}


HdExtComputationCpuCallbackDataSourceHandle
UsdSkelImagingExtComputationCpuCallback(const TfToken &skinningMethod)
{
    TRACE_FUNCTION();

    if (skinningMethod == UsdSkelTokens->classicLinear) {
        static HdExtComputationCpuCallbackDataSourceHandle const result =
            _ExtComputationCpuCallbackDataSource(
                UsdSkelTokens->classicLinear);
        return result;
    }
    if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        static HdExtComputationCpuCallbackDataSourceHandle const result =
            _ExtComputationCpuCallbackDataSource(
                UsdSkelTokens->dualQuaternion);
        return result;
    }

    TF_WARN("Unknown skinning method %s\n", skinningMethod.GetText());

    return nullptr;
}
///////////////////////////////////////////////////////////////////////////////
/// UsdSkelImagingExtComputationGlslKernel

static
HdStringDataSourceHandle
_LoadSkinningComputeKernel(const TfToken &kernelKey)
{
    TRACE_FUNCTION();

    const HioGlslfx gfx(UsdSkelImagingPackageSkinningShader());
    if (!gfx.IsValid()) {
        TF_CODING_ERROR("Couldn't load UsdImagingGLPackageSkinningShader");
        return nullptr;
    }

    const std::string shaderSource = gfx.GetSource(kernelKey);
    if (!TF_VERIFY(!shaderSource.empty())) {
        TF_WARN("Skinning compute shader is missing kernel '%s'",
                kernelKey.GetText());
        return nullptr;
    }

    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg(
        "Kernel for skinning is :\n%s\n", shaderSource.c_str());
    return
        HdRetainedTypedSampledDataSource<std::string>::New(
            shaderSource);
}

HdStringDataSourceHandle
UsdSkelImagingExtComputationGlslKernel(const TfToken &skinningMethod)
{
    TRACE_FUNCTION();

    if (TfGetEnvSetting(USDSKELIMAGING_FORCE_CPU_COMPUTE)) {
        return nullptr;
    }

    if (skinningMethod == UsdSkelTokens->classicLinear) {
        static HdStringDataSourceHandle const result =
            _LoadSkinningComputeKernel(_tokens->skinPointsLBSKernel);
        return result;
    }
    if (skinningMethod == UsdSkelTokens->dualQuaternion) {
        static HdStringDataSourceHandle const result =
            _LoadSkinningComputeKernel(_tokens->skinPointsDQSKernel);
        return result;
    }

    TF_WARN("Unknown skinning method %s\n", skinningMethod.GetText());

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
