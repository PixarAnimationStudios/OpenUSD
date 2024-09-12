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

    return _ComputeNumCappedQuadTopologyPoints(
        numRadial,
        /* numQuadStrips =  */ (2 * (numCapAxial - 1)) + 1,
        /* bottomCapStyle = */ CapStyleSharedEdge,
        /* topCapStyle =    */ CapStyleSharedEdge,
        closedSweep);
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

    // Construct a circular arc of unit radius in the XY plane.
    const std::vector<std::array<ScalarType, 2>> ringXY =
        _GenerateUnitArcXY<ScalarType>(numRadial, sweepDegrees);

    // Initialize the center offset of the bottom (offset0) and top (offset1)
    // spherical caps to 0.  These will be adjusted below by half of the height
    // after first adjusting for the case when the radii are different.
    ScalarType offset0 = 0;
    ScalarType offset1 = 0;

    // Initialize the radii of the bottom (radius0) and top (radius1) spherical
    // caps to the given bottom and top radius of the cylindrical portion of the
    // capsule.
    ScalarType radius0 = bottomRadius;
    ScalarType radius1 = topRadius;

    // This angle represents the latitude where the bottom spherical cap will
    // transition to the cylindrical portion of the capsule, as well as the
    // angle where the top spherical cap begins after transitioning from the
    // cylindrical portion.
    ScalarType latitudeRange = 0.0;

    if (bottomRadius != topRadius && height != 0) {
        // We need to calculate the angle where the transition should occur
        // between the cylindrical and spherical caps of the capsule, as well
        // as adjust the radii and center offsets of the spherical caps.
        // Imagine the capsule aligned with the X-axis and in cross section in
        // order to use trigonometry to determine these values.  For clarity,
        // the spherical caps are omitted from this drawing.
        //
        //          A /---------
        //           / |        --------
        //          /  |                --------
        //         /   |                        --------
        //        /    | B                               -------- C
        //       /     | - - - - - - - - - - - - - - - - - - - -/|
        //      /      |                                       / |
        //     /_______|______________________________________/__|
        //   D         E                                     F    G
        //
        // |AE| bottomRadius   |CG| topRadius   |BC| height
        //  D center of the bottom spherical cap
        //  F center of the top spherical cap
        //
        // Triangles ADE, ABC, and CFG are all right triangles and are also
        // similar because the tangents of the spherical caps must be tangent
        // to the cylindrical portion of the capsule.  We need to calcuate
        // |DE| and |FG| to determine the spherical cap center offsets, as well
        // as |AD| and |CF| to determine the spherical cap radii.

        // Calculate the slope of segment AC, using |AB| / |BC|.
        const ScalarType slope = (bottomRadius - topRadius) / height;

        // Use the slope and the law of similar triangles to calculate the
        // spherical cap center offsets.  For the bottom cap that is:
        //   |DE| / |AE| = |AB| / |BC|
        //   |DE| / |AE| = slope
        //   |DE|        = slope * |AE|
        offset0 = -(slope * bottomRadius);
        offset1 = -(slope * topRadius);

        // Use the Pythagoream theorem to calculate the spherical cap radii.
        //   |AD| = sqrt(|AE|^2 + |DE|^2)
        //   |CF| = sqrt(|CG|^2 + |FG|^2)
        radius0 = GfSqrt(GfSqr(bottomRadius) + GfSqr(offset0));
        radius1 = GfSqrt(GfSqr(topRadius) + GfSqr(offset1));

        // Use the slope to determine the angle at A of triangle ADE to
        // calculate the latitude for transitioning between the spherical caps
        // and the cylindrical portion of the capsule.
        latitudeRange = atan(slope);
    }

    // Adjust the sphere offsets to include the height.
    offset0 -= 0.5 * height;
    offset1 += 0.5 * height;


    // Calculate the number of axial points of the sphere caps so that the
    // number used is relative to the size of the caps, which will lead to the
    // the mesh of the caps having similar density.
    const size_t numCapAxial0 = GfMin(GfMax(
        size_t(GfRound(numCapAxial * 2 * (0.5 * M_PI + latitudeRange) / M_PI)),
        minNumCapAxial), (2 * numCapAxial) - minNumCapAxial);
    const size_t numCapAxial1 = (2 * numCapAxial) - numCapAxial0;

    // Bottom point:
    ptWriter.Write(PointType(0.0, 0.0, offset0 - radius0));

    // Bottom hemisphere latitude rings:
    for (size_t axIdx = 1; axIdx < (numCapAxial0 + 1); ++axIdx) {
        // Latitude range: (-0.5pi, latitudeRange]
        const ScalarType latAngle = GfLerp(double(axIdx) / double(numCapAxial0),
            ScalarType(-0.5 * M_PI), latitudeRange);

        const ScalarType radScale = radius0 * cos(latAngle);
        const ScalarType latitude = offset0 + (radius0 * sin(latAngle));

        ptWriter.WriteArc(radScale, ringXY, latitude);
    }

    // Top hemisphere latitude rings:
    for (size_t axIdx = 0; axIdx < numCapAxial1; ++axIdx) {
        // Latitude range: [latitudeRange, 0.5pi)
        const ScalarType latAngle = GfLerp(double(axIdx) / double(numCapAxial1),
            latitudeRange, ScalarType(0.5 * M_PI));

        const ScalarType radScale = radius1 * cos(latAngle);
        const ScalarType latitude = offset1 + (radius1 * sin(latAngle));

        ptWriter.WriteArc(radScale, ringXY, latitude);
    }

    // Top point:
    ptWriter.Write(PointType(0.0, 0.0, offset1 + radius1));
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
