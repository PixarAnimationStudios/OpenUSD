//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_PRMAN_XPU_LOADER_RENDERER_PLUGIN_H
#define PXR_IMAGING_PLUGIN_HD_PRMAN_XPU_LOADER_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"
#include "hdPrmanLoader/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanXpuLoaderRendererPlugin : public HdPrmanLoaderRendererPlugin {
public:
    HdPrmanXpuLoaderRendererPlugin();
    virtual ~HdPrmanXpuLoaderRendererPlugin();
protected:
    TfToken _GetRenderVariant() override;
};
    
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_PRMAN_XPU_LOADER_RENDERER_PLUGIN_H
