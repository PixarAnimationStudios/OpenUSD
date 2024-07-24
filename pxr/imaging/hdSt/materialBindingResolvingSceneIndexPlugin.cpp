//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdSt/materialBindingResolvingSceneIndexPlugin.h"

#include "pxr/imaging/hdsi/materialBindingResolvingSceneIndex.h"

#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_MaterialBindingResolvingSceneIndexPlugin")));

static const char* const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdSt_MaterialBindingResolvingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName, _tokens->sceneIndexPluginName, nullptr,
        insertionPhase, HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

HdSt_MaterialBindingResolvingSceneIndexPlugin::
    HdSt_MaterialBindingResolvingSceneIndexPlugin()
    = default;

HdSt_MaterialBindingResolvingSceneIndexPlugin::
    ~HdSt_MaterialBindingResolvingSceneIndexPlugin()
    = default;

HdSceneIndexBaseRefPtr
HdSt_MaterialBindingResolvingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdContainerDataSourceHandle& inputArgs)
{
    TF_UNUSED(inputArgs);
    return HdsiMaterialBindingResolvingSceneIndex::New(
        inputSceneIndex,
        {
            HdTokens->preview,
            HdMaterialBindingsSchemaTokens->allPurpose,
        },
        HdMaterialBindingsSchemaTokens->allPurpose);
}

PXR_NAMESPACE_CLOSE_SCOPE
