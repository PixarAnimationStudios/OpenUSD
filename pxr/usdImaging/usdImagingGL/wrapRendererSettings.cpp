//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python.hpp>
#include <boost/python/converter/from_python.hpp>

#include "pxr/usdImaging/usdImagingGL/rendererSettings.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
