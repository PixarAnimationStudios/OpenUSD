//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiGL/buffer.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/sampler.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"
#include "pxr/imaging/hgiGL/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLResourceBindings::HgiGLResourceBindings(
    HgiResourceBindingsDesc const& desc)
    : HgiResourceBindings(desc)
{
}

HgiGLResourceBindings::~HgiGLResourceBindings() = default;

void
HgiGLResourceBindings::BindResources()
{
    std::vector<uint32_t> textures(_descriptor.textures.size(), 0);
    std::vector<uint32_t> samplers(textures.size(), 0);
    std::vector<uint32_t> images(textures.size(), 0);

    bool hasTex = false;
    bool hasSampler = false;
    bool hasImage = false;

    //
    // Bind Textures, images and samplers
    //
    for (HgiTextureBindDesc const& texDesc : _descriptor.textures) {
        // OpenGL does not support arrays-of-textures bound to a unit.
        // (Which is different from texture-arrays. See Vulkan/Metal)
        if (!TF_VERIFY(texDesc.textures.size() == 1)) continue;

        uint32_t unit = texDesc.bindingIndex;
        if (textures.size() <= unit) {
            textures.resize(unit+1, 0);
            samplers.resize(unit+1, 0);
            images.resize(unit+1, 0);
        }

        if (texDesc.resourceType == HgiBindResourceTypeSampledImage ||
            texDesc.resourceType == HgiBindResourceTypeCombinedSamplerImage) {
            // Texture sampling (for graphics pipeline)
            hasTex = true;
            HgiTextureHandle const& texHandle = texDesc.textures.front();
            HgiGLTexture* glTex = static_cast<HgiGLTexture*>(texHandle.Get());
            textures[texDesc.bindingIndex] = glTex->GetTextureId();
        } else if (texDesc.resourceType == HgiBindResourceTypeStorageImage) {
            // Image load/store (usually for compute pipeline)
            hasImage = true;
            HgiTextureHandle const& texHandle = texDesc.textures.front();
            HgiGLTexture* glTex = static_cast<HgiGLTexture*>(texHandle.Get());
            images[texDesc.bindingIndex] = glTex->GetTextureId();
        } else {
            TF_CODING_ERROR("Unsupported texture bind resource type");
        }

        // 'StorageImage' types do not need a sampler, so check if we have one.
        if (!texDesc.samplers.empty()) {
            hasSampler = true;
            HgiSamplerHandle const& smpHandle = texDesc.samplers.front();
            HgiGLSampler* glSmp = static_cast<HgiGLSampler*>(smpHandle.Get());
            samplers[texDesc.bindingIndex] = glSmp->GetSamplerId();
        }
    }

    if (hasTex) {
        glBindTextures(0, textures.size(), textures.data());
    }

    if (hasSampler) {
        glBindSamplers(0, samplers.size(), samplers.data());
    }

    // 'texture units' are separate from 'texture image units' in OpenGL.
    // glBindImageTextures should not reset textures bound with glBindTextures.
    if (hasImage) {
        glBindImageTextures(0, images.size(), images.data());
    }

    //
    // Bind Buffers
    //
    for (HgiBufferBindDesc const & bufDesc : _descriptor.buffers) {
        // OpenGL does not support arrays-of-buffers bound to a unit.
        // (Which is different from buffer-arrays. See Vulkan/Metal)
        if (!TF_VERIFY(bufDesc.buffers.size() == 1)) continue;

        if (bufDesc.buffers.size() != bufDesc.offsets.size()) {
            TF_CODING_ERROR("Invalid number of buffer offsets");
            continue;
        }

        if (!bufDesc.sizes.empty() &&
            bufDesc.buffers.size() != bufDesc.sizes.size()) {
            TF_CODING_ERROR("Invalid number of buffer sizes");
            continue;
        }

        HgiBufferHandle const & bufHandle = bufDesc.buffers.front();
        HgiGLBuffer const * glbuffer =
            static_cast<HgiGLBuffer*>(bufHandle.Get());
        GLuint const bufferId = glbuffer->GetBufferId();

        uint32_t const offset = bufDesc.offsets.front();
        uint32_t const size = bufDesc.sizes.empty() ? 0 : bufDesc.sizes.front();
        uint32_t const bindingIndex = bufDesc.bindingIndex;

        if (offset != 0 && size == 0) {
            TF_CODING_ERROR("Invalid size for buffer with offset");
            continue;
        }

        GLenum target = 0;
        if (bufDesc.resourceType == HgiBindResourceTypeUniformBuffer) {
            target = GL_UNIFORM_BUFFER;
        } else if (bufDesc.resourceType == HgiBindResourceTypeStorageBuffer) {
            target = GL_SHADER_STORAGE_BUFFER;
        } else {
            TF_CODING_ERROR("Unknown buffer type to bind");
            continue;
        }

        if (size != 0) {
            glBindBufferRange(target, bindingIndex, bufferId, offset, size);
        } else {
            glBindBufferBase(target, bindingIndex, bufferId);
        }
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}


PXR_NAMESPACE_CLOSE_SCOPE
