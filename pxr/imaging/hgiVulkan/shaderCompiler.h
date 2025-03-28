//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_SHADERCOMPILER_H
#define PXR_IMAGING_HGIVULKAN_SHADERCOMPILER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

#include <stdint.h>
#include <stdlib.h>
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


/// Compiles ascii shader code (glsl) into spirv binary code (spirvOut).
/// Returns true if successful. Errors can optionally be captured.
/// numShaderCodes determines how many strings are provided via shaderCodes.
/// 'name' is purely for debugging compile errors. It can be anything.
HGIVULKAN_API
bool HgiVulkanCompileGLSL(
    const char* name,
    const char* shaderCodes[],
    uint8_t numShaderCodes,
    HgiShaderStage stage,
    std::vector<unsigned int>* spirvOUT,
    std::string* errors = nullptr);

/// Uses spirv-reflection to create new descriptor set layout information for
/// the provided spirv.
/// This information can be merged with the info of the other shader stage
/// modules to create the pipeline layout.
/// During Hgi pipeline layout creation we know the shader modules
/// (HgiShaderProgram), but not the HgiResourceBindings so we must use
/// spirv reflection to discover the descriptorSet info for the module.
HGIVULKAN_API
HgiVulkanDescriptorSetInfoVector HgiVulkanGatherDescriptorSetInfo(
    std::vector<unsigned int> const& spirv);

/// Given all of the DescriptorSetInfos of all of the shader modules in a
/// shader program, this function merges them and creates the descriptorSet
/// layouts needed during pipeline layout creation.
/// The caller takes ownership of the returned layouts and must destroy them.
HGIVULKAN_API
VkDescriptorSetLayoutVector HgiVulkanMakeDescriptorSetLayouts(
    HgiVulkanDevice* device,
    std::vector<HgiVulkanDescriptorSetInfoVector> const& infos,
    std::string const& debugName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
