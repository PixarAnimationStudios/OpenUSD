//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/sphereMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/vt/types.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static VtVec3fArray
_WrapGeneratePoints(
    const size_t numRadial,
    const size_t numAxial,
    const float radius)
{
    const size_t numPoints =
        GeomUtilSphereMeshGenerator::ComputeNumPoints(numRadial, numAxial);
    if (numPoints == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray points(numPoints);
    GeomUtilSphereMeshGenerator::GeneratePoints(
        points.begin(), numRadial, numAxial, radius);

    return points;
}

static VtVec3fArray
_WrapGenerateNormals(
    const size_t numRadial,
    const size_t numAxial)
{
    const size_t numNormals =
        GeomUtilSphereMeshGenerator::ComputeNumNormals(numRadial, numAxial);
    if (numNormals == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray normals(numNormals);
    GeomUtilSphereMeshGenerator::GenerateNormals(
        normals.begin(), numRadial, numAxial);

    return normals;
}

void wrapSphereMeshGenerator()
{
    using This = GeomUtilSphereMeshGenerator;

    // Pull the constexpr values into variables so boost can odr-use them.
    static constexpr size_t minNumRadial = This::minNumRadial;
    static constexpr size_t minNumAxial = This::minNumAxial;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    class_<This>("SphereMeshGenerator", no_init)
        .def_readonly("minNumRadial", minNumRadial)
        .def_readonly("minNumAxial", minNumAxial)

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