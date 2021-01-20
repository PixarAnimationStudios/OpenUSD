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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/graphicsPipeline.h"
#include "pxr/imaging/hgiVulkan/pipelineCache.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

using HgiAttachmentDescConstPtrVector = std::vector<HgiAttachmentDesc const*>;


HgiVulkanGraphicsPipeline::HgiVulkanGraphicsPipeline(
    HgiVulkanDevice* device,
    HgiGraphicsPipelineDesc const& desc)
    : HgiGraphicsPipeline(desc)
    , _device(device)
    , _inflightBits(0)
    , _vkPipeline(nullptr)
    , _vkRenderPass(nullptr)
    , _vkPipelineLayout(nullptr)
{
    VkGraphicsPipelineCreateInfo pipeCreateInfo =
        {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

    //
    // Shaders
    //
    HgiShaderFunctionHandleVector const& sfv =
        desc.shaderProgram->GetShaderFunctions();

    // Shader reflection produced descriptor set information that we need
    // to create the pipeline layout.
    std::vector<HgiVulkanDescriptorSetInfoVector> descriptorSetInfos;

    std::vector<VkPipelineShaderStageCreateInfo> stages;
    stages.reserve(sfv.size());

    for (HgiShaderFunctionHandle const& sf : sfv) {
        HgiVulkanShaderFunction const* s =
            static_cast<HgiVulkanShaderFunction const*>(sf.Get());

        descriptorSetInfos.push_back(s->GetDescriptorSetInfo());

        VkPipelineShaderStageCreateInfo stage =
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stage.stage = s->GetShaderStage();
        stage.module = s->GetShaderModule();
        stage.pName = s->GetShaderFunctionName();
        stage.pNext = nullptr;
        stage.pSpecializationInfo = nullptr; // XXX allows shader optimizations
        stage.flags = 0;
        stages.push_back(std::move(stage));
    }

    pipeCreateInfo.stageCount = (uint32_t) stages.size();
    pipeCreateInfo.pStages = stages.data();

    //
    // Vertex Input State
    // The input state includes the format and arrangement of the vertex data.
    //

    std::vector<VkVertexInputBindingDescription> vertBufs;
    std::vector<VkVertexInputAttributeDescription> vertAttrs;

    for (HgiVertexBufferDesc const& vbo : desc.vertexBuffers) {
        for (HgiVertexAttributeDesc const& va : vbo.vertexAttributes) {
            VkVertexInputAttributeDescription ad;
            ad.binding = vbo.bindingIndex;
            ad.location = va.shaderBindLocation;
            ad.offset = va.offset;
            ad.format = HgiVulkanConversions::GetFormat(va.format);
            vertAttrs.push_back(std::move(ad));
        }

        VkVertexInputBindingDescription vib;
        vib.binding = vbo.bindingIndex;
        vib.stride = vbo.vertexStride;
        vib.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertBufs.push_back(std::move(vib));
    }

    VkPipelineVertexInputStateCreateInfo vertexInput =
        {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.pVertexAttributeDescriptions = vertAttrs.data();
    vertexInput.vertexAttributeDescriptionCount = (uint32_t) vertAttrs.size();
    vertexInput.pVertexBindingDescriptions = vertBufs.data();
    vertexInput.vertexBindingDescriptionCount = (uint32_t) vertBufs.size();
    pipeCreateInfo.pVertexInputState = &vertexInput;

    //
    // Input assembly state
    // Declare how your vertices form the geometry you want to draw.
    //
    VkPipelineInputAssemblyStateCreateInfo inputAssembly =
        {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

    inputAssembly.topology =
        HgiVulkanConversions::GetPrimitiveType(desc.primitiveType);
    pipeCreateInfo.pInputAssemblyState = &inputAssembly;

    //
    // Viewport and Scissor state
    // If these are set via a command, state this in Dynamic states below.
    //
    VkPipelineViewportStateCreateInfo viewportState =
        {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;
    viewportState.pViewports = nullptr;
    pipeCreateInfo.pViewportState = &viewportState;

    //
    // Rasterization state
    // Rasterization operations.
    //
    HgiRasterizationState const& ras = desc.rasterizationState;

    VkPipelineRasterizationStateCreateInfo rasterState =
        {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterState.lineWidth = ras.lineWidth;
    rasterState.cullMode = HgiVulkanConversions::GetCullMode(ras.cullMode);
    rasterState.polygonMode =
        HgiVulkanConversions::GetPolygonMode(ras.polygonMode);
    rasterState.frontFace = HgiVulkanConversions::GetWinding(ras.winding);
    rasterState.rasterizerDiscardEnable = !ras.rasterizerEnabled;
    pipeCreateInfo.pRasterizationState = &rasterState;

    //
    // Multisample state
    //
    HgiMultiSampleState const& ms = desc.multiSampleState;

    VkPipelineMultisampleStateCreateInfo multisampleState =
        {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampleState.pSampleMask = nullptr;
    multisampleState.rasterizationSamples =
        HgiVulkanConversions::GetSampleCount(ms.sampleCount);
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.minSampleShading = 0.5f;
    multisampleState.alphaToCoverageEnable = ms.alphaToCoverageEnable;
    multisampleState.alphaToOneEnable = VK_FALSE;
    pipeCreateInfo.pMultisampleState = &multisampleState;

    //
    // Depth Stencil state
    //
    VkPipelineDepthStencilStateCreateInfo depthStencilState =
        {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

    depthStencilState.depthTestEnable = desc.depthState.depthTestEnabled;
    depthStencilState.depthWriteEnable = desc.depthState.depthWriteEnabled;

    depthStencilState.depthCompareOp =
        HgiVulkanConversions::GetDepthCompareFunction(
            desc.depthState.depthCompareFn);

    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.minDepthBounds = 0;
    depthStencilState.maxDepthBounds = 0;

    depthStencilState.stencilTestEnable =
        desc.depthState.stencilTestEnabled;

    if (desc.depthState.stencilTestEnabled) {
        TF_CODING_ERROR("Missing implementation stencil mask enabled");
    } else {
        depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
        depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
        depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depthStencilState.back.compareMask = 0;
        depthStencilState.back.reference = 0;
        depthStencilState.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depthStencilState.back.writeMask = 0;
        depthStencilState.front = depthStencilState.back;
    }

    pipeCreateInfo.pDepthStencilState = &depthStencilState;

    //
    // Color blend state
    // Per attachment configuration of how output color blends with destination.
    //
    std::vector<VkPipelineColorBlendAttachmentState> colorAttachState;
    for (HgiAttachmentDesc const& attach : desc.colorAttachmentDescs) {
        VkPipelineColorBlendAttachmentState ca =
            {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};

        ca.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                            VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT |
                            VK_COLOR_COMPONENT_A_BIT;

        ca.blendEnable = attach.blendEnabled;
        ca.alphaBlendOp =
            HgiVulkanConversions::GetBlendEquation(attach.alphaBlendOp);
        ca.colorBlendOp =
            HgiVulkanConversions::GetBlendEquation(attach.colorBlendOp);
        ca.srcColorBlendFactor =
            HgiVulkanConversions::GetBlendFactor(attach.srcColorBlendFactor);
        ca.dstColorBlendFactor =
            HgiVulkanConversions::GetBlendFactor(attach.dstColorBlendFactor);
        ca.srcAlphaBlendFactor =
            HgiVulkanConversions::GetBlendFactor(attach.srcAlphaBlendFactor);
        ca.dstAlphaBlendFactor =
            HgiVulkanConversions::GetBlendFactor(attach.dstAlphaBlendFactor);

        colorAttachState.push_back(std::move(ca));
    }

    VkPipelineColorBlendStateCreateInfo colorBlendState =
        {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendState.attachmentCount = (uint32_t) colorAttachState.size();
    colorBlendState.pAttachments = colorAttachState.data();
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendState.blendConstants[0] = 1.0f;
    colorBlendState.blendConstants[1] = 1.0f;
    colorBlendState.blendConstants[2] = 1.0f;
    colorBlendState.blendConstants[3] = 1.0f;
    pipeCreateInfo.pColorBlendState = &colorBlendState;

    //
    // Dynamic States
    // States that change during command buffer execution via a command
    //
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState =
        {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = (uint32_t)  TfArraySize(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;
    pipeCreateInfo.pDynamicState = &dynamicState;

    //
    // Generate Pipeline layout
    //
    bool usePushConstants = desc.shaderConstantsDesc.byteSize > 0;
    VkPushConstantRange pcRanges;
    if (usePushConstants) {
        TF_VERIFY(desc.shaderConstantsDesc.byteSize % 4 == 0,
            "Push constants not multipes of 4");
        pcRanges.offset = 0;
        pcRanges.size = desc.shaderConstantsDesc.byteSize;
        pcRanges.stageFlags = HgiVulkanConversions::GetShaderStages(
            desc.shaderConstantsDesc.stageUsage);
    }

    VkPipelineLayoutCreateInfo pipeLayCreateInfo =
        {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeLayCreateInfo.pushConstantRangeCount = usePushConstants ? 1 : 0;
    pipeLayCreateInfo.pPushConstantRanges = &pcRanges;

    _vkDescriptorSetLayouts = HgiVulkanMakeDescriptorSetLayouts(
        device, descriptorSetInfos, desc.debugName);
    pipeLayCreateInfo.setLayoutCount= (uint32_t) _vkDescriptorSetLayouts.size();
    pipeLayCreateInfo.pSetLayouts = _vkDescriptorSetLayouts.data();

    TF_VERIFY(
        vkCreatePipelineLayout(
            _device->GetVulkanDevice(),
            &pipeLayCreateInfo,
            HgiVulkanAllocator(),
            &_vkPipelineLayout) == VK_SUCCESS
    );

    // Debug label
    if (!desc.debugName.empty()) {
        std::string debugLabel = "PipelineLayout " + desc.debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkPipelineLayout,
            VK_OBJECT_TYPE_PIPELINE_LAYOUT,
            debugLabel.c_str());
    }

    pipeCreateInfo.layout = _vkPipelineLayout;

    //
    // RenderPass
    //
    _CreateRenderPass();
    TF_VERIFY(_vkRenderPass);
    pipeCreateInfo.renderPass = _vkRenderPass;

    //
    // Create pipeline
    //
    HgiVulkanPipelineCache* pCache = device->GetPipelineCache();

    TF_VERIFY(
        vkCreateGraphicsPipelines(
            _device->GetVulkanDevice(),
            pCache->GetVulkanPipelineCache(),
            1,
            &pipeCreateInfo,
            HgiVulkanAllocator(),
            &_vkPipeline) == VK_SUCCESS
    );

    // Debug label
    if (!desc.debugName.empty()) {
        std::string debugLabel = "Pipeline " + desc.debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)_vkPipeline,
            VK_OBJECT_TYPE_PIPELINE,
            debugLabel.c_str());
    }
}

HgiVulkanGraphicsPipeline::~HgiVulkanGraphicsPipeline()
{
    for (HgiVulkan_Framebuffer const& fb : _framebuffers) {
        vkDestroyFramebuffer(
            _device->GetVulkanDevice(),
            fb.vkFramebuffer,
            HgiVulkanAllocator());
    }

    vkDestroyRenderPass(
        _device->GetVulkanDevice(),
        _vkRenderPass,
        HgiVulkanAllocator());

    vkDestroyPipelineLayout(
        _device->GetVulkanDevice(),
        _vkPipelineLayout,
        HgiVulkanAllocator());

    vkDestroyPipeline(
        _device->GetVulkanDevice(),
        _vkPipeline,
        HgiVulkanAllocator());

    for (VkDescriptorSetLayout layout : _vkDescriptorSetLayouts) {
        vkDestroyDescriptorSetLayout(
            _device->GetVulkanDevice(),
            layout,
            HgiVulkanAllocator());
    }
}

void
HgiVulkanGraphicsPipeline::BindPipeline(VkCommandBuffer cb)
{
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, _vkPipeline);
}

VkPipelineLayout
HgiVulkanGraphicsPipeline::GetVulkanPipelineLayout() const
{
    return _vkPipelineLayout;
}

VkRenderPass
HgiVulkanGraphicsPipeline::GetVulkanRenderPass() const
{
    return _vkRenderPass;
}

VkFramebuffer
HgiVulkanGraphicsPipeline::AcquireVulkanFramebuffer(
        HgiGraphicsCmdsDesc const& gfxDesc,
        GfVec2i* dimensions)
{
    for (HgiVulkan_Framebuffer const& fb : _framebuffers) {
        if (fb.desc == gfxDesc) {
            if (dimensions) {
                *dimensions = fb.dimensions;
            }
            return fb.vkFramebuffer;
        }
    }

    // Limit the size of number of frame buffer we store in a pipeline.
    // If we resize the viewport, we may end up re-creating the framebuffer
    // for the pipeline quite frequently.
    // We keep more than one just in case the client uses the same pipeline
    // for multiple differently sized attachments that are compatible with
    // the pipeline. E.g. a blur pyramid where attachment config is the same,
    // but the sizes shrink.
    if (_framebuffers.size() > 32) {
        auto fbIt = _framebuffers.begin();
        vkDestroyFramebuffer(
            _device->GetVulkanDevice(),
            fbIt->vkFramebuffer,
            HgiVulkanAllocator());
        _framebuffers.erase(fbIt);
    }

    HgiVulkan_Framebuffer framebuffer;
    framebuffer.desc = gfxDesc;

    // Make a list of all attachments (color, depth, resolve).
    std::vector<HgiTextureHandle> textures;
    textures.insert(
        textures.end(),
        gfxDesc.colorTextures.begin(),
        gfxDesc.colorTextures.end());

    if (gfxDesc.depthTexture) {
        textures.push_back(gfxDesc.depthTexture);
    }

    textures.insert(
        textures.end(),
        gfxDesc.colorResolveTextures.begin(),
        gfxDesc.colorResolveTextures.end());

    if (gfxDesc.depthResolveTexture){
        textures.push_back(gfxDesc.depthResolveTexture);
    }

    std::vector<VkImageView> views;
    for (HgiTextureHandle const& texHandle : textures) {
        HgiVulkanTexture* tex = static_cast<HgiVulkanTexture*>(texHandle.Get());
        views.push_back(tex->GetImageView());
        framebuffer.dimensions[0] = tex->GetDescriptor().dimensions[0];
        framebuffer.dimensions[1] = tex->GetDescriptor().dimensions[1];
    }

    VkFramebufferCreateInfo fbCreateInfo =
        {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbCreateInfo.renderPass = _vkRenderPass;
    fbCreateInfo.attachmentCount = (uint32_t) views.size();
    fbCreateInfo.pAttachments = views.data();
    fbCreateInfo.width = framebuffer.dimensions[0];
    fbCreateInfo.height = framebuffer.dimensions[1];
    fbCreateInfo.layers = 1;

    TF_VERIFY(
        vkCreateFramebuffer(
            _device->GetVulkanDevice(),
            &fbCreateInfo,
            HgiVulkanAllocator(),
            &framebuffer.vkFramebuffer) == VK_SUCCESS
    );

    // Debug label
    if (!_descriptor.debugName.empty()) {
        std::string debugLabel = "Framebuffer " + _descriptor.debugName;
        HgiVulkanSetDebugName(
            _device,
            (uint64_t)framebuffer.vkFramebuffer,
            VK_OBJECT_TYPE_FRAMEBUFFER,
            debugLabel.c_str());
    }

    TF_VERIFY(framebuffer.dimensions[0] > 0 && framebuffer.dimensions[1] > 0);

    _framebuffers.push_back(std::move(framebuffer));

    if (dimensions) {
        *dimensions = framebuffer.dimensions;
    }
    return framebuffer.vkFramebuffer;
}

HgiVulkanDevice*
HgiVulkanGraphicsPipeline::GetDevice() const
{
    return _device;
}

VkClearValueVector const&
HgiVulkanGraphicsPipeline::GetClearValues() const
{
    return _vkClearValues;
}

uint64_t &
HgiVulkanGraphicsPipeline::GetInflightBits()
{
    return _inflightBits;
}

static void
_ProcessAttachment(
    HgiAttachmentDesc const& attachment,
    uint32_t attachmentIndex,
    HgiSampleCount sampleCount,
    VkClearValue* vkClearValue,
    VkAttachmentDescription2* vkAttachDesc,
    VkAttachmentReference2* vkRef)
{
    bool isDepthAttachment = attachment.usage & HgiTextureUsageBitsDepthTarget;
    //
    // Reference
    //
    vkRef->sType = {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};
    vkRef->pNext = nullptr;
    vkRef->attachment = attachmentIndex;
    vkRef->aspectMask = isDepthAttachment ? 
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : 
        VK_IMAGE_ASPECT_COLOR_BIT;
    // The desired layout of the image during the sub pass
    vkRef->layout = isDepthAttachment ? 
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //
    // Description
    //
    // The layout at the end of the render pass.
    // XXX We don't know previous or next passes so for now we transition back
    // to our default. This may cause non-ideal image transitions.
    VkImageLayout layout =
        HgiVulkanTexture::GetDefaultImageLayout(attachment.usage);

    vkAttachDesc->sType = {VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2};
    vkAttachDesc->pNext = nullptr;
    vkAttachDesc->finalLayout = layout;
    vkAttachDesc->flags = 0;
    vkAttachDesc->format = HgiVulkanConversions::GetFormat(attachment.format);
    vkAttachDesc->initialLayout = layout;
    vkAttachDesc->loadOp = HgiVulkanConversions::GetLoadOp(attachment.loadOp);
    vkAttachDesc->samples = HgiVulkanConversions::GetSampleCount(sampleCount);
    vkAttachDesc->storeOp= HgiVulkanConversions::GetStoreOp(attachment.storeOp);
    // XXX Hgi doesn't provide stencil ops, assume it matches depth attachment.
    vkAttachDesc->stencilLoadOp = vkAttachDesc->loadOp;
    vkAttachDesc->stencilStoreOp = vkAttachDesc->storeOp;

    //
    // Clear value
    //
    vkClearValue->color.float32[0] = attachment.clearValue[0];
    vkClearValue->color.float32[1] = attachment.clearValue[1];
    vkClearValue->color.float32[2] = attachment.clearValue[2];
    vkClearValue->color.float32[3] = attachment.clearValue[3];
    vkClearValue->depthStencil.depth = attachment.clearValue[0];
    vkClearValue->depthStencil.stencil = uint32_t(attachment.clearValue[1]);
}

void
HgiVulkanGraphicsPipeline::_CreateRenderPass()
{
    HgiSampleCount samples = _descriptor.multiSampleState.sampleCount;

    if (!_descriptor.colorResolveAttachmentDescs.empty()) {
        TF_VERIFY(
            _descriptor.colorAttachmentDescs.size() ==
            _descriptor.colorResolveAttachmentDescs.size(),
            "Count mismatch between color and resolve attachments");
        TF_VERIFY(
            samples > HgiSampleCount1,
            "Pipeline sample count must be greater than one to use resolve");
    }

    // Determine description and reference for each attachment
    _vkClearValues.clear();
    std::vector<VkAttachmentDescription2> vkDescriptions;
    std::vector<VkAttachmentReference2> vkColorReferences;
    VkAttachmentReference2 vkDepthReference = 
        {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};
    std::vector<VkAttachmentReference2> vkColorResolveReferences;
    VkAttachmentReference2 vkDepthResolveReference = 
        {VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2};

    // Process color attachments
    for (HgiAttachmentDesc const& desc : _descriptor.colorAttachmentDescs) {
        uint32_t slot = (uint32_t) vkDescriptions.size();
        VkClearValue vkClear;
        VkAttachmentDescription2 vkDesc;
        VkAttachmentReference2 vkRef;
        _ProcessAttachment(desc, slot, samples, &vkClear, &vkDesc, &vkRef);
        _vkClearValues.push_back(vkClear);
        vkDescriptions.push_back(vkDesc);
        vkColorReferences.push_back(vkRef);
    }

    // Process depth attachment
    bool hasDepth = _descriptor.depthAttachmentDesc.format != HgiFormatInvalid;
    if (hasDepth) {
        HgiAttachmentDesc const& desc = _descriptor.depthAttachmentDesc;
        uint32_t slot = (uint32_t) vkDescriptions.size();
        VkClearValue vkClear;
        VkAttachmentDescription2 vkDesc;
        VkAttachmentReference2* vkRef = &vkDepthReference;
        _ProcessAttachment(desc, slot, samples, &vkClear, &vkDesc, vkRef);
        _vkClearValues.push_back(vkClear);
        vkDescriptions.push_back(vkDesc);
    }

    // Process color resolve attachments
    for (HgiAttachmentDesc const& desc:_descriptor.colorResolveAttachmentDescs){
        uint32_t slot = (uint32_t) vkDescriptions.size();
        VkClearValue vkClear;
        VkAttachmentDescription2 vkDesc;
        VkAttachmentReference2 vkRef;
        _ProcessAttachment(desc,slot,HgiSampleCount1,&vkClear, &vkDesc, &vkRef);
        _vkClearValues.push_back(vkClear);
        vkDescriptions.push_back(vkDesc);
        vkColorResolveReferences.push_back(vkRef);
    }

    // Process depth resolve attachment
    bool hasDepthResolve =
        _descriptor.depthResolveAttachmentDesc.format != HgiFormatInvalid;
    if (hasDepthResolve) {
        HgiAttachmentDesc const& desc = _descriptor.depthResolveAttachmentDesc;
        uint32_t slot = (uint32_t) vkDescriptions.size();
        VkClearValue vkClear;
        VkAttachmentDescription2 vkDesc;
        VkAttachmentReference2* vkRef = &vkDepthResolveReference;
        _ProcessAttachment(desc,slot,HgiSampleCount1, &vkClear, &vkDesc, vkRef);
        _vkClearValues.push_back(vkClear);
        vkDescriptions.push_back(vkDesc);
    }

    //
    // Attachments
    //
    VkSubpassDescription2KHR subpassDesc =
        {VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2_KHR};
    subpassDesc.flags = 0;
    subpassDesc.viewMask = 0;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments = nullptr;
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments = nullptr;
    subpassDesc.colorAttachmentCount = (uint32_t) vkColorReferences.size();
    subpassDesc.pColorAttachments = vkColorReferences.data();
    subpassDesc.pResolveAttachments = vkColorResolveReferences.data();
    subpassDesc.pDepthStencilAttachment= hasDepth ? &vkDepthReference : nullptr;

    if (hasDepthResolve) {
        VkSubpassDescriptionDepthStencilResolveKHR depthResolve =
            {VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE_KHR};
        depthResolve.pDepthStencilResolveAttachment = &vkDepthResolveReference;
        depthResolve.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
        depthResolve.stencilResolveMode = VK_RESOLVE_MODE_NONE;
        subpassDesc.pNext = &depthResolve;
    }

    //
    // SubPass dependencies
    //
    // Use subpass dependencies to transition image layouts and act as barrier
    // to ensure the read and write operations happen when it is allowed.
    //
    VkSubpassDependency2KHR dependencies[2] =
        {{VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2_KHR}, 
         {VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2_KHR}};

    // Start of subpass -- ensure shader reading is completed before FB write.
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // XXX non optimal masks, but we don't know enough details.
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask =
        VK_ACCESS_MEMORY_READ_BIT |
        VK_ACCESS_MEMORY_WRITE_BIT;

    dependencies[0].viewOffset = 0;

    // End of subpass -- ensure FB write is finished before shader reads.
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // XXX non optimal masks, but we don't know enough details.
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcStageMask= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].srcAccessMask =
        VK_ACCESS_MEMORY_READ_BIT |
        VK_ACCESS_MEMORY_WRITE_BIT;

    dependencies[1].viewOffset = 0;

    //
    // Create the renderpass
    //
    VkRenderPassCreateInfo2KHR renderPassInfo = 
        {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2_KHR};
    renderPassInfo.attachmentCount = (uint32_t) vkDescriptions.size();
    renderPassInfo.pAttachments = vkDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = &dependencies[0];

    // XXX vkCreateRenderPass2 (without KHR) seems to crash.
    // So we use KHR version for the function AND all the structs.
    // We could cache this fn ptr on device, but hopefully it is tmp and the
    // non KHR version will work in the future.
    PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR = 0;
    vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR) vkGetDeviceProcAddr(
        _device->GetVulkanDevice(), "vkCreateRenderPass2KHR");

    TF_VERIFY(
        vkCreateRenderPass2KHR(
            _device->GetVulkanDevice(),
            &renderPassInfo,
            HgiVulkanAllocator(),
            &_vkRenderPass) == VK_SUCCESS
    );

    // Debug label
    if (!_descriptor.debugName.empty()) {
        std::string debugLabel = "RenderPass " + _descriptor.debugName;
        HgiVulkanSetDebugName(
            _device,
            (uint64_t)_vkRenderPass,
            VK_OBJECT_TYPE_RENDER_PASS,
            debugLabel.c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
