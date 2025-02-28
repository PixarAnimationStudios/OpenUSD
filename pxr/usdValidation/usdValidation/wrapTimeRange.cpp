//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/interval.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/usdValidation/usdValidation/timeRange.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/object.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapUsdValidationTimeRange()
{
    class_<UsdValidationTimeRange>("ValidationTimeRange")
        .def(init<const UsdTimeCode &>(
            args("timeCode")))
        .def(init<const GfInterval &, bool>(
            (args("interval"), args("includeTimeCodeDefault") = false)))
        .def("IncludesTimeCodeDefault", 
             &UsdValidationTimeRange::IncludesTimeCodeDefault)
        .def("GetInterval", &UsdValidationTimeRange::GetInterval);
}
