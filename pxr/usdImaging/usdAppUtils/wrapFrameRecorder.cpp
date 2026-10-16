//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/frameRecorder.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void
wrapFrameRecorder(nanobind::module_ &m)
{
    using This = UsdAppUtilsFrameRecorder;

    nb::class_<This>(m, "FrameRecorder")
        .def(nb::init<>())
        .def(nb::init<const TfToken&, bool, bool>(),
             nb::arg("rendererPluginId") = TfToken(),
             nb::arg("gpuEnabled") = true,
             nb::arg("drawModeEnabled") = true)
        .def("GetCurrentRendererId", &This::GetCurrentRendererId)
        .def("SetActiveRenderPassPrimPath",
            &This::SetActiveRenderPassPrimPath)
        .def("SetActiveRenderSettingsPrimPath",
            &This::SetActiveRenderSettingsPrimPath)
        .def("SetRendererPlugin", &This::SetRendererPlugin)
        .def("SetImageWidth", &This::SetImageWidth)
        .def("SetCameraLightEnabled", &This::SetCameraLightEnabled)
        .def("SetDomeLightVisibility", &This::SetDomeLightVisibility)
        .def("SetComplexity", &This::SetComplexity)
        .def("SetColorCorrectionMode", &This::SetColorCorrectionMode)
        .def("SetPrimaryCameraPrimPath", &This::SetPrimaryCameraPrimPath)
        .def("SetIncludedPurposes", &This::SetIncludedPurposes,
             nb::arg("purposes"))
        .def(
            "Record",
            &This::Record,
            nb::arg("stage"),
            nb::arg("usdCamera"),
            nb::arg("timeCode"),
            nb::arg("outputImagePath"))
    ;
}
