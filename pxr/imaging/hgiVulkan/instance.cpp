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
std::vector<const char*>
_RemoveUnsupportedInstanceLayers(
    const std::vector<const char*>& desiredLayers)
{
    // Determine available instance layers.
    uint32_t numAvailableLayers = 0u;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkEnumerateInstanceLayerProperties(&numAvailableLayers, nullptr)
    );
    std::vector<VkLayerProperties> availableLayers;
    availableLayers.resize(numAvailableLayers);
    HGIVULKAN_VERIFY_VK_RESULT(
        vkEnumerateInstanceLayerProperties(
            &numAvailableLayers,
            availableLayers.data())
    );

    std::vector<const char*> layers;

    // Only add layers to the list if they're available.
    for (const auto& lay : desiredLayers) {
        if (std::any_of(availableLayers.begin(), availableLayers.end(),
            [name = lay](const VkLayerProperties& p) 
            { return strcmp(p.layerName, name) == 0; })) {
            layers.push_back(lay);
        } else if (HgiVulkanIsDebugEnabled()) {
            TF_STATUS("Instance layer %s is not available, skipping it", lay);
        }
    }

    return layers;
}

static
std::vector<const char*>
_RemoveUnsupportedInstanceExtensions(
    const std::vector<const char*>& desiredExtensions)
{
    // Determine available instance extensions.
    uint32_t numAvailableExtensions = 0u;
    HGIVULKAN_VERIFY_VK_RESULT(vkEnumerateInstanceExtensionProperties(
        nullptr, &numAvailableExtensions, nullptr));
    std::vector<VkExtensionProperties> availableExtensions;
    availableExtensions.resize(numAvailableExtensions);
    HGIVULKAN_VERIFY_VK_RESULT(vkEnumerateInstanceExtensionProperties(
        nullptr, &numAvailableExtensions,
        availableExtensions.data()));

    std::vector<const char*> extensions;

    // Only add extensions to the list if they're available.
    for (const auto& ext : desiredExtensions) {
        if (std::any_of(availableExtensions.begin(), availableExtensions.end(),
            [name = ext](const VkExtensionProperties& p) 
            { return strcmp(p.extensionName, name) == 0; })) {
            extensions.push_back(ext);
        } else {
            TF_STATUS("Instance extension %s is not available, skipping it",
                ext);
        }
    }

    return extensions;
}

HgiVulkanInstance::HgiVulkanInstance()
    : vkDebugMessenger(nullptr)
    , vkCreateDebugUtilsMessengerEXT(nullptr)
    , vkDestroyDebugUtilsMessengerEXT(nullptr)
    , _vkInstance(nullptr)
    , _hasPresentation(false)
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

    std::vector<const char*> layers;
    if (HgiVulkanIsDebugEnabled()) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    layers = _RemoveUnsupportedInstanceLayers(layers);
    extensions = _RemoveUnsupportedInstanceExtensions(extensions);

    _hasPresentation = std::any_of(extensions.begin(), extensions.end(),
        [](const char* extensionName) {
            return strcmp(extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0;
        });

    createInfo.ppEnabledLayerNames = layers.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());

    #if defined(VK_USE_PLATFORM_METAL_EXT)
        if (std::find(extensions.begin(), extensions.end(),
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) !=
                extensions.end()) {
            createInfo.flags |=
                VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
    #endif

    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateInstance(
            &createInfo,
            HgiVulkanAllocator(),
            &_vkInstance)
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
