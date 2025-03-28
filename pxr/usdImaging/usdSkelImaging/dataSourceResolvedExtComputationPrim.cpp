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

#include "pxr/base/gf/dualQuatf.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/quaternion.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

template<typename T>
HdDataSourceBaseHandle
_ToDataSource(const T &value)
{
    return HdRetainedTypedSampledDataSource<T>::New(value);
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
        return UsdSkelImagingExtAggregatorComputationInputNameTokens->allTokens;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        TRACE_FUNCTION();

        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->restPoints) {
            // Simply use the primvar value data source from the prim from
            // the input scene.
            return _GetPrimvarValueDataSource(HdPrimvarsSchemaTokens->points);
        }

        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->geomBindXform) {
            // Use the primvar value from the prim from the input scene.
            // But convert to GfMatrix4f.
            return
                _ToDataSource(_GetGeomBindXform());
        }

        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->hasConstantInfluences) {
            return
                _ToDataSource(_GetJointInfluencesData()->hasConstantInfluences);
        }

        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->numInfluencesPerComponent) {
            return
                _ToDataSource(_GetJointInfluencesData()->numInfluencesPerComponent);
        }

        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->influences) {
            return
                _ToDataSource(_GetJointInfluencesData()->influences);
        }

        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->blendShapeOffsets) {
            return
                _ToDataSource(_GetBlendShapeData()->blendShapeOffsets);
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->blendShapeOffsetRanges) {
            return
                _ToDataSource(_GetBlendShapeData()->blendShapeOffsetRanges);
        }
        if (name == UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->numBlendShapeOffsetRanges) {
            return
                _ToDataSource<int>(
                    _GetBlendShapeData()->blendShapeOffsetRanges.size());
        }

        return nullptr;
    }

private:
    _ExtAggregatorComputationInputValuesDataSource(
        UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource)
     : _resolvedPrimSource(std::move(resolvedPrimSource))
    {
    }

    std::shared_ptr<UsdSkelImagingBlendShapeData> _GetBlendShapeData()
    {
        return _resolvedPrimSource->GetBlendShapeData();
    }

    std::shared_ptr<UsdSkelImagingJointInfluencesData> _GetJointInfluencesData()
    {
        return _resolvedPrimSource->GetJointInfluencesData();
    }

    HdSampledDataSourceHandle _GetPrimvarValueDataSource(const TfToken &name) {
        TRACE_FUNCTION();

        return
            _resolvedPrimSource
                ->GetPrimvars().GetPrimvar(name).GetPrimvarValue();
    }

    GfMatrix4f _GetGeomBindXform() {
        TRACE_FUNCTION();

        auto const ds = HdMatrixDataSource::Cast(
            _GetPrimvarValueDataSource(
                UsdSkelImagingBindingSchemaTokens->geomBindTransformPrimvar));
        if (!ds) {
            return GfMatrix4f(1.0f);
        }
        return GfMatrix4f(ds->GetTypedValue(0.0f));
    }

    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle const _resolvedPrimSource;
};

// Data source for locator extComputations:inputValues:blendShapeWeights on
// skinningComputation prim.
class _BlendShapeWeightsDataSource : public HdFloatArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BlendShapeWeightsDataSource);

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtArray<float>
    GetTypedValue(const HdSampledDataSource::Time shutterOffset) override {
        TRACE_FUNCTION();

        return UsdSkelImagingComputeBlendShapeWeights(
            *_blendShapeData,
            UsdSkelImagingGetTypedValue(_blendShapes, shutterOffset),
            UsdSkelImagingGetTypedValue(_blendShapeWeights, shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override
    {
        TRACE_FUNCTION();

        if (!_blendShapeWeights) {
            return false;
        }

        return
            _blendShapeWeights->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

private:
    _BlendShapeWeightsDataSource(
        std::shared_ptr<UsdSkelImagingBlendShapeData> blendShapeData,
        HdTokenArrayDataSourceHandle blendShapes,
        HdFloatArrayDataSourceHandle blendShapeWeights)
     : _blendShapeData(std::move(blendShapeData))
     , _blendShapes(std::move(blendShapes))
     , _blendShapeWeights(std::move(blendShapeWeights))
    {
    }

    std::shared_ptr<UsdSkelImagingBlendShapeData> const _blendShapeData;
    HdTokenArrayDataSourceHandle const _blendShapes;
    HdFloatArrayDataSourceHandle const _blendShapeWeights;
};

// Data source for locator extComputations:inputValues:skinningXforms on
// skinningComputation prim.
//
// Takes skinnigXforms from resolved skeleton schema (in skelSkinningXforms)
// applies jointMapper from jointInfluencesData.
//
class _SkinningXformsDataSource : public HdMatrix4fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SkinningXformsDataSource);

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtMatrix4fArray
    GetTypedValue(const HdSampledDataSource::Time shutterOffset) override {
        TRACE_FUNCTION();

        VtMatrix4fArray result;
        _jointInfluencesData->jointMapper.RemapTransforms(
            _skelSkinningXforms->GetTypedValue(shutterOffset),
            &result);
        return result;
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override
    {
        TRACE_FUNCTION();

        return _skelSkinningXforms->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _SkinningXformsDataSource(
        std::shared_ptr<UsdSkelImagingJointInfluencesData> jointInfluencesData,
        HdMatrix4fArrayDataSourceHandle skelSkinningXforms)
     : _jointInfluencesData(std::move(jointInfluencesData))
     , _skelSkinningXforms(std::move(skelSkinningXforms))
    {
    }

    std::shared_ptr<UsdSkelImagingJointInfluencesData> const _jointInfluencesData;
    HdMatrix4fArrayDataSourceHandle const _skelSkinningXforms;
};

// Extract the Scale & Shear parts of 4x4 matrix by removing the
// translation & rotation. Return only the upper-left 3x3 matrix.
GfMatrix3f
_ComputeSkinningScaleXform(const GfMatrix4f &skinningXform)
{
    GfMatrix4f scaleOrientMat, factoredRotMat, perspMat;
    GfVec3f scale, translation;

    // From _ExtractSkinningScaleXforms in skeletonAdapter.cpp
    if (!skinningXform.Factor(
            &scaleOrientMat, &scale, &factoredRotMat,
            &translation, &perspMat)) {
        // Unable to decompose.
        return GfMatrix3f(1.0f);
    }

    // Remove shear & extract rotation
    factoredRotMat.Orthonormalize();
            // Calculate the scale + shear transform

    const GfMatrix4f tmpNonScaleXform =
        factoredRotMat * GfMatrix4f(1.0).SetTranslate(translation);
    return
        (skinningXform * tmpNonScaleXform.GetInverse()).
            ExtractRotationMatrix();   // Extract the upper-left 3x3 matrix
}

// Extract the Scale & Shear parts of 4x4 matrices by removing the
// translation & rotation. Return only the upper-left 3x3 matrices.
VtArray<GfMatrix3f>
_ComputeSkinningScaleXforms(const VtArray<GfMatrix4f> &skinningXforms)
{
    const GfMatrix4f * const skinningXformsData = skinningXforms.data();

    VtArray<GfMatrix3f> result;
    result.resize(
        skinningXforms.size(),
        [skinningXformsData](
            GfMatrix3f * const begin, GfMatrix3f * const end) {
            const GfMatrix4f * skinningXform = skinningXformsData;
            for (GfMatrix3f * skinningScaleXform = begin;
                 skinningScaleXform < end;
                 ++skinningScaleXform) {

                new (skinningScaleXform) GfMatrix3f(
                    _ComputeSkinningScaleXform(*skinningXform));

                ++skinningXform;
            }
        });
    return result;
}

// Data source for locator extComputations:inputValues:skinningScaleXforms on
// skinningComputation prim.
class _SkinningScaleXformsDataSource : public HdMatrix3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SkinningScaleXformsDataSource);

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtArray<GfMatrix3f>
    GetTypedValue(const HdSampledDataSource::Time shutterOffset) override {
        TRACE_FUNCTION();

        return
            _ComputeSkinningScaleXforms(
                UsdSkelImagingGetTypedValue(_skinningXforms, shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override
    {
        TRACE_FUNCTION();

        if (!_skinningXforms) {
            return false;
        }

        return
            _skinningXforms->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

private:
    _SkinningScaleXformsDataSource(
        HdMatrix4fArrayDataSourceHandle skinningXforms)
     : _skinningXforms(skinningXforms)
    {
    }

    HdMatrix4fArrayDataSourceHandle const _skinningXforms;
};

GfQuatf
_ToGfQuatf(const GfQuaternion &q)
{
    return {static_cast<float>(q.GetReal()), GfVec3f(q.GetImaginary())};
}

// Extract the translation & rotation parts of 4x4 matrix into dual quaternion.
GfDualQuatf
_ComputeSkinningDualQuat(const GfMatrix4f &skinningXform)
{
    // Taken from _ExtractSkinningDualQuats in skeletonAdapter.cpp

    GfMatrix4f scaleOrientMat, factoredRotMat, perspMat;
    GfVec3f scale, translation;
    if (!skinningXform.Factor(
            &scaleOrientMat, &scale, &factoredRotMat,
            &translation, &perspMat)) {
        // Unable to decompose.
        return GfDualQuatf::GetZero();
    }

    // Remove shear & extract rotation
    factoredRotMat.Orthonormalize();
    const GfQuaternion rotationQ =
        factoredRotMat.ExtractRotationMatrix().ExtractRotationQuaternion();
    return {_ToGfQuatf(rotationQ), translation};
}

GfVec4f
_ToVec4f(const GfQuatf &q)
{
    const GfVec3f &img = q.GetImaginary();
    return { img[0], img[1], img[2], q.GetReal() };
}

// Use a pair of Vec4f to represent a dual quaternion.
VtArray<GfVec4f>
_ComputeSkinningDualQuats(const VtArray<GfMatrix4f> &skinningXforms)
{
    const GfMatrix4f * const skinningXformsData = skinningXforms.data();

    VtArray<GfVec4f> result;
    result.resize(
        2 * skinningXforms.size(),
        [skinningXformsData](
            GfVec4f * const begin, GfVec4f * const end) {
            const GfMatrix4f * skinningXform = skinningXformsData;
            for (GfVec4f * skinningDualQuat = begin;
                 skinningDualQuat < end;
                 skinningDualQuat += 2) {

                const GfDualQuatf dq = _ComputeSkinningDualQuat(*skinningXform);

                new (skinningDualQuat    ) GfVec4f(_ToVec4f(dq.GetReal()));
                new (skinningDualQuat + 1) GfVec4f(_ToVec4f(dq.GetDual()));

                ++skinningXform;
            }
        });
    return result;
}

// Data source for locator extComputations:inputValues:skinningDualQuats on
// skinningComputation prim.
class _SkinningDualQuatsDataSource : public HdVec4fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SkinningDualQuatsDataSource);

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtArray<GfVec4f>
    GetTypedValue(const HdSampledDataSource::Time shutterOffset) override {
        TRACE_FUNCTION();

        return
            _ComputeSkinningDualQuats(
                UsdSkelImagingGetTypedValue(_skinningXforms, shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override
    {
        TRACE_FUNCTION();

        if (!_skinningXforms) {
            return false;
        }

        return
            _skinningXforms->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

private:
    _SkinningDualQuatsDataSource(
        HdMatrix4fArrayDataSourceHandle skinningXforms)
     : _skinningXforms(skinningXforms)
    {
    }

    HdMatrix4fArrayDataSourceHandle const _skinningXforms;
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
    HD_DECLARE_DATASOURCE(
        _ExtComputationInputValuesDataSource);

    TfTokenVector GetNames() override {
        if (_GetSkinningMethod() == UsdSkelTokens->dualQuaternion) {
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
                                ->primWorldToLocal) {
            // Typed sampled data source holding inverse of xform:matrix from
            // prim from input scene.
            return _resolvedPrimSource->GetPrimWorldToLocal();

        }

        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->blendShapeWeights) {
            return _BlendShapeWeightsDataSource::New(
                _resolvedPrimSource->GetBlendShapeData(),
                _GetResolvedSkeletonSchema().GetBlendShapes(),
                _GetResolvedSkeletonSchema().GetBlendShapeWeights());
        }

        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->skinningXforms) {
            return _GetSkinningXforms();
        }

        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->skinningScaleXforms) {
            if (_GetSkinningMethod() != UsdSkelTokens->dualQuaternion) {
                return nullptr;
            }
            return _SkinningScaleXformsDataSource::New(_GetSkinningXforms());
        }

        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->skinningDualQuats) {
            if (_GetSkinningMethod() != UsdSkelTokens->dualQuaternion) {
                return nullptr;
            }
            return _SkinningDualQuatsDataSource::New(_GetSkinningXforms());
        }

        if (name == UsdSkelImagingExtComputationInputNameTokens
                                ->skelLocalToWorld) {
            return _GetResolvedSkeletonSchema().GetSkelLocalToWorld();
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

    const TfToken &_GetSkinningMethod()
    {
        return _resolvedPrimSource->GetSkinningMethod();
    }

    HdMatrix4fArrayDataSourceHandle _GetSkinningXforms()
    {
        // Apply jointMapper to skinning xforms from resolved skeleton
        // if necessary.

        HdMatrix4fArrayDataSourceHandle skelSkinningXforms =
            _GetResolvedSkeletonSchema().GetSkinningTransforms();
        if (!skelSkinningXforms) {
            return nullptr;
        }

        std::shared_ptr<UsdSkelImagingJointInfluencesData> jointInfluencesData =
            _resolvedPrimSource->GetJointInfluencesData();
        if (jointInfluencesData->jointMapper.IsNull() ||
            jointInfluencesData->jointMapper.IsIdentity()) {
            return skelSkinningXforms;
        }

        return _SkinningXformsDataSource::New(
            std::move(jointInfluencesData), std::move(skelSkinningXforms));
    }

    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle const _resolvedPrimSource;
};

// Data source for locator extComputations:dispatchCount and
// extComputations:elementCount on skinningComputation prim.

class _NumPointsDataSource : public HdSizetDataSource
{
public:
    HD_DECLARE_DATASOURCE(_NumPointsDataSource);

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    size_t GetTypedValue(const HdSampledDataSource::Time shutterOffset) override {
        TRACE_FUNCTION();

        HdSampledDataSourceHandle const ds = _GetPoints();
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

        HdSampledDataSourceHandle const ds = _GetPoints();
        if (!ds) {
            return false;
        }
        return ds->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _NumPointsDataSource(const HdPrimvarsSchema &primvars)
     : _primvars(primvars)
    {
    }

    HdSampledDataSourceHandle _GetPoints() {
        return _primvars.GetPrimvar(HdPrimvarsSchemaTokens->points)
                                .GetPrimvarValue();
    }

    const HdPrimvarsSchema _primvars;
};

// Prim data source skinningInputAggregatorComputation prim.
HdContainerDataSourceHandle
_ExtAggregatorComputationPrimDataSource(
    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource)
{
    TRACE_FUNCTION();

    return
        HdRetainedContainerDataSource::New(
            HdExtComputationSchema::GetSchemaToken(),
            HdExtComputationSchema::Builder()
                .SetInputValues(
                    _ExtAggregatorComputationInputValuesDataSource::New(
                        std::move(resolvedPrimSource)))
                .Build());
}

// Data source for locator extComputation:inputComputations on
// skinningComputation prim.
HdContainerDataSourceHandle
_ExtComputationInputComputations(const SdfPath &primPath)
{
    TRACE_FUNCTION();

    static const TfTokenVector names =
        UsdSkelImagingExtAggregatorComputationInputNameTokens->allTokens;

    HdPathDataSourceHandle const pathSrc =
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            primPath.AppendChild(
                UsdSkelImagingExtComputationNameTokens->aggregatorComputation));

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
_ExtComputationOutputs()
{
    static const TfToken names[] = {
        UsdSkelImagingExtComputationOutputNameTokens->skinnedPoints
    };
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
    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource)
{
    TRACE_FUNCTION();

    static HdContainerDataSourceHandle const outputs =
        _ExtComputationOutputs();
    const SdfPath &primPath = resolvedPrimSource->GetPrimPath();
    HdSizetDataSourceHandle const numPoints =
        _NumPointsDataSource::New(resolvedPrimSource->GetPrimvars());

    return
        HdRetainedContainerDataSource::New(
            HdExtComputationSchema::GetSchemaToken(),
            HdExtComputationSchema::Builder()
                .SetInputValues(
                    _ExtComputationInputValuesDataSource::New(
                        resolvedPrimSource))
                .SetInputComputations(
                    _ExtComputationInputComputations(
                        primPath))
                .SetOutputs(
                    outputs)
                .SetGlslKernel(
                    UsdSkelImagingExtComputationGlslKernel(
                        resolvedPrimSource->GetSkinningMethod()))
                .SetCpuCallback(
                    UsdSkelImagingExtComputationCpuCallback(
                        resolvedPrimSource->GetSkinningMethod()))
                .SetDispatchCount(numPoints)
                .SetElementCount(numPoints)
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
                                ->computation) {
        return
            _ExtComputationPrimDataSource(
                std::move(resolvedPrimSource));
    }
    if (computationName == UsdSkelImagingExtComputationNameTokens
                                ->aggregatorComputation) {
        return
            _ExtAggregatorComputationPrimDataSource(
                std::move(resolvedPrimSource));
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
