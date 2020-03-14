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
#include "pxr/imaging/glf/baseTexture.h"

#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapBaseTexture()
{    
    using This    = GlfBaseTexture;
    using ThisPtr = GlfBaseTexturePtr;

    class_<This, bases<GlfTexture>, ThisPtr, boost::noncopyable>(
        "BaseTexture", no_init)

    	.add_property( "GlTextureName", make_function(
                &This::GetGlTextureName,
                return_value_policy<return_by_value>()))

        .add_property( "width", make_function(
                &This::GetWidth,
                return_value_policy<return_by_value>()))

        .add_property( "height", make_function(
            &This::GetHeight,
            return_value_policy<return_by_value>()))

        .add_property( "format", make_function(
            &This::GetFormat,
            return_value_policy<return_by_value>()))

        .add_property( "textureInfo", make_function(
            &This::GetTextureInfo,
            return_value_policy<return_by_value>(),
            (arg("forceLoad") = true)))
        ;
}

TF_REFPTR_CONST_VOLATILE_GET(GlfBaseTexture)
