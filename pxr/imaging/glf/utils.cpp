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

#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/utils.h"
#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE

struct _FormatDesc {
    GLenum format;
    GLenum type;
    GLenum internalFormat;
};

static const _FormatDesc FORMAT_DESC[] =
{
    // glFormat, glType,        glInternatFormat  // HioFormat
    {GL_RED,  GL_UNSIGNED_BYTE, GL_R8          }, // UNorm8
    {GL_RG,   GL_UNSIGNED_BYTE, GL_RG8         }, // UNorm8Vec2
    {GL_RGB,  GL_UNSIGNED_BYTE, GL_RGB8        }, // UNorm8Vec3
    {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8       }, // UNorm8Vec4

    {GL_RED,  GL_BYTE,          GL_R8_SNORM    }, // SNorm8
    {GL_RG,   GL_BYTE,          GL_RG8_SNORM   }, // SNorm8Vec2
    {GL_RGB,  GL_BYTE,          GL_RGB8_SNORM  }, // SNorm8Vec3
    {GL_RGBA, GL_BYTE,          GL_RGBA8_SNORM }, // SNorm8Vec4

    {GL_RED,  GL_HALF_FLOAT,    GL_R16F        }, // Float16
    {GL_RG,   GL_HALF_FLOAT,    GL_RG16F       }, // Float16Vec2
    {GL_RGB,  GL_HALF_FLOAT,    GL_RGB16F      }, // Float16Vec3
    {GL_RGBA, GL_HALF_FLOAT,    GL_RGBA16F     }, // Float16Vec4

    {GL_RED,  GL_FLOAT,         GL_R32F        }, // Float32
    {GL_RG,   GL_FLOAT,         GL_RG32F       }, // Float32Vec2
    {GL_RGB,  GL_FLOAT,         GL_RGB32F      }, // Float32Vec3
    {GL_RGBA, GL_FLOAT,         GL_RGBA32F     }, // Float32Vec4

    {GL_RED,  GL_DOUBLE,        GL_RED         }, // Double64
    {GL_RG,   GL_DOUBLE,        GL_RG          }, // Double64Vec2
    {GL_RGB,  GL_DOUBLE,        GL_RGB         }, // Double64Vec3
    {GL_RGBA, GL_DOUBLE,        GL_RGBA        }, // Double64Vec4

    {GL_RED,  GL_UNSIGNED_SHORT,GL_R16UI       }, // UInt16
    {GL_RG,   GL_UNSIGNED_SHORT,GL_RG16UI      }, // UInt16Vec2
    {GL_RGB,  GL_UNSIGNED_SHORT,GL_RGB16UI     }, // UInt16Vec3
    {GL_RGBA, GL_UNSIGNED_SHORT,GL_RGBA16UI    }, // UInt16Vec4

    {GL_RED,  GL_SHORT,         GL_R16I        }, // Int16
    {GL_RG,   GL_SHORT,         GL_RG16I       }, // Int16Vec2
    {GL_RGB,  GL_SHORT,         GL_RGB16I      }, // Int16Vec3
    {GL_RGBA, GL_SHORT,         GL_RGBA16I     }, // Int16Vec4

    {GL_RED,  GL_UNSIGNED_INT,  GL_R32UI       }, // UInt32
    {GL_RG,   GL_UNSIGNED_INT,  GL_RG32UI      }, // UInt32Vec2
    {GL_RGB,  GL_UNSIGNED_INT,  GL_RGB32UI     }, // UInt32Vec3
    {GL_RGBA, GL_UNSIGNED_INT,  GL_RGBA32UI    }, // UInt32Vec4

    {GL_RED,  GL_INT,           GL_R32I        }, // Int32
    {GL_RG,   GL_INT,           GL_RG32I       }, // Int32Vec2
    {GL_RGB,  GL_INT,           GL_RGB32I      }, // Int32Vec3
    {GL_RGBA, GL_INT,           GL_RGBA32I     }, // Int32Vec4

    {GL_NONE, GL_NONE, GL_NONE }, // UNorm8srgb - not supported by OpenGL
    {GL_NONE, GL_NONE, GL_NONE }, // UNorm8Vec2srgb - not supported by OpenGL
    {GL_RGB,  GL_UNSIGNED_BYTE, GL_SRGB8,       },  // UNorm8Vec3srgb
    {GL_RGBA, GL_UNSIGNED_BYTE, GL_SRGB8_ALPHA8 }, // UNorm8Vec4sRGB

    {GL_RGB,  GL_FLOAT,
              GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT   }, // BC6FloatVec3
    {GL_RGB,  GL_FLOAT,
              GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT }, // BC6UFloatVec3
    {GL_RGBA, GL_UNSIGNED_BYTE,
              GL_COMPRESSED_RGBA_BPTC_UNORM         }, // BC7UNorm8Vec4
    {GL_RGBA, GL_UNSIGNED_BYTE,
              GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM   }, // BC7UNorm8Vec4srgb

};

// A few random format validations to make sure the GL table stays
// aligned with the HioFormat table.
constexpr bool _CompileTimeValidateHioFormatTable() {
    return (TfArraySize(FORMAT_DESC) == HioFormatCount &&
            HioFormatUNorm8 == 0 &&
            HioFormatFloat32 == 12 &&
            HioFormatUInt32 == 28 &&
            HioFormatBC6FloatVec3 == 40) ? true : false;
}

static_assert(_CompileTimeValidateHioFormatTable(), 
              "_FormatDesc array in glfUtils out of sync with HioFormat enum");


GLenum
GlfGetBaseFormat(int numComponents)
{
    switch (numComponents) {
        case 1:
            return GL_RED;
        case 2:
            return GL_RG;
        case 3:
            return GL_RGB;
        case 4:
            return GL_RGBA;
        default:
            TF_CODING_ERROR("Unsupported numComponents");
            return 1;
    }
}

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
GlfGetNumElements(HioFormat hioFormat)
{
    return GlfGetNumElements(GlfGetGLFormat(hioFormat));
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

int
GlfGetElementSize(HioFormat hioFormat)
{
    return GlfGetElementSize(GlfGetGLType(hioFormat));
}

GLenum
GlfGetGLType(HioFormat hioFormat)
{
    const _FormatDesc &desc = FORMAT_DESC[hioFormat];
    return desc.type;
}

GLenum
GlfGetGLFormat(HioFormat hioFormat)
{
    const _FormatDesc &desc = FORMAT_DESC[hioFormat];
    return desc.format;
}

GLenum
GlfGetGLInternalFormat(HioFormat hioFormat)
{
    const _FormatDesc &desc = FORMAT_DESC[hioFormat];
    return desc.internalFormat;
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

