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
#ifndef PXR_IMAGING_HGI_VULKAN_RESOURCEBINDINGS_H
#define PXR_IMAGING_HGI_VULKAN_RESOURCEBINDINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;


///
/// \class HgiVulkanResourceBindings
///
/// Vulkan implementation of HgiResourceBindings.
///
///
class HgiVulkanResourceBindings final : public HgiResourceBindings
{
public:
    HGIVULKAN_API
    ~HgiVulkanResourceBindings() override;

    /// Binds the resources to GPU.
    HGIVULKAN_API
    void BindResources(
        VkCommandBuffer cb,
        VkPipelineBindPoint bindPoint,
        VkPipelineLayout layout);

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanResourceBindings(
        HgiVulkanDevice* device,
        HgiResourceBindingsDesc const& desc);

private:
    HgiVulkanResourceBindings() = delete;
    HgiVulkanResourceBindings & operator=(const HgiVulkanResourceBindings&) = delete;
    HgiVulkanResourceBindings(const HgiVulkanResourceBindings&) = delete;

    HgiVulkanDevice* _device;
    uint64_t _inflightBits;

    VkDescriptorPool _vkDescriptorPool;
    VkDescriptorSetLayout _vkDescriptorSetLayout;
    VkDescriptorSet _vkDescriptorSet;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
