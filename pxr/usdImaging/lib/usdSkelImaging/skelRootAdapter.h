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
#ifndef USDSKELIMAGING_SKELROOTADAPTER_H
#define USDSKELIMAGING_SKELROOTADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"
#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"

#include <boost/unordered_map.hpp>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingSkelRootAdapter
///
/// Support for drawing skeletal data beneath a UsdSkelRoot.
///
class UsdSkelImagingSkelRootAdapter : public UsdImagingPrimAdapter {
public:
    using BaseAdapter = UsdImagingPrimAdapter;

    UsdSkelImagingSkelRootAdapter()
        : BaseAdapter()
    {}

    USDSKELIMAGING_API
    virtual ~UsdSkelImagingSkelRootAdapter();

    USDSKELIMAGING_API
    virtual SdfPath
    Populate(const UsdPrim& prim,
             UsdImagingIndexProxy* index,
             const UsdImagingInstancerContext*
                 instancerContext=nullptr) override;

    USDSKELIMAGING_API
    virtual bool IsSupported(const UsdImagingIndexProxy* index) const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDSKELIMAGING_API
    virtual void TrackVariability(const UsdPrim& prim,
                                  const SdfPath& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  const UsdImagingInstancerContext* 
                                     instancerContext = nullptr) const override;

    /// Thread Safe.
    USDSKELIMAGING_API
    virtual void UpdateForTime(const UsdPrim& prim,
                               const SdfPath& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               const UsdImagingInstancerContext* 
                                   instancerContext = nullptr) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing
    // ---------------------------------------------------------------------- //

    USDSKELIMAGING_API
    virtual HdDirtyBits ProcessPropertyChange(const UsdPrim& prim,
                                              const SdfPath& cachePath,
                                              const TfToken& propertyName) override;

    USDSKELIMAGING_API
    virtual void MarkDirty(const UsdPrim& prim,
                           const SdfPath& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) override;


protected:

    virtual void _RemovePrim(const SdfPath& cachePath,
                             UsdImagingIndexProxy* index);

    GfRange3d _GetExtent(const UsdSkelRoot& skelRoot, UsdTimeCode time) const;

protected:

    /// Data for a skel instance.
    struct _SkelInstance {

        UsdSkelRoot          skelRoot;
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

    _SkelInstance*  _GetSkelInstance(const SdfPath& cachePath) const;

    using _SkelInstanceMap =
        boost::unordered_map<SdfPath,std::shared_ptr<_SkelInstance> >;

private:
    UsdSkelCache     _skelCache;
    _SkelInstanceMap _instanceCache;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKELIMAGING_SKELROOTADAPTER
