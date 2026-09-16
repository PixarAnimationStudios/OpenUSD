//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/debugCodes.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/graphicsPipeline.h"
#include "pxr/imaging/hgiWebGPU/pipelineBindGroups.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/spirvTransforms.h"
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


HgiWebGPUGraphicsPipeline::HgiWebGPUGraphicsPipeline(
    HgiWebGPU *hgi,
    HgiGraphicsPipelineDesc const& desc)
    : HgiGraphicsPipeline(desc)
    , _pipeline(nullptr)
{
    if (desc.rasterizationState.polygonMode != HgiPolygonModeFill) {
        TF_WARN("Non-fill polygon mode is unsupported by WebGPU");
    }

    wgpu::Device device = hgi->GetPrimaryDevice();
    // get the shaders for this pipeline
    HgiShaderFunctionHandleVector const& sfv =
        desc.shaderProgram->GetShaderFunctions();

    _shaderStates.resize(_shaderStates.size()+1);
    auto &shaderState = *_shaderStates.rbegin();

    // collect all the bind group layout entries and merge visibility
    // the key to this sorted list is the binding group set
    BindGroupsLayoutMap bindGroupLayouts;
    HgiWebGPUShaderFunction const* fragmentShaderFn = nullptr;
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
        }
        else if( shaderStage == HgiShaderStageFragment )
        {
            shaderState.fragmentState.module = shaderFn->GetShaderModule();
            fragmentShaderFn = shaderFn;
        } else {
            TF_CODING_ERROR("Shader stages other than vertex and fragment are not currently supported.");
        }
    }

    wgpu::PipelineLayout pipelineLayout = _pipelineBindGroups.CreatePipelineLayout(
        device, bindGroupLayouts, "BindGroup" + desc.debugName);

    // setup the pipeline
    wgpu::RenderPipelineDescriptor pipelineDesc;
    pipelineDesc.layout = pipelineLayout;

    wgpu::DepthStencilState depthStencilDesc{};
    if( desc.depthAttachmentDesc.format != HgiFormatInvalid )
    {
        depthStencilDesc.format = HgiWebGPUConversions::GetDepthOrStencilTextureFormat(desc.depthAttachmentDesc.usage, desc.depthAttachmentDesc.format);
        depthStencilDesc.depthWriteEnabled = desc.depthState.depthWriteEnabled;
        if (desc.depthState.depthTestEnabled) {
            depthStencilDesc.depthCompare = HgiWebGPUConversions::GetCompareFunction(desc.depthState.depthCompareFn);
        } else {
            depthStencilDesc.depthCompare = HgiWebGPUConversions::GetCompareFunction(HgiCompareFunctionAlways);
        }
        depthStencilDesc.stencilBack = _GetStencilFaceState(desc.depthState.stencilBack);
        depthStencilDesc.stencilFront = _GetStencilFaceState(desc.depthState.stencilFront);
        // TODO: Should it be desc.depthState.stencilFront or desc.depthState.stencilBack?
        depthStencilDesc.stencilReadMask = desc.depthState.stencilFront.readMask;
        depthStencilDesc.stencilWriteMask = desc.depthState.stencilFront.readMask;
        pipelineDesc.depthStencil = &depthStencilDesc;

        if (desc.depthState.depthBiasEnabled && desc.primitiveType != HgiPrimitiveTypeLineList) {
            depthStencilDesc.depthBias = desc.depthState.depthBiasConstantFactor;
            depthStencilDesc.depthBiasSlopeScale = desc.depthState.depthBiasSlopeFactor;
        }
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
                     "\toffset: %llu \n"
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
    pipelineDesc.primitive.frontFace = HgiWebGPUConversions::GetWinding(desc.rasterizationState.winding);
    pipelineDesc.primitive.cullMode = HgiWebGPUConversions::GetCullMode(desc.rasterizationState.cullMode);

    wgpu::MultisampleState multisampleState;
    if (desc.multiSampleState.multiSampleEnable && desc.multiSampleState.sampleCount > 1) {
        uint32_t sampleCount = desc.multiSampleState.sampleCount;
        // Integer formats don't support multisampling in WebGPU; the whole render
        // pass falls back to sampleCount = 1 when any color target is integer.
        for (const auto& colorDesc : desc.colorAttachmentDescs) {
            const auto sampleType = HgiWebGPUConversions::GetTextureSampleType(colorDesc.format);
            if (sampleType == wgpu::TextureSampleType::Sint || sampleType == wgpu::TextureSampleType::Uint) {
                sampleCount = 1;
                break;
            }
        }
        if (sampleCount > 1) {
            multisampleState.count = sampleCount;
            multisampleState.alphaToCoverageEnabled = desc.multiSampleState.alphaToCoverageEnable;

            // WebGPU has no equivalent of GL_SAMPLE_ALPHA_TO_ONE, so it writes
            // the coverage alpha to the render target, making the final image
            // semi-transparent. We emulate it by patching the fragment shader
            // to write sample mask directly from alpha and override alpha to 1.
            wgpu::ShaderModule alphaToOneModule;
            if (desc.multiSampleState.alphaToCoverageEnable &&
                desc.multiSampleState.alphaToOneEnable && fragmentShaderFn) {
                std::vector<uint32_t> spirv =
                    fragmentShaderFn->GetSpirvBinary();
                if (ApplySpirvAlphaToOneEmulation(spirv, sampleCount)) {
                    alphaToOneModule =
                        HgiWebGPUShaderFunction::CreateShaderModuleFromSpirv(
                            device, spirv,
                            fragmentShaderFn->GetDescriptor().debugName +
                                "_alphaToOne_" + std::to_string(sampleCount) +
                                "x");
                    if (alphaToOneModule) {
                        shaderState.fragmentState.module = alphaToOneModule;
                        multisampleState.alphaToCoverageEnabled = false;
                    }
                } else {
                    TF_CODING_ERROR(
                        "ApplySpirvAlphaToOneEmulation transform failed");
                }
            }

            pipelineDesc.multisample = multisampleState;
        }
    }

    std::vector<wgpu::ColorTargetState> colorDescriptors;
    for( auto &ct : desc.colorAttachmentDescs )
    {
        wgpu::ColorTargetState colorDesc;
        colorDesc.format = HgiWebGPUConversions::GetPixelFormat(ct.format);
        colorDesc.writeMask =  HgiWebGPUConversions::GetColorWriteMask(ct.colorMask);

        wgpu::BlendState blendState;
        if (ct.blendEnabled) {
            wgpu::BlendComponent blendAlphaDesc;
            blendAlphaDesc.operation = HgiWebGPUConversions::GetBlendEquation(ct.alphaBlendOp);
            blendAlphaDesc.srcFactor = HgiWebGPUConversions::GetBlendFactor(ct.srcAlphaBlendFactor);
            blendAlphaDesc.dstFactor = HgiWebGPUConversions::GetBlendFactor(ct.dstAlphaBlendFactor);

            wgpu::BlendComponent blendColorDesc;
            blendColorDesc.operation = HgiWebGPUConversions::GetBlendEquation(ct.colorBlendOp);
            blendColorDesc.srcFactor = HgiWebGPUConversions::GetBlendFactor(ct.srcColorBlendFactor);
            blendColorDesc.dstFactor = HgiWebGPUConversions::GetBlendFactor(ct.dstColorBlendFactor);

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
    pipelineDesc.label = desc.debugName.c_str();
    _pipeline = device.CreateRenderPipeline(&pipelineDesc);
}

HgiWebGPUGraphicsPipeline::~HgiWebGPUGraphicsPipeline()
{
}


wgpu::RenderPipeline HgiWebGPUGraphicsPipeline::GetPipeline() const
{
    return _pipeline;
}

const HgiWebGPUPipelineBindGroups& HgiWebGPUGraphicsPipeline::GetPipelineBindGroups() const
{
    return _pipelineBindGroups;
}

PXR_NAMESPACE_CLOSE_SCOPE
