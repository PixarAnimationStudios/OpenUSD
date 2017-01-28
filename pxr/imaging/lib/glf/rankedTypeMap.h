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
#ifndef GLF_RANKED_TYPE_MAP_H
#define GLF_RANKED_TYPE_MAP_H

/// \file glf/rankedTypeMap.h

#include "pxr/pxr.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class GlfRankedTypeMap
///
/// Holds a token-to-type map with support for precedence per type.
///
class GlfRankedTypeMap {
public:
    typedef TfToken key_type;
    typedef TfType mapped_type;
    typedef int Precedence;

    /// Add key/value pairs from plugins.  If \p whitelist isn't empty the
    /// it's a comma separated list of types and only those types are added.
    /// \p keyMetadataName has the metadata key with the single key or a
    /// list of keys to map to the type.  All types derived from \p baseType
    /// are considered.
    template <class DEBUG_TYPE>
    void Add(const mapped_type& baseType,
             const std::string& keyMetadataName,
             DEBUG_TYPE debugType,
             const std::string& whitelist = std::string());

    /// Add a key/value pair if it's not in the map or the given precedence
    /// is larger than the current precedence.  This does nothing if the
    /// value is the unknown type.
    void Add(const key_type& key,const mapped_type& type, Precedence precedence)
    {
        if (type) {
            auto i = _typeMap.find(key);
            if (i == _typeMap.end() || i->second.precedence < precedence) {
                _typeMap[key] = { type, precedence };
            }
        }
    }

    /// Returns the highest precedence type for the given key or the unknown
    /// type if the key was not added.
    mapped_type Find(const key_type& key) const
    {
        auto i = _typeMap.find(key);
        return i == _typeMap.end() ? mapped_type() : i->second.type;
    }

private:
    struct _Mapped {
        TfType type;
        Precedence precedence;
    };
    typedef TfToken::HashFunctor _Hash;
    typedef TfHashMap<key_type, _Mapped, _Hash> _TypeMap;

    _TypeMap _typeMap;
};

template <class DEBUG_TYPE>
void
GlfRankedTypeMap::Add(
    const mapped_type& baseType,
    const std::string& keyMetadataName,
    DEBUG_TYPE debugType,
    const std::string& whitelist)
{
    // Statically load all plugin information, note that Plug does not crack
    // open the libraries, it only reads metadata from text files.
    PlugRegistry& plugReg = PlugRegistry::GetInstance();
    std::set<TfType> types;
    PlugRegistry::GetAllDerivedTypes(baseType, &types);

    const std::vector<std::string> restrictions = TfStringSplit(whitelist, ",");

    for (auto type: types) {
        // Get the plugin.
        PlugPluginPtr plugin = plugReg.GetPluginForType(type);
        if (!plugin) {
            TF_DEBUG(debugType).Msg(
	            "[PluginDiscover] Plugin could not be loaded "
		    "for TfType '%s'\n",
                    type.GetTypeName().c_str());
            continue;
        }

        // Check the whitelist.
        if (!restrictions.empty()) {
            bool goodPlugin = false;
            for (const auto& restriction: restrictions) {
                if (type.GetTypeName() == restriction) {
                    goodPlugin = true;
                }
            }
            if (!goodPlugin) {
                TF_DEBUG(debugType).Msg(	
                    "[PluginDiscover] Skipping restricted plugin: '%s'\n", 
                    type.GetTypeName().c_str());
                continue;
            }
        }

        JsObject const& metadata = plugin->GetMetadataForType(type);

        JsObject::const_iterator keyIt = metadata.find(keyMetadataName);
        if (keyIt == metadata.end()) {
            TF_RUNTIME_ERROR("[PluginDiscover] '%s' metadata "
	            "was not present for plugin '%s'\n", 
                    keyMetadataName.c_str(), type.GetTypeName().c_str());
            continue;
        }

        // Default precedence is 1. Plugins at equal precedence will be
        // registered in order of discovery.
        int precedence = 1;

        JsObject::const_iterator precedenceIt = metadata.find("precedence");
        if (precedenceIt != metadata.end()) {
            if (!precedenceIt->second.Is<int>()) {
                TF_RUNTIME_ERROR("[PluginDiscover] 'precedence' metadata "
                        "can not be read for plugin '%s'\n", 
                        type.GetTypeName().c_str());
            } else {
                precedence = precedenceIt->second.Get<int>();
            }
        }

        TF_DEBUG(debugType).Msg(	
			"[PluginDiscover] Plugin discovered '%s'\n", 
                        type.GetTypeName().c_str());

        typedef std::string Name;
	if (keyIt->second.Is<Name>()) {
            // single name
            Name const & name = keyIt->second.Get<Name>();
            Add(TfToken(name), type, precedence);

	} else if (keyIt->second.IsArrayOf<Name>()) {
            // list of names
	    for (const auto& name: keyIt->second.GetArrayOf<Name>()) {
                Add(TfToken(name), type, precedence);
	    }
	}
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif
