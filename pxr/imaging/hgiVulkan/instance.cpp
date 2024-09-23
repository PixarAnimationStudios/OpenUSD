//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

#include <vector>
#include <algorithm>


PXR_NAMESPACE_OPEN_SCOPE

static
bool
_CheckInstanceValidationLayerSupport(const char * layerName)
{  
    uint32_t layerCount;  
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);  
    std::vector<VkLayerProperties> availableLayers(layerCount);
  
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto& layerProperties : availableLayers) {  
        if (strcmp(layerName, layerProperties.layerName) == 0) {  
            return true;
        }  
    }  

    return false;  
}

static
std::vector<const char*>
_RemoveUnsupportedInstanceExtensions(
    const std::vector<const char*>& desiredExtensions)
{
    // Determine available instance extensions.
    uint32_t numAvailableExtensions = 0u;
    TF_VERIFY(vkEnumerateInstanceExtensionProperties(
        nullptr, &numAvailableExtensions, nullptr) == VK_SUCCESS);
    std::vector<VkExtensionProperties> availableExtensions;
    availableExtensions.resize(numAvailableExtensions);
    TF_VERIFY(vkEnumerateInstanceExtensionProperties(
        nullptr, &numAvailableExtensions,
        availableExtensions.data()) == VK_SUCCESS);

    std::vector<const char*> extensions;

    // Only add extensions to the list if they're available.
    for (const auto& ext : desiredExtensions) {
        if (std::find_if(availableExtensions.begin(), availableExtensions.end(),
            [name = ext](const VkExtensionProperties& p) 
            { return strcmp(p.extensionName, name) == 0; })
                != availableExtensions.end()) {
            extensions.push_back(ext);
        }
    }

    return extensions;
}

HgiVulkanInstance::HgiVulkanInstance()
    : vkDebugMessenger(nullptr)
    , vkCreateDebugUtilsMessengerEXT(nullptr)
    , vkDestroyDebugUtilsMessengerEXT(nullptr)
    , _vkInstance(nullptr)
{
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;

    // Setup instance extensions.
    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,

        // Pick platform specific surface extension
        #if defined(VK_USE_PLATFORM_WIN32_KHR)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        #elif defined(VK_USE_PLATFORM_XLIB_KHR)
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        #elif defined(VK_USE_PLATFORM_METAL_EXT)
            VK_EXT_METAL_SURFACE_EXTENSION_NAME,
            // See: https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Runtime_UserGuide.md#interacting-with-the-moltenvk-runtime
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        #else
            #error Unsupported Platform
        #endif

        // Extensions for interop with OpenGL
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,

        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };

    // Enable validation layers extension.
    // Requires VK_LAYER_PATH to be set.
    const std::vector<const char*> debugLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    if (HgiVulkanIsDebugEnabled()) {
        for (const auto& debugLayer : debugLayers) {
            if (!_CheckInstanceValidationLayerSupport(debugLayer)) {
                TF_WARN("Instance layer %s is not present, instance "
                    "creation will fail", debugLayer);
            }
        }
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        createInfo.ppEnabledLayerNames = debugLayers.data();
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(debugLayers.size());
    }

    extensions = _RemoveUnsupportedInstanceExtensions(extensions);
    
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

    #if defined(VK_USE_PLATFORM_METAL_EXT)
        if (std::find(extensions.begin(), extensions.end(),
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) !=
                extensions.end()) {
            createInfo.flags |=
                VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
    #endif

    TF_VERIFY(
        vkCreateInstance(
            &createInfo,
            HgiVulkanAllocator(),
            &_vkInstance) == VK_SUCCESS
    );

    HgiVulkanCreateDebug(this);
}

HgiVulkanInstance::~HgiVulkanInstance()
{
    HgiVulkanDestroyDebug(this);
    vkDestroyInstance(_vkInstance, HgiVulkanAllocator());
}

VkInstance const&
HgiVulkanInstance::GetVulkanInstance() const
{
    return _vkInstance;
}


PXR_NAMESPACE_CLOSE_SCOPE