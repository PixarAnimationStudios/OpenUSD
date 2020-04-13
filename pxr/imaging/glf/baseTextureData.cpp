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

PXR_NAMESPACE_OPEN_SCOPE


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
    switch (type) {
    case GL_UNSIGNED_INT:
        switch (numElements) {
        case 1:
            return GL_R16;
        case 2:
            return GL_RG16;
        case 3:
            return GL_RGB16;
        case 4:
            return GL_RGBA16;
        default:
            break;
        }
    case GL_HALF_FLOAT:
        switch (numElements) {
        case 1:
            return GL_R16F;
        case 2:
            return GL_RG16F;
        case 3:
            return GL_RGB16F;
        case 4:
            return GL_RGBA16F;
        default:
            break;
        }
    case GL_FLOAT:
    case GL_DOUBLE:
        switch (numElements) {
        case 1:
            return GL_R32F;
        case 2:
            return GL_RG32F;
        case 3:
            return GL_RGB32F;
        case 4:
            return GL_RGBA32F;
        default:
            break;
        }
    case GL_UNSIGNED_BYTE:
        switch (numElements) {
        case 1:
            return GL_R8;
        case 2:
            return GL_RG8;
        case 3:
            return isSRGB ? GL_SRGB8 : GL_RGB8;
        case 4:
            return isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        default:
            break;
        }
    default:
        break;
    }

    TF_CODING_ERROR("Unsupported image data "
                    "format: %d "
                    "type: %d "
                    "isSRGB: %d ", format, type, isSRGB);
    return GL_RGBA;
}


PXR_NAMESPACE_CLOSE_SCOPE

