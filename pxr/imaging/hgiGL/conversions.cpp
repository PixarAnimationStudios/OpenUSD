//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgiGL/conversions.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

struct _FormatDesc {
    GLenum format;
    GLenum type;
    GLenum internalFormat;
};

static const _FormatDesc FORMAT_DESC[] =
{
    // format,  type,             internal format
    {GL_RED,  GL_UNSIGNED_BYTE, GL_R8          }, // UNorm8
    {GL_RG,   GL_UNSIGNED_BYTE, GL_RG8         }, // UNorm8Vec2
    // {GL_RGB,  GL_UNSIGNED_BYTE, GL_RGB8       }, // Unsupported by HgiFormat
    {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8       }, // UNorm8Vec4

    {GL_RED,  GL_BYTE,          GL_R8_SNORM    }, // SNorm8
    {GL_RG,   GL_BYTE,          GL_RG8_SNORM   }, // SNorm8Vec2
    // {GL_RGB,  GL_BYTE,         GL_RGB8_SNORM  }, // Unsupported by HgiFormat
    {GL_RGBA, GL_BYTE,          GL_RGBA8_SNORM }, // SNorm8Vec4

    {GL_RED,  GL_HALF_FLOAT,    GL_R16F        }, // Float16
    {GL_RG,   GL_HALF_FLOAT,    GL_RG16F       }, // Float16Vec2
    {GL_RGB,  GL_HALF_FLOAT,    GL_RGB16F      }, // Float16Vec3
    {GL_RGBA, GL_HALF_FLOAT,    GL_RGBA16F     }, // Float16Vec4

    {GL_RED,  GL_FLOAT,         GL_R32F        }, // Float32
    {GL_RG,   GL_FLOAT,         GL_RG32F       }, // Float32Vec2
    {GL_RGB,  GL_FLOAT,         GL_RGB32F      }, // Float32Vec3
    {GL_RGBA, GL_FLOAT,         GL_RGBA32F     }, // Float32Vec4

    {GL_RED_INTEGER,  GL_SHORT, GL_R16I        }, // Int16
    {GL_RG_INTEGER,   GL_SHORT, GL_RG16I       }, // Int16Vec2
    {GL_RGB_INTEGER,  GL_SHORT, GL_RGB16I      }, // Int16Vec3
    {GL_RGBA_INTEGER, GL_SHORT, GL_RGBA16I     }, // Int16Vec4

    {GL_RED_INTEGER,  GL_UNSIGNED_SHORT,GL_R16UI        }, // UInt16
    {GL_RG_INTEGER,   GL_UNSIGNED_SHORT,GL_RG16UI       }, // UInt16Vec2
    {GL_RGB_INTEGER,  GL_UNSIGNED_SHORT,GL_RGB16UI      }, // UInt16Vec3
    {GL_RGBA_INTEGER, GL_UNSIGNED_SHORT,GL_RGBA16UI     }, // UInt16Vec4

    {GL_RED_INTEGER,  GL_INT,   GL_R32I        }, // Int32
    {GL_RG_INTEGER,   GL_INT,   GL_RG32I       }, // Int32Vec2
    {GL_RGB_INTEGER,  GL_INT,   GL_RGB32I      }, // Int32Vec3
    {GL_RGBA_INTEGER, GL_INT,   GL_RGBA32I     }, // Int32Vec4

    // {GL_RGB,  GL_UNSIGNED_BYTE, GL_SRGB8      }, // Unsupported by HgiFormat
    {GL_RGBA, GL_UNSIGNED_BYTE, GL_SRGB8_ALPHA8}, // UNorm8Vec4sRGB,

    {GL_RGB,  GL_FLOAT,
              GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT  }, // BC6FloatVec3
    {GL_RGB,  GL_FLOAT,
              GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT}, // BC6UFloatVec3
    {GL_RGBA, GL_UNSIGNED_BYTE,
              GL_COMPRESSED_RGBA_BPTC_UNORM }, // BC7UNorm8Vec4
    {GL_RGBA, GL_UNSIGNED_BYTE,
              GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM }, // BC7UNorm8Vec4srgb
    {GL_RGBA, GL_UNSIGNED_BYTE,
              GL_COMPRESSED_RGBA_S3TC_DXT1_EXT }, // BC1UNorm8Vec4
    {GL_RGBA, GL_UNSIGNED_BYTE,
              GL_COMPRESSED_RGBA_S3TC_DXT5_EXT }, // BC3UNorm8Vec4

    {GL_DEPTH_STENCIL, GL_FLOAT, GL_DEPTH32F_STENCIL8}, // Float32UInt8

    {GL_INT_2_10_10_10_REV, GL_INT_2_10_10_10_REV, GL_RGBA },
                                                // PackedInt10Int10Int10Int2

};

// A few random format validations to make sure out GL table stays aligned
// with the HgiFormat table.
constexpr bool _CompileTimeValidateHgiFormatTable() {
    return (TfArraySize(FORMAT_DESC) == HgiFormatCount &&
            HgiFormatUNorm8 == 0 &&
            HgiFormatFloat16Vec4 == 9 &&
            HgiFormatFloat32Vec4 == 13 &&
            HgiFormatUInt16Vec4 == 21 &&
            HgiFormatUNorm8Vec4srgb == 26 &&
            HgiFormatBC3UNorm8Vec4 == 32) ? true : false;
}

static_assert(_CompileTimeValidateHgiFormatTable(), 
              "_FormatDesc array out of sync with HgiFormat enum");

static const uint32_t
_ShaderStageTable[][2] =
{
    {HgiShaderStageVertex,              GL_VERTEX_SHADER},
    {HgiShaderStageFragment,            GL_FRAGMENT_SHADER},
    {HgiShaderStageCompute,             GL_COMPUTE_SHADER},
    {HgiShaderStageTessellationControl, GL_TESS_CONTROL_SHADER},
    {HgiShaderStageTessellationEval,    GL_TESS_EVALUATION_SHADER},
    {HgiShaderStageGeometry,            GL_GEOMETRY_SHADER}
};

static const uint32_t
_CullModeTable[HgiCullModeCount][2] =
{
    {HgiCullModeNone,         GL_NONE},
    {HgiCullModeFront,        GL_FRONT},
    {HgiCullModeBack,         GL_BACK},
    {HgiCullModeFrontAndBack, GL_FRONT_AND_BACK}
};

static const uint32_t
_PolygonModeTable[HgiCullModeCount][2] =
{
    {HgiPolygonModeFill,  GL_FILL},
    {HgiPolygonModeLine,  GL_LINE},
    {HgiPolygonModePoint, GL_POINT},
};

static uint32_t
_blendEquationTable[HgiBlendOpCount][2] =
{
    {HgiBlendOpAdd,             GL_FUNC_ADD},
    {HgiBlendOpSubtract,        GL_FUNC_SUBTRACT},
    {HgiBlendOpReverseSubtract, GL_FUNC_REVERSE_SUBTRACT},
    {HgiBlendOpMin,             GL_MIN},
    {HgiBlendOpMax,             GL_MAX},
};

static uint32_t _blendFactorTable[HgiBlendFactorCount][2] =
{
    {HgiBlendFactorZero,                  GL_ZERO},
    {HgiBlendFactorOne,                   GL_ONE},
    {HgiBlendFactorSrcColor,              GL_SRC_COLOR},
    {HgiBlendFactorOneMinusSrcColor,      GL_ONE_MINUS_SRC_COLOR},
    {HgiBlendFactorDstColor,              GL_DST_COLOR},
    {HgiBlendFactorOneMinusDstColor,      GL_ONE_MINUS_DST_COLOR},
    {HgiBlendFactorSrcAlpha,              GL_SRC_ALPHA},
    {HgiBlendFactorOneMinusSrcAlpha,      GL_ONE_MINUS_SRC_ALPHA},
    {HgiBlendFactorDstAlpha,              GL_DST_ALPHA},
    {HgiBlendFactorOneMinusDstAlpha,      GL_ONE_MINUS_DST_ALPHA},
    {HgiBlendFactorConstantColor,         GL_CONSTANT_COLOR},
    {HgiBlendFactorOneMinusConstantColor, GL_ONE_MINUS_CONSTANT_COLOR},
    {HgiBlendFactorConstantAlpha,         GL_CONSTANT_ALPHA},
    {HgiBlendFactorOneMinusConstantAlpha, GL_ONE_MINUS_CONSTANT_ALPHA},
    {HgiBlendFactorSrcAlphaSaturate,      GL_SRC_ALPHA_SATURATE},
    {HgiBlendFactorSrc1Color,             GL_SRC1_COLOR},
    {HgiBlendFactorOneMinusSrc1Color,     GL_ONE_MINUS_SRC1_COLOR},
    {HgiBlendFactorSrc1Alpha,             GL_SRC1_ALPHA},
    {HgiBlendFactorOneMinusSrc1Alpha,     GL_ONE_MINUS_SRC1_COLOR},
};

static uint32_t
_compareFunctionTable[HgiCompareFunctionCount][2] =
{
    {HgiCompareFunctionNever,    GL_NEVER},
    {HgiCompareFunctionLess,     GL_LESS},
    {HgiCompareFunctionEqual,    GL_EQUAL},
    {HgiCompareFunctionLEqual,   GL_LEQUAL},
    {HgiCompareFunctionGreater,  GL_GREATER},
    {HgiCompareFunctionNotEqual, GL_NOTEQUAL},
    {HgiCompareFunctionGEqual,   GL_GEQUAL},
    {HgiCompareFunctionAlways,   GL_ALWAYS},
};

static uint32_t
_stencilOpTable[HgiStencilOpCount][2] =
{
    {HgiStencilOpKeep,           GL_KEEP},
    {HgiStencilOpZero,           GL_ZERO},
    {HgiStencilOpReplace,        GL_REPLACE},
    {HgiStencilOpIncrementClamp, GL_INCR},
    {HgiStencilOpDecrementClamp, GL_DECR},
    {HgiStencilOpInvert,         GL_INVERT},
    {HgiStencilOpIncrementWrap,  GL_INCR_WRAP},
    {HgiStencilOpDecrementWrap,  GL_DECR_WRAP},
};

static uint32_t
_textureTypeTable[HgiTextureTypeCount][2] =
{
    {HgiTextureType1D,      GL_TEXTURE_1D},
    {HgiTextureType2D,      GL_TEXTURE_2D},
    {HgiTextureType3D,      GL_TEXTURE_3D},
    {HgiTextureType1DArray, GL_TEXTURE_1D_ARRAY},
    {HgiTextureType2DArray, GL_TEXTURE_2D_ARRAY}
};

static uint32_t
_samplerAddressModeTable[HgiSamplerAddressModeCount][2] =
{
    {HgiSamplerAddressModeClampToEdge,        GL_CLAMP_TO_EDGE},
    {HgiSamplerAddressModeMirrorClampToEdge,  GL_MIRROR_CLAMP_TO_EDGE},
    {HgiSamplerAddressModeRepeat,             GL_REPEAT},
    {HgiSamplerAddressModeMirrorRepeat,       GL_MIRRORED_REPEAT},
    {HgiSamplerAddressModeClampToBorderColor, GL_CLAMP_TO_BORDER}
};

static const uint32_t
_componentSwizzleTable[HgiComponentSwizzleCount][2] =
{
    {HgiComponentSwizzleZero, GL_ZERO},
    {HgiComponentSwizzleOne,  GL_ONE},
    {HgiComponentSwizzleR,    GL_RED},
    {HgiComponentSwizzleG,    GL_GREEN},
    {HgiComponentSwizzleB,    GL_BLUE},
    {HgiComponentSwizzleA,    GL_ALPHA}
};

static const uint32_t
_primitiveTypeTable[HgiPrimitiveTypeCount][2] =
{
    {HgiPrimitiveTypePointList,    GL_POINTS},
    {HgiPrimitiveTypeLineList,     GL_LINES},
    {HgiPrimitiveTypeLineStrip,    GL_LINES_ADJACENCY},
    {HgiPrimitiveTypeTriangleList, GL_TRIANGLES},
    {HgiPrimitiveTypePatchList,    GL_PATCHES},
    {HgiPrimitiveTypeLineListWithAdjacency, GL_LINES_ADJACENCY}
};

static const std::string
_imageLayoutFormatTable[HgiFormatCount][2] =
{ 
    {"HgiFormatUNorm8",            "r8"},
    {"HgiFormatUNorm8Vec2",        "rg8"},
    {"HgiFormatUNorm8Vec4",        "rgba8"},
    {"HgiFormatSNorm8",            "r8_snorm"},
    {"HgiFormatSNorm8Vec2",        "rg8_snorm"},
    {"HgiFormatSNorm8Vec4",        "rgba8_snorm"},
    {"HgiFormatFloat16",           "r16f"},
    {"HgiFormatFloat16Vec2",       "rg16f"},
    {"HgiFormatFloat16Vec3",       ""},
    {"HgiFormatFloat16Vec4",       "rgba16f"},
    {"HgiFormatFloat32",           "r32f"},
    {"HgiFormatFloat32Vec2",       "rg32f"},
    {"HgiFormatFloat32Vec3",       ""},
    {"HgiFormatFloat32Vec4",       "rgba32f" },
    {"HgiFormatInt16",             "r16i"},
    {"HgiFormatInt16Vec2",         "rg16i"},
    {"HgiFormatInt16Vec3",         ""},
    {"HgiFormatInt16Vec4",         "rgba16i"},
    {"HgiFormatUInt16",            "r16ui"},
    {"HgiFormatUInt16Vec2",        "rg16ui"},
    {"HgiFormatUInt16Vec3",        ""},
    {"HgiFormatUInt16Vec4",        "rgba16ui"},
    {"HgiFormatInt32",             "r32i"},
    {"HgiFormatInt32Vec2",         "rg32i"},
    {"HgiFormatInt32Vec3",         ""},
    {"HgiFormatInt32Vec4",         "rgba32i"},
    {"HgiFormatUNorm8Vec4srgb",    ""},
    {"HgiFormatBC6FloatVec3",      ""},
    {"HgiFormatBC6UFloatVec3",     ""},
    {"HgiFormatBC7UNorm8Vec4",     ""},
    {"HgiFormatBC7UNorm8Vec4srgb", ""},
    {"HgiFormatBC1UNorm8Vec4",     ""},
    {"HgiFormatBC3UNorm8Vec4",     ""},
    {"HgiFormatFloat32UInt8",      ""},
};

void
HgiGLConversions::GetFormat(
        HgiFormat inFormat,
        HgiTextureUsage inUsage,
        GLenum *outFormat, 
        GLenum *outType, 
        GLenum *outInternalFormat)
{
    if ((inFormat < 0) || (inFormat >= HgiFormatCount))
    {
        TF_CODING_ERROR("Unexpected  %d", inFormat);
        if (outFormat) {
            *outFormat = GL_RGBA;
        }
        if (outType) {
            *outType = GL_BYTE;
        }
        if (outInternalFormat) {
            *outInternalFormat = GL_RGBA8;
        }
        return;
    }

    const _FormatDesc &desc = FORMAT_DESC[inFormat];
    const bool depthTarget = inUsage & HgiTextureUsageBitsDepthTarget;
    if (outFormat) {
        if (depthTarget && inFormat == HgiFormatFloat32) {
            *outFormat = GL_DEPTH_COMPONENT;
        }
        else {
            *outFormat = desc.format;
        }
    }
    if (outType) {
        *outType = desc.type;
    }
    if (outInternalFormat) {
        if (depthTarget && inFormat == HgiFormatFloat32) {
            *outInternalFormat = GL_DEPTH_COMPONENT32F;
        }
        else {
            *outInternalFormat = desc.internalFormat;
        }
    }
}

GLenum
HgiGLConversions::GetFormatType(HgiFormat inFormat)
{
    const _FormatDesc &desc = FORMAT_DESC[inFormat];
    return desc.type;
}

bool
HgiGLConversions::IsVertexAttribIntegerFormat(HgiFormat inFormat)
{
    const _FormatDesc &desc = FORMAT_DESC[inFormat];
    return desc.type == GL_BYTE ||
           desc.type == GL_UNSIGNED_BYTE ||
           desc.type == GL_SHORT ||
           desc.type == GL_UNSIGNED_SHORT ||
           desc.type == GL_INT ||
           desc.type == GL_UNSIGNED_INT;
}

std::vector<GLenum>
HgiGLConversions::GetShaderStages(HgiShaderStage ss)
{
    std::vector<GLenum> stages;
    for (const auto& f : _ShaderStageTable) {
        if (ss & f[0]) stages.push_back(f[1]);
    }

    if (stages.empty()) {
        TF_CODING_ERROR("Missing shader stage table entry");
    }
    return stages;
}

GLenum
HgiGLConversions::GetCullMode(HgiCullMode cm)
{
    return _CullModeTable[cm][1];
}

GLenum
HgiGLConversions::GetPolygonMode(HgiPolygonMode pm)
{
    return _PolygonModeTable[pm][1];
}

GLenum
HgiGLConversions::GetBlendFactor(HgiBlendFactor bf)
{
    return _blendFactorTable[bf][1];
}

GLenum
HgiGLConversions::GetBlendEquation(HgiBlendOp bo)
{
    return _blendEquationTable[bo][1];
}

GLenum
HgiGLConversions::GetCompareFunction(HgiCompareFunction cf)
{
    return _compareFunctionTable[cf][1];
}

GLenum
HgiGLConversions::GetStencilOp(HgiStencilOp op)
{
    return _stencilOpTable[op][1];
}

GLenum
HgiGLConversions::GetTextureType(HgiTextureType tt)
{
    return _textureTypeTable[tt][1];
}

GLenum
HgiGLConversions::GetSamplerAddressMode(HgiSamplerAddressMode am)
{
    return _samplerAddressModeTable[am][1];
}

GLenum
HgiGLConversions::GetMagFilter(HgiSamplerFilter sf)
{
    switch(sf) {
        case HgiSamplerFilterNearest: return GL_NEAREST;
        case HgiSamplerFilterLinear: return GL_LINEAR;
        default: break;
    }

    TF_CODING_ERROR("Unsupported sampler options");
    return GL_NONE;
}

GLenum
HgiGLConversions::GetMinFilter(
    HgiSamplerFilter minFilter, 
    HgiMipFilter mipFilter)
{
    switch(mipFilter) {
    // No mip-filter supplied (no mipmapping), return min-filter
    case HgiMipFilterNotMipmapped : 
        switch(minFilter) {
            case HgiSamplerFilterNearest: return GL_NEAREST;
            case HgiSamplerFilterLinear: return GL_LINEAR;
            default: TF_CODING_ERROR("Unsupported type"); break;
        }

    // Mip filter is nearest, combine min and mip filter into one enum
    case HgiMipFilterNearest:
        switch(minFilter) {
            case HgiSamplerFilterNearest: return GL_NEAREST_MIPMAP_NEAREST;
            case HgiSamplerFilterLinear: return GL_LINEAR_MIPMAP_NEAREST;
            default: TF_CODING_ERROR("Unsupported typr"); break;
        }

    // Mip filter is linear, combine min and mip filter into one enum
    case HgiMipFilterLinear:
        switch(minFilter) {
            case HgiSamplerFilterNearest: return GL_NEAREST_MIPMAP_LINEAR;
            case HgiSamplerFilterLinear: return GL_LINEAR_MIPMAP_LINEAR;
            default: TF_CODING_ERROR("Unsupported typr"); break;
        }

    default: break;
    }

    TF_CODING_ERROR("Unsupported sampler options");
    return GL_NONE;
}

GfVec4f
HgiGLConversions::GetBorderColor(HgiBorderColor borderColor)
{
    switch(borderColor) {
        case HgiBorderColorTransparentBlack: return GfVec4f(0, 0, 0, 0);
        case HgiBorderColorOpaqueBlack: return GfVec4f(0, 0, 0, 1);
        case HgiBorderColorOpaqueWhite: return GfVec4f(1, 1, 1, 1);
        default: break;
    }

    TF_CODING_ERROR("Unsupported sampler options");
    return GfVec4f(0, 0, 0, 0);
}

GLenum
HgiGLConversions::GetComponentSwizzle(HgiComponentSwizzle componentSwizzle)
{
    return _componentSwizzleTable[componentSwizzle][1];
}

GLenum
HgiGLConversions::GetPrimitiveType(HgiPrimitiveType pt)
{
    return _primitiveTypeTable[pt][1];
}

std::string 
HgiGLConversions::GetImageLayoutFormatQualifier(HgiFormat inFormat)
{
    const std::string layoutQualifier = _imageLayoutFormatTable[inFormat][1];
    if (layoutQualifier.empty()) {
        TF_WARN("Given HgiFormat is not a supported image unit format, "
                "defaulting to rgba16f");
        return _imageLayoutFormatTable[9][1];
    }
    return layoutQualifier;
}

PXR_NAMESPACE_CLOSE_SCOPE
