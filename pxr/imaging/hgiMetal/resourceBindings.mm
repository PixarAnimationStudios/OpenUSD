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
#include "pxr/imaging/hgiMetal/buffer.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/sampler.h"
#include "pxr/imaging/hgiMetal/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalResourceBindings::HgiMetalResourceBindings(
    HgiResourceBindingsDesc const& desc)
    : HgiResourceBindings(desc)
{
}

HgiMetalResourceBindings::~HgiMetalResourceBindings() = default;

void
HgiMetalResourceBindings::BindResources(
    id<MTLRenderCommandEncoder> renderEncoder)
{
    //
    // Bind Textures and Samplers
    //

    for (HgiTextureBindDesc const& texDesc : _descriptor.textures) {
        if (!TF_VERIFY(texDesc.textures.size() == 1)) continue;

        HgiTextureHandle const& texHandle = texDesc.textures.front();
        HgiMetalTexture* metalTexture =
            static_cast<HgiMetalTexture*>(texHandle.Get());

        HgiSamplerHandle const& smpHandle = texDesc.samplers.front();
        HgiMetalSampler* metalSmp =
            static_cast<HgiMetalSampler*>(smpHandle.Get());

        if (texDesc.stageUsage & HgiShaderStageVertex) {
            [renderEncoder setVertexTexture:metalTexture->GetTextureId()
                                    atIndex:texDesc.bindingIndex];
            [renderEncoder setVertexSamplerState:metalSmp->GetSamplerId()
                                    atIndex:texDesc.bindingIndex];
        }
        if (texDesc.stageUsage & HgiShaderStageFragment) {
            [renderEncoder setFragmentTexture:metalTexture->GetTextureId()
                                      atIndex:texDesc.bindingIndex];
            [renderEncoder setFragmentSamplerState:metalSmp->GetSamplerId()
                                    atIndex:texDesc.bindingIndex];
        }
    }

    //
    // Bind Buffers
    //

    // Note that index and vertex buffers are not bound here.
    // They are bound via the GraphicsEncoder.

    for (HgiBufferBindDesc const& bufDesc : _descriptor.buffers) {
        if (!TF_VERIFY(bufDesc.buffers.size() == 1)) continue;

        uint32_t unit = bufDesc.bindingIndex;

        HgiBufferHandle const& bufHandle = bufDesc.buffers.front();
        HgiMetalBuffer* metalbuffer =
            static_cast<HgiMetalBuffer*>(bufHandle.Get());

        id<MTLBuffer> h = metalbuffer->GetBufferId();
        NSUInteger offset = bufDesc.offsets.front();
        
        if (bufDesc.stageUsage & HgiShaderStageVertex) {
            [renderEncoder setVertexBuffer:h offset:offset atIndex:unit];
        }
        if (bufDesc.stageUsage & HgiShaderStageFragment) {
            [renderEncoder setFragmentBuffer:h offset:offset atIndex:unit];
        }
    }
}

void
HgiMetalResourceBindings::BindResources(
    id<MTLComputeCommandEncoder> computeEncoder)
{
    //
    // Bind Textures and Samplers
    //

    for (HgiTextureBindDesc const& texDesc : _descriptor.textures) {
        if (!TF_VERIFY(texDesc.textures.size() == 1)) continue;

        HgiTextureHandle const& texHandle = texDesc.textures.front();
        HgiMetalTexture* metalTexture =
            static_cast<HgiMetalTexture*>(texHandle.Get());

        HgiSamplerHandle const& smpHandle = texDesc.samplers.front();
        HgiMetalSampler* metalSmp =
            static_cast<HgiMetalSampler*>(smpHandle.Get());

        if (texDesc.stageUsage & HgiShaderStageCompute) {
            [computeEncoder setTexture:metalTexture->GetTextureId()
                               atIndex:texDesc.bindingIndex];
            [computeEncoder setSamplerState:metalSmp->GetSamplerId()
                                    atIndex:texDesc.bindingIndex];
        }
    }

    //
    // Bind Buffers
    //

    // Note that index and vertex buffers are not bound here.
    // They are bound via the GraphicsEncoder.

    for (HgiBufferBindDesc const& bufDesc : _descriptor.buffers) {
        if (!TF_VERIFY(bufDesc.buffers.size() == 1)) continue;

        uint32_t unit = bufDesc.bindingIndex;

        HgiBufferHandle const& bufHandle = bufDesc.buffers.front();
        HgiMetalBuffer* metalbuffer =
            static_cast<HgiMetalBuffer*>(bufHandle.Get());

        id<MTLBuffer> h = metalbuffer->GetBufferId();
        NSUInteger offset = bufDesc.offsets.front();
        
        if (bufDesc.stageUsage & HgiShaderStageCompute) {
            [computeEncoder setBuffer:h offset:offset atIndex:unit];
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
