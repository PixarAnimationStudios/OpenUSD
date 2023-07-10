//
// Copyright 2022 Pixar
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
// ported from https://github.com/toji/web-texture-tool/blob/main/src/webgpu-mipmap-generator.js

#include "mipmapGenerator.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include <vector>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

WebGPUMipmapGenerator::WebGPUMipmapGenerator(wgpu::Device const &device)
    :_device(device),
      _mipmapShaderModule(nullptr)
{
    wgpu::SamplerDescriptor samplerDsc = {};
    samplerDsc.minFilter = wgpu::FilterMode::Linear;
    _sampler = _device.CreateSampler(&samplerDsc);
}

WebGPUMipmapGenerator::~WebGPUMipmapGenerator()
{
    if( _mipmapShaderModule ) {
        _mipmapShaderModule.Release();
    }
    _sampler.Release();
    // TODO: Should we release all pipelines?

}

wgpu::RenderPipeline WebGPUMipmapGenerator::_getMipmapPipeline(wgpu::TextureFormat const &format) {
    auto pipelineIt = _pipelines.find(format);
    if (pipelineIt == _pipelines.end()) {
        // Shader modules is shared between all pipelines, so only create once.
        if (!_mipmapShaderModule) {
            wgpu::ShaderModuleWGSLDescriptor wgslDesc = {};
            wgslDesc.source = R"(
            var<private> pos : array<vec2<f32>, 3> = array<vec2<f32>, 3>(
                          vec2<f32>(-1.0, -1.0), vec2<f32>(-1.0, 3.0), vec2<f32>(3.0, -1.0));
            struct VertexOutput {
                @builtin(position) position : vec4<f32>,
                                              @location(0) texCoord : vec2<f32>,
            };
            @vertex
                fn vertexMain(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
                var output : VertexOutput;
                output.texCoord = pos[vertexIndex] * vec2<f32>(0.5, -0.5) + vec2<f32>(0.5);
                output.position = vec4<f32>(pos[vertexIndex], 0.0, 1.0);
                return output;
            }
            @group(0) @binding(0) var imgSampler : sampler;
            @group(0) @binding(1) var img : texture_2d<f32>;
            @fragment
                fn fragmentMain(@location(0) texCoord : vec2<f32>) -> @location(0) vec4<f32> {
                return textureSample(img, imgSampler, texCoord);
            })";
            wgpu::ShaderModuleDescriptor mipmapShaderModuleDsc = {};
            mipmapShaderModuleDsc.nextInChain = &wgslDesc;
            _mipmapShaderModule = _device.CreateShaderModule(&mipmapShaderModuleDsc);
        }

        wgpu::RenderPipelineDescriptor pipelineDsc = {};
        wgpu::VertexState vertexState = {};
        vertexState.module = _mipmapShaderModule;
        vertexState.entryPoint = "vertexMain";
        pipelineDsc.vertex = vertexState;
        wgpu::FragmentState fragmentState = {};
        fragmentState.module = _mipmapShaderModule;
        fragmentState.entryPoint = "fragmentMain";
        wgpu::ColorTargetState colorDesc = {};
        colorDesc.format = format;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorDesc;

        wgpu::BindGroupLayoutEntry samplerBGL;
        samplerBGL.visibility = wgpu::ShaderStage::Fragment;
        samplerBGL.binding = 0;
        samplerBGL.sampler.type = wgpu::SamplerBindingType::Filtering;
        wgpu::BindGroupLayoutEntry textureBGL;
        textureBGL.visibility = wgpu::ShaderStage::Fragment;
        textureBGL.texture.sampleType = wgpu::TextureSampleType::Float;
        textureBGL.binding = 1;

        std::vector<wgpu::BindGroupLayoutEntry> entries {
                samplerBGL,
                textureBGL
        };
        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor;
        bindGroupLayoutDescriptor.label = "mipmapGeneratorBGL";
        bindGroupLayoutDescriptor.entryCount = 2;
        bindGroupLayoutDescriptor.entries = entries.data();

        wgpu::BindGroupLayout bindGroupLayout = _device.CreateBindGroupLayout(&bindGroupLayoutDescriptor);
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        wgpu::PipelineLayout pipelineLayout = _device.CreatePipelineLayout(&pipelineLayoutDesc);

        pipelineDsc.layout = pipelineLayout;

        pipelineDsc.fragment = &fragmentState;
        wgpu::RenderPipeline newFormatPipeline = _device.CreateRenderPipeline(&pipelineDsc);
        _pipelines.emplace(format, newFormatPipeline);
        return newFormatPipeline;

    }
    return pipelineIt->second;

}
wgpu::Texture WebGPUMipmapGenerator::generateMipmap(const wgpu::Texture& texture, const HgiTextureDesc& textureDescriptor) {
    const wgpu::TextureDimension dimension = HgiWebGPUConversions::GetTextureType(textureDescriptor.type);

    if (dimension ==  wgpu::TextureDimension::e3D || dimension == wgpu::TextureDimension::e1D) {
        TF_WARN("Generating mipmaps for non-2d textures is currently unsupported!");
        return texture;
    }

    wgpu::Texture mipTexture = texture;
    const wgpu::TextureFormat format = HgiWebGPUConversions::GetPixelFormat(textureDescriptor.format);
    const uint32_t  mipLevelCount =  textureDescriptor.mipLevels;
    const int32_t width = textureDescriptor.dimensions[0];
    const int32_t height = textureDescriptor.dimensions[1];
    const uint32_t arrayLayerCount = textureDescriptor.layerCount;
    const wgpu::RenderPipeline pipeline = _getMipmapPipeline(format);
    const bool renderToSource = textureDescriptor.usage & HgiTextureUsageBitsColorTarget || textureDescriptor.usage & HgiTextureUsageBitsDepthTarget;

    // If the texture was created with RENDER_ATTACHMENT usage we can render directly between mip levels.
    if (!renderToSource) {
        // Otherwise we have to use a separate texture to render into. It can be one mip level smaller than the source
        // texture, since we already have the top level.
        wgpu::TextureDescriptor mipTextureDescriptor = {};
        mipTextureDescriptor.size.width = std::ceil(width / 2);
        mipTextureDescriptor.size.height = std::ceil(height / 2);
        mipTextureDescriptor.size.depthOrArrayLayers = arrayLayerCount;
        mipTextureDescriptor.format = format;
        mipTextureDescriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::RenderAttachment;
        mipTextureDescriptor.mipLevelCount = mipLevelCount - 1;
        mipTexture = _device.CreateTexture(&mipTextureDescriptor);
    }

    const wgpu::CommandEncoder commandEncoder = _device.CreateCommandEncoder({});
    // TODO: Consider making this static.
    const wgpu::BindGroupLayout bindGroupLayout = pipeline.GetBindGroupLayout(0);

    for (uint32_t arrayLayer = 0; arrayLayer < arrayLayerCount; ++arrayLayer) {
        wgpu::TextureViewDescriptor textureViewDesc = {};
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
        textureViewDesc.baseArrayLayer = arrayLayer;
        textureViewDesc.arrayLayerCount = 1;

        wgpu::TextureView srcView = texture.CreateView(&textureViewDesc);

        uint32_t dstMipLevel = renderToSource ? 1 : 0;

        wgpu::BindGroupEntry samplerEntry = {};
        samplerEntry.sampler = _sampler;
        samplerEntry.binding = 0;

        wgpu::BindGroupEntry textureEntry = {};
        textureEntry.textureView = srcView;
        textureEntry.binding = 1;

        const std::vector<wgpu::BindGroupEntry> entries = {
                samplerEntry,
                textureEntry
        };

        wgpu::BindGroupDescriptor bindGroupDsc = {};
        std::string bindGroupDscLabel = "Mipmap BindGroupDescriptor";
        bindGroupDsc.label = bindGroupDscLabel.c_str();
        bindGroupDsc.layout = bindGroupLayout;
        bindGroupDsc.entryCount = entries.size();
        bindGroupDsc.entries = entries.data();
        const wgpu::BindGroup bindGroup = _device.CreateBindGroup(&bindGroupDsc);

        for (uint32_t i = 1; i < mipLevelCount; ++i) {
            wgpu::TextureViewDescriptor dstViewDesc = {};
            dstViewDesc.baseMipLevel = dstMipLevel++;
            dstViewDesc.mipLevelCount = 1;
            dstViewDesc.dimension = wgpu::TextureViewDimension::e2D;
            dstViewDesc.baseArrayLayer = arrayLayer;
            dstViewDesc.arrayLayerCount = 1;
            const wgpu::TextureView dstView = mipTexture.CreateView(&dstViewDesc);

            wgpu::RenderPassDescriptor passDesc = {};
            passDesc.colorAttachmentCount = 1;
            wgpu::RenderPassColorAttachment colorAttachment = {};
            colorAttachment.view = dstView;
            colorAttachment.loadOp = wgpu::LoadOp::Clear;
            colorAttachment.storeOp = wgpu::StoreOp::Store;

            passDesc.colorAttachments = &colorAttachment;
            const wgpu::RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&passDesc);

            passEncoder.SetPipeline(pipeline);
            passEncoder.SetBindGroup(0, bindGroup);
            passEncoder.Draw(3, 1, 0, 0);
            passEncoder.End();

            srcView = dstView;
        }
        srcView.Release();
    }

    // If we didn't render to the source texture, finish by copying the mip results from the temporary mipmap texture
    // to the source.
    if (!renderToSource) {
        wgpu::Extent3D mipLevelSize = {};
        mipLevelSize.width = std::ceil(width / 2);
        mipLevelSize.height = std::ceil(height / 2);
        mipLevelSize.depthOrArrayLayers = arrayLayerCount;

        for (uint32_t i = 1; i < mipLevelCount; ++i) {
            wgpu::ImageCopyTexture imageCopyTextureSrc = {};
            imageCopyTextureSrc.texture = mipTexture;
            imageCopyTextureSrc.mipLevel = i - 1;
            wgpu::ImageCopyTexture imageCopyTextureDst = {};
            imageCopyTextureDst.texture = texture;
            imageCopyTextureDst.mipLevel = i;
            commandEncoder.CopyTextureToTexture(&imageCopyTextureSrc, &imageCopyTextureDst, &mipLevelSize);

            mipLevelSize.width = std::ceil(mipLevelSize.width / 2);
            mipLevelSize.height = std::ceil(mipLevelSize.height / 2);
        }
    }
    const wgpu::CommandBuffer commands = commandEncoder.Finish();
    _device.GetQueue().Submit(1, &commands);

    if (!renderToSource) {
        mipTexture.Destroy();
    }

    return texture;
}

PXR_NAMESPACE_CLOSE_SCOPE