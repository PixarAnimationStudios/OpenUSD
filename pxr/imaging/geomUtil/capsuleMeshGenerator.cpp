//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/capsuleMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/vt/types.h"

#include <array>
#include <cmath>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


// static
size_t
GeomUtilCapsuleMeshGenerator::ComputeNumPoints(
    const size_t numRadial,
    const size_t numCapAxial,
    const bool closedSweep)
{
    if ((numRadial < minNumRadial) || (numCapAxial < minNumCapAxial)) {
        return 0;
    }

    const size_t numRadialPoints =
        _ComputeNumRadialPoints(numRadial, closedSweep);

    return ((2 * numCapAxial) * numRadialPoints) + 2;
}

// static
PxOsdMeshTopology
GeomUtilCapsuleMeshGenerator::GenerateTopology(
    const size_t numRadial,
    const size_t numCapAxial,
    const bool closedSweep)
{
    if ((numRadial < minNumRadial) || (numCapAxial < minNumCapAxial)) {
        return PxOsdMeshTopology();
    }

    return _GenerateCappedQuadTopology(
        numRadial,
        /* numQuadStrips =  */ (2 * (numCapAxial - 1)) + 1,
        /* bottomCapStyle = */ CapStyleSharedEdge,
        /* topCapStyle =    */ CapStyleSharedEdge,
        closedSweep);
}

// static
template<typename PointType>
void
GeomUtilCapsuleMeshGenerator::_GeneratePointsImpl(
    const size_t numRadial,
    const size_t numCapAxial,
    const typename PointType::ScalarType bottomRadius,
    const typename PointType::ScalarType topRadius,
    const typename PointType::ScalarType height,
    const typename PointType::ScalarType sweepDegrees,
    const _PointWriter<PointType>& ptWriter)
{
    using ScalarType = typename PointType::ScalarType;

    if ((numRadial < minNumRadial) || (numCapAxial < minNumCapAxial)) {
        return;
    }

    const ScalarType twoPi = 2.0 * M_PI;
    const ScalarType sweepRadians =
        GfClamp((ScalarType) GfDegreesToRadians(sweepDegrees), -twoPi, twoPi);
    const bool closedSweep = GfIsClose(std::abs(sweepRadians), twoPi, 1e-6);

    // Construct a circular arc of unit radius in the XY plane.
    const size_t numRadialPoints =
        _ComputeNumRadialPoints(numRadial, closedSweep);
    std::vector<std::array<ScalarType, 2>> ringXY(numRadialPoints);

    for (size_t radIdx = 0; radIdx < numRadialPoints; ++radIdx) {
        // Longitude range: [0, sweep]
        const ScalarType longAngle =
            (ScalarType(radIdx) / ScalarType(numRadial)) * sweepRadians;
        ringXY[radIdx][0] = cos(longAngle);
        ringXY[radIdx][1] = sin(longAngle);
    }

    ScalarType latitudeRange = 0.0;
    if (bottomRadius != topRadius)
    {
        // We're going to tesselate two spheres for the end caps. We need to
        // calculate the latitude at which to clip the spheres, which we can
        // derive from the line tangent to the two spheres.
        // Consider the larger circle at (0,0) of radius r1 and the smaller
        // at (0, height), of radius r2.
        // Shrink both circles by r2 - i.e. one circle is reduced to a point.
        const ScalarType rDiff = (bottomRadius - topRadius);

        // Now, the latidude at which to clip the sphere is the angle of
        // this point from the horizontal:
        latitudeRange = acos(rDiff / height);
    }

    // Bottom point:
    ptWriter.Write(PointType(0.0, 0.0, -(bottomRadius + (0.5 * height))));

    // Bottom hemisphere latitude rings:
    for (size_t axIdx = 1; axIdx < (numCapAxial + 1); ++axIdx) {
        // Latitude range: (-0.5pi, latitudeRange]
        const ScalarType latAngle = GfLerp(double(axIdx) / double(numCapAxial),
            ScalarType(-0.5 * M_PI), latitudeRange);

        const ScalarType radScale = cos(latAngle);
        const ScalarType latitude =
            -(0.5 * height) + (bottomRadius * sin(latAngle));

        for (size_t radIdx = 0; radIdx < numRadialPoints; ++radIdx) {
            ptWriter.Write(
                PointType(radScale * bottomRadius * ringXY[radIdx][0],
                          radScale * bottomRadius * ringXY[radIdx][1],
                          latitude));
        }
    }

    // Top hemisphere latitude rings:
    for (size_t axIdx = 0; axIdx < numCapAxial; ++axIdx) {
        // Latitude range: [latitudeRange, 0.5pi)
        const ScalarType latAngle = GfLerp(double(axIdx) / double(numCapAxial),
            latitudeRange, ScalarType(0.5 * M_PI));

        const ScalarType radScale = cos(latAngle);
        const ScalarType latitude =
            (0.5 * height) + (topRadius * sin(latAngle));

        for (size_t radIdx = 0; radIdx < numRadialPoints; ++radIdx) {
            ptWriter.Write(PointType(radScale * topRadius * ringXY[radIdx][0],
                                     radScale * topRadius * ringXY[radIdx][1],
                                     latitude));
        }
    }

    // Top point:
    ptWriter.Write(PointType(0.0, 0.0, topRadius + (0.5 * height)));
}

// Force-instantiate _GeneratePointsImpl for the supported point types.  Only
// these instantiations will ever be needed due to the SFINAE machinery on the
// calling method template (the public GeneratePoints, in the header).
template GEOMUTIL_API void GeomUtilCapsuleMeshGenerator::_GeneratePointsImpl(
    const size_t, const size_t, const float, const float, const float,
    const float, const GeomUtilCapsuleMeshGenerator::_PointWriter<GfVec3f>&);

template GEOMUTIL_API void GeomUtilCapsuleMeshGenerator::_GeneratePointsImpl(
    const size_t, const size_t, const double, const double, const double,
    const double, const GeomUtilCapsuleMeshGenerator::_PointWriter<GfVec3d>&);


PXR_NAMESPACE_CLOSE_SCOPE
