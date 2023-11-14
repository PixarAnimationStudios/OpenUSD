//
// Copyright 2021 Pixar
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
// language governing permissions and limitations under thApache License.
//
#include "pxr/imaging/hgiGL/capabilities.h"

#include "pxr/imaging/hgi/debugCodes.h"

#include "pxr/imaging/garch/glApi.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIGL_ENABLE_BINDLESS_BUFFER, false,
                      "Use GL bindless buffer extension");
TF_DEFINE_ENV_SETTING(HGIGL_ENABLE_MULTI_DRAW_INDIRECT, true,
                      "Use GL multi draw indirect extension");
TF_DEFINE_ENV_SETTING(HGIGL_ENABLE_BUILTIN_BARYCENTRICS, false,
                      "Use GL built in barycentric coordinates");
TF_DEFINE_ENV_SETTING(HGIGL_ENABLE_SHADER_DRAW_PARAMETERS, true,
                      "Use GL shader draw params if available (OpenGL 4.5+)");
TF_DEFINE_ENV_SETTING(HGIGL_ENABLE_BINDLESS_TEXTURE, false,
                      "Use GL bindless texture extension");
TF_DEFINE_ENV_SETTING(HGIGL_GLSL_VERSION, 0,
                      "GLSL version");

// Set defaults based on GL spec minimums
static const int _DefaultMaxUniformBlockSize          = 16*1024;
static const int _DefaultMaxShaderStorageBlockSize    = 16*1024*1024;
static const int _DefaultGLSLVersion                  = 400;
static const int _DefaultMaxClipDistances             = 8;

HgiGLCapabilities::HgiGLCapabilities()
    : _glVersion(0)
    , _glslVersion(_DefaultGLSLVersion)
{
    _LoadCapabilities();
}

void
HgiGLCapabilities::_LoadCapabilities()
{
    GarchGLApiLoad();

    // Reset values to reasonable defaults based of OpenGL minimums.
    // So that if we early out, systems can still depend on the
    // capabilities values being valid.
    _maxUniformBlockSize              = _DefaultMaxUniformBlockSize;
    _maxShaderStorageBlockSize        = _DefaultMaxShaderStorageBlockSize;
    _uniformBufferOffsetAlignment     = 0;
    _maxClipDistances                 = _DefaultMaxClipDistances;
    bool multiDrawIndirectEnabled     = false;
    bool bindlessTextureEnabled       = false;
    bool bindlessBufferEnabled        = false;
    bool builtinBarycentricsEnabled   = false;
    bool shaderDrawParametersEnabled  = false;
    bool conservativeRasterEnabled    = false;

    const char *glVendorStr = (const char*)glGetString(GL_VENDOR);
    const char *glRendererStr = (const char*)glGetString(GL_RENDERER);
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
        _glVersion = major * 100 + minor * 10;
    }

    if (_glVersion >= 200) {
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
            _glslVersion = major * 100 + minor * 10;
        }
    } else {
        _glslVersion = 0;
    }

    GLint maxClipDistances = 0;
    glGetIntegerv(GL_MAX_CLIP_PLANES, &maxClipDistances);
    _maxClipDistances = maxClipDistances;

    // initialize by Core versions
    if (_glVersion >= 310) {
        GLint maxUniformBlockSize = 0;
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE,
                      &maxUniformBlockSize);
        _maxUniformBlockSize = maxUniformBlockSize;

        GLint uniformBufferOffsetAlignment = 0;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
                      &uniformBufferOffsetAlignment);
        _uniformBufferOffsetAlignment = uniformBufferOffsetAlignment;
    }
    if (_glVersion >= 430) {
        GLint maxShaderStorageBlockSize = 0;
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,
                      &maxShaderStorageBlockSize);
        _maxShaderStorageBlockSize = maxShaderStorageBlockSize;
    }
    if (_glVersion >= 450) {
        multiDrawIndirectEnabled = true;
    }
    if (_glVersion >= 460) {
        shaderDrawParametersEnabled = true;
    }

    // initialize by individual extension.
    if (GARCH_GLAPI_HAS(ARB_bindless_texture)) {
        bindlessTextureEnabled = true;
    }
    if (GARCH_GLAPI_HAS(NV_shader_buffer_load)) {
        bindlessBufferEnabled = true;
    }
    if (GARCH_GLAPI_HAS(NV_fragment_shader_barycentric)) {
        builtinBarycentricsEnabled = true;
    }
    if (GARCH_GLAPI_HAS(ARB_multi_draw_indirect)) {
        multiDrawIndirectEnabled = true;
    }
    if (GARCH_GLAPI_HAS(ARB_shader_draw_parameters)) {
        shaderDrawParametersEnabled = true;
    }
    if (GARCH_GLAPI_HAS(NV_conservative_raster)) {
        conservativeRasterEnabled = true;
    }

    // Environment variable overrides (only downgrading is possible)
    if (!TfGetEnvSetting(HGIGL_ENABLE_BINDLESS_TEXTURE)) {
        bindlessTextureEnabled = false;
    }
    if (!TfGetEnvSetting(HGIGL_ENABLE_BINDLESS_BUFFER)) {
        bindlessBufferEnabled = false;
    }
    if (!TfGetEnvSetting(HGIGL_ENABLE_BUILTIN_BARYCENTRICS)) {
        builtinBarycentricsEnabled = false;
    }
    if (!TfGetEnvSetting(HGIGL_ENABLE_MULTI_DRAW_INDIRECT)) {
        multiDrawIndirectEnabled = false;
    }
    if (!TfGetEnvSetting(HGIGL_ENABLE_SHADER_DRAW_PARAMETERS)) {
        shaderDrawParametersEnabled = false;
    }

    // For debugging and unit testing
    if (TfGetEnvSetting(HGIGL_GLSL_VERSION) > 0) {
        // GLSL version override
        _glslVersion = std::min(_glslVersion, 
                                TfGetEnvSetting(HGIGL_GLSL_VERSION));
    }

    _SetFlag(HgiDeviceCapabilitiesBitsMultiDrawIndirect,
        multiDrawIndirectEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsBindlessTextures, 
        bindlessTextureEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsBindlessBuffers, 
        bindlessBufferEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsBuiltinBarycentrics,
        builtinBarycentricsEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsShaderDrawParameters, 
        shaderDrawParametersEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsShaderDoublePrecision, 
        true);
    _SetFlag(HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne,
        true);
    _SetFlag(HgiDeviceCapabilitiesBitsConservativeRaster, 
        conservativeRasterEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsStencilReadback,
        true);
    _SetFlag(HgiDeviceCapabilitiesBitsCustomDepthRange,
        true);

    if (TfDebug::IsEnabled(HGI_DEBUG_DEVICE_CAPABILITIES)) {
        std::cout
            << "HgiGLCapabilities: \n"
            << "  GL_VENDOR                          = "
            <<    glVendorStr << "\n"
            << "  GL_RENDERER                        = "
            <<    glRendererStr << "\n"
            << "  GL_VERSION                         = "
            <<    glVersionStr << "\n"
            << "  GL version                         = "
            <<    _glVersion << "\n"
            << "  GLSL version                       = "
            <<    _glslVersion << "\n"

            << "  GL_MAX_UNIFORM_BLOCK_SIZE          = "
            <<    _maxUniformBlockSize << "\n"
            << "  GL_MAX_SHADER_STORAGE_BLOCK_SIZE   = "
            <<    _maxShaderStorageBlockSize << "\n"
            << "  GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT = "
            <<    _uniformBufferOffsetAlignment << "\n"

            // order alphabetically
            << "  ARB_bindless_texture               = "
            <<    bindlessTextureEnabled << "\n"
            << "  ARB_multi_draw_indirect            = "
            <<    multiDrawIndirectEnabled << "\n"
            << "  ARB_shader_draw_parameters         = "
            <<    shaderDrawParametersEnabled << "\n"
            << "  NV_fragment_shader_barycentric     = "
            <<    builtinBarycentricsEnabled << "\n"
            << "  NV_shader_buffer_load              = "
            <<    bindlessBufferEnabled << "\n"
            << "  NV_conservative_raster             = "
            <<    conservativeRasterEnabled << "\n"
            ;
    }
}

HgiGLCapabilities::~HgiGLCapabilities() = default;

int
HgiGLCapabilities::GetAPIVersion() const {
    return _glVersion;
}

int
HgiGLCapabilities::GetShaderVersion() const {
    return _glslVersion;
}

PXR_NAMESPACE_CLOSE_SCOPE
