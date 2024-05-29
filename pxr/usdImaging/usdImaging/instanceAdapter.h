//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_INSTANCE_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_INSTANCE_ADAPTER_H

/// \file usdImaging/instanceAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/base/tf/hashmap.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingInstanceAdapter
///
/// Delegate support for instanced prims.
///
/// In addition to prim schemas that support instancing, like the point
/// instancer, USD has a built in instancing feature that will allow prims
/// composed from the same assets, with compatible attributes, to be
/// de-duplicated inside of USD.
///
/// When these prims are found during scene load, the prim location is
/// marked as an instance (meaning prim.IsInstance() == true), and its
/// descendants are added to a new hidden scene root.  There can be
/// multiple prototype scene roots, and each one can be pointed to by
/// many instance prims, and these prototype sub-scenes can themselves
/// contain instances.
///
/// We handle this by sending all instance prims to the instance adapter. In
/// order to preserve USD's native instancing work during rendering, for each
/// prototype scene root, we insert one hydra gprim per prototype USD gprim,
/// and we insert a hydra instancer that computes all of the places these gprims
/// (and any child instancers) are referenced in the scene, adjusting the
/// instancing count accordingly.
///
/// The instance adapter is responsible for computing and passing down a
/// small amount of inheritable data that we allow to vary per-instance:
/// for example, transform and visibility state, and inherited constant
/// primvars.  Otherwise, prototypes have no knowledge of the instance prims
/// that refer to them.
///
/// Just like the scene root, the root of the prototype tree isn't allowed to
/// have attributes or a prim type; those are set on the instance prim instead.
/// This means if a gprim is directly instanced, USD won't actually de-duplicate
/// it.  The instance adapter could theoretically bucket such gprims together,
/// but the difficulty of doing so is the same as the difficulty of
/// deduplicating arbitrary prims in the scene.  Instead, the instance adapter
/// refuses to image directly-instanced gprims, and the recommended authoring
/// guidelines is to only enable USD instancing on enclosing scopes or xforms.
///
/// There's a small set of extremely-special-case prims that are allowed to be
/// directly instanced, including cards and support prims that designate e.g.
/// skinning buffers.  These prim adapters opt-in via CanPopulateUsdInstance,
/// and generally require very careful coding and support in the instance
/// adapter; but they are useful for restricted schemas where we know how to
/// vary the data per-instance or know how to efficiently aggregate instances.
///
/// Finally, there's a small (hopefully shrinking) set of inherited attributes
/// that we need to respect, but don't know how to vary per-instance; for
/// example, material bindings.  If two instances point to the same USD proto
/// root, but have different material bindings, we currently populate two
/// hydra instancers with two sets of hydra prototypes.  This cuts into the
/// efficiency of instancing, so we try to minimize it.
///
class UsdImagingInstanceAdapter : public UsdImagingPrimAdapter
{
public:
    using BaseAdapter = UsdImagingPrimAdapter;
    
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

    // As this adapter hijacks the adapter for the child prim
    // We need to forward these messages on, in case the child
    // adapter needs them
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

    // ---------------------------------------------------------------------- //
    /// \name Instancing
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    std::vector<VtArray<TfToken>> GetInstanceCategories(
        UsdPrim const& prim) override;

    USDIMAGING_API
    GfMatrix4d GetInstancerTransform(UsdPrim const& instancerPrim,
                                     SdfPath const& instancerPath,
                                     UsdTimeCode time) const override;

    USDIMAGING_API
    SdfPath GetInstancerId(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const override;

    USDIMAGING_API
    SdfPathVector GetInstancerPrototypes(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const override;

    USDIMAGING_API
    size_t SampleInstancerTransform(UsdPrim const& instancerPrim,
                                    SdfPath const& instancerPath,
                                    UsdTimeCode time,
                                    size_t maxSampleCount,
                                    float *sampleTimes,
                                    GfMatrix4d *sampleValues) override;

    USDIMAGING_API
    size_t SampleTransform(UsdPrim const& prim, 
                           SdfPath const& cachePath,
                           UsdTimeCode time, 
                           size_t maxNumSamples, 
                           float *sampleTimes,
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
    TfToken GetPurpose(
        UsdPrim const& usdPrim, 
        SdfPath const& cachePath,
        TfToken const& instanceInheritablePurpose) const override;

    USDIMAGING_API
    PxOsdSubdivTags GetSubdivTags(UsdPrim const& usdPrim,
                                  SdfPath const& cachePath,
                                  UsdTimeCode time) const override;

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
    bool IsChildPath(const SdfPath& path) const override;

    bool GetVisible(UsdPrim const& usdPrim, 
                    SdfPath const& cachePath,
                    UsdTimeCode time) const override;

    USDIMAGING_API
    bool GetDoubleSided(UsdPrim const& prim, 
                        SdfPath const& cachePath, 
                        UsdTimeCode time) const override;

    USDIMAGING_API
    GfMatrix4d GetTransform(UsdPrim const& prim, 
                            SdfPath const& cachePath,
                            UsdTimeCode time,
                            bool ignoreRootTransform = false) const override;

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
        SdfPath const &parentInstancerPath,
        SdfPath const &instancerPath,
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
    USDIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) override final;

private:

    SdfPath _Populate(UsdPrim const& prim,
                      UsdImagingIndexProxy* index,
                      UsdImagingInstancerContext const* instancerContext,
                      SdfPath const& parentProxyPath);

    struct _ProtoPrim;
    struct _ProtoGroup;
    struct _InstancerData;

    bool _IsChildPrim(UsdPrim const& prim,
                      SdfPath const& cachePath) const;

    // Returns true if the given prim serves as an instancer.
    bool _PrimIsInstancer(UsdPrim const& prim) const;

    // Inserts prototype prims for the usdPrim given by \p iter into the
    // \p index. Any inserted gprims will be inserted under a special path
    // combining \p ctx.instancerCachePath and \p ctx.childName.
    SdfPath 
    _InsertProtoPrim(UsdPrimRange::iterator *iter,
                     const UsdImagingPrimAdapterSharedPtr& primAdapter,
                     const UsdImagingInstancerContext& ctx,
                     UsdImagingIndexProxy* index);

    // For a usd path, collects the instancers to resync.
    void _ResyncPath(SdfPath const& cachePath,
                     UsdImagingIndexProxy* index,
                     bool reload);
    // Removes and optionally reloads all instancer data, both locally and
    // from the render index.
    void _ResyncInstancer(SdfPath const& instancerPath,
                          UsdImagingIndexProxy* index, bool reload);

    // Computes per-frame data in the instancer map. This is primarily used
    // during update to send new instance indices out to Hydra.
    struct _ComputeInstanceMapFn;
    VtIntArray _ComputeInstanceMap(UsdPrim const& instancerPrim,
            _InstancerData const& instrData,
            UsdTimeCode time) const;

    // Precomputes the instancer visibility data (as visible, invis, varying
    // per-node), and returns whether the instance map is variable.
    // Note: this function assumes the instancer data is already locked by
    // the caller...
    struct _ComputeInstanceMapVariabilityFn;
    bool _ComputeInstanceMapVariability(UsdPrim const& instancerPrim,
                                        _InstancerData const& instrData) const;

    // Gets the associated _ProtoPrim and instancer context for the given 
    // instancer and cache path.
    _ProtoPrim const& _GetProtoPrim(SdfPath const& instancerPath, 
                                     SdfPath const& cachePath,
                                     UsdImagingInstancerContext* ctx) const;

    // Gets the associated _ProtoPrim and instancerContext if cachePath is a 
    // child path and returns \c true, otherwise returns \c false.
    bool _GetProtoPrimForChild(
            UsdPrim const& usdPrim,
            SdfPath const& cachePath,
            _ProtoPrim const** proto,
            UsdImagingInstancerContext* ctx) const;

    // Computes the transforms for all instances corresponding to the given
    // instancer.
    struct _ComputeInstanceTransformFn;
    bool _ComputeInstanceTransforms(UsdPrim const& instancer,
                                    VtMatrix4dArray* transforms,
                                    UsdTimeCode time) const;

    // Gathers the authored transforms time samples given an instancer.
    struct _GatherInstanceTransformTimeSamplesFn;
    bool _GatherInstanceTransformsTimeSamples(UsdPrim const& instancer,
                                              GfInterval interval,
                                              std::vector<double>* outTimes) 
                                                  const;

    // Gathers the specified primvar time samples given an instancer.
    struct _GatherInstancePrimvarTimeSamplesFn;
    bool _GatherInstancePrimvarTimeSamples(UsdPrim const& instancer,
                                           TfToken const& key,
                                           GfInterval interval,
                                           std::vector<double>* outTimes) 
                                               const;

    // Returns true if any of the instances corresponding to the given
    // instancer has a varying transform.
    struct _IsInstanceTransformVaryingFn;
    bool _IsInstanceTransformVarying(UsdPrim const& instancer) const;

    // Computes the value of a primvar for all instances corresponding to the
    // given instancer. The templated version runs the templated functor,
    // and the un-templated version does type dispatch.
    template<typename T> struct _ComputeInheritedPrimvarFn;

    template<typename T>
    bool _ComputeInheritedPrimvar(UsdPrim const& instancer,
                                  TfToken const& primvarName,
                                  VtValue *result,
                                  UsdTimeCode time) const;

    bool _ComputeInheritedPrimvar(UsdPrim const& instancer,
                                  TfToken const& primvarName,
                                  SdfValueTypeName const& type,
                                  VtValue *result,
                                  UsdTimeCode time) const;

    // Returns true if any of the instances corresponding to the given
    // instancer has varying inherited primvars.
    struct _IsInstanceInheritedPrimvarVaryingFn;
    bool _IsInstanceInheritedPrimvarVarying(UsdPrim const& instancer) const;

    struct _PopulateInstanceSelectionFn;
    struct _GetScenePrimPathsFn;
    struct _GetInstanceCategoriesFn;

    // Helper functions for dealing with "actual" instances to be drawn.
    //
    // Suppose we have:
    //    /Root
    //        Instance_A (prototype: /__Prototype_1)
    //        Instance_B (prototype: /__Prototype_1)
    //    /__Prototype_1
    //        AnotherInstance_A (prototype: /__Prototype_2)
    //    /__Prototype_2
    //
    // /__Prototype_2 has only one associated instance in the Usd scenegraph: 
    // /__Prototype_1/AnotherInstance_A. However, imaging actually needs to draw
    // two instances of /__Prototype_2, because AnotherInstance_A is a nested 
    // instance beneath /__Prototype_1, and there are two instances of
    // /__Prototype_1.
    //
    // Each instance to be drawn is addressed by the chain of instances
    // that caused it to be drawn. In the above example, the two instances 
    // of /__Prototype_2 to be drawn are:
    //
    //  [ /Root/Instance_A, /__Prototype_1/AnotherInstance_A ],
    //  [ /Root/Instance_B, /__Prototype_1/AnotherInstance_A ]
    //
    // This "instance context" describes the chain of opinions that
    // ultimately affect the final drawn instance. For example, the 
    // transform of each instance to draw is the combined transforms
    // of the prims in each context.
    template <typename Functor>
    void _RunForAllInstancesToDraw(UsdPrim const& instancer, Functor* fn) const;
    template <typename Functor>
    bool _RunForAllInstancesToDrawImpl(UsdPrim const& instancer, 
                                       std::vector<UsdPrim>* instanceContext,
                                       size_t* instanceIdx,
                                       Functor* fn) const;

    typedef TfHashMap<SdfPath, size_t, SdfPath::Hash> _InstancerDrawCounts;
    size_t _CountAllInstancesToDraw(UsdPrim const& instancer) const;
    size_t _CountAllInstancesToDrawImpl(UsdPrim const& instancer,
                                        _InstancerDrawCounts* drawCounts) const;

    // A proto prim represents a single adapter under a prototype root declared
    // on the instancer.
    struct _ProtoPrim {
        _ProtoPrim() {}
        // Each prim will become a prototype "child" under the instancer. This
        // path is the path to the prim on the Usd Stage (the path to a single
        // mesh, for example).
        SdfPath path;           
        // The prim adapter for the actual prototype prim.
        UsdImagingPrimAdapterSharedPtr adapter;
    };

    // Indexed by prototype cachePath (each prim has one entry)
    typedef TfHashMap<SdfPath, _ProtoPrim, SdfPath::Hash> _PrimMap;

    // All data associated with a given instancer prim. PrimMap could
    // technically be split out to avoid two lookups, however it seems cleaner
    // to keep everything bundled up under the instancer path.
    struct _InstancerData {
        _InstancerData() : numInstancesToDraw(0), refresh(false) { }

        // The prototype prim path associated with this instancer.
        SdfPath prototypePath;

        // The USD material path associated with this instancer.
        SdfPath materialUsdPath;

        // The drawmode associated with this instancer.
        TfToken drawMode;

        // The purpose value associated with this instance that can be inherited
        // by proto prims that need to inherit ancestor purpose.
        TfToken inheritablePurpose;

        // Inherited primvar
        struct PrimvarInfo {
            TfToken name;
            SdfValueTypeName type;
            bool operator==(const PrimvarInfo& rhs) const;
            bool operator<(const PrimvarInfo& rhs) const;
        };
        std::vector<PrimvarInfo> inheritedPrimvars;

        // Paths to Usd instance prims. Note that this is not necessarily
        // equivalent to all the instances that will be drawn. See below.
        SdfPathSet instancePaths;

        // Number of actual instances of this instancer that will be 
        // drawn. See comment on _RunForAllInstancesToDraw.
        // XXX: This is mutable so that we can precache it in TrackVariability;
        // it's inappropriate to track it in _Populate since not all instances
        // will have been populated.
        mutable size_t numInstancesToDraw;

        // Cached visibility. This vector contains an entry for each instance
        // that will be drawn (i.e., visibility.size() == numInstancesToDraw).
        enum Visibility {
            Invisible, //< Invisible over all time
            Visible,   //< Visible over all time
            Varying,   //< Visibility varies over time
            Unknown    //< Visibility has not yet been checked
        };
        // XXX: This is mutable so that we can precache visibility per-instance
        // in TrackVariability().  Can we replace this with some kind of usage
        // of an inherited cache?
        mutable std::vector<Visibility> visibility;

        // Map of all rprims for this instancer prim.
        _PrimMap primMap;

        // This is a set of reference paths, where this instancer needs
        // to deferer to another instancer.  While refered to here as a child
        // instancer, the actual relationship is more like a directed graph.
        SdfPathSet childPointInstancers;

        // Nested (child) native instances.
        SdfPathVector nestedInstances;

        // Parent native instances.
        SdfPathVector parentInstances;

        // Flag indicating we've asked the delegate to refresh this instancer
        // (via TrackVariability/UpdateForTime).  We record this so we don't
        // do it multiple times.
        mutable bool refresh;
    };

    // Map from hydra instancer cache path to the various instancer state we
    // need to answer adapter queries.
    // Note: this map is modified in multithreaded code paths and must be
    // locked.
    typedef std::unordered_map<SdfPath, _InstancerData, SdfPath::Hash> 
        _InstancerDataMap;
    _InstancerDataMap _instancerData;

    // Map from USD instance prim paths to the cache path of the hydra instancer
    // they are assigned to (which will typically be the path to the first
    // instance of this instance group we run across).
    // XXX: consider to move this forwarding map into HdRenderIndex.
    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash>
        _InstanceToInstancerMap;
    _InstanceToInstancerMap _instanceToInstancerMap;

    // Hd and UsdImaging think of instancing in terms of an 'instancer' that
    // specifies a list of 'prototype' prims that are shared per instance.
    //
    // For Usd scenegraph instancing, a prototype prim and its descendents
    // roughly correspond to the instancer and prototype prims. However,
    // Hd requires a different instancer and rprims for different combinations
    // of inherited attributes (material binding, draw mode, etc).
    // This means we cannot use the Usd prototype prim as the instancer, because
    // we can't represent this in the case where multiple Usd instances share
    // the same prototype but have different bindings.
    //
    // Instead, we use the first instance of a prototype with a given set of
    // inherited attributes as our instancer. For example, if /A and /B are
    // both instances of /__Prototype_1 but /A and /B have different material
    // bindings authored on them, both /A and /B will be instancers,
    // with their own set of rprims and instance indices.
    //
    // The below is a multimap from prototype path to the cache path of the
    // hydra instancer. The data for the instancer is located in the
    // _InstancerDataMap.
    typedef TfHashMultiMap<SdfPath, SdfPath, SdfPath::Hash>
        _PrototypeToInstancerMap;
    _PrototypeToInstancerMap _prototypeToInstancerMap;

    // Map from instance cache path to their instancer path.
    // Note: this is for reducing proto prim lookup in _GetProtoPrim method.
    typedef std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>
        _ProtoPrimToInstancerMap;
    _ProtoPrimToInstancerMap _protoPrimToInstancerMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_INSTANCE_ADAPTER_H
