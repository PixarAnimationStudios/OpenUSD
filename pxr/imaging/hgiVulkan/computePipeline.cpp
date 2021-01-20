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

#include "pxr/imaging/hgiVulkan/computePipeline.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/pipelineCache.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/shaderProgram.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanComputePipeline::HgiVulkanComputePipeline(
    HgiVulkanDevice* device,
    HgiComputePipelineDesc const& desc)
    : HgiComputePipeline(desc)
    , _device(device)
    , _inflightBits(0)
    , _vkPipeline(nullptr)
    , _vkPipelineLayout(nullptr)
{
    VkComputePipelineCreateInfo pipeCreateInfo =
        {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};

    HgiShaderFunctionHandleVector const& sfv =
        desc.shaderProgram->GetShaderFunctions();

    if (sfv.empty()) {
        TF_CODING_ERROR("Missing compute program");
        return;
    }

    HgiVulkanShaderFunction const* s =
        static_cast<HgiVulkanShaderFunction const*>(sfv.front().Get());

    HgiVulkanDescriptorSetInfoVector const& setInfo = s->GetDescriptorSetInfo();

    pipeCreateInfo.stage.sType =
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    pipeCreateInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeCreateInfo.stage.module = s->GetShaderModule();
    pipeCreateInfo.stage.pName = s->GetShaderFunctionName();
    pipeCreateInfo.stage.pNext = nullptr;
    pipeCreateInfo.stage.pSpecializationInfo = nullptr;
    pipeCreateInfo.stage.flags = 0;

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
        pcRanges.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkPipelineLayoutCreateInfo pipeLayCreateInfo =
        {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeLayCreateInfo.pushConstantRangeCount = usePushConstants ? 1 : 0;
    pipeLayCreateInfo.pPushConstantRanges = &pcRanges;

    _vkDescriptorSetLayouts = HgiVulkanMakeDescriptorSetLayouts(
        device, {setInfo}, desc.debugName);
    pipeLayCreateInfo.setLayoutCount = (uint32_t)_vkDescriptorSetLayouts.size();
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
    // Create pipeline
    //
    HgiVulkanPipelineCache* pCache = device->GetPipelineCache();

    TF_VERIFY(
        vkCreateComputePipelines(
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

HgiVulkanComputePipeline::~HgiVulkanComputePipeline()
{
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
HgiVulkanComputePipeline::BindPipeline(VkCommandBuffer cb)
{
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, _vkPipeline);
}

VkPipelineLayout
HgiVulkanComputePipeline::GetVulkanPipelineLayout() const
{
    return _vkPipelineLayout;
}

HgiVulkanDevice*
HgiVulkanComputePipeline::GetDevice() const
{
    return _device;
}

uint64_t &
HgiVulkanComputePipeline::GetInflightBits()
{
    return _inflightBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
