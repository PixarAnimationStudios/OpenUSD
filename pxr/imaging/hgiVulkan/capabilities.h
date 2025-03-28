//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_CAPABILITIES_H
#define PXR_IMAGING_HGIVULKAN_CAPABILITIES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/capabilities.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

/// \class HgiVulkanCapabilities
///
/// Reports the capabilities of the Vulkan device.
///
class HgiVulkanCapabilities final : public HgiCapabilities
{
public:
    HGIVULKAN_API
    HgiVulkanCapabilities(HgiVulkanDevice* device);

    HGIVULKAN_API
    ~HgiVulkanCapabilities();

    HGIVULKAN_API
    int GetAPIVersion() const override;
    
    HGIVULKAN_API
    int GetShaderVersion() const override;

    bool supportsTimeStamps;
    VkPhysicalDeviceProperties vkDeviceProperties;
    VkPhysicalDeviceProperties2 vkDeviceProperties2;
    VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT
        vkVertexAttributeDivisorProperties;
    VkPhysicalDeviceFeatures vkDeviceFeatures;
    VkPhysicalDeviceFeatures2 vkDeviceFeatures2;
    VkPhysicalDeviceVulkan11Features vkVulkan11Features;
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT vkIndexingFeatures;
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT
        vkVertexAttributeDivisorFeatures;
    VkPhysicalDeviceMemoryProperties vkMemoryProperties;
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR vkBarycentricFeatures;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
