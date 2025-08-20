//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/dataSourceResolvedSkeletonPrim.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/resolvedSkeletonSchema.h"
#include "pxr/usdImaging/usdSkelImaging/skelData.h"
#include "pxr/usdImaging/usdSkelImaging/skelGuideData.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"
#include "pxr/usdImaging/usdSkelImaging/utils.h"

#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/trace/trace.h"

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
        HdSampledDataSourceHandle const ds[] = {
            _translationsDataSource, _rotationsDataSource, _scalesDataSource };

        if (!HdGetMergedContributingSampleTimesForInterval(
                std::size(ds), ds,
                startTime, endTime,
                outSampleTimes)) {
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
        HdVec3fArrayDataSourceHandle translationsDataSource,
        HdQuatfArrayDataSourceHandle rotationsDataSource,
        HdVec3hArrayDataSourceHandle scalesDataSource)
      : _data(std::move(data))
      , _restTransformsDataSource(std::move(restTransformsDataSource))
      , _translationsDataSource(std::move(translationsDataSource))
      , _rotationsDataSource(std::move(rotationsDataSource))
      , _scalesDataSource(std::move(scalesDataSource))
      , _valueAtZero(_Compute(0.0f))
    {
    }

    VtArray<GfMatrix4f> _Compute(const Time shutterOffset) {
        TRACE_FUNCTION();

        if (!TF_VERIFY(_data)) {
            return {};
        }

        return UsdSkelImagingComputeSkinningTransforms(
            *_data,
            _restTransformsDataSource,
            UsdSkelImagingGetTypedValue(
                _translationsDataSource, shutterOffset),
            UsdSkelImagingGetTypedValue(
                _rotationsDataSource, shutterOffset),
            UsdSkelImagingGetTypedValue(
                _scalesDataSource, shutterOffset));
    }

    std::shared_ptr<UsdSkelImagingSkelData> const _data;
    HdMatrix4fArrayDataSourceHandle const _restTransformsDataSource;
    HdVec3fArrayDataSourceHandle const _translationsDataSource;
    HdQuatfArrayDataSourceHandle const _rotationsDataSource;
    HdVec3hArrayDataSourceHandle const _scalesDataSource;

    // Safe value at zero. Similar to how the xform data source for
    // the flattening scene index works.
    const VtArray<GfMatrix4f> _valueAtZero;
};

/// Data source for resolvedSkeleton
class _ResolvedSkeletonSchemaDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ResolvedSkeletonSchemaDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector names = {
            UsdSkelImagingResolvedSkeletonSchemaTokens->skelLocalToWorld,
            UsdSkelImagingResolvedSkeletonSchemaTokens->skinningTransforms,
            UsdSkelImagingResolvedSkeletonSchemaTokens->blendShapes,
            UsdSkelImagingResolvedSkeletonSchemaTokens->blendShapeWeights };

        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        TRACE_FUNCTION();

        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
                        ->skelLocalToWorld) {
            return _resolvedSkeletonSource->GetSkelLocalToWorld();
        }
        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
                        ->skinningTransforms) {
            return _resolvedSkeletonSource->GetSkinningTransforms();
        }
        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
                        ->blendShapes) {
            return _GetAnimationSchema().GetBlendShapes();
        }
        if (name == UsdSkelImagingResolvedSkeletonSchemaTokens
                        ->blendShapeWeights) {
            return _GetAnimationSchema().GetBlendShapeWeights();
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

    const UsdSkelImagingAnimationSchema &_GetAnimationSchema() const {
        return _resolvedSkeletonSource->GetAnimationSchema();
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

/// Data source for primvars/points - for mesh guide.
class _PointsPrimvarDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsPrimvarDataSource);

    TfTokenVector GetNames() override {
        static const TfTokenVector names = {
            HdPrimvarSchemaTokens->primvarValue,
            HdPrimvarSchemaTokens->interpolation,
            HdPrimvarSchemaTokens->role };
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override {
        TRACE_FUNCTION();

        if (name == HdPrimvarSchemaTokens->primvarValue) {
            return _PointsPrimvarValueDataSource::New(
                _resolvedSkeletonSource->GetSkelGuideData(),
                _resolvedSkeletonSource->GetSkinningTransforms());
        }
        if (name == HdPrimvarSchemaTokens->interpolation) {
            static HdDataSourceBaseHandle const result =
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->vertex);
            return result;
        }
        if (name == HdPrimvarSchemaTokens->role) {
            static HdDataSourceBaseHandle const result =
                HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->point);
            return result;
        }
        return nullptr;
    }

private:
    _PointsPrimvarDataSource(
        UsdSkelImagingDataSourceResolvedSkeletonPrimHandle
            resolvedSkeletonSource)
     : _resolvedSkeletonSource(std::move(resolvedSkeletonSource))
    {
    }

    UsdSkelImagingDataSourceResolvedSkeletonPrimHandle const
        _resolvedSkeletonSource;
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
        return
            HdRetainedContainerDataSource::New(
                HdPrimvarsSchemaTokens->points,
                _PointsPrimvarDataSource::New(shared_from_this()));
    }

    return nullptr;
}

HdMatrixDataSourceHandle
UsdSkelImagingDataSourceResolvedSkeletonPrim::GetSkelLocalToWorld() const
{
    return HdXformSchema::GetFromParent(_primSource).GetMatrix();
}

HdMatrix4fArrayDataSourceHandle
UsdSkelImagingDataSourceResolvedSkeletonPrim::GetSkinningTransforms()
{
    TRACE_FUNCTION();

    return _SkinningTransformsDataSource::New(
        _skelDataCache.Get(),
        _restTransformsDataSource,
        _animationSchema.GetTranslations(),
        _animationSchema.GetRotations(),
        _animationSchema.GetScales());
}

const HdDataSourceLocatorSet &
UsdSkelImagingDataSourceResolvedSkeletonPrim::
GetDependendendOnDataSourceLocators()
{
    static const HdDataSourceLocatorSet result{
        UsdSkelImagingSkeletonSchema::GetDefaultLocator(),
        UsdSkelImagingBindingSchema::GetAnimationSourceLocator(),
        HdXformSchema::GetDefaultLocator()
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
            UsdSkelImagingBindingSchema::GetAnimationSourceLocator())) {
        // Our _animationSource and _animationSchema are invalid.
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

    if (dirtyLocators.Intersects(HdXformSchema::GetDefaultLocator())) {
        if (newDirtyLocators) {
            newDirtyLocators->insert(
                UsdSkelImagingResolvedSkeletonSchema::
                GetSkelLocalToWorldLocator());
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
ProcessDirtyLocators(
        const TfToken &dirtiedPrimType,
        const HdDataSourceLocatorSet &dirtyLocators,
        HdSceneIndexObserver::DirtiedPrimEntries * const entries)
{
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
