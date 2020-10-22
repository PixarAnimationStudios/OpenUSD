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
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanCapabilities::HgiVulkanCapabilities(HgiVulkanDevice* device)
    : supportsTimeStamps(false)
{
    VkPhysicalDevice physicalDevice = device->GetVulkanPhysicalDevice();

    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, 0);
    std::vector<VkQueueFamilyProperties> queues(queueCount);

    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice,
        &queueCount,
        queues.data());

    // Grab the properties of all queues up until the (gfx) queue we are using.
    uint32_t gfxQueueIndex = device->GetGfxQueueFamilyIndex();

    // The last queue we grabbed the properties of is our gfx queue.
    if (TF_VERIFY(gfxQueueIndex < queues.size())) {
        VkQueueFamilyProperties const& gfxQueue = queues[gfxQueueIndex];
        supportsTimeStamps = gfxQueue.timestampValidBits > 0;
    }

    vkGetPhysicalDeviceProperties(physicalDevice, &vkDeviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &vkDeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &vkMemoryProperties);

    // Indexing features ext for resource bindings
    vkIndexingFeatures.pNext = nullptr;
    vkIndexingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

    // Query device features
    vkDeviceFeatures2.pNext = nullptr;
    vkDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vkDeviceFeatures2.pNext = &vkIndexingFeatures;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &vkDeviceFeatures2);

    // Verify we meet extension requirements
    #if !defined(VK_USE_PLATFORM_MACOS_MVK)
        TF_VERIFY(
            vkIndexingFeatures.shaderSampledImageArrayNonUniformIndexing &&
            vkIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing);
    #endif

    if (HgiVulkanIsDebugEnabled()) {
        TF_WARN("Selected GPU %s", vkDeviceProperties.deviceName);
    }
}

HgiVulkanCapabilities::~HgiVulkanCapabilities() = default;

PXR_NAMESPACE_CLOSE_SCOPE
