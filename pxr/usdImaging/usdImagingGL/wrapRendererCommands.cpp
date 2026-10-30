//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/imaging/hd/command.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

using UsdImagingGLRendererCommandArgDescriptor = HdCommandArgDescriptor;

using UsdImagingGLRendererCommandDescriptor = HdCommandDescriptor;

void 
wrapRendererCommands(nb::module_ &m)
{
    // Wrap the UsdImagingGLRendererCommandArgDescriptor struct.
    // Accessible as UsdImagingGL.RendererCommandArgDescriptor
    nb::class_<UsdImagingGLRendererCommandArgDescriptor>(
            m, "RendererCommandArgDescriptor",
            "Renderer Command Argument Metadata")
        .def_ro("argName", 
                &UsdImagingGLRendererCommandArgDescriptor::argName)

        .def_ro("defaultValue",
                &UsdImagingGLRendererCommandArgDescriptor::defaultValue)
        ;

    // Wrap the UsdImagingGLRendererCommandDescriptor struct.
    // Accessible as UsdImagingGL.RendererCommandDescriptor
    nb::class_<UsdImagingGLRendererCommandDescriptor>(
            m, "RendererCommandDescriptor",
            "Renderer Command Metadata")
        .def_ro("commandName", 
                &UsdImagingGLRendererCommandDescriptor::commandName)

        .def_ro("commandDescription",
                &UsdImagingGLRendererCommandDescriptor::commandDescription)

        .def_ro("commandArgs",
                &UsdImagingGLRendererCommandDescriptor::commandArgs)
        ;
}
