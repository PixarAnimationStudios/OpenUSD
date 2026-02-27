//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/extComputationPrimvarPruningSceneIndexPlugin.h"

#if PXR_VERSION >= 2402

#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/pxr.h"
#if PXR_VERSION >= 2402
#include "pxr/imaging/hdsi/extComputationPrimvarPruningSceneIndex.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Register the plugins conditionally.
    for(const auto& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        // Register the plugins conditionally.
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr, // no argument data necessary
            HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin::
                GetInsertionPhase(),
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin::
HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_ExtComputationPrimvarPruningSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
#if PXR_VERSION >= 2402
    return HdSiExtComputationPrimvarPruningSceneIndex::New(inputScene);
#else
    return inputScene;
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2402
