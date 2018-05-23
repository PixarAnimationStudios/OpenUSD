//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "shadingModeRegistry.h"
#include "debugCodes.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/token.h"

#include <OP/OP_Node.h>
#include <OP/OP_OperatorTable.h>

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    (houdiniPlugin)
    (UsdHoudini)
    (ShadingModePlugin)
);

namespace {

template <typename T> bool inline
getData(const JsValue& any, T& val)
{
    if (!any.Is<T>()) {
        TF_CODING_ERROR("Bad plugInfo.json");
        return false;
    }

    val = any.Get<T>();
    return true;
}

bool inline
readNestedDict(
        const JsObject& data,
        const std::vector<TfToken>& keys,
        JsObject& dict)
{
    JsObject currDict = data;
    for (const auto& currKey: keys) {
        JsValue any;
        if (!TfMapLookup(currDict, currKey, &any)) {
            return false;
        }

        if (!any.IsObject()) {
            TF_CODING_ERROR("Bad plugInfo data.");
            return false;
        }
        currDict = any.GetJsObject();
    }
    dict = currDict;
    return true;
}

bool inline
hasPlugin(
    const PlugPluginPtr& plug,
    const std::vector<TfToken>& scope,
    const TfToken& pluginType)
{
    JsObject metadata = plug->GetMetadata();
    JsObject houdiniMetadata;
    if (!readNestedDict(metadata, scope, houdiniMetadata)) {
        return false;
    }

    JsValue any;
    if (TfMapLookup(houdiniMetadata, pluginType, &any)) {
        bool hasPlugin = false;
        return getData(any, hasPlugin) & hasPlugin;
    }

    return false;
}

void inline
loadAllPlugins(std::once_flag& once_flag, const std::vector<TfToken>& scope, const TfToken& pluginType, OP_OperatorTable* table) {
    std::call_once(once_flag, [&scope, &table, &pluginType](){
        for (const auto& plug: PlugRegistry::GetInstance().GetAllPlugins()) {
            if (hasPlugin(plug, scope, pluginType)) {
                TF_DEBUG(PXRUSDHOUDINI_REGISTRY).Msg(
                    "Found UsdHoudini plugin %s: Loading from: %s",
                    plug->GetName().c_str(),
                    plug->GetPath().c_str());
                if (!table->loadDSO(plug->GetPath().c_str())) {
                    TF_CODING_ERROR("Failed to load usdHoudini plugin.");
                }
            }
        }
    });
}

}

using _ExporterRegistryElem = std::tuple<GusdShadingModeRegistry::ExporterFn, TfToken>;
using _ExporterRegistry = std::map<TfToken, _ExporterRegistryElem>;
static _ExporterRegistry _exporterRegistry;

bool
GusdShadingModeRegistry::registerExporter(
    const std::string& name,
    const std::string& label,
    GusdShadingModeRegistry::ExporterFn creator) {
    auto insertStatus = _exporterRegistry.insert(
        {TfToken(name), _ExporterRegistryElem{creator, TfToken(label)}}
    );
    return insertStatus.second;
}

GusdShadingModeRegistry::ExporterFn
GusdShadingModeRegistry::_getExporter(const TfToken& name) {
    TfRegistryManager::GetInstance().SubscribeTo<GusdShadingModeRegistry>();
    const auto it = _exporterRegistry.find(name);
    return it == _exporterRegistry.end() ? nullptr : std::get<0>(it->second);
}

GusdShadingModeRegistry::ExporterList
GusdShadingModeRegistry::_listExporters() {
    TfRegistryManager::GetInstance().SubscribeTo<GusdShadingModeRegistry>();
    GusdShadingModeRegistry::ExporterList ret;
    ret.reserve(_exporterRegistry.size());
    for (const auto& it: _exporterRegistry) {
        ret.emplace_back(it.first, std::get<1>(it.second));
    }
    return ret;
}

void
GusdShadingModeRegistry::loadPlugins(OP_OperatorTable* table) {
    static std::once_flag _shadingModesLoaded;
    static std::vector<TfToken> scope = {_tokens->UsdHoudini};
    loadAllPlugins(_shadingModesLoaded, scope, _tokens->ShadingModePlugin, table);
}

TF_INSTANTIATE_SINGLETON(GusdShadingModeRegistry);

GusdShadingModeRegistry&
GusdShadingModeRegistry::getInstance() {
    return TfSingleton<GusdShadingModeRegistry>::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
