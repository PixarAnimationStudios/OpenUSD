//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GEOM_UTIL_DISK_MESH_GENERATOR_H
#define PXR_IMAGING_GEOM_UTIL_DISK_MESH_GENERATOR_H

#include "pxr/imaging/geomUtil/api.h"
#include "pxr/imaging/geomUtil/meshGeneratorBase.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;
class PxOsdMeshTopology;

/// This class provides an implementation for generating topology and point
/// positions on a circular Disk given the radius with numRadial segments.
/// The generated Disk is centered at the origin.
///
/// An optional transform may be provided to GeneratePoints to orient the
/// Disk as necessary.
///
/// Usage:
/// \code{.cpp}
///
/// const size_t numRadial = 8;
/// const size_t numPoints =
///     GeomUtilDiskMeshGenerator::ComputeNumPoints(numRadial);
/// const float radius = 6;
///
/// MyPointContainer<GfVec3f> points(numPoints);
///
/// GeomUtilDiskMeshGenerator::GeneratePoints(
///     points.begin(), numRadial, radius);
///
/// \endcode
///
class GeomUtilDiskMeshGenerator final
    : public GeomUtilMeshGeneratorBase
{
public:
    static constexpr size_t minNumRadial = 3;

    GEOMUTIL_API
    static size_t ComputeNumPoints(
        const size_t numRadial,
        const bool closedSweep = true);

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
        const GfMatrix4d* framePtr = nullptr)
    {
        constexpr ScalarType sweep = 360;

        GeneratePoints(iter, numRadial,
                       radius, sweep, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const ScalarType radius,
        const ScalarType sweepDegrees,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GeneratePointsImpl(numRadial, radius, sweepDegrees,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GeneratePoints;

private:

    template<typename PointType>
    static void _GeneratePointsImpl(
        const size_t numRadial,
        const typename PointType::ScalarType radius,
        const typename PointType::ScalarType sweepDegrees,
        const _PointWriter<PointType>& ptWriter);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_DISK_MESH_GENERATOR_H
