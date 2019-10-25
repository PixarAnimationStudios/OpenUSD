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
    // format,  type,          internal format
    {GL_RED,  GL_UNSIGNED_BYTE, GL_R8},      // HdFormatUNorm8,
    {GL_RG,   GL_UNSIGNED_BYTE, GL_RG8},     // HdFormatUNorm8Vec2,
    {GL_RGB,  GL_UNSIGNED_BYTE, GL_RGB8},    // HdFormatUNorm8Vec3,
    {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA8},   // HdFormatUNorm8Vec4,

    {GL_RED,  GL_BYTE,          GL_R8_SNORM},      // HdFormatSNorm8,
    {GL_RG,   GL_BYTE,          GL_RG8_SNORM},     // HdFormatSNorm8Vec2,
    {GL_RGB,  GL_BYTE,          GL_RGB8_SNORM},    // HdFormatSNorm8Vec3,
    {GL_RGBA, GL_BYTE,          GL_RGBA8_SNORM},   // HdFormatSNorm8Vec4,

    {GL_RED,  GL_HALF_FLOAT,    GL_R16F},    // HdFormatFloat16,
    {GL_RG,   GL_HALF_FLOAT,    GL_RG16F},   // HdFormatFloat16Vec2,
    {GL_RGB,  GL_HALF_FLOAT,    GL_RGB16F},  // HdFormatFloat16Vec3,
    {GL_RGBA, GL_HALF_FLOAT,    GL_RGBA16F}, // HdFormatFloat16Vec4,

    {GL_RED,  GL_FLOAT,         GL_R32F},    // HdFormatFloat32,
    {GL_RG,   GL_FLOAT,         GL_RG32F},   // HdFormatFloat32Vec2,
    {GL_RGB,  GL_FLOAT,         GL_RGB32F},  // HdFormatFloat32Vec3,
    {GL_RGBA, GL_FLOAT,         GL_RGBA32F}, // HdFormatFloat32Vec4,

    {GL_RED,  GL_INT,           GL_R32I},    // HdFormatInt32,
    {GL_RG,   GL_INT,           GL_RG32I},   // HdFormatInt32Vec2,
    {GL_RGB,  GL_INT,           GL_RGB32I},  // HdFormatInt32Vec3,
    {GL_RGBA, GL_INT,           GL_RGBA32I}, // HdFormatInt32Vec4,
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

void
HgiGLConversions::GetFormat(
        HgiFormat inFormat,
        GLenum *outFormat, GLenum *outType, GLenum *outInternalFormat)
{
    if ((inFormat < 0) || (inFormat >= HgiFormatCount))
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


PXR_NAMESPACE_CLOSE_SCOPE
