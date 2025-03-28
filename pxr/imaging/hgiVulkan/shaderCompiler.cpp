//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/shaderCompiler.h"
#include "pxr/imaging/hgiVulkan/spirv_reflect.h"

#include <shaderc/shaderc.hpp>

#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE


static shaderc_shader_kind
_GetShaderStage(HgiShaderStage stage)
{
    switch(stage) {
        case HgiShaderStageVertex:
            return shaderc_glsl_vertex_shader;
        case HgiShaderStageTessellationControl:
            return shaderc_glsl_tess_control_shader;
        case HgiShaderStageTessellationEval:
            return shaderc_glsl_tess_evaluation_shader;
        case HgiShaderStageGeometry:
            return shaderc_glsl_geometry_shader;
        case HgiShaderStageFragment:
            return shaderc_glsl_fragment_shader;
        case HgiShaderStageCompute:
            return shaderc_glsl_compute_shader;
    }

    TF_CODING_ERROR("Unknown stage");
    return shaderc_glsl_infer_from_source;
}

bool
HgiVulkanCompileGLSL(
    const char* name,
    const char* shaderCodes[],
    uint8_t numShaderCodes,
    HgiShaderStage stage,
    std::vector<unsigned int>* spirvOUT,
    std::string* errors)
{
    if (numShaderCodes==0 || !spirvOUT) {
        if (errors) {
            errors->append("No shader to compile %s", name);
        }
        return false;
    }

    std::string source;
    for (uint8_t i=0; i<numShaderCodes; ++i) {
        source += shaderCodes[i];
    }

    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan,
                                 shaderc_env_version_vulkan_1_0);
    options.SetTargetSpirv(shaderc_spirv_version_1_0);

    shaderc_shader_kind const kind = _GetShaderStage(stage);

    shaderc::Compiler compiler;
    shaderc::SpvCompilationResult result =
        compiler.CompileGlslToSpv(source, kind, name, options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        *errors = result.GetErrorMessage();
        return false;
    }

    spirvOUT->assign(result.cbegin(), result.cend());

    return true;
}

static bool
_VerifyResults(SpvReflectShaderModule* module, SpvReflectResult const& result)
{
    if (!TF_VERIFY(result == SPV_REFLECT_RESULT_SUCCESS)) {
        spvReflectDestroyShaderModule(module);
        return false;
    }

    return true;
}

static VkDescriptorSetLayout
_CreateDescriptorSetLayout(
    HgiVulkanDevice* device,
    VkDescriptorSetLayoutCreateInfo const& createInfo,
    std::string const& debugName)
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

HgiVulkanDescriptorSetInfoVector
HgiVulkanGatherDescriptorSetInfo(
    std::vector<unsigned int> const& spirv)
{
    // This code is based on main_descriptors.cpp in the SPIRV-Reflect repo.

    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(
        spirv.size()*sizeof(uint32_t), spirv.data(), &module);
    if (!_VerifyResults(&module, result)) {
        return HgiVulkanDescriptorSetInfoVector();
    }

    uint32_t count = 0;
    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    if (!_VerifyResults(&module, result)) {
        return HgiVulkanDescriptorSetInfoVector();
    }

    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    if (!_VerifyResults(&module, result)) {
        return HgiVulkanDescriptorSetInfoVector();
    }

    // Generate all necessary data structures to create a VkDescriptorSetLayout
    // for each descriptor set in this shader.
    std::vector<HgiVulkanDescriptorSetInfo> infos(sets.size());

    for (size_t s = 0; s < sets.size(); s++) {
        SpvReflectDescriptorSet const& reflSet = *(sets[s]);
        HgiVulkanDescriptorSetInfo& info = infos[s];
        info.bindings.resize(reflSet.binding_count);

        for (uint32_t b = 0; b < reflSet.binding_count; b++) {
            SpvReflectDescriptorBinding const& reflBinding =
                *(reflSet.bindings[b]);

            VkDescriptorSetLayoutBinding& layoutBinding = info.bindings[b];
            layoutBinding.binding = reflBinding.binding;
            layoutBinding.descriptorType =
                static_cast<VkDescriptorType>(reflBinding.descriptor_type);
            layoutBinding.descriptorCount = 1;

            for (uint32_t d = 0; d < reflBinding.array.dims_count; d++) {
                layoutBinding.descriptorCount *= reflBinding.array.dims[d];
            }
            layoutBinding.stageFlags =
                static_cast<VkShaderStageFlagBits>(module.shader_stage);
        }

        info.setNumber = reflSet.set;
    }

    spvReflectDestroyShaderModule(&module);

    return infos;
}

static bool
_IsDescriptorTextureType(VkDescriptorType descType) {
    return (descType == VK_DESCRIPTOR_TYPE_SAMPLER ||
            descType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            descType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
            descType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
}

VkDescriptorSetLayoutVector
HgiVulkanMakeDescriptorSetLayouts(
    HgiVulkanDevice* device,
    std::vector<HgiVulkanDescriptorSetInfoVector> const& infos,
    std::string const& debugName)
{
    std::unordered_map<uint32_t, HgiVulkanDescriptorSetInfo> mergedInfos;

    // Merge the binding info of each of the infos such that the resource
    // bindings information for each of the shader stage modules is merged
    // together. For example a vertex shader may have different buffers and
    // textures bound than a fragment shader. We merge them all together to
    // create the descriptor set layout for that shader program.

    for (HgiVulkanDescriptorSetInfoVector const& infoVec : infos) {
        for (HgiVulkanDescriptorSetInfo const& info : infoVec) {

            // Get the set (or create one)
            HgiVulkanDescriptorSetInfo& trg = mergedInfos[info.setNumber];

            for (VkDescriptorSetLayoutBinding const& bi : info.bindings) {

                // If two shader modules have the same binding information for
                // a specific resource, we only want to insert it once.
                // For example both the vertex shaders and fragment shader may
                // have a texture bound at the same binding index.

                VkDescriptorSetLayoutBinding* dst = nullptr;
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
        HgiVulkanDescriptorSetInfo& info = pair.second;
        info.createInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.createInfo.bindingCount = info.bindings.size();
        info.createInfo.pBindings = info.bindings.data();
    }

    // Create VkDescriptorSetLayouts out of the merge infos above.
    VkDescriptorSetLayoutVector layouts;

    for (auto const& pair : mergedInfos) {
        HgiVulkanDescriptorSetInfo const& info = pair.second;
        VkDescriptorSetLayout layout = _CreateDescriptorSetLayout(
            device, info.createInfo, debugName);
        layouts.push_back(layout);
    }

    return layouts;
}

PXR_NAMESPACE_CLOSE_SCOPE