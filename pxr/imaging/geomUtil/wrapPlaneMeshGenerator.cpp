//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/planeMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/vt/types.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

static VtVec3fArray
_WrapGenerateNormals()
{
    const size_t numNormals =
        GeomUtilPlaneMeshGenerator::ComputeNumNormals();
    if (numNormals == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray normals(numNormals);
    GeomUtilPlaneMeshGenerator::GenerateNormals(
        normals.begin());

    return normals;
}

void wrapPlaneMeshGenerator()
{
    using This = GeomUtilPlaneMeshGenerator;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    class_<This>("PlaneMeshGenerator", no_init)

        .def("ComputeNumPoints", &This::ComputeNumPoints)
        .staticmethod("ComputeNumPoints")

        .def("ComputeNumNormals", &This::ComputeNumNormals)
        .staticmethod("ComputeNumNormals")

        .def("GetNormalsInterpolation", &This::GetNormalsInterpolation)
        .staticmethod("GetNormalsInterpolation")

        .def("GenerateTopology", &This::GenerateTopology)
        .staticmethod("GenerateTopology")

        .def("GeneratePoints", &_WrapGeneratePoints)
        .staticmethod("GeneratePoints")

        .def("GenerateNormals", &_WrapGenerateNormals)
        .staticmethod("GenerateNormals")
    ;
}
