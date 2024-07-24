//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/pinnedCurveExpandingSceneIndexPlugin.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hdsi/pinnedCurveExpandingSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_PinnedCurveExpandingSceneIndexPlugin"))
);

////////////////////////////////////////////////////////////////////////////////
// Plugin registrations
////////////////////////////////////////////////////////////////////////////////

static const char * const _rendererDisplayName = "Prman";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_PinnedCurveExpandingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Should be chained *after*:
    // - extComputationPrimvarPruningSceneIndex (to allow expansion of computed
    //   primvars on pinned curves) and
    // - procedural plugin (HdGpSceneIndexPlugin) to allow expansion of
    //   computed primvars on procedurally generated pinned curves.
    //
    const HdSceneIndexPluginRegistry::InsertionPhase
        insertionPhase = 3; // HdGpSceneIndexPlugin()::GetInsertionPhase() + 1;

    // Register the plugins conditionally.
    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _rendererDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr, // no argument data necessary
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

////////////////////////////////////////////////////////////////////////////////
// Scene Index Implementations
////////////////////////////////////////////////////////////////////////////////

HdPrman_PinnedCurveExpandingSceneIndexPlugin::
HdPrman_PinnedCurveExpandingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_PinnedCurveExpandingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    TF_UNUSED(inputArgs);
    return HdsiPinnedCurveExpandingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
