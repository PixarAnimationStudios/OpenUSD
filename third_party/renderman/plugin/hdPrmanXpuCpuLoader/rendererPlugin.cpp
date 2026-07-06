//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrmanXpuCpuLoader/rendererPlugin.h"

#include "pxr/base/arch/library.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdPrmanXpuCpuLoaderRendererPlugin>();
}

HdPrmanXpuCpuLoaderRendererPlugin::HdPrmanXpuCpuLoaderRendererPlugin()
    : HdPrmanLoaderRendererPlugin()
{
}

HdPrmanXpuCpuLoaderRendererPlugin::~HdPrmanXpuCpuLoaderRendererPlugin()
{
}

TfToken HdPrmanXpuCpuLoaderRendererPlugin::_GetRenderVariant()
{
    return HdPrmanLoaderTokens->xpu;
}

int HdPrmanXpuCpuLoaderRendererPlugin::_GetCpuConfig(
    HdRenderSettingsMap const& settingsMap)
{
    // Enable CPU
    return 1;
}

std::vector<int> HdPrmanXpuCpuLoaderRendererPlugin::_GetGpuConfig(
    HdRenderSettingsMap const& settingsMap)
{
    // Disable GPUs
    return std::vector<int>();
}

PXR_NAMESPACE_CLOSE_SCOPE
