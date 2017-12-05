//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hdSt/package.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE


static TfToken
_GetShaderPath(char const * shader)
{
    static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
    const std::string path =
        PlugFindPluginResource(plugin, TfStringCatPaths("shaders", shader));
    TF_VERIFY(!path.empty(), "Could not find shader: %s\n", shader);

    return TfToken(path);
}

TfToken
HdStPackageRenderPassShader()
{
    static TfToken renderPassShader =
        _GetShaderPath("renderPassShader.glslfx");
    return renderPassShader;
}

TfToken
HdStPackageFallbackLightingShader()
{
    static TfToken fallbackLightingShader =
        _GetShaderPath("fallbackLightingShader.glslfx");
    return fallbackLightingShader;
}

PXR_NAMESPACE_CLOSE_SCOPE
