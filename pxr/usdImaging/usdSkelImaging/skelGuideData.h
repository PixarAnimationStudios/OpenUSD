//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_SKEL_GUIDE_DATA_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_SKEL_GUIDE_DATA_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usdSkel/animMapper.h"
#include "pxr/usd/usdSkel/topology.h"

#include "pxr/base/vt/array.h"

#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdSkelImagingSkelData;

/// Data to compute the skeleton guide as mesh.
///
/// The data can be given to the below functions to obtain the topology
/// and geometry of the mesh.
///
/// The mesh depicts the posed skeleton by rendering each skeleton joint that has
/// a parent joint as a pyramid-shaped bone.
/// the UsdSkelTopology. Some points of the bone are affected by the underlying
/// joint and other by its parent joint.
///
struct UsdSkelImagingSkelGuideData
{
    /// Path of skeleton prim - used only to emit warnings/errors.
    SdfPath primPath;

    /// Number of joints in UsdSkelTopology to create the data.
    ///
    /// Used only to emit warnings/errors.
    ///
    size_t numJoints;

    /// Indices into joints of UsdSkelTopology - one for each point of the mesh.
    ///
    /// Corresponds to
    /// UsdSkelSkeletonAdapter::_SkelData::_boneMeshJointIndices.
    ///
    VtIntArray boneJointIndices;

    /// The points of the mesh before applying the skinning transforms.
    ///
    /// Corresponds to
    /// UsdSkelSkeletonAdapter::_SkelData::_boneMeshPoints.
    ///
    VtVec3fArray boneMeshPoints;
};

/// Compute data.
USDSKELIMAGING_API
UsdSkelImagingSkelGuideData
UsdSkelImagingComputeSkelGuideData(
    const UsdSkelImagingSkelData &skelData);

/// Compute faceVertexCounts of mesh topology for guide.
USDSKELIMAGING_API
VtIntArray
UsdSkelImagingComputeSkelGuideFaceVertexCounts(
    const UsdSkelImagingSkelGuideData &skelGuideData);

/// Compute faceVertexIndices of mesh topology for guide.
USDSKELIMAGING_API
VtIntArray
UsdSkelImagingComputeSkelGuideFaceVertexIndices(
    const UsdSkelImagingSkelGuideData &skelGuideData);

/// Apply skinning transforms to obtain posed mesh points.
USDSKELIMAGING_API
VtVec3fArray
UsdSkelImagingComputeSkelGuidePoints(
    const UsdSkelImagingSkelGuideData &skelGuideData,
    const VtArray<GfMatrix4f> &skinningTransforms);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
