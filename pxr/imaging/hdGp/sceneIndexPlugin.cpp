//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdGp/sceneIndexPlugin.h"
#include "pxr/imaging/hdGp/generativeProceduralResolvingSceneIndex.h"
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
    // Safe to register the plugin always. If the env var isn't set, the plugin
    // won't be considered when creating the scene index plugin chain.
    //
    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        HdSceneIndexPluginRegistryTokens->allRenderers,
        TfToken("HdGpSceneIndexPlugin"),
        nullptr,   // no argument data necessary
        HdGpSceneIndexPlugin::GetInsertionPhase(),
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

HdGpSceneIndexPlugin::HdGpSceneIndexPlugin() = default;

/* virtual */
HdSceneIndexBaseRefPtr
HdGpSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    // Comment above in the registry block applies. This function shouldn't be
    // called if the plugin isn't enabled.
    if (!TF_VERIFY(_IsEnabled(inputArgs))) {
        return inputScene;
    }

    return _AppendProceduralResolvingSceneIndex(inputScene, inputArgs);
}

/* virtual */
bool
HdGpSceneIndexPlugin::_IsEnabled(
    const HdContainerDataSourceHandle &inputArgs) const
{
    static bool isEnabled = TfGetEnvSetting(HDGP_INCLUDE_DEFAULT_RESOLVER);
    return isEnabled;
}

HdSceneIndexBaseRefPtr
HdGpSceneIndexPlugin::_AppendProceduralResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    if (inputArgs) {
        using _TokenDs = HdTypedSampledDataSource<TfToken>;
        if (_TokenDs::Handle tds =_TokenDs::Cast(
                inputArgs->Get(_tokens->proceduralPrimTypeName))) {
            return HdGpGenerativeProceduralResolvingSceneIndex::New(
                inputScene,
                tds->GetTypedValue(0.0f));
        }
    }

    return HdGpGenerativeProceduralResolvingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE



