//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/converter/from_python.hpp"

#include "pxr/usdImaging/usdImagingGL/rendererSettings.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void 
wrapRendererSettings()
{
    // Wrap the UsdImagingGLRendererSetting::Type enum. Accessible as
    // UsdImagingGL.RendererSettingType.
    enum_<UsdImagingGLRendererSetting::Type>("RendererSettingType")
            .value("FLAG", UsdImagingGLRendererSetting::TYPE_FLAG)
            .value("INT", UsdImagingGLRendererSetting::TYPE_INT)
            .value("FLOAT", UsdImagingGLRendererSetting::TYPE_FLOAT)
            .value("STRING", UsdImagingGLRendererSetting::TYPE_STRING)
    ;

    // Wrap the UsdImagingGLRendererSetting struct.
    // Accessible as UsdImagingGL.RendererSetting
    typedef UsdImagingGLRendererSetting Setting;
    class_<UsdImagingGLRendererSetting>("RendererSetting",
            "Renderer Setting Metadata")
        .add_property("key", make_getter(&Setting::key,
                return_value_policy<return_by_value>()))
        .def_readonly("name", &Setting::name)
        .def_readonly("type", &Setting::type)
        .add_property("defValue", make_getter(&Setting::defValue,
                return_value_policy<return_by_value>()))
        ;
}
