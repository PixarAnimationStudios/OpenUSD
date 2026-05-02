//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "rendererPlugin.h"
#include "renderDelegate.h"

#include "pxr/imaging/hd/rendererPluginRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdParticleFieldRendererPlugin>();
}

HdRenderDelegate* HdParticleFieldRendererPlugin::CreateRenderDelegate() {
    return new HdParticleFieldRenderDelegate();
}

HdRenderDelegate* HdParticleFieldRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    return new HdParticleFieldRenderDelegate(settingsMap);
}

void HdParticleFieldRendererPlugin::DeleteRenderDelegate(
    HdRenderDelegate* renderDelegate)
{
    delete renderDelegate;
}

#if HD_API_VERSION >= 83
bool HdParticleFieldRendererPlugin::IsSupported(
    HdRendererCreateArgs const &rendererCreateArgs,
    std::string *reasonWhyNot) const {
    return true;
}
#else
bool HdParticleFieldRendererPlugin::IsSupported(bool gpuEnabled) const
{
    return true;
}
#endif

#if HD_API_VERSION >= 98
bool HdParticleFieldRendererPlugin::IsSupported(
    const HdRendererCreateArgsSchema &rendererCreateArgs,
    std::string *reasonWhyNot) const {
    return true;
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
