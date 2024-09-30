//
// Copyright 2020 benmalartre
//
// Unlicensed 
// 
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/plugin/LoFi/glConversions.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <cctype>

PXR_NAMESPACE_OPEN_SCOPE


static const HioFormat FORMAT_DESC[] =
{
    // HioFormat            // HdFormat
    HioFormatUNorm8,        // HdFormatUNorm8,
    HioFormatUNorm8Vec2,    // HdFormatUNorm8Vec2,
    HioFormatUNorm8Vec3,    // HdFormatUNorm8Vec3,
    HioFormatUNorm8Vec4,    // HdFormatUNorm8Vec4,

    HioFormatSNorm8,        // HdFormatSNorm8,
    HioFormatSNorm8Vec2,    // HdFormatSNorm8Vec2,
    HioFormatSNorm8Vec3,    // HdFormatSNorm8Vec3,
    HioFormatSNorm8Vec4,    // HdFormatSNorm8Vec4,

    HioFormatFloat16,       // HdFormatFloat16,
    HioFormatFloat16Vec2,   // HdFormatFloat16Vec2,
    HioFormatFloat16Vec3,   // HdFormatFloat16Vec3,
    HioFormatFloat16Vec4,   // HdFormatFloat16Vec4,

    HioFormatFloat32,       // HdFormatFloat32,
    HioFormatFloat32Vec2,   // HdFormatFloat32Vec2,
    HioFormatFloat32Vec3,   // HdFormatFloat32Vec3,
    HioFormatFloat32Vec4,   // HdFormatFloat32Vec4,
	
	HioFormatInt16,         // HdFormatInt16,
    HioFormatInt16Vec2,     // HdFormatInt16Vec2,
    HioFormatInt16Vec3,     // HdFormatInt16Vec3,
    HioFormatInt16Vec4,     // HdFormatInt16Vec4,

    HioFormatUInt16,        // HdFormatUInt16,
    HioFormatUInt16Vec2,    // HdFormatUInt16Vec2,
    HioFormatUInt16Vec3,    // HdFormatUInt16Vec3,
    HioFormatUInt16Vec4,    // HdFormatUInt16Vec4,

    HioFormatInt32,         // HdFormatInt32,
    HioFormatInt32Vec2,     // HdFormatInt32Vec2,
    HioFormatInt32Vec3,     // HdFormatInt32Vec3,
    HioFormatInt32Vec4,     // HdFormatInt32Vec4,

    HioFormatFloat32,       // HdFormatFloat32UInt8
	
};
static_assert(TfArraySize(FORMAT_DESC) ==
        HdFormatCount, "FORMAT_DESC to HdFormat enum mismatch");

size_t
LoFiGLConversions::GetComponentSize(int glDataType)
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
LoFiGLConversions::GetGlDepthFunc(HdCompareFunction func)
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
    static_assert(
        (sizeof(HD_2_GL_DEPTH_FUNC) / sizeof(HD_2_GL_DEPTH_FUNC[0])) ==
                HdCmpFuncLast, "Mismatch enum sizes in convert function");

    return HD_2_GL_DEPTH_FUNC[func];
}

GLenum
LoFiGLConversions::GetGlStencilFunc(HdCompareFunction func)
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
    static_assert(
        (sizeof(HD_2_GL_STENCIL_FUNC) / sizeof(HD_2_GL_STENCIL_FUNC[0])) ==
                HdCmpFuncLast, "Mismatch enum sizes in convert function");

    return HD_2_GL_STENCIL_FUNC[func];
}

GLenum
LoFiGLConversions::GetGlStencilOp(HdStencilOp op)
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
    static_assert(
        (sizeof(HD_2_GL_STENCIL_OP) / sizeof(HD_2_GL_STENCIL_OP[0])) ==
                HdStencilOpLast, "Mismatch enum sizes in convert function");

    return HD_2_GL_STENCIL_OP[op];
}

GLenum
LoFiGLConversions::GetGlBlendOp(HdBlendOp op)
{
    static GLenum HD_2_GL_BLEND_OP[] =
    {
        GL_FUNC_ADD,              // HdBlendOpAdd
        GL_FUNC_SUBTRACT,         // HdBlendOpSubtract
        GL_FUNC_REVERSE_SUBTRACT, // HdBlendOpReverseSubtract
        GL_MIN,                   // HdBlendOpMin
        GL_MAX,                   // HdBlendOpMax
    };
    static_assert(
        (sizeof(HD_2_GL_BLEND_OP) / sizeof(HD_2_GL_BLEND_OP[0])) ==
                HdBlendOpLast, "Mismatch enum sizes in convert function");

    return HD_2_GL_BLEND_OP[op];
}

GLenum
LoFiGLConversions::GetGlBlendFactor(HdBlendFactor factor)
{
    static GLenum HD_2_GL_BLEND_FACTOR[] =
    {
        GL_ZERO,                        // HdBlendFactorZero,
        GL_ONE,                         // HdBlendFactorOne,
        GL_SRC_COLOR,                   // HdBlendFactorSrcColor,
        GL_ONE_MINUS_SRC_COLOR,         // HdBlendFactorOneMinusSrcColor,
        GL_DST_COLOR,                   // HdBlendFactorDstColor,
        GL_ONE_MINUS_DST_COLOR,         // HdBlendFactorOneMinusDstColor,
        GL_SRC_ALPHA,                   // HdBlendFactorSrcAlpha,
        GL_ONE_MINUS_SRC_ALPHA,         // HdBlendFactorOneMinusSrcAlpha,
        GL_DST_ALPHA,                   // HdBlendFactorDstAlpha,
        GL_ONE_MINUS_DST_ALPHA,         // HdBlendFactorOneMinusDstAlpha,
        GL_CONSTANT_COLOR,              // HdBlendFactorConstantColor,
        GL_ONE_MINUS_CONSTANT_COLOR,    // HdBlendFactorOneMinusConstantColor,
        GL_CONSTANT_ALPHA,              // HdBlendFactorConstantAlpha,
        GL_ONE_MINUS_CONSTANT_ALPHA,    // HdBlendFactorOneMinusConstantAlpha,
        GL_SRC_ALPHA_SATURATE,          // HdBlendFactorSrcAlphaSaturate,
        GL_SRC1_COLOR,                  // HdBlendFactorSrc1Color,
        GL_ONE_MINUS_SRC1_COLOR,        // HdBlendFactorOneMinusSrc1Color,
        GL_SRC1_ALPHA,                  // HdBlendFactorSrc1Alpha,
        GL_ONE_MINUS_SRC1_COLOR,        // HdBlendFactorOneMinusSrc1Alpha,
    };
    static_assert(
        (sizeof(HD_2_GL_BLEND_FACTOR) / sizeof(HD_2_GL_BLEND_FACTOR[0])) ==
                HdBlendFactorLast, "Mismatch enum sizes in convert function");

    return HD_2_GL_BLEND_FACTOR[factor];
}

HioFormat
LoFiGLConversions::GetHioFormat(HdFormat inFormat)
{
    if ((inFormat < 0) || (inFormat >= HdFormatCount)) {
        TF_CODING_ERROR("Unexpected HdFormat %d", inFormat);
        return HioFormatUNorm8Vec4;
    }
    return FORMAT_DESC[inFormat];
}

int
LoFiGLConversions::GetGLAttribType(HdType type)
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
    case HdTypeFloatMat3:
    case HdTypeFloatMat4:
        return GL_FLOAT;
    case HdTypeDouble:
    case HdTypeDoubleVec2:
    case HdTypeDoubleVec3:
    case HdTypeDoubleVec4:
    case HdTypeDoubleMat3:
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
    (mat3)
    (mat4)

    ((_double, "double"))
    (dvec2)
    (dvec3)
    (dvec4)
    (dmat3)
    (dmat4)

    ((_int, "int"))
    (ivec2)
    (ivec3)
    (ivec4)

    ((_uint, "uint"))
    (uvec2)
    (uvec3)
    (uvec4)

    (packed_2_10_10_10)
);

TfToken
LoFiGLConversions::GetGLSLTypename(HdType type)
{
    switch (type) {
    case HdTypeInvalid:
    default:
        return TfToken();

    // Packed types (require special handling in codegen)...
    case HdTypeInt32_2_10_10_10_REV:
        return _glTypeNames->packed_2_10_10_10;

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
    case HdTypeFloatVec4:
        return _glTypeNames->vec4;
    case HdTypeFloatMat3:
        return _glTypeNames->mat3;
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
    case HdTypeDoubleMat3:
        return _glTypeNames->dmat3;
    case HdTypeDoubleMat4:
        return _glTypeNames->dmat4;
    };
}

// This isn't an exhaustive checker. It doesn't check for built-in/internal
// variable names in GLSL, reserved keywords and such.
static bool
_IsIdentiferGLSLCompatible(std::string const& in)
{
    char const *p = in.c_str();

    // Leading non-alpha characters are not allowed.
    if (*p && !isalpha(*p)) {
        return false;
    }
    // Characters must be in [_a-zA-Z0-9]
    while (*p) {
        if (isalnum(*p)) {
            p++;
        } else {
            // _ is allowed, but __ isn't
            if (*p == '_' && *(p-1) != '_') {
                // checking the last character is safe here, because of the
                // earlier check for leading non-alpha characters.
                p++;
            } else {
                return false;
            }
        }
    }

    return true;
}

TfToken
LoFiGLConversions::GetGLSLIdentifier(TfToken const& identifier)
{
    std::string const& in = identifier.GetString();
    // Avoid allocating a string and constructing a token for the general case,
    // wherein identifers conform to the naming rules.
    if (_IsIdentiferGLSLCompatible(in)) {
        return identifier;
    }

    // Name-mangling rules:
    // https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.4.60.pdf
    // We choose to specifically disallow:
    // 1) Leading non-alpha characters: GLSL allows leading underscores, but we
    //    choose to reserve them for internal use.
    // 2) Consecutive underscores: To avoid unintended GLSL behaviors.
    std::string result;
    result.reserve(in.size());
    char const *p = in.c_str();

    // Skip leading non-alpha characters.
    while (*p && !isalpha(*p)) {
        ++p;
    }
    for (; *p; ++p) {
        bool isValidChar = isalnum(*p) || (*p == '_');
        if (!isValidChar) {
            // Replace characters not in [_a-zA-Z0-9] with _, unless the last
            // character  added was also _.
            // Calling back() is safe here because the first character is either
            // alpha-numeric or null, as guaranteed by the while loop above.
            if (result.back() != '_') {
                result.push_back('_');
            }
        } else if (*p == '_' && result.back() == '_') {
            // no-op to skip consecutive _
        } else {
            result.push_back(*p);
        }
    }

    if (result.empty()) {
        TF_CODING_ERROR("Invalid identifier '%s' could not be name-mangled",
                        identifier.GetText());
        return identifier;
    }

    return TfToken(result);
}

PXR_NAMESPACE_CLOSE_SCOPE

