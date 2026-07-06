//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_PRMAN_XPU_GPU_LOADER_RENDERER_PLUGIN_H
#define PXR_IMAGING_PLUGIN_HD_PRMAN_XPU_GPU_LOADER_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"
#include "hdPrmanLoader/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanXpuGpuLoaderRendererPlugin : public HdPrmanLoaderRendererPlugin {
public:
    HdPrmanXpuGpuLoaderRendererPlugin();
    virtual ~HdPrmanXpuGpuLoaderRendererPlugin();
protected:
    TfToken _GetRenderVariant() override;
    int _GetCpuConfig(HdRenderSettingsMap const& settingsMap) override;
    std::vector<int> _GetGpuConfig(HdRenderSettingsMap const& settingsMap) override;
};
    
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_PRMAN_XPU_GPU_LOADER_RENDERER_PLUGIN_H
