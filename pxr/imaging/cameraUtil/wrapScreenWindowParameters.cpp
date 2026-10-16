//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file camera/wrapScreenWindowParameters.h

#include "pxr/imaging/cameraUtil/screenWindowParameters.h"

#include "pxr/base/tf/pyNanobindBridge.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void
wrapScreenWindowParameters(nb::module_& m)
{
    nb::class_<CameraUtilScreenWindowParameters>(m, "ScreenWindowParameters")
        .def(nb::init<const GfCamera&>())
        .def_prop_ro("screenWindow",
                     &CameraUtilScreenWindowParameters::GetScreenWindow)
        .def_prop_ro("fieldOfView",
                     &CameraUtilScreenWindowParameters::GetFieldOfView)
        .def_prop_ro("zFacingViewMatrix",
                     &CameraUtilScreenWindowParameters::GetZFacingViewMatrix);
}
