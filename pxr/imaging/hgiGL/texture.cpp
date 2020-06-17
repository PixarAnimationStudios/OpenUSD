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
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

static
void
_GlTextureStorageND(
    const HgiTextureType textureType,
    const GLuint texture,
    const GLsizei levels,
    const GLenum internalformat,
    const GfVec3i &dimensions)
{
    switch(textureType) {
    case HgiTextureType2D:
        glTextureStorage2D(texture,
                           levels,
                           internalformat,
                           dimensions[0], dimensions[1]);
        break;
    case HgiTextureType3D:
        glTextureStorage3D(texture,
                           levels,
                           internalformat,
                           dimensions[0], dimensions[1], dimensions[2]);
        break;
    default:
        TF_CODING_ERROR("Unsupported HgiTextureType enum value");
        break;
    }
}

static
void
_GlTextureSubImageND(
    const HgiTextureType textureType,
    const GLuint texture,
    const GLint level,
    const GfVec3i &offsets,
    const GfVec3i &dimensions,
    const GLenum format,
    const GLenum type,
    const void * pixels)
{
    switch(textureType) {
    case HgiTextureType2D:
        glTextureSubImage2D(texture,
                            level,
                            offsets[0], offsets[1],
                            dimensions[0], dimensions[1],
                            format,
                            type,
                            pixels);
        break;
    case HgiTextureType3D:
        glTextureSubImage3D(texture,
                            level,
                            offsets[0], offsets[1], offsets[2],
                            dimensions[0], dimensions[1], dimensions[2],
                            format,
                            type,
                            pixels);
        break;
    default:
        TF_CODING_ERROR("Unsupported HgiTextureType enum value");
        break;
    }
}

static
void
_GlCompressedTextureSubImageND(
    const HgiTextureType textureType,
    const GLuint texture,
    const GLint level,
    const GfVec3i &offsets,
    const GfVec3i &dimensions,
    const GLenum format,
    const GLsizei imageSize,
    const void * pixels)
{
    switch(textureType) {
    case HgiTextureType2D:
        glCompressedTextureSubImage2D(
            texture,
            level,
            offsets[0], offsets[1],
            dimensions[0], dimensions[1],
            format,
            imageSize,
            pixels);
        break;
    case HgiTextureType3D:
        glCompressedTextureSubImage3D(
            texture,
            level,
            offsets[0], offsets[1], offsets[2],
            dimensions[0], dimensions[1], dimensions[2],
            format,
            imageSize,
            pixels);
        break;
    default:
        TF_CODING_ERROR("Unsupported HgiTextureType enum value");
        break;
    }
}

static
bool _IsValidCompression(HgiTextureDesc const & desc)
{
    switch(desc.type) {
    case HgiTextureType2D:
        if ( desc.dimensions[0] % 4 != 0 ||
             desc.dimensions[1] % 4 != 0) {
            TF_CODING_ERROR("Compressed texture with width or height "
                            "not a multiple of 4");
            return false;
        }
        return true;
    case HgiTextureType3D:
        if ( desc.dimensions[0] % 4 != 0 ||
             desc.dimensions[1] % 4 != 0 ||
             desc.dimensions[2] % 4 != 0) {
            TF_CODING_ERROR("Compressed texture with width, height or depth"
                            "not a multiple of 4");
            return false;
        }
        return true;
    default:
        TF_CODING_ERROR("Compression not supported for given texture "
                        "type");
        return false;
    }
}

HgiGLTexture::HgiGLTexture(HgiTextureDesc const & desc)
    : HgiTexture(desc)
    , _textureId(0)
{
    if (desc.layerCount > 1) {
        // XXX Further below we are missing support for layered textures.
        TF_CODING_ERROR("XXX Missing implementation for texture arrays");
    }

    GLenum glInternalFormat = 0;
    GLenum glFormat = 0;
    GLenum glPixelType = 0;
    const bool isCompressed = HgiIsCompressed(desc.format);

    if (desc.usage & HgiTextureUsageBitsDepthTarget) {
        TF_VERIFY(desc.format == HgiFormatFloat32 ||
                  desc.format == HgiFormatFloat32UInt8);
        
        if (desc.format == HgiFormatFloat32UInt8) {
            glFormat = GL_DEPTH_STENCIL;
            glInternalFormat = GL_DEPTH32F_STENCIL8;
        } else {
            glFormat = GL_DEPTH_COMPONENT;
            glInternalFormat = GL_DEPTH_COMPONENT32F;
        }
        glPixelType = GL_FLOAT;
    } else {
        HgiGLConversions::GetFormat(
            desc.format, 
            &glFormat, 
            &glPixelType,
            &glInternalFormat);
    }

    if (isCompressed && !_IsValidCompression(desc)) {
        return;
    }

    if (desc.sampleCount == HgiSampleCount1) {
        glCreateTextures(
            HgiGLConversions::GetTextureType(desc.type),
            1, 
            &_textureId);
    } else {
        if (desc.type != HgiTextureType2D) {
            TF_CODING_ERROR("Only 2d multisample textures are supported");
        }
        glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &_textureId);
    }

    if (!_descriptor.debugName.empty()) {
        glObjectLabel(GL_TEXTURE, _textureId,-1, _descriptor.debugName.c_str());
    }

    if (desc.sampleCount == HgiSampleCount1) {
        // XXX sampler state etc should all be set via tex descriptor.
        //     (probably pass in HgiSamplerHandle in tex descriptor)
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_textureId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        const uint16_t mips = desc.mipLevels;
        GLint minFilter = mips > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        glTextureParameteri(_textureId, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(_textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        float aniso = 2.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
        glTextureParameterf(_textureId, GL_TEXTURE_MAX_ANISOTROPY_EXT,aniso);
        glTextureParameteri(_textureId, GL_TEXTURE_BASE_LEVEL, /*low-mip*/0);
        glTextureParameteri(_textureId, GL_TEXTURE_MAX_LEVEL, /*hi-mip*/mips-1);

        _GlTextureStorageND(
            desc.type,
            _textureId,
            mips,
            glInternalFormat,
            desc.dimensions);

        if (desc.initialData && desc.pixelsByteSize > 0) {
            if (isCompressed) {
                _GlCompressedTextureSubImageND(
                    desc.type,
                    _textureId,
                    /*mip*/0,
                    /*offsets*/GfVec3i(0),
                    desc.dimensions,
                    glInternalFormat,
                    desc.pixelsByteSize,
                    desc.initialData);
            } else {
                _GlTextureSubImageND(
                    desc.type,
                    _textureId,
                    /*mip*/0,
                    /*offsets*/GfVec3i(0),
                    desc.dimensions,
                    glFormat,
                    glPixelType,
                    desc.initialData);
            }
        }
    } else {
        // Note: Setting sampler state values on multi-sample texture is invalid
        glTextureStorage2DMultisample(
            _textureId,
            desc.sampleCount,
            glInternalFormat,
            desc.dimensions[0],
            desc.dimensions[1],
            GL_TRUE);
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLTexture::~HgiGLTexture()
{
    if (_textureId > 0) {
        glDeleteTextures(1, &_textureId);
        _textureId = 0;
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}

size_t
HgiGLTexture::GetByteSizeOfResource() const
{
    GfVec3i const& s = _descriptor.dimensions;
    return HgiDataSizeOfFormat(_descriptor.format) * 
        s[0] * s[1] * std::max(s[2], 1);
}

uint64_t
HgiGLTexture::GetRawResource() const
{
    return (uint64_t) _textureId;
}

PXR_NAMESPACE_CLOSE_SCOPE
