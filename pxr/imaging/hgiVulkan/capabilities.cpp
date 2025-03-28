//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT, true,
                      "Use Vulkan multi draw indirect");
TF_DEFINE_ENV_SETTING(HGIVULKAN_ENABLE_BUILTIN_BARYCENTRICS, false,
                      "Use Vulkan built in barycentric coordinates");

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

    // Vertex attribute divisor properties ext
    vkVertexAttributeDivisorProperties.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT;
    vkVertexAttributeDivisorProperties.pNext = nullptr;
        
    vkDeviceProperties2.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    vkDeviceProperties2.properties = vkDeviceProperties;
    vkDeviceProperties2.pNext = &vkVertexAttributeDivisorProperties;
    vkGetPhysicalDeviceProperties2(physicalDevice, &vkDeviceProperties2);

    // Vertex attribute divisor features ext
    vkVertexAttributeDivisorFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT;
    
    // Barycentric features
    const bool barycentricExtSupported = device->IsSupportedExtension(
        VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
    if (barycentricExtSupported) {
        vkBarycentricFeatures.sType =
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
        vkBarycentricFeatures.pNext = nullptr;
        vkVertexAttributeDivisorFeatures.pNext = &vkBarycentricFeatures;
    } else {
        vkVertexAttributeDivisorFeatures.pNext = nullptr;
    }

    // Indexing features ext for resource bindings
    vkIndexingFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    vkIndexingFeatures.pNext = &vkVertexAttributeDivisorFeatures;

    // Vulkan 1.1 features
    vkVulkan11Features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vkVulkan11Features.pNext = &vkIndexingFeatures;

    // Query device features
    vkDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    vkDeviceFeatures2.pNext = &vkVulkan11Features;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &vkDeviceFeatures2);

    // Verify we meet feature and extension requirements

    // Storm with HgiVulkan needs gl_BaseInstance/gl_BaseInstanceARB in shader.
    TF_VERIFY(
        vkVulkan11Features.shaderDrawParameters);

    TF_VERIFY(
        vkIndexingFeatures.shaderSampledImageArrayNonUniformIndexing &&
        vkIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing);

    TF_VERIFY(
        vkVertexAttributeDivisorFeatures.vertexAttributeInstanceRateDivisor);

    if (HgiVulkanIsDebugEnabled()) {
        TF_WARN("Selected GPU %s", vkDeviceProperties.deviceName);
    }

    _maxClipDistances = vkDeviceProperties.limits.maxClipDistances;
    _maxUniformBlockSize = vkDeviceProperties.limits.maxUniformBufferRange;
    _maxShaderStorageBlockSize =
        vkDeviceProperties.limits.maxStorageBufferRange;
    _uniformBufferOffsetAlignment =
        vkDeviceProperties.limits.minUniformBufferOffsetAlignment;

    const bool conservativeRasterEnabled = (device->IsSupportedExtension(
        VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME));
    const bool shaderDrawParametersEnabled =
        vkVulkan11Features.shaderDrawParameters;
    bool multiDrawIndirectEnabled = true;
    bool builtinBarycentricsEnabled =
        barycentricExtSupported &&
        vkBarycentricFeatures.fragmentShaderBarycentric;

    // Check Hgi env settings
    if (!TfGetEnvSetting(HGIVULKAN_ENABLE_MULTI_DRAW_INDIRECT)) {
        multiDrawIndirectEnabled = false;
    }
    if (!TfGetEnvSetting(HGIVULKAN_ENABLE_BUILTIN_BARYCENTRICS)) {
        builtinBarycentricsEnabled = false;
    }

    _SetFlag(HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne, false);
    _SetFlag(HgiDeviceCapabilitiesBitsStencilReadback, true);
    _SetFlag(HgiDeviceCapabilitiesBitsShaderDoublePrecision, true);
    _SetFlag(HgiDeviceCapabilitiesBitsConservativeRaster, 
        conservativeRasterEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsBuiltinBarycentrics, 
        builtinBarycentricsEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsShaderDrawParameters, 
        shaderDrawParametersEnabled);
     _SetFlag(HgiDeviceCapabilitiesBitsMultiDrawIndirect,
        multiDrawIndirectEnabled);
}

HgiVulkanCapabilities::~HgiVulkanCapabilities() = default;

int
HgiVulkanCapabilities::GetAPIVersion() const
{
    return vkDeviceProperties.apiVersion;
}

int
HgiVulkanCapabilities::GetShaderVersion() const
{
    // Note: This is not the Vulkan Shader Language version. It is provided for
    // compatibility with code that is asking for the GLSL version.
    return 450;
}

PXR_NAMESPACE_CLOSE_SCOPE
