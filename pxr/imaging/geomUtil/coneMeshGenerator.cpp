//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/coneMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/vt/types.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


// static
size_t
GeomUtilConeMeshGenerator::ComputeNumPoints(
    const size_t numRadial,
    const bool closedSweep)
{
    if (numRadial < minNumRadial) {
        return 0;
    }

    return _ComputeNumCappedQuadTopologyPoints(
        numRadial,
        /* numQuadStrips =  */ 1,
        /* bottomCapStyle = */ CapStyleSeparateEdge,
        /* topCapStyle =    */ CapStyleNone,
        closedSweep);
}

// static
PxOsdMeshTopology
GeomUtilConeMeshGenerator::GenerateTopology(
    const size_t numRadial,
    const bool closedSweep)
{
    if (numRadial < minNumRadial) {
        return PxOsdMeshTopology();
    }

    return _GenerateCappedQuadTopology(
        numRadial,
        /* numQuadStrips =  */ 1,
        /* bottomCapStyle = */ CapStyleSeparateEdge,
        /* topCapStyle =    */ CapStyleNone,
        closedSweep);
}

// static
template<typename PointType>
void
GeomUtilConeMeshGenerator::_GeneratePointsImpl(
    const size_t numRadial,
    const typename PointType::ScalarType radius,
    const typename PointType::ScalarType height,
    const typename PointType::ScalarType sweepDegrees,
    const _PointWriter<PointType>& ptWriter)
{
    using ScalarType = typename PointType::ScalarType;

    if (numRadial < minNumRadial) {
        return;
    }

    // Construct a circular arc of unit radius in the XY plane.
    const std::vector<std::array<ScalarType, 2>> ringXY =
        _GenerateUnitArcXY<ScalarType>(numRadial, sweepDegrees);

    const ScalarType zMax = 0.5 * height;
    const ScalarType zMin = -zMax;

    // Bottom point:
    ptWriter.Write(PointType(0.0, 0.0, zMin));

    // Bottom rings; two consecutive rings at the same point locations, the
    // first for the bottom triangle fan and the second for the main
    // cone quads (for normals reasons the bottom "edge" is not shared):
    ptWriter.WriteArc(radius, ringXY, zMin);
    ptWriter.WriteArc(radius, ringXY, zMin);

    // Top "ring" (all points coincident); the cone consists of degenerate
    // quads, so edges between left/right faces generate smooth normals but
    // there's no continuity over the top "point" as would happen with a
    // triangle fan.
    const PointType topPoint(0.0, 0.0, zMax);
    for (size_t radIdx = 0; radIdx < ringXY.size(); ++radIdx) {
        ptWriter.Write(topPoint);
    }
}

// Force-instantiate _GeneratePointsImpl for the supported point types.  Only
// these instantiations will ever be needed due to the SFINAE machinery on the
// calling method template (the public GeneratePoints, in the header).
template GEOMUTIL_API void GeomUtilConeMeshGenerator::_GeneratePointsImpl(
    const size_t, const float, const float, const float,
    const GeomUtilConeMeshGenerator::_PointWriter<GfVec3f>&);

template GEOMUTIL_API void GeomUtilConeMeshGenerator::_GeneratePointsImpl(
    const size_t, const double, const double, const double,
    const GeomUtilConeMeshGenerator::_PointWriter<GfVec3d>&);


// static
template<typename PointType>
void
GeomUtilConeMeshGenerator::_GenerateNormalsImpl(
    const size_t numRadial,
    const typename PointType::ScalarType radius,
    const typename PointType::ScalarType height,
    const typename PointType::ScalarType sweepDegrees,
    const _PointWriter<PointType>& ptWriter)
{
    using ScalarType = typename PointType::ScalarType;

    if (numRadial < minNumRadial) {
        return;
    }

    // Construct a circular arc of unit radius in the XY plane.
    const std::vector<std::array<ScalarType, 2>> ringXY =
        _GenerateUnitArcXY<ScalarType>(numRadial, sweepDegrees);

    // Determine the radius scalar and latitude for the normals
    // that are perpendicular to the sides of the cone.
    ScalarType radScale, latitude;
    if (height != 0) {
        // Calculate the following directly, without using trig functions:
        // radScale = cos(atan(slope)) =   1.0 / sqrt(1.0 + slope^2)
        // latitude = sin(atan(slope)) = slope / sqrt(1.0 + slope^2)
        const ScalarType slope = radius / height;
        radScale = 1.0 / GfSqrt(1.0 + GfSqr(slope));
        latitude = slope * radScale;
    }
    else {
        // Degenerate cone, just use something sensible.
        radScale = 0.0;
        latitude = 1.0;
    }

    constexpr PointType baseNormal(0.0, 0.0, -1.0);

    // Bottom point:
    ptWriter.WriteDir(baseNormal);

    // First bottom ring which is part of the base, so use the base normal.
    for (size_t i = 0; i < ringXY.size(); ++i) {
        ptWriter.WriteDir(baseNormal);
    }

    // Second bottom ring and top "ring" are the normals at the sides
    // of the cone and are the same normals.
    ptWriter.WriteArcDir(radScale, ringXY, latitude);
    ptWriter.WriteArcDir(radScale, ringXY, latitude);
}

// Force-instantiate _GenerateNormalsImpl for the supported point types.
// Only these instantiations will ever be needed due to the SFINAE machinery on
// the calling method template (the public GeneratePoints, in the header).
template GEOMUTIL_API void GeomUtilConeMeshGenerator::_GenerateNormalsImpl(
    const size_t, const float, const float, const float,
    const GeomUtilConeMeshGenerator::_PointWriter<GfVec3f>&);

template GEOMUTIL_API void GeomUtilConeMeshGenerator::_GenerateNormalsImpl(
    const size_t, const double, const double, const double,
    const GeomUtilConeMeshGenerator::_PointWriter<GfVec3d>&);


PXR_NAMESPACE_CLOSE_SCOPE