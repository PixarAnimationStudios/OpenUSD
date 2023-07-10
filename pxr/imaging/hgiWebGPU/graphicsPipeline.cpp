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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/debugCodes.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/texture.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

wgpu::StencilFaceState _GetStencilFaceState(const HgiStencilState &hgiStencilState) {
    wgpu::StencilFaceState stencilFaceState;
    stencilFaceState.compare = HgiWebGPUConversions::GetCompareFunction(hgiStencilState.compareFn);
    stencilFaceState.failOp = HgiWebGPUConversions::GetStencilOp(hgiStencilState.stencilFailOp);
    stencilFaceState.depthFailOp = HgiWebGPUConversions::GetStencilOp(hgiStencilState.depthFailOp);
    stencilFaceState.passOp = HgiWebGPUConversions::GetStencilOp(hgiStencilState.depthStencilPassOp);
    return stencilFaceState;
}

wgpu::BindGroupLayout _CreateBindGroupLayout(
        const wgpu::Device &device,
        const std::unordered_map<uint32_t ,wgpu::BindGroupLayoutEntry> &bindGroupLayoutEntries,
        const std::string &debugName) {
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
    bindGroupLayoutDesc.label = debugName.c_str();
    bindGroupLayoutDesc.entryCount = bindGroupLayoutEntries.size();
    std::vector<wgpu::BindGroupLayoutEntry> entries;
    entries.reserve(bindGroupLayoutEntries.size());
    for(auto const &e : bindGroupLayoutEntries) {
        entries.push_back(e.second);
    }
    bindGroupLayoutDesc.entries = entries.data();
    return device.CreateBindGroupLayout(&bindGroupLayoutDesc);
}

HgiWebGPUGraphicsPipeline::HgiWebGPUGraphicsPipeline(
    HgiWebGPU *hgi,
    HgiGraphicsPipelineDesc const& desc)
    : HgiGraphicsPipeline(desc)
    , _pipeline(nullptr)
{
    wgpu::Device device = hgi->GetPrimaryDevice();
    // get the shaders for this pipeline
    HgiShaderFunctionHandleVector const& sfv =
        desc.shaderProgram->GetShaderFunctions();

    _shaderStates.resize(_shaderStates.size()+1);
    auto &shaderState = *_shaderStates.rbegin();

    // collect all the bind group layout entries and merge visibility
    // the key to this sorted list is the binding group set
    BindGroupsLayoutMap bindGroupLayouts;
    for (HgiShaderFunctionHandle const& sf : sfv)
    {
        HgiWebGPUShaderFunction const* shaderFn =
            static_cast<HgiWebGPUShaderFunction const*>(sf.Get());
        const BindGroupsLayoutMap &shaderBindGroupsEntries = shaderFn->GetBindGroups();
        if (bindGroupLayouts.size() == 0) {
            bindGroupLayouts.insert(shaderBindGroupsEntries.begin(), shaderBindGroupsEntries.end());
        } else if (bindGroupLayouts.size() != shaderBindGroupsEntries.size()) {
            TF_CODING_ERROR("Shader function number of binding groups doesnt match the expected size");
        } else {
            for (const auto & [bindGroup, bindGroupEntries] : shaderBindGroupsEntries) {
                BindGroupLayoutEntryMap &accVisibilityEntries = bindGroupLayouts[bindGroup];
                for (const auto & [bindingIndex, e]: bindGroupEntries) {
                    auto entrySearch = accVisibilityEntries.find(bindingIndex);
                    if (entrySearch != accVisibilityEntries.end()) {
                        entrySearch->second.visibility |= e.visibility;
                    } else {
                        accVisibilityEntries.emplace(e.binding, e);
                    }
                }
            }
        }

        const auto &shaderStage = shaderFn->GetDescriptor().shaderStage;
        if( shaderStage == HgiShaderStageVertex )
        {
            shaderState.vertexState.module = shaderFn->GetShaderModule();
            shaderState.vertexState.entryPoint = shaderFn->GetShaderEntryPoint();
        }
        else if( shaderStage == HgiShaderStageFragment )
        {
            shaderState.fragmentState.module = shaderFn->GetShaderModule();
            shaderState.fragmentState.entryPoint = shaderFn->GetShaderEntryPoint();
        } else {
            TF_CODING_ERROR("Shader stages other than vertex and fragment are not currently supported.");
        }
    }

    for (auto &bglEntries: bindGroupLayouts) {
        wgpu::BindGroupLayout bindGroupLayout = _CreateBindGroupLayout(
                hgi->GetPrimaryDevice(),
                bglEntries.second,
                std::string("BindGroup" + desc.debugName));
        _bindGroupLayoutList.push_back(bindGroupLayout);
    }

	wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
	pipelineLayoutDesc.bindGroupLayoutCount = _bindGroupLayoutList.size();
	pipelineLayoutDesc.bindGroupLayouts = _bindGroupLayoutList.data();
	wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

	// setup the pipeline
	wgpu::RenderPipelineDescriptor pipelineDesc;
	pipelineDesc.layout = pipelineLayout;

    wgpu::DepthStencilState depthStencilDesc{};
    if( desc.depthAttachmentDesc.format != HgiFormatInvalid )
    {
        depthStencilDesc.format = HgiWebGPUConversions::GetDepthOrStencilTextureFormat(desc.depthAttachmentDesc.usage, desc.depthAttachmentDesc.format);
        depthStencilDesc.depthWriteEnabled = desc.depthState.depthWriteEnabled;
        depthStencilDesc.depthCompare = HgiWebGPUConversions::GetCompareFunction(desc.depthState.depthCompareFn);
        depthStencilDesc.stencilBack = _GetStencilFaceState(desc.depthState.stencilBack);
        depthStencilDesc.stencilFront = _GetStencilFaceState(desc.depthState.stencilFront);
        // TODO: Should it be desc.depthState.stencilFront or desc.depthState.stencilBack?
        depthStencilDesc.stencilReadMask = desc.depthState.stencilFront.readMask;
        depthStencilDesc.stencilWriteMask = desc.depthState.stencilFront.readMask;
        pipelineDesc.depthStencil = &depthStencilDesc;
    }

    // setup the vertex buffer layout(s)
    std::vector<wgpu::VertexBufferLayout> vertexBufferDescriptors;

    // hold onto these until pipeline creation
    std::vector<std::vector<wgpu::VertexAttribute>> vertAttrArray;

    for (HgiVertexBufferDesc const& vbo : desc.vertexBuffers) {
        wgpu::VertexBufferLayout vib;

        if (vbo.vertexStepFunction == HgiVertexBufferStepFunctionPerVertex) {
            vib.arrayStride = vbo.vertexStride;
            vib.stepMode = wgpu::VertexStepMode::Vertex;
        } else if (vbo.vertexStepFunction == HgiVertexBufferStepFunctionPerInstance) {
            vib.arrayStride = vbo.vertexStride;
            vib.stepMode = wgpu::VertexStepMode::Instance;
        } else if (vbo.vertexStepFunction == HgiVertexBufferStepFunctionPerDrawCommand
                   || vbo.vertexStepFunction == HgiVertexBufferStepFunctionConstant) {
            vib.arrayStride = 0;
            vib.stepMode = wgpu::VertexStepMode::Vertex;
        } else {
            TF_WARN("Step function not implemented for WebGPU");
            vib.arrayStride = vbo.vertexStride;
            vib.stepMode = wgpu::VertexStepMode::Vertex;
        }

        TF_DEBUG(HGIWEBGPU_DEBUG_GRAPHICS_PIPELINE).Msg("HgiWebGPUGraphicsPipeline:Processing buffer[%lu]\n"
                                                        "stride: %llu\n", vertAttrArray.size(), vib.arrayStride);
        vertAttrArray.resize(vertAttrArray.size() + 1);
        std::vector<wgpu::VertexAttribute> &vertAttrs = vertAttrArray[vertAttrArray.size()-1];
        for (HgiVertexAttributeDesc const& va : vbo.vertexAttributes) {
            wgpu::VertexAttribute ad;
            ad.shaderLocation = va.shaderBindLocation;
            ad.offset = va.offset;
            ad.format = HgiWebGPUConversions::GetVertexFormat(va.format);
            TF_DEBUG(HGIWEBGPU_DEBUG_GRAPHICS_PIPELINE)
                .Msg("\tAttribute[%lu]:\n"
                     "\tshaderLocation: %u \n"
                     "\toffset: %u \n"
                     "\tformat: %u \n",
                     vertAttrs.size(),
                     ad.shaderLocation,
                     ad.offset,
                     ad.format
                     );
            vertAttrs.push_back(std::move(ad));
        }

        vib.attributeCount = vertAttrs.size();
        vib.attributes = vertAttrs.data();
        vertexBufferDescriptors.push_back(std::move(vib));
    }

	shaderState.vertexState.bufferCount = vertexBufferDescriptors.size();
	shaderState.vertexState.buffers = vertexBufferDescriptors.data();

	pipelineDesc.primitive.topology = HgiWebGPUConversions::GetPrimitiveTopology(desc.primitiveType);
    pipelineDesc.primitive.frontFace =HgiWebGPUConversions::GetWinding(desc.rasterizationState.winding);
    pipelineDesc.primitive.cullMode = HgiWebGPUConversions::GetCullMode(desc.rasterizationState.cullMode);

    wgpu::MultisampleState multisampleState;
    if (desc.multiSampleState.multiSampleEnable) {
        multisampleState.count = desc.multiSampleState.sampleCount;
        multisampleState.alphaToCoverageEnabled = desc.multiSampleState.alphaToCoverageEnable;
        pipelineDesc.multisample = multisampleState;
    }

    std::vector<wgpu::ColorTargetState> colorDescriptors;
    for( auto &ct : desc.colorAttachmentDescs )
    {
        wgpu::ColorTargetState colorDesc;
        colorDesc.format = HgiWebGPUConversions::GetPixelFormat(ct.format);

        if (ct.blendEnabled) {
            wgpu::BlendComponent blendAlphaDesc;
            blendAlphaDesc.operation = HgiWebGPUConversions::GetBlendEquation(ct.alphaBlendOp);
            blendAlphaDesc.srcFactor = HgiWebGPUConversions::GetBlendFactor(ct.srcAlphaBlendFactor);
            blendAlphaDesc.dstFactor = HgiWebGPUConversions::GetBlendFactor(ct.dstAlphaBlendFactor);

            wgpu::BlendComponent blendColorDesc;
            blendColorDesc.operation = HgiWebGPUConversions::GetBlendEquation(ct.colorBlendOp);
            blendColorDesc.srcFactor = HgiWebGPUConversions::GetBlendFactor(ct.srcColorBlendFactor);
            blendColorDesc.dstFactor = HgiWebGPUConversions::GetBlendFactor(ct.dstColorBlendFactor);

            wgpu::BlendState blendState;
            blendState.color = blendColorDesc;
            blendState.alpha = blendAlphaDesc;

            colorDesc.blend = &blendState;
        }

        colorDescriptors.push_back(colorDesc);
    }

	shaderState.fragmentState.targetCount = colorDescriptors.size();
	shaderState.fragmentState.targets = colorDescriptors.data();

	pipelineDesc.vertex = shaderState.vertexState;
	pipelineDesc.fragment = &shaderState.fragmentState;
    _pipeline = device.CreateRenderPipeline(&pipelineDesc);
}

HgiWebGPUGraphicsPipeline::~HgiWebGPUGraphicsPipeline()
{
}


wgpu::RenderPipeline HgiWebGPUGraphicsPipeline::GetPipeline() const
{
    return _pipeline;
}

const std::vector<wgpu::BindGroupLayout>& HgiWebGPUGraphicsPipeline::GetBindGroupLayoutList() const
{
    return _bindGroupLayoutList;
}

PXR_NAMESPACE_CLOSE_SCOPE
