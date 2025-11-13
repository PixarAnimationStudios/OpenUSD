//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrmanXpuLoader/rendererPlugin.h"

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
    HdRendererPluginRegistry::Define<HdPrmanXpuLoaderRendererPlugin>();
}

HdPrmanXpuLoaderRendererPlugin::HdPrmanXpuLoaderRendererPlugin()
    : HdPrmanLoaderRendererPlugin()
{
}

HdPrmanXpuLoaderRendererPlugin::~HdPrmanXpuLoaderRendererPlugin()
{
}

TfToken HdPrmanXpuLoaderRendererPlugin::_GetRenderVariant()
{
    return HdPrmanLoaderTokens->xpu;
}

PXR_NAMESPACE_CLOSE_SCOPE