//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//
#include "pxr/imaging/hgiVulkan/descriptorSetLayouts.h"

#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

VkDescriptorSetLayout
_CreateDescriptorSetLayout(
    HgiVulkanDevice *device,
    VkDescriptorSetLayoutCreateInfo const &createInfo,
    std::string const &debugName)
{
    VkDescriptorSetLayout layout = nullptr;
    HGIVULKAN_VERIFY_VK_RESULT(
        vkCreateDescriptorSetLayout(
            device->GetVulkanDevice(),
            &createInfo,
            HgiVulkanAllocator(),
            &layout)
    );

    // Debug label
    if (!debugName.empty()) {
        std::string debugLabel = "DescriptorSetLayout " + debugName;
        HgiVulkanSetDebugName(
            device,
            (uint64_t)layout,
            VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
            debugLabel.c_str());
    }

    return layout;
}

bool
_IsDescriptorTextureType(VkDescriptorType descType) {
    return (descType == VK_DESCRIPTOR_TYPE_SAMPLER ||
            descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            descType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
            descType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
}

} // namespace

VkDescriptorSetLayoutVector
HgiVulkanMakeDescriptorSetLayouts(
    HgiVulkanDevice *device,
    std::vector<HgiVulkanDescriptorSetInfoVector> const &infos,
    std::string const &debugName)
{
    std::unordered_map<uint32_t, HgiVulkanDescriptorSetInfo> mergedInfos;

    // Merge the binding info of each of the infos such that the resource
    // bindings information for each of the shader stage modules is merged
    // together. For example a vertex shader may have different buffers and
    // textures bound than a fragment shader. We merge them all together to
    // create the descriptor set layout for that shader program.

    for (HgiVulkanDescriptorSetInfoVector const &infoVec : infos) {
        for (HgiVulkanDescriptorSetInfo const &info : infoVec) {

            // Get the set (or create one)
            HgiVulkanDescriptorSetInfo &trg = mergedInfos[info.setNumber];

            for (VkDescriptorSetLayoutBinding const &bi : info.bindings) {

                // If two shader modules have the same binding information for
                // a specific resource, we only want to insert it once.
                // For example both the vertex shaders and fragment shader may
                // have a texture bound at the same binding index.

                VkDescriptorSetLayoutBinding *dst = nullptr;
                for (VkDescriptorSetLayoutBinding& bind : trg.bindings) {
                    if (bind.binding == bi.binding) {
                        dst = &bind;
                        break;
                    }
                }

                // It is a new binding we haven't seen before. Add it
                if (!dst) {
                    trg.setNumber = info.setNumber;
                    trg.bindings.push_back(bi);
                    dst = &trg.bindings.back();
                }

                // These need to match the shader stages used when creating the
                // VkDescriptorSetLayout in HgiVulkanResourceBindings.
                if (dst->stageFlags != HgiVulkanConversions::GetShaderStages(
                    HgiShaderStageCompute)) {

                    if (_IsDescriptorTextureType(dst->descriptorType)) {
                        dst->stageFlags =
                            HgiVulkanConversions::GetShaderStages(
                                HgiShaderStageGeometry |
                                HgiShaderStageFragment);
                    } else {
                        dst->stageFlags =
                            HgiVulkanConversions::GetShaderStages(
                                HgiShaderStageVertex |
                                HgiShaderStageTessellationControl |
                                HgiShaderStageTessellationEval |
                                HgiShaderStageGeometry |
                                HgiShaderStageFragment);
                    }
                }
            }
        }
    }

    // Generate the VkDescriptorSetLayoutCreateInfos for the bindings we merged.
    for (auto& pair : mergedInfos) {
        HgiVulkanDescriptorSetInfo &info = pair.second;
        info.createInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.createInfo.bindingCount = info.bindings.size();
        info.createInfo.pBindings = info.bindings.data();
    }

    // Create VkDescriptorSetLayouts out of the merge infos above.
    VkDescriptorSetLayoutVector layouts;

    for (auto const &pair : mergedInfos) {
        HgiVulkanDescriptorSetInfo const &info = pair.second;
        VkDescriptorSetLayout layout = _CreateDescriptorSetLayout(
            device, info.createInfo, debugName);
        layouts.push_back(layout);
    }

    return layouts;
}

PXR_NAMESPACE_CLOSE_SCOPE
