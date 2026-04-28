//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_DEVICE_FILTER_H
#define PXR_IMAGING_HGIVULKAN_DEVICE_FILTER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hgi/deviceFilter.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/capabilities.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiVulkanDeviceFilter
///
/// Hgi creation will fail if all devices are filtered out.
class HgiVulkanDeviceFilter : public HgiDeviceFilter
{
public:
    virtual ~HgiVulkanDeviceFilter() = default;

    const TfToken& GetTargetHgiName() const override final
    {
        return HgiTokens->Vulkan;
    }

    /// If the filter is directly loading a driver, then instance extensions are
    /// unknown until the instance is actually created. Override this function
    /// to provide the missing extensions, using the workarounds described here:
    /// https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderDriverInterface.md#limitations-of-vk_lunarg_direct_driver_loading
    virtual std::vector<VkExtensionProperties>
    GetDirectDriverInstanceExtensions() const
    {
        return {};
    }

    /// Edit the instance creation info just before it is created. This will
    /// already have been pre-populated according to the HgiVulkan requirements.
    /// Changes should be compatible with the existing instance parameters.
    virtual void PreInstantiate(VkInstanceCreateInfo& info)
    {
    }

    virtual bool FilterDevice(const HgiCapabilities& capabilities) override
    {
        return true;
    }

    /// Overload for more detailed device filtering. This will already have been
    /// pre-populated according to the HgiVulkan requirements. Changes should be
    /// compatible with the existing device parameters.
    virtual bool FilterDevice(const HgiVulkanCapabilities& capabilities,
        VkPhysicalDevice device, VkDeviceCreateInfo& info) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
