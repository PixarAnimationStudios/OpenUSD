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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/baseTextureData.h"
#include "pxr/imaging/glf/utils.h"

GlfBaseTextureData::~GlfBaseTextureData()
{
    /* nothing */
}

/* static */
GLenum
GlfBaseTextureData::_GLInternalFormatFromImageData(
    GLenum format, GLenum type, bool isSRGB)
{
    int numElements = GlfGetNumElements(format);
    bool g =  (numElements == 1);
    bool a =  (numElements == 4);

    switch (type) {
    case GL_UNSIGNED_INT:
        return g ? GL_R16 : (a ? GL_RGBA16 : GL_RGB16);        
    case GL_HALF_FLOAT:
        return g ? GL_R16F : (a ? GL_RGBA16F : GL_RGB16F);
    case GL_FLOAT:
    case GL_DOUBLE:
        return g ? GL_R32F : (a ? GL_RGBA32F : GL_RGB32F);
    case GL_UNSIGNED_BYTE:
    default:
        return g ? GL_RED : (a ? (isSRGB ? GL_SRGB_ALPHA : GL_RGBA)
                               : (isSRGB ? GL_SRGB : GL_RGB));
    }
}

