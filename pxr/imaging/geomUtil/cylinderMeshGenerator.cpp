//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/cylinderMeshGenerator.h"

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
GeomUtilCylinderMeshGenerator::ComputeNumPoints(
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
        /* topCapStyle =    */ CapStyleSeparateEdge,
        closedSweep);
}

// static
PxOsdMeshTopology
GeomUtilCylinderMeshGenerator::GenerateTopology(
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
        /* topCapStyle =    */ CapStyleSeparateEdge,
        closedSweep);
}

// static
template<typename PointType>
void
GeomUtilCylinderMeshGenerator::_GeneratePointsImpl(
    const size_t numRadial,
    const typename PointType::ScalarType bottomRadius,
    const typename PointType::ScalarType topRadius,
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
    // cylinder quads (for normals reasons the bottom "edge" is not shared):
    ptWriter.WriteArc(bottomRadius, ringXY, zMin);
    ptWriter.WriteArc(bottomRadius, ringXY, zMin);

    // And another two rings, for the top edge.
    ptWriter.WriteArc(topRadius, ringXY, zMax);
    ptWriter.WriteArc(topRadius, ringXY, zMax);

    // Top point:
    ptWriter.Write(PointType(0.0, 0.0, zMax));
}

// Force-instantiate _GeneratePointsImpl for the supported point types.  Only
// these instantiations will ever be needed due to the SFINAE machinery on the
// calling method template (the public GeneratePoints, in the header).
template GEOMUTIL_API void GeomUtilCylinderMeshGenerator::_GeneratePointsImpl(
    const size_t, const float, const float, const float, const float,
    const GeomUtilCylinderMeshGenerator::_PointWriter<GfVec3f>&);

template GEOMUTIL_API void GeomUtilCylinderMeshGenerator::_GeneratePointsImpl(
    const size_t, const double, const double, const double, const double,
    const GeomUtilCylinderMeshGenerator::_PointWriter<GfVec3d>&);


PXR_NAMESPACE_CLOSE_SCOPE