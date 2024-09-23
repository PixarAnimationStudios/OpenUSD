//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GEOM_UTIL_CAPSULE_MESH_GENERATOR_H
#define PXR_IMAGING_GEOM_UTIL_CAPSULE_MESH_GENERATOR_H

#include "pxr/imaging/geomUtil/api.h"
#include "pxr/imaging/geomUtil/meshGeneratorBase.h"
#include "pxr/imaging/geomUtil/tokens.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;
class PxOsdMeshTopology;

/// This class provides an implementation for generating topology, point
/// positions and surface normals on a capsule. The simplest form takes a radius
/// and height and is a cylinder capped by two hemispheres that is centered at
/// the origin. The generated capsule is made up of circular cross-sections in
/// the XY plane. Each cross-section has numRadial segments. Successive cross-
/// sections for each of the hemispheres are generated at numCapAxial locations
/// along the Z and -Z axes respectively. The height is aligned with the Z axis
/// and represents the height of just the cylindrical portion.
///
/// An optional transform may be provided to GeneratePoints and GenerateNormals
/// to orient the capsule as necessary (e.g., whose height is aligned with the
/// Y axis).
///
/// An additional overload of GeneratePoints is provided to specify different
/// radii and heights for the bottom and top caps, as well as the sweep angle
/// for the capsule about the +Z axis.  When the sweep is less than 360 degrees,
/// the generated geometry is not closed.
///
/// When the radii are different, the numCapAxial parameter is doubled and the
/// number of cross-sections will be divided between the top and bottom
/// hemispheres relative to the angle that each portion uses.  The topology will
/// remain the same while the density of the mesh is more even than if the
/// bottom and top caps used the same number of cross-sections.
///
/// Usage:
/// \code{.cpp}
///
/// const size_t numRadial = 4, numCapAxial = 4;
/// const size_t numPoints =
///     GeomUtilCapsuleMeshGenerator::ComputeNumPoints(numRadial, numCapAxial);
/// const float radius = 1, height = 2;
///
/// MyPointContainer<GfVec3f> points(numPoints);
///
/// GeomUtilCapsuleMeshGenerator::GeneratePoints(
///     points.begin(), numRadial, numCapAxial, radius, height);
///
/// const size_t numNormals =
///     GeomUtilCapsuleMeshGenerator::ComputeNumNormals(numRadial, numCapAxial);
///
/// MyPointContainer<GfVec3f> normals(numNormals);
///
/// GeomUtilCapsuleMeshGenerator::GenerateNormals(
///     normals.begin(), numRadial, numCapAxial, radius, height);
///
/// \endcode
///
class GeomUtilCapsuleMeshGenerator final
    : public GeomUtilMeshGeneratorBase
{
public:
    static constexpr size_t minNumRadial = 3;
    static constexpr size_t minNumCapAxial = 1;

    GEOMUTIL_API
    static size_t ComputeNumPoints(
        const size_t numRadial,
        const size_t numCapAxial,
        const bool closedSweep = true);

    static size_t ComputeNumNormals(
        const size_t numRadial,
        const size_t numCapAxial,
        const bool closedSweep = true)
    {
        // Normals are per point.
        return ComputeNumPoints(numRadial, numCapAxial, closedSweep);
    }

    static TfToken GetNormalsInterpolation()
    {
        // Normals are per point.
        return GeomUtilInterpolationTokens->vertex;
    }

    GEOMUTIL_API
    static PxOsdMeshTopology GenerateTopology(
        const size_t numRadial,
        const size_t numCapAxial,
        const bool closedSweep = true);

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const size_t numCapAxial,
        const ScalarType radius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        GeneratePoints(iter, numRadial, numCapAxial,
                       /* bottomRadius =    */ radius,
                       /* topRadius    =    */ radius,
                       height, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GeneratePoints(
        PointIterType iter,
        const size_t numRadial,
        const size_t numCapAxial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        constexpr ScalarType sweep = 360;

        GeneratePoints(iter, numRadial, numCapAxial,
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
        const size_t numCapAxial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const ScalarType sweepDegrees,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GeneratePointsImpl(numRadial, numCapAxial, bottomRadius, topRadius,
            height, sweepDegrees,
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
        const size_t numCapAxial,
        const ScalarType radius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        GenerateNormals(iter, numRadial, numCapAxial,
                        /* bottomRadius =    */ radius,
                        /* topRadius    =    */ radius,
                        height, framePtr);
    }

    template<typename PointIterType,
             typename ScalarType,
             typename Enabled =
                typename _EnableIfGfVec3Iterator<PointIterType>::type>
    static void GenerateNormals(
        PointIterType iter,
        const size_t numRadial,
        const size_t numCapAxial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const GfMatrix4d* framePtr = nullptr)
    {
        constexpr ScalarType sweep = 360;

        GenerateNormals(iter, numRadial, numCapAxial,
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
        const size_t numCapAxial,
        const ScalarType bottomRadius,
        const ScalarType topRadius,
        const ScalarType height,
        const ScalarType sweepDegrees,
        const GfMatrix4d* framePtr = nullptr)
    {
        using PointType =
            typename std::iterator_traits<PointIterType>::value_type;

        _GenerateNormalsImpl(numRadial, numCapAxial, bottomRadius, topRadius,
            height, sweepDegrees,
            framePtr ? _PointWriter<PointType>(iter, framePtr)
                     : _PointWriter<PointType>(iter));
    }

    using GeomUtilMeshGeneratorBase::GenerateNormals;

private:

    template<typename ScalarType>
    static size_t _ComputeNumBottomCapAxial(
        const size_t numCapAxial,
        const ScalarType latitudeRange);

    static size_t _ComputeNumTopCapAxial(
        const size_t numCapAxial,
        const size_t numBottomCapAxial);

    template<typename PointType>
    static void _GeneratePointsImpl(
        const size_t numRadial,
        const size_t numCapAxial,
        const typename PointType::ScalarType bottomRadius,
        const typename PointType::ScalarType topRadius,
        const typename PointType::ScalarType height,
        const typename PointType::ScalarType sweep,
        const _PointWriter<PointType>& ptWriter);

    template<typename PointType>
    static void _GenerateNormalsImpl(
        const size_t numRadial,
        const size_t numCapAxial,
        const typename PointType::ScalarType bottomRadius,
        const typename PointType::ScalarType topRadius,
        const typename PointType::ScalarType height,
        const typename PointType::ScalarType sweep,
        const _PointWriter<PointType>& ptWriter);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_GEOM_UTIL_CAPSULE_MESH_GENERATOR_H
