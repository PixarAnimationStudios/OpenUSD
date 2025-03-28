//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_STORM_RENDERER_PLUGIN_H
#define PXR_IMAGING_PLUGIN_HD_STORM_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStormRendererPlugin final : public HdRendererPlugin {
public:
    HdStormRendererPlugin()          = default;
    virtual ~HdStormRendererPlugin() = default;

    virtual HdRenderDelegate *CreateRenderDelegate() override;
    virtual HdRenderDelegate *CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;

    virtual void DeleteRenderDelegate(HdRenderDelegate *renderDelegate) 
        override;

    virtual bool IsSupported(bool gpuEnabled = true) const override;

private:
    HdStormRendererPlugin(const HdStormRendererPlugin &)             = delete;
    HdStormRendererPlugin &operator =(const HdStormRendererPlugin &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_STORM_RENDERER_PLUGIN_H
