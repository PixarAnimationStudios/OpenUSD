//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// ported from https://github.com/toji/web-texture-tool/blob/main/src/webgpu-mipmap-generator.js

#include "pxr/imaging/hgiWebGPU/mipmapGenerator.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUMipmapGenerator::HgiWebGPUMipmapGenerator(wgpu::Device const& device)
    : _device(device)
    , _mipmapShaderModule(nullptr)
{
    wgpu::SamplerDescriptor samplerDsc = {};
    samplerDsc.minFilter = wgpu::FilterMode::Linear;
    _sampler = _device.CreateSampler(&samplerDsc);
}

HgiWebGPUMipmapGenerator::~HgiWebGPUMipmapGenerator()
{
    _mipmapShaderModule = nullptr;
}

wgpu::RenderPipeline
HgiWebGPUMipmapGenerator::_getMipmapPipeline(wgpu::TextureFormat const& format)
{
    auto pipelineIt = _pipelines.find(format);
    if (pipelineIt == _pipelines.end()) {
        // Shader modules is shared between all pipelines, so only create once.
        if (!_mipmapShaderModule) {
            wgpu::ShaderSourceWGSL wgslDesc = {};
            wgslDesc.code = R"(
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
            _mipmapShaderModule =
                _device.CreateShaderModule(&mipmapShaderModuleDsc);
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

        std::vector<wgpu::BindGroupLayoutEntry> entries{samplerBGL, textureBGL};
        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor;
        bindGroupLayoutDescriptor.label = "mipmapGeneratorBGL";
        bindGroupLayoutDescriptor.entryCount = 2;
        bindGroupLayoutDescriptor.entries = entries.data();

        wgpu::BindGroupLayout bindGroupLayout =
            _device.CreateBindGroupLayout(&bindGroupLayoutDescriptor);
        wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
        pipelineLayoutDesc.bindGroupLayoutCount = 1;
        pipelineLayoutDesc.bindGroupLayouts = &bindGroupLayout;
        wgpu::PipelineLayout pipelineLayout =
            _device.CreatePipelineLayout(&pipelineLayoutDesc);

        pipelineDsc.layout = pipelineLayout;

        pipelineDsc.fragment = &fragmentState;
        wgpu::RenderPipeline newFormatPipeline =
            _device.CreateRenderPipeline(&pipelineDsc);
        _pipelines.emplace(format, newFormatPipeline);
        return newFormatPipeline;
    }
    return pipelineIt->second;
}
wgpu::Texture
HgiWebGPUMipmapGenerator::generateMipmap(
    const wgpu::Texture& texture, const HgiTextureDesc& textureDescriptor)
{
    const wgpu::TextureDimension dimension =
        HgiWebGPUConversions::GetTextureType(textureDescriptor.type);

    if (dimension == wgpu::TextureDimension::e3D ||
        dimension == wgpu::TextureDimension::e1D) {
        TF_WARN(
            "Generating mipmaps for non-2d textures is currently unsupported!");
        return texture;
    }

    wgpu::Texture mipTexture = texture;
    const wgpu::TextureFormat format =
        HgiWebGPUConversions::GetPixelFormat(textureDescriptor.format);
    const uint32_t mipLevelCount = textureDescriptor.mipLevels;
    const int32_t width = textureDescriptor.dimensions[0];
    const int32_t height = textureDescriptor.dimensions[1];
    if (width == 1 || height == 1 || mipLevelCount == 1) {
        return texture;
    }

    const uint32_t arrayLayerCount = textureDescriptor.layerCount;
    const wgpu::RenderPipeline pipeline = _getMipmapPipeline(format);
    const bool renderToSource =
        textureDescriptor.usage & HgiTextureUsageBitsColorTarget ||
        textureDescriptor.usage & HgiTextureUsageBitsDepthTarget;

    // If the texture was created with RENDER_ATTACHMENT usage we can render
    // directly between mip levels.
    if (!renderToSource) {
        // Otherwise we have to use a separate texture to render into. It can be
        // one mip level smaller than the source texture, since we already have
        // the top level.
        wgpu::TextureDescriptor mipTextureDescriptor = {};
        mipTextureDescriptor.size.width = std::max(width / 2, 1);
        mipTextureDescriptor.size.height = std::max(height / 2, 1);
        mipTextureDescriptor.size.depthOrArrayLayers = arrayLayerCount;
        mipTextureDescriptor.format = format;
        mipTextureDescriptor.usage = wgpu::TextureUsage::TextureBinding |
            wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::RenderAttachment;
        mipTextureDescriptor.mipLevelCount = mipLevelCount - 1;
        mipTexture = _device.CreateTexture(&mipTextureDescriptor);
    }

    const wgpu::CommandEncoder commandEncoder =
        _device.CreateCommandEncoder({});
    // TODO: Consider making this static.
    const wgpu::BindGroupLayout bindGroupLayout =
        pipeline.GetBindGroupLayout(0);

    for (uint32_t arrayLayer = 0; arrayLayer < arrayLayerCount; ++arrayLayer) {
        wgpu::TextureViewDescriptor textureViewDesc = {};
        textureViewDesc.baseMipLevel = 0;
        textureViewDesc.mipLevelCount = 1;
        textureViewDesc.dimension = wgpu::TextureViewDimension::e2D;
        textureViewDesc.baseArrayLayer = arrayLayer;
        textureViewDesc.arrayLayerCount = 1;

        wgpu::TextureView srcView = texture.CreateView(&textureViewDesc);

        uint32_t dstMipLevel = renderToSource ? 1 : 0;

        for (uint32_t i = 1; i < mipLevelCount; ++i) {
            wgpu::TextureViewDescriptor dstViewDesc = {};
            dstViewDesc.baseMipLevel = dstMipLevel++;
            dstViewDesc.mipLevelCount = 1;
            dstViewDesc.dimension = wgpu::TextureViewDimension::e2D;
            dstViewDesc.baseArrayLayer = arrayLayer;
            dstViewDesc.arrayLayerCount = 1;
            const wgpu::TextureView dstView =
                mipTexture.CreateView(&dstViewDesc);

            wgpu::RenderPassDescriptor passDesc = {};
            passDesc.colorAttachmentCount = 1;
            wgpu::RenderPassColorAttachment colorAttachment = {};
            colorAttachment.view = dstView;
            colorAttachment.loadOp = wgpu::LoadOp::Clear;
            colorAttachment.storeOp = wgpu::StoreOp::Store;

            passDesc.colorAttachments = &colorAttachment;
            const wgpu::RenderPassEncoder passEncoder =
                commandEncoder.BeginRenderPass(&passDesc);

            wgpu::BindGroupEntry samplerEntry = {};
            samplerEntry.sampler = _sampler;
            samplerEntry.binding = 0;

            wgpu::BindGroupEntry textureEntry = {};
            textureEntry.textureView = srcView;
            textureEntry.binding = 1;

            const std::vector<wgpu::BindGroupEntry> entries = {
                samplerEntry, textureEntry};

            wgpu::BindGroupDescriptor bindGroupDsc = {};
            std::string bindGroupDscLabel = "Mipmap BindGroupDescriptor";
            bindGroupDsc.label = bindGroupDscLabel.c_str();
            bindGroupDsc.layout = bindGroupLayout;
            bindGroupDsc.entryCount = entries.size();
            bindGroupDsc.entries = entries.data();
            const wgpu::BindGroup bindGroup =
                _device.CreateBindGroup(&bindGroupDsc);

            passEncoder.SetPipeline(pipeline);
            passEncoder.SetBindGroup(0, bindGroup);
            passEncoder.Draw(3, 1, 0, 0);
            passEncoder.End();

            srcView = dstView;
        }
    }

    // If we didn't render to the source texture, finish by copying the mip
    // results from the temporary mipmap texture to the source.
    if (!renderToSource) {
        wgpu::Extent3D mipLevelSize = {};
        mipLevelSize.width = std::max(1, width / 2);
        mipLevelSize.height = std::max(1, height / 2);
        mipLevelSize.depthOrArrayLayers = arrayLayerCount;

        for (uint32_t i = 1; i < mipLevelCount; ++i) {
            wgpu::TexelCopyTextureInfo imageCopyTextureSrc = {};
            imageCopyTextureSrc.texture = mipTexture;
            imageCopyTextureSrc.mipLevel = i - 1;
            wgpu::TexelCopyTextureInfo imageCopyTextureDst = {};
            imageCopyTextureDst.texture = texture;
            imageCopyTextureDst.mipLevel = i;
            commandEncoder.CopyTextureToTexture(
                &imageCopyTextureSrc, &imageCopyTextureDst, &mipLevelSize);

            mipLevelSize.width =
                std::max(1u, mipLevelSize.width / 2);
            mipLevelSize.height =
                std::max(1u, mipLevelSize.height / 2);
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
