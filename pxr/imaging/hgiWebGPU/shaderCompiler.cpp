//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiWebGPU/shaderCompiler.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/imaging/hgiWebGPU/spirvTransforms.h"

#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <spirv-tools/optimizer.hpp>

PXR_NAMESPACE_OPEN_SCOPE

static EShLanguage
GetShaderStage(HgiShaderStage stage)
{
    switch(stage) {
        case HgiShaderStageVertex:
            return EShLangVertex;
        case HgiShaderStageTessellationControl:
            return EShLangTessControl;
        case HgiShaderStageTessellationEval:
            return EShLangTessEvaluation;
        case HgiShaderStageGeometry:
            return EShLangGeometry;
        case HgiShaderStageFragment:
            return EShLangFragment;
        case HgiShaderStageCompute:
            return EShLangCompute;
        default:
            TF_CODING_ERROR("Unknown stage");
            return EShLangCount;
    }
}

bool
HgiWebGPUCompileGLSL(
    const char* name,
    const char* shaderCodes[],
    uint8_t numShaderCodes,
    HgiShaderStage stage,
    std::vector<uint32_t>* spirvOut,
    std::string* errors)
{
    if (ARCH_UNLIKELY(!spirvOut)) {
        TF_CODING_ERROR("spirvOut is null");
        return false;
    }

    if (ARCH_UNLIKELY(numShaderCodes == 0)) {
        if (errors) {
            errors->append(TfStringPrintf("No shader to compile %s", name));
        }
        return false;
    }

    std::string source;
    for (uint8_t i=0; i<numShaderCodes; ++i) {
        source += shaderCodes[i];
    }

    const EShLanguage glslangStage = GetShaderStage(stage);

    glslang::TShader shader(glslangStage);
    const char* shader_strings = source.data();
    const int shader_lengths = static_cast<int>(source.size());
    shader.setStringsWithLengthsAndNames(&shader_strings, &shader_lengths,
                                         &name, 1);
    shader.setEntryPoint("main");
    shader.setAutoMapLocations(false);
    shader.setAutoMapBindings(false);
    shader.setEnvClient(glslang::EShClientVulkan,
                        glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EshTargetSpv,
                        glslang::EShTargetSpv_1_0);

    glslang::TProgram program;
    program.addShader(&shader);
    auto controls = static_cast<EShMessages>(EShMsgSpvRules |
        EShMsgVulkanRules | EShMsgCascadingErrors);
    const int defaultVersion = 110;
    const bool forwardCompatible = false;
    bool success = shader.parse(GetDefaultResources(),
        defaultVersion, forwardCompatible, controls);

    if (!success) {
        errors->append(shader.getInfoLog());
        errors->append(shader.getInfoDebugLog());
        return false;
    }

    glslang::SpvOptions options;
    options.generateDebugInfo = false;
    options.disableOptimizer = true;
    options.optimizeSize = false;
    success = program.link(EShMsgDefault) && program.mapIO();
    if (!success) {
        errors->append(program.getInfoLog());
        errors->append(program.getInfoDebugLog());
        return false;
    }

    spv::SpvBuildLogger logger;
    glslang::GlslangToSpv(*program.getIntermediate(glslangStage),
        *spirvOut, &logger, &options);

    std::string warningErrors = logger.getAllMessages();

    if (!warningErrors.empty()) {
        errors->append(warningErrors);
        return false;
    }

    // Storm generates a lot of dead code, and even dead buffer bindings that
    // waste precious binding slots (we only have 10!). Clean this up.
    spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_0);
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());
    std::vector<uint32_t> optimized;
    if (!optimizer.Run(spirvOut->data(), spirvOut->size(), &optimized)) {
        if (errors) {
            errors->append("SPIR-V optimization failed");
        }
        return false;
    }
    *spirvOut = std::move(optimized);

    if (stage == HgiShaderStageVertex) {
        // Since the WebGPU coordinate convention is inverted for
        // framebuffers and textures compared to the expected OpenGL
        // convention, Hgi wants us to render upside down, which we
        // do by negating the gl_Position.y value. Metal has the
        // same issue, and it solves it by using a negative viewport
        // height, which isn't supported by WebGPU, so we have to take
        // a more complicate approach... Flipping Y also changes
        // the handedness of the coordinate system, which reverses
        // the winding order. For that we solve it the same way as
        // Metal does: return the opposite in
        // HgiWebGPUConversions::GetWinding().
        if (ARCH_UNLIKELY(!ApplySpirvViewportFlip(*spirvOut))) {
            errors->append("ApplySpirvViewportFlip transform failed");
            return false;
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
