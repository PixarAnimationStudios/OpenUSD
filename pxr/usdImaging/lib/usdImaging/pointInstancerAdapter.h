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
#ifndef USDIMAGING_POINT_INSTANCER_ADAPTER_H
#define USDIMAGING_POINT_INSTANCER_ADAPTER_H

#include "pxr/usdImaging/usdImaging/version.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"

#include <mutex>
#include <boost/unordered_map.hpp> 
#include <boost/shared_ptr.hpp> 

/// Delegate support for UsdGeomMesh.
///
class UsdImagingPointInstancerAdapter : public UsdImagingPrimAdapter {
public:
    typedef UsdImagingPrimAdapter BaseAdapter;

    UsdImagingPointInstancerAdapter()
        : BaseAdapter()
        { }
    virtual ~UsdImagingPointInstancerAdapter();

    virtual SdfPath Populate(
        UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const* instancerContext = NULL);

    virtual bool ShouldCullChildren(UsdPrim const& prim);

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
    
    virtual void TrackVariabilityPrep(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      int requestedBits,
                                      UsdImagingInstancerContext const* 
                                          instancerContext = NULL);

    /// Thread Safe.
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  int requestedBits,
                                  int* dirtyBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL);

    virtual void UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath, 
                                   UsdTimeCode time,
                                   int requestedBits,
                                   UsdImagingInstancerContext const* 
                                       instancerContext = NULL);

    /// Thread Safe.
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               int requestedBits,
                               int* resultBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL);

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    virtual int ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName);

    virtual void ProcessPrimResync(SdfPath const& usdPath,
                                   UsdImagingIndexProxy* index);

    virtual void ProcessPrimRemoval(SdfPath const& usdPath,
                                    UsdImagingIndexProxy* index);

    // ---------------------------------------------------------------------- //
    /// \name Instancing
    // ---------------------------------------------------------------------- //

    virtual SdfPath GetPathForInstanceIndex(SdfPath const &path,
                                            int instanceIndex,
                                            int *instanceCountForThisLevel,
                                            int *absoluteInstanceIndex,
                                            SdfPath *rprimPath=NULL,
                                            SdfPathVector *instanceContext=NULL);

    // ---------------------------------------------------------------------- //
    /// \name Selection
    // ---------------------------------------------------------------------- //
    virtual bool PopulateSelection(SdfPath const &path,
                                   VtIntArray const &instanceIndices,
                                   HdxSelectionSharedPtr const &result);

    // ---------------------------------------------------------------------- //
    /// \name Utilities 
    // ---------------------------------------------------------------------- //

    virtual SdfPathVector GetDependPaths(SdfPath const &path) const;

private:

    struct _ProtoRprim;
    struct _ProtoGroup;
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
    void _ProcessPrimRemoval(SdfPath const& usdPath,
                             UsdImagingIndexProxy* index,
                             SdfPathVector* instancersToReload);

    // Removes all instancer data, both locally and from the render index.
    void _UnloadInstancer(SdfPath const& instancerPath,
                          UsdImagingIndexProxy* index);

    // Updates per-frame data in the instancer map. This is primarily used
    // during update to send new instance indices out to Hydra.
    void _UpdateInstanceMap(SdfPath const &instancerPath, UsdTimeCode time);

    // Returns true if the instancer is visible, taking into account all
    // parent instancers visibilities.
    bool _GetInstancerVisible(SdfPath const &instancerPath, UsdTimeCode time);

    // Update the dirty bits per-instancer. This is only executed once per
    // instancer, this method uses the instancer mutex to avoid redundant work.
    //
    // Returns the instancer's dirty bits.
    int _UpdateDirtyBits(UsdPrim const& instancerPrim);

    // Gets the associated _ProtoRprim for the given instancer and cache path.
    _ProtoRprim const& _GetProtoRprim(SdfPath const& instancerPath, 
                                      SdfPath const& cachePath) const;

    // Takes the transform in the value cache (this must exist before calling
    // this method) and applies a corrective transform to 1) remove any
    // transforms above the model root (root proto path) and 2) apply the 
    // instancer transform.
    void _CorrectTransform(UsdPrim const& instancer,
                           UsdPrim const& proto,
                           SdfPath const& cachePath,
                           SdfPathVector const& protoPathChain,
                           UsdTimeCode time);

    // Similar to CorrectTransform, requires a visibility value exist in the
    // ValueCache, removes any visibility opinions above the model root (proto
    // root path) and applies the instancer visibility.
    void _ComputeProtoVisibility(UsdPrim const& protoRoot,
                                 UsdPrim const& protoGprim,
                                 UsdTimeCode time,
                                 bool* vis);

    // Computes the Purpose for the prototype, stopping at the proto root.
    void _ComputeProtoPurpose(UsdPrim const& protoRoot,
                              UsdPrim const& protoGprim,
                              TfToken* purpose);

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

    // A _Prototype represents a complete set of rprims for a given prototype
    // path declared on the instancer;
    struct _Prototype {
        // The enabled flag is used to disable all rprims associated with a
        // protoGroup; it marks them as invisible and disables data updates.
        bool enabled;
        // When requiresUpdate is false and enabled is true, it indicates that
        // the rprim was drawn for a previous frame with the newly desired time;
        // this is a cache hit and Usd-data fetch is skipped.
        bool requiresUpdate;
        // A vector of prototype indices that also index into the primvar data.
        // All elements in this array can be dispatched as a single hardware
        // draw call (though this is a detail of the renderer implementation).
        VtIntArray indices;
        // The root prototype path, typically the model root, which is not a
        // gprim and not actually a prototype from Hydra's perspective.
        SdfPath protoRootPath;
    };
    typedef boost::shared_ptr<_Prototype> _PrototypeSharedPtr;

    // A proto rprim represents a single rprim under a prototype root declared
    // on the instancer. For example, a character may be targeted by the
    // prototypes relationship, which will have many meshes, each mesh is
    // represented as a proto rprim.
    struct _ProtoRprim {
        _ProtoRprim() : variabilityBits(0), visible(true), initialized(false) {}
        // Each rprim will become a prototype "child" under the instancer. This
        // path is the path to the gprim on the Usd Stage (the path to a single
        // mesh, for example).
        SdfPath path;           
        // A list of paths we had to hop across when resolving native USD 
        // instances. TODO: We should probably remove the path member in favor
        // of just using this vector.
        SdfPathVector paths;
        // The prim adapter for the actual prototype gprim.
        UsdImagingPrimAdapterSharedPtr adapter;
        // The prototype group that this rprim belongs to.
        // Over time, as instances are animated, multiple copies of the
        // prototype may be required to, for example, draw two different frames
        // of animation. This ID maps the rprim its associated instance data
        // over time.
        _PrototypeSharedPtr prototype;
        // Tracks the variability of the underlying adapter to avoid
        // redundantly reading data. This value is stored as
        // HdChangeTracker::DirtyBits bit flags.
        int variabilityBits;
        // When variabilityBits does not include HdChangeTracker::DirtyVisibility
        // the visible field is the unvarying value for visibility.
        bool visible;
        // We need to ensure that all prims have their topology (e.g.)
        // initialized because even if they are invisible, they need to be
        // scheduled into a command buffer.
        bool initialized;
    };

    // Indexed by cachePath (each rprim has one entry)
    typedef boost::unordered_map<SdfPath,
                                 _ProtoRprim, SdfPath::Hash> _ProtoRPrimMap;

    // Map from usdPath -> cachePath(s), useful for change processing, when all
    // we get is a usdPath.
    typedef boost::unordered_map<SdfPath, 
                                 SdfPathVector, SdfPath::Hash> _UsdToCacheMap;

    // All data asscoiated with a given Instancer prim. PrimMap could
    // technically be split out to avoid two lookups, however it seems cleaner
    // to keep everything bundled up under the instancer path.
    struct _InstancerData {
        SdfPath parentInstancerPath;
        _ProtoRPrimMap protoRprimMap;
        _UsdToCacheMap usdToCacheMap;
        std::vector<_PrototypeSharedPtr> prototypes;
        std::mutex mutex;
        int dirtyBits;
        bool visible;
    };

    // A map of instancer data, one entry per instancer prim that has been
    // populated.
    typedef boost::unordered_map<SdfPath /*instancerPath*/, 
                                 _InstancerData, 
                                 SdfPath::Hash> _InstancerDataMap;
    _InstancerDataMap _instancerData;
};


#endif // USDIMAGING_POINT_INSTANCER_ADAPTER_H
