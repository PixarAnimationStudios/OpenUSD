//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/resolveTarget.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdResolveTarget()
{
    class_<UsdResolveTarget>("ResolveTarget")
        .def(init<>())
        .def("GetPrimIndex", &UsdResolveTarget::GetPrimIndex, 
             return_value_policy<return_by_value>())
        .def("GetStartNode", &UsdResolveTarget::GetStartNode)
        .def("GetStartLayer", &UsdResolveTarget::GetStartLayer)
        .def("GetStopNode", &UsdResolveTarget::GetStopNode)
        .def("GetStopLayer", &UsdResolveTarget::GetStopLayer)
        .def("IsNull", &UsdResolveTarget::IsNull)
        ;
}
