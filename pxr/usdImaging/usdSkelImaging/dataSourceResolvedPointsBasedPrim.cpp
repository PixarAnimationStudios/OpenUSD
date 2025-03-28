//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourceResolvedPointsBasedPrim.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/blendShapeData.h"
#include "pxr/usdImaging/usdSkelImaging/blendShapeSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"
#include "pxr/usdImaging/usdSkelImaging/jointInfluencesData.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

#include "pxr/usd/usdSkel/tokens.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

/// GfMatrix4d-typed sampled data source giving inverse matrix for
/// given data source (of same type).
class _MatrixInverseDataSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MatrixInverseDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(const Time shutterOffset) override {
        if (shutterOffset == 0.0f) {
            return _valueAtZero;
        }

        return _Compute(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {
        return _inputSrc->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _MatrixInverseDataSource(HdMatrixDataSourceHandle inputSrc)
     : _inputSrc(std::move(inputSrc))
     , _valueAtZero(_Compute(0.0f))
    {
    }

    GfMatrix4d _Compute(const Time shutterOffset)
    {
        if (!_inputSrc) {
            return GfMatrix4d(1.0);
        }
        return _inputSrc->GetTypedValue(shutterOffset).GetInverse();
    }

    HdMatrixDataSourceHandle const _inputSrc;
    const GfMatrix4d _valueAtZero;
};

}

UsdSkelImagingDataSourceResolvedPointsBasedPrim::Handle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::New(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    SdfPath primPath,
    HdContainerDataSourceHandle primSource)
{
    if (!primSource) {
        return nullptr;
    }

    const UsdSkelImagingBindingSchema bindingSchema =
        UsdSkelImagingBindingSchema::GetFromParent(primSource);

    HdBoolDataSourceHandle const hasSkelRootDs =
        bindingSchema.GetHasSkelRoot();

    const bool hasSkelRoot =
        hasSkelRootDs && hasSkelRootDs->GetTypedValue(0.0f);

    HdPathDataSourceHandle const skeletonPathDataSource =
        bindingSchema.GetSkeleton();
    if (!skeletonPathDataSource) {
        return nullptr;
    }
    SdfPath skeletonPath = skeletonPathDataSource->GetTypedValue(0.0f);
    if (skeletonPath.IsEmpty()) {
        return nullptr;
    }

    VtArray<SdfPath> blendShapeTargetPaths =
        UsdSkelImagingGetTypedValue(bindingSchema.GetBlendShapeTargets());

    HdContainerDataSourceHandle skeletonPrimSource =
        sceneIndex->GetPrim(skeletonPath).dataSource;

    UsdSkelImagingResolvedSkeletonSchema resolvedSkeletonSchema =
        UsdSkelImagingResolvedSkeletonSchema::GetFromParent(
            skeletonPrimSource);

    return New(
        sceneIndex,
        std::move(primPath),
        std::move(primSource),
        hasSkelRoot,
        std::move(blendShapeTargetPaths),
        std::move(skeletonPath),
        std::move(skeletonPrimSource),
        std::move(resolvedSkeletonSchema));
};

static
TfToken
_GetSkinningMethod(
    const HdPrimvarsSchema &primvars,
    const SdfPath &primPath) // For warning messages only.
{
    const TfToken method =
        UsdSkelImagingGetTypedValue(
            HdTokenDataSource::Cast(
                primvars
                    .GetPrimvar(UsdSkelImagingBindingSchemaTokens
                                    ->skinningMethodPrimvar)
                    .GetPrimvarValue()));
    if (method.IsEmpty()) {
        return UsdSkelTokens->classicLinear;
    }

    if (method != UsdSkelTokens->classicLinear &&
        method != UsdSkelTokens->dualQuaternion) {
        TF_WARN("Unknown skinning method %s on prim %s. "
                "Falling back to classicLinear.\n",
                method.GetText(), primPath.GetText());
        return UsdSkelTokens->classicLinear;
    }

    return method;
}

UsdSkelImagingDataSourceResolvedPointsBasedPrim::
UsdSkelImagingDataSourceResolvedPointsBasedPrim(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    SdfPath primPath,
    HdContainerDataSourceHandle primSource,
    const bool hasSkelRoot,
    VtArray<SdfPath> blendShapeTargetPaths,
    SdfPath skeletonPath,
    HdContainerDataSourceHandle skeletonPrimSource,
    UsdSkelImagingResolvedSkeletonSchema resolvedSkeletonSchema)
 : _sceneIndex(sceneIndex)
 , _primPath(primPath)
 , _primSource(std::move(primSource))
 , _hasSkelRoot(hasSkelRoot)
 , _primvars(HdPrimvarsSchema::GetFromParent(_primSource))
 , _skinningMethod(_GetSkinningMethod(_primvars, primPath))
 , _blendShapeTargetPaths(std::move(blendShapeTargetPaths))
 , _skeletonPath(std::move(skeletonPath))
 , _skeletonPrimSource(std::move(skeletonPrimSource))
 , _resolvedSkeletonSchema(std::move(resolvedSkeletonSchema))
 , _blendShapeDataCache(sceneIndex, primPath)
 , _jointInfluencesDataCache(_primSource, _skeletonPrimSource)
{
}

UsdSkelImagingDataSourceResolvedPointsBasedPrim::
~UsdSkelImagingDataSourceResolvedPointsBasedPrim() = default;

static
HdContainerDataSourceHandle
_ExtComputationPrimvars(const SdfPath &primPath)
{
    static const TfToken names[] = {
        HdPrimvarsSchemaTokens->points
    };
    HdDataSourceBaseHandle const values[] = {
        HdExtComputationPrimvarSchema::Builder()
            .SetInterpolation(
                HdExtComputationPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->vertex))
            .SetRole(
                HdExtComputationPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->point))
            .SetSourceComputation(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    primPath.AppendChild(
                        UsdSkelImagingExtComputationNameTokens->computation)))
            .SetSourceComputationOutputName(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    UsdSkelImagingExtComputationOutputNameTokens->skinnedPoints))
            .SetValueType(
                HdRetainedTypedSampledDataSource<HdTupleType>::New(
                    HdTupleType{HdTypeFloatVec3, 1}))
        .Build()
    };

    static_assert(std::size(names) == std::size(values));
    return HdExtComputationPrimvarsSchema::BuildRetained(
        std::size(names), names, values);
}

static
HdContainerDataSourceHandle
_BlockPointsPrimvars()
{
    const TfToken names[] = {
        HdPrimvarsSchemaTokens->points
    };
    HdDataSourceBaseHandle const values[] = {
        HdBlockDataSource::New()
    };

    static_assert(std::size(names) == std::size(values));
    return HdPrimvarsSchema::BuildRetained(std::size(names), names, values);
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::Get(const TfToken &name)
{
    HdDataSourceBaseHandle inputSrc = _primSource->Get(name);

    if (!HasExtComputations()) {
        return inputSrc;
    }

    if (name == HdExtComputationPrimvarsSchema::GetSchemaToken()) {
        return HdOverlayContainerDataSource::OverlayedContainerDataSources(
            _ExtComputationPrimvars(_primPath),
            HdContainerDataSource::Cast(inputSrc));
    }

    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        // Block points primvar.
        static HdContainerDataSourceHandle ds = _BlockPointsPrimvars();
        return ds;
    }

    return inputSrc;
}

static
void
_AddIfNecessary(const TfToken &name, TfTokenVector * const names)
{
    if (std::find(names->begin(), names->end(), name) != names->end()) {
        return;
    }
    names->push_back(name);
}

TfTokenVector
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetNames()
{
    TfTokenVector names = _primSource->GetNames();

    if (!_resolvedSkeletonSchema) {
        return names;
    }

    _AddIfNecessary(HdExtComputationPrimvarsSchema::GetSchemaToken(), &names);
    return names;
}

HdMatrixDataSourceHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetPrimWorldToLocal() const {
    return
        _MatrixInverseDataSource::New(
            HdXformSchema::GetFromParent(_primSource).GetMatrix());
}

std::shared_ptr<UsdSkelImagingBlendShapeData>
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetBlendShapeData()
{
    return _blendShapeDataCache.Get();
}

std::shared_ptr<UsdSkelImagingJointInfluencesData>
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetJointInfluencesData()
{
    return _jointInfluencesDataCache.Get();
}

const HdDataSourceLocatorSet &
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetDependendendOnDataSourceLocators()
{
    static const HdDataSourceLocatorSet result{
        UsdSkelImagingBindingSchema::GetDefaultLocator(),
        HdPrimvarsSchema::GetDefaultLocator(),
        HdXformSchema::GetDefaultLocator()
    };

    return result;
}

bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::_ProcessDirtyLocators(
    const HdDataSourceLocatorSet &dirtyLocators,
    HdDataSourceLocatorSet * const dirtyLocatorsForAggregatorComputation,
    HdDataSourceLocatorSet * const dirtyLocatorsForComputation)
{
    if (dirtyLocators.Contains(
            UsdSkelImagingBindingSchema::GetSkeletonLocator())) {
        return true;
    }
    if (dirtyLocators.Contains(
            UsdSkelImagingBindingSchema::GetHasSkelRootLocator())) {
        return true;
    }
    if (dirtyLocators.Contains(
            UsdSkelImagingBindingSchema::GetBlendShapeTargetsLocator())) {
        return true;
    }
    if (dirtyLocators.Contains(
            HdPrimvarsSchema::GetDefaultLocator())) {
        return true;
    }
    static const HdDataSourceLocator skinningMethodLocator =
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(UsdSkelImagingBindingSchemaTokens->skinningMethodPrimvar);
    if (dirtyLocators.Contains(skinningMethodLocator)) {
        return true;
    }

    if (!_resolvedSkeletonSchema) {
        return false;
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingBindingSchema::GetBlendShapesLocator())) {
        _blendShapeDataCache.Invalidate();

        if (dirtyLocatorsForAggregatorComputation) {
            static const HdDataSourceLocatorSet aggregatorInputLocators{
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->blendShapeOffsets),
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->blendShapeOffsetRanges),
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->numBlendShapeOffsetRanges)};
            dirtyLocatorsForAggregatorComputation->insert(
                aggregatorInputLocators);
        }

        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                        ->blendShapeWeights);
            dirtyLocatorsForComputation->insert(
                inputLocator);
        }
    }

    static const HdDataSourceLocatorSet jointInfluencesDataLocators{
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(UsdSkelImagingBindingSchemaTokens->jointIndicesPrimvar),
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(UsdSkelImagingBindingSchemaTokens->jointWeightsPrimvar),
        UsdSkelImagingBindingSchema::GetJointsLocator()};
    if (dirtyLocators.Intersects(
            jointInfluencesDataLocators)) {
        _jointInfluencesDataCache.Invalidate();

        if (dirtyLocatorsForAggregatorComputation) {
            static const HdDataSourceLocatorSet aggregatorInputLocators{
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->hasConstantInfluences),
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->numInfluencesPerComponent),
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->influences)};
            dirtyLocatorsForAggregatorComputation->insert(
                aggregatorInputLocators);
        }
    }

    static const HdDataSourceLocator pointsPrimvarLocator =
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdPrimvarsSchemaTokens->points);
    if (dirtyLocators.Intersects(pointsPrimvarLocator)) {
        if (dirtyLocatorsForAggregatorComputation) {
            static const HdDataSourceLocator aggregatorInputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->restPoints);
            dirtyLocatorsForAggregatorComputation->insert(
                aggregatorInputLocator);
        }

        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocatorSet inputLocators{
                HdExtComputationSchema::GetDispatchCountLocator(),
                HdExtComputationSchema::GetElementCountLocator()};
            dirtyLocatorsForComputation->insert(
                inputLocators);
        }
    }

    static const HdDataSourceLocator geomBindXformPrimvarLocator =
        HdPrimvarsSchema::GetDefaultLocator()
             .Append(
                 UsdSkelImagingBindingSchemaTokens->geomBindTransformPrimvar);
    if (dirtyLocators.Intersects(geomBindXformPrimvarLocator)) {
        if (dirtyLocatorsForAggregatorComputation) {
            static const HdDataSourceLocator aggregatorInputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->geomBindXform);
            dirtyLocatorsForAggregatorComputation->insert(
                aggregatorInputLocator);
        }
    }

    if (dirtyLocators.Intersects(HdXformSchema::GetDefaultLocator())) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                        ->primWorldToLocal);
            dirtyLocatorsForComputation->insert(inputLocator);
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::_ProcessDirtySkeletonLocators(
    const HdDataSourceLocatorSet &dirtyLocators,
    HdDataSourceLocatorSet * const dirtyLocatorsForAggregatorComputation,
    HdDataSourceLocatorSet * const dirtyLocatorsForComputation)
{

    if (dirtyLocators.Contains(
            UsdSkelImagingResolvedSkeletonSchema::GetDefaultLocator())) {
        return true;
    }

    if (!_resolvedSkeletonSchema) {
        return false;
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingResolvedSkeletonSchema::
            GetSkelLocalToWorldLocator())) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                        ->skelLocalToWorld);
            dirtyLocatorsForComputation->insert(
                inputLocator);
        }
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingResolvedSkeletonSchema::
            GetSkinningTransformsLocator())) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                        ->skinningXforms);
            dirtyLocatorsForComputation->insert(
                inputLocator);
        }
    }

    static const HdDataSourceLocatorSet blendLocators{
        UsdSkelImagingResolvedSkeletonSchema::GetBlendShapesLocator(),
        UsdSkelImagingResolvedSkeletonSchema::GetBlendShapeWeightsLocator()};
    if (dirtyLocators.Intersects(blendLocators)) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                        ->blendShapeWeights);
            dirtyLocatorsForComputation->insert(
                inputLocator);
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::_ProcessDirtySkelBlendShapeLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * const dirtyLocatorsForAggregatorComputation,
        HdDataSourceLocatorSet * const dirtyLocatorsForComputation)
{
    if (!_resolvedSkeletonSchema) {
        return false;
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingBlendShapeSchema::GetDefaultLocator())) {
        _blendShapeDataCache.Invalidate();

        if (dirtyLocatorsForAggregatorComputation) {
            static const HdDataSourceLocatorSet aggregatorInputLocators{
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->blendShapeOffsets),
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->blendShapeOffsetRanges),
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->numBlendShapeOffsetRanges)};
            dirtyLocatorsForAggregatorComputation->insert(
                aggregatorInputLocators);
        }

        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                        ->blendShapeWeights);
            dirtyLocatorsForComputation->insert(
                inputLocator);
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::ProcessDirtyLocators(
    const TfToken &dirtiedPrimType,
    const HdDataSourceLocatorSet &dirtyLocators,
    HdSceneIndexObserver::DirtiedPrimEntries * const entries)
{
    HdDataSourceLocatorSet dirtyLocatorsForAggregatorComputation;
    HdDataSourceLocatorSet dirtyLocatorsForComputation;

    HdDataSourceLocatorSet * const
        dirtyLocatorsForAggregatorComputationPtr =
            entries ? &dirtyLocatorsForAggregatorComputation : nullptr;
    HdDataSourceLocatorSet * const
        dirtyLocatorsForComputationPtr =
            entries ? &dirtyLocatorsForComputation : nullptr;

    bool result = false;
    if (dirtiedPrimType == UsdSkelImagingPrimTypeTokens->skeleton) {
        result = _ProcessDirtySkeletonLocators(
            dirtyLocators,
            dirtyLocatorsForAggregatorComputationPtr,
            dirtyLocatorsForComputationPtr);
    } else if (dirtiedPrimType ==
                            UsdSkelImagingPrimTypeTokens->skelBlendShape) {
        result = _ProcessDirtySkelBlendShapeLocators(
            dirtyLocators,
            dirtyLocatorsForAggregatorComputationPtr,
            dirtyLocatorsForComputationPtr);
    } else {
        result = _ProcessDirtyLocators(
            dirtyLocators,
            dirtyLocatorsForAggregatorComputationPtr,
            dirtyLocatorsForComputationPtr);
    }

    if (entries) {
        bool sendPointsPrimvarValueDirty = false;

        if (!dirtyLocatorsForAggregatorComputation.IsEmpty()) {
            entries->push_back({
                _primPath.AppendChild(
                    UsdSkelImagingExtComputationNameTokens
                    ->aggregatorComputation),
                std::move(dirtyLocatorsForAggregatorComputation)});
            sendPointsPrimvarValueDirty = true;
        }
        if (!dirtyLocatorsForComputation.IsEmpty()) {
            entries->push_back({
                _primPath.AppendChild(
                    UsdSkelImagingExtComputationNameTokens->computation),
                std::move(dirtyLocatorsForComputation)});
            sendPointsPrimvarValueDirty = true;
        }

        if (sendPointsPrimvarValueDirty) {
            static const HdDataSourceLocator locator =
                HdPrimvarsSchema::GetDefaultLocator()
                    .Append(HdPrimvarsSchemaTokens->points)
                    .Append(HdPrimvarSchemaTokens->primvarValue);
            entries->push_back({ _primPath, locator});
        }
    }

    return result;
}

UsdSkelImagingDataSourceResolvedPointsBasedPrim::
_BlendShapeDataCache::_BlendShapeDataCache(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath)
 : _sceneIndex(sceneIndex)
 , _primPath(primPath)
{
}

std::shared_ptr<UsdSkelImagingBlendShapeData>
UsdSkelImagingDataSourceResolvedPointsBasedPrim::
_BlendShapeDataCache::_Compute()
{
    return
        std::make_shared<UsdSkelImagingBlendShapeData>(
            UsdSkelImagingComputeBlendShapeData(
                _sceneIndex, _primPath));
}

UsdSkelImagingDataSourceResolvedPointsBasedPrim::
_JointInfluencesDataCache::_JointInfluencesDataCache(
    HdContainerDataSourceHandle const &primSource,
    HdContainerDataSourceHandle const &skeletonPrimSource)
 : _primSource(primSource)
 , _skeletonPrimSource(skeletonPrimSource)
{
}

std::shared_ptr<UsdSkelImagingJointInfluencesData>
UsdSkelImagingDataSourceResolvedPointsBasedPrim::
_JointInfluencesDataCache::_Compute()
{
    return
        std::make_shared<UsdSkelImagingJointInfluencesData>(
            UsdSkelImagingComputeJointInfluencesData(
                _primSource, _skeletonPrimSource));
}

PXR_NAMESPACE_CLOSE_SCOPE
