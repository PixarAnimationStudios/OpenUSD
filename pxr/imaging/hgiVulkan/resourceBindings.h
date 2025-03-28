//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_VULKAN_RESOURCEBINDINGS_H
#define PXR_IMAGING_HGI_VULKAN_RESOURCEBINDINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;


///
/// \class HgiVulkanResourceBindings
///
/// Vulkan implementation of HgiResourceBindings.
///
///
class HgiVulkanResourceBindings final : public HgiResourceBindings
{
public:
    HGIVULKAN_API
    ~HgiVulkanResourceBindings() override;

    /// Binds the resources to GPU.
    HGIVULKAN_API
    void BindResources(
        VkCommandBuffer cb,
        VkPipelineBindPoint bindPoint,
        VkPipelineLayout layout);

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanResourceBindings(
        HgiVulkanDevice* device,
        HgiResourceBindingsDesc const& desc);

private:
    HgiVulkanResourceBindings() = delete;
    HgiVulkanResourceBindings & operator=(const HgiVulkanResourceBindings&) = delete;
    HgiVulkanResourceBindings(const HgiVulkanResourceBindings&) = delete;

    HgiVulkanDevice* _device;
    uint64_t _inflightBits;

    VkDescriptorPool _vkDescriptorPool;
    VkDescriptorSetLayout _vkDescriptorSetLayout;
    VkDescriptorSet _vkDescriptorSet;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
