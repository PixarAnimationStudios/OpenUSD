//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GEOM_UTIL_SPHERE_MESH_GENERATOR_H
#define PXR_IMAGING_GEOM_UTIL_SPHERE_MESH_GENERATOR_H

#include "pxr/imaging/geomUtil/api.h"
#include "pxr/imaging/geomUtil/meshGeneratorBase.h"
#include "pxr/imaging/geomUtil/tokens.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;
class PxOsdMeshTopology;

/// This class provides an implementation for generating topology, point
/// positions and surface normals on a sphere with a given radius. The sphere
/// is made up of circular cross-sections in the XY plane and is centered at the
/// origin. Each cross-section has numRadial segments. Successive cross-sections
/// are generated at numAxial locations along the Z axis, with the bottom of the
/// sphere at Z = -r and top at Z = r.
///
/// An optional transform may be provided to GeneratePoints and GenerateNormals
/// to orient the sphere as necessary (e.g., cross-sections in the YZ plane).
///
/// An additional overload of GeneratePoints is provided to specify a sweep
/// angle for the sphere about the +Z axis.  When the sweep is less than 360
/// degrees, the generated geometry is not closed.
///
/// Usage:
/// \code{.cpp}
///
/// const size_t numRadial = 4, numAxial = 4;
/// const size_t numPoints =
///     GeomUtilSphereMeshGenerator::ComputeNumPoints(numRadial, numAxial);
/// const float radius = 5;
///
/// MyPointContainer<GfVec3f> points(numPoints);
///
/// GeomUtilSphereMeshGenerator::GeneratePoints(
///     points.begin(), numRadial, numAxial, radius);
///
/// const size_t numNormals =
///     GeomUtilSphereMeshGenerator::ComputeNumPoints(numRadial, numAxial);
///
/// MyPointContainer<GfVec3f> normals(numNormals);
///
/// GeomUtilSphereMeshGenerator::GenerateNormals(
///     normals.begin(), numRadial, numAxial);
///
/// \endcode
///
class GeomUtilSphereMeshGenerator final
    : public GeomUtilMeshGeneratorBase
{
public:
    static constexpr size_t minNumRadial = 3;
    static constexpr size_t minNumAxial = 2;

    GEOMUTIL_API
    static size_t ComputeNumPoints(
        const size_t numRadial,
        const size_t numAxial,
        const bool closedSweep = true);

    static size_t ComputeNumNormals(
        const size_t numRadial,
        const size_t numAxial,
        const bool closedSweep = true)
    {
        // Normals are per point.
        return ComputeNumPoints(numRadial, numAxial, closedSweep);
    }

    static TfToken GetNormalsInterpolation()
    {
        // Normals are per point.
        return GeomUtilInterpolationTokens->vertex;
    }

    GEOMUTIL_API
    static PxOsdMeshTopology GenerateTopology(
        const size_t numRadial,
        const size_t numAxial,
        const bool closedSweep = true);

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const size_t numAxial,
        const ScalarType radius,
        const GfMatrix4d* framePtr = nullptr)
    {
        constexpr ScalarType sweep = 360;
        GeneratePoints(iter, numRadial, numAxial, radius, sweep, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const size_t numAxial,
        const ScalarType radius,
        const ScalarType sweepDegrees,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GeneratePointsImpl(numRadial, numAxial, radius, sweepDegrees,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GeneratePoints;

    template<typename PointIterType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GenerateNormals(
        PointIterType iter,
        const size_t numRadial,
        const size_t numAxial,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        constexpr typename PointType::ScalarType sweep = 360;
        GenerateNormals(iter, numRadial, numAxial, sweep, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GenerateNormals(
        PointIterType iter,
        const size_t numRadial,
        const size_t numAxial,
        const ScalarType sweepDegrees,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GenerateNormalsImpl(numRadial, numAxial, sweepDegrees,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GenerateNormals;

private:

    template<typename PointType>
    static void _GeneratePointsImpl(
        const size_t numRadial,
        const size_t numAxial,
        const typename PointType::ScalarType radius,
        const typename PointType::ScalarType sweepDegrees,
        const _PointWriter<PointType>& ptWriter);

    template<typename PointType>
    static void _GenerateNormalsImpl(
        const size_t numRadial,
        const size_t numAxial,
        const typename PointType::ScalarType sweepDegrees,
        const _PointWriter<PointType>& ptWriter);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_SPHERE_MESH_GENERATOR_H
