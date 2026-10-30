//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/capsuleMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

void wrapCapsuleMeshGenerator(nb::module_& m)
{
    using This = GeomUtilCapsuleMeshGenerator;

    // Pull the constexpr values into variables so boost can odr-use them.
    static constexpr size_t minNumRadial = This::minNumRadial;
    static constexpr size_t minNumCapAxial = This::minNumCapAxial;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    nb::class_<This>(m, "CapsuleMeshGenerator")
        .def_ro_static("minNumRadial", &minNumRadial)
        .def_ro_static("minNumCapAxial", &minNumCapAxial)

        .def_static("ComputeNumPoints", &This::ComputeNumPoints,
                    nb::arg("numRadial"), nb::arg("numCapAxial"),
                    nb::arg("closedSweep") = true)
        .def_static("ComputeNumNormals", &This::ComputeNumNormals,
                    nb::arg("numRadial"), nb::arg("numCapAxial"),
                    nb::arg("closedSweep") = true)
        .def_static("GetNormalsInterpolation", &This::GetNormalsInterpolation)
        .def_static("GenerateTopology", &This::GenerateTopology,
                    nb::arg("numRadial"), nb::arg("numCapAxial"),
                    nb::arg("closedSweep") = true)
        .def_static("GeneratePoints", &_WrapGeneratePoints)
        .def_static("GenerateNormals", &_WrapGenerateNormals)
    ;
}