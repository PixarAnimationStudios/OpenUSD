//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/plugin/hdStorm/rendererPlugin.h"

#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hd/renderDelegateInfo.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexInputArgsSchema.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdStormRendererPlugin>();
}

HdRenderDelegate *
HdStormRendererPlugin::CreateRenderDelegate()
{
    return new HdStRenderDelegate();
}

HdRenderDelegate*
HdStormRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    return new HdStRenderDelegate(settingsMap);
}

void
HdStormRendererPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
    delete renderDelegate;
}

bool
HdStormRendererPlugin::IsSupported(
    HdRendererCreateArgs const &rendererCreateArgs,
    std::string * reasonWhyNot) const
{
    const bool gpuEnabled = rendererCreateArgs.gpuEnabled;

    const bool support = gpuEnabled &&
        HdStRenderDelegate::IsSupported(rendererCreateArgs);
    if (!support) {
        TF_DEBUG(HD_RENDERER_PLUGIN).Msg(
            "hdStorm renderer plugin unsupported: %s\n",
            gpuEnabled ? "hgi unsupported" : "no gpu");
        if (reasonWhyNot) {
            *reasonWhyNot = gpuEnabled ? "Hgi unsupported" : "No GPU";
        }
    }
    return support;
}

HdContainerDataSourceHandle
HdStormRendererPlugin::GetSceneIndexInputArgs() const
{
    static HdContainerDataSourceHandle const result =
        HdSceneIndexInputArgsSchema::Builder()
            .SetMotionBlurSupport(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .SetCameraMotionBlurSupport(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .SetLegacyRenderDelegateInfo(
                HdRetainedTypedSampledDataSource<HdRenderDelegateInfo>::New(
                    HdStRenderDelegate::GetRenderDelegateInfo()))      
            .Build();

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
