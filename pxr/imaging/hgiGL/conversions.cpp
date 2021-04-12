//
// Copyright 2019 Pixar
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

};

// A few random format validations to make sure out GL table stays aligned
// with the HgiFormat table.
constexpr bool _CompileTimeValidateHgiFormatTable() {
    return (TfArraySize(FORMAT_DESC) == HgiFormatCount &&
            HgiFormatUNorm8 == 0 &&
            HgiFormatFloat16Vec4 == 9 &&
            HgiFormatFloat32Vec4 == 13 &&
            HgiFormatUInt16Vec4 == 17 &&
            HgiFormatUNorm8Vec4srgb == 22 &&
            HgiFormatBC3UNorm8Vec4 == 28) ? true : false;
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
    {HgiPrimitiveTypePatchList,    GL_PATCHES}
};

void
HgiGLConversions::GetFormat(
        HgiFormat inFormat,
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
    if (outFormat) {
        *outFormat = desc.format;
    }
    if (outType) {
        *outType = desc.type;
    }
    if (outInternalFormat) {
        *outInternalFormat = desc.internalFormat;
    }
}

GLenum
HgiGLConversions::GetFormatType(HgiFormat inFormat)
{
    const _FormatDesc &desc = FORMAT_DESC[inFormat];
    return desc.type;
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
HgiGLConversions::GetDepthCompareFunction(HgiCompareFunction cf)
{
    return _compareFunctionTable[cf][1];
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

PXR_NAMESPACE_CLOSE_SCOPE
