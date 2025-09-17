//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/materialPrimvarTransferSceneIndexPlugin.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hdsi/materialPrimvarTransferSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_MaterialPrimvarTransferSceneIndexPlugin"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

static const char* const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdSt_MaterialPrimvarTransferSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Should be chained *after* the extComputationPrimvarPruningSceneIndex and
    // procedural expansion.
    // Avoiding an additional dependency on hdGp in hdSt and hardcoding it
    // for now.
    const HdSceneIndexPluginRegistry::InsertionPhase
        insertionPhase = 3; // HdGpSceneIndexPlugin()::GetInsertionPhase() + 1;

    // Register the plugins conditionally.
    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr, // no argument data necessary
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdSt_MaterialPrimvarTransferSceneIndexPlugin::
HdSt_MaterialPrimvarTransferSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_MaterialPrimvarTransferSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
    return HdsiMaterialPrimvarTransferSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
