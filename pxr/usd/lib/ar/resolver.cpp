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
// XXX: 
// We need to include Python.h at the top of this file because one
// of the headers below (likely tf/type.h) includes Python.h, but 
// Python.h needs to come at the top of the file or else we run
// into compilation errors. This should be removed when we fix
// the Python include issues throughout the codebase.
#include <Python.h>

#include "pxr/usd/ar/debugCodes.h"
#include "pxr/usd/ar/defaultResolver.h"
#include "pxr/usd/ar/defineResolver.h"
#include "pxr/usd/ar/resolver.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <string>
#include <vector>

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ArResolver>();
}

TF_DEFINE_ENV_SETTING(
    PXR_AR_DISABLE_PLUGIN_RESOLVER, false,
    "Disables plugin resolver implementation, falling back to default "
    "supplied by Ar.");

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
    BOOST_FOREACH(const TfType& type, types) {
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
        : resolver(new Ar_DefaultResolver)
    {
        if (TfGetEnvSetting(PXR_AR_DISABLE_PLUGIN_RESOLVER)) {
            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): Plugin asset resolver disabled via "
                "PXR_AR_DISABLE_PLUGIN_RESOLVER. Using default resolver.\n");
            return;
        }

        std::set<TfType> resolverTypes;
        PlugRegistry::GetAllDerivedTypes(
            TfType::Find<ArResolver>(), &resolverTypes);

        if (resolverTypes.empty()) {
            TF_DEBUG(AR_RESOLVER_INIT).Msg(
                "ArGetResolver(): No plugin asset resolver found. "
                "Using default resolver.\n");
            return;
        }

        // We expect to find only one plugin asset resolver implementation;
        // if we find more than one, we'll issue a warning and just pick one.
        // We want to make sure we always pick the same one to ensure 
        // stable behavior, but TfType's operator< is not stable across runs.
        // So, we pick the "minimum" element based on type name.
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

        PlugPluginPtr plugin = PlugRegistry::GetInstance()
            .GetPluginForType(resolverType);
        if (not TF_VERIFY(
                plugin, 
                "Failed to find plugin for %s", 
                resolverType.GetTypeName().c_str())
            or not TF_VERIFY(
                plugin->Load(), 
                "Failed to load plugin %s for %s",
                plugin->GetName().c_str(),
                resolverType.GetTypeName().c_str())) {
            return;
        }

        ArResolverFactoryBase* factory =
            resolverType.GetFactory<ArResolverFactoryBase>();
        if (not factory) {
            TF_CODING_ERROR("Cannot manufacture plugin asset resolver");
            return;
        }

        ArResolver* tmpResolver = factory->New();
        if (not tmpResolver) {
            TF_CODING_ERROR("Failed to manufacture plugin asset resolver");
            return;
        }

        resolver.reset(tmpResolver);
        TF_VERIFY(resolver);

        TF_DEBUG(AR_RESOLVER_INIT).Msg(
            "ArGetResolver(): Using asset resolver %s from plugin %s\n",
            resolverType.GetTypeName().c_str(),
            plugin->GetPath().c_str());

    }

    boost::shared_ptr<ArResolver> resolver;
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
