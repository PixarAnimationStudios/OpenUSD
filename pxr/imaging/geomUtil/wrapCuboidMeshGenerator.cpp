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
#include "pxr/imaging/geomUtil/cuboidMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/vt/types.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static VtVec3fArray
_WrapGeneratePoints(
    const float xLength,
    const float yLength,
    const float zLength)
{
    const size_t numPoints =
        GeomUtilCuboidMeshGenerator::ComputeNumPoints();
    if (numPoints == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray points(numPoints);
    GeomUtilCuboidMeshGenerator::GeneratePoints(
        points.begin(), xLength, yLength, zLength);

    return points;
}

void wrapCuboidMeshGenerator()
{
    using This = GeomUtilCuboidMeshGenerator;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    class_<This>("CuboidMeshGenerator", no_init)

        .def("ComputeNumPoints", &This::ComputeNumPoints)
        .staticmethod("ComputeNumPoints")

        .def("GenerateTopology", &This::GenerateTopology)
        .staticmethod("GenerateTopology")

        .def("GeneratePoints", &_WrapGeneratePoints)
        .staticmethod("GeneratePoints")
    ;
}