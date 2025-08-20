//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "hdPrman/tetMeshConversionSceneIndexPlugin.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdsi/tetMeshConversionSceneIndex.h"
#include "hdPrman/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_TetMeshConversionSceneIndexPlugin"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_TetMeshConversionSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    for( auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            nullptr, // No input args.
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Implementation of HdPrman_TetMeshConversionSceneIndexPlugin

HdPrman_TetMeshConversionSceneIndexPlugin::
HdPrman_TetMeshConversionSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_TetMeshConversionSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdsiTetMeshConversionSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
