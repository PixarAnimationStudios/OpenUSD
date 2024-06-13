//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/updateObjectSettingsSceneIndexPlugin.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "hdPrman/updateObjectSettingsSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_UpdateObjectSettingsSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "Prman";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry
        ::Define<HdPrman_UpdateObjectSettingsSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // We need an "insertion point" that's *after* general material resolve.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 115;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr, // No input args.
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

HdPrman_UpdateObjectSettingsSceneIndexPlugin::
HdPrman_UpdateObjectSettingsSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_UpdateObjectSettingsSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdPrman_UpdateObjectSettingsSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
