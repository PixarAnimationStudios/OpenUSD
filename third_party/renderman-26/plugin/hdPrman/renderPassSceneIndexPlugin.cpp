//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/renderPassSceneIndexPlugin.h"

#if PXR_VERSION >= 2408

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "hdPrman/renderPassSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_RenderPassSceneIndexPlugin"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry
        ::Define<HdPrman_RenderPassSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // We need an "insertion point" that's *after* general material resolve.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 115;

    for( auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr, // No input args.
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

HdPrman_RenderPassSceneIndexPlugin::
HdPrman_RenderPassSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_RenderPassSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdPrman_RenderPassSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2408
