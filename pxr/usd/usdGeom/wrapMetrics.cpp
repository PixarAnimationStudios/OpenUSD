//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/metrics.h"

#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

void wrapMetrics(nanobind::module_ &m)
{
    m.def("GetStageUpAxis", UsdGeomGetStageUpAxis, nb::arg("stage"));
    m.def("SetStageUpAxis", UsdGeomSetStageUpAxis,
          nb::arg("stage"), nb::arg("upAxis"));
    m.def("GetFallbackUpAxis", UsdGeomGetFallbackUpAxis);

    m.def("GetStageMetersPerUnit", UsdGeomGetStageMetersPerUnit, nb::arg("stage"));
    m.def("StageHasAuthoredMetersPerUnit", UsdGeomStageHasAuthoredMetersPerUnit, nb::arg("stage"));
    m.def("SetStageMetersPerUnit", UsdGeomSetStageMetersPerUnit,
          nb::arg("stage"), nb::arg("metersPerUnit"));
    m.def("LinearUnitsAre", UsdGeomLinearUnitsAre,
          nb::arg("authoredUnits"), nb::arg("standardUnits"), nb::arg("epsilon") = 1e-5);

    nb::class_<UsdGeomLinearUnits> cls(m, "LinearUnits");
    cls
        .def_ro_static("nanometers", &UsdGeomLinearUnits::nanometers)
        .def_ro_static("micrometers", &UsdGeomLinearUnits::micrometers)
        .def_ro_static("millimeters", &UsdGeomLinearUnits::millimeters)
        .def_ro_static("centimeters", &UsdGeomLinearUnits::centimeters)
        .def_ro_static("decimeters", &UsdGeomLinearUnits::decimeters)
        .def_ro_static("meters", &UsdGeomLinearUnits::meters)
        .def_ro_static("kilometers", &UsdGeomLinearUnits::kilometers)
        .def_ro_static("lightYears", &UsdGeomLinearUnits::lightYears)
        .def_ro_static("inches", &UsdGeomLinearUnits::inches)
        .def_ro_static("feet", &UsdGeomLinearUnits::feet)
        .def_ro_static("yards", &UsdGeomLinearUnits::yards)
        .def_ro_static("miles", &UsdGeomLinearUnits::miles);

}
