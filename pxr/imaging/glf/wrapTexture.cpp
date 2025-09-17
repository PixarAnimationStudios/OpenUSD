//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/glf/texture.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"

#include "pxr/external/boost/python/bases.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/overloads.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapTexture()
{    
    typedef GlfTexture This;
    typedef GlfTexturePtr ThisPtr;

    class_<This, ThisPtr, noncopyable>(
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
