//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_VULKAN_SAMPLER_H
#define PXR_IMAGING_HGI_VULKAN_SAMPLER_H

#include "pxr/imaging/hgi/sampler.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;


///
/// \class HgiVulkanSampler
///
/// Vulkan implementation of HgiSampler
///
class HgiVulkanSampler final : public HgiSampler
{
public:
    HGIVULKAN_API
    ~HgiVulkanSampler() override;

    HGIVULKAN_API
    uint64_t GetRawResource() const override;

    /// Returns the vulkan sampler object.
    HGIVULKAN_API
    VkSampler GetVulkanSampler() const;

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanSampler(
        HgiVulkanDevice* device,
        HgiSamplerDesc const& desc);

private:
    HgiVulkanSampler() = delete;
    HgiVulkanSampler & operator=(const HgiVulkanSampler&) = delete;
    HgiVulkanSampler(const HgiVulkanSampler&) = delete;

    VkSampler _vkSampler;

    HgiVulkanDevice* _device;
    uint64_t _inflightBits;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif