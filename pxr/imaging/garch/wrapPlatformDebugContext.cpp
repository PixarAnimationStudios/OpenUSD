//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glPlatformDebugContext.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"

#include "pxr/external/boost/python/class.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapPlatformDebugContext()
{    
    typedef GarchGLPlatformDebugContext This;

    class_<This, TfWeakPtr<This>,
           noncopyable>("GLPlatformDebugContext", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(GarchGLPlatformDebugContext::New))
        .def("makeCurrent", &This::makeCurrent)
    ;
}
