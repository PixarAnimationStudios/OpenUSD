//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/usdMtlx/backdoor.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;
PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdMtlxBackdoor(nanobind::module_ &m)
{
    m.def("_TestString", UsdMtlx_TestString,
          nb::arg("buffer"), nb::arg("nodeGraphs") = false);
    m.def("_TestFile", UsdMtlx_TestFile,
          nb::arg("pathname"), nb::arg("nodeGraphs") = false);
}
