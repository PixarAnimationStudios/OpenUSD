//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdSt/tetMeshConversionSceneIndexPlugin.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdsi/tetMeshConversionSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_TetMeshConversionSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_TetMeshConversionSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr, // no argument data necessary
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Implementation of HdSt_TetMeshConversionSceneIndexPlugin

HdSt_TetMeshConversionSceneIndexPlugin::
HdSt_TetMeshConversionSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_TetMeshConversionSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return HdsiTetMeshConversionSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
