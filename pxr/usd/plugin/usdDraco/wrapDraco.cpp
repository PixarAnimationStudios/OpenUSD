//
// Copyright 2019 Google LLC
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "writer.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdDraco(nb::module_ &m)
{
    m.def("_WriteDraco", UsdDraco_WriteDraco,
          nb::arg("mesh"),
          nb::arg("fileName"),
          nb::arg("qp"),
          nb::arg("qt"),
          nb::arg("qn"),
          nb::arg("cl"),
          nb::arg("preservePolygons"),
          nb::arg("preservePositionOrder"),
          nb::arg("preserveHoles"));
    m.def("_PrimvarSupported", UsdDraco_PrimvarSupported,
          nb::arg("primvar"));
}
