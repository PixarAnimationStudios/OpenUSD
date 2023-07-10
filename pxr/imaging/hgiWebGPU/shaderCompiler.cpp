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
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/shaderCompiler.h"

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

PXR_NAMESPACE_CLOSE_SCOPE
