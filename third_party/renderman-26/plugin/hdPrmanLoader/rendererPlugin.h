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
#include "hdPrmanLoader/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDPRMAN_LOADER_TOKENS           \
    (xpu)                               \
    ((xpuCpuConfig, "ri:xpuCpuConfig"))   \
    ((xpuGpuConfig, "ri:xpuGpuConfig"))
    /* end */

TF_DECLARE_PUBLIC_TOKENS(HdPrmanLoaderTokens, HDPRMANLOADER_API, HDPRMAN_LOADER_TOKENS);

class HdPrmanLoaderRendererPlugin : public HdRendererPlugin 
{
public:
    HDPRMANLOADER_API
    HdPrmanLoaderRendererPlugin();

    HDPRMANLOADER_API
    virtual ~HdPrmanLoaderRendererPlugin();

    HDPRMANLOADER_API
    HdRenderDelegate *CreateRenderDelegate() override;

    HDPRMANLOADER_API
    HdRenderDelegate *CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;
    
    HDPRMANLOADER_API
    void DeleteRenderDelegate(HdRenderDelegate *) override;

    HDPRMANLOADER_API
#if PXR_VERSION < 2305
    bool IsSupported() const override;
#else
    bool IsSupported(bool gpuEnabled = true) const override;
#endif

protected:

    HDPRMANLOADER_API
    virtual TfToken _GetRenderVariant() { return TfToken(); }

    HDPRMANLOADER_API
    virtual int _GetCpuConfig(HdRenderSettingsMap const& settingsMap);

    HDPRMANLOADER_API
    virtual std::vector<int> _GetGpuConfig(HdRenderSettingsMap const& settingsMap);

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
        HdRenderSettingsMap const& settingsMap, \
        TfToken const& rileyVariant, \
        int xpuCpuConfig, \
        std::vector<int> xpuGpuConfig)
#define HDPRMAN_LOADER_DELETE_DELEGATE \
    extern "C" ARCH_EXPORT void HdPrmanLoaderDeleteDelegate( \
        HdRenderDelegate *renderDelegate)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_PLUGIN_HD_PRMAN_LOADER_RENDERER_PLUGIN_H