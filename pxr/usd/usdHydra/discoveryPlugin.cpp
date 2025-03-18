//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdHydra/discoveryPlugin.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/resolverContextBinder.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

static std::string
_GetShaderResourcePath(char const * resourceName="")
{
    static PlugPluginPtr plugin = PLUG_THIS_PLUGIN;
    const std::string path = PlugFindPluginResource(plugin, 
            TfStringCatPaths("shaders", resourceName));

    TF_VERIFY(!path.empty(), "Could not find shader resource: %s\n", 
              resourceName);

    return path;
}

const SdrStringVec& 
UsdHydraDiscoveryPlugin::GetSearchURIs() const
{
    static const SdrStringVec searchPaths{_GetShaderResourcePath()};
    return searchPaths;
}

SdrShaderNodeDiscoveryResultVec
UsdHydraDiscoveryPlugin::DiscoverShaderNodes(const Context &context)
{
    SdrShaderNodeDiscoveryResultVec result;

    static std::string shaderDefsFile = _GetShaderResourcePath(
            "shaderDefs.usda");
    if (shaderDefsFile.empty())
        return result;

    auto resolverContext = ArGetResolver().CreateDefaultContextForAsset(
            shaderDefsFile);

    const UsdStageRefPtr stage = UsdStage::Open(shaderDefsFile, 
            resolverContext);

    if (!stage) {
        TF_RUNTIME_ERROR("Could not open file '%s' on a USD stage.", 
                         shaderDefsFile.c_str());
        return result;
    }

    ArResolverContextBinder binder(resolverContext);
    const TfToken discoveryType(ArGetResolver().GetExtension(
            shaderDefsFile));

    auto rootPrims = stage->GetPseudoRoot().GetChildren();
    for (const auto &shaderDef : rootPrims) {
        UsdShadeShader shader(shaderDef);
        if (!shader) {
            continue;
        }

        auto discoveryResults = UsdShadeShaderDefUtils::GetDiscoveryResults(
                shader, shaderDefsFile);

        result.insert(result.end(), discoveryResults.begin(), 
                      discoveryResults.end());

        if (discoveryResults.empty()) {
            TF_RUNTIME_ERROR("Found shader definition <%s> with no valid "
                "discovery results. This is likely because there are no "
                "resolvable info:sourceAsset values.", 
                shaderDef.GetPath().GetText());
        }
    }

    return result;
}

SDR_REGISTER_DISCOVERY_PLUGIN(UsdHydraDiscoveryPlugin);


PXR_NAMESPACE_CLOSE_SCOPE
