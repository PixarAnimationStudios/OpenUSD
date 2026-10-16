//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "authoring.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapAuthoring(nb::module_ &m)
{
    using This = InvertibleRigsExample_Authoring;

    nb::class_<This>(m, "Authoring")
        .def(nb::init<const UsdStageRefPtr &>())
        .def("BreakdownInputAvars", &This::BreakdownInputAvars)
        .def("CompensateSwitch", &This::CompensateSwitch)
    ;
}
