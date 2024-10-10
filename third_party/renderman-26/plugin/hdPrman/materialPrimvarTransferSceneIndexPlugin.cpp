//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/materialPrimvarTransferSceneIndexPlugin.h"

#if PXR_VERSION >= 2302

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hdsi/materialPrimvarTransferSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_MaterialPrimvarTransferSceneIndexPlugin"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_MaterialPrimvarTransferSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Should be chained *after* the extComputationPrimvarPruningSceneIndex and
    // procedural expansion.
    // Avoiding an additional dependency on hdGp in hdPrman and hardcoding it
    // for now.
    const HdSceneIndexPluginRegistry::InsertionPhase
        insertionPhase = 3; // HdGpSceneIndexPlugin()::GetInsertionPhase() + 1;

    for( auto const& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        // Register the plugins conditionally.
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr, // no argument data necessary
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_MaterialPrimvarTransferSceneIndexPlugin::
HdPrman_MaterialPrimvarTransferSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_MaterialPrimvarTransferSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
    return HdsiMaterialPrimvarTransferSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_VERSION >= 2302
