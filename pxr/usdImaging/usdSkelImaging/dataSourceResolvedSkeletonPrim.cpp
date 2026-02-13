//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourceResolvedSkeletonPrim.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourcePrimvar.h"
#include "pxr/usdImaging/usdSkelImaging/resolvedSkeletonSchema.h"
#include "pxr/usdImaging/usdSkelImaging/skelData.h"
#include "pxr/usdImaging/usdSkelImaging/skelGuideData.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"
#include "pxr/usdImaging/usdSkelImaging/utils.h"

#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/skinningSettings.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Wrapper around std::set_union to compute the set-wise union of two
// sorted vectors of sample times.
static std::vector<HdSampledDataSource::Time>
_Union(const std::vector<HdSampledDataSource::Time> &a,
       const std::vector<HdSampledDataSource::Time> &b)
{
    std::vector<HdSampledDataSource::Time> result;
    std::set_union(a.begin(), a.end(),
                   b.begin(), b.end(),
                   std::back_inserter(result));
    return result;
}

/// Data source for resolvedSkeleton/skinningTransforms
class _SkinningTransformsDataSource : public HdMatrix4fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SkinningTransformsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtArray<GfMatrix4f> GetTypedValue(const Time shutterOffset) override {
        if (shutterOffset == 0.0f) {
            return _valueAtZero;
        }

        return _Compute(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {
        TRACE_FUNCTION();
        
        // XXX TODO
        // parallelize this?
        std::vector<HdSampledDataSourceHandle> ds;
        ds.reserve(_animationSchemas.size() * 3);
        for (const auto& animSchema : _animationSchemas) {
            ds.push_back(animSchema.GetTranslations());
            ds.push_back(animSchema.GetRotations());
            ds.push_back(animSchema.GetScales());
        };

        if (!HdGetMergedContributingSampleTimesForInterval(
            std::size(ds), ds.data(), startTime, endTime, outSampleTimes)) {
            return false;
        }

        if (outSampleTimes) {
            // Replicate behavior of usdSkel/skeletonAdapter.cpp
            // and usdImagingDelegate.
            //
            // startTime and endTime are explictily added by
            // _UnionTimeSample in skeletonAdapter.cpp
            //
            // The 0 sample time ended up in a more circuitous route:
            // If a USD attribute is not animated, the UsdImagingDelegate
            // sample method gives a sample at time zero.
            // HdsiExtComputationPrimvarPruningSceneIndex takes the union
            // of all input time samples.
            // For skeletons served by the UsdImagingDelegate, the
            // geomBindTransform is typically not animated and ultimately
            // causes the 0 sample time to be seen by the render delegate.
            //
            // TODO: This should be controlled by the Usd MotionAPI.
            // It is unclear though whether to apply it to the Skeleton
            // or the affected mesh.
            //
            *outSampleTimes = _Union(
                *outSampleTimes, { startTime, 0.0f, endTime });
        }

        return true;
    }

private:
    _SkinningTransformsDataSource(
        std::shared_ptr<UsdSkelImagingSkelData> data,
        HdMatrix4fArrayDataSourceHandle restTransformsDataSource,
        const VtArray<UsdSkelImagingAnimationSchema>& animationSchemas,
        const VtArray<SdfPath>& animationSources)
      : _data(std::move(data))
      , _restTransformsDataSource(std::move(restTransformsDataSource))
      , _animationSchemas(std::move(animationSchemas))
      , _animationSources(std::move(animationSources))
      , _valueAtZero(_Compute(0.0f))
    {
    }

    VtArray<GfMatrix4f> _Compute(const Time shutterOffset) {
        TRACE_FUNCTION();

        if (!TF_VERIFY(_data)) {
            return {};
        }

        return UsdSkelImagingComputeSkinningTransforms(
            *_data, _restTransformsDataSource, _animationSchemas, 
            _animationSources, shutterOffset);
    }

    std::shared_ptr<UsdSkelImagingSkelData> const _data;
    HdMatrix4fArrayDataSourceHandle const _restTransformsDataSource;
    const VtArray<UsdSkelImagingAnimationSchema> _animationSchemas;
    const VtArray<SdfPath> _animationSources;

    // Safe value at zero. Similar to how the xform data source for
    // the flattening scene index works.
    const VtArray<GfMatrix4f> _valueAtZero;
};

/// Data source for resolvedSkeleton/blendShapes
/// This concatenates all the blendShapes from resolved animation schemas
/// on the resolved skeleton data source.
class _BlendShapesDataSource : public HdTokenArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BlendShapesDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtTokenArray GetTypedValue(const Time shutterOffset) override {
        if (shutterOffset == 0.0f) {
            return _valueAtZero;
        }

        return _Compute(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {
        TRACE_FUNCTION();

        // XXX TODO
        // parallelize this?
        std::vector<HdSampledDataSourceHandle> ds;
        ds.reserve(_animSchemas.size());
        for (const auto& animSchema : _animSchemas) {
            ds.push_back(animSchema.GetBlendShapes());
        };

        if (!HdGetMergedContributingSampleTimesForInterval(
            std::size(ds), ds.data(), startTime, endTime, outSampleTimes)) {
            return false;
        }

        if (outSampleTimes) {
            // Same logic as _SkinningTransformsDataSource
            *outSampleTimes = _Union(
                *outSampleTimes, { startTime, 0.0f, endTime });
        }

        return true;
    }

private:
    _BlendShapesDataSource(
        const VtArray<UsdSkelImagingAnimationSchema>& animationSchemas,
        const HdVec2iArrayDataSourceHandle& blendShapeRanges)
      : _animSchemas(std::move(animationSchemas))
      , _blendShapeRanges(std::move(blendShapeRanges))
      , _valueAtZero(_Compute(0.0f))
    {
    }

    VtTokenArray _Compute(const Time shutterOffset) {
        TRACE_FUNCTION();

        if (_animSchemas.empty()) {
            return {};
        }

        const VtVec2iArray& ranges = UsdSkelImagingGetTypedValue(
            _blendShapeRanges, shutterOffset);
        const GfVec2i& lastRange = ranges.back();
        const int numBlendShapes = lastRange[0] + lastRange[1];

        // XXX TODO
        // This is a performance hotspot, should investigate why it's so
        // expensive even in parallel. Initial guess is the cost of processing
        // so many TfTokens?
        VtTokenArray result;
        result.resize(
            numBlendShapes,
            [&] (TfToken* start, TfToken* end) {
                WorkParallelForN(
                    _animSchemas.size(),
                    [&] (size_t animStart, size_t animEnd) {
                        TRACE_FUNCTION_SCOPE(
                            "ComputeBlendShapes inner loop");

                        for (size_t i = animStart; i < animEnd; ++i) {
                            const VtTokenArray& shapes = 
                                UsdSkelImagingGetTypedValue(
                                    _animSchemas[i].GetBlendShapes(), 
                                    shutterOffset);
                            for (int j = 0; j < ranges[i][1]; ++j) {
                                new (start + ranges[i][0] + j) 
                                    TfToken(shapes[j]);
                            }
                        }
                    });
            });

        return result;
    }

    const VtArray<UsdSkelImagingAnimationSchema> _animSchemas;
    const HdVec2iArrayDataSourceHandle _blendShapeRanges;

    // XXX
    // pattern copied from _SkinningTransformsDataSource but doesn't feel
    // necessary in both?
    const VtTokenArray _valueAtZero;
};

/// Data source for resolvedSkeleton/blendShapeWeights
/// This concatenates all the blendShapeWeights from resolved animation schemas
/// on the resolved skeleton data source.
class _BlendShapeWeightsDataSource : public HdFloatArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BlendShapeWeightsDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtFloatArray GetTypedValue(const Time shutterOffset) override {
        if (shutterOffset == 0.0f) {
            return _valueAtZero;
        }

        return _Compute(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {
        TRACE_FUNCTION();

        // XXX TODO
        // parallelize this?
        std::vector<HdSampledDataSourceHandle> ds;
        ds.reserve(_animSchemas.size());
        for (const auto& animSchema : _animSchemas) {
            ds.push_back(animSchema.GetBlendShapeWeights());
        };

        if (!HdGetMergedContributingSampleTimesForInterval(
            std::size(ds), ds.data(), startTime, endTime, outSampleTimes)) {
            return false;
        }

        if (outSampleTimes) {
            // Same logic as _SkinningTransformsDataSource
            *outSampleTimes = _Union(
                *outSampleTimes, { startTime, 0.0f, endTime });
        }

        return true;
    }

private:
    _BlendShapeWeightsDataSource(
        const VtArray<UsdSkelImagingAnimationSchema>& animationSchemas,
        const HdVec2iArrayDataSourceHandle& blendShapeRanges)
      : _animSchemas(std::move(animationSchemas))
      , _blendShapeRanges(std::move(blendShapeRanges))
      , _valueAtZero(_Compute(0.0f))
    {
    }

    VtFloatArray _Compute(const Time shutterOffset) {
        TRACE_FUNCTION();

        if (_animSchemas.empty()) {
            return {};
        }

        const VtVec2iArray& ranges = UsdSkelImagingGetTypedValue(
            _blendShapeRanges, shutterOffset);
        const GfVec2i& lastRange = ranges.back();
        const int numBlendShapes = lastRange[0] + lastRange[1];

        VtFloatArray result;
        result.resize(
            numBlendShapes,
            [&] (float* start, float* end) {
                WorkParallelForN(
                    _animSchemas.size(),
                    [&] (size_t animStart, size_t animEnd) {
                        TRACE_FUNCTION_SCOPE(
                            "ComputeBlendShapeWeights inner loop");

                        for (size_t i = animStart; i < animEnd; ++i) {
                            const VtFloatArray& weights = 
                                UsdSkelImagingGetTypedValue(
                                    _animSchemas[i].GetBlendShapeWeights(), 
                                    shutterOffset);
                            for (int j = 0; j < ranges[i][1]; ++j) {
                                new (start + ranges[i][0] + j) float(weights[j]);
                            }
                        }
                    }, 5000 /* grainSize */);
            });

        return result;
    }

    const VtArray<UsdSkelImagingAnimationSchema> _animSchemas;
    const HdVec2iArrayDataSourceHandle _blendShapeRanges;

    // XXX
    // pattern copied from _SkinningTransformsDataSource but doesn't feel
    // necessary in both?
    const VtFloatArray _valueAtZero;
};

/// Data source for resolvedSkeleton/blendShapeWeights
/// This provides the ranges (offset, numElements tuple) of the concatenated 
/// blendShapes and blendShapeWeights from resolved animation schemas on the 
/// resolved skeleton data source.
/// Because animation schemas don't necessarily have the same number of
/// blend shapes, this provides the downstream data source (namely 
/// UsdSkelImagingDataSourceResolvedPointsBasedPrim) a way to restore them
/// individually. 
/// This also facilitates _BlendShapesDataSource and 
/// _BlendShapeWeightsDataSource to compute values in parallel.
class _BlendShapeRangesDataSource : public HdVec2iArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_BlendShapeRangesDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec2iArray GetTypedValue(const Time shutterOffset) override {
        if (shutterOffset == 0.0f) {
            return _valueAtZero;
        }

        return _Compute(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {
        TRACE_FUNCTION();

        // XXX TODO
        // parallelize this?
        std::vector<HdSampledDataSourceHandle> ds;
        ds.reserve(_animSchemas.size());
        for (const auto& animSchema : _animSchemas) {
            ds.push_back(animSchema.GetBlendShapeWeights());
        };

        if (!HdGetMergedContributingSampleTimesForInterval(
            std::size(ds), ds.data(), startTime, endTime, outSampleTimes)) {
            return false;
        }

        if (outSampleTimes) {
            // Same logic as _SkinningTransformsDataSource
            *outSampleTimes = _Union(
                *outSampleTimes, { startTime, 0.0f, endTime });
        }

        return true;
    }

private:
    _BlendShapeRangesDataSource(
        const VtArray<UsdSkelImagingAnimationSchema>& animationSchemas)
      : _animSchemas(std::move(animationSchemas))
      , _valueAtZero(_Compute(0.0f))
    {
    }

    VtVec2iArray _Compute(const Time shutterOffset) {
        TRACE_FUNCTION();

        if (_animSchemas.empty()) {
            return {};
        }

        // XXX TODO
        // we can't use tbb::parallel_scan() so need to implement our own
        // parallel_inclusive_scan() here.
        // 35ms for 1k agents here
        int offset = 0;
        VtVec2iArray result;
        result.resize(
            _animSchemas.size(),
            [&] (GfVec2i* start, GfVec2i* end)
            {   
                for (size_t i = 0; i < _animSchemas.size(); ++i) {
                    const int size = UsdSkelImagingGetTypedValue(
                        _animSchemas[i].GetBlendShapeWeights(), 
                        shutterOffset).size();
                    new (start + i) GfVec2i(offset, size);
                    offset += size;
                }
            });
        return result;
    }

    const VtArray<UsdSkelImagingAnimationSchema> _animSchemas;

    // XXX
    // pattern copied from _SkinningTransformsDataSource but doesn't feel
    // necessary in both?
    const VtVec2iArray _valueAtZero;
};

/// Data source for resolvedSkeleton
class _ResolvedSkeletonSchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ResolvedSkeletonSchemaDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector names = {
            UsdSkelImagingResolvedSkeletonSchemaTokens->skelLocalToCommonSpace,
            UsdSkelImagingResolvedSkeletonSchemaTokens->skinningTransforms,
            UsdSkelImagingResolvedSkeletonSchemaTokens->blendShapes,
            UsdSkelImagingResolvedSkeletonSchemaTokens->blendShapeWeights,
            UsdSkelImagingResolvedSkeletonSchemaTokens->blendShapeRanges };

        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        TRACE_FUNCTION();

        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
            ->skelLocalToCommonSpace) {
            // XXX
            // need to investigate:
            // this entry is surprisingly expensive, in the 10k human female
            // test it's taking almost 3s to compute this and in multiple 
            // levels of call sites in both HdStPopulateConstantPrimvars() and 
            // HdStPopulateVertexPrimvars() since there's only 1 skeleton in 
            // the scene it's prob getting called per instance?
            return _resolvedSkeletonSource->GetSkelLocalToCommonSpace();
        }
        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
            ->skinningTransforms) {
            return _resolvedSkeletonSource->GetSkinningTransforms();
        }
        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
            ->blendShapes) {
            return _resolvedSkeletonSource->GetBlendShapes();
        }
        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
            ->blendShapeWeights) {
            return _resolvedSkeletonSource->GetBlendShapeWeights();
        }
        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
            ->blendShapeRanges) {
            return _resolvedSkeletonSource->GetBlendShapeRanges();
        }
        return nullptr;
    }

private:
    _ResolvedSkeletonSchemaDataSource(
        UsdSkelImagingDataSourceResolvedSkeletonPrimHandle
            resolvedSkeletonSource)
     : _resolvedSkeletonSource(std::move(resolvedSkeletonSource))
    {
    }

    UsdSkelImagingDataSourceResolvedSkeletonPrimHandle const
        _resolvedSkeletonSource;
};

/// Data source for mesh/topology - for guide.
class _MeshTopologySchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MeshTopologySchemaDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector names = {
            HdMeshTopologySchemaTokens->faceVertexCounts,
            HdMeshTopologySchemaTokens->faceVertexIndices,
            HdMeshTopologySchemaTokens->orientation
        };

        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        TRACE_FUNCTION();

        if (name == HdMeshTopologySchemaTokens->faceVertexCounts) {
            return
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    UsdSkelImagingComputeSkelGuideFaceVertexCounts(
                        *_GetSkelGuideData()));
        }

        if (name == HdMeshTopologySchemaTokens->faceVertexIndices) {
            return
                HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    UsdSkelImagingComputeSkelGuideFaceVertexIndices(
                        *_GetSkelGuideData()));
        }

        if (name == HdMeshTopologySchemaTokens->orientation) {
            static HdDataSourceBaseHandle const result =
                HdMeshTopologySchema::BuildOrientationDataSource(
                    HdMeshTopologySchemaTokens->rightHanded);
            return result;
        }

        return nullptr;
    }

private:
    _MeshTopologySchemaDataSource(
        UsdSkelImagingDataSourceResolvedSkeletonPrimHandle
            resolvedSkeletonSource)
     : _resolvedSkeletonSource(std::move(resolvedSkeletonSource))
    {
    }

    std::shared_ptr<UsdSkelImagingSkelGuideData>
    _GetSkelGuideData() {
        return _resolvedSkeletonSource->GetSkelGuideData();
    }

    UsdSkelImagingDataSourceResolvedSkeletonPrimHandle const
        _resolvedSkeletonSource;
};

/// Data source for primvars/points/primvarValue - for mesh guide.
class _PointsPrimvarValueDataSource : public HdVec3fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsPrimvarValueDataSource);

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtVec3fArray GetTypedValue(const Time shutterOffset) override {
        TRACE_FUNCTION();

        return UsdSkelImagingComputeSkelGuidePoints(
            *_data,
            UsdSkelImagingGetTypedValue(
                _skinningTransforms, shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {

        return _skinningTransforms->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    _PointsPrimvarValueDataSource(
        std::shared_ptr<UsdSkelImagingSkelGuideData> data,
        HdMatrix4fArrayDataSourceHandle skinningTransforms)
     : _data(std::move(data))
     , _skinningTransforms(std::move(skinningTransforms))
    {
    }

    std::shared_ptr<UsdSkelImagingSkelGuideData> const _data;
    HdMatrix4fArrayDataSourceHandle const _skinningTransforms;
};

VtArray<UsdSkelImagingAnimationSchema>
_GetInstanceAnimationSchemas(
    const VtArray<SdfPath>& instanceAnimationSources,
    const HdSceneIndexBaseRefPtr& sceneIndex)
{
    VtArray<UsdSkelImagingAnimationSchema> result;
    result.resize(instanceAnimationSources.size(), 
        [&](UsdSkelImagingAnimationSchema* start, 
            UsdSkelImagingAnimationSchema* end)
        {
            for (size_t i = 0; i < instanceAnimationSources.size(); ++i) {
                new (start + i) UsdSkelImagingAnimationSchema(
                    UsdSkelImagingAnimationSchema::GetFromParent(
                        instanceAnimationSources[i].IsEmpty()? nullptr: 
                        sceneIndex->GetPrim(
                            instanceAnimationSources[i]).dataSource));
            }
        });
    return result;
}

template<typename T>
HdDataSourceBaseHandle
_ToDataSource(const T& value)
{
    return HdRetainedTypedSampledDataSource<T>::New(value);
}

// Data source that provides the primvars for skeleton guide vertex shader 
// skinning
class _SkelGuideSkinningPrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_SkelGuideSkinningPrimvarsDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector names{
            HdPrimvarsSchemaTokens->points,

            // ExtComputationInputValues
            HdSkinningInputTokens->skinningXforms,
            HdSkinningInputTokens->skelLocalToCommonSpace,
            HdSkinningInputTokens->commonSpaceToPrimLocal,

            // ExtAggregatorComputationInputValues
            HdSkinningSkelInputTokens->geomBindTransform,

            HdSkinningInputTokens->hasConstantInfluences,
            // TODO
            // If we can get vertex primvar to accept tensor values in 
            // imaging/hdSt/mesh.cpp _PopulateVertexPrimvars()#1417
            // (buffer source array size is currently hardcoded to 1)
            // then we'd change these 2 to
            // skel:jointIndices and skel:jointWeights 
            HdSkinningInputTokens->numInfluencesPerComponent,
            HdSkinningInputTokens->influences,

            // XXX
            // need to check if skinningMethod primvar actually exists, it's 
            // possible that it got aggregated to instance primvar on the instancer.
            // if it doesn't exist then we create the fall back primvar here.
            // enumerate skinningMethod because we can't pass down existing
            // Token primvar to vertex shader.
            HdSkinningInputTokens->numSkinningMethod,
            // Extra primvars needed for processing instance/vertex indexing
            // in the vertex shader.
            HdSkinningInputTokens->numJoints
        };

        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        TRACE_FUNCTION();

        if (name == HdPrimvarsSchemaTokens->points) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource(_data->boneMeshPoints), 
                HdPrimvarSchemaTokens->vertex, 
                HdPrimvarSchemaTokens->point);
        }

        // ExtComputationInputValues
        if (name == HdSkinningInputTokens->skinningXforms) {
            return UsdSkelImaging_DataSourcePrimvar::New(_skinningTransforms);
        }
        if (name == HdSkinningInputTokens->skelLocalToCommonSpace) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource(GfMatrix4f(1.0f)));
        }
        if (name == HdSkinningInputTokens->commonSpaceToPrimLocal) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource(GfMatrix4f(1.0f)));
        }

        // ExtAggregatorComputationInputValues
        if (name == HdSkinningSkelInputTokens->geomBindTransform) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource(GfMatrix4f(1.0f)));
        }
        if (name == HdSkinningInputTokens->hasConstantInfluences) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource(false));
        }
        if (name == HdSkinningInputTokens->numInfluencesPerComponent) {
            return UsdSkelImaging_DataSourcePrimvar::New(_ToDataSource<int>(1));
        }
        if (name == HdSkinningInputTokens->influences) {
            const auto data = _data;
            const size_t numPoints = data->boneJointIndices.size();
            VtVec2fArray influences;
            influences.resize(
                numPoints,
                [&data, &numPoints] (GfVec2f* start, GfVec2f* end) {
                    for (size_t i = 0; i < numPoints; ++i) {
                        new (start + i) GfVec2f(
                            static_cast<float>(data->boneJointIndices[i]), 
                            1.0f);
                    }
                });
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource(influences));
        }

        if (name == HdSkinningInputTokens->numSkinningMethod) {
            // default LBS = 0
            return UsdSkelImaging_DataSourcePrimvar::New(_ToDataSource<int>(0));
        }
        if (name == HdSkinningInputTokens->numJoints) {
            return UsdSkelImaging_DataSourcePrimvar::New(
                _ToDataSource(int(_data->numJoints)));
        }
        return nullptr;
    }

private:
    _SkelGuideSkinningPrimvarsDataSource(
        std::shared_ptr<UsdSkelImagingSkelGuideData> data,
        HdMatrix4fArrayDataSourceHandle const& skinningTransforms)
     : _data(std::move(data))
     , _skinningTransforms(std::move(skinningTransforms))
    {
    }

    std::shared_ptr<UsdSkelImagingSkelGuideData> const _data;
    HdMatrix4fArrayDataSourceHandle const _skinningTransforms;
};

}

/// Read rest transforms from UsdSkelImagingSkeletonSchema lazily and convert
/// to VtArray<GfMatrix4f>.
class UsdSkelImagingDataSourceResolvedSkeletonPrim::_RestTransformsDataSource
    : public HdMatrix4fArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(_RestTransformsDataSource);

    void Invalidate() { _cache.Invalidate(); }

    VtValue GetValue(const Time shutterOffset) override {
        return VtValue(GetTypedValue(shutterOffset));
    }

    VtArray<GfMatrix4f> GetTypedValue(const Time shutterOffset) override {
        return *_cache.Get();
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<float> * const outSampleTimes) override {
        return false;
    }

private:
    _RestTransformsDataSource(const UsdSkelImagingSkeletonSchema &schema)
     : _cache(schema)
    { }

    class _Cache : public UsdSkelImagingSharedPtrThunk<VtArray<GfMatrix4f>>
    {
    public:
        _Cache(const UsdSkelImagingSkeletonSchema &schema)
         : _schema(schema)
        {}
    protected:
        Handle _Compute() override {
            const VtArray<GfMatrix4d> m =
                UsdSkelImagingGetTypedValue(_schema.GetRestTransforms());

            return
                std::make_shared<VtArray<GfMatrix4f>>(m.begin(), m.end());
        }
    private:
        const UsdSkelImagingSkeletonSchema _schema;
    };
    _Cache _cache;
};

UsdSkelImagingDataSourceResolvedSkeletonPrim::
UsdSkelImagingDataSourceResolvedSkeletonPrim(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    HdContainerDataSourceHandle const &primSource)
 : _primPath(primPath)
 , _primSource(primSource)
 , _animationSource(
     UsdSkelImagingGetTypedValue(
         UsdSkelImagingBindingSchema::GetFromParent(primSource)
             .GetAnimationSource()))
 , _animationSchema(
     UsdSkelImagingAnimationSchema::GetFromParent(
         _animationSource.IsEmpty()
         ? nullptr
         : sceneIndex->GetPrim(_animationSource).dataSource))
 , _skelDataCache(sceneIndex, primPath)
 , _skelGuideDataCache(this)
 , _restTransformsDataSource(
     _RestTransformsDataSource::New(
         UsdSkelImagingSkeletonSchema::GetFromParent(primSource)))
 , _xformResolver(sceneIndex, primSource)
 // XXX
 // Should verify if the instancer we get from _xformResolver that instances
 // this skeleton and the instancer with prototypes bound to this skeleton are
 // the same thing. Is it possible that they could be different?
 , _instanceAnimationSources(std::move(
    _xformResolver.GetInstanceAnimationSource()))
 , _instanceAnimationSchemas(std::move(
    _GetInstanceAnimationSchemas(_instanceAnimationSources, sceneIndex)))
{
}

UsdSkelImagingDataSourceResolvedSkeletonPrim::
~UsdSkelImagingDataSourceResolvedSkeletonPrim() = default;

TfTokenVector
UsdSkelImagingDataSourceResolvedSkeletonPrim::GetNames()
{
    static const TfTokenVector names = {
        UsdSkelImagingResolvedSkeletonSchema::GetSchemaToken(),
        HdMeshSchema::GetSchemaToken(),
        HdPrimvarsSchema::GetSchemaToken()
    };
    return names;
}

HdDataSourceBaseHandle
UsdSkelImagingDataSourceResolvedSkeletonPrim::Get(const TfToken &name)
{
    TRACE_FUNCTION();

    if (name == UsdSkelImagingResolvedSkeletonSchema::GetSchemaToken()) {
        return _ResolvedSkeletonSchemaDataSource::New(shared_from_this());
    }
    if (name == HdMeshSchema::GetSchemaToken()) {
        static HdDataSourceBaseHandle const subdivSchemeDs =
            HdRetainedTypedSampledDataSource<TfToken>::New(
                PxOsdOpenSubdivTokens->none);
        return
            HdRetainedContainerDataSource::New(
                HdMeshSchemaTokens->topology,
                _MeshTopologySchemaDataSource::New(shared_from_this()),
                HdMeshSchemaTokens->subdivisionScheme,
                subdivSchemeDs,
                HdMeshSchemaTokens->doubleSided,
                HdRetainedTypedSampledDataSource<bool>::New(true));
    }
    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        if (HdSkinningSettings::IsSkinningDeferred()) {
            return _SkelGuideSkinningPrimvarsDataSource::New(
                GetSkelGuideData(), GetSkinningTransforms());
        }
        return HdRetainedContainerDataSource::New(
            HdPrimvarsSchemaTokens->points,
            UsdSkelImaging_DataSourcePrimvar::New(
                _PointsPrimvarValueDataSource::New(
                    GetSkelGuideData(), GetSkinningTransforms()),
                HdPrimvarSchemaTokens->vertex, 
                HdPrimvarSchemaTokens->point));
    }
    return nullptr;
}

bool 
UsdSkelImagingDataSourceResolvedSkeletonPrim::
_ShouldResolveInstanceAnimation() const
{
    // animationSource bound on the skeleton wins so only use 
    // instanceAnimationSource when there's no bound animationSource.
    return HdSkinningSettings::IsSkinningDeferred() && !_animationSchema;
}

const VtArray<SdfPath>
UsdSkelImagingDataSourceResolvedSkeletonPrim::
GetResolvedAnimationSources() const
{
    return _ShouldResolveInstanceAnimation() ?
        _instanceAnimationSources : 
        VtArray<SdfPath>({ _animationSource });
}

const VtArray<UsdSkelImagingAnimationSchema>
UsdSkelImagingDataSourceResolvedSkeletonPrim::
GetResolvedAnimationSchemas() const
{
    return _ShouldResolveInstanceAnimation() ?
        _instanceAnimationSchemas :
        VtArray<UsdSkelImagingAnimationSchema>({ _animationSchema });
}

HdMatrixDataSourceHandle
UsdSkelImagingDataSourceResolvedSkeletonPrim::GetSkelLocalToCommonSpace() const
{
    TRACE_FUNCTION();
    return _xformResolver.GetPrimLocalToCommonSpace();
}

HdMatrix4fArrayDataSourceHandle
UsdSkelImagingDataSourceResolvedSkeletonPrim::GetSkinningTransforms()
{
    TRACE_FUNCTION();
    return _SkinningTransformsDataSource::New(
        _skelDataCache.Get(), _restTransformsDataSource, 
        GetResolvedAnimationSchemas(), GetResolvedAnimationSources());
}

HdTokenArrayDataSourceHandle 
UsdSkelImagingDataSourceResolvedSkeletonPrim::GetBlendShapes() const
{
    TRACE_FUNCTION();
    return _BlendShapesDataSource::New(
        GetResolvedAnimationSchemas(), GetBlendShapeRanges());
}

HdFloatArrayDataSourceHandle 
UsdSkelImagingDataSourceResolvedSkeletonPrim::GetBlendShapeWeights() const
{
    TRACE_FUNCTION();
    return _BlendShapeWeightsDataSource::New(
        GetResolvedAnimationSchemas(), GetBlendShapeRanges());
}

HdVec2iArrayDataSourceHandle
UsdSkelImagingDataSourceResolvedSkeletonPrim::GetBlendShapeRanges() const
{
    TRACE_FUNCTION();
    return _BlendShapeRangesDataSource::New(GetResolvedAnimationSchemas());
}

const HdDataSourceLocatorSet &
UsdSkelImagingDataSourceResolvedSkeletonPrim::
GetDependendendOnDataSourceLocators()
{
    static const HdDataSourceLocatorSet result{
        UsdSkelImagingSkeletonSchema::GetDefaultLocator(),
        UsdSkelImagingBindingSchema::GetAnimationSourceLocator(),
        UsdSkelImagingDataSourceXformResolver::GetXformLocator()
    };
    return result;
}

static
const HdDataSourceLocator &
_PointsPrimvarValueLocator()
{
    static const HdDataSourceLocator result =
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdPrimvarsSchemaTokens->points)
            .Append(HdPrimvarSchemaTokens->primvarValue);
    return result;
}

bool
UsdSkelImagingDataSourceResolvedSkeletonPrim::
_ProcessSkeletonDirtyLocators(
    const HdDataSourceLocatorSet &dirtyLocators,
    HdDataSourceLocatorSet * const newDirtyLocators)
{
    TRACE_FUNCTION();

    if (dirtyLocators.Contains(
            UsdSkelImagingSkeletonSchema::GetDefaultLocator())) {
        // The entire skeleton schema was changed, blow everything.
        // resolved skeleton schema data source.
        return true;
    }

    if (dirtyLocators.Contains(
            UsdSkelImagingBindingSchema::GetAnimationSourceLocator()) &&
        !_ShouldResolveInstanceAnimation()) {
        // Our _animationSource and _animationSchema are invalid.
        // Just indicate that we want to blow everything.
        return true;
    }

    if (dirtyLocators.Contains(
            UsdSkelImagingDataSourceXformResolver::GetInstancedByLocator())) {
        // Instancers have changed.
        // Just indicate that we want to blow everything.
        return true;
    }

    static const HdDataSourceLocatorSet skelDataLocators = {
        UsdSkelImagingSkeletonSchema::GetJointsLocator(),
        UsdSkelImagingSkeletonSchema::GetBindTransformsLocator() };
    if (dirtyLocators.Intersects(skelDataLocators)) {
        _skelDataCache.Invalidate();
        _skelGuideDataCache.Invalidate();
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetSkinningTransformsLocator());
            newDirtyLocators->insert(
                HdMeshSchema::GetTopologyLocator());
            newDirtyLocators->insert(
                _PointsPrimvarValueLocator());
        }
    }

    if (dirtyLocators.Contains(
            UsdSkelImagingSkeletonSchema::GetRestTransformsLocator())) {
        _restTransformsDataSource->Invalidate();
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetSkinningTransformsLocator());
            newDirtyLocators->insert(
                _PointsPrimvarValueLocator());
        }
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::GetXformLocator())) {
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetSkelLocalToCommonSpaceLocator());
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedSkeletonPrim::
_ProcessSkelAnimationDirtyLocators(
    const HdDataSourceLocatorSet &dirtyLocators,
    HdDataSourceLocatorSet * const newDirtyLocators)
{
    TRACE_FUNCTION();

    if (dirtyLocators.Contains(
            UsdSkelImagingAnimationSchema::GetDefaultLocator())) {
        return true;
    }
    if (dirtyLocators.Contains(
            UsdSkelImagingDataSourceXformResolver::GetInstancedByLocator())) {
        return true;
    }

    if (dirtyLocators.Contains(
            UsdSkelImagingAnimationSchema::GetJointsLocator())) {
        _skelDataCache.Invalidate();
        _skelGuideDataCache.Invalidate();
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetSkinningTransformsLocator());
            newDirtyLocators->insert(
                _PointsPrimvarValueLocator());
        }
    }

    static const HdDataSourceLocatorSet transformsLocators{
        UsdSkelImagingAnimationSchema::GetTranslationsLocator(),
        UsdSkelImagingAnimationSchema::GetRotationsLocator(),
        UsdSkelImagingAnimationSchema::GetScalesLocator()};
    if (dirtyLocators.Intersects(transformsLocators)) {
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetSkinningTransformsLocator());
            newDirtyLocators->insert(
                _PointsPrimvarValueLocator());
        }
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingAnimationSchema::GetBlendShapesLocator())) {
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::GetBlendShapesLocator());
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetBlendShapeRangesLocator());
        }
    }
    if (dirtyLocators.Intersects(
            UsdSkelImagingAnimationSchema::GetBlendShapeWeightsLocator())) {
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetBlendShapeWeightsLocator());
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedSkeletonPrim::
_ProcessInstancerDirtyLocators(
    const HdDataSourceLocatorSet &dirtyLocators,
    HdDataSourceLocatorSet * const newDirtyLocators)
{
    TRACE_FUNCTION();

    if (dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::GetInstancedByLocator())) {
        return true;
    }
    if (dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::
                GetInstanceAnimationSourceLocator()) &&
        _ShouldResolveInstanceAnimation()) {
        return true;
    }

    if (dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::GetXformLocator()) ||
        dirtyLocators.Intersects(
            UsdSkelImagingDataSourceXformResolver::GetInstanceXformLocator())) {
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetSkelLocalToCommonSpaceLocator());
        }
    }

    return false;
}

bool
UsdSkelImagingDataSourceResolvedSkeletonPrim::
ProcessDirtyLocators(
        const TfToken &dirtiedPrimType,
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * const entries)
{
    TRACE_FUNCTION();
    
    HdDataSourceLocatorSet newDirtyLocators;
    bool result = false;

    if (dirtiedPrimType == UsdSkelImagingPrimTypeTokens->skeleton) {
        result =
            _ProcessSkeletonDirtyLocators(
                dirtyLocators, entries ? &newDirtyLocators : nullptr);
    } else if (dirtiedPrimType == UsdSkelImagingPrimTypeTokens->skelAnimation) {
        result =
            _ProcessSkelAnimationDirtyLocators(
                dirtyLocators, entries ? &newDirtyLocators : nullptr);
    } else if (dirtiedPrimType == HdPrimTypeTokens->instancer) {
        result =
            _ProcessInstancerDirtyLocators(
                dirtyLocators, entries ? &newDirtyLocators : nullptr);
    }                

    if (entries) {
        if (!newDirtyLocators.IsEmpty()) {
            entries->push_back({_primPath, std::move(newDirtyLocators)});
        }
    }

    return result;
}

UsdSkelImagingDataSourceResolvedSkeletonPrim::
_SkelDataCache::_SkelDataCache(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath)
 : _sceneIndex(sceneIndex)
 , _primPath(primPath)
{
}

std::shared_ptr<UsdSkelImagingSkelData>
UsdSkelImagingDataSourceResolvedSkeletonPrim::
_SkelDataCache::_Compute()
{
    TRACE_FUNCTION();

    return std::make_shared<UsdSkelImagingSkelData>(
        UsdSkelImagingComputeSkelData(_sceneIndex, _primPath));
}

UsdSkelImagingDataSourceResolvedSkeletonPrim::
_SkelGuideDataCache::_SkelGuideDataCache(
    UsdSkelImagingDataSourceResolvedSkeletonPrim * const resolvedSkeleton)
 : _resolvedSkeleton(resolvedSkeleton)
{
}

std::shared_ptr<UsdSkelImagingSkelGuideData>
UsdSkelImagingDataSourceResolvedSkeletonPrim::
_SkelGuideDataCache::_Compute()
{
    TRACE_FUNCTION();

    return std::make_shared<UsdSkelImagingSkelGuideData>(
        UsdSkelImagingComputeSkelGuideData(
            *_resolvedSkeleton->GetSkelData()));
}

PXR_NAMESPACE_CLOSE_SCOPE
