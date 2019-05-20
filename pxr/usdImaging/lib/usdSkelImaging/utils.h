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
#ifndef USDSKELIMAGING_UTILS_H
#define USDSKELIMAGING_UTILS_H

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

/// @}


PXR_NAMESPACE_CLOSE_SCOPE


#endif // USDSKELIMAGING_UTILS_H
