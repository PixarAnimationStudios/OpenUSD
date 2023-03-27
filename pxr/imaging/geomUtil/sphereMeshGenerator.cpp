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

    const size_t numRadialPoints =
        _ComputeNumRadialPoints(numRadial, closedSweep);

    return((numAxial - 1) * numRadialPoints) + 2;
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

    const ScalarType twoPi = 2.0 * M_PI;
    const ScalarType sweepRadians =
        GfClamp((ScalarType) GfDegreesToRadians(sweepDegrees), -twoPi, twoPi);
    const bool closedSweep = GfIsClose(std::abs(sweepRadians), twoPi, 1e-6);
    
    // Construct a circular arc/ring of the specified radius in the XY plane.
    const size_t numRadialPoints =
        _ComputeNumRadialPoints(numRadial, closedSweep);
    std::vector<std::array<ScalarType, 2>> ringXY(numRadialPoints);

    for (size_t radIdx = 0; radIdx < numRadialPoints; ++radIdx) {
        // Longitude range: [0, sweep]
        const ScalarType longAngle =
            (ScalarType(radIdx) / ScalarType(numRadial)) * sweepRadians;
        ringXY[radIdx][0] = radius * cos(longAngle);
        ringXY[radIdx][1] = radius * sin(longAngle);
    }

    // Bottom point:
    ptWriter.Write(PointType(0.0, 0.0, -radius));

    // Latitude rings:
    for (size_t axIdx = 1; axIdx < numAxial; ++axIdx) {
        // Latitude range: (-0.5pi, 0.5pi)
        const ScalarType latAngle =
            ((ScalarType(axIdx) / ScalarType(numAxial)) - 0.5) * M_PI;

        const ScalarType radScale = cos(latAngle);
        const ScalarType latitude = radius * sin(latAngle);

        for (size_t radIdx = 0; radIdx < numRadialPoints; ++radIdx) {
            ptWriter.Write(PointType(radScale * ringXY[radIdx][0],
                                     radScale * ringXY[radIdx][1],
                                     latitude));
        }
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