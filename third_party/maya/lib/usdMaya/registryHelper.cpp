//
// Copyright 2016 Pixar
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
#include "usdMaya/registryHelper.h"

#include "usdMaya/debugCodes.h"

#include "pxr/base/js/converter.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"

#include <maya/MGlobal.h>

#include <map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(_tokens, 
    (mayaPlugin)
    (providesTranslator)
    (UsdMaya)
    (ShadingModePlugin)
);

template <typename T>
bool
_GetData(const JsValue& any, T* val)
{
    if (!any.Is<T>()) {
        TF_CODING_ERROR("bad plugInfo.json");
        return false;
    }

    *val = any.Get<T>();
    return true;
}

template <typename T>
bool
_GetData(const JsValue& any, std::vector<T>* val)
{
    if (!any.IsArrayOf<T>()) {
        TF_CODING_ERROR("bad plugInfo.json");
        return false;
    }

    *val = any.GetArrayOf<T>();
    return true;
}

static bool
_ReadNestedDict(
        const JsObject& data,
        const std::vector<TfToken>& keys,
        JsObject* dict)
{
    JsObject currDict = data;
    TF_FOR_ALL(iter, keys) {
        const TfToken& currKey = *iter;
        JsValue any;
        if (!TfMapLookup(currDict, currKey, &any)) {
            return false;
        }

        if (!any.IsObject()) {
            TF_CODING_ERROR("bad plugInfo data.");
            return false;
        }
        currDict = any.GetJsObject();
    }
    *dict = currDict;
    return true;
}

static bool
_ProvidesForType(
        const PlugPluginPtr& plug,
        const std::vector<TfToken>& scope,
        const std::string& typeName,
        std::string* mayaPluginName)
{

    JsObject metadata = plug->GetMetadata();
    JsObject mayaTranslatorMetadata;
    if (!_ReadNestedDict(metadata, scope, &mayaTranslatorMetadata)) {
        return false;
    }

    JsValue any;
    if (!TfMapLookup(mayaTranslatorMetadata, _tokens->providesTranslator, &any)) {
        return false;
    }
    std::vector<std::string> usdTypes;
    if (!_GetData(any, &usdTypes)) {
        return false;
    }

    bool provides = std::find(
            usdTypes.begin(), usdTypes.end(), 
            typeName) != usdTypes.end();
    if (provides) {
        if (TfMapLookup(mayaTranslatorMetadata, _tokens->mayaPlugin, &any)) {
            return _GetData(any, mayaPluginName);
        }
    }

    return provides;
}

static bool
_HasShadingModePlugin(
    const PlugPluginPtr& plug,
    const std::vector<TfToken>& scope,
    std::string* mayaPluginName)
{
    JsObject metadata = plug->GetMetadata();
    JsObject mayaTranslatorMetadata;
    if (!_ReadNestedDict(metadata, scope, &mayaTranslatorMetadata)) {
        return false;
    }

    JsValue any;
    if (TfMapLookup(mayaTranslatorMetadata, _tokens->mayaPlugin, &any)) {
        return _GetData(any, mayaPluginName);
    }

    return false;
}

/* static */
std::string
_PluginDictScopeToDebugString(
        const std::vector<TfToken>& scope)
{
    std::vector<std::string> s;
    TF_FOR_ALL(iter, scope) {
        s.push_back(iter->GetString());
    }
    return TfStringJoin(s, "/");
}

/* static */
void
UsdMaya_RegistryHelper::FindAndLoadMayaPlug(
        const std::vector<TfToken>& scope,
        const std::string& value)
{
    std::string mayaPlugin;
    PlugPluginPtrVector plugins = PlugRegistry::GetInstance().GetAllPlugins();
    TF_FOR_ALL(plugIter, plugins) {
        PlugPluginPtr plug = *plugIter;
        if (_ProvidesForType(plug, scope, value, &mayaPlugin)) {
            if (!mayaPlugin.empty()) {
                TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
                        "Found usdMaya plugin %s:  %s = %s.  Loading maya plugin %s.\n", 
                        plug->GetName().c_str(),
                        _PluginDictScopeToDebugString(scope).c_str(),
                        value.c_str(),
                        mayaPlugin.c_str());
                std::string loadPluginCmd = TfStringPrintf(
                        "loadPlugin -quiet %s", mayaPlugin.c_str());
                if (MGlobal::executeCommand(loadPluginCmd.c_str())) {
                    // Need to ensure Python script modules are loaded
                    // properly for this library (Maya's loadPlugin will not
                    // load script modules like TfDlopen would).
                    TfScriptModuleLoader::GetInstance().LoadModules();
                }
                else {
                    TF_CODING_ERROR("Unable to load mayaplugin %s\n",
                            mayaPlugin.c_str());
                }
            }
            else {
                TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
                        "Found usdMaya plugin %s: %s = %s.  No maya plugin.\n", 
                        plug->GetName().c_str(),
                        _PluginDictScopeToDebugString(scope).c_str(),
                        value.c_str());
            }
            break;
        }
    }
}

/* static */
void
UsdMaya_RegistryHelper::LoadShadingModePlugins() {
    static std::once_flag _shadingModesLoaded;
    static std::vector<TfToken> scope = {_tokens->UsdMaya, _tokens->ShadingModePlugin};
    std::call_once(_shadingModesLoaded, [](){        
        PlugPluginPtrVector plugins = PlugRegistry::GetInstance().GetAllPlugins();
        std::string mayaPlugin;
        TF_FOR_ALL(plugIter, plugins) {
            PlugPluginPtr plug = *plugIter;
            if (_HasShadingModePlugin(plug, scope, &mayaPlugin) && !mayaPlugin.empty()) {
                TF_DEBUG(PXRUSDMAYA_REGISTRY).Msg(
                            "Found usdMaya plugin %s: Loading maya plugin %s.\n", 
                            plug->GetName().c_str(),
                            mayaPlugin.c_str());
                std::string loadPluginCmd = TfStringPrintf(
                        "loadPlugin -quiet %s", mayaPlugin.c_str());
                if (MGlobal::executeCommand(loadPluginCmd.c_str())) {
                    // Need to ensure Python script modules are loaded
                    // properly for this library (Maya's loadPlugin will not
                    // load script modules like TfDlopen would).
                    TfScriptModuleLoader::GetInstance().LoadModules();
                } else {
                    TF_CODING_ERROR("Unable to load mayaplugin %s\n",
                            mayaPlugin.c_str());
                }
            }
        }
    });
}

/* static */
VtDictionary
UsdMaya_RegistryHelper::GetComposedInfoDictionary(
    const std::vector<TfToken>& scope)
{
    VtDictionary result;

    std::map<std::string, std::vector<std::string>> keyDefinitionSites;
    PlugPluginPtrVector plugins = PlugRegistry::GetInstance().GetAllPlugins();
    for (const PlugPluginPtr& plugin : plugins) {
        JsObject curJsDict;
        if (_ReadNestedDict(plugin->GetMetadata(), scope, &curJsDict)) {
            const VtValue curValue =
                    JsConvertToContainerType<VtValue, VtDictionary>(curJsDict);
            if (curValue.IsHolding<VtDictionary>()) {
                for (const std::pair<std::string, VtValue>& pair :
                        curValue.UncheckedGet<VtDictionary>()) {
                    result[pair.first] = pair.second;
                    keyDefinitionSites[pair.first].push_back(plugin->GetName());
                }
            }
            else {
                TF_RUNTIME_ERROR("Unable to read scope '%s' from plugInfo for "
                        "plugin '%s'",
                        TfStringJoin(scope.begin(), scope.end(), "/").c_str(),
                        plugin->GetName().c_str());
            }
        }
    }

    // Validate that keys are only defined once globally.
    for (const std::pair<std::string, std::vector<std::string>>& pair :
            keyDefinitionSites) {
        if (pair.second.size() != 1) {
            TF_RUNTIME_ERROR(
                    "Key '%s' is defined in multiple plugins (%s). "
                    "Key values must be defined in only one plugin at a time. "
                    "Plugin values will be ignored for this key.",
                    pair.first.c_str(),
                    TfStringJoin(pair.second, ", ").c_str());
            result.erase(pair.first);
        }
    }

    return result;
}

/* static */
void
UsdMaya_RegistryHelper::AddUnloader(const std::function<void()>& func)
{
    if (TfRegistryManager::GetInstance().AddFunctionForUnload(func)) {
        // It is likely that the registering plugin library is opened/closed
        // by Maya and not via TfDlopen/TfDlclose. This means that the
        // unloaders won't be invoked unless we use RunUnloadersAtExit(),
        // which allows unloaders to be called from normal dlclose().
        TfRegistryManager::GetInstance().RunUnloadersAtExit();
    }
    else {
        TF_CODING_ERROR(
                "Couldn't add unload function (was this function called from "
                "outside a TF_REGISTRY_FUNCTION block?)");
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

