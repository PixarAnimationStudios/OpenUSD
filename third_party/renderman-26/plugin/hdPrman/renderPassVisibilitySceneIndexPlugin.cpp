//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/renderPassVisibilitySceneIndexPlugin.h"

#if PXR_VERSION >= 2408

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "hdPrman/renderPassVisibilitySceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_RenderPassVisibilitySceneIndexPlugin"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry
        ::Define<HdPrman_RenderPassVisibilitySceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // We need an "insertion point" that's *after* general material resolve.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 115;

    for (auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr, // No input args.
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

HdPrman_RenderPassVisibilitySceneIndexPlugin::
HdPrman_RenderPassVisibilitySceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_RenderPassVisibilitySceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdPrman_RenderPassVisibilitySceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2408
