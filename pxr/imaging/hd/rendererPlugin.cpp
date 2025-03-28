//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/rendererPlugin.h"

#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdRendererPlugin>();
}

HdRenderDelegate*
HdRendererPlugin::CreateRenderDelegate(HdRenderSettingsMap const& settingsMap)
{
    // The settings map is currently an opt-in API, so if there's no
    // derived implementation, fall back to the settings-less factory.
    return CreateRenderDelegate();
}

//
// As this class is a pure interface class, it does not need a
// vtable.  However, it is possible that some users will use rtti.
// This will cause a problem for some of our compilers:
//
// In particular clang will throw a warning: -wweak-vtables
// For gcc, there is an issue were the rtti typeid's are different.
//
// As destruction of the class is not on the performance path,
// the body of the deleter is provided here, so a vtable is created
// in this compilation unit.
HdRendererPlugin::~HdRendererPlugin() = default;

HdPluginRenderDelegateUniqueHandle
HdRendererPlugin::CreateDelegate(HdRenderSettingsMap const& settingsMap)
{
    if (!IsSupported()) {
        return nullptr;
    }

    HdRendererPluginRegistry::GetInstance().AddPluginReference(this);

    HdPluginRenderDelegateUniqueHandle result =
        HdPluginRenderDelegateUniqueHandle(
            HdRendererPluginHandle(this),
            CreateRenderDelegate(settingsMap));

    if (TfDebug::IsEnabled(HD_RENDERER_PLUGIN)) {
        std::stringstream ss;
        for (const auto& pair : settingsMap) {
            ss << "\t" << pair.first << ": " << pair.second << "\n";
        }
        TF_DEBUG(HD_RENDERER_PLUGIN).Msg(
            "%s instance of renderer plugin '%s' with settings map:\n%s",
            result ? "Created" : "Failed to create",
            GetPluginId().GetText(), ss.str().c_str());
    }

    // provide render delegate instance with display name to facilitate
    // association of this renderer to other code and resources
    if (result) {
        result->_SetRendererDisplayName(GetDisplayName());
    }

    return result;
}

TfToken
HdRendererPlugin::GetPluginId() const
{
    return HdRendererPluginRegistry::GetInstance().GetPluginId(this);
}

std::string
HdRendererPlugin::GetDisplayName() const
{
    TfToken pluginId =
        HdRendererPluginRegistry::GetInstance().GetPluginId(this);
    HfPluginDesc desc;
    if (!HdRendererPluginRegistry::GetInstance()
            .GetPluginDesc(pluginId, &desc)) {
        // Note, this is unlikely since if pluginId were illegal, this class
        // would not have been instantiated...
        TF_CODING_ERROR("Unable to get display name for '%s'",
                pluginId.GetText());
        return std::string();
    }

    return desc.displayName;
}

PXR_NAMESPACE_CLOSE_SCOPE

