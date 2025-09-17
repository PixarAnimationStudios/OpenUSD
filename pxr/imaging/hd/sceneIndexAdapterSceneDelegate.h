//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SCENE_INDEX_ADAPTER_SCENE_DELEGATE_H
#define PXR_IMAGING_HD_SCENE_INDEX_ADAPTER_SCENE_DELEGATE_H

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/usd/sdf/pathTable.h"
#include <thread>
#include <tbb/concurrent_unordered_map.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSceneIndexAdapterSceneDelegate
///
/// Scene delegate which observes notices from an HdSceneIndex and applies them
/// to an HdRenderIndex. This serves as "back-end" emulation in order for
/// scenes described via the HdSceneIndex/HdDataSource APIs to be accessible
/// by legacy render delegates.
///
class HdSceneIndexAdapterSceneDelegate 
        : public HdSceneDelegate
        , public HdSceneIndexObserver
{
public:

    HdSceneIndexAdapterSceneDelegate(
            HdSceneIndexBaseRefPtr inputSceneIndex,
            HdRenderIndex *parentIndex,
            SdfPath const &delegateID);

    ~HdSceneIndexAdapterSceneDelegate() override;

    // ------------------------------------------------------------------------

    /// Returns the end of a scene index chain containing the filters
    /// necessary for input to an instance of this scene delegate.
    static HdSceneIndexBaseRefPtr AppendDefaultSceneFilters(
        HdSceneIndexBaseRefPtr inputSceneIndex, SdfPath const &delegateID);

    // satisfying HdSceneIndexObserver ----------------------------------------
    void PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries) override;

    void PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries) override;

    void PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries) override;

    void PrimsRenamed(
        const HdSceneIndexBase &sender,
        const RenamedPrimEntries &entries) override;

    // ------------------------------------------------------------------------
    // HdSceneIndexDelegate API

    // ------------------------------------------------------------------------
    // Rprim API

    HdMeshTopology GetMeshTopology(SdfPath const &id) override;
    HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const &id) override;
    PxOsdSubdivTags GetSubdivTags(SdfPath const &id) override;
    GfRange3d GetExtent(SdfPath const &id) override;
    bool GetVisible(SdfPath const &id) override;
    bool GetDoubleSided(SdfPath const &id) override;
    HdCullStyle GetCullStyle(SdfPath const &id) override;
    VtValue GetShadingStyle(SdfPath const &id) override;
    HdDisplayStyle GetDisplayStyle(SdfPath const &id) override;
    HdReprSelector GetReprSelector(SdfPath const &id) override;
    TfToken GetRenderTag(SdfPath const &id) override;
    VtArray<TfToken> GetCategories(SdfPath const &id) override;
    HdVolumeFieldDescriptorVector GetVolumeFieldDescriptors(
            SdfPath const &volumeId) override;

    // ------------------------------------------------------------------------
    // Transform API

    GfMatrix4d GetTransform(SdfPath const &id) override;
    size_t SampleTransform(SdfPath const &id, size_t maxSampleCount,
        float *sampleTimes, GfMatrix4d *sampleValues) override;
    size_t SampleTransform(SdfPath const &id,
        float startTime, float endTime,
        size_t maxSampleCount,
        float *sampleTimes, GfMatrix4d *sampleValues) override;

    GfMatrix4d GetInstancerTransform(
        SdfPath const &instancerId) override;
    size_t SampleInstancerTransform(SdfPath const &instancerId,
        size_t maxSampleCount, float *sampleTimes,
        GfMatrix4d *sampleValues) override;
    size_t SampleInstancerTransform(SdfPath const &instancerId,
        float startTime, float endTime,
        size_t maxSampleCount, float *sampleTimes,
        GfMatrix4d *sampleValues) override;

    // ------------------------------------------------------------------------
    // Primvar API

    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(
        SdfPath const &id, HdInterpolation interpolation) override;
    
    VtValue Get(SdfPath const &id, TfToken const &key) override;
    VtValue GetIndexedPrimvar(SdfPath const &id, TfToken const &key, 
            VtIntArray *outIndices) override;

    size_t SamplePrimvar(SdfPath const &id, TfToken const &key,
            size_t maxSampleCount, float *sampleTimes, 
            VtValue *sampleValues) override;
    size_t SamplePrimvar(SdfPath const &id, TfToken const &key,
            float startTime, float endTime,
            size_t maxSampleCount, float *sampleTimes, 
            VtValue *sampleValues) override;
    size_t SampleIndexedPrimvar(SdfPath const &id, TfToken const &key,
            size_t maxNumSamples, float *times, VtValue *samples, 
            VtIntArray *sampleIndices) override;
    size_t SampleIndexedPrimvar(SdfPath const &id, TfToken const &key,
            float startTime, float endTime,                                
            size_t maxNumSamples, float *times, VtValue *samples, 
            VtIntArray *sampleIndices) override;
    
    // ------------------------------------------------------------------------
    // Instancer API

    std::vector<VtArray<TfToken>> GetInstanceCategories(
        SdfPath const &instancerId) override;
    VtIntArray GetInstanceIndices(
        SdfPath const &instancerId, SdfPath const &prototypeId) override;
    SdfPath GetInstancerId(SdfPath const &primId) override;
    SdfPathVector GetInstancerPrototypes(SdfPath const &instancerId) override;

    // ------------------------------------------------------------------------
    // Material API

    SdfPath GetMaterialId(SdfPath const &id) override;
    VtValue GetMaterialResource(SdfPath const &id) override;
    HdIdVectorSharedPtr GetCoordSysBindings(SdfPath const &id) override;

    // ------------------------------------------------------------------------
    // Renderbuffer API

    HdRenderBufferDescriptor GetRenderBufferDescriptor(
        SdfPath const &id) override;

    // ------------------------------------------------------------------------
    // Light API

    VtValue GetLightParamValue(SdfPath const &id,
        TfToken const &paramName) override;

    // ------------------------------------------------------------------------
    // Camera API

    VtValue GetCameraParamValue(SdfPath const &cameraId,
        TfToken const &paramName) override;

    // ------------------------------------------------------------------------
    // ExtComputation API

    // ... on the rprim
    HdExtComputationPrimvarDescriptorVector
        GetExtComputationPrimvarDescriptors(
            SdfPath const &id, HdInterpolation interpolationMode) override;

    // ... on the sprim
    TfTokenVector GetExtComputationSceneInputNames(
        SdfPath const &computationId) override;
    VtValue GetExtComputationInput(
        SdfPath const &computationId, TfToken const &input) override;
    size_t SampleExtComputationInput(
        SdfPath const &computationId,
        TfToken const &input,
        size_t maxSampleCount,
        float *sampleTimes,
        VtValue *sampleValues) override;
    size_t SampleExtComputationInput(
        SdfPath const &computationId,
        TfToken const &input,
        float startTime,
        float endTime,
        size_t maxSampleCount,
        float *sampleTimes,
        VtValue *sampleValues) override;

    HdExtComputationInputDescriptorVector GetExtComputationInputDescriptors(
        SdfPath const &computationId) override;
    HdExtComputationOutputDescriptorVector GetExtComputationOutputDescriptors(
        SdfPath const &computationId) override;

    std::string GetExtComputationKernel(SdfPath const &computationId) override;
    void InvokeExtComputation(SdfPath const &computationId,
                              HdExtComputationContext *context) override;

    TfTokenVector GetTaskRenderTags(SdfPath const &taskId) override;
    
    void Sync(HdSyncRequestVector* request) override;
    void PostSyncCleanup() override;

    // NOTE: The remaining scene delegate functions aren't used for emulation:
    // - GetScenePrimPath
    // - IsEnabled

private:
    // Compute and return an HdSceneIndexPrim from the input scene index.
    // Uses a per-thread single-entry cache to re-use this computation
    // across sequential Get...() calls in the public API.  This API returns
    // the prim by value rather than reference because callers may
    // indirectly re-invoke _GetInputPrim() on the same thread, but with
    // a different id path, if they make use of a TBB work queue.
    HdSceneIndexPrim _GetInputPrim(SdfPath const& id);

    using _InputPrimCacheEntry = std::pair<SdfPath, HdSceneIndexPrim>;

    // A cache of the last prim accessed, per thread
    tbb::concurrent_unordered_map<std::thread::id, _InputPrimCacheEntry,
        std::hash<std::thread::id> > _inputPrimCache;

    void _PrimAdded(
        const SdfPath &primPath,
        const TfToken &primType);

    VtValue _GetPrimvar(SdfPath const &id, TfToken const &key, 
        VtIntArray *outIndices);

    VtValue _GetPrimvar(
        const HdContainerDataSourceHandle &primvarsDataSource, 
        TfToken const &key,
        VtIntArray *outIndices);

    VtValue _GetImageShaderValue(
        HdSceneIndexPrim prim,
        const TfToken& key);

    size_t _SamplePrimvar(SdfPath const &id, TfToken const &key,
        float startTime, float endTime,
        size_t maxNumSamples, float *times, VtValue *samples, 
        VtIntArray *sampleIndices);

    HdSceneIndexBaseRefPtr _inputSceneIndex;

    struct _PrimCacheEntry
    {
        TfToken primType;

        using PrimvarDescriptorsArray =
            std::array<HdPrimvarDescriptorVector, HdInterpolationCount>;
        std::shared_ptr<PrimvarDescriptorsArray> primvarDescriptors;
        using ExtCmpPrimvarDescriptorsArray =
            std::array<HdExtComputationPrimvarDescriptorVector,
                HdInterpolationCount>;
        std::shared_ptr<ExtCmpPrimvarDescriptorsArray> extCmpPrimvarDescriptors;
    };

    using _PrimCacheTable = SdfPathTable<_PrimCacheEntry>;
    _PrimCacheTable _primCache;

    std::shared_ptr<_PrimCacheEntry::PrimvarDescriptorsArray>
        _ComputePrimvarDescriptors(
            const HdContainerDataSourceHandle &primDataSource);
    std::shared_ptr<_PrimCacheEntry::ExtCmpPrimvarDescriptorsArray>
        _ComputeExtCmpPrimvarDescriptors(
            const HdContainerDataSourceHandle &primDataSource);

    bool _sceneDelegatesBuilt;
    std::vector<HdSceneDelegate*> _sceneDelegates;

    // Hint cache of all prim paths that have been populated with
    // geomSubset children.  This is purely an optimization and
    // not authoritative -- it may have false positioves, such as
    // if subsets are removed later.  These hints provide a way
    // to skip the expense of _GatherGeomSubsets() when no subsets
    // have been populated.
    std::unordered_set<SdfPath, SdfPath::Hash> _geomSubsetParents;

    // Cache for rprim locator set -> dirty bits translation.
    HdDataSourceLocatorSet _cachedLocatorSet;
    HdDirtyBits _cachedDirtyBits;
    TfToken _cachedPrimType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
