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
    const HdContainerDataSourceHandle &inputArgs,
    const std::string &renderInstanceId)
{
    if (HdSceneIndexPlugin *plugin = _GetSceneIndexPlugin(sceneIndexPluginId)) {
        HdSceneIndexBaseRefPtr result =
            plugin->AppendSceneIndex(renderInstanceId, inputScene, inputArgs);

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
    const _EntriesByPhasesMap &entriesByPhases,
    const HdContainerDataSourceHandle &argsUnderlay,
    const std::string &renderInstanceId)
{
    HdSceneIndexBaseRefPtr result = inputScene;
    for (const auto &phasesPair : entriesByPhases) {
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
                    entry.sceneIndexPluginId, result, args, renderInstanceId);
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

    const TfToken pluginTypeToken(pluginType.GetTypeName());

    if (loadWithRendererValue.GetType() == JsValue::StringType) {
        _preloadsForRenderer[loadWithRendererValue.GetString()].push_back(
            pluginTypeToken);

    } else if (loadWithRendererValue.GetType() == JsValue::ArrayType) {
        for (const std::string &s  : 
                loadWithRendererValue.GetArrayOf<std::string>()) {
            _preloadsForRenderer[s].push_back(pluginTypeToken);
        }
    }

    const JsValue &loadWithAppsValue =
        plugRegistry.GetDataFromPluginMetaData(
            pluginType, "loadWithApps");
    if (loadWithAppsValue.GetType() == JsValue::StringType) {
        _preloadAppsForPlugins[pluginTypeToken]
            .insert(loadWithAppsValue.GetString());
    } else if (loadWithAppsValue.GetType() == JsValue::ArrayType) {
        const std::vector<std::string> loadWithApps =
            loadWithAppsValue.GetArrayOf<std::string>();
        _preloadAppsForPlugins[pluginTypeToken] = std::set<std::string>(
            loadWithApps.begin(), loadWithApps.end());
    }
}

void
HdSceneIndexPluginRegistry::_LoadPluginsForRenderer(
    const std::string &rendererDisplayName,
    const std::string &appName)
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

            TfTokenVector &rendererPlugins = plit->second;

            for (auto iter = rendererPlugins.begin();
                    iter != rendererPlugins.end();) {
                const TfToken &id = *iter;

                const auto appsIter = _preloadAppsForPlugins.find(id);
                if (appsIter != _preloadAppsForPlugins.end()) {
                    const std::set<std::string> &apps = appsIter->second;
                    if (!apps.empty() && !apps.count(appName)) {
                        // This plugin has a non-empty array entry for
                        // loadWithApps and the app we're making scene indexes
                        // for isn't in the array: don't load it.
                        ++iter;
                        continue;
                    }
                }

                // this only ensures that the plug-in is loaded as the plug-in
                // itself might do further registration relevant to below.
                _GetSceneIndexPlugin(id);

                // Preload only needs to happen once per process. Remove this
                // plugin so we don't try to load it again later.
                iter = rendererPlugins.erase(iter);
            }

            if (rendererPlugins.empty()) {
                // Common case: we loaded all the plugins. We can entirely
                // erase the entry for this renderer from the overall map.
                _preloadsForRenderer.erase(plit);
            }
        }
    }
}

// static
HdSceneIndexPluginRegistry::_EntriesByPhasesMap
HdSceneIndexPluginRegistry::_RendererEntriesToPhaseMap(
        const _RendererEntries& rendererEntries)
{
    // Assert that we will visit "start" before "end".
    static_assert(
        HdSceneIndexPluginRegistry::InsertionOrderAtStart
        < HdSceneIndexPluginRegistry::InsertionOrderAtEnd);

    std::map<InsertionPhase, _EntryList> ret;
    for (const auto& phaseOrderAndEntries : rendererEntries) {
        const auto& [phaseAndOrder, entryList] = phaseOrderAndEntries;
        const InsertionPhase& insertionPhase = phaseAndOrder.first;
        _EntryList& entries = ret[insertionPhase];
        entries.insert(entries.end(), entryList.begin(), entryList.end());
        // sort the entries from this phase so that we get a stable order that
        // does not depend on the order in which the plugins are loaded.
        std::sort(
            entries.end() - entryList.size(), entries.end(),
            [](const _Entry& a, const _Entry& b) {
                return a.sceneIndexPluginId < b.sceneIndexPluginId;
            });
    }
    return ret;
}

HdSceneIndexPluginRegistry::_EntriesByPhasesMap
HdSceneIndexPluginRegistry::_ComputeEntriesByPhasesMap(
    const std::string& rendererDisplayName) const
{
    _EntriesByPhasesMap mergedPhasesMap;
    // append scene indices registered to run for all renderers first
    // Note, this means a plugin registered to run "at end" of a phase, will
    // run before a renderer specific plugin registered to run "at start" of a
    // phase.
    _RenderersMap::const_iterator it = _sceneIndicesForRenderers.find("");
    if (it != _sceneIndicesForRenderers.end()) {
        mergedPhasesMap = _RendererEntriesToPhaseMap(it->second);
    }

    // append scene indices registered to run for specified renderer
    if (!rendererDisplayName.empty()) {
        it = _sceneIndicesForRenderers.find(rendererDisplayName);
        if (it != _sceneIndicesForRenderers.end()) {
            for (auto const& phaseEntry : _RendererEntriesToPhaseMap(it->second)) {
                InsertionPhase phase = phaseEntry.first;
                _EntryList& mergedEntries = mergedPhasesMap[phase];
                mergedEntries.insert(
                    mergedEntries.end(), phaseEntry.second.begin(),
                    phaseEntry.second.end());
            }
        }
    }
    return mergedPhasesMap;
}

HdSceneIndexBaseRefPtr
HdSceneIndexPluginRegistry::AppendSceneIndicesForRenderer(
    const std::string &rendererDisplayName,
    const HdSceneIndexBaseRefPtr &inputScene,
    const std::string &renderInstanceId,
    const std::string &appName)
{
    _LoadPluginsForRenderer(rendererDisplayName, appName);

    HdContainerDataSourceHandle underlayArgs =
        HdRetainedContainerDataSource::New(
            HdSceneIndexPluginRegistryTokens->rendererDisplayName,
            HdRetainedTypedSampledDataSource<std::string>::New(
                rendererDisplayName));

    HdSceneIndexBaseRefPtr scene = _AppendForPhases(
        inputScene, _ComputeEntriesByPhasesMap(rendererDisplayName),
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
    _sceneIndicesForRenderers[rendererDisplayName]
                             [{ insertionPhase, insertionOrder }]
                                 .emplace_back(sceneIndexPluginId, inputArgs);
}

void
HdSceneIndexPluginRegistry::RegisterSceneIndexForRenderer(
    const std::string &rendererDisplayName,
    SceneIndexAppendCallback callback,
    const HdContainerDataSourceHandle &inputArgs,
    InsertionPhase insertionPhase,
    InsertionOrder insertionOrder)
{
    _sceneIndicesForRenderers[rendererDisplayName]
                             [{ insertionPhase, insertionOrder }]
                                 .emplace_back(callback, inputArgs);
}

std::vector<TfToken>
HdSceneIndexPluginRegistry::LoadAndGetSceneIndexPluginIds(
    const std::string& rendererDisplayName, const std::string& appName)
{
    std::vector<TfToken> ret;
    _LoadPluginsForRenderer(rendererDisplayName, appName);
    for (const auto& phaseAndEntryList :
         _ComputeEntriesByPhasesMap(rendererDisplayName)) {
        const _EntryList& entryList = phaseAndEntryList.second;
        for (const _Entry& entry: entryList) {
            ret.push_back(entry.sceneIndexPluginId);
        }
    }
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
