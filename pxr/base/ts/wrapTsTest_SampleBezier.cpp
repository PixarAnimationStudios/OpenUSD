//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_SampleBezier.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE


void wrapTsTest_SampleBezier(nb::module_ &m)
{
    m.def("TsTest_SampleBezier", &TsTest_SampleBezier,
          nb::arg("splineData"),
          nb::arg("numSamples"),
          nb::rv_policy::copy);
}
