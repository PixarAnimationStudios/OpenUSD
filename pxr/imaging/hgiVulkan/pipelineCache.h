//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_VULKAN_PIPELINE_CACHE_H
#define PXR_IMAGING_HGI_VULKAN_PIPELINE_CACHE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

/// \class HgiVulkanPipelineCache
///
/// Wrapper for Vulkan pipeline cache.
///
class HgiVulkanPipelineCache final
{
public:
    HGIVULKAN_API
    HgiVulkanPipelineCache(HgiVulkanDevice* device);

    HGIVULKAN_API
    ~HgiVulkanPipelineCache();

    /// Returns the vulkan pipeline cache.
    HGIVULKAN_API
    VkPipelineCache GetVulkanPipelineCache() const;

private:
    HgiVulkanPipelineCache() = delete;
    HgiVulkanPipelineCache & operator=(const HgiVulkanPipelineCache&) = delete;
    HgiVulkanPipelineCache(const HgiVulkanPipelineCache&) = delete;

    HgiVulkanDevice* _device;
    VkPipelineCache _vkPipelineCache;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
