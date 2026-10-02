//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_DESCRIPTOR_SET_LAYOUTS_H
#define PXR_IMAGING_HGIVULKAN_DESCRIPTOR_SET_LAYOUTS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include <cstdint>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;

struct HgiVulkanDescriptorSetInfo
{
  uint32_t setNumber;
  VkDescriptorSetLayoutCreateInfo createInfo;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

using HgiVulkanDescriptorSetInfoVector =
    std::vector<HgiVulkanDescriptorSetInfo>;

using VkDescriptorSetLayoutVector = std::vector<VkDescriptorSetLayout>;

/// Binding index of the first texture in a descriptor set.
///
/// Vulkan has one binding space per descriptor set, so textures are offset way
/// above buffers. Using a constant value keeps the index reassignment simple,
/// with the caveat taht client buffer binding indices must stay below this
/// value.
constexpr uint32_t HgiVulkanTextureBindIndexBase = 128;

/// Given all of the DescriptorSetInfos of all of the shader modules in a
/// shader program, this function merges them and creates the descriptorSet
/// layouts needed during pipeline layout creation.
/// The caller takes ownership of the returned layouts and must destroy them.
HGIVULKAN_API
VkDescriptorSetLayoutVector HgiVulkanMakeDescriptorSetLayouts(
    HgiVulkanDevice *device,
    std::vector<HgiVulkanDescriptorSetInfoVector> const &infos,
    std::string const &debugName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
