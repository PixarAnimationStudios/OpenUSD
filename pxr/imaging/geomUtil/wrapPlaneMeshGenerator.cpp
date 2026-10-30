//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/geomUtil/planeMeshGenerator.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

void wrapPlaneMeshGenerator(nb::module_& m)
{
    using This = GeomUtilPlaneMeshGenerator;

    // Note: These are only "classes" for name scoping, and are uninstantiable;
    // hence no need to bother declaring bases.
    nb::class_<This>(m, "PlaneMeshGenerator")
        .def_static("ComputeNumPoints", &This::ComputeNumPoints)
        .def_static("ComputeNumNormals", &This::ComputeNumNormals)
        .def_static("GetNormalsInterpolation", &This::GetNormalsInterpolation)
        .def_static("GenerateTopology", &This::GenerateTopology)
        .def_static("GeneratePoints", &_WrapGeneratePoints)
        .def_static("GenerateNormals", &_WrapGenerateNormals)
    ;
}
