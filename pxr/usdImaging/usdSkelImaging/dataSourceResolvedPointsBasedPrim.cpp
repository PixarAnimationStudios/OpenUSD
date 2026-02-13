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
#include "pxr/usdImaging/usdSkelImaging/dataSourcePrimvar.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"
#include "pxr/usdImaging/usdSkelImaging/extComputations.h"
#include "pxr/usdImaging/usdSkelImaging/jointInfluencesData.h"
#include "pxr/usdImaging/usdSkelImaging/skeletonSchema.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

#include "pxr/usd/usdSkel/tokens.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/skinningSettings.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/gf/dualQuatf.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/quaternion.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

template<typename T>
HdDataSourceBaseHandle
_ToDataSource(const T& value)
{
    return HdRetainedTypedSampledDataSource<T>::New(value);
}

/// GfMatrix4d-typed sampled data source giving inverse matrix for
/// given data source (of same type).
class _MatrixInverseDataSource : public HdMatrixDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MatrixInverseDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        TRACE_FUNCTION();
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(const Time shutterOffset) override {
        TRACE_FUNCTION();

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

    GfMatrix4d _Compute(const Time shutterOffset) {
        TRACE_FUNCTION();

        if (!_inputSrc) {
            return GfMatrix4d(1.0);
        }
        return _inputSrc->GetTypedValue(shutterOffset).GetInverse();
    }

    HdMatrixDataSourceHandle const _inputSrc;
    const GfMatrix4d _valueAtZero;
};

} // namespace

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
    TRACE_FUNCTION();

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
 , _mesh(HdMeshSchema::GetFromParent(_primSource))
 , _skinningMethod(_GetSkinningMethod(_primvars, primPath))
 , _blendShapeTargetPaths(std::move(blendShapeTargetPaths))
 , _skeletonPath(std::move(skeletonPath))
 , _skeletonPrimSource(std::move(skeletonPrimSource))
 , _resolvedSkeletonSchema(std::move(resolvedSkeletonSchema))
 , _blendShapeDataCache(sceneIndex, primPath)
 , _jointInfluencesDataCache(_primSource, _skeletonPrimSource)
 , _xformResolver(sceneIndex, _primSource)
{
}

UsdSkelImagingDataSourceResolvedPointsBasedPrim::
~UsdSkelImagingDataSourceResolvedPointsBasedPrim() = default;

namespace
{

HdContainerDataSourceHandle
_ExtComputationPrimvars(
    const SdfPath &primPath,
    const bool haveNormalsComputations,
    const HdPrimvarsSchema &primvars)
{
    TfTokenVector names = {
        HdPrimvarsSchemaTokens->points
    };
    std::vector<HdDataSourceBaseHandle> values = {
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
                        UsdSkelImagingExtComputationNameTokens
                            ->pointsComputation)))
            .SetSourceComputationOutputName(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    UsdSkelImagingExtComputationOutputNameTokens
                        ->skinnedPoints))
            .SetValueType(
                HdRetainedTypedSampledDataSource<HdTupleType>::New(
                    HdTupleType{HdTypeFloatVec3, 1}))
        .Build()
    };

    if (haveNormalsComputations) {
        const HdTokenDataSourceHandle normalsInterpolationDs =
            primvars.GetPrimvar(HdPrimvarsSchemaTokens->normals)
                .GetInterpolation();
        const TfToken normalsInterpolation =
            normalsInterpolationDs
                ? normalsInterpolationDs->GetTypedValue(0.0f)
                : HdPrimvarSchemaTokens->vertex;

        names.push_back(HdPrimvarsSchemaTokens->normals);
        values.push_back(
            HdExtComputationPrimvarSchema::Builder()
                .SetInterpolation(
                    HdExtComputationPrimvarSchema::BuildInterpolationDataSource(
                        normalsInterpolation))
                .SetRole(
                    HdExtComputationPrimvarSchema::BuildRoleDataSource(
                        HdPrimvarSchemaTokens->normal))
                .SetSourceComputation(
                    HdRetainedTypedSampledDataSource<SdfPath>::New(
                        primPath.AppendChild(
                            UsdSkelImagingExtComputationNameTokens
                                ->normalsComputation)))
                .SetSourceComputationOutputName(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        UsdSkelImagingExtComputationOutputNameTokens
                            ->skinnedNormals))
                .SetValueType(
                    HdRetainedTypedSampledDataSource<HdTupleType>::New(
                        HdTupleType{HdTypeFloatVec3, 1}))
            .Build());
    }

    return HdExtComputationPrimvarsSchema::BuildRetained(
        names.size(), names.data(), values.data());
}

HdContainerDataSourceHandle
_BlockPointsAndNormalsPrimvars()
{
    const TfToken names[] = {
        HdPrimvarsSchemaTokens->points,
        HdPrimvarsSchemaTokens->normals
    };
    HdDataSourceBaseHandle const values[] = {
        HdBlockDataSource::New(),
        HdBlockDataSource::New()
    };

    static_assert(std::size(names) == std::size(values));
    return HdPrimvarsSchema::BuildRetained(std::size(names), names, values);
}


class _SkinningPrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SkinningPrimvarsDataSource);

    TfTokenVector GetNames() override {
        // XXX
        // should prob check if skinningMethod primvar actually exists, it's 
        // possible that it got aggregated to instance primvar on the instancer.
        // if it doesn't exist then we create the fall back primvar here.
        // enumerate skinningMethod because we can't pass down existing
        // Token primvar to vertex shader.
        // also, base on that, we should decide we want to filter out
        // dualQuat or skinningXforms inputs. see:
        // _ExtComputationInputNamesForClassicLinear() in
        // dataSourceResolvedExtComputationPrim.cpp.
        static const TfTokenVector names = 
            HdSkinningSettings::GetSkinningInputNames();
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        TRACE_FUNCTION();

        // ExtComputationInputValues
        if (name == HdSkinningInputTokens->skinningXforms) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetSkinningTransforms());
        }
        if (name == HdSkinningInputTokens->skinningScaleXforms) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetSkinningScaleTransforms());
        }
        if (name == HdSkinningInputTokens->skinningDualQuats) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetSkinningDualQuats());
        }
        if (name == HdSkinningInputTokens->blendShapeWeights) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetBlendShapeWeights());
        }
        if (name == HdSkinningInputTokens->skelLocalToCommonSpace) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _GetResolvedSkeletonSchema().GetSkelLocalToCommonSpace());
        }
        if (name == HdSkinningInputTokens->commonSpaceToPrimLocal) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetCommonSpaceToPrimLocal());
        }

        // ExtAggregatorComputationInputValues
        if (name == HdSkinningSkelInputTokens->geomBindTransform) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetGeomBindTransform());
        }
        if (name == HdSkinningInputTokens->hasConstantInfluences) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetHasConstantInfluences());
        }
        if (name == HdSkinningInputTokens->numInfluencesPerComponent) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetNumInfluencesPerComponent());
        }
        if (name == HdSkinningInputTokens->influences) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetInfluences());
        }

        if (name == HdSkinningInputTokens->blendShapeOffsets) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetBlendShapeOffsets());
        }
        if (name == HdSkinningInputTokens->blendShapeOffsetRanges) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetBlendShapeOffsetRanges());
        }
        if (name == HdSkinningInputTokens->numBlendShapeOffsetRanges) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _resolvedPrimSource->GetNumBlendShapeOffsetRanges());
        }

        if (name == HdSkinningInputTokens->numSkinningMethod) {
            // numSkinningMethod: default LBS = 0, DQS = 1,
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource<int>(_IsDualQuatSkinning() ? 1 : 0));
        }
        if (name == HdSkinningInputTokens->numJoints) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource<int>(_GetNumJoints()));
        }
        if (name == HdSkinningInputTokens->numBlendShapeWeights) {
            return UsdSkelImaging_DataSourcePrimvar::New(_ToDataSource<int>(
                _resolvedPrimSource->GetBlendShapeData()->numSubShapes));
        }
        return _resolvedPrimSource->Get(name);
    }

private:
    _SkinningPrimvarsDataSource(
        UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle const& 
            resolvedPrimSource,
        HdContainerDataSourceHandle const& skeletonPrimSource)

     : _resolvedPrimSource(std::move(resolvedPrimSource)),
       _skeletonPrimSource(std::move(skeletonPrimSource))
    {
    }


    int _GetNumJoints() const {
        const UsdSkelImagingSkeletonSchema& schema =
            UsdSkelImagingSkeletonSchema::GetFromParent(_skeletonPrimSource);
        return schema? schema.GetJoints()->GetValue(0.0f).GetArraySize(): 0;
    }

    const UsdSkelImagingResolvedSkeletonSchema &_GetResolvedSkeletonSchema() {
        return _resolvedPrimSource->GetResolvedSkeletonSchema();
    }

    bool _IsDualQuatSkinning() const {
        return _resolvedPrimSource->GetSkinningMethod() == 
            UsdSkelTokens->dualQuaternion;
    }


    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle const 
        _resolvedPrimSource;
    HdContainerDataSourceHandle const _skeletonPrimSource;
};

void
_AddIfNecessary(const TfToken &name, TfTokenVector * const names)
{
    if (std::find(names->begin(), names->end(), name) != names->end()) {
        return;
    }
    names->push_back(name);
}

} // namespace

HdDataSourceBaseHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::Get(const TfToken &name)
{
    TRACE_FUNCTION();

    // For vertex shader codepath we convert all the extComputation inputValues
    // to primvars.
    if (HdSkinningSettings::IsSkinningDeferred() &&
        name == HdPrimvarsSchema::GetSchemaToken()) {
        // skeleton guide primvars are handled in 
        // UsdSkelImagingDataSourceResolvedSkeletonPrim
        if (_primPath != _skeletonPath && _skeletonPrimSource) {
            return _SkinningPrimvarsDataSource::New(
                shared_from_this(), _skeletonPrimSource);
        }
    }

    HdDataSourceBaseHandle inputSrc = _primSource->Get(name);

    if (!HasExtComputations()) {
        return inputSrc;
    }

    if (name == HdExtComputationPrimvarsSchema::GetSchemaToken()) {
        return
            HdOverlayContainerDataSource::OverlayedContainerDataSources(
                _ExtComputationPrimvars(
                    _primPath,
                    HasNormalsExtComputations(),
                    _primvars),
                HdContainerDataSource::Cast(inputSrc));
    }

    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        // Block points primvar.
        // The normals are also blocked since they are either deformed by the
        // computation or recomputed after skinning,
        static HdContainerDataSourceHandle ds =
            _BlockPointsAndNormalsPrimvars();
        return ds;
    }

    return inputSrc;
}

TfTokenVector
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetNames()
{
    TfTokenVector names = _primSource->GetNames();

    if (!_resolvedSkeletonSchema) {
        return names;
    }
    
    if (!HdSkinningSettings::IsSkinningDeferred()) {
        _AddIfNecessary(
            HdExtComputationPrimvarsSchema::GetSchemaToken(), &names);
    }
    return names;
}

namespace 
{

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

        // XXX TODO
        // we need to chop up and remap the concatenated instance skinning 
        // transforms here.
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

// Data source for locator extComputations:inputValues:blendShapeWeights on
// skinningComputation prim.
class _BlendShapeWeightsDataSource : public HdFloatArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BlendShapeWeightsDataSource);

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtFloatArray
    GetTypedValue(const HdSampledDataSource::Time shutterOffset) override {
        TRACE_FUNCTION();

        const TfSpan<const TfToken> blendShapes(  
            UsdSkelImagingGetTypedValue(_blendShapes, shutterOffset));
        const TfSpan<const float> blendShapeWeights(
            UsdSkelImagingGetTypedValue(_blendShapeWeights, shutterOffset));
        const TfSpan<const GfVec2i> blendShapeRanges(
            UsdSkelImagingGetTypedValue(_blendShapeRanges, shutterOffset));

        const size_t numSubShapes = _blendShapeData->numSubShapes;

        // XXX TODO
        // might wanna tweak the grainSize here to speed up when there's not as
        // many agents, since this is not as expensive as we initially thought.
        VtFloatArray result;
        result.resize(
            blendShapeRanges.size() * numSubShapes,
            [&] (float* start, float* end) {
                WorkParallelForN(
                    blendShapeRanges.size(),
                    [&] (size_t rangeStart, size_t rangeEnd) {
                        TRACE_FUNCTION_SCOPE(
                            "ComputeBlendShapeWeights inner loop");
                        
                        for (size_t i = rangeStart; i < rangeEnd; ++i) {
                            const GfVec2i& range = blendShapeRanges[i];
                            const auto shapes = 
                                blendShapes.subspan(range[0], range[1]);
                            const auto weights = 
                                blendShapeWeights.subspan(range[0], range[1]);

                            const VtFloatArray& outWeights = 
                                UsdSkelImagingComputeBlendShapeWeights(
                                    *_blendShapeData, shapes, weights);

                            // XXX
                            // outWeights.size() should equal to numSubShapes
                            // do we need a TF_VERIFY here?
                            for (size_t j = 0; j < numSubShapes; ++j) {
                                new (start + i * numSubShapes + j) 
                                    float(outWeights[j]);
                            }
                        }
                    }, 5000 /* grainSize */);
            });

        return result;
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
        HdFloatArrayDataSourceHandle blendShapeWeights,
        HdVec2iArrayDataSourceHandle blendShapeRanges)
     : _blendShapeData(std::move(blendShapeData))
     , _blendShapes(std::move(blendShapes))
     , _blendShapeWeights(std::move(blendShapeWeights))
     , _blendShapeRanges(std::move(blendShapeRanges))
    {
    }

    std::shared_ptr<UsdSkelImagingBlendShapeData> const _blendShapeData;
    HdTokenArrayDataSourceHandle const _blendShapes;
    HdFloatArrayDataSourceHandle const _blendShapeWeights;
    HdVec2iArrayDataSourceHandle const _blendShapeRanges;
};

// Extract the Scale & Shear parts of 4x4 matrix by removing the
// translation & rotation. Return only the upper-left 3x3 matrix.
GfMatrix3f
_ComputeSkinningScaleXform(const GfMatrix4f &skinningXform)
{
    TRACE_FUNCTION();

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
    TRACE_FUNCTION();

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
    TRACE_FUNCTION();

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
    TRACE_FUNCTION();

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

        return _ComputeSkinningDualQuats(
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

} // namespace


HdFloatArrayDataSourceHandle 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetBlendShapeWeights()
{
    TRACE_FUNCTION();
    return _BlendShapeWeightsDataSource::New(GetBlendShapeData(),
        _resolvedSkeletonSchema.GetBlendShapes(),
        _resolvedSkeletonSchema.GetBlendShapeWeights(),
        _resolvedSkeletonSchema.GetBlendShapeRanges());
}

HdMatrix4fArrayDataSourceHandle 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetSkinningTransforms()
{
    // Apply jointMapper to skinning xforms from resolved skeleton
    // if necessary.
    TRACE_FUNCTION();

    HdMatrix4fArrayDataSourceHandle skelSkinningXforms =
        _resolvedSkeletonSchema.GetSkinningTransforms();
    if (!skelSkinningXforms) {
        return nullptr;
    }

    std::shared_ptr<UsdSkelImagingJointInfluencesData> jointInfluencesData =
         GetJointInfluencesData();
    if (jointInfluencesData->jointMapper.IsNull() ||
        jointInfluencesData->jointMapper.IsIdentity()) {
        return skelSkinningXforms;
    }

    return _SkinningXformsDataSource::New(
        std::move(jointInfluencesData), std::move(skelSkinningXforms));
}

HdMatrix3fArrayDataSourceHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetSkinningScaleTransforms()
{
    TRACE_FUNCTION();
    return _SkinningScaleXformsDataSource::New(GetSkinningTransforms());
}

HdVec4fArrayDataSourceHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetSkinningDualQuats()
{
    TRACE_FUNCTION();
    return _SkinningDualQuatsDataSource::New(GetSkinningTransforms());
}

HdMatrixDataSourceHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::
GetCommonSpaceToPrimLocal() const 
{
    TRACE_FUNCTION();
    return _MatrixInverseDataSource::New(
        _xformResolver.GetPrimLocalToCommonSpace());
}

HdSampledDataSourceHandle 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetPoints()
{
    TRACE_FUNCTION();
    return GetPrimvars().GetPrimvar(
        HdPrimvarsSchemaTokens->points).GetPrimvarValue();
}

HdDataSourceBaseHandle 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetGeomBindTransform()
{
    TRACE_FUNCTION();

    const auto primvarValueDs = HdMatrixDataSource::Cast(
        GetPrimvars().GetPrimvar(
            UsdSkelImagingBindingSchemaTokens->geomBindTransformPrimvar)
            .GetPrimvarValue());

    // Convert the primvar value to GfMatrix4f for vertex shader access.
    return _ToDataSource(primvarValueDs? 
        GfMatrix4f(primvarValueDs->GetTypedValue(0.0f)): GfMatrix4f(1.0f));
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetHasConstantInfluences() 
{
    TRACE_FUNCTION();
    return _ToDataSource(GetJointInfluencesData()->hasConstantInfluences);
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetNumInfluencesPerComponent() 
{
    TRACE_FUNCTION();
    return _ToDataSource(GetJointInfluencesData()->numInfluencesPerComponent);
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetInfluences() 
{
    TRACE_FUNCTION();
    return _ToDataSource(GetJointInfluencesData()->influences);
}

HdDataSourceBaseHandle 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetBlendShapeOffsets()
{
    TRACE_FUNCTION();
    return _ToDataSource(GetBlendShapeData()->blendShapeOffsets);
}

HdDataSourceBaseHandle 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetBlendShapeOffsetRanges()
{
    TRACE_FUNCTION();
    return _ToDataSource(GetBlendShapeData()->blendShapeOffsetRanges);
}

HdDataSourceBaseHandle 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetNumBlendShapeOffsetRanges()
{
    TRACE_FUNCTION();
    return _ToDataSource<int>(
        GetBlendShapeData()->blendShapeOffsetRanges.size());
}

HdSampledDataSourceHandle 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetNormals() const
{
    TRACE_FUNCTION();
    return GetPrimvars().GetPrimvar(
        HdPrimvarsSchemaTokens->normals).GetPrimvarValue();
}

HdSampledDataSourceHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetFaceVertexIndices()
{
    TRACE_FUNCTION();

    const HdContainerDataSourceHandle meshDs =
        HdContainerDataSource::Cast(
            Get(HdMeshSchemaTokens->mesh));
    if (!meshDs) {
        return nullptr;
    }

    const HdMeshSchema meshSchema = HdMeshSchema(meshDs);
    const HdMeshTopologySchema topoSchema = meshSchema.GetTopology();
    if (!topoSchema.IsDefined()) {
        return nullptr;
    }

    return HdSampledDataSource::Cast(
        topoSchema.GetContainer()->Get(
            HdMeshTopologySchemaTokens->faceVertexIndices));
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceResolvedPointsBasedPrim::GetHasFaceVaryingNormals()
{
    TRACE_FUNCTION();

    const HdPrimvarSchema normalsPrimvar =
        GetPrimvars().GetPrimvar(HdPrimvarsSchemaTokens->normals);
    HdTokenDataSourceHandle interpDs = normalsPrimvar.GetInterpolation();
    if (!interpDs) {
        return nullptr;
    }
    const TfToken interpolation = interpDs->GetTypedValue(0.0f);
    const bool hasFaceVaryingNormals =
        interpolation == HdPrimvarSchemaTokens->faceVarying;
    return _ToDataSource(hasFaceVaryingNormals);
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

bool 
UsdSkelImagingDataSourceResolvedPointsBasedPrim::HasExtComputations() const 
{
    return
        // If we're not using vertex shader skinning.
        !HdSkinningSettings::IsSkinningDeferred() &&
        // Points are only posed if we bind a Skeleton prim (and the
        // UsdSkelImagingSkeletonResolvingSceneIndex has populated the
        // resolved skeleton schema).
        _resolvedSkeletonSchema &&
        // Do not use ext computation if this prim was the Skeleton itself.
        // For the Skeleton prim itself, the
        // UsdSkelImagingSkeletonResolvingSceneIndex has populated the
        // points primvar already (with the points for the mesh guide)
        // and changed the prim type to mesh.
        _primPath != _skeletonPath &&
        // We only skin prims if they are under a SkelRoot.
        //
        // Note that when we bake the points of a skinned prim, we also
        // change the SkelRoot to a different prim type (such as Scope
        // or Xform) so that the baked points are not skinned again.
        _hasSkelRoot;
}

bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::HasNormalsExtComputations() const
{
    static const bool normalComputationsEnabled =
        TfGetEnvSetting(USDSKELIMAGING_ENABLE_NORMAL_COMPUTATIONS);

    if (normalComputationsEnabled) {
        // If there are ext computations ...
        if (HasExtComputations()) {
            // And the subdivision scheme is "none" ...
            const HdTokenDataSourceHandle subdivisionSchemeDs =
                _mesh.GetSubdivisionScheme();
            const TfToken subdivisionScheme =
                subdivisionSchemeDs
                    ? subdivisionSchemeDs->GetTypedValue(0.0f)
                    : PxOsdOpenSubdivTokens->none;
            if (subdivisionScheme == PxOsdOpenSubdivTokens->none) {
                // And there are authored normals ...
                const VtVec3fArray normals =
                    UsdSkelImagingGetTypedValue(
                        HdVec3fArrayDataSource::Cast(
                            GetNormals()));

                // Then we should have ext computations for normals.
                return !normals.empty();
            }
        }
    }

    return false;
}

const HdDataSourceLocatorSet &
UsdSkelImagingDataSourceResolvedPointsBasedPrim::
GetDependendendOnDataSourceLocators()
{
    static const HdDataSourceLocatorSet result{
        UsdSkelImagingBindingSchema::GetDefaultLocator(),
        HdPrimvarsSchema::GetDefaultLocator(),
        UsdSkelImagingDataSourceXformResolver::GetXformLocator()
    };

    return result;
}

// TODO
// maybe move these dirty locator methods below to a separate cpp file for
// cleaner read.
bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::_ProcessDirtyLocators(
    const HdDataSourceLocatorSet &dirtyLocators,
    HdDataSourceLocatorSet * const dirtyLocatorsForAggregatorComputation,
    HdDataSourceLocatorSet * const dirtyLocatorsForComputation,
    TfTokenVector * const dirtyPrimvars)
{
    TRACE_FUNCTION();
    
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
    if (dirtyLocators.Contains(
            UsdSkelImagingDataSourceXformResolver::GetInstancedByLocator())) {
        // Instancers have changed.
        // Just indicate that we want to blow everything.
        return true;
    }
    static const HdDataSourceLocator skinningMethodLocator =
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(UsdSkelImagingBindingSchemaTokens->skinningMethodPrimvar);
    if (dirtyLocators.Contains(skinningMethodLocator)) {
        // XXX
        // we can potentially just dirty skinningXforms, skinningScaleXforms
        // and skinningDualQuats and return false here?
        /*
        if (dirtyPrimvars) {
            dirtyPrimvars->push_back(HdSkinningInputTokens->numSkinningMethod);
        }*/
        return true;
    }

    if (!_resolvedSkeletonSchema) {
        return false;
    }

    _ProcessDirtySkelBlendShapeLocators(dirtyLocators,
        dirtyLocatorsForAggregatorComputation, dirtyLocatorsForComputation,
        dirtyPrimvars);

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

        if (dirtyPrimvars) {
            static const TfTokenVector influencePrimvars{
                HdSkinningInputTokens->hasConstantInfluences,
                HdSkinningInputTokens->numInfluencesPerComponent,
                HdSkinningInputTokens->influences
            };
            dirtyPrimvars->insert(dirtyPrimvars->end(),
                influencePrimvars.begin(), influencePrimvars.end());
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

    if (HasNormalsExtComputations()) {
        static const HdDataSourceLocator normalsPrimvarLocator =
            HdPrimvarsSchema::GetDefaultLocator()
                .Append(HdPrimvarsSchemaTokens->normals);
        if (dirtyLocators.Intersects(normalsPrimvarLocator)) {
            if (dirtyLocatorsForAggregatorComputation) {
                static const HdDataSourceLocator aggregatorInputLocator =
                    HdExtComputationSchema::GetInputValuesLocator()
                        .Append(
                            UsdSkelImagingExtAggregatorComputationInputNameTokens
                                ->restNormals);
                dirtyLocatorsForAggregatorComputation->insert(
                    aggregatorInputLocator);
            }

            if (dirtyLocatorsForComputation) {
                static const HdDataSourceLocatorSet inputLocators{
                    HdExtComputationSchema::GetDispatchCountLocator(),
                    HdExtComputationSchema::GetElementCountLocator()};
                dirtyLocatorsForComputation->insert(inputLocators);
            }
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

    if (dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::GetXformLocator())) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                        ->commonSpaceToPrimLocal);
            dirtyLocatorsForComputation->insert(inputLocator);
        }

        if (dirtyPrimvars) {
            dirtyPrimvars->push_back(
                HdSkinningInputTokens->commonSpaceToPrimLocal);
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::_ProcessDirtySkeletonLocators(
    const HdDataSourceLocatorSet &dirtyLocators,
    HdDataSourceLocatorSet * const dirtyLocatorsForAggregatorComputation,
    HdDataSourceLocatorSet * const dirtyLocatorsForComputation,
    TfTokenVector * const dirtyPrimvars)
{
    TRACE_FUNCTION();
    
    if (dirtyLocators.Contains(
            UsdSkelImagingResolvedSkeletonSchema::GetDefaultLocator())) {
        return true;
    }

    if (!_resolvedSkeletonSchema) {
        return false;
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingResolvedSkeletonSchema::
            GetSkelLocalToCommonSpaceLocator())) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                            ->skelLocalToCommonSpace);
            dirtyLocatorsForComputation->insert(inputLocator);
        }
        if (dirtyPrimvars) {
            dirtyPrimvars->push_back(
                HdSkinningInputTokens->skelLocalToCommonSpace);
        }
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingResolvedSkeletonSchema::
            GetSkinningTransformsLocator())) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocatorSet inputLocators{
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(UsdSkelImagingExtComputationInputNameTokens
                        ->skinningXforms),
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(UsdSkelImagingExtComputationInputNameTokens
                        ->skinningScaleXforms),
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(UsdSkelImagingExtComputationInputNameTokens
                        ->skinningDualQuats)};
            dirtyLocatorsForComputation->insert(inputLocators);
        }
        if (dirtyPrimvars) {
            dirtyPrimvars->push_back(HdSkinningInputTokens->skinningXforms);
            dirtyPrimvars->push_back(
                HdSkinningInputTokens->skinningScaleXforms);
            dirtyPrimvars->push_back(HdSkinningInputTokens->skinningDualQuats);
            dirtyPrimvars->push_back(HdSkinningInputTokens->numJoints);
        }
    }

    static const HdDataSourceLocatorSet blendLocators{
        UsdSkelImagingResolvedSkeletonSchema::GetBlendShapesLocator(),
        UsdSkelImagingResolvedSkeletonSchema::GetBlendShapeWeightsLocator(),
        UsdSkelImagingResolvedSkeletonSchema::GetBlendShapeRangesLocator()};
    if (dirtyLocators.Intersects(blendLocators)) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator().Append(
                    UsdSkelImagingExtComputationInputNameTokens
                        ->blendShapeWeights);
            dirtyLocatorsForComputation->insert(
                inputLocator);
        }
        if (dirtyPrimvars) {
            dirtyPrimvars->push_back(HdSkinningInputTokens->blendShapeWeights);
            dirtyPrimvars->push_back(
                HdSkinningInputTokens->numBlendShapeWeights);
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::
_ProcessDirtySkelBlendShapeLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * const dirtyLocatorsForAggregatorComputation,
        HdDataSourceLocatorSet * const dirtyLocatorsForComputation,
        TfTokenVector * dirtyPrimvars)
{
    TRACE_FUNCTION();

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

        if (dirtyPrimvars) {
            static const TfTokenVector blendShapePrimvars{
                HdSkinningInputTokens->blendShapeOffsets,
                HdSkinningInputTokens->blendShapeOffsetRanges,
                HdSkinningInputTokens->numBlendShapeOffsetRanges,
                HdSkinningInputTokens->blendShapeWeights,
                HdSkinningInputTokens->numBlendShapeWeights
            };
            dirtyPrimvars->insert(dirtyPrimvars->end(), 
                blendShapePrimvars.begin(), blendShapePrimvars.end());
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedPointsBasedPrim::_ProcessDirtyInstancerLocators(
        const HdDataSourceLocatorSet &dirtyLocators,
        HdDataSourceLocatorSet * const dirtyLocatorsForAggregatorComputation,
        HdDataSourceLocatorSet * const dirtyLocatorsForComputation)
{
    TRACE_FUNCTION();

    if (dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::GetInstancedByLocator())) {
        return true;
    }

    if (!_resolvedSkeletonSchema) {
        return false;
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::GetXformLocator()) ||
        dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::GetInstanceXformLocator())) {
        if (dirtyLocatorsForComputation) {
            static const HdDataSourceLocator inputLocator =
                HdExtComputationSchema::GetInputValuesLocator()
                    .Append(
                        UsdSkelImagingExtComputationInputNameTokens
                        ->commonSpaceToPrimLocal);
            dirtyLocatorsForComputation->insert(inputLocator);
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
    TRACE_FUNCTION();
    
    HdDataSourceLocatorSet dirtyLocatorsForAggregatorComputation;
    HdDataSourceLocatorSet dirtyLocatorsForComputation;
    TfTokenVector dirtyPrimvars;

    HdDataSourceLocatorSet * const dirtyLocatorsForAggregatorComputationPtr =
        entries && HasExtComputations() ? 
        &dirtyLocatorsForAggregatorComputation : nullptr;
    HdDataSourceLocatorSet * const dirtyLocatorsForComputationPtr =
        entries && HasExtComputations() ?
        &dirtyLocatorsForComputation : nullptr;
    // dirtyPrimvars is for vertex shader code path so we check if there's no
    // extComputation.
    TfTokenVector * const dirtyPrimvarsPtr = 
        entries && !HasExtComputations() ? &dirtyPrimvars : nullptr;

    bool result = false;
    if (dirtiedPrimType == UsdSkelImagingPrimTypeTokens->skeleton) {
        result = _ProcessDirtySkeletonLocators(
            dirtyLocators,
            dirtyLocatorsForAggregatorComputationPtr,
            dirtyLocatorsForComputationPtr,
            dirtyPrimvarsPtr);
    } else if (dirtiedPrimType ==
            UsdSkelImagingPrimTypeTokens->skelBlendShape) {
        result = _ProcessDirtySkelBlendShapeLocators(
            dirtyLocators,
            dirtyLocatorsForAggregatorComputationPtr,
            dirtyLocatorsForComputationPtr,
            dirtyPrimvarsPtr);
    } else if (dirtiedPrimType == HdPrimTypeTokens->instancer) {
        result = _ProcessDirtyInstancerLocators(
            dirtyLocators,
            dirtyLocatorsForAggregatorComputationPtr,
            dirtyLocatorsForComputationPtr);
    } else {
        result = _ProcessDirtyLocators(
            dirtyLocators,
            dirtyLocatorsForAggregatorComputationPtr,
            dirtyLocatorsForComputationPtr,
            dirtyPrimvarsPtr);
    }

    if (entries) {
        const bool hasNormalsComputations = HasNormalsExtComputations();
        bool sendPrimvarsValueDirty = false;

        if (!dirtyLocatorsForAggregatorComputation.IsEmpty()) {
            entries->push_back({
                _primPath.AppendChild(
                    UsdSkelImagingExtComputationNameTokens
                        ->pointsAggregatorComputation),
                dirtyLocatorsForAggregatorComputation});

            if (hasNormalsComputations) {
                entries->push_back({
                    _primPath.AppendChild(
                        UsdSkelImagingExtComputationNameTokens
                            ->normalsAggregatorComputation),
                    dirtyLocatorsForAggregatorComputation});
            }
            sendPrimvarsValueDirty = true;
        }
        if (!dirtyLocatorsForComputation.IsEmpty()) {
            entries->push_back({
                _primPath.AppendChild(
                    UsdSkelImagingExtComputationNameTokens->pointsComputation),
                dirtyLocatorsForComputation});

            if (hasNormalsComputations) {
                entries->push_back({
                    _primPath.AppendChild(
                        UsdSkelImagingExtComputationNameTokens
                            ->normalsComputation),
                    dirtyLocatorsForComputation});
            }
            sendPrimvarsValueDirty = true;
        }

        if (sendPrimvarsValueDirty) {
            static const HdDataSourceLocator pointsLocator =
                HdPrimvarsSchema::GetDefaultLocator()
                    .Append(HdPrimvarsSchemaTokens->points)
                    .Append(HdPrimvarSchemaTokens->primvarValue);
            entries->push_back({ _primPath, pointsLocator});

            if (hasNormalsComputations) {
                static const HdDataSourceLocator normalsLocator =
                    HdPrimvarsSchema::GetDefaultLocator()
                        .Append(HdPrimvarsSchemaTokens->normals)
                        .Append(HdPrimvarSchemaTokens->primvarValue);
                entries->push_back({ _primPath, normalsLocator});
            }
        }

        if (!dirtyPrimvars.empty()) {
            HdDataSourceLocatorSet primvarLocators;
            for (const TfToken& primvar : dirtyPrimvars) {
                primvarLocators.insert(
                    HdPrimvarsSchema::GetDefaultLocator().Append(primvar)
                    .Append(HdPrimvarSchemaTokens->primvarValue));
            }
            entries->push_back({ _primPath, std::move(primvarLocators) });
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
