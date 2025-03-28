//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/debuggingSceneIndexPlugin.h"

#include "pxr/imaging/hdsi/debuggingSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

#include "pxr/pxr.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDSI_DEBUGGING_SCENE_INDEX_INSERTION_PHASE,
                      "",
                      "Insertion phase for the debugging scene index. "
                      "Either an integer or an empty string (to not insert the "
                      "debugging scene index).");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdsiDebuggingSceneIndexPlugin")));

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdsiDebuggingSceneIndexPlugin>();
}

static
std::optional<HdSceneIndexPluginRegistry::InsertionPhase>
_InsertionPhase()
{
    const std::string value =
        TfGetEnvSetting(HDSI_DEBUGGING_SCENE_INDEX_INSERTION_PHASE);
    if (value.empty()) {
        return std::nullopt;
    }

    try {
        return stoi(value);
    }
    catch(const std::invalid_argument &) {
        TF_WARN(
            "HDSI_DEBUGGING_SCENE_INDEX_INSERTION_PHASE needs to be empty "
            "or an integer.");
    }
    catch(const std::out_of_range &) {
        TF_WARN(
            "HDSI_DEBUGGING_SCENE_INDEX_INSERTION_PHASE too large.");
    }
    return std::nullopt;
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    static const std::optional<HdSceneIndexPluginRegistry::InsertionPhase>
        insertionPhase = _InsertionPhase();

    if (!insertionPhase) {
        return;
    }

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        /* rendererDisplayName = */ "",
        _tokens->sceneIndexPluginName,
        /* inputArgs = */ nullptr,
        *insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
}

HdsiDebuggingSceneIndexPlugin::
HdsiDebuggingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdsiDebuggingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    return HdsiDebuggingSceneIndex::New(inputScene, inputArgs);
}

PXR_NAMESPACE_CLOSE_SCOPE
