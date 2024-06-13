//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usdImaging/usdAppUtils/frameRecorder.h"

#include <boost/python.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/scope.hpp>

using namespace boost::python;


PXR_NAMESPACE_USING_DIRECTIVE


void
wrapFrameRecorder()
{
    using This = UsdAppUtilsFrameRecorder;

    scope s = class_<This, boost::noncopyable>("FrameRecorder")
        .def(init<>())
        .def(init<const TfToken&, bool>(
            (arg("rendererPluginId") = TfToken(), 
             arg("gpuEnabled") = true)))
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
        .def("SetIncludedPurposes", &This::SetIncludedPurposes,
             (arg("purposes")))
        .def(
            "Record",
            &This::Record,
            (arg("stage"),
             arg("usdCamera"),
             arg("timeCode"),
             arg("outputImagePath")))
    ;
}
