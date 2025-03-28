//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/imaging/hd/rendererPluginHandle.h"
#include "pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h"

#include "pxr/base/tf/instantiateSingleton.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_INSTANTIATE_SINGLETON( HdRendererPluginRegistry );

HdRendererPluginRegistry &
HdRendererPluginRegistry::GetInstance()
{
    return TfSingleton< HdRendererPluginRegistry >::GetInstance();
}


HdRendererPluginRegistry::HdRendererPluginRegistry()
 : HfPluginRegistry(TfType::Find<HdRendererPlugin>())
{
}


HdRendererPluginRegistry::~HdRendererPluginRegistry() = default;

TfToken 
HdRendererPluginRegistry::GetDefaultPluginId(bool gpuEnabled)
{
    // Get all the available plugins to see if any of them is supported on this
    // platform and use the first one as the default.
    // 
    // Important note, we want to avoid loading plugins as much as possible, 
    // we would prefer to only load plugins when the user asks for them.  So
    // we will only load plugins until we find the first one that works.
    HfPluginDescVector pluginDescriptors;
    GetPluginDescs(&pluginDescriptors);
    for (const HfPluginDesc &desc : pluginDescriptors) {
        
        HdRendererPlugin *plugin = HdRendererPluginRegistry::GetInstance().
            GetRendererPlugin(desc.id);

        // Important to bail out as soon as we found a plugin that works to
        // avoid loading plugins unnecessary as that can be arbitrarily
        // expensive.
        if (plugin && plugin->IsSupported(gpuEnabled)) {
            HdRendererPluginRegistry::GetInstance().ReleasePlugin(plugin);

            TF_DEBUG(HD_RENDERER_PLUGIN).Msg(
                "Default renderer plugin (gpu: %s): %s\n",
                gpuEnabled ? "y" : "n", desc.id.GetText());
            return desc.id;
        }

        HdRendererPluginRegistry::GetInstance().ReleasePlugin(plugin);
    }

    TF_DEBUG(HD_RENDERER_PLUGIN).Msg(
        "Default renderer plugin (gpu: %s): none\n",
        gpuEnabled ? "y" : "n");
    return TfToken();
}

HdRendererPlugin *
HdRendererPluginRegistry::GetRendererPlugin(const TfToken &pluginId)
{
    return static_cast<HdRendererPlugin *>(GetPlugin(pluginId));
}

HdRendererPluginHandle
HdRendererPluginRegistry::GetOrCreateRendererPlugin(const TfToken &pluginId)
{
    return HdRendererPluginHandle(
            static_cast<HdRendererPlugin*>(GetPlugin(pluginId)));
}

HdPluginRenderDelegateUniqueHandle
HdRendererPluginRegistry::CreateRenderDelegate(
    const TfToken &pluginId,
    HdRenderSettingsMap const & settingsMap)
{
    HdRendererPluginHandle plugin = GetOrCreateRendererPlugin(pluginId);
    if (!plugin) {
        TF_CODING_ERROR("Couldn't find plugin for id %s", pluginId.GetText());
        return nullptr;
    }

    HdPluginRenderDelegateUniqueHandle result =
        plugin->CreateDelegate(settingsMap);

    return result;
}

void
HdRendererPluginRegistry::_CollectAdditionalMetadata(
    const PlugRegistry &plugRegistry, const TfType &pluginType)
{
    TF_DEBUG(HD_RENDERER_PLUGIN).Msg(
        "Renderer plugin discovery: %s\n",
        pluginType.GetTypeName().c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE

