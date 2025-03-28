//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/capsuleMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/vt/types.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static VtVec3fArray
_WrapGeneratePoints(
    const size_t numRadial,
    const size_t numCapAxial,
    const float radius,
    const float height)
{
    const size_t numPoints =
        GeomUtilCapsuleMeshGenerator::ComputeNumPoints(numRadial, numCapAxial);
    if (numPoints == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray points(numPoints);
    GeomUtilCapsuleMeshGenerator::GeneratePoints(
        points.begin(), numRadial, numCapAxial, radius, height);

    return points;
}

static VtVec3fArray
_WrapGenerateNormals(
    const size_t numRadial,
    const size_t numCapAxial,
    const float radius,
    const float height)
{
    const size_t numNormals =
        GeomUtilCapsuleMeshGenerator::ComputeNumNormals(numRadial, numCapAxial);
    if (numNormals == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray normals(numNormals);
    GeomUtilCapsuleMeshGenerator::GenerateNormals(
        normals.begin(), numRadial, numCapAxial, radius, height);

    return normals;
}

void wrapCapsuleMeshGenerator()
{
    using This = GeomUtilCapsuleMeshGenerator;

    // Pull the constexpr values into variables so boost can odr-use them.
    static constexpr size_t minNumRadial = This::minNumRadial;
    static constexpr size_t minNumCapAxial = This::minNumCapAxial;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    class_<This>("CapsuleMeshGenerator", no_init)
        .def_readonly("minNumRadial", minNumRadial)
        .def_readonly("minNumCapAxial", minNumCapAxial)

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