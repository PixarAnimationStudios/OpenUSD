//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/type.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapTraits(nb::module_ &m)
{
    m.def("IsFloatingPointType",
        (bool (*)(const TfType&))GfIsFloatingPointType,
        nb::arg("type"),
        "Return True if the TfType represents a floating-point type "
        "(double, float, or GfHalf).");
}
