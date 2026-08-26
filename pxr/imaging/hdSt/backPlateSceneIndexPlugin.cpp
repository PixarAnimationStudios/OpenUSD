// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "backPlateSceneIndexPlugin.h"

#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/imaging/hdsi/backPlateSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_BackPlateSceneIndexPlugin"))
);

static const char* const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_BackPlateSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName, _tokens->sceneIndexPluginName, nullptr,
        insertionPhase, HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

HdSt_BackPlateSceneIndexPlugin::HdSt_BackPlateSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_BackPlateSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdContainerDataSourceHandle& inputArgs)
{
    TF_UNUSED(inputArgs);

    return HdsiBackPlateSceneIndex::New(inputSceneIndex);
}

PXR_NAMESPACE_CLOSE_SCOPE

