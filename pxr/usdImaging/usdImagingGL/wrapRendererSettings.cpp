//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include <nanobind/nanobind.h>

#include "pxr/usdImaging/usdImagingGL/rendererSettings.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void 
wrapRendererSettings(nb::module_ &m)
{
    // Wrap the UsdImagingGLRendererSetting::Type enum. Accessible as
    // UsdImagingGL.RendererSettingType.
    nb::enum_<UsdImagingGLRendererSetting::Type>(m, "RendererSettingType")
            .value("FLAG", UsdImagingGLRendererSetting::TYPE_FLAG)
            .value("INT", UsdImagingGLRendererSetting::TYPE_INT)
            .value("FLOAT", UsdImagingGLRendererSetting::TYPE_FLOAT)
            .value("STRING", UsdImagingGLRendererSetting::TYPE_STRING)
    ;

    // Wrap the UsdImagingGLRendererSetting struct.
    // Accessible as UsdImagingGL.RendererSetting
    typedef UsdImagingGLRendererSetting Setting;
    nb::class_<UsdImagingGLRendererSetting>(m, "RendererSetting",
            "Renderer Setting Metadata")
        .def(nb::init<>())
        .def_ro("key", &Setting::key)
        .def_ro("name", &Setting::name)
        .def_ro("type", &Setting::type)
        .def_ro("defValue", &Setting::defValue)
        ;
}
