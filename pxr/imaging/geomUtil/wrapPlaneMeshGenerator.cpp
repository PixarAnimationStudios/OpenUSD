//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/planeMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/vt/types.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static VtVec3fArray
_WrapGeneratePoints(
    const float xLength,
    const float yLength)
{
    const size_t numPoints =
        GeomUtilPlaneMeshGenerator::ComputeNumPoints();
    if (numPoints == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray points(numPoints);
    GeomUtilPlaneMeshGenerator::GeneratePoints(
        points.begin(), xLength, yLength);

    return points;
}

void wrapPlaneMeshGenerator()
{
    using This = GeomUtilPlaneMeshGenerator;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    class_<This>("PlaneMeshGenerator", no_init)

        .def("ComputeNumPoints", &This::ComputeNumPoints)
        .staticmethod("ComputeNumPoints")

        .def("GenerateTopology", &This::GenerateTopology)
        .staticmethod("GenerateTopology")

        .def("GeneratePoints", &_WrapGeneratePoints)
        .staticmethod("GeneratePoints")
    ;
}
