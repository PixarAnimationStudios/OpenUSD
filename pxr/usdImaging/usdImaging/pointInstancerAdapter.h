//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_POINT_INSTANCER_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_POINT_INSTANCER_ADAPTER_H

/// \file usdImaging/pointInstancerAdapter.h

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/instanceablePrimAdapter.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/denseHashMap.h"

#include "pxr/pxr.h"

#include <atomic>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

/// Delegate support for UsdGeomPointInstancer
///
class UsdImagingPointInstancerAdapter : public UsdImagingInstanceablePrimAdapter 
{
public:
    using BaseAdapter = UsdImagingInstanceablePrimAdapter;

    USDIMAGING_API
    SdfPath Populate(
        UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const* instancerContext = nullptr) override;

    USDIMAGING_API
    bool ShouldCullChildren() const override;

    USDIMAGING_API
    bool IsInstancerAdapter() const override;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDIMAGING_API
    TfToken GetImagingSubprimType(
            UsdPrim const& prim,
            TfToken const& subprim) override;

    USDIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
    
    USDIMAGING_API
    void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL) const override;

    USDIMAGING_API
    void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    HdDirtyBits ProcessPropertyChange(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              TfToken const& propertyName) 
                                                override;

    USDIMAGING_API
    void ProcessPrimResync(SdfPath const& cachePath,
                                   UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void ProcessPrimRemoval(SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkRefineLevelDirty(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkReprDirty(UsdPrim const& prim,
                               SdfPath const& cachePath,
                               UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkCullStyleDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkRenderTagDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkTransformDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkVisibilityDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkLightParamsDirty(
        const UsdPrim& prim,
        const SdfPath& cachePath,
        UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkCollectionsDirty(
        const UsdPrim& prim,
        const SdfPath& cachePath,
        UsdImagingIndexProxy* index) override;

    // ---------------------------------------------------------------------- //
    /// \name Instancing
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    GfMatrix4d GetInstancerTransform(UsdPrim const& instancerPrim,
                                     SdfPath const& instancerPath,
                                     UsdTimeCode time) const override;

    USDIMAGING_API
    size_t SampleInstancerTransform(UsdPrim const& instancerPrim,
                                    SdfPath const& instancerPath,
                                    UsdTimeCode time,
                                    size_t maxNumSamples,
                                    float *sampleTimes,
                                    GfMatrix4d *sampleValues) override;

    USDIMAGING_API
    SdfPath GetInstancerId(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const override;

    USDIMAGING_API
    SdfPathVector GetInstancerPrototypes(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const override;

    USDIMAGING_API
    GfMatrix4d GetTransform(UsdPrim const& prim, 
                            SdfPath const& cachePath,
                            UsdTimeCode time,
                            bool ignoreRootTransform = false) const override;

    USDIMAGING_API
    size_t SampleTransform(UsdPrim const& prim, 
                           SdfPath const& cachePath,
                           UsdTimeCode time, 
                           size_t maxNumSamples, 
                           float  *sampleTimes,
                           GfMatrix4d *sampleValues) override;

    USDIMAGING_API
    size_t SamplePrimvar(UsdPrim const& usdPrim,
                         SdfPath const& cachePath,
                         TfToken const& key,
                         UsdTimeCode time,
                         size_t maxNumSamples, 
                         float *sampleTimes,
                         VtValue *sampleValues,
                         VtIntArray *sampleIndices) override;

    USDIMAGING_API
    PxOsdSubdivTags GetSubdivTags(UsdPrim const& usdPrim,
                                  SdfPath const& cachePath,
                                  UsdTimeCode time) const override;

    USDIMAGING_API
    bool IsChildPath(const SdfPath& path) const override;

    bool GetVisible(UsdPrim const& prim, 
                    SdfPath const& cachePath,
                    UsdTimeCode time) const override;

    USDIMAGING_API
    TfToken GetPurpose(
        UsdPrim const& usdPrim, 
        SdfPath const& cachePath,
        TfToken const& instanceInheritablePurpose) const override;

    USDIMAGING_API
    VtValue GetTopology(UsdPrim const& prim,
                        SdfPath const& cachePath,
                        UsdTimeCode time) const override;

    USDIMAGING_API
    HdCullStyle GetCullStyle(UsdPrim const& prim,
                             SdfPath const& cachePath,
                             UsdTimeCode time) const override;

    USDIMAGING_API
    GfRange3d GetExtent(UsdPrim const& usdPrim, 
                        SdfPath const& cachePath, 
                        UsdTimeCode time) const override;

    USDIMAGING_API
    bool GetDoubleSided(UsdPrim const& usdPrim, 
                   SdfPath const& cachePath, 
                   UsdTimeCode time) const override;


    USDIMAGING_API
    SdfPath GetMaterialId(UsdPrim const& prim, 
                          SdfPath const& cachePath, 
                          UsdTimeCode time) const override;

    USDIMAGING_API
    VtValue GetLightParamValue(
        const UsdPrim& prim,
        const SdfPath& cachePath,
        const TfToken& paramName,
        UsdTimeCode time) const override;
    
    USDIMAGING_API
    VtValue GetMaterialResource(
        const UsdPrim& prim,
        const SdfPath& cachePath,
        UsdTimeCode time) const override;

    USDIMAGING_API
    HdExtComputationInputDescriptorVector
    GetExtComputationInputs(UsdPrim const& prim,
                            SdfPath const& cachePath,
                            const UsdImagingInstancerContext* instancerContext)
                                    const override;

    USDIMAGING_API
    HdExtComputationOutputDescriptorVector
    GetExtComputationOutputs(UsdPrim const& prim,
                             SdfPath const& cachePath,
                             const UsdImagingInstancerContext* instancerContext)
                                    const override;

    USDIMAGING_API
    HdExtComputationPrimvarDescriptorVector
    GetExtComputationPrimvars(
            UsdPrim const& prim,
            SdfPath const& cachePath,
            HdInterpolation interpolation,
            const UsdImagingInstancerContext* instancerContext) const override;

    USDIMAGING_API
    VtValue 
    GetExtComputationInput(
            UsdPrim const& prim,
            SdfPath const& cachePath,
            TfToken const& name,
            UsdTimeCode time,
            const UsdImagingInstancerContext* instancerContext) const override;

    USDIMAGING_API
    std::string 
    GetExtComputationKernel(
            UsdPrim const& prim,
            SdfPath const& cachePath,
            const UsdImagingInstancerContext* instancerContext) const override;

    USDIMAGING_API
    VtValue
    GetInstanceIndices(UsdPrim const& instancerPrim,
                       SdfPath const& instancerCachePath,
                       SdfPath const& prototypeCachePath,
                       UsdTimeCode time) const override;

    USDIMAGING_API
    VtValue Get(UsdPrim const& prim,
                SdfPath const& cachePath,
                TfToken const& key,
                UsdTimeCode time,
                VtIntArray *outIndices) const override;

    // ---------------------------------------------------------------------- //
    /// \name Nested instancing support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    GfMatrix4d GetRelativeInstancerTransform(
        SdfPath const &instancerPath,
        SdfPath const &protoInstancerPath,
        UsdTimeCode time) const override;

    // ---------------------------------------------------------------------- //
    /// \name Picking & selection
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    SdfPath GetScenePrimPath(
        SdfPath const& cachePath,
        int instanceIndex,
        HdInstancerContext *instancerContext) const override;

    USDIMAGING_API
    SdfPathVector GetScenePrimPaths(
        SdfPath const& cachePath,
        std::vector<int> const& instanceIndices,
        std::vector<HdInstancerContext> *instancerCtxs) const override;

    USDIMAGING_API
    bool PopulateSelection(
        HdSelection::HighlightMode const& highlightMode,
        SdfPath const &cachePath,
        UsdPrim const &usdPrim,
        int const hydraInstanceIndex,
        VtIntArray const &parentInstanceIndices,
        HdSelectionSharedPtr const &result) const override;

    // ---------------------------------------------------------------------- //
    /// \name Volume field information
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    HdVolumeFieldDescriptorVector
    GetVolumeFieldDescriptors(UsdPrim const& usdPrim, SdfPath const &id,
                              UsdTimeCode time) const override;

protected:
    friend class UsdImagingInstanceAdapter;
    // ---------------------------------------------------------------------- //
    /// \name Utility
    // ---------------------------------------------------------------------- //
    
    // Given the USD path for a prim of this adapter's type, returns
    // the prim's Hydra cache path. This version will reserve a path in
    // the adapter's instancer data map for the given point instancer USD
    // path, including any necessary variant selection path.
    //
    // This method is marked const, but it is not const! If called with the
    // path of a populated point instancer, it will modify the instancer data
    // cache and return a new path. 
    USDIMAGING_API
    SdfPath
    ResolveCachePath(
        const SdfPath& usdPath,
        const UsdImagingInstancerContext* ctx = nullptr) const override;
        
    USDIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) override final;

private:
    struct _ProtoPrim;
    struct _InstancerData;

    SdfPath _Populate(UsdPrim const& prim,
                   UsdImagingIndexProxy* index,
                   UsdImagingInstancerContext const* instancerContext);

    void _PopulatePrototype(int protoIndex,
                            _InstancerData& instrData,
                            UsdPrim const& protoRootPrim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const *instancerContext);

    // Process prim removal and output a set of affected instancer paths is
    // provided.
    void _ProcessPrimRemoval(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index,
                             SdfPathVector* instancersToReload);

    // Removes all instancer data, both locally and from the render index.
    void _UnloadInstancer(SdfPath const& instancerPath,
                          UsdImagingIndexProxy* index);

    // Updates per-frame instancer visibility.
    void _UpdateInstancerVisibility(SdfPath const& instancerPath,
                                    _InstancerData const& instrData,
                                    UsdTimeCode time) const;

    // Returns true if the instancer is visible, taking into account all
    // parent instancers visibilities.
    bool _GetInstancerVisible(SdfPath const &instancerPath, UsdTimeCode time) 
        const;

    // Gets the associated _ProtoPrim for the given instancer and cache path.
    _ProtoPrim const& _GetProtoPrim(SdfPath const& instancerPath, 
                                    SdfPath const& cachePath) const;

    // Gets the associated _ProtoPrim and instancerContext if cachePath is a 
    // child path and returns \c true, otherwise returns \c false.
    //
    // Note that the returned instancer context may not be as fully featured as
    // your needs may be.
    bool _GetProtoPrimForChild(
            UsdPrim const& usdPrim,
            SdfPath const& cachePath,
            _ProtoPrim const** proto,
            UsdImagingInstancerContext* ctx) const;

    // Gets the UsdPrim to use from the given _ProtoPrim.
    const UsdPrim _GetProtoUsdPrim(_ProtoPrim const& proto) const;

    // Takes the transform applies a corrective transform to 1) remove any
    // transforms above the model root (root proto path) and 2) apply the 
    // instancer transform.
    GfMatrix4d _CorrectTransform(UsdPrim const& instancer,
                                 UsdPrim const& proto,
                                 SdfPath const& cachePath,
                                 SdfPathVector const& protoPathChain,
                                 GfMatrix4d const& inTransform,
                                 UsdTimeCode time) const;

    // Similar to CorrectTransform, requires a visibility value exist in the
    // ValueCache, removes any visibility opinions above the model root (proto
    // root path) and applies the instancer visibility.
    void _ComputeProtoVisibility(UsdPrim const& protoRoot,
                                 UsdPrim const& protoGprim,
                                 UsdTimeCode time,
                                 bool* vis) const;

    /*
      PointInstancer (InstancerData)
         |
         +-- Prototype[0]------+-- ProtoRprim (mesh, curve, ...)
         |                     +-- ProtoRprim
         |                     +-- ProtoRprim
         |
         +-- Prototype[1]------+-- ProtoRprim
         |                     +-- ProtoRprim
         .
         .
     */

    // A proto prim represents a single populated prim under a prototype root
    // declared on the instancer. For example, a character may be targeted
    // by the prototypes relationship; it will have many meshes, and each
    // mesh is represented as a separate proto prim.
    struct _ProtoPrim {
        _ProtoPrim() : variabilityBits(0), visible(true) {}
        // Each prim will become a prototype "child" under the instancer.
        // paths is a list of paths we had to hop across when resolving native
        // USD instances.
        SdfPathVector paths;
        // The prim adapter for the actual prototype prim.
        UsdImagingPrimAdapterSharedPtr adapter;
        // The root prototype path, typically the model root, which is a subtree
        // and might contain several imageable prims.
        SdfPath protoRootPath;
        // Tracks the variability of the underlying adapter to avoid
        // redundantly reading data. This value is stored as
        // HdDirtyBits bit flags.
        // XXX: This is mutable so we can set it in TrackVariability.
        mutable HdDirtyBits variabilityBits;
        // When variabilityBits does not include HdChangeTracker::DirtyVisibility
        // the visible field is the unvarying value for visibility.
        // XXX: This is mutable so we can set it in TrackVariability.
        mutable bool visible;
    };

    // Indexed by cachePath (each prim has one entry)
    typedef std::unordered_map<SdfPath, _ProtoPrim, SdfPath::Hash> _ProtoPrimMap;

    // All data associated with a given Instancer prim. PrimMap could
    // technically be split out to avoid two lookups, however it seems cleaner
    // to keep everything bundled up under the instancer path.
    struct _InstancerData {
        _InstancerData() {}
        SdfPath parentInstancerCachePath;
        _ProtoPrimMap protoPrimMap;
        SdfPathVector prototypePaths;

        using PathToIndexMap = TfDenseHashMap<SdfPath, size_t, SdfPath::Hash>;
        PathToIndexMap prototypePathIndices;


        // XXX: We keep a bunch of state around visibility that's set in
        // TrackVariability and UpdateForTime.  "visible", and "visibleTime"
        // (the cache key for visible) are set in UpdateForTime and guarded
        // by "mutex".
        mutable std::mutex mutex;
        mutable bool variableVisibility;
        mutable bool visible;
        mutable UsdTimeCode visibleTime;
        mutable std::atomic_bool initialized = std::atomic_bool(false);
    };

    // A map of instancer data, one entry per instancer prim that has been
    // populated. This must be mutable so we can modify it in ResolveCachePath.
    // Note: this is accessed in multithreaded code paths and must be protected
    typedef std::unordered_map<SdfPath /*instancerPath*/, 
                               _InstancerData, 
                               SdfPath::Hash> _InstancerDataMap;
    mutable _InstancerDataMap _instancerData;
    
    inline static std::atomic_int _globalVariantCounter = std::atomic_int(0);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_POINT_INSTANCER_ADAPTER_H
