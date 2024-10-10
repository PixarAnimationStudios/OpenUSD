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
    // Needs to be inserted earlier to allow plugins that follow to transform
    // primvar data without having to concern themselves about computed
    // primvars.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    // Register the plugins conditionally.
    for(const auto& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
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
