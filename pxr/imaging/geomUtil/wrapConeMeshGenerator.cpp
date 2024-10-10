//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/coneMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/vt/types.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static VtVec3fArray
_WrapGeneratePoints(
    const size_t numRadial,
    const float radius,
    const float height)
{
    const size_t numPoints =
        GeomUtilConeMeshGenerator::ComputeNumPoints(numRadial);
    if (numPoints == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray points(numPoints);
    GeomUtilConeMeshGenerator::GeneratePoints(
        points.begin(), numRadial, radius, height);

    return points;
}

static VtVec3fArray
_WrapGenerateNormals(
    const size_t numRadial,
    const float radius,
    const float height)
{
    const size_t numNormals =
        GeomUtilConeMeshGenerator::ComputeNumNormals(numRadial);
    if (numNormals == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray normals(numNormals);
    GeomUtilConeMeshGenerator::GenerateNormals(
        normals.begin(), numRadial, radius, height);

    return normals;
}

void wrapConeMeshGenerator()
{
    using This = GeomUtilConeMeshGenerator;

    // Pull the constexpr values into variables so boost can odr-use them.
    static constexpr size_t minNumRadial = This::minNumRadial;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    class_<This>("ConeMeshGenerator", no_init)
        .def_readonly("minNumRadial", minNumRadial)

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