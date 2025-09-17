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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapMetrics()
{
    def("GetStageUpAxis", UsdGeomGetStageUpAxis, arg("stage"));
    def("SetStageUpAxis", UsdGeomSetStageUpAxis, 
        (arg("stage"), arg("upAxis")));
    def("GetFallbackUpAxis", UsdGeomGetFallbackUpAxis);

    def("GetStageMetersPerUnit", UsdGeomGetStageMetersPerUnit, arg("stage"));
    def("StageHasAuthoredMetersPerUnit", UsdGeomStageHasAuthoredMetersPerUnit, arg("stage"));
    def("SetStageMetersPerUnit", UsdGeomSetStageMetersPerUnit, 
        (arg("stage"), arg("metersPerUnit")));
    def("LinearUnitsAre", UsdGeomLinearUnitsAre, 
        (arg("authoredUnits"), arg("standardUnits"), arg("epsilon")=1e-5));

    pxr_boost::python::class_<UsdGeomLinearUnits>
        cls("LinearUnits", pxr_boost::python::no_init);
    cls
        .def_readonly("nanometers", UsdGeomLinearUnits::nanometers)
        .def_readonly("micrometers", UsdGeomLinearUnits::micrometers)
        .def_readonly("millimeters", UsdGeomLinearUnits::millimeters)
        .def_readonly("centimeters", UsdGeomLinearUnits::centimeters)
        .def_readonly("meters", UsdGeomLinearUnits::meters)
        .def_readonly("kilometers", UsdGeomLinearUnits::kilometers)
        .def_readonly("lightYears", UsdGeomLinearUnits::lightYears)
        .def_readonly("inches", UsdGeomLinearUnits::inches)
        .def_readonly("feet", UsdGeomLinearUnits::feet)
        .def_readonly("yards", UsdGeomLinearUnits::yards)
        .def_readonly("miles", UsdGeomLinearUnits::miles);
}
