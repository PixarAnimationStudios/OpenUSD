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
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/sampler.h"

#include <float.h>

PXR_NAMESPACE_OPEN_SCOPE


HgiVulkanSampler::HgiVulkanSampler(
    HgiVulkanDevice* device,
    HgiSamplerDesc const& desc)
    : HgiSampler(desc)
    , _vkSampler(nullptr)
    , _device(device)
    , _inflightBits(0)
{
    VkSamplerCreateInfo sampler = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = HgiVulkanConversions::GetMinMagFilter(desc.magFilter);
    sampler.minFilter = HgiVulkanConversions::GetMinMagFilter(desc.minFilter);
    sampler.addressModeU =
        HgiVulkanConversions::GetSamplerAddressMode(desc.addressModeU);
    sampler.addressModeV =
        HgiVulkanConversions::GetSamplerAddressMode(desc.addressModeV);
    sampler.addressModeW =
        HgiVulkanConversions::GetSamplerAddressMode(desc.addressModeW);

    sampler.compareEnable = VK_FALSE; // Eg. Percentage-closer filtering
    sampler.compareOp = VK_COMPARE_OP_ALWAYS;

    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    sampler.mipLodBias = 0.0f;
    sampler.mipmapMode = HgiVulkanConversions::GetMipFilter(desc.mipFilter);
    sampler.minLod = 0.0f;
    sampler.maxLod = FLT_MAX;

    HgiVulkanCapabilities const& caps = device->GetDeviceCapabilities();
    sampler.anisotropyEnable = caps.vkDeviceFeatures.samplerAnisotropy;
    sampler.maxAnisotropy = sampler.anisotropyEnable ?
        caps.vkDeviceProperties.limits.maxSamplerAnisotropy : 1.0f;

    TF_VERIFY(
        vkCreateSampler(
            device->GetVulkanDevice(),
            &sampler,
            HgiVulkanAllocator(),
            &_vkSampler) == VK_SUCCESS
    );
}

HgiVulkanSampler::~HgiVulkanSampler()
{
    vkDestroySampler(
        _device->GetVulkanDevice(),
        _vkSampler,
        HgiVulkanAllocator());
}

uint64_t
HgiVulkanSampler::GetRawResource() const
{
    return (uint64_t) _vkSampler;
}

VkSampler
HgiVulkanSampler::GetVulkanSampler() const
{
    return _vkSampler;
}

HgiVulkanDevice*
HgiVulkanSampler::GetDevice() const
{
    return _device;
}

uint64_t &
HgiVulkanSampler::GetInflightBits()
{
    return _inflightBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
