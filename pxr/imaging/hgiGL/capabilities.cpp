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
                      "Use built in barycentric coordinates");
TF_DEFINE_ENV_SETTING(HGIGL_ENABLE_SHADER_DRAW_PARAMETERS, true,
                      "Use GL shader draw params if available (OpenGL 4.5+)");

HgiGLCapabilities::HgiGLCapabilities()
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
    int glVersion                     = 0;
    bool multiDrawIndirectEnabled     = false;
    bool bindlessBufferEnabled        = false;
    bool builtinBarycentricsEnabled   = false;
    bool shaderDrawParametersEnabled  = false;

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
        glVersion = major * 100 + minor * 10;
    }

    if (glVersion >= 450) {
        multiDrawIndirectEnabled = true;
    }
    if (glVersion >= 460) {
        shaderDrawParametersEnabled = true;
    }

    // initialize by individual extension.
    if (GARCH_GLAPI_HAS(NV_shader_buffer_load)) {
        bindlessBufferEnabled = true;
    }
    if (GARCH_GLAPI_HAS(NV_fragment_shader_barycentric)) {
        builtinBarycentricsEnabled = true;
    }
    if (GARCH_GLAPI_HAS(ARB_multi_draw_indirect)) {
        multiDrawIndirectEnabled = true;
    }
#if defined(GL_VERSION_4_5)
    if (GARCH_GLAPI_HAS(ARB_shader_draw_parameters)) {
        shaderDrawParametersEnabled = true;
    }
#endif

    // Environment variable overrides (only downgrading is possible)
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

    _SetFlag(HgiDeviceCapabilitiesBitsMultiDrawIndirect,
        multiDrawIndirectEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsBindlessBuffers, 
        bindlessBufferEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsBuiltinBarycentrics,
        builtinBarycentricsEnabled);
    _SetFlag(HgiDeviceCapabilitiesBitsShaderDrawParameters, 
        shaderDrawParametersEnabled);

    if (TfDebug::IsEnabled(HGI_DEBUG_DEVICE_CAPABILITIES)) {
        std::cout
            << "HgiGLCapabilities: \n"
            << "  GL_VENDOR                          = "
            <<    glVendorStr << "\n"
            << "  GL_RENDERER                        = "
            <<    glRendererStr << "\n"
            << "  GL_VERSION                         = "
            <<    glVersionStr << "\n"

            // order alphabetically
            << "  ARB_multi_draw_indirect            = "
            <<    multiDrawIndirectEnabled << "\n"
            << "  ARB_shader_draw_parameters         = "
            <<    shaderDrawParametersEnabled << "\n"
            << "  NV_fragment_shader_barycentric     = "
            <<    builtinBarycentricsEnabled << "\n"
            << "  NV_shader_buffer_load              = "
            <<    bindlessBufferEnabled << "\n"
            ;
    }
}

HgiGLCapabilities::~HgiGLCapabilities() = default;

PXR_NAMESPACE_CLOSE_SCOPE
