//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexUtil.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(HdSceneIndexPluginRegistry);

TF_DEFINE_PUBLIC_TOKENS(HdSceneIndexPluginRegistryTokens,
    HDSCENEINDEXPLUGINREGISTRY_TOKENS);


HdSceneIndexPluginRegistry &
HdSceneIndexPluginRegistry::GetInstance()
{
    return TfSingleton<HdSceneIndexPluginRegistry>::GetInstance();
}

HdSceneIndexPluginRegistry::HdSceneIndexPluginRegistry()
 : HfPluginRegistry(TfType::Find<HdSceneIndexPlugin>())
{
    TfSingleton<HdSceneIndexPluginRegistry>::SetInstanceConstructed(*this);
    TfRegistryManager::GetInstance().SubscribeTo<HdSceneIndexPlugin>();

    // Force discovery at instantiation time
    std::vector<HfPluginDesc> descs;
    HdSceneIndexPluginRegistry::GetInstance().GetPluginDescs(&descs);
}

HdSceneIndexPluginRegistry::~HdSceneIndexPluginRegistry() = default;

HdSceneIndexPlugin *
HdSceneIndexPluginRegistry::_GetSceneIndexPlugin(const TfToken &pluginId)
{
    return static_cast<HdSceneIndexPlugin*>(GetPlugin(pluginId));
}

HdSceneIndexBaseRefPtr
HdSceneIndexPluginRegistry::AppendSceneIndex(
    const TfToken &sceneIndexPluginId,
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    if (HdSceneIndexPlugin *plugin = _GetSceneIndexPlugin(sceneIndexPluginId)) {
        HdSceneIndexBaseRefPtr result =
            plugin->AppendSceneIndex(inputScene, inputArgs);

        // NOTE: While HfPluginRegistry has a ref count mechanism for
        //       life time of plug-in instances, we don't need them to be
        //       cleaned up -- so we won't manually decrement their ref count
        //ReleasePlugin(plugin);

        return result;
    } else {
        return inputScene;
    }
}

HdSceneIndexBaseRefPtr
HdSceneIndexPluginRegistry::_AppendForPhases(
    const HdSceneIndexBaseRefPtr &inputScene,
    const _PhasesMap &phasesMap,
    const HdContainerDataSourceHandle &argsUnderlay,
    const std::string &renderInstanceId)
{
    HdSceneIndexBaseRefPtr result = inputScene;
    for (const auto &phasesPair : phasesMap) {
        for (const _Entry &entry : phasesPair.second) {
            
            HdContainerDataSourceHandle args = entry.args;

            if (args) {
                if (argsUnderlay) {

                    HdContainerDataSourceHandle c[] = {
                        args,
                        argsUnderlay,
                    };

                    args = HdOverlayContainerDataSource::New(2, c);

                }
            } else {
                args = argsUnderlay;
            }

            if (entry.callback) {
                result = entry.callback(renderInstanceId, result, args);
            } else {
                result = AppendSceneIndex(
                    entry.sceneIndexPluginId, result, args);
            }

        }
    }
    return result;
}

void
HdSceneIndexPluginRegistry::_CollectAdditionalMetadata(
        const PlugRegistry &plugRegistry, const TfType &pluginType)
{
    const JsValue &loadWithRendererValue =
        plugRegistry.GetDataFromPluginMetaData(pluginType,
            "loadWithRenderer");

    if (loadWithRendererValue.GetType() == JsValue::StringType) {
        _preloadsForRenderer[loadWithRendererValue.GetString()].push_back(
            TfToken(pluginType.GetTypeName().c_str()));

    } else if (loadWithRendererValue.GetType() == JsValue::ArrayType) {
        for (const std::string &s  : 
                loadWithRendererValue.GetArrayOf<std::string>()) {
            _preloadsForRenderer[s].push_back(
                TfToken(pluginType.GetTypeName().c_str()));
        }
    }
}

HdSceneIndexBaseRefPtr
HdSceneIndexPluginRegistry::AppendSceneIndicesForRenderer(
    const std::string &rendererDisplayName,
    const HdSceneIndexBaseRefPtr &inputScene,
    const std::string &renderInstanceId)
{

    // Preload any renderer plug-ins which have been tagged (via plugInfo) to
    // be loaded along with the specified renderer (or any renderer)
    const std::string preloadKeys[] = {
        std::string(""),        // preload for any renderer
        rendererDisplayName,    // preload for this renderer
    };

    for (size_t i = 0; i < TfArraySize(preloadKeys); ++i) {
        _PreloadMap::iterator plit = _preloadsForRenderer.find(preloadKeys[i]);
        if (plit != _preloadsForRenderer.end()) {
            for (const TfToken &id : plit->second) {
                // this only ensures that the plug-in is loaded as the plug-in
                // itself might do further registration relevant to below.
                _GetSceneIndexPlugin(id);
            }

            // Preload only needs to happen once per process. Remove the
            // just-processed key.
            _preloadsForRenderer.erase(plit);
        }
    }

    HdContainerDataSourceHandle underlayArgs =
        HdRetainedContainerDataSource::New(
            HdSceneIndexPluginRegistryTokens->rendererDisplayName,
            HdRetainedTypedSampledDataSource<std::string>::New(
                rendererDisplayName));

    _PhasesMap mergedPhasesMap;

    // append scene indices registered to run for all renderers first
    _RenderersMap::const_iterator it = _sceneIndicesForRenderers.find("");
    if (it != _sceneIndicesForRenderers.end()) {
        mergedPhasesMap = it->second;
    }
    // append scene indices registered to run for specified renderer
    if (!rendererDisplayName.empty()) {
        it = _sceneIndicesForRenderers.find(rendererDisplayName);
        if (it != _sceneIndicesForRenderers.end()) {
            for (auto const &phaseEntry: it->second) {
                InsertionPhase phase = phaseEntry.first;
                _EntryList &mergedEntries = mergedPhasesMap[phase];
                mergedEntries.insert(
                    mergedEntries.end(),
                    phaseEntry.second.begin(),
                    phaseEntry.second.end());
            }
        }
    }

    HdSceneIndexBaseRefPtr scene =
        _AppendForPhases(inputScene, mergedPhasesMap,
                         underlayArgs, renderInstanceId);
    if (TfGetEnvSetting<bool>(HD_USE_ENCAPSULATING_SCENE_INDICES)) {
        scene = HdMakeEncapsulatingSceneIndex(
            { inputScene }, scene);
        scene->SetDisplayName("Scene index plugins");
    }
    return scene;
}

void
HdSceneIndexPluginRegistry::RegisterSceneIndexForRenderer(
    const std::string &rendererDisplayName,
    const TfToken &sceneIndexPluginId,
    const HdContainerDataSourceHandle &inputArgs,
    InsertionPhase insertionPhase,
    InsertionOrder insertionOrder)
{
    _EntryList &entryList =
        _sceneIndicesForRenderers[rendererDisplayName][insertionPhase];

    if (insertionOrder == InsertionOrderAtStart) {
        entryList.insert(entryList.begin(), _Entry{sceneIndexPluginId, inputArgs});
    } else {
        entryList.emplace_back(sceneIndexPluginId, inputArgs);
    }
}

void
HdSceneIndexPluginRegistry::RegisterSceneIndexForRenderer(
    const std::string &rendererDisplayName,
    SceneIndexAppendCallback callback,
    const HdContainerDataSourceHandle &inputArgs,
    InsertionPhase insertionPhase,
    InsertionOrder insertionOrder)
{
    _EntryList &entryList =
        _sceneIndicesForRenderers[rendererDisplayName][insertionPhase];

    if (insertionOrder == InsertionOrderAtStart) {
        entryList.insert(entryList.begin(), _Entry{callback, inputArgs});
    } else {
        entryList.emplace_back(callback, inputArgs);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
