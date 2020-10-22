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
#ifndef PXR_IMAGING_HGI_VULKAN_COMPUTE_PIPELINE_H
#define PXR_IMAGING_HGI_VULKAN_COMPUTE_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

using VkDescriptorSetLayoutVector = std::vector<VkDescriptorSetLayout>;


/// \class HgiVulkanComputePipeline
///
/// Vulkan implementation of HgiComputePipeline.
///
class HgiVulkanComputePipeline final : public HgiComputePipeline
{
public:
    HGIVULKAN_API
    ~HgiVulkanComputePipeline() override;

    /// Apply pipeline state
    HGIVULKAN_API
    void BindPipeline(VkCommandBuffer cb);

    /// Returns the vulkan pipeline layout
    HGIVULKAN_API
    VkPipelineLayout GetVulkanPipelineLayout() const;

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanComputePipeline(
        HgiVulkanDevice* device,
        HgiComputePipelineDesc const& desc);

private:
    HgiVulkanComputePipeline() = delete;
    HgiVulkanComputePipeline & operator=(const HgiVulkanComputePipeline&) = delete;
    HgiVulkanComputePipeline(const HgiVulkanComputePipeline&) = delete;

    HgiVulkanDevice* _device;
    uint64_t _inflightBits;
    VkPipeline _vkPipeline;
    VkPipelineLayout _vkPipelineLayout;
    VkDescriptorSetLayoutVector _vkDescriptorSetLayouts;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
