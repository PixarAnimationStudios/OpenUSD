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
#include <boost/python/converter/from_python.hpp>

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/imaging/hd/command.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

using UsdImagingGLRendererCommandArgDescriptor = HdCommandArgDescriptor;

using UsdImagingGLRendererCommandDescriptor = HdCommandDescriptor;

void 
wrapRendererCommands()
{
    // Wrap the UsdImagingGLRendererCommandArgDescriptor struct.
    // Accessible as UsdImagingGL.RendererCommandArgDescriptor
    class_<UsdImagingGLRendererCommandArgDescriptor>(
            "RendererCommandArgDescriptor",
            "Renderer Command Argument Metadata", no_init)
        .add_property("argName", 
                make_getter(
                    &UsdImagingGLRendererCommandArgDescriptor::argName,
                    return_value_policy<return_by_value>()))

        .def_readonly("defaultValue",
                  &UsdImagingGLRendererCommandArgDescriptor::defaultValue)
        ;



    // Wrap the UsdImagingGLRendererCommandDescriptor struct.
    // Accessible as UsdImagingGL.RendererCommandDescriptor
    class_<UsdImagingGLRendererCommandDescriptor>("RendererCommandDescriptor",
            "Renderer Command Metadata", no_init)
        .add_property("commandName", 
                make_getter(
                    &UsdImagingGLRendererCommandDescriptor::commandName,
                    return_value_policy<return_by_value>()))

        .def_readonly("commandDescription",
                  &UsdImagingGLRendererCommandDescriptor::commandDescription)

        .def_readonly("commandArgs",
                  &UsdImagingGLRendererCommandDescriptor::commandArgs)
        ;
}
