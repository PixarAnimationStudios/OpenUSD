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
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


struct _FormatDesc {
    GLenum format;
    GLenum type;
    GLenum internalFormat;
};

static const _FormatDesc FORMAT_DESC[] =
{
    // format,  type,          internal format
    {GL_RED,  GL_UNSIGNED_BYTE, GL_RED},     // HdFormatR8UNorm,
    {GL_RED,  GL_BYTE,          GL_R8},      // HdFormatR8SNorm.

    {GL_RG,   GL_UNSIGNED_BYTE, GL_RG8},     // HdFormatR8G8UNorm,
    {GL_RG,   GL_BYTE,          GL_RG8},     // HdFormatR8G8SNorm,

    {GL_RGB,  GL_UNSIGNED_BYTE, GL_RGB8},    // HdFormatR8G8B8UNorm,
    {GL_RGB,  GL_BYTE,          GL_RGB8},    // HdFormatR8G8B8SNorm,

    {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8},   // HdFormatR8G8B8A8UNorm,
    {GL_RGBA, GL_BYTE,          GL_RGBA8},   // HdFormatR8G8B8A8SNorm,

    {GL_RED,  GL_FLOAT,         GL_R32F},    // HdFormatR32Float,

    {GL_RG,   GL_FLOAT,         GL_RG32F},   // HdFormatR32G32Float,

    {GL_RGB,  GL_FLOAT,         GL_RGB32F},  // HdFormatR32G32B32Float,

    {GL_RGBA,  GL_FLOAT,        GL_RGBA32F}, // HdFormatR32G32B32A32Float,
};
static_assert(TfArraySize(FORMAT_DESC) ==  HdFormatCount, "FORMAT_DESC to HdFormat enum mismatch");

size_t
HdStGLConversions::GetComponentSize(int glDataType)
{
    switch (glDataType) {
        case GL_BOOL:
            // Note that we don't use GLboolean here because according to
            // code in vtBufferSource, everything gets rounded up to 
            // size of single value in interleaved struct rounds up to
            // sizeof(GLint) according to GL spec.
            //      _size = std::max(sizeof(T), sizeof(GLint));
            return sizeof(GLint);
        case GL_BYTE:
            return sizeof(GLbyte);
        case GL_UNSIGNED_BYTE:
            return sizeof(GLubyte);
        case GL_SHORT:
            return sizeof(GLshort);
        case GL_UNSIGNED_SHORT:
            return sizeof(GLushort);
        case GL_INT:
            return sizeof(GLint);
        case GL_UNSIGNED_INT:
            return sizeof(GLuint);
        case GL_FLOAT:
            return sizeof(GLfloat);
        case GL_2_BYTES:
            return 2;
        case GL_3_BYTES:
            return 3;
        case GL_4_BYTES:
            return 4;
        case GL_UNSIGNED_INT64_ARB:
            return sizeof(GLuint64EXT);
        case GL_DOUBLE:
            return sizeof(GLdouble);
        case GL_INT_2_10_10_10_REV:
            return sizeof(GLint);
        // following enums are for bindless texture pointers.
        case GL_SAMPLER_2D:
            return sizeof(GLuint64EXT);
        case GL_SAMPLER_2D_ARRAY:
            return sizeof(GLuint64EXT);
        case GL_INT_SAMPLER_BUFFER:
            return sizeof(GLuint64EXT);
    };

    TF_CODING_ERROR("Unexpected GL datatype 0x%x", glDataType);
    return 1;
}


GLenum
HdStGLConversions::GetGlDepthFunc(HdCompareFunction func)
{
    static GLenum HD_2_GL_DEPTH_FUNC[] =
    {
        GL_NEVER,    // HdCmpFuncNever
        GL_LESS,     // HdCmpFuncLess
        GL_EQUAL,    // HdCmpFuncEqual
        GL_LEQUAL,   // HdCmpFuncLEqual
        GL_GREATER,  // HdCmpFuncGreater
        GL_NOTEQUAL, // HdCmpFuncNotEqual
        GL_GEQUAL,   // HdCmpFuncGEqual
        GL_ALWAYS,   // HdCmpFuncAlways
    };
    static_assert((sizeof(HD_2_GL_DEPTH_FUNC) / sizeof(HD_2_GL_DEPTH_FUNC[0])) == HdCmpFuncLast, "Mismatch enum sizes in convert function");

    return HD_2_GL_DEPTH_FUNC[func];
}

GLenum
HdStGLConversions::GetGlStencilFunc(HdCompareFunction func)
{
    static GLenum HD_2_GL_STENCIL_FUNC[] =
    {
        GL_NEVER,    // HdCmpFuncNever
        GL_LESS,     // HdCmpFuncLess
        GL_EQUAL,    // HdCmpFuncEqual
        GL_LEQUAL,   // HdCmpFuncLEqual
        GL_GREATER,  // HdCmpFuncGreater
        GL_NOTEQUAL, // HdCmpFuncNotEqual
        GL_GEQUAL,   // HdCmpFuncGEqual
        GL_ALWAYS,   // HdCmpFuncAlways
    };
    static_assert((sizeof(HD_2_GL_STENCIL_FUNC) / sizeof(HD_2_GL_STENCIL_FUNC[0])) == HdCmpFuncLast, "Mismatch enum sizes in convert function");

    return HD_2_GL_STENCIL_FUNC[func];
}

GLenum
HdStGLConversions::GetGlStencilOp(HdStencilOp op)
{
    static GLenum HD_2_GL_STENCIL_OP[] =
    {
        GL_KEEP,      // HdStencilOpKeep
        GL_ZERO,      // HdStencilOpZero
        GL_REPLACE,   // HdStencilOpReplace
        GL_INCR,      // HdStencilOpIncrement
        GL_INCR_WRAP, // HdStencilOpIncrementWrap
        GL_DECR,      // HdStencilOpDecrement
        GL_DECR_WRAP, // HdStencilOpDecrementWrap
        GL_INVERT,    // HdStencilOpInvert
    };
    static_assert((sizeof(HD_2_GL_STENCIL_OP) / sizeof(HD_2_GL_STENCIL_OP[0])) == HdStencilOpLast, "Mismatch enum sizes in convert function");

    return HD_2_GL_STENCIL_OP[op];
}

GLenum 
HdStGLConversions::GetMinFilter(HdMinFilter filter)
{
    switch (filter) {
        case HdMinFilterNearest : return GL_NEAREST;
        case HdMinFilterLinear :  return GL_LINEAR;
        case HdMinFilterNearestMipmapNearest : return GL_NEAREST_MIPMAP_NEAREST;
        case HdMinFilterLinearMipmapNearest : return GL_LINEAR_MIPMAP_NEAREST;
        case HdMinFilterNearestMipmapLinear : return GL_NEAREST_MIPMAP_LINEAR;
        case HdMinFilterLinearMipmapLinear : return GL_LINEAR_MIPMAP_LINEAR;
    }

    TF_CODING_ERROR("Unexpected HdMinFilter type %d", filter);
    return GL_NEAREST_MIPMAP_LINEAR; 
}

GLenum 
HdStGLConversions::GetMagFilter(HdMagFilter filter)
{
    switch (filter) {
        case HdMagFilterNearest : return GL_NEAREST;
        case HdMagFilterLinear : return GL_LINEAR;
    }

    TF_CODING_ERROR("Unexpected HdMagFilter type %d", filter);
    return GL_LINEAR;
}

GLenum 
HdStGLConversions::GetWrap(HdWrap wrap)
{
    switch (wrap) {
        case HdWrapClamp : return GL_CLAMP_TO_EDGE;
        case HdWrapRepeat : return GL_REPEAT;
        case HdWrapBlack : return GL_CLAMP_TO_BORDER;
        case HdWrapUseMetaDict : return GL_REPEAT;
    }

    TF_CODING_ERROR("Unexpected HdWrap type %d", wrap);
    return GL_REPEAT;
}

void
HdStGLConversions::GetGlFormat(HdFormat inFormat, GLenum *outFormat, GLenum *outType, GLenum *outInternalFormat)
{
    if ((inFormat < 0) || (inFormat >= HdFormatCount))
    {
        TF_CODING_ERROR("Unexpected HdFormat %d", inFormat);
        *outFormat         = GL_RGBA;
        *outType           = GL_BYTE;
        *outInternalFormat = GL_RGBA8;
        return;
    }

    const _FormatDesc &desc = FORMAT_DESC[inFormat];

    *outFormat         = desc.format;
    *outType           = desc.type;
    *outInternalFormat = desc.internalFormat;
}

int
HdStGLConversions::GetGLAttribType(HdType type)
{
    switch (type) {
    case HdTypeInt32:
    case HdTypeInt32Vec2:
    case HdTypeInt32Vec3:
    case HdTypeInt32Vec4:
        return GL_INT;
    case HdTypeUInt32:
    case HdTypeUInt32Vec2:
    case HdTypeUInt32Vec3:
    case HdTypeUInt32Vec4:
        return GL_UNSIGNED_INT;
    case HdTypeFloat:
    case HdTypeFloatVec2:
    case HdTypeFloatVec3:
    case HdTypeFloatVec4:
    case HdTypeFloatMat4:
        return GL_FLOAT;
    case HdTypeDouble:
    case HdTypeDoubleVec2:
    case HdTypeDoubleVec3:
    case HdTypeDoubleVec4:
    case HdTypeDoubleMat4:
        return GL_DOUBLE;
    case HdTypeInt32_2_10_10_10_REV:
        return GL_INT_2_10_10_10_REV;
    default:
        break;
    };
    return -1;
}

TF_DEFINE_PRIVATE_TOKENS(
    _glTypeNames,
    ((_bool, "bool"))

    ((_float, "float"))
    (vec2)
    (vec3)
    (vec4)
    (mat4)

    ((_double, "double"))
    (dvec2)
    (dvec3)
    (dvec4)
    (dmat4)

    ((_int, "int"))
    (ivec2)
    (ivec3)
    (ivec4)

    ((_uint, "uint"))
    (uvec2)
    (uvec3)
    (uvec4)
);

TfToken
HdStGLConversions::GetGLSLTypename(HdType type)
{
    switch (type) {
    case HdTypeInvalid:
    default:
        return TfToken();
    case HdTypeBool:
        return _glTypeNames->_bool;

    case HdTypeInt32:
        return _glTypeNames->_int;
    case HdTypeInt32Vec2:
        return _glTypeNames->ivec2;
    case HdTypeInt32Vec3:
        return _glTypeNames->ivec3;
    case HdTypeInt32Vec4:
        return _glTypeNames->ivec4;

    case HdTypeUInt32:
        return _glTypeNames->_uint;
    case HdTypeUInt32Vec2:
        return _glTypeNames->uvec2;
    case HdTypeUInt32Vec3:
        return _glTypeNames->uvec3;
    case HdTypeUInt32Vec4:
        return _glTypeNames->uvec4;

    case HdTypeFloat:
        return _glTypeNames->_float;
    case HdTypeFloatVec2:
        return _glTypeNames->vec2;
    case HdTypeFloatVec3:
        return _glTypeNames->vec3;
    case HdTypeInt32_2_10_10_10_REV:
        // Special case: treat as a vec4.
    case HdTypeFloatVec4:
        return _glTypeNames->vec4;
    case HdTypeFloatMat4:
        return _glTypeNames->mat4;

    case HdTypeDouble:
        return _glTypeNames->_double;
    case HdTypeDoubleVec2:
        return _glTypeNames->dvec2;
    case HdTypeDoubleVec3:
        return _glTypeNames->dvec3;
    case HdTypeDoubleVec4:
        return _glTypeNames->dvec4;
    case HdTypeDoubleMat4:
        return _glTypeNames->dmat4;
    };
}

PXR_NAMESPACE_CLOSE_SCOPE

