//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    static TfToken s = _GetShaderPath("compute.glslfx");
    return s;
}

TfToken
HdStPackageDomeLightShader()
{
    static TfToken s = _GetShaderPath("domeLight.glslfx");
    return s;
}

TfToken
HdStPackageFallbackDomeLightTexture()
{
    static TfToken t = _GetTexturePath("fallbackBlackDomeLight.png");
    return t;
}

TfToken
HdStPackagePtexTextureShader()
{
    static TfToken s = _GetShaderPath("ptexTexture.glslfx");
    return s;
}

TfToken
HdStPackageRenderPassShader()
{
    static TfToken s = _GetShaderPath("renderPassShader.glslfx");
    return s;
}

TfToken
HdStPackageFallbackLightingShader()
{
    static TfToken s = _GetShaderPath("fallbackLightingShader.glslfx");
    return s;
}

TfToken
HdStPackageFallbackMaterialNetworkShader()
{
    static TfToken s = _GetShaderPath("fallbackMaterialNetwork.glslfx");
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
    static TfToken s = _GetShaderPath("fallbackVolume.glslfx");
    return s;
}

TfToken
HdStPackageImageShader()
{
    static TfToken s = _GetShaderPath("imageShader.glslfx");
    return s;
}

TfToken
HdStPackageSimpleLightingShader()
{
    static TfToken s = _GetShaderPath("simpleLightingShader.glslfx");
    return s;
}

TfToken
HdStPackageWidgetShader()
{
    static TfToken s = _GetShaderPath("widgetShader.glslfx");
    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
