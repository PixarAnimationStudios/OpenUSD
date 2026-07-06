//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/distanceHeuristicQuery.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/tuple.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
pxr_boost::python::tuple
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
    return pxr_boost::python::make_tuple(lodIndex, distanceOut);
}

static
UsdLodDistanceHeuristicQuery*
_WrapInit(const std::string& lodDomain,
          const GfVec3f& center,
          const GfRange3d& extent,
          const VtFloatArray& thresholds,
          const VtFloatArray& blendThresholds)
{
    return new UsdLodDistanceHeuristicQuery(
        TfToken(lodDomain), center, extent, thresholds, blendThresholds);
}

void wrapDistanceHeuristicQuery()
{
    typedef UsdLodDistanceHeuristicQuery This;

    class_<This, bases<UsdLodHeuristicQuery>>("DistanceHeuristicQuery")

        .def("__init__",
             make_constructor(&_WrapInit,
                              default_call_policies(),
                              (arg("lodDomain"),
                               arg("center") = GfVec3f(0.0, 0.0, 0.0),
                               arg("extent") = GfRange3d(),
                               arg("thresholds") = VtFloatArray(),
                               arg("blendThresholds") = VtFloatArray())))
        .def(init<>())

        .def(init<const UsdLodDistanceHeuristicQuery&>())

        .def_readwrite(
            "center", &UsdLodDistanceHeuristicQuery::center)
        .def_readwrite(
            "extent", &UsdLodDistanceHeuristicQuery::extent)
        .def_readwrite(
            "thresholds", &UsdLodDistanceHeuristicQuery::thresholds)
        .def_readwrite(
            "blendThresholds", &UsdLodDistanceHeuristicQuery::blendThresholds)

        .def("ComputeDistance", &_WrapComputeDistance1,
             (arg("viewpoint"),
              arg("transform")))
        .def("ComputeDistance", &_WrapComputeDistance2,
             (arg("viewpoint"),
              arg("transform"),
              arg("prevDistance"),
              arg("hysteresis")))
        .def("ComputeLOD", &_WrapComputeLOD1,
             arg("distance"))
        .def("ComputeLOD", &_WrapComputeLOD2,
             (arg("viewpoint"),
              arg("transform")))
        .def("ComputeLOD", &_WrapComputeLOD3,
             (arg("viewpoint"),
              arg("transform"),
              arg("prevDistance"),
              arg("hysteresis")))
        ;
}
