//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDERER_PLUGIN_H
#define PXR_IMAGING_HD_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hf/pluginBase.h"
#include "pxr/imaging/hd/renderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class HdRenderIndex;
class HdPluginRenderDelegateUniqueHandle;

///
/// This class defines a renderer plugin interface for Hydra.
/// A renderer plugin is a dynamically discovered and loaded at run-time using
/// the Plug system.
///
/// This object has singleton behavior, in that is instantiated once per
/// library (managed by the plugin registry).
///
/// The class is used to factory objects that provide delegate support
/// to other parts of the Hydra Ecosystem.
///
class HdRendererPlugin : public HfPluginBase {
public:

    ///
    /// Create a render delegate through the plugin and wrap it in a
    /// handle that keeps this plugin alive until render delegate is
    /// destroyed. Initial settings can be passed in.
    ///
    HD_API
    HdPluginRenderDelegateUniqueHandle CreateDelegate(
        HdRenderSettingsMap const &settingsMap = {});

    ///
    /// Look-up plugin id in plugin registry.
    ///
    HD_API
    TfToken GetPluginId() const;

    ///
    /// Look-up display name in plugin registry.
    ///
    HD_API
    std::string GetDisplayName() const;

    ///
    /// Clients should use CreateDelegate since this method
    /// will eventually become protected, use CreateRenderDelegateHandle
    /// instead.
    ///
    /// Factory a Render Delegate object, that Hydra can use to
    /// factory prims and communicate with a renderer.
    ///
    virtual HdRenderDelegate *CreateRenderDelegate() = 0;

    ///
    /// Clients should use CreateDelegate since this method
    /// will eventually become protected.
    ///
    /// Factory a Render Delegate object, that Hydra can use to
    /// factory prims and communicate with a renderer.  Pass in initial
    /// settings...
    ///
    HD_API
    virtual HdRenderDelegate *CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap);

    ///
    /// Clients should use CreateDelegate since this method
    /// will eventually become protected.
    ///
    /// Release the object factoried by CreateRenderDelegate().
    ///
    virtual void DeleteRenderDelegate(HdRenderDelegate *renderDelegate) = 0;

    ///
    /// Returns \c true if this renderer plugin is supported in the running 
    /// process and \c false if not.
    /// 
    /// This gives the plugin a chance to perform some runtime checks to make
    /// sure that the system meets minimum requirements.  The \p gpuEnabled
    /// parameter indicates if the GPU is available for use by the plugin in
    /// case this information is necessary to make this determination.
    ///
    virtual bool IsSupported(bool gpuEnabled = true) const = 0;

protected:
    HdRendererPlugin() = default;
    HD_API
    ~HdRendererPlugin() override;

private:
    // This class doesn't require copy support.
    HdRendererPlugin(const HdRendererPlugin &)             = delete;
    HdRendererPlugin &operator =(const HdRendererPlugin &) = delete;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_RENDERER_PLUGIN_H
