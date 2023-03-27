//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

    const size_t numRadialPoints =
        _ComputeNumRadialPoints(numRadial, closedSweep);

    return (3 * numRadialPoints) + 1;
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

    const ScalarType twoPi = 2.0 * M_PI;
    const ScalarType sweepRadians =
        GfClamp((ScalarType) GfDegreesToRadians(sweepDegrees), -twoPi, twoPi);
    const bool closedSweep = GfIsClose(std::abs(sweepRadians), twoPi, 1e-6);

    // Construct a circular arc/ring of the specified radius in the XY plane.
    const size_t numRadialPoints =
        _ComputeNumRadialPoints(numRadial, closedSweep);
    std::vector<std::array<ScalarType, 2>> ringXY(numRadialPoints);

    for (size_t radIdx = 0; radIdx < numRadialPoints; ++radIdx) {
        // Longitude range: [0, 2pi)
        const ScalarType longAngle =
            (ScalarType(radIdx) / ScalarType(numRadial)) * sweepRadians;
        ringXY[radIdx][0] = radius * cos(longAngle);
        ringXY[radIdx][1] = radius * sin(longAngle);
    }

    const ScalarType zMax = 0.5 * height;
    const ScalarType zMin = -zMax;

    // Bottom point:
    ptWriter.Write(PointType(0.0, 0.0, zMin));

    // Bottom rings; two consecutive rings at the same point locations, the
    // first for the bottom triangle fan and the second for the main
    // cone quads (for normals reasons the bottom "edge" is not shared):
    for (size_t ringIdx = 0; ringIdx < 2; ++ringIdx) {
        for (size_t radIdx = 0; radIdx < numRadialPoints; ++radIdx) {
            ptWriter.Write(PointType(ringXY[radIdx][0],
                                     ringXY[radIdx][1],
                                     zMin));
        }
    }

    // Top "ring" (all points coincident); the cone consists of degenerate
    // quads, so edges between left/right faces generate smooth normals but
    // there's no continuity over the top "point" as would happen with a
    // triangle fan.
    for (size_t radIdx = 0; radIdx < numRadialPoints; ++radIdx) {
        ptWriter.Write(PointType(0.0, 0.0, zMax));
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


PXR_NAMESPACE_CLOSE_SCOPE