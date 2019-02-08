//
// Copyright 2018 Pixar
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
#ifndef USDSKELIMAGING_SKELETONADAPTER_H
#define USDSKELIMAGING_SKELETONADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/usd/usdSkel/binding.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"

#include <boost/unordered_map.hpp>
#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingSkeletonAdapter
///
/// Support for drawing bones of a UsdSkelSkeleton.  
///
class UsdSkelImagingSkeletonAdapter : public UsdImagingPrimAdapter {
public:
    using BaseAdapter = UsdImagingPrimAdapter;

    UsdSkelImagingSkeletonAdapter()
        : BaseAdapter()
    {}

    USDSKELIMAGING_API
    virtual ~UsdSkelImagingSkeletonAdapter();

    USDSKELIMAGING_API
    SdfPath
    Populate(const UsdPrim& prim,
             UsdImagingIndexProxy* index,
             const UsdImagingInstancerContext*
                 instancerContext=nullptr) override;

    USDSKELIMAGING_API
    bool IsSupported(const UsdImagingIndexProxy* index) const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDSKELIMAGING_API
    void TrackVariability(const UsdPrim& prim,
                          const SdfPath& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          const UsdImagingInstancerContext* 
                             instancerContext = nullptr) const override;

    /// Thread Safe.
    USDSKELIMAGING_API
    void UpdateForTime(const UsdPrim& prim,
                       const SdfPath& cachePath, 
                       UsdTimeCode time,
                       HdDirtyBits requestedBits,
                       const UsdImagingInstancerContext*
                           instancerContext=nullptr) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing
    // ---------------------------------------------------------------------- //

    USDSKELIMAGING_API
    HdDirtyBits ProcessPropertyChange(const UsdPrim& prim,
                                      const SdfPath& cachePath,
                                      const TfToken& propertyName) override;

    USDSKELIMAGING_API
    void MarkDirty(const UsdPrim& prim,
                   const SdfPath& cachePath,
                   HdDirtyBits dirty,
                   UsdImagingIndexProxy* index) override;

    USDSKELIMAGING_API
    virtual void MarkRefineLevelDirty(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingIndexProxy* index) override;

    USDSKELIMAGING_API
    virtual void MarkReprDirty(UsdPrim const& prim,
                               SdfPath const& cachePath,
                               UsdImagingIndexProxy* index) override;

    USDSKELIMAGING_API
    virtual void MarkCullStyleDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDSKELIMAGING_API
    void MarkTransformDirty(const UsdPrim& prim,
                            const SdfPath& cachePath,
                            UsdImagingIndexProxy* index) override;

    USDSKELIMAGING_API
    void MarkVisibilityDirty(const UsdPrim& prim,
                             const SdfPath& cachePath,
                             UsdImagingIndexProxy* index) override;

    USDSKELIMAGING_API
    void MarkMaterialDirty(const UsdPrim& prim,
                           const SdfPath& cachePath,
                           UsdImagingIndexProxy* index) override;
                           
    // ---------------------------------------------------------------------- //
    /// \name Computation API
    // ---------------------------------------------------------------------- //
    USDSKELIMAGING_API
    void InvokeComputation(SdfPath const& computationPath,
                           HdExtComputationContext* context) override;

    // ---------------------------------------------------------------------- //
    /// \name Non-virtual public API
    // ---------------------------------------------------------------------- //

    USDSKELIMAGING_API
    void RegisterSkelBinding(UsdSkelBinding const& binding);

protected:
    // ---------------------------------------------------------------------- //
    /// \name Utility methods
    // ---------------------------------------------------------------------- //
    void _RemovePrim(const SdfPath& cachePath,
                     UsdImagingIndexProxy* index) override;

private:
    // ---------------------------------------------------------------------- //
    /// Handlers for the Bone Mesh
    // ---------------------------------------------------------------------- //
    bool _IsCallbackForSkeleton(const UsdPrim& prim) const;
    
    /// Note: Methods below have been lifted from GprimAdapter.
    /// Reads the extent from the given prim. If the extent is not authored,
    /// an empty GfRange3d is returned, the extent will not be computed.
    GfRange3d _GetExtent(const UsdPrim& prim, UsdTimeCode time) const;

    /// Returns a value holding color for \p prim,
    /// taking into account explicitly authored color on the prim.
    GfVec3f _GetSkeletonDisplayColor(const UsdPrim& prim,
                                     UsdTimeCode time) const;

    /// Returns a value holding opacity for \p prim,
    /// taking into account explicitly authored opacity on the prim.
    float _GetSkeletonDisplayOpacity(const UsdPrim& prim,
                                     UsdTimeCode time) const;
    
     void _TrackBoneMeshVariability(
            const UsdPrim& prim,
            const SdfPath& cachePath,
            HdDirtyBits* timeVaryingBits,
            const UsdImagingInstancerContext* 
                instancerContext = nullptr) const;

    void _UpdateBoneMeshForTime(
            const UsdPrim& prim,
            const SdfPath& cachePath, 
            UsdTimeCode time,
            HdDirtyBits requestedBits,
            const UsdImagingInstancerContext* instancerContext=nullptr) const;

    // ---------------------------------------------------------------------- //
    /// Common utitily methods for skinning computations & skinned prims
    // ---------------------------------------------------------------------- //
    bool _IsAffectedByTimeVaryingJointXforms(const SdfPath& skinnedPrimPath)
        const;

    // ---------------------------------------------------------------------- //
    /// Handlers for the skinning computations
    // ---------------------------------------------------------------------- //
    bool _IsSkinningComputationPath(const SdfPath& cachePath) const;
    
    bool
    _IsSkinningInputAggregatorComputationPath(const SdfPath& cachePath)const;

    void _TrackSkinningComputationVariability(
            const UsdPrim& skinnedPrim,
            const SdfPath& computationPath,
            HdDirtyBits* timeVaryingBits,
            const UsdImagingInstancerContext* 
                instancerContext = nullptr) const;
    
    VtVec3fArray _GetSkinnedPrimPoints(const UsdPrim& skinnedPrim,
                                       const SdfPath& skinnedPrimCachePath,
                                       UsdTimeCode time) const;

    void _UpdateSkinningComputationForTime(
            const UsdPrim& skinnedPrim,
            const SdfPath& computationPath, 
            UsdTimeCode time,
            HdDirtyBits requestedBits,
            const UsdImagingInstancerContext* instancerContext=nullptr) const;

    void _UpdateSkinningInputAggregatorComputationForTime(
            const UsdPrim& skinnedPrim,
            const SdfPath& computationPath, 
            UsdTimeCode time,
            HdDirtyBits requestedBits,
            const UsdImagingInstancerContext* instancerContext=nullptr) const;
    
    SdfPath _GetSkinningComputationPath(const SdfPath& skinnedPrimPath) const;

    SdfPath _GetSkinningInputAggregatorComputationPath(
        const SdfPath& skinnedPrimPath) const;

    // Static helper methods
    static
    std::string _LoadSkinningComputeKernel();

    static
    const std::string& _GetSkinningComputeKernel();
 
    // ---------------------------------------------------------------------- //
    /// Handlers for the skinned prim
    // ---------------------------------------------------------------------- //
    bool _IsSkinnedPrimPath(const SdfPath& cachePath) const;

    void _TrackSkinnedPrimVariability(
            const UsdPrim& prim,
            const SdfPath& cachePath,
            HdDirtyBits* timeVaryingBits,
            const UsdImagingInstancerContext* 
                instancerContext = nullptr) const;
    
    void _UpdateSkinnedPrimForTime(
            const UsdPrim& prim,
            const SdfPath& cachePath, 
            UsdTimeCode time,
            HdDirtyBits requestedBits,
            const UsdImagingInstancerContext* instancerContext=nullptr) const;


    // ---------------------------------------------------------------------- //
    /// Populated skeleton state
    // ---------------------------------------------------------------------- //
    /// Data for a skel instance.
    struct _SkelData {

        UsdSkelSkeletonQuery skelQuery;

        /// Compute bone mesh topology, and intiailize
        /// other rest-state data for imaging bones.
        HdMeshTopology ComputeTopologyAndRestState();

        /// Compute animated  bone mesh points.
        VtVec3fArray ComputePoints(UsdTimeCode time) const; 

    private:
        // Cache of a mesh for a skeleton (at rest)
        // TODO: Dedupe this infromation across UsdSkelSkeleton instances.
        VtVec3fArray    _boneMeshPoints;
        VtIntArray      _boneMeshJointIndices;
        size_t          _numJoints;
    };

    _SkelData*  _GetSkelData(const SdfPath& cachePath) const;
    
    UsdSkelCache _skelCache;
    using _SkelDataMap =
        boost::unordered_map<SdfPath,std::shared_ptr<_SkelData> >;
    _SkelDataMap _skelDataCache;

    // ---------------------------------------------------------------------- //
    /// Skeleton -> Skinned Prim(s) state
    /// (Populated via UsdSkelImagingSkelRootAdapter::Populate)
    // ---------------------------------------------------------------------- //
    using _SkelBindingMap =
        std::unordered_map<SdfPath, UsdSkelBinding, SdfPath::Hash>;
    _SkelBindingMap _skelBindingMap;

    // ---------------------------------------------------------------------- //
    /// Skinned Prim -> Skeleton
    /// (Updated locally)
    // ---------------------------------------------------------------------- //
    using _SkinnedPrimToSkelMap =
        std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>;
    _SkinnedPrimToSkelMap _skinnedPrimToSkelMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKELIMAGING_SKELETONADAPTER
 