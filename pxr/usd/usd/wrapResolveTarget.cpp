//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/resolveTarget.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdResolveTarget(nb::module_ &m)
{
    nb::class_<UsdResolveTarget>(m, "ResolveTarget")
        .def(nb::init<>())
        .def("GetPrimIndex", &UsdResolveTarget::GetPrimIndex, 
             nb::rv_policy::copy)
        .def("GetStartNode", &UsdResolveTarget::GetStartNode)
        .def("GetStartLayer", &UsdResolveTarget::GetStartLayer)
        .def("GetStopNode", &UsdResolveTarget::GetStopNode)
        .def("GetStopLayer", &UsdResolveTarget::GetStopLayer)
        .def("IsNull", &UsdResolveTarget::IsNull)
        ;
}
