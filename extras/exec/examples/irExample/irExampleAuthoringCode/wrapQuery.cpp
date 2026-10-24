//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "query.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapQuery(nb::module_ &m)
{
    using This = InvertibleRigsExample_Query;

    nb::class_<This>(m, "Query")
        .def(nb::init<const UsdStageRefPtr &>())
        .def("FindSwitchAvars", &This::FindSwitchAvars)
    ;
}
