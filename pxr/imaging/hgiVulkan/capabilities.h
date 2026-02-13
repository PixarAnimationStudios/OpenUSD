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
#include "pxr/imaging/hgi/types.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include <set>
#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

struct HgiVulkanFormatInfo
{
    VkImageType type;
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageCreateFlags createFlags;
    bool hostImageCopyOptimal;
};

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

    HGIVULKAN_API
    bool SupportsMemoryToTextureCopy(VkImageLayout layout) const;

    HGIVULKAN_API
    HgiVulkanFormatInfo GetFormatInfo(
        HgiVulkanDevice* device,
        HgiTextureType type,
        HgiFormat format,
        HgiTextureUsage usage,
        bool optimalTiling,
        bool hostImageCopyDesired) const;

    bool supportsTimeStamps;

    bool supportsNativeInterop;

    bool supportsHostImageCopy;

    VkPhysicalDeviceProperties2 vkDeviceProperties2 {};
    VkPhysicalDeviceMemoryProperties vkMemoryProperties {};
    VkPhysicalDeviceIDProperties vkPhysicalDeviceIdProperties {};
    VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT
        vkVertexAttributeDivisorProperties {};

    VkPhysicalDeviceFeatures2 vkDeviceFeatures2 {};
    VkPhysicalDeviceVulkan11Features vkVulkan11Features {};
    VkPhysicalDeviceVulkan12Features vkVulkan12Features {};
    VkPhysicalDeviceVulkan13Features vkVulkan13Features {};
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT
        vkVertexAttributeDivisorFeatures {};
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR
        vkBarycentricFeatures {};
    VkPhysicalDeviceLineRasterizationFeaturesKHR vkLineRasterizationFeatures {};

private:
    std::set<VkImageLayout> _vkHostImageCopySrcLayouts;
    std::set<VkImageLayout> _vkHostImageCopyDstLayouts;

    struct _HgiFormatInfo
    {
        HgiTextureType type;
        HgiFormat format;
        HgiTextureUsage usage;
        bool optimalTiling;
        bool hostImageCopyDesired;
    };

    struct _HgiFormatHashCompare {
        static bool equal(const _HgiFormatInfo &a, const _HgiFormatInfo &b);
        static size_t hash(const _HgiFormatInfo &a);
    };

    using _HgiFormatInfoToHgiVulkanFormatInfo = 
    tbb::concurrent_hash_map<
        _HgiFormatInfo,
        HgiVulkanFormatInfo,
        _HgiFormatHashCompare>;

    mutable _HgiFormatInfoToHgiVulkanFormatInfo _infoLookup;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
