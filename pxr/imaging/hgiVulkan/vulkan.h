//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_VULKAN_H
#define PXR_IMAGING_HGIVULKAN_VULKAN_H

#include "pxr/base/arch/defines.h"

// Define the platform for Vulkan so vulkan.h below picks the correct includes.
#if defined(ARCH_OS_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR
#elif defined(ARCH_OS_LINUX)
    #define VK_USE_PLATFORM_XLIB_KHR
#elif defined(ARCH_OS_OSX)
    #define VK_USE_PLATFORM_METAL_EXT
#else
    #error Unsupported Platform
#endif

#include <vulkan/vulkan.h>

#include "pxr/imaging/hgiVulkan/vk_mem_alloc.h"

// Use the default allocator (nullptr)
inline VkAllocationCallbacks*
HgiVulkanAllocator() {
    return nullptr;
}

#endif
