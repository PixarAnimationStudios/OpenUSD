//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hgiWebGPU/shaderCompiler.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/Public/ResourceLimits.h"

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

    glslang::InitializeProcess();
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
    auto controls = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgCascadingErrors);
    const int defaultVersion = 110;
    const bool forwardCompatible = false;
    bool success = shader.parse(GetDefaultResources(),  defaultVersion, forwardCompatible ,controls);

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
    glslang::GlslangToSpv(*program.getIntermediate(glslangStage), *spirvOUT, &logger, &options);

    std::string warningErrors = logger.getAllMessages();

    if (!warningErrors.empty()) {
        errors->append(warningErrors);
        return false;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
