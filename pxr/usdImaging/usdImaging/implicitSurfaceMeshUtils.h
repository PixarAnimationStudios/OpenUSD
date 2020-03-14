//
// Copyright 2019 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_IMPLICIT_SURFACE_MESH_UTILS_H
#define PXR_USD_IMAGING_USD_IMAGING_IMPLICIT_SURFACE_MESH_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class PxOsdMeshTopology;
class TfToken;

// Sphere

/// Return a topology object for the canonical "unit sphere" mesh.  This is
/// constructed once and is identical for all spheres.  The indices refer to the
/// points array returned by UsdImagingGetUnitSphereMeshPoints().
USDIMAGING_API
const PxOsdMeshTopology&
UsdImagingGetUnitSphereMeshTopology();

/// Return an array of points for the canonical "unit sphere" mesh.  This is a
/// mesh describing a sphere that fits in a unit-sized bounding box, centered on
/// the origin.  Note that this means the diameter, not radius, is one unit!
///
/// These points are constructed once and are identical for all spheres, with
/// topology provided by UsdImagingGetUnitSphereMeshTopology().  To represent
/// spheres of a different size, use with the transform produced by the
/// companion function UsdImagingGenerateSphereOrCubeTransform().
USDIMAGING_API
const VtVec3fArray&
UsdImagingGetUnitSphereMeshPoints();

// Cube

/// Return a topology object for the canonical "unit cube" mesh.  This is
/// constructed once and is identical for all cubes.  The indices refer to the
/// points array returned by UsdImagingGetUnitCubeMeshPoints().
USDIMAGING_API
const PxOsdMeshTopology&
UsdImagingGetUnitCubeMeshTopology();

/// Return an array of points for the canonical "unit cube" mesh.  This is a
/// mesh describing a cube with unit-length edges, centered on the origin.
///
/// These points are constructed once and are identical for all cubes, with
/// topology provided by UsdImagingGetUnitCubeMeshTopology().  To represent
/// cubes of a different size, use with the transform produced by the
/// companion function UsdImagingGenerateSphereOrCubeTransform().
USDIMAGING_API
const VtVec3fArray&
UsdImagingGetUnitCubeMeshPoints();

// Cone

/// Return a topology object for the canonical "unit cone" mesh.  This is
/// constructed once and is identical for all cones.  The indices refer to the
/// points array returned by UsdImagingGetUnitConeMeshPoints().
USDIMAGING_API
const PxOsdMeshTopology&
UsdImagingGetUnitConeMeshTopology();

/// Return an array of points for the canonical "unit cone" mesh.  This is a
/// mesh describing a cone that fits in a unit-sized bounding box, centered on
/// the origin.  Note that this means the diameter, not radius, is one unit!
/// The circular-disk face of the cone lies in the XY plane, with the large end
/// on the negative-Z side and the cone point on the positive-Z side.
///
/// These points are constructed once and are identical for all cones, with
/// topology provided by UsdImagingGetUnitConeMeshTopology().  To represent
/// cones of a different radius, height, or axis orientation, use with the
/// transform produced by the companion function
/// UsdImagingGenerateConeOrCylinderTransform().
USDIMAGING_API
const VtVec3fArray&
UsdImagingGetUnitConeMeshPoints();

// Cylinder

/// Return a topology object for the canonical "unit cylinder" mesh.  This is
/// constructed once and is identical for all cylinders.  The indices refer to
/// the points array returned by UsdImagingGetUnitCylinderMeshPoints().
USDIMAGING_API
const PxOsdMeshTopology&
UsdImagingGetUnitCylinderMeshTopology();

/// Return an array of points for the canonical "unit cylinder" mesh.  This is a
/// mesh describing a cylinder that fits in a unit-sized bounding box, centered
/// on the origin.  Note that this means the diameter, not radius, is one unit!
/// The circular-disk face of the cone lies in the XY plane, with the height of
/// the cylinder aligned along the Z axis.
///
/// These points are constructed once and are identical for all cylinders, with
/// topology provided by UsdImagingGetUnitCylinderMeshTopology().  To represent
/// cylinders of a different radius, height, or axis orientation, use with the
/// transform produced by the companion function
/// UsdImagingGenerateConeOrCylinderTransform().
USDIMAGING_API
const VtVec3fArray&
UsdImagingGetUnitCylinderMeshPoints();

// Capsule

/// Return a topology object for use with all generated "capsule" meshes.  This
/// is constructed once and is identical for all capsules.  The indices refer to
/// the points array returned by UsdImagingGenerateCapsuleMeshPoints().
USDIMAGING_API
const PxOsdMeshTopology&
UsdImagingGetCapsuleMeshTopology();

/// Generate an array of points describing a "capsule": a cylinder with
/// hemispherical caps on each end.  The given height is the length of the
/// cylinder portion exclusively, and the given radius applies to both cylinder
/// and hemisphere components.  The cylinder will be oriented along the given
/// axis.
///
/// Unlike the other primitives in this library, it's not possible to use a
/// constant set of points and effect radius and height adjustments by varying
/// the transform matrix.  This function will generate the points with the
/// requested parameters, and no additional transform is required.  The returned
/// points are for use with the topology provided by
/// UsdImagingGetCapsuleMeshTopology().
USDIMAGING_API
VtVec3fArray
UsdImagingGenerateCapsuleMeshPoints(
    const double height,
    const double radius,
    const TfToken& axis);

// Transforms

/// Generate a transform to inflate the "unit sphere" or "unit cube" mesh to the
/// specified size.  This is a uniform scale matrix.  Note that the parameter is
/// the net scale, so when using with the "unit sphere" mesh be sure to pass the
/// desired diameter (not radius)!
USDIMAGING_API
GfMatrix4d
UsdImagingGenerateSphereOrCubeTransform(
    const double size);

/// Generate a transform for use with the "unit cone" or "unit cylinder" meshes,
/// which transforms the mesh to have the specified height and radius, while
/// aligned along the specified axis.  This is a combination of rotation and
/// nonuniform scales.  This function is for use only with the cone and cylinder
/// primitives, hence its parameterization by radius; the net scale for the
/// radial axes will be twice the given value.
USDIMAGING_API
GfMatrix4d
UsdImagingGenerateConeOrCylinderTransform(
    const double height,
    const double radius,
    const TfToken& axis);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_IMPLICIT_SURFACE_MESH_UTILS_H
