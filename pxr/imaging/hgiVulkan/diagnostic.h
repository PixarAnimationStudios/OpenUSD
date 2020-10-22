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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
