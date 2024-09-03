//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/sphereMeshGenerator.h"

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
GeomUtilSphereMeshGenerator::ComputeNumPoints(
    const size_t numRadial,
    const size_t numAxial,
    const bool closedSweep)
{ 
    if ((numRadial < minNumRadial) || (numAxial < minNumAxial)) {
        return 0;
    }

    return _ComputeNumCappedQuadTopologyPoints(
        numRadial,
        /* numQuadStrips =  */ numAxial - 2,
        /* bottomCapStyle = */ CapStyleSharedEdge,
        /* topCapStyle =    */ CapStyleSharedEdge,
        closedSweep);
}

// static
PxOsdMeshTopology
GeomUtilSphereMeshGenerator::GenerateTopology(
    const size_t numRadial,
    const size_t numAxial,
    const bool closedSweep)
{
    if ((numRadial < minNumRadial) || (numAxial < minNumAxial)) {
        return PxOsdMeshTopology();
    }

    return _GenerateCappedQuadTopology(
        numRadial,
        /* numQuadStrips =  */ numAxial - 2,
        /* bottomCapStyle = */ CapStyleSharedEdge,
        /* topCapStyle =    */ CapStyleSharedEdge,
        closedSweep);
}

// static
template<typename PointType>
void
GeomUtilSphereMeshGenerator::_GeneratePointsImpl(
    const size_t numRadial,
    const size_t numAxial,
    const typename PointType::ScalarType radius,
    const typename PointType::ScalarType sweepDegrees,
    const _PointWriter<PointType>& ptWriter)
{
    using ScalarType = typename PointType::ScalarType;

    if ((numRadial < minNumRadial) || (numAxial < minNumAxial)) {
        return;
    }

    // Construct a circular arc/ring of the specified radius in the XY plane.
    const std::vector<std::array<ScalarType, 2>> ringXY =
        _GenerateUnitArcXY<ScalarType>(numRadial, sweepDegrees);

    // Bottom point:
    ptWriter.Write(PointType(0.0, 0.0, -radius));

    // Latitude rings:
    for (size_t axIdx = 1; axIdx < numAxial; ++axIdx) {
        // Latitude range: (-0.5pi, 0.5pi)
        const ScalarType latAngle =
            ((ScalarType(axIdx) / ScalarType(numAxial)) - 0.5) * M_PI;

        const ScalarType radScale = radius * cos(latAngle);
        const ScalarType latitude = radius * sin(latAngle);

        ptWriter.WriteArc(radScale, ringXY, latitude);
    }

    // Top point:
    ptWriter.Write(PointType(0.0, 0.0, radius));
}

// Force-instantiate _GeneratePointsImpl for the supported point types.  Only
// these instantiations will ever be needed due to the SFINAE machinery on the
// calling method template (the public GeneratePoints, in the header).
template GEOMUTIL_API void GeomUtilSphereMeshGenerator::_GeneratePointsImpl(
    const size_t, const size_t, const float, const float,
    const GeomUtilSphereMeshGenerator::_PointWriter<GfVec3f>&);

template GEOMUTIL_API void GeomUtilSphereMeshGenerator::_GeneratePointsImpl(
    const size_t, const size_t, const double, const double,
    const GeomUtilSphereMeshGenerator::_PointWriter<GfVec3d>&);


PXR_NAMESPACE_CLOSE_SCOPE