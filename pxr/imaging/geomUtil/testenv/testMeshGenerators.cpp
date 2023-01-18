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

#include "pxr/pxr.h"

#include "pxr/imaging/geomUtil/capsuleMeshGenerator.h"
#include "pxr/imaging/geomUtil/coneMeshGenerator.h"
#include "pxr/imaging/geomUtil/cuboidMeshGenerator.h"
#include "pxr/imaging/geomUtil/cylinderMeshGenerator.h"
#include "pxr/imaging/geomUtil/sphereMeshGenerator.h"
#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/array.h"

#include <iostream>
#include <fstream>

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {

static void
_LogHeader(std::string const &msg, std::ofstream &out)
{
    out << msg << std::endl;
    out << std::string(msg.length(), '-') << std::endl;
}

static void
_LogFooter(std::ofstream &out)
{
    out << std::endl << std::endl;
}

template <typename T>
static void
_Log(PxOsdMeshTopology const &topology,
     VtArray<T> const &points,
     std::ofstream &out)
{
    out << "Topology:\n";
    out << "  " << topology << std::endl << std::endl;

    out << "Points:\n";
    out << "  " << points << std::endl << std::endl;
}

}

static bool TestTopologyAndPointGeneration(
    const float sweep, std::ofstream &out)
{
    const bool closedSweep =
        GfIsClose(cos(GfDegreesToRadians(sweep)), 1.0, 1e-4);
    {
        _LogHeader("1. Capsule", out);

        using MeshGen = GeomUtilCapsuleMeshGenerator;

        const size_t numRadial = 10, numCapAxial = 4;
        const float radius = 0.5, height = 2;

        out << "radius = "   << radius
            << ", height = " << height
            << ", sweep = "  << sweep
            << std::endl << std::endl;

        const PxOsdMeshTopology topology =
            MeshGen::GenerateTopology(numRadial, numCapAxial, closedSweep);

        const size_t numPoints =
            MeshGen::ComputeNumPoints(numRadial, numCapAxial, closedSweep);
        VtVec3fArray points(numPoints);
        if (closedSweep) {
            MeshGen::GeneratePoints(
                points.begin(), numRadial, numCapAxial, radius, height);

        } else {
            MeshGen::GeneratePoints(
                points.begin(), numRadial, numCapAxial,
                /* bottomRadius =    */ radius,
                /* topRadius    =    */ radius,
                height,
                /* bottomCapHeight = */ radius,
                /* topCapHeight =    */ radius,
                sweep);
        }
        
        _Log(topology, points, out);

        _LogFooter(out);
    }

    {
        _LogHeader("2. Cone", out);

        using MeshGen = GeomUtilConeMeshGenerator;

        const size_t numRadial = 10;
        const float radius = 0.5, height = 2;

        out << "radius = "   << radius 
            << ", height = " << height
            << ", sweep = "  << sweep
            << std::endl << std::endl;

        const PxOsdMeshTopology topology =
            MeshGen::GenerateTopology(numRadial, closedSweep);

        const size_t numPoints =
            MeshGen::ComputeNumPoints(numRadial, closedSweep);
        VtVec3fArray points(numPoints);
        MeshGen::GeneratePoints(
            points.begin(), numRadial, radius, height, sweep);
        
        _Log(topology, points, out);

        _LogFooter(out);
    }

    {
        _LogHeader("3. Cube", out);

        using MeshGen = GeomUtilCuboidMeshGenerator;

        const float side = 1.0;

        out << "side = " << side << std::endl << std::endl;

        const PxOsdMeshTopology topology = MeshGen::GenerateTopology();

        const size_t numPoints = MeshGen::ComputeNumPoints();
        VtVec3fArray points(numPoints);
        MeshGen::GeneratePoints(points.begin(), side, side, side);
        
        _Log(topology, points, out);

        _LogFooter(out);
    }

    {
        _LogHeader("4. Cylinder", out);

        using MeshGen = GeomUtilCylinderMeshGenerator;

        const size_t numRadial = 10;
        const float radius = 0.5, height = 2;

        out << "radius = "   << radius 
            << ", height = " << height
            << ", sweep = "  << sweep
            << std::endl << std::endl;

        const PxOsdMeshTopology topology =
            MeshGen::GenerateTopology(numRadial, closedSweep);

        const size_t numPoints =
            MeshGen::ComputeNumPoints(numRadial, closedSweep);
        VtVec3fArray points(numPoints);
        if (closedSweep) {
            MeshGen::GeneratePoints(
                points.begin(), numRadial, radius, height);

        } else {
            MeshGen::GeneratePoints(
                points.begin(), numRadial,
                /* bottomRadius = */ radius,
                /* topRadius =    */ radius,
                height, sweep);
        }
        
        _Log(topology, points, out);

        _LogFooter(out);
    }

    {
         _LogHeader("5. Sphere", out);

        using MeshGen = GeomUtilSphereMeshGenerator;

        const size_t numRadial = 10, numAxial = 10;
        const float radius = 0.5;

        out << "radius = "   << radius 
            << ", sweep = "  << sweep
            << std::endl << std::endl;

        const PxOsdMeshTopology topology =
            MeshGen::GenerateTopology(numRadial, numAxial, closedSweep);

        const size_t numPoints =
            MeshGen::ComputeNumPoints(numRadial, numAxial, closedSweep);
        VtVec3fArray points(numPoints);
        MeshGen::GeneratePoints(
            points.begin(), numRadial, numAxial, radius, sweep);
        
        _Log(topology, points, out);

        _LogFooter(out);
    }

    return true;
}

int main()
{ 
    TfErrorMark mark;

    std::ofstream out1("generatedMeshes_closed.txt");
    std::ofstream out2("generatedMeshes_open.txt");

    bool success =    TestTopologyAndPointGeneration(/*sweep = */ 360, out1)
                   && TestTopologyAndPointGeneration(/*sweep = */ 120, out2);

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
