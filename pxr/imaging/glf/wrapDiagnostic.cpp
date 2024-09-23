//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapDiagnostic()
{    
    def("RegisterDefaultDebugOutputMessageCallback",
        &GlfRegisterDefaultDebugOutputMessageCallback);

    class_<GlfGLQueryObject, boost::noncopyable>("GLQueryObject")
        .def("Begin", &GlfGLQueryObject::Begin)
        .def("BeginPrimitivesGenerated", &GlfGLQueryObject::BeginPrimitivesGenerated)
        .def("BeginTimeElapsed", &GlfGLQueryObject::BeginTimeElapsed)
        .def("BeginSamplesPassed", &GlfGLQueryObject::BeginSamplesPassed)
        .def("End", &GlfGLQueryObject::End)
        .def("GetResult", &GlfGLQueryObject::GetResult)
        .def("GetResultNoWait", &GlfGLQueryObject::GetResultNoWait)

        ;
}
