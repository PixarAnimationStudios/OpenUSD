//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_RENDERER_PLUGIN_H
#define PXR_IMAGING_PLUGIN_LOFI_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

extern uint32_t LOFI_GL_VERSION;

///
/// \class LoFiRendererPlugin
///
/// A registered child of HdRendererPlugin, this is the class that gets
/// loaded when a Hydra application asks to draw with a certain renderer.
/// It supports rendering via creation/destruction of renderer-specific
/// classes. The render delegate is the Hydra-facing entrypoint into the
/// renderer; it's responsible for creating specialized implementations of Hydra
/// prims (which translate scene data into drawable representations) and Hydra
/// renderpasses (which draw the scene to the framebuffer).
///
class LoFiRendererPlugin final : public HdRendererPlugin 
{
public:
    LoFiRendererPlugin() = default;
    virtual ~LoFiRendererPlugin() = default;

    /// Construct a new render delegate of type LoFiRenderDelegate.
    virtual HdRenderDelegate *CreateRenderDelegate() override;

    /// Construct a new render delegate of type LoFiRenderDelegate.
    virtual HdRenderDelegate *CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;

    /// Destroy a render delegate created by this class's CreateRenderDelegate.
    ///   \param renderDelegate The render delegate to delete.
    virtual void DeleteRenderDelegate(
        HdRenderDelegate *renderDelegate) override;

    /// Checks to see if the plugin is supported on the running system.
    virtual bool IsSupported() const override;

private:
    // This class does not support copying.
    LoFiRendererPlugin(const LoFiRendererPlugin&) = delete;
    LoFiRendererPlugin &operator =(const LoFiRendererPlugin&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_LOFI_RENDERER_PLUGIN_H
