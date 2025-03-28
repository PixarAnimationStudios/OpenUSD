//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_UTILS_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_UTILS_H

///  \file usdSkelImaging/utils.h
///
/// Collection of utility methods for imaging skels.
///

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/base/vt/array.h"


PXR_NAMESPACE_OPEN_SCOPE


class HdMeshTopology;
class UsdSkelTopology;


/// \defgroup UsdSkelImaging_BoneUtils Bone Utilities
/// Utilities for constructing bone meshes.
/// @{


/// Compute mesh topology for imaging \p skelTopology.
/// The number of points that the mesh is expected to have are return in
/// \p numPoints.
USDSKELIMAGING_API
bool
UsdSkelImagingComputeBoneTopology(const UsdSkelTopology& skelTopology,
                                  HdMeshTopology* meshTopology,
                                  size_t* numPoints);


/// Compute mesh points for imaging a skeleton, given the
/// \p topology of the skeleton and \p skelXforms.
/// The \p numPoints corresponds to the number of points computed by
/// UsdSkelImagingComputeBoneTopology.
USDSKELIMAGING_API
bool
UsdSkelImagingComputeBonePoints(const UsdSkelTopology& topology,
                                const VtMatrix4dArray& jointSkelXforms,
                                size_t numPoints,
                                VtVec3fArray* points);

/// \overload
USDSKELIMAGING_API
bool
UsdSkelImagingComputeBonePoints(const UsdSkelTopology& topology,
                                const GfMatrix4d* jointSkelXforms,
                                GfVec3f* points, size_t numPoints);


/// Compute joint indices corresponding to each point in a bone mesh.
/// This can be used to animate a bone mesh using normal skinning algos.
/// This does not compute joint weights (they would all be 1s).
/// The \p numPoints corresponds to the number of points computed by
/// UsdSkelImagingComputeBoneTopology.
USDSKELIMAGING_API
bool
UsdSkelImagingComputeBoneJointIndices(const UsdSkelTopology& topology,
                                      VtIntArray* jointIndices,
                                      size_t numPoints);


/// \overload
USDSKELIMAGING_API
bool
UsdSkelImagingComputeBoneJointIndices(const UsdSkelTopology& topology,
                                      int* jointIndices, size_t numPoints);

/// Compute mesh points for imaging a single bone of a skeleton.
///
/// A bone corresponds to a joint that has a parent joint. The input to this
/// function is the transform of that joint and parent joint.
USDSKELIMAGING_API
void
UsdSkelImagingComputePointsForSingleBone(const GfMatrix4d& xform,
                                         const GfMatrix4d& parentXform,
                                         GfVec3f* points);

/// @}


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXR_USD_IMAGING_USD_SKEL_IMAGING_UTILS_H
