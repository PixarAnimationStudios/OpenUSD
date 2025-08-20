//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/generativeProceduralSceneIndexPlugin.h"
#include "hdPrman/tokens.h"
#include "pxr/imaging/hdGp/generativeProceduralResolvingSceneIndex.h"
#include "pxr/imaging/hdGp/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

// Used to check if HdGp default resolver was requested.
HDGP_API
extern TfEnvSetting<bool> HDGP_INCLUDE_DEFAULT_RESOLVER;

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry
        ::Define<HdPrman_GenerativeProceduralSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    if (TfGetEnvSetting(HDGP_INCLUDE_DEFAULT_RESOLVER)) {
        // If the HdGp default procedural resolver was requested,
        // do not add another, since it would be redundant.
        return;
    }

    for (const auto& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            TfToken("HdPrman_GenerativeProceduralSceneIndexPlugin"),
            nullptr,   // no argument data necessary
            HdPrman_GenerativeProceduralSceneIndexPlugin::GetInsertionPhase(),
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

HdPrman_GenerativeProceduralSceneIndexPlugin::HdPrman_GenerativeProceduralSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_GenerativeProceduralSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdGpGenerativeProceduralResolvingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE



