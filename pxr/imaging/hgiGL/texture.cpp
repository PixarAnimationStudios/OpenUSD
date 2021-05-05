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
    const GfVec3i &dimensions,
    const GLsizei layerCount)
{
    switch(textureType) {
    case HgiTextureType1D:
        glTextureStorage1D(texture,
                           levels,
                           internalformat,
                           dimensions[0]);
        break;
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
    case HgiTextureType1DArray:
        glTextureStorage2D(texture,
                           levels,
                           internalformat,
                           dimensions[0], layerCount);
        break;
    case HgiTextureType2DArray:
        glTextureStorage3D(texture,
                           levels,
                           internalformat,
                           dimensions[0], dimensions[1], layerCount);
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
    const uint32_t layerCount,
    const GLenum format,
    const GLenum type,
    const void * pixels)
{
    switch(textureType) {
    case HgiTextureType1D:
        glTextureSubImage1D(texture,
                            level,
                            offsets[0],
                            dimensions[0],
                            format,
                            type,
                            pixels);
        break;
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
    case HgiTextureType1DArray:
        glTextureSubImage2D(texture,
                            level,
                            offsets[0], offsets[1],
                            dimensions[0], layerCount,
                            format,
                            type,
                            pixels);
        break;
    case HgiTextureType2DArray:
        glTextureSubImage3D(texture,
                            level,
                            offsets[0], offsets[1], offsets[2],
                            dimensions[0], dimensions[1], layerCount,
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
        HgiGLObjectLabel(GL_TEXTURE, _textureId, _descriptor.debugName);
    }

    if (desc.sampleCount == HgiSampleCount1) {
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
            desc.dimensions,
            desc.layerCount);

        // Upload texel data
        if (desc.initialData && desc.pixelsByteSize > 0) {
            // Upload each (available) mip
            const std::vector<HgiMipInfo> mipInfos =
                HgiGetMipInfos(
                    desc.format,
                    desc.dimensions,
                    desc.layerCount,
                    desc.pixelsByteSize);
            const size_t mipLevels = std::min(
                mipInfos.size(), size_t(desc.mipLevels));
            const char * const initialData = reinterpret_cast<const char *>(
                desc.initialData);

            for (size_t mip = 0; mip < mipLevels; mip++) {
                const HgiMipInfo &mipInfo = mipInfos[mip];

                if (isCompressed) {
                    _GlCompressedTextureSubImageND(
                        desc.type,
                        _textureId,
                        mip,
                        /*offsets*/GfVec3i(0),
                        mipInfo.dimensions,
                        glInternalFormat,
                        mipInfo.byteSizePerLayer * desc.layerCount,
                        initialData + mipInfo.byteOffset);
                } else {
                    _GlTextureSubImageND(
                        desc.type,
                        _textureId,
                        mip,
                        /*offsets*/GfVec3i(0),
                        mipInfo.dimensions,
                        desc.layerCount,
                        glFormat,
                        glPixelType,
                        initialData + mipInfo.byteOffset);
                }
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

    const GLint swizzleMask[] = {
        GLint(HgiGLConversions::GetComponentSwizzle(desc.componentMapping.r)),
        GLint(HgiGLConversions::GetComponentSwizzle(desc.componentMapping.g)),
        GLint(HgiGLConversions::GetComponentSwizzle(desc.componentMapping.b)),
        GLint(HgiGLConversions::GetComponentSwizzle(desc.componentMapping.a)) };

    glTextureParameteriv(
        _textureId,
        GL_TEXTURE_SWIZZLE_RGBA,
        swizzleMask);

    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLTexture::HgiGLTexture(HgiTextureViewDesc const & desc)
    : HgiTexture(desc.sourceTexture->GetDescriptor())
    , _textureId(0)
{
    // Update the texture descriptor to reflect the view desc
    _descriptor.debugName = desc.debugName;
    _descriptor.format = desc.format;
    _descriptor.layerCount = desc.layerCount;
    _descriptor.mipLevels = desc.mipLevels;

    HgiGLTexture* srcTexture =
        static_cast<HgiGLTexture*>(desc.sourceTexture.Get());
    GLenum glInternalFormat = 0;

    if (srcTexture->GetDescriptor().usage & HgiTextureUsageBitsDepthTarget) {
        TF_VERIFY(desc.format == HgiFormatFloat32 ||
                  desc.format == HgiFormatFloat32UInt8);
        
        if (desc.format == HgiFormatFloat32UInt8) {
            glInternalFormat = GL_DEPTH32F_STENCIL8;
        } else {
            glInternalFormat = GL_DEPTH_COMPONENT32F;
        }
    } else {
        GLenum glFormat = 0;
        GLenum glPixelType = 0;
        HgiGLConversions::GetFormat(
            desc.format,
            &glFormat,
            &glPixelType,
            &glInternalFormat);
    }

    // Note we must use glGenTextures, not glCreateTextures.
    // glTextureView requires the textureId to be unbound and not given a type.
    glGenTextures(1, &_textureId);

    GLenum textureType =
        HgiGLConversions::GetTextureType(srcTexture->GetDescriptor().type);

    glTextureView(
        _textureId,
        textureType,
        srcTexture->GetTextureId(),
        glInternalFormat, 
        desc.sourceFirstMip, 
        desc.mipLevels,
        desc.sourceFirstLayer,
        desc.layerCount);

    if (!desc.debugName.empty()) {
        HgiGLObjectLabel(GL_TEXTURE, _textureId, desc.debugName);
    }

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
    return _GetByteSizeOfResource(_descriptor);
}

uint64_t
HgiGLTexture::GetRawResource() const
{
    return (uint64_t) _textureId;
}

PXR_NAMESPACE_CLOSE_SCOPE
