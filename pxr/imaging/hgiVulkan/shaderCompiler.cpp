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
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/shaderCompiler.h"
#include "pxr/imaging/hgiVulkan/spirv_reflect.h"

// The glslang header versions must match the vulkan libraries we link against.
// The vulkan SDK (including glslang) must have been compiled with the same
// compiler that HgiVulkan is compiled with.
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <OGLCompilersDLL/InitializeDll.h>


PXR_NAMESPACE_OPEN_SCOPE


EShLanguage
_GetShaderStage(HgiShaderStage stage)
{
    switch(stage) {
        case HgiShaderStageVertex: return EShLangVertex;
        case HgiShaderStageTessellationControl: return EShLangTessControl;
        case HgiShaderStageTessellationEval: return EShLangTessEvaluation;
        case HgiShaderStageGeometry: return EShLangGeometry;
        case HgiShaderStageFragment: return EShLangFragment;
        case HgiShaderStageCompute: return EShLangCompute;
    }

    TF_CODING_ERROR("Unknown stage");
    return EShLangCount;
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
    // InitializeProcess() should be called exactly once per PROCESS.
    // We also have the option to call glslang::FinalizeProcess() in case we
    // need to re-initialize in the process, but we have no such need currently.
    static bool glslangInitialized = glslang::InitializeProcess();

    if (ARCH_UNLIKELY(!glslangInitialized)) {
        TF_CODING_ERROR("Glslang not initialized in process.");
    }

    // Each thread that uses glslang compiler must initialize.
    glslang::InitThread();

    if (numShaderCodes==0 || !spirvOUT) {
        if (errors) {
            errors->append("No shader to compile %s", name);
        }
        return false;
    }

    EShLanguage shaderType = _GetShaderStage(stage);
    glslang::TShader shader(shaderType);
    shader.setStrings(shaderCodes, numShaderCodes);

    //
    // Set up Vulkan/SpirV Environment
    //

    // Maps approx to #define VULKAN 100
    int ClientInputSemanticsVersion = 100;

    glslang::EShTargetClientVersion vulkanClientVersion =
        glslang::EShTargetVulkan_1_0;

    glslang::EShTargetLanguageVersion targetVersion =
        glslang::EShTargetSpv_1_0;

    shader.setEnvInput(
        glslang::EShSourceGlsl,
        shaderType,
        glslang::EShClientVulkan,
        ClientInputSemanticsVersion);

    shader.setEnvClient(glslang::EShClientVulkan, vulkanClientVersion);
    shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

    //
    // Setup compiler limits/caps
    //

    // Reference see file: StandAlone/ResourceLimits.cpp on Khronos git
    const TBuiltInResource DefaultTBuiltInResource = {
        /* .MaxLights = */ 32,
        /* .MaxClipPlanes = */ 6,
        /* .MaxTextureUnits = */ 32,
        /* .MaxTextureCoords = */ 32,
        /* .MaxVertexAttribs = */ 64,
        /* .MaxVertexUniformComponents = */ 4096,
        /* .MaxVaryingFloats = */ 64,
        /* .MaxVertexTextureImageUnits = */ 32,
        /* .MaxCombinedTextureImageUnits = */ 80,
        /* .MaxTextureImageUnits = */ 32,
        /* .MaxFragmentUniformComponents = */ 4096,
        /* .MaxDrawBuffers = */ 32,
        /* .MaxVertexUniformVectors = */ 128,
        /* .MaxVaryingVectors = */ 8,
        /* .MaxFragmentUniformVectors = */ 16,
        /* .MaxVertexOutputVectors = */ 16,
        /* .MaxFragmentInputVectors = */ 15,
        /* .MinProgramTexelOffset = */ -8,
        /* .MaxProgramTexelOffset = */ 7,
        /* .MaxClipDistances = */ 8,
        /* .MaxComputeWorkGroupCountX = */ 65535,
        /* .MaxComputeWorkGroupCountY = */ 65535,
        /* .MaxComputeWorkGroupCountZ = */ 65535,
        /* .MaxComputeWorkGroupSizeX = */ 1024,
        /* .MaxComputeWorkGroupSizeY = */ 1024,
        /* .MaxComputeWorkGroupSizeZ = */ 64,
        /* .MaxComputeUniformComponents = */ 1024,
        /* .MaxComputeTextureImageUnits = */ 16,
        /* .MaxComputeImageUniforms = */ 8,
        /* .MaxComputeAtomicCounters = */ 8,
        /* .MaxComputeAtomicCounterBuffers = */ 1,
        /* .MaxVaryingComponents = */ 60,
        /* .MaxVertexOutputComponents = */ 64,
        /* .MaxGeometryInputComponents = */ 64,
        /* .MaxGeometryOutputComponents = */ 128,
        /* .MaxFragmentInputComponents = */ 128,
        /* .MaxImageUnits = */ 8,
        /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
        /* .MaxCombinedShaderOutputResources = */ 8,
        /* .MaxImageSamples = */ 0,
        /* .MaxVertexImageUniforms = */ 0,
        /* .MaxTessControlImageUniforms = */ 0,
        /* .MaxTessEvaluationImageUniforms = */ 0,
        /* .MaxGeometryImageUniforms = */ 0,
        /* .MaxFragmentImageUniforms = */ 8,
        /* .MaxCombinedImageUniforms = */ 8,
        /* .MaxGeometryTextureImageUnits = */ 16,
        /* .MaxGeometryOutputVertices = */ 256,
        /* .MaxGeometryTotalOutputComponents = */ 1024,
        /* .MaxGeometryUniformComponents = */ 1024,
        /* .MaxGeometryVaryingComponents = */ 64,
        /* .MaxTessControlInputComponents = */ 128,
        /* .MaxTessControlOutputComponents = */ 128,
        /* .MaxTessControlTextureImageUnits = */ 16,
        /* .MaxTessControlUniformComponents = */ 1024,
        /* .MaxTessControlTotalOutputComponents = */ 4096,
        /* .MaxTessEvaluationInputComponents = */ 128,
        /* .MaxTessEvaluationOutputComponents = */ 128,
        /* .MaxTessEvaluationTextureImageUnits = */ 16,
        /* .MaxTessEvaluationUniformComponents = */ 1024,
        /* .MaxTessPatchComponents = */ 120,
        /* .MaxPatchVertices = */ 32,
        /* .MaxTessGenLevel = */ 64,
        /* .MaxViewports = */ 16,
        /* .MaxVertexAtomicCounters = */ 0,
        /* .MaxTessControlAtomicCounters = */ 0,
        /* .MaxTessEvaluationAtomicCounters = */ 0,
        /* .MaxGeometryAtomicCounters = */ 0,
        /* .MaxFragmentAtomicCounters = */ 8,
        /* .MaxCombinedAtomicCounters = */ 8,
        /* .MaxAtomicCounterBindings = */ 1,
        /* .MaxVertexAtomicCounterBuffers = */ 0,
        /* .MaxTessControlAtomicCounterBuffers = */ 0,
        /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
        /* .MaxGeometryAtomicCounterBuffers = */ 0,
        /* .MaxFragmentAtomicCounterBuffers = */ 1,
        /* .MaxCombinedAtomicCounterBuffers = */ 1,
        /* .MaxAtomicCounterBufferSize = */ 16384,
        /* .MaxTransformFeedbackBuffers = */ 4,
        /* .MaxTransformFeedbackInterleavedComponents = */ 64,
        /* .MaxCullDistances = */ 8,
        /* .MaxCombinedClipAndCullDistances = */ 8,
        /* .MaxSamples = */ 4,
        /* .maxMeshOutputVerticesNV = */ 256,
        /* .maxMeshOutputPrimitivesNV = */ 512,
        /* .maxMeshWorkGroupSizeX_NV = */ 32,
        /* .maxMeshWorkGroupSizeY_NV = */ 1,
        /* .maxMeshWorkGroupSizeZ_NV = */ 1,
        /* .maxTaskWorkGroupSizeX_NV = */ 32,
        /* .maxTaskWorkGroupSizeY_NV = */ 1,
        /* .maxTaskWorkGroupSizeZ_NV = */ 1,
        /* .maxMeshViewCountNV = */ 4,

        /* .limits = */ {
            /* .nonInductiveForLoops = */ 1,
            /* .whileLoops = */ 1,
            /* .doWhileLoops = */ 1,
            /* .generalUniformIndexing = */ 1,
            /* .generalAttributeMatrixVectorIndexing = */ 1,
            /* .generalVaryingIndexing = */ 1,
            /* .generalSamplerIndexing = */ 1,
            /* .generalVariableIndexing = */ 1,
            /* .generalConstantMatrixVectorIndexing = */ 1,
        }};

    EShMessages messages = (EShMessages) (EShMsgSpvRules | EShMsgVulkanRules);

    //
    // Run PreProcess
    //
    const int defaultVersion = 100;
    std::string preprocessedGLSL;

    // Includes are resolved in glslfx so we currently do not support
    // using #include in glsl (GL_GOOGLE_include_directive).
    glslang::TShader::ForbidIncluder noIncludes;

    bool preProcessOK = shader.preprocess(
            &DefaultTBuiltInResource,
            defaultVersion,
            ECoreProfile,
            false,
            false,
            messages,
            &preprocessedGLSL,
            noIncludes);

    if (!preProcessOK) {
        if (errors) {
            errors->append("GLSL Preprocessing Failed for: ");
            errors->append(name);
            errors->append("\n");
            errors->append(shader.getInfoLog());
            errors->append(shader.getInfoDebugLog());
        }
        return false;
    }

    //
    // Parse and link shader
    //
    const char* preprocessedCStr = preprocessedGLSL.c_str();
    shader.setStrings(&preprocessedCStr, 1);

    if (!shader.parse(&DefaultTBuiltInResource, 100, false, messages)) {
        if (errors) {
            errors->append("GLSL Parsing Failed for: ");
            errors->append(name);
            errors->append("\n");
            errors->append(shader.getInfoLog());
            errors->append(shader.getInfoDebugLog());
        }
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if(!program.link(messages)) {
        if (errors) {
            errors->append("GLSL linking failed for: ");
            errors->append(name);
            errors->append("\n");
            errors->append(shader.getInfoLog());
            errors->append(shader.getInfoDebugLog());
        }
        return false;
    }

    //
    // Convert to SPIRV
    //
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = false;
    spvOptions.optimizeSize = false;
    spvOptions.disassemble = false;
    spvOptions.validate = false;

    glslang::GlslangToSpv(
        *program.getIntermediate(shaderType),
        *spirvOUT,
        &logger,
        &spvOptions);

    if (logger.getAllMessages().length() > 0) {
        if (errors) {
            errors->append(logger.getAllMessages().c_str());
        }
    }

    // glslang can also directly output the spirv binary for us:
    // glslang::OutputSpvBin(*spirvOUT, "filename.spv");

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
    TF_VERIFY(
        vkCreateDescriptorSetLayout(
            device->GetVulkanDevice(),
            &createInfo,
            HgiVulkanAllocator(),
            &layout) == VK_SUCCESS
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

                // If a vertex module and a fragment module access the same
                // resource, we need to merge the stageFlags.
                dst->stageFlags |= bi.stageFlags;
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
