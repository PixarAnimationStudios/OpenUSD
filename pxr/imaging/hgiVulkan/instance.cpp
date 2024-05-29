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


PXR_NAMESPACE_OPEN_SCOPE


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
        #elif defined(VK_USE_PLATFORM_MACOS_MVK)
            VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
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
    const char* debugLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    if (HgiVulkanIsDebugEnabled()) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        createInfo.ppEnabledLayerNames = debugLayers;
        createInfo.enabledLayerCount = (uint32_t)  TfArraySize(debugLayers);
    }

    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = (uint32_t) extensions.size();

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
