//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_VULKAN_PIPELINE_H
#define PXR_IMAGING_HGI_VULKAN_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

using VkDescriptorSetLayoutVector = std::vector<VkDescriptorSetLayout>;

/// \class HgiVulkanPipeline
///
/// Vulkan implementation of HgiGraphicsPipeline.
///
class HgiVulkanGraphicsPipeline final : public HgiGraphicsPipeline
{
public:
    HGIVULKAN_API
    ~HgiVulkanGraphicsPipeline() override;

    /// Apply pipeline state
    HGIVULKAN_API
    void BindPipeline(VkCommandBuffer cb);

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the vulkan pipeline layout
    HGIVULKAN_API
    VkPipelineLayout GetVulkanPipelineLayout() const;

    /// Returns the vulkan render pass
    HGIVULKAN_API
    VkRenderPass GetVulkanRenderPass() const;

    /// Returns the vulkan frame buffer, creating it if needed.
    HGIVULKAN_API
    VkFramebuffer AcquireVulkanFramebuffer(
        HgiGraphicsCmdsDesc const& gfxDesc,
        GfVec2i* dimensions);

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

    /// Returns true if any of the attachments in HgiGraphicsPipelineDesc 
    /// specify a clear operation.
    HGIVULKAN_API
    bool GetClearNeeded() const
    {
        return _clearNeeded;
    }

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanGraphicsPipeline(
        HgiVulkanDevice* device,
        HgiGraphicsPipelineDesc const& desc);

private:
    HgiVulkanGraphicsPipeline() = delete;
    HgiVulkanGraphicsPipeline & operator=(const HgiVulkanGraphicsPipeline&) = delete;
    HgiVulkanGraphicsPipeline(const HgiVulkanGraphicsPipeline&) = delete;

    void _ProcessAttachment(
        HgiAttachmentDesc const& attachment,
        uint32_t attachmentIndex,
        HgiSampleCount sampleCount,
        VkAttachmentDescription2* vkAttachDesc,
        VkAttachmentReference2* vkRef);
    void _CreateRenderPass();

    struct HgiVulkan_Framebuffer {
        GfVec2i dimensions;
        HgiGraphicsCmdsDesc desc;
        VkFramebuffer vkFramebuffer;
    };

    HgiVulkanDevice* _device;
    uint64_t _inflightBits;
    VkPipeline _vkPipeline;
    VkRenderPass _vkRenderPass;
    VkPipelineLayout _vkPipelineLayout;
    VkDescriptorSetLayoutVector _vkDescriptorSetLayouts;
    bool _clearNeeded;

    std::vector<HgiVulkan_Framebuffer> _framebuffers;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
