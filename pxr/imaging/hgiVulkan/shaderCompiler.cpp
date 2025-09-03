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
    if (HgiVulkanIsDebugEnabled()) {
        options.SetGenerateDebugInfo();
    }

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

PXR_NAMESPACE_CLOSE_SCOPE
