//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_INSTANCE_H
#define PXR_IMAGING_HGIVULKAN_INSTANCE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HgiVkInstance
///
/// Initializes the Vulkan library and contains the apps Vulkan state.
///
class HgiVulkanInstance final
{
public:
    HGIVULKAN_API
    HgiVulkanInstance();

    HGIVULKAN_API
    ~HgiVulkanInstance();

    /// Returns the vulkan instance.
    HGIVULKAN_API
    VkInstance const& GetVulkanInstance() const;

    /// Instance Extension function pointers
    VkDebugUtilsMessengerEXT vkDebugMessenger = 0;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = 0;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = 0;

private:
    VkInstance _vkInstance;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif