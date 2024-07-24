//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
