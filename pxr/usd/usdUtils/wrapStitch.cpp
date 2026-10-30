//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapStitch.cpp

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdUtils/stitch.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void 
wrapStitch(nanobind::module_ &m)
{
    m.def("StitchLayers", 
        (void(*)(const SdfLayerHandle&, const SdfLayerHandle&))
            &UsdUtilsStitchLayers,
        nb::arg("strongLayer"), nb::arg("weakLayer"));
    m.def("StitchInfo", 
        (void(*)(const SdfSpecHandle&, const SdfSpecHandle&))
            &UsdUtilsStitchInfo, 
        nb::arg("strongObj"), nb::arg("weakObj"));
}
