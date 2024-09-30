//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyEnum.h"

#include "pxr/external/boost/python/bases.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/overloads.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

static GlfDrawTargetRefPtr _NewDrawTarget(
    GfVec2i const & size)
{
    return GlfDrawTarget::New(size);
}

static GlfDrawTargetRefPtr _NewDrawTarget2(
    unsigned int width, unsigned int height)
{
    return GlfDrawTarget::New(GfVec2i(width, height));
}

} // anonymous namespace 

void wrapDrawTarget()
{
    typedef GlfDrawTarget This;
    typedef GlfDrawTargetPtr ThisPtr;
    
    class_<This, ThisPtr, noncopyable>("DrawTarget", no_init)
        .def(TfPyRefAndWeakPtr())
        .def("__init__",TfMakePyConstructor(&_NewDrawTarget))
        .def("__init__",TfMakePyConstructor(&_NewDrawTarget2))
        .def("AddAttachment", &This::AddAttachment)
        .def("Bind", &This::Bind)
        .def("Unbind", &This::Unbind)
        .def("WriteToFile", 
            &This::WriteToFile, (
             arg("attachment"),
             arg("filename"),
             arg("viewMatrix") = GfMatrix4d(1),
             arg("projectionMatrix") = GfMatrix4d(1)))
        
        ;
}
