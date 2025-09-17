//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_DIAGNOSTIC_H
#define PXR_IMAGING_HGIVULKAN_DIAGNOSTIC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiVulkan/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanCommandBuffer;
class HgiVulkanDevice;
class HgiVulkanInstance;


/// Returns true if debugging is enabled (HGIVULKAN_DEBUG=1)
HGIVULKAN_API
bool HgiVulkanIsDebugEnabled();

/// Setup vulkan debug callbacks
HGIVULKAN_API
void HgiVulkanCreateDebug(HgiVulkanInstance* instance);

/// Tear down vulkan debug callbacks
HGIVULKAN_API
void HgiVulkanDestroyDebug(HgiVulkanInstance* instance);

/// Setup vulkan device debug callbacks
HGIVULKAN_API
void HgiVulkanSetupDeviceDebug(
    HgiVulkanInstance* instance,
    HgiVulkanDevice* device);

/// Add a debug name to a vulkan object
HGIVULKAN_API
void HgiVulkanSetDebugName(
    HgiVulkanDevice* device,
    uint64_t vulkanObject, /*Handle to vulkan object cast to uint64_t*/
    VkObjectType objectType,
    const char* name);

/// Begin a label in a vulkan command buffer
HGIVULKAN_API
void HgiVulkanBeginLabel(
    HgiVulkanDevice* device,
    HgiVulkanCommandBuffer* cb,
    const char* label);

/// End the last pushed label in a vulkan command buffer
HGIVULKAN_API
void HgiVulkanEndLabel(
    HgiVulkanDevice* device,
    HgiVulkanCommandBuffer* cb);

/// Begin a label in the vulkan device gfx queue
HGIVULKAN_API
void HgiVulkanBeginQueueLabel(
    HgiVulkanDevice* device,
    const char* label);

/// End the last pushed label in the vulkan device gfx queue
HGIVULKAN_API
void HgiVulkanEndQueueLabel(HgiVulkanDevice* device);

/// Returns a string representation of VkResult
HGIVULKAN_API
const char* HgiVulkanResultString(VkResult result);

#define HGIVULKAN_VERIFY_VK_RESULT(cmd)                                 \
  {                                                                     \
    const VkResult result = cmd;                                        \
    TF_VERIFY(                                                          \
        result == VK_SUCCESS,                                           \
        "%s: %s",                                                       \
        #cmd,                                                           \
        HgiVulkanResultString(result));                                 \
  } while(0)

PXR_NAMESPACE_CLOSE_SCOPE

#endif