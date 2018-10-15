//
// Copyright 2018 Pixar
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
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/glf/debugCodes.h"
#include "pxr/imaging/glf/glew.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <iostream>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


TF_INSTANTIATE_SINGLETON(GlfContextCaps);

TF_DEFINE_ENV_SETTING(GLF_ENABLE_SHADER_STORAGE_BUFFER, true,
                      "Use GL shader storage buffer (OpenGL 4.3)");
TF_DEFINE_ENV_SETTING(GLF_ENABLE_BINDLESS_BUFFER, false,
                      "Use GL bindless buffer extention");
TF_DEFINE_ENV_SETTING(GLF_ENABLE_BINDLESS_TEXTURE, false,
                      "Use GL bindless texture extention");
TF_DEFINE_ENV_SETTING(GLF_ENABLE_MULTI_DRAW_INDIRECT, true,
                      "Use GL mult draw indirect extention");
TF_DEFINE_ENV_SETTING(GLF_ENABLE_DIRECT_STATE_ACCESS, true,
                      "Use GL direct state access extention");
TF_DEFINE_ENV_SETTING(GLF_ENABLE_COPY_BUFFER, true,
                      "Use GL copy buffer data");
TF_DEFINE_ENV_SETTING(GLF_ENABLE_SHADER_DRAW_PARAMETERS, true,
                      "Use GL shader draw params if available (OpenGL 4.5+)");

TF_DEFINE_ENV_SETTING(GLF_GLSL_VERSION, 0,
                      "GLSL version");


// Initialize members to ensure a sane starting state.
GlfContextCaps::GlfContextCaps()
    : glVersion(0)
    , coreProfile(false)

    , maxArrayTextureLayers(0)
    , maxUniformBlockSize(0)
    , maxShaderStorageBlockSize(0)
    , maxTextureBufferSize(0)
    , uniformBufferOffsetAlignment(0)

    , arrayTexturesEnabled(false)
    , shaderStorageBufferEnabled(false)
    , bufferStorageEnabled(false)
    , directStateAccessEnabled(false)
    , multiDrawIndirectEnabled(false)
    , bindlessTextureEnabled(false)
    , bindlessBufferEnabled(false)

    , glslVersion(400)
    , explicitUniformLocation(false)
    , shadingLanguage420pack(false)
    , shaderDrawParametersEnabled(false)

    , copyBufferEnabled(true)
{
}

/*static*/
GlfContextCaps&
GlfContextCaps::GetInstance()
{
    // Make sure the render context caps have been populated.
    // This needs to be called on a thread that has the gl context
    // bound before we go wide on the cpus.
    //
    // Because we have unit tests that side step almost all Hd machinery, we 
    // must call _LoadCaps() here, so we can ensure the object is in a good 
    // state for all clients.
    //
    // XXX: Move this to an render context change event api. (bug #124971)

    static std::once_flag renderContextLoad;
    GlfContextCaps& caps = TfSingleton<GlfContextCaps>::GetInstance();
    std::call_once(renderContextLoad, [&caps](){ caps._LoadCaps(); });
    return caps;
}

void
GlfContextCaps::_LoadCaps()
{
    // XXX: consider to move this class into glf

    // note that this function is called without GL context, in some unit tests.

    arrayTexturesEnabled         = false;
    shaderStorageBufferEnabled   = false;
    bufferStorageEnabled         = false;
    directStateAccessEnabled     = false;
    multiDrawIndirectEnabled     = false;
    bindlessTextureEnabled       = false;
    bindlessBufferEnabled        = false;
    explicitUniformLocation      = false;
    shadingLanguage420pack       = false;
    shaderDrawParametersEnabled  = false;
    maxArrayTextureLayers        = 256;          // GL spec minimum
    maxUniformBlockSize          = 16*1024;      // GL spec minimum
    maxShaderStorageBlockSize    = 16*1024*1024; // GL spec minimum
    maxTextureBufferSize         = 64*1024;      // GL spec minimum
    uniformBufferOffsetAlignment = 0;

    const char *glVersionStr = (const char*)glGetString(GL_VERSION);

    // GL hasn't been initialized yet.
    if (glVersionStr == NULL) return;

    const char *dot = strchr(glVersionStr, '.');
    if (TF_VERIFY((dot && dot != glVersionStr),
                  "Can't parse GL_VERSION %s", glVersionStr)) {
        // GL_VERSION = "4.5.0 <vendor> <version>"
        //              "4.1 <vendor-os-ver> <version>"
        //              "4.1 <vendor-os-ver>"
        int major = std::max(0, std::min(9, *(dot-1) - '0'));
        int minor = std::max(0, std::min(9, *(dot+1) - '0'));
        glVersion = major * 100 + minor * 10;
    }

    if (glVersion >= 200) {
        const char *glslVersionStr =
            (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        dot = strchr(glslVersionStr, '.');
        if (TF_VERIFY((dot && dot != glslVersionStr),
                      "Can't parse GL_SHADING_LANGUAGE_VERSION %s",
                      glslVersionStr)) {
            // GL_SHADING_LANGUAGE_VERSION = "4.10"
            //                               "4.50 <vendor>"
            int major = std::max(0, std::min(9, *(dot-1) - '0'));
            int minor = std::max(0, std::min(9, *(dot+1) - '0'));
            glslVersion = major * 100 + minor * 10;
        }
    } else {
        glslVersion = 0;
    }

    if (glVersion >= 300) {
        glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayTextureLayers);
        arrayTexturesEnabled = true;
    }

    // initialize by Core versions
    if (glVersion >= 310) {
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,
                      &maxUniformBlockSize);
        glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE,
                      &maxTextureBufferSize);
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
                      &uniformBufferOffsetAlignment);
    }
    if (glVersion >= 320) {
        GLint profileMask = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
        coreProfile = (profileMask & GL_CONTEXT_CORE_PROFILE_BIT);
    }
    if (glVersion >= 420) {
        shadingLanguage420pack = true;
    }
    if (glVersion >= 430) {
        shaderStorageBufferEnabled = true;
        explicitUniformLocation = true;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,
                      &maxShaderStorageBlockSize);
    }
    if (glVersion >= 440) {
        bufferStorageEnabled = true;
    }
    if (glVersion >= 450) {
        multiDrawIndirectEnabled = true;
        directStateAccessEnabled = true;
    }
    if (glVersion >= 460) {
        shaderDrawParametersEnabled = true;
    }

    // initialize by individual extension.
    if (GLEW_ARB_bindless_texture && glMakeTextureHandleResidentNV) {
        bindlessTextureEnabled = true;
    }
    if (GLEW_NV_shader_buffer_load && glMakeNamedBufferResidentNV) {
        bindlessBufferEnabled = true;
    }
    if (GLEW_ARB_explicit_uniform_location) {
        explicitUniformLocation = true;
    }
    if (GLEW_ARB_shading_language_420pack) {
        shadingLanguage420pack = true;
    }
    if (GLEW_ARB_multi_draw_indirect) {
        multiDrawIndirectEnabled = true;
    }
#if defined(GLEW_VERSION_4_5)  // glew 1.11 or newer (usd requirement is 1.10)
    if (GLEW_ARB_direct_state_access) {
        directStateAccessEnabled = true;
    }
    if (GLEW_ARB_shader_draw_parameters) {
        shaderDrawParametersEnabled = true;
    }
#endif
    if (GLEW_EXT_direct_state_access) {
        directStateAccessEnabled = true;
    }

    // Environment variable overrides (only downgrading is possible)
    if (!TfGetEnvSetting(GLF_ENABLE_SHADER_STORAGE_BUFFER)) {
        shaderStorageBufferEnabled = false;
    }
    if (!TfGetEnvSetting(GLF_ENABLE_BINDLESS_TEXTURE)) {
        bindlessTextureEnabled = false;
    }
    if (!TfGetEnvSetting(GLF_ENABLE_BINDLESS_BUFFER)) {
        bindlessBufferEnabled = false;
    }
    if (!TfGetEnvSetting(GLF_ENABLE_MULTI_DRAW_INDIRECT)) {
        multiDrawIndirectEnabled = false;
    }
    if (!TfGetEnvSetting(GLF_ENABLE_DIRECT_STATE_ACCESS)) {
        directStateAccessEnabled = false;
    }
    if (!TfGetEnvSetting(GLF_ENABLE_SHADER_DRAW_PARAMETERS)) {
        shaderDrawParametersEnabled = false;
    }

    // For debugging and unit testing
    if (TfGetEnvSetting(GLF_GLSL_VERSION) > 0) {
        // GLSL version override
        glslVersion = std::min(glslVersion, TfGetEnvSetting(GLF_GLSL_VERSION));

        // downgrade to the overridden GLSL version
        shadingLanguage420pack      &= (glslVersion >= 420);
        explicitUniformLocation     &= (glslVersion >= 430);
        bindlessTextureEnabled      &= (glslVersion >= 430);
        bindlessBufferEnabled       &= (glslVersion >= 430);
        shaderStorageBufferEnabled  &= (glslVersion >= 430);
        shaderDrawParametersEnabled &= (glslVersion >= 450);
    }

    // For driver issues workaround
    if (!TfGetEnvSetting(GLF_ENABLE_COPY_BUFFER)) {
        copyBufferEnabled = false;
    }

    if (TfDebug::IsEnabled(GLF_DEBUG_CONTEXT_CAPS)) {
        std::cout
            << "GlfContextCaps: \n"
            << "  GL version                         = "
            <<    glVersion << "\n"
            << "  GLSL version                       = "
            <<    glslVersion << "\n"

            << "  GL_MAX_UNIFORM_BLOCK_SIZE          = "
            <<    maxUniformBlockSize << "\n"
            << "  GL_MAX_SHADER_STORAGE_BLOCK_SIZE   = "
            <<    maxShaderStorageBlockSize << "\n"
            << "  GL_MAX_TEXTURE_BUFFER_SIZE         = "
            <<    maxTextureBufferSize << "\n"
            << "  GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT = "
            <<    uniformBufferOffsetAlignment << "\n"
            // order alphabetically
            << "  ARB_bindless_texture               = "
            <<    bindlessTextureEnabled << "\n"
            << "  ARB_direct_state_access            = "
            <<    directStateAccessEnabled << "\n"
            << "  ARB_explicit_uniform_location      = "
            <<    explicitUniformLocation << "\n"
            << "  ARB_multi_draw_indirect            = "
            <<    multiDrawIndirectEnabled << "\n"
            << "  ARB_shader_draw_parameters   = "
            <<    shaderDrawParametersEnabled << "\n"
            << "  ARB_shader_storage_buffer_object   = "
            <<    shaderStorageBufferEnabled << "\n"
            << "  ARB_shading_language_420pack       = "
            <<    shadingLanguage420pack << "\n"
            << "  NV_shader_buffer_load              = "
            <<    bindlessBufferEnabled << "\n"

            ;

        if (!copyBufferEnabled) {
            std::cout << "  CopyBuffer : disabled\n";
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

