#include "rendererPlugin.h"
#include "renderDelegate.h"

#include <pxr/imaging/hd/rendererPluginRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) { HdRendererPluginRegistry::Define<HdParticleFieldRendererPlugin>(); }

HdRenderDelegate* HdParticleFieldRendererPlugin::CreateRenderDelegate() { return new HdParticleFieldRenderDelegate(); }

HdRenderDelegate* HdParticleFieldRendererPlugin::CreateRenderDelegate(HdRenderSettingsMap const& settingsMap) {
    return new HdParticleFieldRenderDelegate(settingsMap);
}

void HdParticleFieldRendererPlugin::DeleteRenderDelegate(HdRenderDelegate* renderDelegate) { delete renderDelegate; }

bool HdParticleFieldRendererPlugin::IsSupported(bool gpuEnabled) const { return true; }

PXR_NAMESPACE_CLOSE_SCOPE
