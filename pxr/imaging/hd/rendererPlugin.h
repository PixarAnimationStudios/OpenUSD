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

class HdRenderer;
class HdPluginRenderDelegateUniqueHandle;
class HdPluginRendererUniqueHandle;
struct HdRendererCreateArgs;
TF_DECLARE_REF_PTRS(HdSceneIndexBase);

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
class HdRendererPlugin : public HfPluginBase
{
public:

    /// \name Hydra 2.0 API
    /// @{

    ///
    /// Returns \c true if this renderer plugin is supported in the running 
    /// process and \c false if not.
    /// 
    /// This gives the plugin a chance to perform some runtime checks to make
    /// sure that the system meets minimum requirements.  The 
    /// \p rendererCreateArgs parameter should conform to
    /// HdRendererCreateArgsSchema to indicate the resources available when 
    /// making this determination.
    /// 
    /// The \p reasonWhyNot param, when provided, can be filled with the reason
    /// why the renderer plugin is not supported.
    HD_API
    virtual bool IsSupported(
        HdContainerDataSourceHandle const &rendererCreateArgs,
        std::string *reasonWhyNot = nullptr) const;

    ///
    /// Create renderer through the plugin and wrap it in a handle that
    /// keeps this plugin alive until the renderer is destroyed.
    ///
    /// The renderer is populated from the given scene index.
    /// rendererCreateArgs should conform to HdRendererCreateArgsSchema.
    ///
    /// Note that for a seamless transition, this  Hydra 2.0 method
    /// falls back to creating a Hydra 1.0 render delegate and the 
    /// necessary "back-end" emulation for render
    /// plugins that do not implement the Hydra 2.0
    /// _CreateRenderer.
    ///
    HD_API
    HdPluginRendererUniqueHandle CreateRenderer(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        HdContainerDataSourceHandle const &rendererCreateArgs);

    /// @}

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

    /// \name Hydra 1.0 API
    /// @{

    ///
    /// Returns \c true if this renderer plugin is supported in the running 
    /// process and \c false if not.
    /// 
    /// This gives the plugin a chance to perform some runtime checks to make
    /// sure that the system meets minimum requirements.  The 
    /// \p rendererCreateArgs parameter indicates the resources available when 
    /// making this determination.
    /// 
    /// The \p reasonWhyNot param, when provided, can be filled with the reason
    /// why the renderer plugin is not supported.
    virtual bool IsSupported(
        HdRendererCreateArgs const &rendererCreateArgs,
        std::string *reasonWhyNot = nullptr) const = 0;

    ///
    /// Create a render delegate through the plugin and wrap it in a
    /// handle that keeps this plugin alive until render delegate is
    /// destroyed. Initial settings can be passed in.
    ///
    HD_API
    HdPluginRenderDelegateUniqueHandle CreateDelegate(
        HdRenderSettingsMap const &settingsMap = {});
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

    /// @}

    ///
    /// \deprecated Use IsSupported overload below.
    ///
    /// Returns \c true if this renderer plugin is supported in the running 
    /// process and \c false if not.
    /// 
    /// This gives the plugin a chance to perform some runtime checks to make
    /// sure that the system meets minimum requirements.  The \p gpuEnabled
    /// parameter indicates if the GPU is available for use by the plugin in
    /// case this information is necessary to make this determination.
    ///
    HD_API
    virtual bool IsSupported(bool gpuEnabled = true) const;

protected:
    HdRendererPlugin() = default;
    HD_API
    ~HdRendererPlugin() override;

    HD_API
    virtual std::unique_ptr<HdRenderer> _CreateRenderer(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        HdContainerDataSourceHandle const &rendererCreateArgs);

    // Instantiates render delegate and uses "back-end" emulation.
    HD_API
    std::unique_ptr<HdRenderer> _CreateRendererFromRenderDelegate(
        HdSceneIndexBaseRefPtr const &sceneIndex,
        HdContainerDataSourceHandle const &rendererCreateArgs);

private:
    // This class doesn't require copy support.
    HdRendererPlugin(const HdRendererPlugin &)             = delete;
    HdRendererPlugin &operator =(const HdRendererPlugin &) = delete;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_RENDERER_PLUGIN_H
