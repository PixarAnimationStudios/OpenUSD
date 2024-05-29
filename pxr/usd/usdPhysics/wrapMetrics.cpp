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

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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

    boost::python::class_<UsdPhysicsMassUnits>
        cls("MassUnits", boost::python::no_init);
    cls
        .def_readonly("kilograms", UsdPhysicsMassUnits::kilograms)
        .def_readonly("grams", UsdPhysicsMassUnits::grams)
        .def_readonly("slugs", UsdPhysicsMassUnits::slugs);
}
