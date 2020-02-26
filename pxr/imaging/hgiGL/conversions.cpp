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
#include <GL/glew.h>

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
    uint8_t channelCount;
};

static const _FormatDesc FORMAT_DESC[] =
{
    // format,  type,        internal format  elements
    {GL_RED,  GL_UNSIGNED_BYTE, GL_R8,          1}, // HdFormatUNorm8,
    {GL_RG,   GL_UNSIGNED_BYTE, GL_RG8,         2}, // HdFormatUNorm8Vec2,
    {GL_RGB,  GL_UNSIGNED_BYTE, GL_RGB8,        3}, // HdFormatUNorm8Vec3,
    {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8,       4}, // HdFormatUNorm8Vec4,

    {GL_RED,  GL_BYTE,          GL_R8_SNORM,    1}, // HdFormatSNorm8,
    {GL_RG,   GL_BYTE,          GL_RG8_SNORM,   2}, // HdFormatSNorm8Vec2,
    {GL_RGB,  GL_BYTE,          GL_RGB8_SNORM,  3}, // HdFormatSNorm8Vec3,
    {GL_RGBA, GL_BYTE,          GL_RGBA8_SNORM, 4}, // HdFormatSNorm8Vec4,

    {GL_RED,  GL_HALF_FLOAT,    GL_R16F,        1}, // HdFormatFloat16,
    {GL_RG,   GL_HALF_FLOAT,    GL_RG16F,       2}, // HdFormatFloat16Vec2,
    {GL_RGB,  GL_HALF_FLOAT,    GL_RGB16F,      3}, // HdFormatFloat16Vec3,
    {GL_RGBA, GL_HALF_FLOAT,    GL_RGBA16F,     4}, // HdFormatFloat16Vec4,

    {GL_RED,  GL_FLOAT,         GL_R32F,        1}, // HdFormatFloat32,
    {GL_RG,   GL_FLOAT,         GL_RG32F,       2}, // HdFormatFloat32Vec2,
    {GL_RGB,  GL_FLOAT,         GL_RGB32F,      3}, // HdFormatFloat32Vec3,
    {GL_RGBA, GL_FLOAT,         GL_RGBA32F,     4}, // HdFormatFloat32Vec4,

    {GL_RED,  GL_INT,           GL_R32I,        1}, // HdFormatInt32,
    {GL_RG,   GL_INT,           GL_RG32I,       2}, // HdFormatInt32Vec2,
    {GL_RGB,  GL_INT,           GL_RGB32I,      3}, // HdFormatInt32Vec3,
    {GL_RGBA, GL_INT,           GL_RGBA32I,     4}, // HdFormatInt32Vec4,
};

constexpr bool _CompileTimeValidateHgiFormatTable() {
    return (TfArraySize(FORMAT_DESC) == HgiFormatCount &&
            HgiFormatUNorm8 == 0 &&
            HgiFormatFloat16Vec4 == 11 &&
            HgiFormatFloat32Vec4 == 15 &&
            HgiFormatInt32Vec4 == 19) ? true : false;
}

static_assert(_CompileTimeValidateHgiFormatTable(), 
              "_FormatDesc array out of sync with HgiFormat enum");

static const uint32_t
_ShaderStageTable[][2] =
{
    {HgiShaderStageVertex,   GL_VERTEX_SHADER},
    {HgiShaderStageFragment, GL_FRAGMENT_SHADER},
    {HgiShaderStageCompute,  GL_COMPUTE_SHADER}
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

void
HgiGLConversions::GetFormat(
        HgiFormat inFormat,
        GLenum *outFormat, 
        GLenum *outType, 
        GLenum *outInternalFormat)
{
    if ((inFormat < 0) || (inFormat >= HgiFormatCount))
    {
        TF_CODING_ERROR("Unexpected HdFormat %d", inFormat);
        *outFormat = GL_RGBA;
        *outType = GL_BYTE;
        *outInternalFormat = GL_RGBA8;
        return;
    }

    const _FormatDesc &desc = FORMAT_DESC[inFormat];

    *outFormat = desc.format;
    *outType = desc.type;
    *outInternalFormat = desc.internalFormat;
}

GLenum
HgiGLConversions::GetFormatType(HgiFormat inFormat)
{
    const _FormatDesc &desc = FORMAT_DESC[inFormat];
    return desc.type;
}

int8_t
HgiGLConversions::GetElementCount(HgiFormat inFormat)
{
    const _FormatDesc &desc = FORMAT_DESC[inFormat];
    return desc.channelCount;
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

PXR_NAMESPACE_CLOSE_SCOPE
