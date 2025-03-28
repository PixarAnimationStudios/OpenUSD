//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdSt/nurbsApproximatingSceneIndexPlugin.h"

#include "pxr/imaging/hdsi/nurbsApproximatingSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hio/glslfx.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_NurbsApproximatingSceneIndexPlugin")));

static const char* const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdSt_NurbsApproximatingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName, _tokens->sceneIndexPluginName, nullptr,
        insertionPhase, HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

HdSt_NurbsApproximatingSceneIndexPlugin::
HdSt_NurbsApproximatingSceneIndexPlugin() = default;

HdSt_NurbsApproximatingSceneIndexPlugin::
~HdSt_NurbsApproximatingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_NurbsApproximatingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdContainerDataSourceHandle& inputArgs)
{
    TF_UNUSED(inputArgs);
    return HdsiNurbsApproximatingSceneIndex::New(
        inputSceneIndex);
}

PXR_NAMESPACE_CLOSE_SCOPE
