//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdSt/dependencyForwardingSceneIndexPlugin.h"

#include "pxr/imaging/hd/dependencyForwardingSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_DependencyForwardingSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_DependencyForwardingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 1000;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
}

HdSt_DependencyForwardingSceneIndexPlugin::
HdSt_DependencyForwardingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_DependencyForwardingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdDependencyForwardingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
