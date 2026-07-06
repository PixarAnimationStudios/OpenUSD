//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/screenSizeHeuristicQuery.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/tuple.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static
double
_WrapComputeScreenSize1(const UsdLodScreenSizeHeuristicQuery& query,
                        const GfFrustum& frustum,
                        const GfMatrix4d& transform)
{
    return query.ComputeScreenSize(frustum, transform);
}

static
double
_WrapComputeScreenSize2(const UsdLodScreenSizeHeuristicQuery& query,
                        const GfFrustum& frustum,
                        const GfMatrix4d& transform,
                        const double prevSize,
                        const double hysteresis)
{
    return query.ComputeScreenSize(frustum, transform,
                                   prevSize, hysteresis);
}

static
double
_WrapComputeLOD1(const UsdLodScreenSizeHeuristicQuery& query,
                 double size)
{
    return query.ComputeLOD(size);
}

static
double
_WrapComputeLOD2(const UsdLodScreenSizeHeuristicQuery& query,
                 const GfFrustum& frustum,
                 const GfMatrix4d& transform)
{
    return query.ComputeLOD(frustum, transform);
}

static
pxr_boost::python::tuple
_WrapComputeLOD3(const UsdLodScreenSizeHeuristicQuery& query,
                 const GfFrustum& frustum,
                 const GfMatrix4d& transform,
                 const double prevSize,
                 const double hysteresis)
{
    double sizeOut;
    double lod = query.ComputeLOD(frustum, transform,
                                  prevSize, hysteresis,
                                  &sizeOut);
    return pxr_boost::python::make_tuple(lod, sizeOut);
}

static
UsdLodScreenSizeHeuristicQuery*
_WrapInit(const std::string& lodDomain,
          const GfRange3d& extent,
          const std::string& projectionMethod,
          const VtFloatArray& thresholds,
          const VtFloatArray& blendThresholds)
{
    return new UsdLodScreenSizeHeuristicQuery(
        TfToken(lodDomain), extent, TfToken(projectionMethod),
        thresholds, blendThresholds);
}

void wrapScreenSizeHeuristicQuery()
{
    typedef UsdLodScreenSizeHeuristicQuery This;

    class_<This, bases<UsdLodHeuristicQuery>>("ScreenSizeHeuristicQuery")

        .def("__init__",
             make_constructor(&_WrapInit,
                              default_call_policies(),
                              (arg("lodDomain"),
                               arg("extent") = GfRange3d(),
                               arg("projectionMethod") = std::string(),
                               arg("thresholds") = VtFloatArray(),
                               arg("blendThresholds") = VtFloatArray())))
        .def(init<>())

        .def(init<const UsdLodScreenSizeHeuristicQuery&>())

        .def_readwrite(
            "extent", &UsdLodScreenSizeHeuristicQuery::extent)
        .add_property(
            "projectionMethod",
            +[](const This& self) {
                 return self.projectionMethod.GetString();
             },
            +[](This& self, const std::string& value) {
                 self.projectionMethod = TfToken(value);
             })
        .def_readwrite(
            "thresholds", &UsdLodScreenSizeHeuristicQuery::thresholds)
        .def_readwrite(
            "blendThresholds",
            &UsdLodScreenSizeHeuristicQuery::blendThresholds)

        .def("ComputeScreenSize",
             &_WrapComputeScreenSize1,
             (arg("frustum"),
              arg("transform")))
        .def("ComputeScreenSize",
             &_WrapComputeScreenSize2,
             (arg("frustum"),
              arg("transform"),
              arg("prevSize"),
              arg("hysteresis")))
        .def("ComputeLOD", &_WrapComputeLOD1,
             arg("size"))
        .def("ComputeLOD", &_WrapComputeLOD2,
             (arg("frustum"),
              arg("transform")))
        .def("ComputeLOD", &_WrapComputeLOD3,
             (arg("frustum"),
              arg("transform"),
              arg("prevSize"),
              arg("hysteresis")))
        ;
}
