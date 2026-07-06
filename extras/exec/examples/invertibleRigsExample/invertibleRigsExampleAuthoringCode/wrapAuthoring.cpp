//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "authoring.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/noncopyable.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapAuthoring()
{
    using This = InvertibleRigsExample_Authoring;

    class_<This, noncopyable>("Authoring", no_init)
        .def(init<const UsdStageRefPtr &>())
        .def("BreakdownInputAvars", &This::BreakdownInputAvars)
        .def("CompensateSwitch", &This::CompensateSwitch)
    ;
}
