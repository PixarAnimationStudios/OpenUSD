//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/meshLightResolvingSceneIndexPlugin.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "hdPrman/meshLightResolvingSceneIndex.h"
#include "hdPrman/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_MeshLightResolvingSceneIndexPlugin"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_MeshLightResolvingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // We need an "insertion point" that's *after* general material resolve.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 115;
    const HdContainerDataSourceHandle inputRIS = HdRetainedContainerDataSource::New(
        HdPrmanDisplayNamesTokens->RenderManRIS, 
        HdRetainedTypedSampledDataSource<bool>::New(true)
    );
    for(auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        // Pass a token through to signify RIS so we can disable motion blur.
        const bool isRIS = (pluginDisplayName == HdPrmanDisplayNamesTokens->RenderManRIS);
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            isRIS ? inputRIS : nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

HdPrman_MeshLightResolvingSceneIndexPlugin::
HdPrman_MeshLightResolvingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_MeshLightResolvingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    const bool isRIS = (inputArgs && inputArgs->Get(HdPrmanDisplayNamesTokens->RenderManRIS));
    return HdPrmanMeshLightResolvingSceneIndex::New(inputScene, isRIS);
}

PXR_NAMESPACE_CLOSE_SCOPE
