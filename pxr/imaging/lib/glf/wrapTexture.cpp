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
#include "pxr/imaging/glf/texture.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;

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
    


PXR_NAMESPACE_CLOSE_SCOPE

