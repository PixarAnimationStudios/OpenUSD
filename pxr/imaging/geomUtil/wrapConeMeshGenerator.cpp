//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/coneMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

void wrapConeMeshGenerator(nb::module_& m)
{
    using This = GeomUtilConeMeshGenerator;

    // Pull the constexpr values into variables so boost can odr-use them.
    static constexpr size_t minNumRadial = This::minNumRadial;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    nb::class_<This>(m, "ConeMeshGenerator")
        .def_ro_static("minNumRadial", &minNumRadial)

        .def_static("ComputeNumPoints", &This::ComputeNumPoints,
                    nb::arg("numRadial"), nb::arg("closedSweep") = true)
        .def_static("ComputeNumNormals", &This::ComputeNumNormals,
                    nb::arg("numRadial"), nb::arg("closedSweep") = true)
        .def_static("GetNormalsInterpolation", &This::GetNormalsInterpolation)
        .def_static("GenerateTopology", &This::GenerateTopology,
                    nb::arg("numRadial"), nb::arg("closedSweep") = true)
        .def_static("GeneratePoints", &_WrapGeneratePoints)
        .def_static("GenerateNormals", &_WrapGenerateNormals)
    ;
}