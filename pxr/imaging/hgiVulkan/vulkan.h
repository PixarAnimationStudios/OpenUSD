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
#ifndef PXR_IMAGING_HGIVULKAN_VULKAN_H
#define PXR_IMAGING_HGIVULKAN_VULKAN_H

#include "pxr/base/arch/defines.h"

// Define the platform for Vulkan so vulkan.h below picks the correct includes.
#if defined(ARCH_OS_WINDOWS)
    #define VK_USE_PLATFORM_WIN32_KHR
#elif defined(ARCH_OS_LINUX)
    #define VK_USE_PLATFORM_XLIB_KHR
#elif defined(ARCH_OS_OSX)
    #define VK_USE_PLATFORM_MACOS_MVK
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
