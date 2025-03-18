//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_JOINT_INFLUENCES_DATA_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_JOINT_INFLUENCES_DATA_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/usd/usdSkel/animMapper.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrimvarsSchema;

/// Some of the data feeding into the ext computations to skin a prim.
/// They are related to which points are influenced by what skinning
/// transform. These data come from the SkelBindingAPI primvars.
struct UsdSkelImagingJointInfluencesData
{
    // Each vec2f is a pair of a joint index and weight.
    //
    // If hasConstantInfluences is false, then the array contains
    // numInfluencesPerComponent elements for each point of the skinned
    // prim.
    //
    // Otherwise, the array just contains numInfluencesPerComponent
    // and every point is affected the same way.
    VtVec2fArray influences;
    bool hasConstantInfluences;
    int numInfluencesPerComponent;

    // Remapping of joints in skeleton to joints used for skinning.
    UsdSkelAnimMapper jointMapper;
};

/// Compute data from SkelBindingAPI prim data source and bound
/// Skeleton prim data source.
USDSKELIMAGING_API
UsdSkelImagingJointInfluencesData
UsdSkelImagingComputeJointInfluencesData(
    HdContainerDataSourceHandle const &primSource,
    HdContainerDataSourceHandle const &skeletonPrimSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
