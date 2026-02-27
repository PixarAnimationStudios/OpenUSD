//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourceResolvedExtComputationPrim.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/blendShapeData.h"
#include "pxr/usdImaging/usdSkelImaging/extComputations.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceResolvedPointsBasedPrim.h"
#include "pxr/usdImaging/usdSkelImaging/jointInfluencesData.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

#include "pxr/usd/usdSkel/tokens.h"

#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TfTokenVector
_ExtComputationInputNamesForPoints()
{
    TfTokenVector result;
    for (const TfToken &name :
             UsdSkelImagingExtAggregatorComputationInputNameTokens->allTokens) {
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->restNormals) {
            continue;
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->faceVertexIndices) {
            continue;
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->hasFaceVaryingNormals) {
            continue;
        }
        result.push_back(name);
    }
    return result;
}

TfTokenVector
_ExtComputationInputNamesForNormals()
{
    TfTokenVector result;
    for (const TfToken &name :
             UsdSkelImagingExtAggregatorComputationInputNameTokens->allTokens) {
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->restPoints) {
            continue;
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->blendShapeOffsets) {
            continue;
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->blendShapeOffsetRanges) {
            continue;
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                        ->numBlendShapeOffsetRanges) {
            continue;
        }
        result.push_back(name);
    }
    return result;
}

// Data source for locator extComputation:inputValues on
// skinningInputAggregatorComputation prim.
class _ExtAggregatorComputationInputValuesDataSource
      : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(
        _ExtAggregatorComputationInputValuesDataSource);

    TfTokenVector GetNames() override
    {
        // TODO
        // We might wanna consider reusing HdSkinningInputTokens here.
        if (_computationType == UsdSkelImagingExtComputationTypeTokens
                                    ->points) {
            static const TfTokenVector pointInputNames =
                _ExtComputationInputNamesForPoints();
            return pointInputNames;
        } else {
            static const TfTokenVector normalInputNames =
                _ExtComputationInputNamesForNormals();
            return normalInputNames;
        }
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        TRACE_FUNCTION();

        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->restPoints) {
            return _resolvedPrimSource->GetPoints();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->geomBindXform) {
            return _resolvedPrimSource->GetGeomBindTransform();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->hasConstantInfluences) {
            return _resolvedPrimSource->GetHasConstantInfluences();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->numInfluencesPerComponent) {
            return _resolvedPrimSource->GetNumInfluencesPerComponent();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->influences) {
            return _resolvedPrimSource->GetInfluences();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->blendShapeOffsets) {
            return _resolvedPrimSource->GetBlendShapeOffsets();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->blendShapeOffsetRanges) {
            return _resolvedPrimSource->GetBlendShapeOffsetRanges();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->numBlendShapeOffsetRanges) {
            return _resolvedPrimSource->GetNumBlendShapeOffsetRanges();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->restNormals) {
            return _resolvedPrimSource->GetNormals();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->faceVertexIndices) {
            return _resolvedPrimSource->GetFaceVertexIndices();
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->hasFaceVaryingNormals) {
            return _resolvedPrimSource->GetHasFaceVaryingNormals();
        }

        return nullptr;
    }

private:
    _ExtAggregatorComputationInputValuesDataSource(
        UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource,
        const TfToken& computationType)
      : _resolvedPrimSource(std::move(resolvedPrimSource))
      , _computationType(computationType)
    {
    }

    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle const _resolvedPrimSource;
    const TfToken _computationType;
};

TfTokenVector
_ExtComputationInputNamesForClassicLinear()
{
    TfTokenVector result;
    for (const TfToken &name :
             UsdSkelImagingExtComputationInputNameTokens->allTokens) {
        if (name == UsdSkelImagingExtComputationInputNameTokens
                        ->skinningScaleXforms) {
            continue;
        }
        if (name == UsdSkelImagingExtComputationInputNameTokens
                        ->skinningDualQuats) {
            continue;
        }
        result.push_back(name);
    }
    return result;
}

class _ExtComputationInputValuesDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ExtComputationInputValuesDataSource);

    TfTokenVector GetNames() override {
        // TODO
        // We might wanna consider reusing HdSkinningInputTokens here.
        if (_IsDualQuatSkinning()) {
            return UsdSkelImagingExtComputationInputNameTokens->allTokens;
        } else {
            static const TfTokenVector result =
                _ExtComputationInputNamesForClassicLinear();
            return result;
        }
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        TRACE_FUNCTION();

        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->commonSpaceToPrimLocal) {
            // Typed sampled data source holding inverse of xform:matrix from
            // prim from input scene.
            return _resolvedPrimSource->GetCommonSpaceToPrimLocal();
        }
        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->blendShapeWeights) {
            return _resolvedPrimSource->GetBlendShapeWeights();
        }
        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->skinningXforms) {
            return _resolvedPrimSource->GetSkinningTransforms();
        }
        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->skinningScaleXforms) {
            return _IsDualQuatSkinning() ? 
                _resolvedPrimSource->GetSkinningScaleTransforms() : nullptr;
        }
        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->skinningDualQuats) {
            return _IsDualQuatSkinning() ? 
                _resolvedPrimSource->GetSkinningDualQuats() : nullptr;
        }
        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->skelLocalToCommonSpace) {
            return _GetResolvedSkeletonSchema().GetSkelLocalToCommonSpace();
        }

        return nullptr;
    }

private:
    _ExtComputationInputValuesDataSource(
        UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource)
     : _resolvedPrimSource(std::move(resolvedPrimSource))
    {
    }

    const UsdSkelImagingResolvedSkeletonSchema &_GetResolvedSkeletonSchema()
    {
        return _resolvedPrimSource->GetResolvedSkeletonSchema();
    }

    bool _IsDualQuatSkinning() const
    {
        return _resolvedPrimSource->GetSkinningMethod() == 
            UsdSkelTokens->dualQuaternion;
    }

    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle const _resolvedPrimSource;
};

// Data source for locator extComputations:dispatchCount and
// extComputations:elementCount on skinningComputation prim.

class _NumElementsDataSource : public HdSizetDataSource
{
public:
    HD_DECLARE_DATASOURCE(_NumElementsDataSource);

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    size_t GetTypedValue(const HdSampledDataSource::Time shutterOffset) override
    {
        TRACE_FUNCTION();

        HdSampledDataSourceHandle const ds = _GetPrimvarValue();
        if (!ds) {
            return 0;
        }
        return ds->GetValue(shutterOffset).GetArraySize();
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override
    {
        TRACE_FUNCTION();

        HdSampledDataSourceHandle const ds = _GetPrimvarValue();
        if (!ds) {
            return false;
        }
        return ds->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _NumElementsDataSource(
        const HdPrimvarsSchema &primvars,
        const TfToken &primvarName)
      : _primvars(primvars)
      , _primvarName(primvarName)
    {
    }

    HdSampledDataSourceHandle _GetPrimvarValue()
    {
        return _primvars.GetPrimvar(_primvarName).GetPrimvarValue();
    }

    const HdPrimvarsSchema _primvars;
    const TfToken _primvarName;
};

// Prim data source skinningInputAggregatorComputation prim.
HdContainerDataSourceHandle
_ExtAggregatorComputationPrimDataSource(
    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource,
    const TfToken &computationType)
{
    TRACE_FUNCTION();

    return
        HdRetainedContainerDataSource::New(
            HdExtComputationSchema::GetSchemaToken(),
            HdExtComputationSchema::Builder()
                .SetInputValues(
                    _ExtAggregatorComputationInputValuesDataSource::New(
                        std::move(resolvedPrimSource),
                        computationType))
                .Build());
}

// Data source for locator extComputation:inputComputations on
// skinningComputation prim.
HdContainerDataSourceHandle
_ExtComputationInputComputations(
    const SdfPath &primPath,
    const TfToken &computationType)
{
    TRACE_FUNCTION();

    static const TfTokenVector pointInputNames =
        _ExtComputationInputNamesForPoints();
    static const TfTokenVector normalInputNames =
        _ExtComputationInputNamesForNormals();

    const TfTokenVector &names =
        (computationType == UsdSkelImagingExtComputationTypeTokens->points)
            ? pointInputNames
            : normalInputNames;

    const TfToken computationName = 
        (computationType == UsdSkelImagingExtComputationTypeTokens->points)
            ? UsdSkelImagingExtComputationNameTokens
                ->pointsAggregatorComputation
            : UsdSkelImagingExtComputationNameTokens
                ->normalsAggregatorComputation;

    HdPathDataSourceHandle const pathSrc =
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            primPath.AppendChild(
                computationName));

    std::vector<HdDataSourceBaseHandle> values;
    values.reserve(names.size());

    for (const TfToken &name : names) {
        values.push_back(
            HdExtComputationInputComputationSchema::Builder()
                .SetSourceComputation(pathSrc)
                .SetSourceComputationOutputName(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        name))
                .Build());
    }

    return
        HdExtComputationInputComputationContainerSchema::BuildRetained(
            names.size(), names.data(), values.data());
}

// Data source for locator extComputation:outputs on
// skinningComputation prim.
HdContainerDataSourceHandle
_ExtComputationOutputs(
    const TfToken& computationType)
{
    const TfToken computationOutput =
        (computationType == UsdSkelImagingExtComputationTypeTokens->points)
            ? UsdSkelImagingExtComputationOutputNameTokens->skinnedPoints
            : UsdSkelImagingExtComputationOutputNameTokens->skinnedNormals;
    const TfToken names[] = { computationOutput };
    HdDataSourceBaseHandle const values[] = {
        HdExtComputationOutputSchema::Builder()
            .SetValueType(
                HdRetainedTypedSampledDataSource<HdTupleType>::New(
                    HdTupleType{HdTypeFloatVec3, 1}))
            .Build()
    };

    static_assert(std::size(names) == std::size(values));
    return
        HdExtComputationOutputContainerSchema::BuildRetained(
            std::size(names), names, values);
}

// Prim data source skinningComputation prim.
HdContainerDataSourceHandle
_ExtComputationPrimDataSource(
    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource,
    const TfToken &computationType)
{
    TRACE_FUNCTION();

    const TfToken primvarName = 
        (computationType == UsdSkelImagingExtComputationTypeTokens->points)
            ? HdPrimvarsSchemaTokens->points
            : HdPrimvarsSchemaTokens->normals;
    HdSizetDataSourceHandle const elementCount =
        _NumElementsDataSource::New(
            resolvedPrimSource->GetPrimvars(),
            primvarName);

    return
        HdRetainedContainerDataSource::New(
            HdExtComputationSchema::GetSchemaToken(),
            HdExtComputationSchema::Builder()
                .SetInputValues(
                    _ExtComputationInputValuesDataSource::New(
                        resolvedPrimSource))
                .SetInputComputations(
                    _ExtComputationInputComputations(
                        resolvedPrimSource->GetPrimPath(),
                        computationType))
                .SetOutputs(
                    _ExtComputationOutputs(
                        computationType))
                .SetGlslKernel(
                    UsdSkelImagingExtComputationGlslKernel(
                        resolvedPrimSource->GetSkinningMethod(),
                        computationType))
                .SetCpuCallback(
                    UsdSkelImagingExtComputationCpuCallback(
                        resolvedPrimSource->GetSkinningMethod()))
                .SetDispatchCount(elementCount)
                .SetElementCount(elementCount)
                .Build());
}

}

HdContainerDataSourceHandle
UsdSkelImagingDataSourceResolvedExtComputationPrim(
    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource,
    const TfToken &computationName)
{
    TRACE_FUNCTION();

    if (computationName == UsdSkelImagingExtComputationNameTokens
                                ->pointsComputation) {
        return
            _ExtComputationPrimDataSource(
                std::move(resolvedPrimSource),
                UsdSkelImagingExtComputationTypeTokens->points);
    }
    if (computationName == UsdSkelImagingExtComputationNameTokens
                                ->normalsComputation) {
        return
            _ExtComputationPrimDataSource(
                std::move(resolvedPrimSource),
                UsdSkelImagingExtComputationTypeTokens->normals);
    }
    if (computationName == UsdSkelImagingExtComputationNameTokens
                                ->pointsAggregatorComputation) {
        return
            _ExtAggregatorComputationPrimDataSource(
                std::move(resolvedPrimSource),
                UsdSkelImagingExtComputationTypeTokens->points);
    }
    if (computationName == UsdSkelImagingExtComputationNameTokens
                                ->normalsAggregatorComputation) {
        return
            _ExtAggregatorComputationPrimDataSource(
                std::move(resolvedPrimSource),
                UsdSkelImagingExtComputationTypeTokens->normals);
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
