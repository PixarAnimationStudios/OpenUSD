//
// Copyright 2020 Pixar
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
#include "pxr/usd/usdLux/discoveryPlugin.h"

#include "pxr/usd/usdLux/boundableLightBase.h"
#include "pxr/usd/usdLux/lightDefParser.h"
#include "pxr/usd/usdLux/lightFilter.h"
#include "pxr/usd/usdLux/nonboundableLightBase.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/usd/schemaRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

const NdrStringVec& 
UsdLux_DiscoveryPlugin::GetSearchURIs() const
{
    static const NdrStringVec empty;
    return empty;
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (usdLux)
);

NdrNodeDiscoveryResultVec
UsdLux_DiscoveryPlugin::DiscoverNodes(const Context &context)
{
    NdrNodeDiscoveryResultVec result;

    // We want to discover nodes for all concrete schema types that derive from 
    // UsdLuxBoundableLightBase, UsdLuxNonboundableLightBase, and 
    // UsdLuxLightFilter.
    static const TfType boundableLightType = 
        TfType::Find<UsdLuxBoundableLightBase>();
    static const TfType nonboundableLightType = 
        TfType::Find<UsdLuxNonboundableLightBase>();
    static const TfType lightFilterType = TfType::Find<UsdLuxLightFilter>();
    // LightFilter is a concrete type and is legit to instantiate while light 
    // base types are abstract and cannot be instantiated. LightFilter must be
    // included in the discovery types.
    std::set<TfType> types({lightFilterType});
    PlugRegistry::GetAllDerivedTypes(boundableLightType, &types);
    PlugRegistry::GetAllDerivedTypes(nonboundableLightType, &types);
    PlugRegistry::GetAllDerivedTypes(lightFilterType, &types);

    for (const TfType &type : types) {
        static const PlugRegistry &plugRegistry = PlugRegistry::GetInstance();
        const PlugPluginPtr &plugin = plugRegistry.GetPluginForType(type);
        if (!plugin) {
            continue;
        }
        if (_tokens->usdLux != plugin->GetName()) {
            continue;
        }
        const TfToken name = UsdSchemaRegistry::GetConcreteSchemaTypeName(type);
        // The type name from the schema registry will be empty if the type is 
        // not concrete (i.e. abstract); we skip abstract types.
        if (!name.IsEmpty()) {
            // The schema type name is the name and identifier. The URIs are 
            // left empty as these nodes can be populated from the schema
            // registry prim definitions. 
            result.emplace_back(
                /*identifier=*/ name,
                NdrVersion().GetAsDefault(),
                name,
                /*family=*/ TfToken(), 
                UsdLux_LightDefParserPlugin::_GetDiscoveryType(),
                UsdLux_LightDefParserPlugin::_GetSourceType(),
                /*uri=*/ "", 
                /*resolvedUri=*/ "");
        }
    }

    return result;
}

NDR_REGISTER_DISCOVERY_PLUGIN(UsdLux_DiscoveryPlugin);

PXR_NAMESPACE_CLOSE_SCOPE
