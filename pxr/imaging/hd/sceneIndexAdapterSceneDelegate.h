//
// Copyright 2021 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_IMAGING_HD_SCENE_INDEX_ADAPTER_SCENE_DELEGATE_H
#define PXR_IMAGING_HD_SCENE_INDEX_ADAPTER_SCENE_DELEGATE_H

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/usd/sdf/pathTable.h"

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

    GfMatrix4d GetInstancerTransform(
        SdfPath const &instancerId) override;
    size_t SampleInstancerTransform(SdfPath const &instancerId,
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
    size_t SampleIndexedPrimvar(SdfPath const &id, TfToken const &key,
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
    // Path Translation API

    SdfPath GetDataSharingId(SdfPath const& primId) override;

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

    HdExtComputationInputDescriptorVector GetExtComputationInputDescriptors(
        SdfPath const &computationId) override;
    HdExtComputationOutputDescriptorVector GetExtComputationOutputDescriptors(
        SdfPath const &computationId) override;

    std::string GetExtComputationKernel(SdfPath const &computationId) override;
    void InvokeExtComputation(SdfPath const &computationId,
                              HdExtComputationContext *context) override;

    void Sync(HdSyncRequestVector* request) override;
    void PostSyncCleanup() override;

    // NOTE: The remaining scene delegate functions aren't used for emulation:
    // - GetTaskRenderTags
    // - GetScenePrimPath
    // - IsEnabled

private:
    void _PrimAdded(
        const SdfPath &primPath,
        const TfToken &primType);

    VtValue _GetPrimvar(SdfPath const &id, TfToken const &key, 
        VtIntArray *outIndices);
    size_t _SamplePrimvar(SdfPath const &id, TfToken const &key,
        size_t maxNumSamples, float *times, VtValue *samples, 
        VtIntArray *sampleIndices);

    HdSceneIndexBaseRefPtr _inputSceneIndex;

    struct _PrimCacheEntry
    {
        _PrimCacheEntry()
        : primvarDescriptorsState(ReadStateUnread)
        , extCmpPrimvarDescriptorsState(ReadStateUnread)
        {}

        _PrimCacheEntry(const _PrimCacheEntry &rhs)
        {
            primType = rhs.primType;
            primvarDescriptorsState.store(rhs.primvarDescriptorsState.load());
            extCmpPrimvarDescriptorsState.store(
                rhs.extCmpPrimvarDescriptorsState.load());
        }

        TfToken primType;

        enum ReadState : unsigned char {
            ReadStateUnread = 0,
            ReadStateReading,
            ReadStateRead,
        };

        std::atomic<ReadState> primvarDescriptorsState;
        std::atomic<ReadState> extCmpPrimvarDescriptorsState;
        std::map<HdInterpolation, HdPrimvarDescriptorVector>
            primvarDescriptors;
        std::map<HdInterpolation, HdExtComputationPrimvarDescriptorVector> 
            extCmpPrimvarDescriptors;
    };

    using _PrimCacheTable = SdfPathTable<_PrimCacheEntry>;
    _PrimCacheTable _primCache;

    bool _sceneDelegatesBuilt;
    std::vector<HdSceneDelegate*> _sceneDelegates;

    // Cache for rprim locator set -> dirty bits translation.
    HdDataSourceLocatorSet _cachedLocatorSet;
    HdDirtyBits _cachedDirtyBits;
    TfToken _cachedPrimType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
