//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GEOM_UTIL_CYLINDER_MESH_GENERATOR_H
#define PXR_IMAGING_GEOM_UTIL_CYLINDER_MESH_GENERATOR_H

#include "pxr/imaging/geomUtil/api.h"
#include "pxr/imaging/geomUtil/meshGeneratorBase.h"
#include "pxr/imaging/geomUtil/tokens.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;
class PxOsdMeshTopology;

/// This class provides an implementation for generating topology, point
/// positions and surface normals on a cylinder with a given radius and height.
/// The cylinder is made up of circular cross-sections in the XY plane and is
/// centered at the origin. Each cross-section has numRadial segments. The
/// height is aligned with the Z axis, with the base at Z = -h/2.
///
/// An optional transform may be provided to GeneratePoints and GenerateNormals
/// to orient the cylinder as necessary (e.g., whose height is aligned with the
/// Y axis).
///
/// An additional overload of GeneratePoints is provided to specify different
/// radii for the bottom and top discs of the cylinder and a sweep angle for
/// cylinder about the +Z axis.  When the sweep is less than 360 degrees, the
/// generated geometry is not closed.
///
/// \note Setting one radius to 0 in order to get a cone is inefficient and
///       could result in artifacts.  Clients should use
///       GeomUtilConeMeshGenerator instead.
///
/// Usage:
/// \code{.cpp}
///
/// const size_t numRadial = 8;
/// const size_t numPoints =
///     GeomUtilCylinderMeshGenerator::ComputeNumPoints(numRadial);
/// const float radius = 1, height = 2;
///
/// MyPointContainer<GfVec3f> points(numPoints);
///
/// GeomUtilCylinderMeshGenerator::GeneratePoints(
///     points.begin(), numRadial, radius, height);
///
/// const size_t numNormals =
///     GeomUtilCylinderMeshGenerator::ComputeNumNormals(numRadial);
///
/// MyPointContainer<GfVec3f> normals(numNormals);
///
/// GeomUtilCylinderMeshGenerator::GenerateNormals(
///     normals.begin(), numRadial, radius, height);
///
/// \endcode
///
class GeomUtilCylinderMeshGenerator final
    : public GeomUtilMeshGeneratorBase
{
public:
    static constexpr size_t minNumRadial = 3;

    GEOMUTIL_API
    static size_t ComputeNumPoints(
        const size_t numRadial,
        const bool closedSweep = true);

    static size_t ComputeNumNormals(
        const size_t numRadial,
        const bool closedSweep = true)
    {
        // Normals are per point.
        return ComputeNumPoints(numRadial, closedSweep);
    }

    static TfToken GetNormalsInterpolation()
    {
        // Normals are per point.
        return GeomUtilInterpolationTokens->vertex;
    }

    GEOMUTIL_API
    static PxOsdMeshTopology GenerateTopology(
        const size_t numRadial,
        const bool closedSweep = true);

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const ScalarType radius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        GeneratePoints(iter, numRadial,
                       /* bottomRadius = */ radius,
                       /* topRadius =    */ radius,
                       height, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        constexpr ScalarType sweep = 360;

        GeneratePoints(iter, numRadial,
                       bottomRadius, topRadius,
                       height, sweep, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const ScalarType sweepDegrees,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GeneratePointsImpl(numRadial, bottomRadius, topRadius, height, 
            sweepDegrees,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GeneratePoints;

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GenerateNormals(
        PointIterType iter,
        const size_t numRadial,
        const ScalarType radius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        GenerateNormals(iter, numRadial,
                             /* bottomRadius = */ radius,
                             /* topRadius =    */ radius,
                             height, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GenerateNormals(
        PointIterType iter,
        const size_t numRadial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        constexpr ScalarType sweep = 360;
        GenerateNormals(iter, numRadial,
                             bottomRadius, topRadius,
                             height, sweep, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GenerateNormals(
        PointIterType iter,
        const size_t numRadial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const ScalarType sweepDegrees,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GenerateNormalsImpl(numRadial, bottomRadius, topRadius, height,
            sweepDegrees,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GenerateNormals;

private:

    template<typename PointType>
    static void _GeneratePointsImpl(
        const size_t numRadial,
        const typename PointType::ScalarType bottomRadius,
        const typename PointType::ScalarType topRadius,
        const typename PointType::ScalarType height,
        const typename PointType::ScalarType sweepDegrees,
        const _PointWriter<PointType>& ptWriter);

    template<typename PointType>
    static void _GenerateNormalsImpl(
        const size_t numRadial,
        const typename PointType::ScalarType bottomRadius,
        const typename PointType::ScalarType topRadius,
        const typename PointType::ScalarType height,
        const typename PointType::ScalarType sweep,
        const _PointWriter<PointType>& ptWriter);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_CYLINDER_MESH_GENERATOR_H
