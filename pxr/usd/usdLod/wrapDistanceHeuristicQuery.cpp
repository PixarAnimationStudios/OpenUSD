//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/distanceHeuristicQuery.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static
double
_WrapComputeDistance1(const UsdLodDistanceHeuristicQuery& query,
                 const GfVec3d& viewpoint,
                 const GfMatrix4d& transform)
{
    return query.ComputeDistance(viewpoint, transform);
}

static
double
_WrapComputeDistance2(const UsdLodDistanceHeuristicQuery& query,
                 const GfVec3d& viewpoint,
                 const GfMatrix4d& transform,
                 const double prevDistance,
                 const double hysteresis)
{
    return query.ComputeDistance(viewpoint, transform,
                                 prevDistance, hysteresis);
}

static
double
_WrapComputeLOD1(const UsdLodDistanceHeuristicQuery& query,
                 double distance)
{
    return query.ComputeLOD(distance);
}

static
double
_WrapComputeLOD2(const UsdLodDistanceHeuristicQuery& query,
                 const GfVec3d& viewpoint,
                 const GfMatrix4d& transform)
{
    return query.ComputeLOD(viewpoint, transform);
}

static
nb::tuple
_WrapComputeLOD3(const UsdLodDistanceHeuristicQuery& query,
                 const GfVec3d& viewpoint,
                 const GfMatrix4d& transform,
                 const double prevDistance,
                 const double hysteresis)
{
    double distanceOut;
    double lodIndex = query.ComputeLOD(viewpoint, transform,
                                       prevDistance, hysteresis,
                                       &distanceOut);
    return nb::make_tuple(lodIndex, distanceOut);
}

void wrapDistanceHeuristicQuery(nb::module_ &m)
{
    typedef UsdLodDistanceHeuristicQuery This;

    nb::class_<This, UsdLodHeuristicQuery>(m, "DistanceHeuristicQuery")
        .def(nb::init<>())
        .def(nb::init<const UsdLodDistanceHeuristicQuery&>())
        .def("__init__",
             +[](This* self,
                 const std::string& lodDomain,
                 const GfVec3f& center,
                 const GfRange3d& extent,
                 const VtFloatArray& thresholds,
                 const VtFloatArray& blendThresholds) {
                 new (self) This(TfToken(lodDomain), center, extent,
                                 thresholds, blendThresholds);
             },
             nb::arg("lodDomain"),
             nb::arg("center") = GfVec3f(0.0, 0.0, 0.0),
             nb::arg("extent") = GfRange3d(),
             nb::arg("thresholds") = VtFloatArray(),
             nb::arg("blendThresholds") = VtFloatArray())

        .def_rw("center", &UsdLodDistanceHeuristicQuery::center)
        .def_rw("extent", &UsdLodDistanceHeuristicQuery::extent)
        .def_rw("thresholds", &UsdLodDistanceHeuristicQuery::thresholds)
        .def_rw("blendThresholds", &UsdLodDistanceHeuristicQuery::blendThresholds)

        .def("ComputeDistance", &_WrapComputeDistance1,
             nb::arg("viewpoint"),
             nb::arg("transform"))
        .def("ComputeDistance", &_WrapComputeDistance2,
             nb::arg("viewpoint"),
             nb::arg("transform"),
             nb::arg("prevDistance"),
             nb::arg("hysteresis"))
        .def("ComputeLOD", &_WrapComputeLOD1,
             nb::arg("distance"))
        .def("ComputeLOD", &_WrapComputeLOD2,
             nb::arg("viewpoint"),
             nb::arg("transform"))
        .def("ComputeLOD", &_WrapComputeLOD3,
             nb::arg("viewpoint"),
             nb::arg("transform"),
             nb::arg("prevDistance"),
             nb::arg("hysteresis"))
        ;
}
