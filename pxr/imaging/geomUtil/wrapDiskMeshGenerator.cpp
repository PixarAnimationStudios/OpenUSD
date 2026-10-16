//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/diskMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static VtVec3fArray
_WrapGeneratePoints(
    const size_t numRadial,
    const float radius)
{
    const size_t numPoints =
        GeomUtilDiskMeshGenerator::ComputeNumPoints(numRadial);
    if (numPoints == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray points(numPoints);
    GeomUtilDiskMeshGenerator::GeneratePoints(
        points.begin(), numRadial, radius);

    return points;
}

static VtVec3fArray
_WrapGenerateNormals()
{
    const size_t numNormals =
        GeomUtilDiskMeshGenerator::ComputeNumNormals();
    if (numNormals == 0) {
        return VtVec3fArray();
    }

    VtVec3fArray normals(numNormals);
    GeomUtilDiskMeshGenerator::GenerateNormals(
        normals.begin());

    return normals;
}

void wrapDiskMeshGenerator(nb::module_& m)
{
    using This = GeomUtilDiskMeshGenerator;

    // Pull the constexpr values into variables so boost can odr-use them.
    static constexpr size_t minNumRadial = This::minNumRadial;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    nb::class_<This>(m, "DiskMeshGenerator")
        .def_ro_static("minNumRadial", &minNumRadial)

        .def_static("ComputeNumPoints", &This::ComputeNumPoints,
                    nb::arg("numRadial"), nb::arg("closedSweep") = true)
        .def_static("ComputeNumNormals", &This::ComputeNumNormals)
        .def_static("GetNormalsInterpolation", &This::GetNormalsInterpolation)
        .def_static("GenerateTopology", &This::GenerateTopology,
                    nb::arg("numRadial"), nb::arg("closedSweep") = true)
        .def_static("GeneratePoints", &_WrapGeneratePoints)
        .def_static("GenerateNormals", &_WrapGenerateNormals)
    ;
}
