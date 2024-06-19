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
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

#include <vector>
#include <algorithm>


PXR_NAMESPACE_OPEN_SCOPE


HgiVulkanInstance::HgiVulkanInstance()
    : vkDebugMessenger(nullptr)
    , vkCreateDebugUtilsMessengerEXT(nullptr)
    , vkDestroyDebugUtilsMessengerEXT(nullptr)
    , _vkInstance(nullptr)
{
    TF_VERIFY(
        volkInitialize() == VK_SUCCESS
    );

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
    const char* debugLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    if (HgiVulkanIsDebugEnabled()) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        createInfo.ppEnabledLayerNames = debugLayers;
        createInfo.enabledLayerCount = static_cast<uint32_t>(TfArraySize(debugLayers));
    }

    uint32_t numAvailableExtensions = 0u;
    TF_VERIFY(vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions,
        nullptr) == VK_SUCCESS);
    std::vector<VkExtensionProperties> availableExtensions;
    availableExtensions.resize(numAvailableExtensions);
    TF_VERIFY(vkEnumerateInstanceExtensionProperties(nullptr, &numAvailableExtensions,
        availableExtensions.data()) == VK_SUCCESS);

    for (auto iter = extensions.begin(); iter != extensions.end();) {
        if (std::find_if(availableExtensions.begin(), availableExtensions.end(),
                [name = *iter](const VkExtensionProperties& p) {return strcmp(p.extensionName, name) == 0;})
                == availableExtensions.end()) {
            iter = extensions.erase(iter);
        } else {
            ++iter;
        }
    }

    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

    #if defined(VK_USE_PLATFORM_METAL_EXT)
        if (std::find(extensions.begin(), extensions.end(),
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) != extensions.end()) {
            createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
    #endif

    TF_VERIFY(
        vkCreateInstance(
            &createInfo,
            HgiVulkanAllocator(),
            &_vkInstance) == VK_SUCCESS
    );

    volkLoadInstance(_vkInstance);

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

PFN_vkGetInstanceProcAddr
HgiVulkanInstance::GetPFNInstancProcAddr()
{
    return vkGetInstanceProcAddr;
}

PXR_NAMESPACE_CLOSE_SCOPE
