//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdLod/screenSizeHeuristicQuery.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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
nb::tuple
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
    return nb::make_tuple(lod, sizeOut);
}

void wrapScreenSizeHeuristicQuery(nb::module_ &m)
{
    typedef UsdLodScreenSizeHeuristicQuery This;

    nb::class_<This, UsdLodHeuristicQuery>(m, "ScreenSizeHeuristicQuery")
        .def(nb::init<>())
        .def(nb::init<const UsdLodScreenSizeHeuristicQuery&>())
        .def("__init__",
             +[](This* self,
                 const std::string& lodDomain,
                 const GfRange3d& extent,
                 const std::string& projectionMethod,
                 const VtFloatArray& thresholds,
                 const VtFloatArray& blendThresholds) {
                 new (self) This(TfToken(lodDomain), extent,
                                 TfToken(projectionMethod),
                                 thresholds, blendThresholds);
             },
             nb::arg("lodDomain"),
             nb::arg("extent") = GfRange3d(),
             nb::arg("projectionMethod") = std::string(),
             nb::arg("thresholds") = VtFloatArray(),
             nb::arg("blendThresholds") = VtFloatArray())

        .def_rw("extent", &UsdLodScreenSizeHeuristicQuery::extent)
        .def_prop_rw(
            "projectionMethod",
            +[](const This& self) {
                 return self.projectionMethod.GetString();
             },
            +[](This& self, const std::string& value) {
                 self.projectionMethod = TfToken(value);
             })
        .def_rw("thresholds", &UsdLodScreenSizeHeuristicQuery::thresholds)
        .def_rw("blendThresholds", &UsdLodScreenSizeHeuristicQuery::blendThresholds)

        .def("ComputeScreenSize",
             &_WrapComputeScreenSize1,
             nb::arg("frustum"),
             nb::arg("transform"))
        .def("ComputeScreenSize",
             &_WrapComputeScreenSize2,
             nb::arg("frustum"),
             nb::arg("transform"),
             nb::arg("prevSize"),
             nb::arg("hysteresis"))
        .def("ComputeLOD", &_WrapComputeLOD1,
             nb::arg("size"))
        .def("ComputeLOD", &_WrapComputeLOD2,
             nb::arg("frustum"),
             nb::arg("transform"))
        .def("ComputeLOD", &_WrapComputeLOD3,
             nb::arg("frustum"),
             nb::arg("transform"),
             nb::arg("prevSize"),
             nb::arg("hysteresis"))
        ;
}
