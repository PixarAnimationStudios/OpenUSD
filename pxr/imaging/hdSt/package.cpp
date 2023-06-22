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
#include "pxr/base/tf/getEnv.h"

PXR_NAMESPACE_OPEN_SCOPE

static const int dxHgiEnabled = TfGetenvInt("HGI_ENABLE_DX", 0);

static TfToken
_GetShaderPath(char const * shader)
{
    static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
    const std::string path =
        PlugFindPluginResource(plugin, TfStringCatPaths("shaders", shader));
    TF_VERIFY(!path.empty(), "Could not find shader: %s\n", shader);

    return TfToken(path);
}

static TfToken
_GetTexturePath(char const * texture)
{
    static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
    const std::string path =
        PlugFindPluginResource(plugin, TfStringCatPaths("textures", texture));
    TF_VERIFY(!path.empty(), "Could not find texture: %s\n", texture);

    return TfToken(path);
}

TfToken
HdStPackageComputeShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ?
      _GetShaderPath("compute.hlslfx"):
      _GetShaderPath("compute.glslfx");
    return s;
}

TfToken
HdStPackageDomeLightShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ?
       _GetShaderPath("domeLight.hlslfx") :
       _GetShaderPath("domeLight.glslfx");
    return s;
}

TfToken
HdStPackageFallbackDomeLightTexture()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ?
       _GetShaderPath("fallbackBlackDomeLight.hlslfx") :
       _GetShaderPath("fallbackBlackDomeLight.glslfx");
    return s;
}

TfToken
HdStPackagePtexTextureShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ?
       _GetShaderPath("ptexTexture.hlslfx") :
       _GetShaderPath("ptexTexture.glslfx");
    return s;
}

TfToken
HdStPackageRenderPassShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ? 
      _GetShaderPath("renderPassShader.hlslfx") : 
      _GetShaderPath("renderPassShader.glslfx");
    return s;
}

TfToken
HdStPackageFallbackLightingShader()
{
    static TfToken s = (1 == dxHgiEnabled) ?
       _GetShaderPath("fallbackLightingShader.hlslfx") :
       _GetShaderPath("fallbackLightingShader.glslfx");
    return s;
}

TfToken
HdStPackageFallbackMaterialNetworkShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ?
       _GetShaderPath("fallbackMaterialNetwork.hlslfx") :
       _GetShaderPath("fallbackMaterialNetwork.glslfx");
    return s;
}

TfToken
HdStPackageInvalidMaterialNetworkShader()
{
    static TfToken s = _GetShaderPath("invalidMaterialNetwork.glslfx");
    return s;
}

TfToken
HdStPackageFallbackVolumeShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ?
       _GetShaderPath("fallbackVolume.hlslfx") :
       _GetShaderPath("fallbackVolume.glslfx");
    return s;
}

TfToken
HdStPackageImageShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ?
       _GetShaderPath("imageShader.hlslfx") :
       _GetShaderPath("imageShader.glslfx");
    return s;
}

TfToken
HdStPackageSimpleLightingShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ? 
      _GetShaderPath("simpleLightingShader.hlslfx") : 
      _GetShaderPath("simpleLightingShader.glslfx");
    return s;
}

TfToken
HdStPackageWidgetShader()
{
    //
    // TODO: this is a temporary solution to switch between (slightly different) libraries 
    // when Gl or DX Hgis are used. Will have to review this and find a cleaner solution.
    static TfToken s = (1 == dxHgiEnabled) ?
       _GetShaderPath("widgetShader.hlslfx") :
       _GetShaderPath("widgetShader.glslfx");
    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
