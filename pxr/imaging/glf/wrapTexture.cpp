//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/texture.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapTexture()
{    
    typedef GlfTexture This;
    typedef GlfTexturePtr ThisPtr;

    class_<This, ThisPtr, boost::noncopyable>(
        "Texture", no_init)
        .def("GetTextureMemoryAllocated", &This::GetTextureMemoryAllocated)
        .staticmethod("GetTextureMemoryAllocated")

        .add_property( "memoryUsed", make_function(
                &This::GetMemoryUsed,
                return_value_policy<return_by_value>()))

        .add_property( "memoryRequested", make_function(
                &This::GetMemoryRequested,
                return_value_policy<return_by_value>()),
                &This::SetMemoryRequested)

        .add_property( "minFilterSupported", make_function(
                &This::IsMinFilterSupported,
                return_value_policy<return_by_value>()))

        .add_property( "magFilterSupported", make_function(
                &This::IsMagFilterSupported,
                return_value_policy<return_by_value>()))
        ;
}
