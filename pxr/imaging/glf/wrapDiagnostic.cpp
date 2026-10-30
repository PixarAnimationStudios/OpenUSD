//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void wrapDiagnostic(nanobind::module_ &m)
{    
    m.def("RegisterDefaultDebugOutputMessageCallback",
        &GlfRegisterDefaultDebugOutputMessageCallback);

    nb::class_<GlfGLQueryObject>(m, "GLQueryObject")
        .def(nb::init<>())
        .def("Begin", &GlfGLQueryObject::Begin)
        .def("BeginPrimitivesGenerated", &GlfGLQueryObject::BeginPrimitivesGenerated)
        .def("BeginTimeElapsed", &GlfGLQueryObject::BeginTimeElapsed)
        .def("BeginSamplesPassed", &GlfGLQueryObject::BeginSamplesPassed)
        .def("End", &GlfGLQueryObject::End)
        .def("GetResult", &GlfGLQueryObject::GetResult)
        .def("GetResultNoWait", &GlfGLQueryObject::GetResultNoWait)

        ;
}
