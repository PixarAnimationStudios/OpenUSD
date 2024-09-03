//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/diskMeshGenerator.h"

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
GeomUtilDiskMeshGenerator::ComputeNumPoints(
    const size_t numRadial,
    const bool closedSweep)
{
    if (numRadial < minNumRadial) {
        return 0;
    }

    return _ComputeNumCappedQuadTopologyPoints(
        numRadial,
        /* numQuadStrips =  */ 0,
        /* bottomCapStyle = */ CapStyleNone,
        /* topCapStyle =    */ CapStyleSeparateEdge,
        closedSweep);
}

// static
PxOsdMeshTopology
GeomUtilDiskMeshGenerator::GenerateTopology(
    const size_t numRadial,
    const bool closedSweep)
{
    if (numRadial < minNumRadial) {
        return PxOsdMeshTopology();
    }

    return _GenerateCappedQuadTopology(
        numRadial,
        /* numQuadStrips =  */ 0,
        /* bottomCapStyle = */ CapStyleNone,
        /* topCapStyle =    */ CapStyleSeparateEdge,
        closedSweep);
}

// static
template<typename PointType>
void
GeomUtilDiskMeshGenerator::_GeneratePointsImpl(
    const size_t numRadial,
    const typename PointType::ScalarType radius,
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

    // A ring for the outer edge.
    ptWriter.WriteArc(radius, ringXY, 0.0);

    // Center point:
    ptWriter.Write(PointType(0.0, 0.0, 0.0));
}

// Force-instantiate _GeneratePointsImpl for the supported point types.  Only
// these instantiations will ever be needed due to the SFINAE machinery on the
// calling method template (the public GeneratePoints, in the header).
template GEOMUTIL_API void GeomUtilDiskMeshGenerator::_GeneratePointsImpl(
    const size_t, const float, const float,
    const GeomUtilDiskMeshGenerator::_PointWriter<GfVec3f>&);

template GEOMUTIL_API void GeomUtilDiskMeshGenerator::_GeneratePointsImpl(
    const size_t, const double, const double,
    const GeomUtilDiskMeshGenerator::_PointWriter<GfVec3d>&);


PXR_NAMESPACE_CLOSE_SCOPE