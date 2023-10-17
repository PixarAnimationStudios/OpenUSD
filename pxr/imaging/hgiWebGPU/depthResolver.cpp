//
// Copyright 2023 Pixar
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

// Ported from https://github.com/playcanvas/engine/blob/main/src/platform/graphics/webgpu/webgpu-resolver.js

#include "pxr/imaging/hgiWebGPU/depthResolver.h"
#include "pxr/imaging/hgiWebGPU/texture.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"

#include <webgpu/webgpu_cpp.h>

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUDepthResolver::HgiWebGPUDepthResolver(wgpu::Device const &device)
        : _device(device) {

}

HgiWebGPUDepthResolver::~HgiWebGPUDepthResolver() {
    if (_resolverShaderModule) {
        _resolverShaderModule = nullptr;
    }
}

wgpu::RenderPipeline HgiWebGPUDepthResolver::_getResolverPipeline(wgpu::TextureFormat const &format) {
    auto pipelineIt = _pipelines.find(format);
    if (pipelineIt == _pipelines.end()) {
        // Shader modules is shared between all pipelines, so only create once.
        if (!_resolverShaderModule) {
            wgpu::ShaderModuleWGSLDescriptor wgslDesc = {};
            wgslDesc.code = R"(
            var<private> pos : array<vec2f, 4> = array<vec2f, 4>(
                vec2(-1.0, 1.0), vec2(1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, -1.0)
            );

            @vertex
            fn vertexMain(@builtin(vertex_index) vertexIndex : u32) -> @builtin(position) vec4f {
              return vec4f(pos[vertexIndex], 0, 1);
            }

            @group(0) @binding(0) var img : texture_depth_multisampled_2d;

            @fragment
            fn fragmentMain(@builtin(position) fragColor: vec4f) -> @builtin(frag_depth) f32 {
                // load the depth value from sample index 0
                return textureLoad(img, vec2i(fragColor.xy), 0u);
            })";
            wgpu::ShaderModuleDescriptor depthResolverShaderModuleDsc = {};
            depthResolverShaderModuleDsc.nextInChain = &wgslDesc;
            _resolverShaderModule = _device.CreateShaderModule(&depthResolverShaderModuleDsc);
        }

        wgpu::RenderPipelineDescriptor pipelineDsc = {};
        wgpu::VertexState vertexState = {};
        vertexState.module = _resolverShaderModule;
        vertexState.entryPoint = "vertexMain";
        pipelineDsc.vertex = vertexState;
        wgpu::FragmentState fragmentState = {};
        fragmentState.module = _resolverShaderModule;
        fragmentState.entryPoint = "fragmentMain";
        fragmentState.targetCount = 0;

        wgpu::DepthStencilState depthStencilDesc{};
        depthStencilDesc.format = format;
        depthStencilDesc.depthWriteEnabled = true;
        depthStencilDesc.depthCompare = wgpu::CompareFunction::Always;
        pipelineDsc.depthStencil = &depthStencilDesc;

        pipelineDsc.fragment = &fragmentState;
        pipelineDsc.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;
        pipelineDsc.label = "RenderPipeline-DepthResolver";
        wgpu::RenderPipeline newFormatPipeline = _device.CreateRenderPipeline(&pipelineDsc);
        _pipelines.emplace(format, newFormatPipeline);
        return newFormatPipeline;

    }
    return pipelineIt->second;
}

void HgiWebGPUDepthResolver::resolveDepth(wgpu::CommandEncoder const &commandEncoder, HgiWebGPUTexture &sourceTexture,
                                     HgiWebGPUTexture &destinationTexture) {

    HgiTextureDesc const &sourceTextureDesc = sourceTexture.GetDescriptor();
    wgpu::Texture const &wgpuSourceTexture = sourceTexture.GetTextureHandle();
    HgiTextureDesc const &destinationTextureDesc = destinationTexture.GetDescriptor();
    wgpu::Texture const &wgpuDestTexture = destinationTexture.GetTextureHandle();
    TF_VERIFY(sourceTextureDesc.sampleCount > 1);
    TF_VERIFY(destinationTextureDesc.sampleCount == 1);
    TF_VERIFY(sourceTextureDesc.layerCount == destinationTextureDesc.layerCount);
    // Since the view is bound to the texture we only are able to support 1 layer
    TF_VERIFY(sourceTextureDesc.layerCount == 1);

    wgpu::TextureFormat format = HgiWebGPUConversions::GetDepthOrStencilTextureFormat(
            destinationTextureDesc.usage, destinationTextureDesc.format);

    // pipeline depends on the format
    const wgpu::RenderPipeline pipeline = _getResolverPipeline(format);

    commandEncoder.PushDebugGroup("DEPTH_RESOLVE-RENDERER");

    // copy depth only (not stencil)
    wgpu::TextureViewDescriptor srcViewDesc{};
    srcViewDesc.dimension = wgpu::TextureViewDimension::e2D;
    srcViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
    srcViewDesc.baseMipLevel = 0;
    srcViewDesc.mipLevelCount = 1;
    srcViewDesc.baseArrayLayer = 0;

    wgpu::TextureView srcView = wgpuSourceTexture.CreateView(&srcViewDesc);

    wgpu::TextureViewDescriptor dstViewDesc{};
    dstViewDesc.dimension = wgpu::TextureViewDimension::e2D;
    dstViewDesc.baseMipLevel = 0;
    dstViewDesc.mipLevelCount = 1;
    dstViewDesc.baseArrayLayer = 0;

    wgpu::TextureView dstView = wgpuDestTexture.CreateView(&dstViewDesc);

    wgpu::RenderPassDescriptor passDesc{};
    passDesc.colorAttachmentCount = 0;
    wgpu::RenderPassDepthStencilAttachment depthStencilDesc;
    depthStencilDesc.depthLoadOp = wgpu::LoadOp::Clear;
    depthStencilDesc.depthStoreOp = wgpu::StoreOp::Store;
    depthStencilDesc.depthClearValue = 0.0f;
    depthStencilDesc.view = dstView;
    passDesc.depthStencilAttachment = &depthStencilDesc;

    passDesc.label = "DepthResolver-PassEncoder";
    const wgpu::RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&passDesc);

    wgpu::BindGroupEntry srcViewEntry;
    srcViewEntry.binding = 0;
    srcViewEntry.textureView = srcView;

    wgpu::BindGroupDescriptor bindGroupDsc = {};
    std::string bindGroupDscLabel = "DepthResolver-BindGroupDescriptor";
    bindGroupDsc.label = bindGroupDscLabel.c_str();
    bindGroupDsc.layout = pipeline.GetBindGroupLayout(0);
    bindGroupDsc.entryCount = 1;
    bindGroupDsc.entries = &srcViewEntry;
    // no need for a sampler when using textureLoad
    wgpu::BindGroup bindGroup = _device.CreateBindGroup(&bindGroupDsc);
    passEncoder.SetPipeline(pipeline);
    passEncoder.SetBindGroup(0, bindGroup);
    passEncoder.Draw(4);
    passEncoder.End();

    commandEncoder.PopDebugGroup();
}

PXR_NAMESPACE_CLOSE_SCOPE