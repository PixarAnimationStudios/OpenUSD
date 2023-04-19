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
#include "pxr/usdImaging/usdImaging/adapterRegistry.h"

#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/instanceAdapter.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/staticData.h"

#include <set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_INSTANTIATE_SINGLETON(UsdImagingAdapterRegistry);

TF_MAKE_STATIC_DATA(TfType, _adapterBaseType) {
    *_adapterBaseType = TfType::Find<UsdImagingPrimAdapter>();
}

TF_MAKE_STATIC_DATA(TfType, _apiSchemaAdapterBaseType) {
    *_apiSchemaAdapterBaseType = TfType::Find<UsdImagingAPISchemaAdapter>();
}

TF_DEFINE_PUBLIC_TOKENS(UsdImagingAdapterKeyTokens, 
                        USD_IMAGING_ADAPTER_KEY_TOKENS);

// static 
bool 
UsdImagingAdapterRegistry::AreExternalPluginsEnabled()
{
    static bool areExternalPluginsEnabled = 
                                TfGetenvBool("USDIMAGING_ENABLE_PLUGINS", true);  
    return areExternalPluginsEnabled;
}

UsdImagingAdapterRegistry::UsdImagingAdapterRegistry() {
    // Statically load all prim-type information, note that Plug does not crack
    // open the libraries, it only reads metadata from text files.
    PlugRegistry& plugReg = PlugRegistry::GetInstance();
    std::set<TfType> types;
    PlugRegistry::GetAllDerivedTypes(*_adapterBaseType, &types);
    std::vector<TfToken> includeDerivedPrimTypes;

    TF_FOR_ALL(typeIt, types) {

        PlugPluginPtr plugin = plugReg.GetPluginForType(*typeIt);
        if (!plugin) {
            TF_DEBUG(USDIMAGING_PLUGINS).Msg("[PluginDiscover] Plugin could "
                    "not be loaded for TfType '%s'\n",
                    typeIt->GetTypeName().c_str());
            continue;
        }

        JsObject const& metadata = plugin->GetMetadataForType(*typeIt);

        // Check to see if external plugins are disabled, if so, check for
        // isInternal flag in the metadata to determine if the plugin should be
        // disabled.
        bool isEnabled = false;
        if (AreExternalPluginsEnabled()) {
            isEnabled = true;
        } else {
            JsObject::const_iterator it = metadata.find("isInternal");
            if (it != metadata.end()) {
                if (!it->second.Is<bool>()) {
                    TF_RUNTIME_ERROR("[PluginDiscover] isInternal metadata was "
                            "corrupted for plugin '%s'; not holding bool\n", 
                            typeIt->GetTypeName().c_str());
                    continue;
                } else { 
                    isEnabled = it->second.Get<bool>();
                }
            }
        }

        if (!isEnabled) {
            TF_DEBUG(USDIMAGING_PLUGINS).Msg("[PluginDiscover] Plugin disabled "
                        "because external plugins were disabled '%s'\n", 
                        typeIt->GetTypeName().c_str());
            continue;
        }


        JsObject::const_iterator it = metadata.find("primTypeName");
        if (it == metadata.end()) {
            TF_RUNTIME_ERROR("[PluginDiscover] primTypeName metadata was not "
                    "present for plugin '%s'\n", 
                    typeIt->GetTypeName().c_str());
            continue;
        }
        if (!it->second.Is<std::string>()) {
            TF_RUNTIME_ERROR("[PluginDiscover] primTypeName metadata was "
                    "corrupted for plugin '%s'\n", 
                    typeIt->GetTypeName().c_str());
            continue;
        }

        const TfToken primTypeName(it->second.Get<std::string>());

        TF_DEBUG(USDIMAGING_PLUGINS).Msg("[PluginDiscover] Plugin discovered "
                        "'%s' for primType '%s'\n", 
                        typeIt->GetTypeName().c_str(), primTypeName.GetText());
        if (_typeMap.count(primTypeName) != 0) {
            TF_CODING_ERROR("[PluginDiscover] A prim adapter for primType '%s' "
                "already exists! Overriding prim adapters at runtime is not "
                "supported. The last discovered adapter (%s) will be used. The "
                "previously discovered adapter (%s) will be discarded.",
                primTypeName.GetText(), typeIt->GetTypeName().c_str(),
                _typeMap[primTypeName].GetTypeName().c_str());
        }
        _typeMap[primTypeName] = *typeIt;

        // Adapters can opt in to being used as the adapter for any derived
        // prim types (without adapters of their own) of the targeted prim type 
        // through additional metadata.
        JsObject::const_iterator includeDerivedIt = 
            metadata.find("includeDerivedPrimTypes");
        if (includeDerivedIt != metadata.end()) {
            if (!includeDerivedIt->second.Is<bool>()) {
                TF_RUNTIME_ERROR("[PluginDiscover] includeDerivedPrimTypes "
                        "metadata was corrupted for plugin '%s'; not holding "
                        "bool\n", 
                        typeIt->GetTypeName().c_str());
                continue;
            } else if (includeDerivedIt->second.Get<bool>()){ 
                includeDerivedPrimTypes.push_back(primTypeName);
            }
        }
    }

    // Process the types whose derived types can use its adapter after all
    // explicit prim type to adapter mappings have been found.
    auto _ProcessDerivedTypes = [&includeDerivedPrimTypes](_TypeMap *tm) {
    for (const TfToken &primTypeName : includeDerivedPrimTypes) {
        const TfType primType = 
            UsdSchemaRegistry::GetTypeFromSchemaTypeName(primTypeName);
        if (!primType) {
            continue;
        }

        const TfType adapterType = (*tm)[primTypeName];

        // Start with just the directly derived types; we'll continue to
        // propagate the adapter type through derived prim types that do not 
        // have their own adapter already.
        std::vector<TfType> derivedTypesStack =
            PlugRegistry::GetDirectlyDerivedTypes(primType);

        while (!derivedTypesStack.empty()) {
            const TfType derivedType = derivedTypesStack.back();
            derivedTypesStack.pop_back();

            const TfToken typeName = 
                UsdSchemaRegistry::GetSchemaTypeName(derivedType);
            if (typeName.IsEmpty()) {
                continue;
            }

            // If the derived type name isn't already in the map, then the 
            // mapping to the ancestor's adapter is added and we'll continue
            // propagating to the next depth of derived types. Otherwise, the
            // derived type's adapter was already set and we skip its derived 
            // types regardless of whether they have adapters already or not.
            if (tm->emplace(typeName, adapterType).second) {
                TF_DEBUG(USDIMAGING_PLUGINS).Msg(
                    "[PluginDiscover] Mapping adapter for type '%s' to derived "
                    "type '%s'\n", 
                    primTypeName.GetText(), typeName.GetText());

                for (const TfType &type : 
                        PlugRegistry::GetDirectlyDerivedTypes(derivedType)) {
                    derivedTypesStack.push_back(type);
                }
            }
        }
    }};

    _ProcessDerivedTypes(&_typeMap);

    // Fill in the _adapterKeys vector, from the valid keys to _typeMap.
    _adapterKeys.reserve(_typeMap.size());
    for (auto const &pair : _typeMap) {
        _adapterKeys.push_back(pair.first);
    }

    types.clear();
    includeDerivedPrimTypes.clear();

    PlugRegistry::GetAllDerivedTypes(*_apiSchemaAdapterBaseType, &types);

    TF_FOR_ALL(typeIt, types) {

        PlugPluginPtr plugin = plugReg.GetPluginForType(*typeIt);
        if (!plugin) {
            TF_DEBUG(USDIMAGING_PLUGINS).Msg("[PluginDiscover] Plugin could "
                    "not be loaded for TfType '%s'\n",
                    typeIt->GetTypeName().c_str());
            continue;
        }

        JsObject const& metadata = plugin->GetMetadataForType(*typeIt);

        // Check to see if external plugins are disabled, if so, check for
        // isInternal flag in the metadata to determine if the plugin should be
        // disabled.
        bool isEnabled = false;
        if (AreExternalPluginsEnabled()) {
            isEnabled = true;
        } else {
            JsObject::const_iterator it = metadata.find("isInternal");
            if (it != metadata.end()) {
                if (!it->second.Is<bool>()) {
                    TF_RUNTIME_ERROR("[PluginDiscover] isInternal metadata was "
                            "corrupted for plugin '%s'; not holding bool\n", 
                            typeIt->GetTypeName().c_str());
                    continue;
                } else { 
                    isEnabled = it->second.Get<bool>();
                }
            }
        }

        if (!isEnabled) {
            TF_DEBUG(USDIMAGING_PLUGINS).Msg("[PluginDiscover] Plugin disabled "
                        "because external plugins were disabled '%s'\n", 
                        typeIt->GetTypeName().c_str());
            continue;
        }

        JsObject::const_iterator it = metadata.find("apiSchemaName");
        if (it == metadata.end()) {
            TF_RUNTIME_ERROR("[PluginDiscover] apiSchemaName metadata was not "
                    "present for plugin '%s'\n", 
                    typeIt->GetTypeName().c_str());
            continue;
        }
        if (!it->second.Is<std::string>()) {
            TF_RUNTIME_ERROR("[PluginDiscover] apiSchemaName metadata was "
                    "corrupted for plugin '%s'\n", 
                    typeIt->GetTypeName().c_str());
            continue;
        }

        const TfToken apiSchemaName(it->second.Get<std::string>());

        TF_DEBUG(USDIMAGING_PLUGINS).Msg("[PluginDiscover] Plugin discovered "
                        "'%s'\n", 
                        typeIt->GetTypeName().c_str());

        // sort the keyless types into a vector
        if (apiSchemaName.IsEmpty()) {
            _keylessApiSchemaAdapterTypes.push_back(*typeIt);
            continue;
        }

        _apiSchemaTypeMap[apiSchemaName] = *typeIt;

        // Adapters can opt in to being used as the adapter for any derived
        // prim types (without adapters of their own) of the targeted prim type 
        // through additional metadata.
        JsObject::const_iterator includeDerivedIt = 
            metadata.find("includeDerivedPrimTypes");
        if (includeDerivedIt != metadata.end()) {
            if (!includeDerivedIt->second.Is<bool>()) {
                TF_RUNTIME_ERROR("[PluginDiscover] includeDerivedPrimTypes "
                        "metadata was corrupted for plugin '%s'; not holding "
                        "bool\n", 
                        typeIt->GetTypeName().c_str());
                continue;
            } else if (includeDerivedIt->second.Get<bool>()){ 
                includeDerivedPrimTypes.push_back(apiSchemaName);
            }
        }
    }

    _ProcessDerivedTypes(&_apiSchemaTypeMap);

        // Fill in the _adapterKeys vector, from the valid keys to _typeMap.
    _apiSchemaTypeMap.reserve(_typeMap.size());
    for (auto const &pair : _apiSchemaTypeMap) {
        _apiSchemaAdapterKeys.push_back(pair.first);
    }

}

bool
UsdImagingAdapterRegistry::HasAdapter(TfToken const& adapterKey)
{
    // Check if the key refers to any special built-in adapter types.
    if (adapterKey == UsdImagingAdapterKeyTokens->instanceAdapterKey) {
        return true;
    }
    return _typeMap.find(adapterKey) != _typeMap.end();
}

const TfTokenVector&
UsdImagingAdapterRegistry::GetAdapterKeys()
{
    return _adapterKeys;
}


template <typename T, typename factoryT>
std::shared_ptr<T>
UsdImagingAdapterRegistry::_ConstructAdapter(
    TfToken const& adapterKey, const _TypeMap &tm)
{
    using _SharedPtr = std::shared_ptr<T>;

    static _SharedPtr NULL_ADAPTER;

    // Lookup the plug-in type name based on the prim type.
    _TypeMap::const_iterator typeIt = tm.find(adapterKey);

    if (typeIt == tm.end()) {
        // Unknown prim type.
        TF_DEBUG(USDIMAGING_PLUGINS).Msg("[PluginLoad] Unknown prim "
                "type '%s'\n",
                adapterKey.GetText());
        return NULL_ADAPTER;
    }

    return _ConstructAdapter<T, factoryT>(adapterKey, typeIt->second);
}

template <typename T, typename factoryT>
std::shared_ptr<T>
UsdImagingAdapterRegistry::_ConstructAdapter(
    TfToken const& adapterKey, const TfType &adapterType)
{
    using _SharedPtr = std::shared_ptr<T>;

    static _SharedPtr NULL_ADAPTER;

    PlugRegistry& plugReg = PlugRegistry::GetInstance();
    PlugPluginPtr plugin = plugReg.GetPluginForType(adapterType);
    if (!plugin || !plugin->Load()) {
        TF_CODING_ERROR("[PluginLoad] PlugPlugin could not be loaded for "
                "TfType '%s'\n",
                adapterType.GetTypeName().c_str());
        return NULL_ADAPTER;
    }

    factoryT* factory =
        adapterType.GetFactory<factoryT>();
    if (!factory) {
        TF_CODING_ERROR("[PluginLoad] Cannot manufacture type '%s' "
                "for Usd prim type '%s'\n",
                adapterType.GetTypeName().c_str(),
                adapterKey.GetText());

        return NULL_ADAPTER;
    }

    _SharedPtr instance = factory->New();
    if (!instance) {
        TF_CODING_ERROR("[PluginLoad] Failed to instantiate type '%s' "
                "for Usd prim type '%s'\n",
                adapterType.GetTypeName().c_str(),
                adapterKey.GetText());
        return NULL_ADAPTER;
    }

    TF_DEBUG(USDIMAGING_PLUGINS).Msg("[PluginLoad] Loaded plugin '%s' > '%s'\n",
                adapterKey.GetText(),
                adapterType.GetTypeName().c_str());

    return instance;
}


UsdImagingPrimAdapterSharedPtr
UsdImagingAdapterRegistry::ConstructAdapter(TfToken const& adapterKey)
{
    // Check if the key refers to any special built-in adapter types.
    if (adapterKey == UsdImagingAdapterKeyTokens->instanceAdapterKey) {
        return UsdImagingPrimAdapterSharedPtr( 
            new UsdImagingInstanceAdapter);
    }

    return _ConstructAdapter<
        UsdImagingPrimAdapter, UsdImagingPrimAdapterFactoryBase>(
            adapterKey, _typeMap);
}

bool
UsdImagingAdapterRegistry::HasAPISchemaAdapter(TfToken const& adapterKey)
{
    return _apiSchemaTypeMap.find(adapterKey) != _apiSchemaTypeMap.end();
}

UsdImagingAPISchemaAdapterSharedPtr
UsdImagingAdapterRegistry::ConstructAPISchemaAdapter(TfToken const& adapterKey)
{
   return _ConstructAdapter<UsdImagingAPISchemaAdapter,
        UsdImagingAPISchemaAdapterFactoryBase>(
            adapterKey, _apiSchemaTypeMap);
}

const TfTokenVector&
UsdImagingAdapterRegistry::GetAPISchemaAdapterKeys()
{
    return _apiSchemaAdapterKeys;
}


UsdImagingAdapterRegistry::ApiSchemaAdapters
UsdImagingAdapterRegistry::ConstructKeylessAPISchemaAdapters()
{
    ApiSchemaAdapters result;
    result.reserve(_keylessApiSchemaAdapterTypes.size());

    for (const TfType &adapterType : _keylessApiSchemaAdapterTypes) {
        if (UsdImagingAPISchemaAdapterSharedPtr instance =
                _ConstructAdapter<UsdImagingAPISchemaAdapter,
                    UsdImagingAPISchemaAdapterFactoryBase>(
                        TfToken(), adapterType)) {
            result.push_back(instance);
        }
    }

    return result;
}


PXR_NAMESPACE_CLOSE_SCOPE

