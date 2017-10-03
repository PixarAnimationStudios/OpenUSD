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

static std::string 
_GetTypeNames(const std::set<TfType>& types)
{
    std::vector<std::string> typeNames;
    typeNames.reserve(types.size());
    for (const auto& type : types) {
        typeNames.push_back(type.GetTypeName());
    }
    return TfStringJoin(typeNames);
}

static bool
_CompareTypeNames(const TfType& x, const TfType& y)
{
    return x.GetTypeName() < y.GetTypeName();
}

namespace
{
struct _ResolverHolder
{
    _ResolverHolder()
    {
        const TfType defaultResolverType = TfType::Find<ArDefaultResolver>();

        std::set<TfType> resolverTypes;
        if (TfGetEnvSetting(PXR_AR_DISABLE_PLUGIN_RESOLVER)) {
            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Plugin asset resolver disabled via "
                "PXR_AR_DISABLE_PLUGIN_RESOLVER. Using default resolver.\n");
        }
        else if (_preferredResolver->empty()) {
            PlugRegistry::GetAllDerivedTypes(
                TfType::Find<ArResolver>(), &resolverTypes);

            if (resolverTypes.size() == 1 &&
                *resolverTypes.begin() == defaultResolverType) {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): No plugin asset resolver found. "
                    "Using default resolver.\n");
            }
        }
        else {
            const TfType resolverType = 
                PlugRegistry::FindTypeByName(*_preferredResolver);
            if (!resolverType) {
                TF_WARN(
                    "ArGetResolver(): Preferred resolver %s not found. "
                    "Using default resolver.",
                    _preferredResolver->c_str());
            }
            else if (!resolverType.IsA<ArResolver>()) {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Preferred resolver %s does not derive "
                    "from ArResolver. Using default resolver.\n",
                    _preferredResolver->c_str());
            }
            else {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Using preferred resolver %s\n",
                    _preferredResolver->c_str());
                resolverTypes.insert(resolverType);
            }
        }

        // Remove the default resolver type to skip the plugin-based code below.
        resolverTypes.erase(defaultResolverType);

        // We expect to find only one plugin asset resolver implementation;
        // if we find more than one, we'll issue a warning and just pick one.
        // We want to make sure we always pick the same one to ensure 
        // stable behavior, but TfType's operator< is not stable across runs.
        // So, we pick the "minimum" element based on type name.
        if (!resolverTypes.empty()) {
            const std::set<TfType>::const_iterator typeIt = std::min_element(
                resolverTypes.begin(), resolverTypes.end(), 
                _CompareTypeNames);
            const TfType& resolverType = *typeIt;

            if (resolverTypes.size() > 1) {
                TF_DEBUG(AR_RESOLVER_INIT).Msg(
                    "ArGetResolver(): Found asset resolver types: [%s]\n",
                    _GetTypeNames(resolverTypes).c_str());

                TF_WARN(
                    "ArGetResolver(): Found multiple plugin asset "
                    "resolvers, using %s", 
                    resolverType.GetTypeName().c_str());
            }

            resolver.reset(_CreatePluginResolver(resolverType));
        }

        if (!resolver) {
            resolver.reset(new ArDefaultResolver);
        }
    }

    ArResolver* _CreatePluginResolver(const TfType& resolverType)
    {
        PlugPluginPtr plugin = PlugRegistry::GetInstance()
            .GetPluginForType(resolverType);
        if (!TF_VERIFY(
                plugin, "Failed to find plugin for %s", 
                resolverType.GetTypeName().c_str())
            || !TF_VERIFY(
                plugin->Load(), "Failed to load plugin %s for %s",
                plugin->GetName().c_str(),
                resolverType.GetTypeName().c_str())) {
            return nullptr;
        }

        ArResolverFactoryBase* factory =
            resolverType.GetFactory<ArResolverFactoryBase>();
        if (!factory) {
            TF_CODING_ERROR("Cannot manufacture plugin asset resolver");
            return nullptr;
        }

        ArResolver* tmpResolver = factory->New();
        if (!tmpResolver) {
            TF_CODING_ERROR("Failed to manufacture plugin asset resolver");
            return nullptr;
        }

        TF_DEBUG(AR_RESOLVER_INIT).Msg(
            "ArGetResolver(): Using asset resolver %s from plugin %s\n",
            resolverType.GetTypeName().c_str(),
            plugin->GetPath().c_str());

        return tmpResolver;
    }

    std::shared_ptr<ArResolver> resolver;
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

PXR_NAMESPACE_CLOSE_SCOPE
