//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_PLUGIN_HD_PRMAN_LOADER_RENDERER_PLUGIN_H
#define PXR_IMAGING_PLUGIN_HD_PRMAN_LOADER_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanLoaderRendererPlugin final : public HdRendererPlugin 
{
public:
    HdPrmanLoaderRendererPlugin();
    virtual ~HdPrmanLoaderRendererPlugin();

    HdRenderDelegate *CreateRenderDelegate() override;
    HdRenderDelegate *CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;
    void DeleteRenderDelegate(HdRenderDelegate *) override;
    bool IsSupported(bool gpuEnabled = true) const override;

private:
    // This class does not support copying.
    HdPrmanLoaderRendererPlugin(
        const HdPrmanLoaderRendererPlugin&) = delete;
    HdPrmanLoaderRendererPlugin &operator =(
        const HdPrmanLoaderRendererPlugin&) = delete;
};

// These macros are used to shim the actual hdPrman delegate implementation
#define HDPRMAN_LOADER_CREATE_DELEGATE \
    extern "C" ARCH_EXPORT HdRenderDelegate* HdPrmanLoaderCreateDelegate( \
        HdRenderSettingsMap const& settingsMap)
#define HDPRMAN_LOADER_DELETE_DELEGATE \
    extern "C" ARCH_EXPORT void HdPrmanLoaderDeleteDelegate( \
        HdRenderDelegate *renderDelegate)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_PRMAN_LOADER_RENDERER_PLUGIN_H
