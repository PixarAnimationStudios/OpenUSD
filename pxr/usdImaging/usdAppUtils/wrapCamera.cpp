//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/camera.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void
wrapCamera(nanobind::module_ &m)
{
    m.def(
        "GetCameraAtPath",
        UsdAppUtilsGetCameraAtPath,
        nb::arg("stage"), nb::arg("cameraPath"));
}
