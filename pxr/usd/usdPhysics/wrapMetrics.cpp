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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapMetrics()
{
    def("GetStageKilogramsPerUnit", UsdPhysicsGetStageKilogramsPerUnit, 
            arg("stage"));
    def("StageHasAuthoredKilogramsPerUnit", 
            UsdPhysicsStageHasAuthoredKilogramsPerUnit, arg("stage"));
    def("SetStageKilogramsPerUnit", UsdPhysicsSetStageKilogramsPerUnit, 
        (arg("stage"), arg("metersPerUnit")));
    def("MassUnitsAre", UsdPhysicsMassUnitsAre, 
        (arg("authoredUnits"), arg("standardUnits"), arg("epsilon")=1e-5));

    pxr_boost::python::class_<UsdPhysicsMassUnits>
        cls("MassUnits", pxr_boost::python::no_init);
    cls
        .def_readonly("kilograms", UsdPhysicsMassUnits::kilograms)
        .def_readonly("grams", UsdPhysicsMassUnits::grams)
        .def_readonly("slugs", UsdPhysicsMassUnits::slugs);
}
