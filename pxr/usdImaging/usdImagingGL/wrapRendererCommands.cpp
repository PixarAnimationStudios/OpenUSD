//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python/converter/from_python.hpp"

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/imaging/hd/command.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
