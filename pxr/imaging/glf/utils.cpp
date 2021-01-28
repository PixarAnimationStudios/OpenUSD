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
// utils.cpp
//

#include "pxr/imaging/glf/utils.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

int
GlfGetNumElements(GLenum format)
{
    switch (format) {
        case GL_DEPTH_COMPONENT:
        case GL_COLOR_INDEX:
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_RED:
            return 1;
        case GL_LUMINANCE_ALPHA :
        case GL_RG:
            return 2;
        case GL_RGB:
            return 3;
        case GL_RGBA:
            return 4;
        default:
            TF_CODING_ERROR("Unsupported format");
            return 1;
    }
}

int
GlfGetElementSize(GLenum type)
{
    switch (type) {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
            return sizeof(GLubyte);
        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
            return sizeof(GLshort);
        case GL_FLOAT:
            return sizeof(GLfloat);
        case GL_DOUBLE:
            return sizeof(GLdouble);
        case GL_HALF_FLOAT:
            return sizeof(GLhalf);
        default:
            TF_CODING_ERROR("Unsupported type");
            return sizeof(GLfloat);
    }
}

HioFormat
GlfGetHioFormat(GLenum glFormat, GLenum glType, bool isSRGB)
{
    switch (glFormat){
        case GL_DEPTH_COMPONENT:
        case GL_COLOR_INDEX:
        case GL_ALPHA:
        case GL_LUMINANCE:
        case GL_RED:
            switch (glType) {
                case GL_UNSIGNED_BYTE:
                    if (isSRGB) {
                        return HioFormatUNorm8srgb;
                    }
                    return HioFormatUNorm8;
                case GL_BYTE:
                    return HioFormatSNorm8;
                case GL_UNSIGNED_SHORT:
                    return HioFormatUInt16;
                case GL_SHORT:
                    return HioFormatInt16;
                case GL_UNSIGNED_INT:
                    return HioFormatUInt32;
                case GL_INT:
                    return HioFormatInt32;
                case GL_HALF_FLOAT:
                    return HioFormatFloat16;
                case GL_FLOAT:
                    return HioFormatFloat32;
                case GL_DOUBLE:
                    return HioFormatDouble64;
            }
        case GL_LUMINANCE_ALPHA :
        case GL_RG:
            switch (glType) {
                case GL_UNSIGNED_BYTE:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec2srgb;
                    }
                    return HioFormatUNorm8Vec2;
                case GL_BYTE:
                    return HioFormatSNorm8Vec2;
                case GL_UNSIGNED_SHORT:
                    return HioFormatUInt16Vec2;
                case GL_SHORT:
                    return HioFormatInt16Vec2;
                case GL_UNSIGNED_INT:
                    return HioFormatUInt32Vec2;
                case GL_INT:
                    return HioFormatInt32Vec2;
                case GL_HALF_FLOAT:
                    return HioFormatFloat16Vec2;
                case GL_FLOAT:
                    return HioFormatFloat32Vec2;
                case GL_DOUBLE:
                    return HioFormatDouble64Vec2;      
            }
        case GL_RGB:
            switch (glType) {
                case GL_UNSIGNED_BYTE:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec3srgb;
                    }
                    return HioFormatUNorm8Vec3;
                case GL_BYTE:
                    return HioFormatSNorm8Vec3;
                case GL_UNSIGNED_SHORT:
                    return HioFormatUInt16Vec3;
                case GL_SHORT:
                    return HioFormatInt16Vec3;
                case GL_UNSIGNED_INT:
                    return HioFormatUInt32Vec3;
                case GL_INT:
                    return HioFormatInt32Vec3;
                case GL_HALF_FLOAT:
                    return HioFormatFloat16Vec3;
                case GL_FLOAT:
                    return HioFormatFloat32Vec3;
                case GL_DOUBLE:
                    return HioFormatDouble64Vec3;             
            }
        case GL_RGBA:
            switch (glType) {
                case GL_UNSIGNED_BYTE:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec4srgb;
                    }
                    return HioFormatUNorm8Vec4;
                case GL_BYTE:
                    return HioFormatSNorm8Vec4;
                case GL_UNSIGNED_SHORT:
                    return HioFormatUInt16Vec4;
                case GL_SHORT:
                    return HioFormatInt16Vec4;
                case GL_UNSIGNED_INT:
                    return HioFormatUInt32Vec4;
                case GL_INT:
                    return HioFormatInt32Vec4;
                case GL_HALF_FLOAT:
                    return HioFormatFloat16Vec4;
                case GL_FLOAT:
                    return HioFormatFloat32Vec4;
                case GL_DOUBLE:
                    return HioFormatDouble64Vec4;             
            }
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
            return HioFormatBC6UFloatVec3;
        case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
            return HioFormatBC6FloatVec3;
        case GL_COMPRESSED_RGBA_BPTC_UNORM:
            return HioFormatBC7UNorm8Vec4;
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
            return HioFormatBC7UNorm8Vec4srgb;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return HioFormatBC1UNorm8Vec4;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return HioFormatBC3UNorm8Vec4;
        default:
            TF_CODING_ERROR("Unsupported type");
            return HioFormatUNorm8Vec3;
    }
}

bool
GlfCheckGLFrameBufferStatus(GLuint target, std::string * reason)
{
    GLenum status = glCheckFramebufferStatus(target);

    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
        return true;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        if (reason) {
            *reason = "Framebuffer unsupported";
        }
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        if (reason) {
            *reason = "Framebuffer incomplete attachment";
        }
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        if (reason) {
            *reason = "Framebuffer incomplete missing attachment";
        }
        return false;
#if defined (GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT)
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        if (reason) {
            *reason = "Framebuffer incomplete dimensions";
        }
        return false;
#endif	
#if defined (GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT)
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        if (reason) {
            *reason = "Framebuffer incomplete formats";
        }
        return false;
#endif	
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        if (reason) {
            *reason = "Framebuffer incomplete draw buffer";
        }
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        if (reason) {
            *reason = "Framebuffer incomplete read buffer";
        }
        return false;
    default:
        if (reason) {
            *reason = TfStringPrintf(
                "Framebuffer error 0x%x", (unsigned int)(status));
        }
        return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

