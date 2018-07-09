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

#include "pxr/pxr.h"
#include "pxr/usd/ar/debugCodes.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ArResolver>();
}

TF_DEFINE_ENV_SETTING(
    PXR_AR_DISABLE_PLUGIN_RESOLVER, false,
    "Disables plugin resolver implementation, falling back to default "
    "supplied by Ar.");

static TfStaticData<std::string> _preferredResolver;

void
ArSetPreferredResolver(const std::string& resolverTypeName)
{
    *_preferredResolver = resolverTypeName;
}

// ------------------------------------------------------------

ArResolver::ArResolver()
{
}

ArResolver::~ArResolver()
{
}

// ------------------------------------------------------------

namespace
{
std::string 
_GetTypeNames(const std::vector<TfType>& types)
{
    std::vector<std::string> typeNames;
    typeNames.reserve(types.size());
    for (const auto& type : types) {
        typeNames.push_back(type.GetTypeName());
    }
    return TfStringJoin(typeNames, ", ");
}

// Global stack of resolvers being constructed used by
// _CreateResolver / ArCreateResolver and ArGetAvailableResolvers.
// These functions are documented to be non-thread-safe.
static TfStaticData<std::vector<TfType>> _resolverStack;

std::vector<TfType> 
_GetAvailableResolvers()
{
    const TfType defaultResolverType = TfType::Find<ArDefaultResolver>();

    std::vector<TfType> sortedResolverTypes;

    if (!TfGetEnvSetting(PXR_AR_DISABLE_PLUGIN_RESOLVER)) {
        std::set<TfType> resolverTypes;
        PlugRegistry::GetAllDerivedTypes(
            TfType::Find<ArResolver>(), &resolverTypes);

        // Remove the default resolver so that we only process plugin types.
        // We'll add the default resolver back later.
        resolverTypes.erase(defaultResolverType);

        // Remove all resolvers that are currently under construction.
        for (const TfType& usedResolver : *_resolverStack) {
            resolverTypes.erase(usedResolver);
        }

        // We typically expect to find only one plugin asset resolver,
        // but if there's more than one we want to ensure this list is in a 
        // consistent order to ensure stable behavior. TfType's operator< 
        // is not stable across runs, so we sort based on typename instead.
        sortedResolverTypes.insert(
            sortedResolverTypes.end(), 
            resolverTypes.begin(), resolverTypes.end());
        std::sort(
            sortedResolverTypes.begin(), sortedResolverTypes.end(),
            [](const TfType& x, const TfType& y) {
                return x.GetTypeName() < y.GetTypeName();
            });
    }

    // The default resolver is always the last resolver to be considered.
    sortedResolverTypes.push_back(defaultResolverType);
    return sortedResolverTypes;
}

std::unique_ptr<ArResolver>
_CreateResolver(const TfType& resolverType, std::string* debugMsg = nullptr)
{
    _resolverStack->push_back(resolverType);
    TfScoped<> popResolverStack([]() { _resolverStack->pop_back(); });

    const TfType defaultResolverType = TfType::Find<ArDefaultResolver>();
    std::unique_ptr<ArResolver> tmpResolver;
    if (!resolverType) {
        TF_CODING_ERROR("Invalid resolver type");
    }
    else if (!resolverType.IsA<ArResolver>()) {
        TF_CODING_ERROR(
            "Given type %s does not derive from ArResolver", 
            resolverType.GetTypeName().c_str());
    }
    else if (resolverType != defaultResolverType) {
        PlugPluginPtr plugin = PlugRegistry::GetInstance()
            .GetPluginForType(resolverType);
        if (!plugin) {
            TF_CODING_ERROR("Failed to find plugin for %s", 
                resolverType.GetTypeName().c_str());
        }
        else if (!plugin->Load()) {
            TF_CODING_ERROR("Failed to load plugin %s for %s",
                plugin->GetName().c_str(),
                resolverType.GetTypeName().c_str());
        }
        else {
            Ar_ResolverFactoryBase* factory =
                resolverType.GetFactory<Ar_ResolverFactoryBase>();
            if (factory) {
                tmpResolver.reset(factory->New());
            }

            if (!tmpResolver) {
                TF_CODING_ERROR(
                    "Failed to manufacture asset resolver %s from plugin %s", 
                    resolverType.GetTypeName().c_str(), 
                    plugin->GetName().c_str());
            }
            else if (debugMsg) {
                *debugMsg = TfStringPrintf(
                    "Using asset resolver %s from plugin %s",
                    resolverType.GetTypeName().c_str(),
                    plugin->GetPath().c_str());
            }
        }
    }

    if (!tmpResolver) {
        if (debugMsg) {
            *debugMsg = TfStringPrintf("Using default asset resolver %s",
                defaultResolverType.GetTypeName().c_str());
        }
        tmpResolver.reset(new ArDefaultResolver);
    }
    
    return tmpResolver;
}

struct _ResolverHolder
{
    _ResolverHolder()
    {
        const TfType defaultResolverType = TfType::Find<ArDefaultResolver>();

        std::vector<TfType> resolverTypes;
        if (TfGetEnvSetting(PXR_AR_DISABLE_PLUGIN_RESOLVER)) {
            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Plugin asset resolver disabled via "
                "PXR_AR_DISABLE_PLUGIN_RESOLVER.\n");
        }
        else if (!_preferredResolver->empty()) {
            const TfType resolverType = 
                PlugRegistry::FindTypeByName(*_preferredResolver);
            if (!resolverType) {
                TF_WARN(
                    "ArGetResolver(): Preferred resolver %s not found. "
                    "Using default resolver.",
                    _preferredResolver->c_str());
                resolverTypes.push_back(defaultResolverType);
            }
            else if (!resolverType.IsA<ArResolver>()) {
                TF_WARN(
                    "ArGetResolver(): Preferred resolver %s does not derive "
                    "from ArResolver. Using default resolver.\n",
                    _preferredResolver->c_str());
                resolverTypes.push_back(defaultResolverType);
            }
            else {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Using preferred resolver %s\n",
                    _preferredResolver->c_str());
                resolverTypes.push_back(resolverType);
            }
        }

        if (resolverTypes.empty()) {
            resolverTypes = _GetAvailableResolvers();

            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Found asset resolver types: [%s]\n",
                _GetTypeNames(resolverTypes).c_str());
        }

        std::string debugMsg;

        // resolverTypes should never be empty -- _GetAvailableResolvers
        // should always return at least the default resolver. Because of this,
        // if there's more than 2 elements in resolverTypes, there must have 
        // been more than one resolver from an external plugin.
        if (TF_VERIFY(!resolverTypes.empty())) {
            const TfType& resolverType = resolverTypes.front();
            if (resolverTypes.size() > 2) {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Found multiple plugin asset "
                    "resolvers, using %s\n", 
                    resolverType.GetTypeName().c_str());
            }

            resolver = _CreateResolver(resolverType, &debugMsg);
        }

        if (!resolver) {
            resolver = _CreateResolver(defaultResolverType, &debugMsg);
        }

        TF_DEBUG(AR_RESOLVER_INIT).Msg(
            "ArGetResolver(): %s\n", debugMsg.c_str());
    }

    std::unique_ptr<ArResolver> resolver;
};
} // end anonymous namespace

ArResolver& 
ArGetResolver()
{
    // If other threads enter this function while another thread is 
    // constructing the holder, it's guaranteed that those threads
    // will wait until the holder is constructed.
    static _ResolverHolder holder;
    return *holder.resolver;
}

std::vector<TfType>
ArGetAvailableResolvers()
{
    return _GetAvailableResolvers();
}

std::unique_ptr<ArResolver>
ArCreateResolver(const TfType& resolverType)
{
    return _CreateResolver(resolverType);
}

PXR_NAMESPACE_CLOSE_SCOPE
