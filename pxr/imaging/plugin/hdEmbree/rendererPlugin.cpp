//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/rendererPlugin.h"

#include "pxr/imaging/hd/renderDelegateInfo.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexCreateArgsSchema.h"

#include "pxr/imaging/plugin/hdEmbree/renderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the embree plugin with the renderer plugin system.
TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdEmbreeRendererPlugin>();
}

static
HdRenderDelegateInfo
_RenderDelegateInfo()
{
    HdRenderDelegateInfo result;

    // Re-implemented from HdEmbreeRenderDelegate::GetMaterialBindingPurpose()
    result.materialBindingPurpose = HdTokens->full;
    // Default from HdRenderDelegate::IsPrimvarFilteringNeeded().
    result.isPrimvarFilteringNeeded = false;
    // No coordSys among HdEmbreeRenderDelegate::GetSupportedSprimTypes().
    result.isCoordSysSupported = false;

    return result;
}

HdContainerDataSourceHandle
HdEmbreeRendererPlugin::GetSceneIndexCreateArgs() const
{
    static HdContainerDataSourceHandle const result =
        HdSceneIndexCreateArgsSchema::Builder()
            .SetMotionBlurSupport(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .SetCameraMotionBlurSupport(
                HdRetainedTypedSampledDataSource<bool>::New(false))
            .SetLegacyRenderDelegateInfo(
                HdRetainedTypedSampledDataSource<HdRenderDelegateInfo>::New(
                    _RenderDelegateInfo()))
            .Build();

    return result;
}

HdRenderDelegate*
HdEmbreeRendererPlugin::CreateRenderDelegate()
{
    return new HdEmbreeRenderDelegate();
}

HdRenderDelegate*
HdEmbreeRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    return new HdEmbreeRenderDelegate(settingsMap);
}

void
HdEmbreeRendererPlugin::DeleteRenderDelegate(HdRenderDelegate *renderDelegate)
{
    delete renderDelegate;
}

bool 
HdEmbreeRendererPlugin::IsSupported(
    const HdRendererCreateArgsSchema & /* rendererCreateArgs */,
    std::string * /* reasonWhyNot */) const
{
    // Nothing more to check for now, we assume if the plugin loads correctly
    // it is supported.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
