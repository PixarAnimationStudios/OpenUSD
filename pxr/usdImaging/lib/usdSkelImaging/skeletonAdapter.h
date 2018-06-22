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

#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"

#include <boost/unordered_map.hpp>


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
    void MarkTransformDirty(const UsdPrim& prim,
                            const SdfPath& cachePath,
                            UsdImagingIndexProxy* index) override;

    USDSKELIMAGING_API
    void MarkVisibilityDirty(const UsdPrim& prim,
                             const SdfPath& cachePath,
                             UsdImagingIndexProxy* index) override;

protected:

    void _RemovePrim(const SdfPath& cachePath,
                     UsdImagingIndexProxy* index) override;

    /// Reads the extent from the given prim. If the extent is not authored,
    /// an empty GfRange3d is returned, the extent will not be computed.
    GfRange3d _GetExtent(const UsdPrim& prim, UsdTimeCode time) const;

    /// Returns the UsdGeomImagable "purpose" for this prim, including any
    /// inherited purpose. Inherited values are strongest.
    TfToken _GetPurpose(const UsdPrim & prim, UsdTimeCode time) const;

    /// Returns a value holding color, opacity for \p prim,
    /// taking into account explicitly authored color on the prim.
    GfVec4f _GetColorAndOpacity(const UsdPrim& prim, UsdTimeCode time) const;

private:

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
    };

    _SkelData*  _GetSkelData(const SdfPath& cachePath) const;

    using _SkelDataMap =
        boost::unordered_map<SdfPath,std::shared_ptr<_SkelData> >;

private:
    UsdSkelCache _skelCache;
    _SkelDataMap _skelDataCache;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKELIMAGING_SKELETONADAPTER
