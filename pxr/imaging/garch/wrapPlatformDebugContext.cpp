//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glPlatformDebugContext.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"

#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapPlatformDebugContext()
{    
    typedef GarchGLPlatformDebugContext This;

    class_<This, TfWeakPtr<This>,
           boost::noncopyable>("GLPlatformDebugContext", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(GarchGLPlatformDebugContext::New))
        .def("makeCurrent", &This::makeCurrent)
    ;
}
