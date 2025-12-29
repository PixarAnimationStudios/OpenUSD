//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdSt/unboundMaterialPruningSceneIndexPlugin.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdsi/unboundMaterialPruningSceneIndex.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX Temporary env setting to allow scene index to be disabled if it
//     regresses performance in some cases.
TF_DEFINE_ENV_SETTING(HDST_ENABLE_UNBOUND_MATERIAL_PRUNING_SCENE_INDEX,
    true, "Enable scene index that prunes unbound materials.");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_UnboundMaterialPruningSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "GL";

static bool
_IsEnabled()
{
    static bool enabled =
        TfGetEnvSetting(HDST_ENABLE_UNBOUND_MATERIAL_PRUNING_SCENE_INDEX);
    return enabled;
}

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_UnboundMaterialPruningSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // Want this as downstream as possible, but before the dependency forwarding
    // scnee index.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 900;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        /* inputArgs = */ nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

HdSt_UnboundMaterialPruningSceneIndexPlugin::
HdSt_UnboundMaterialPruningSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_UnboundMaterialPruningSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    if (!_IsEnabled()) {
        return inputScene;
    }
    
    // Define inputArgs here instead of in the TF_REGISTRY_FUNCTION block.
    // In the future, we may consider renaming the inputArgs parameter to
    // something like "sceneIndexGraphCreateArgs" to allow the app and renderer
    // plugin to provide arguments for scene indices instantiated via the
    // scene index plugin system.
    static const HdTokenArrayDataSourceHandle bindingPurposesDs =
        HdRetainedTypedSampledDataSource<VtArray<TfToken>>::New(
            VtArray<TfToken>({
                HdTokens->preview,
                HdMaterialBindingsSchemaTokens->allPurpose,
            }));

    static const HdContainerDataSourceHandle localInputArgs =
        HdRetainedContainerDataSource::New(
            HdsiUnboundMaterialPruningSceneIndexTokens->materialBindingPurposes,
            bindingPurposesDs);
    
    return HdsiUnboundMaterialPruningSceneIndex::New(
        inputScene, localInputArgs);
}

PXR_NAMESPACE_CLOSE_SCOPE
