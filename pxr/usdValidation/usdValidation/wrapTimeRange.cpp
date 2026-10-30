//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

#include "pxr/pxr.h"
#include "pxr/base/gf/interval.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/usdValidation/usdValidation/timeRange.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapUsdValidationTimeRange(nb::module_ &m)
{
    nb::class_<UsdValidationTimeRange>(m, "ValidationTimeRange")
        .def(nb::init<>())
        .def(nb::init<const UsdTimeCode &>(),
             nb::arg("timeCode"))
        .def(nb::init<const GfInterval &, bool>(),
             nb::arg("interval"), nb::arg("includeTimeCodeDefault") = false)
        .def("IncludesTimeCodeDefault", 
             &UsdValidationTimeRange::IncludesTimeCodeDefault)
        .def("GetInterval", &UsdValidationTimeRange::GetInterval);
}
