//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/sphereMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

void wrapSphereMeshGenerator(nb::module_& m)
{
    using This = GeomUtilSphereMeshGenerator;

    // Pull the constexpr values into variables so boost can odr-use them.
    static constexpr size_t minNumRadial = This::minNumRadial;
    static constexpr size_t minNumAxial = This::minNumAxial;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    nb::class_<This>(m, "SphereMeshGenerator")
        .def_ro_static("minNumRadial", &minNumRadial)
        .def_ro_static("minNumAxial", &minNumAxial)

        .def_static("ComputeNumPoints", &This::ComputeNumPoints,
                    nb::arg("numRadial"), nb::arg("numAxial"),
                    nb::arg("closedSweep") = true)
        .def_static("ComputeNumNormals", &This::ComputeNumNormals,
                    nb::arg("numRadial"), nb::arg("numAxial"),
                    nb::arg("closedSweep") = true)
        .def_static("GetNormalsInterpolation", &This::GetNormalsInterpolation)
        .def_static("GenerateTopology", &This::GenerateTopology,
                    nb::arg("numRadial"), nb::arg("numAxial"),
                    nb::arg("closedSweep") = true)
        .def_static("GeneratePoints", &_WrapGeneratePoints)
        .def_static("GenerateNormals", &_WrapGenerateNormals)
    ;
}