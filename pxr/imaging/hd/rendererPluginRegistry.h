//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDERER_PLUGIN_REGISTRY_H
#define PXR_IMAGING_HD_RENDERER_PLUGIN_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/imaging/hf/pluginRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdRendererPlugin;
class HdRendererPluginHandle;
class HdPluginRenderDelegateUniqueHandle;

class HdRendererPluginRegistry final  : public HfPluginRegistry
{
public:
    ///
    /// Returns the singleton registry for \c HdRendererPlugin
    ///
    HD_API
    static HdRendererPluginRegistry &GetInstance();

    ///
    /// Entry point for defining an HdRendererPlugin plugin.
    ///
    template<typename T, typename... Bases>
    static void Define();

    ///
    /// Returns the id of plugin to use as the default.  To ensure an
    /// appropriate default is found, the \p gpuEnabled parameter will be used
    /// to indicate if the GPU will be available when making the determination.
    ///
    HD_API
    TfToken GetDefaultPluginId(bool gpuEnabled = true);

    ///
    /// \deprecated Use GetOrCreateRendererPlugin instead.
    ///
    /// Returns the renderer plugin for the given id or null
    /// if not found.  The reference count on the returned
    /// delegate is incremented.
    ///
    HD_API
    HdRendererPlugin *GetRendererPlugin(const TfToken &pluginId);

    ///
    /// Returns the renderer plugin for the given id or a null handle
    /// if not found. The plugin is wrapped in a handle that automatically
    /// increments and decrements the reference count and also stores the
    /// plugin id.
    ///
    HD_API
    HdRendererPluginHandle GetOrCreateRendererPlugin(const TfToken &pluginId);

    ///
    /// Returns a render delegate created by the plugin with the given name
    /// if the plugin is supported using given initial settings.
    /// The render delegate is wrapped in a movable handle that
    /// keeps the plugin alive until the render delegate is destroyed by
    /// dropping the handle.
    /// 
    HD_API
    HdPluginRenderDelegateUniqueHandle CreateRenderDelegate(
        const TfToken &pluginId,
        const HdRenderSettingsMap &settingsMap = {});

private:
    // Friend required by TfSingleton to access constructor (as it is private).
    friend class TfSingleton<HdRendererPluginRegistry>;

    void _CollectAdditionalMetadata(
        const PlugRegistry &plugRegistry, const TfType &pluginType) override;

    // Singleton gets private constructed
    HdRendererPluginRegistry();
    ~HdRendererPluginRegistry() override;

    //
    /// This class is not intended to be copied.
    ///
    HdRendererPluginRegistry(const HdRendererPluginRegistry &)            = delete;
    HdRendererPluginRegistry &operator=(const HdRendererPluginRegistry &) = delete;
};


template<typename T, typename... Bases>
void HdRendererPluginRegistry::Define()
{
    HfPluginRegistry::Define<T, HdRendererPlugin, Bases...>();
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_RENDERER_PLUGIN_REGISTRY_H
