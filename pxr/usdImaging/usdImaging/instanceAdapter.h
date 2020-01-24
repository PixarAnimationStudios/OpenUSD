//
// Copyright 2016 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_INSTANCE_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_INSTANCE_ADAPTER_H

/// \file usdImaging/instanceAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/base/tf/hashmap.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingInstanceAdapter
///
/// Delegate support for instanced prims.
///
class UsdImagingInstanceAdapter : public UsdImagingPrimAdapter
{
public:
    typedef UsdImagingPrimAdapter BaseAdapter;

    UsdImagingInstanceAdapter();
    virtual ~UsdImagingInstanceAdapter();

    virtual SdfPath Populate(UsdPrim const& prim, UsdImagingIndexProxy* index,
            UsdImagingInstancerContext const* instancerContext = NULL) override;

    virtual bool ShouldCullChildren() const override;

    virtual bool IsInstancerAdapter() const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
    
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL) const override;

    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    virtual HdDirtyBits ProcessPropertyChange(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              TfToken const& propertyName) override;

    virtual void ProcessPrimResync(SdfPath const& cachePath,
                                   UsdImagingIndexProxy* index) override;

    virtual void ProcessPrimRemoval(SdfPath const& cachePath,
                                   UsdImagingIndexProxy* index) override;


    virtual void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) override;

    // As this adapter hijacks the adapter for the child prim
    // We need to forward these messages on, in case the child
    // adapter needs them
    virtual void MarkRefineLevelDirty(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingIndexProxy* index) override;

    virtual void MarkReprDirty(UsdPrim const& prim,
                               SdfPath const& cachePath,
                               UsdImagingIndexProxy* index) override;

    virtual void MarkCullStyleDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    virtual void MarkRenderTagDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    virtual void MarkTransformDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    virtual void MarkVisibilityDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index) override;



    // ---------------------------------------------------------------------- //
    /// \name Instancing
    // ---------------------------------------------------------------------- //

    virtual SdfPath GetPathForInstanceIndex(SdfPath const &protoCachePath,
                                            int protoIndex,
                                            int *instanceCountForThisLevel,
                                            int *instancerIndex,
                                            SdfPath *masterCachePath = NULL,
                                            SdfPathVector *
                                                instanceContext = NULL) override;

    virtual std::vector<VtArray<TfToken>>
    GetInstanceCategories(UsdPrim const& prim) override;

    virtual size_t
    SampleInstancerTransform(UsdPrim const& instancerPrim,
                             SdfPath const& instancerPath,
                             UsdTimeCode time,
                             size_t maxSampleCount,
                             float *sampleTimes,
                             GfMatrix4d *sampleValues) override;

    virtual size_t
    SampleTransform(UsdPrim const& prim, 
                    SdfPath const& cachePath,
                    UsdTimeCode time, 
                    size_t maxNumSamples, 
                    float *sampleTimes,
                    GfMatrix4d *sampleValues) override;

    virtual size_t
    SamplePrimvar(UsdPrim const& usdPrim,
                  SdfPath const& cachePath,
                  TfToken const& key,
                  UsdTimeCode time,
                  size_t maxNumSamples, 
                  float *sampleTimes,
                  VtValue *sampleValues) override;

    virtual PxOsdSubdivTags GetSubdivTags(UsdPrim const& usdPrim,
                                          SdfPath const& cachePath,
                                          UsdTimeCode time) const override;

    // ---------------------------------------------------------------------- //
    /// \name Nested instancing support
    // ---------------------------------------------------------------------- //

    virtual GfMatrix4d GetRelativeInstancerTransform(
        SdfPath const &parentInstancerPath,
        SdfPath const &instancerPath,
        UsdTimeCode time) const override;

    // ---------------------------------------------------------------------- //
    /// \name Selection
    // ---------------------------------------------------------------------- //
    virtual bool PopulateSelection( 
                                HdSelection::HighlightMode const& highlightMode,
                                SdfPath const &cachePath,
                                UsdPrim const &usdPrim,
                                VtIntArray const &instanceIndices,
                                HdSelectionSharedPtr const &result) override;

    // ---------------------------------------------------------------------- //
    /// \name Volume field information
    // ---------------------------------------------------------------------- //

    virtual HdVolumeFieldDescriptorVector
    GetVolumeFieldDescriptors(UsdPrim const& usdPrim, SdfPath const &id,
                              UsdTimeCode time) const override;

protected:
    virtual void _RemovePrim(SdfPath const& cachePath,
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

    // Inserts prototype prims for the given \p usdPrim into the \p index.
    // Any inserted gprims will be inserted under a special path combining
    // \p instancerPath and \p protoName.
    SdfPath 
    _InsertProtoPrim(UsdPrimRange::iterator *iter,
                     const TfToken& protoName,
                     SdfPath materialId,
                     TfToken drawMode,
                     SdfPath instancerPath,
                     UsdImagingPrimAdapterSharedPtr const& primAdapter,
                     UsdImagingPrimAdapterSharedPtr const& instancerAdapter,
                     UsdImagingIndexProxy* index,
                     bool *isLeafInstancer);

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

    struct _GetPathForInstanceIndexFn;
    struct _PopulateInstanceSelectionFn;

    // Helper functions for dealing with "actual" instances to be drawn.
    //
    // Suppose we have:
    //    /Root
    //        Instance_A (master: /__Master_1)
    //        Instance_B (master: /__Master_1)
    //    /__Master_1
    //        AnotherInstance_A (master: /__Master_2)
    //    /__Master_2
    //
    // /__Master_2 has only one associated instance in the Usd scenegraph: 
    // /__Master_1/AnotherInstance_A. However, imaging actually needs to draw 
    // two instances of /__Master_2, because AnotherInstance_A is a nested 
    // instance beneath /__Master_1, and there are two instances of /__Master_1.
    //
    // Each instance to be drawn is addressed by the chain of instances
    // that caused it to be drawn. In the above example, the two instances 
    // of /__Master_2 to be drawn are:
    //
    //  [ /Root/Instance_A, /__Master_1/AnotherInstance_A ],
    //  [ /Root/Instance_B, /__Master_1/AnotherInstance_A ]
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

    // Indexed by cachePath (each prim has one entry)
    typedef TfHashMap<SdfPath, _ProtoPrim, SdfPath::Hash> _PrimMap;

    // All data associated with a given instancer prim. PrimMap could
    // technically be split out to avoid two lookups, however it seems cleaner
    // to keep everything bundled up under the instancer path.
    struct _InstancerData {
        _InstancerData() : numInstancesToDraw(0) { }

        // The master prim path associated with this instancer.
        SdfPath masterPath;

        // The USD material path associated with this instancer.
        SdfPath materialUsdPath;

        // The drawmode associated with this instancer.
        TfToken drawMode;

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
        std::vector<SdfPath> instancePaths;

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

        // Nested native instances.
        SdfPathVector nestedInstances;
    };

    // Map from instancer cache path to instancer data.
    // Note: this map is modified in multithreaded code paths and must be
    // locked.
    typedef std::unordered_map<SdfPath, _InstancerData, SdfPath::Hash> 
        _InstancerDataMap;
    _InstancerDataMap _instancerData;

    // Map from instance to instancer.
    // XXX: consider to move this forwarding map into HdRenderIndex.
    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash>
        _InstanceToInstancerMap;
    _InstanceToInstancerMap _instanceToInstancerMap;

    // Hd and UsdImaging think of instancing in terms of an 'instancer' that
    // specifies a list of 'prototype' prims that are shared per instance.
    //
    // For Usd scenegraph instancing, a master prim and its descendents
    // roughly correspond to the instancer and prototype prims. However,
    // Hd requires a different instancer and rprims for different combinations
    // of inherited attributes (material binding, draw mode, etc).
    // This means we cannot use the Usd master prim as the instancer, because
    // we can't represent this in the case where multiple Usd instances share
    // the same master but have different bindings.
    //
    // Instead, we use the first instance of a master with a given set of
    // inherited attributes as our instancer. For example, if /A and /B are
    // both instances of /__Master_1 but /A and /B have different material
    // bindings authored on them, both /A and /B will be instancers,
    // with their own set of rprims and instance indices.
    //
    // The below is a multimap from master path to instancer path. The data
    // for the instancer is located in the _InstancerDataMap above.
    typedef TfHashMultiMap<SdfPath, SdfPath, SdfPath::Hash>
        _MasterToInstancerMap;
    _MasterToInstancerMap _masterToInstancerMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_INSTANCE_ADAPTER_H
