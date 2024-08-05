//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/rileyGlobalsSceneIndexPlugin.h"

#include "hdPrman/rileyGlobalsSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_RileyGlobalsSceneIndexPlugin"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

static const char * const _rendererDisplayName = "Prman";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_RileyGlobalsSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 1;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _rendererDisplayName,
        _tokens->sceneIndexPluginName,
        /* inputArgs = */ nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_RileyGlobalsSceneIndexPlugin::
HdPrman_RileyGlobalsSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_RileyGlobalsSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
    return HdPrman_RileyGlobalsSceneIndex::New(inputScene, inputArgs);
#else
    return inputScene;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
