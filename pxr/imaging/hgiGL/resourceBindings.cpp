//
// Copyright 2020 Pixar
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
        } else {
            // A sampler MUST be provided for sampler image textures (Hgi rule).
            TF_VERIFY(texDesc.resourceType != HgiBindResourceTypeSampledImage);
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

    // Note that index and vertex buffers are not bound here.
    // They are set via GraphicsCmds.

    std::vector<uint32_t> ubos(_descriptor.buffers.size(), 0);
    std::vector<uint32_t> sbos(_descriptor.buffers.size(), 0);

    for (HgiBufferBindDesc const& bufDesc : _descriptor.buffers) {
        // OpenGL does not support arrays-of-buffers bound to a unit.
        // (Which is different from buffer-arrays. See Vulkan/Metal)
        if (!TF_VERIFY(bufDesc.buffers.size() == 1)) continue;

        uint32_t unit = bufDesc.bindingIndex;

        std::vector<uint32_t>* dst = nullptr;

        if (bufDesc.resourceType == HgiBindResourceTypeUniformBuffer) {
            dst = &ubos;
        } else if (bufDesc.resourceType == HgiBindResourceTypeStorageBuffer) {
            dst = &sbos;
        } else {
            TF_CODING_ERROR("Unknown buffer type to bind");
            continue;
        }

        if (dst->size() <= unit) {
            dst->resize(unit+1, 0);
        }
        HgiBufferHandle const& bufHandle = bufDesc.buffers.front();
        HgiGLBuffer* glbuffer = static_cast<HgiGLBuffer*>(bufHandle.Get());

        (*dst)[bufDesc.bindingIndex] = glbuffer->GetBufferId();
    }

    if (!ubos.empty()) {
        glBindBuffersBase(GL_UNIFORM_BUFFER, 0, ubos.size(), ubos.data());
    }

    if (!sbos.empty()) {
        glBindBuffersBase(GL_SHADER_STORAGE_BUFFER,0, sbos.size(), sbos.data());
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}


PXR_NAMESPACE_CLOSE_SCOPE
