//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_DEVICE_H
#define PXR_IMAGING_HGIVULKAN_DEVICE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include <mutex>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanCapabilities;
class HgiVulkanCommandQueue;
class HgiVulkanInstance;
class HgiVulkanPipelineCache;


/// \class HgiVulkanDevice
///
/// Vulkan implementation of GPU device.
///
class HgiVulkanDevice final
{
public:
    HGIVULKAN_API
    HgiVulkanDevice(HgiVulkanInstance* instance);

    HGIVULKAN_API
    ~HgiVulkanDevice();

    /// Returns the vulkan device
    HGIVULKAN_API
    VkDevice GetVulkanDevice() const;

    /// Returns the vulkan memory allocator.
    HGIVULKAN_API
    VmaAllocator GetVulkanMemoryAllocator() const;

    /// Returns a VMA pool for images that use API Interop.
    HGIVULKAN_API
    VmaPool GetVMAPoolForInterop(VkImageCreateInfo imageInfo);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    HGIVULKAN_API
    HANDLE GetWin32HandleForMemory(VkDeviceMemory memory);
#endif

    /// Returns the command queue which manages command buffers submission.
    HGIVULKAN_API
    HgiVulkanCommandQueue* GetCommandQueue() const;

    /// Returns the device capablities / features it supports.
    HGIVULKAN_API
    HgiVulkanCapabilities const& GetDeviceCapabilities() const;

    /// Returns the type (or family index) for the graphics queue.
    HGIVULKAN_API
    uint32_t GetGfxQueueFamilyIndex() const;

    /// Returns vulkan physical device
    HGIVULKAN_API
    VkPhysicalDevice GetVulkanPhysicalDevice() const;

    /// Returns the pipeline cache.
    HGIVULKAN_API
    HgiVulkanPipelineCache* GetPipelineCache() const;

    /// Wait for all queued up commands to have been processed on device.
    /// This should ideally never be used as it creates very big stalls, but
    /// is useful for unit testing.
    HGIVULKAN_API
    void WaitForIdle();

    /// Returns true if the provided extension is supported by the device
    bool IsSupportedExtension(const char* extensionName) const;

    /// Device extension function pointers
    PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR = 0;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR = 0;
    PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR = 0;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR = 0;
    PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR = 0;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#endif
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = 0;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = 0;
    PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = 0;
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = 0;
    PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT = 0;
    PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT = 0;

private:
    HgiVulkanDevice() = delete;
    HgiVulkanDevice & operator=(const HgiVulkanDevice&) = delete;
    HgiVulkanDevice(const HgiVulkanDevice&) = delete;

    // Vulkan device objects
    VkPhysicalDevice _vkPhysicalDevice;
    VkDevice _vkDevice;
    std::vector<VkExtensionProperties> _vkExtensions;
    VmaAllocator _vmaAllocator;
    std::mutex _vmaInteropPoolsLock;
    std::unordered_map<uint32_t, VmaPool> _vmaInteropPoolsForMemoryType;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    // A temporary fix until we bump the Vulkan SDK to have VMA v3.2.0+
    // (Vulkan SDK 1.4.304.0+)
    std::mutex _vmaInteropWin32HandleLock;
    std::unordered_map<VkDeviceMemory, HANDLE> _vmaInteropWin32HandleForMemory;
#endif
    uint32_t _vkGfxsQueueFamilyIndex;
    HgiVulkanCommandQueue* _commandQueue;
    HgiVulkanCapabilities* _capabilities;
    HgiVulkanPipelineCache* _pipelineCache;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
