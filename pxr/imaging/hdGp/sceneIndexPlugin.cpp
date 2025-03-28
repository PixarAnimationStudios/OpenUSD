//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdGp/sceneIndexPlugin.h"
#include "pxr/imaging/hdGp/generativeProceduralResolvingSceneIndex.h"
#include "pxr/imaging/hdGp/generativeProceduralPluginRegistry.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (proceduralPrimTypeName)
);

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdGpSceneIndexPlugin>();
}

TF_DEFINE_ENV_SETTING(HDGP_INCLUDE_DEFAULT_RESOLVER, false,
    "Register a default hydra generative procedural resolver to the scene index"
    " chain.");

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // For now, do not add the procedural resolving scene index by default but
    // allow activation of a default configured instance via envvar.
    if (TfGetEnvSetting(HDGP_INCLUDE_DEFAULT_RESOLVER) == true) {

        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            TfToken(), // empty token means all renderers
            TfToken("HdGpSceneIndexPlugin"),
            nullptr,   // no argument data necessary
            HdGpSceneIndexPlugin::GetInsertionPhase(),
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

HdGpSceneIndexPlugin::HdGpSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdGpSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    // Ensure that procedurals are discovered are prior to the scene index
    // querying for specific procedurals. Absence of this was causing a test
    // case to non-deterministically fail due to not finding a registered
    // procedural.
    HdGpGenerativeProceduralPluginRegistry::GetInstance();
    
    HdSceneIndexBaseRefPtr result = inputScene;

    if (inputArgs) {
        using _TokenDs = HdTypedSampledDataSource<TfToken>;
        if (_TokenDs::Handle tds =_TokenDs::Cast(
                inputArgs->Get(_tokens->proceduralPrimTypeName))) {
            return HdGpGenerativeProceduralResolvingSceneIndex::New(result,
                tds->GetTypedValue(0.0f));
        }
    }

    return HdGpGenerativeProceduralResolvingSceneIndex::New(result);
}

PXR_NAMESPACE_CLOSE_SCOPE



