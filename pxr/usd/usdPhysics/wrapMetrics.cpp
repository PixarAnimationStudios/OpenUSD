//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdPhysics/metrics.h"

#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

void wrapMetrics(nanobind::module_ &m)
{
    m.def("GetStageKilogramsPerUnit", UsdPhysicsGetStageKilogramsPerUnit, 
          nb::arg("stage"));
    m.def("StageHasAuthoredKilogramsPerUnit", 
          UsdPhysicsStageHasAuthoredKilogramsPerUnit, nb::arg("stage"));
    m.def("SetStageKilogramsPerUnit", UsdPhysicsSetStageKilogramsPerUnit, 
          nb::arg("stage"), nb::arg("metersPerUnit"));
    m.def("MassUnitsAre", UsdPhysicsMassUnitsAre, 
          nb::arg("authoredUnits"), nb::arg("standardUnits"), nb::arg("epsilon") = 1e-5);

    nb::class_<UsdPhysicsMassUnits> cls(m, "MassUnits");
    cls
        .def_ro_static("kilograms", &UsdPhysicsMassUnits::kilograms)
        .def_ro_static("grams", &UsdPhysicsMassUnits::grams)
        .def_ro_static("slugs", &UsdPhysicsMassUnits::slugs);

}
