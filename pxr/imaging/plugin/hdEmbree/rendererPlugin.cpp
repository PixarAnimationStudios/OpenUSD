//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/plugin/hdEmbree/rendererPlugin.h"

#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/plugin/hdEmbree/renderDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the embree plugin with the renderer plugin system.
TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdEmbreeRendererPlugin>();
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
HdEmbreeRendererPlugin::IsSupported(bool /* gpuEnabled */) const
{
    // Nothing more to check for now, we assume if the plugin loads correctly
    // it is supported.
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
