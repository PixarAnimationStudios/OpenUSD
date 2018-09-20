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
#include "pxr/usdImaging/usdShaders/discoveryPlugin.h"

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

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    /* node metadata */
    (family)
    (version)

    ((infoNamespace, "info:"))
    ((baseSourceAsset, ":sourceAsset"))
);

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

const NdrStringVec& 
UsdShadersDiscoveryPlugin::GetSearchURIs() const
{
    static const NdrStringVec searchPaths{_GetShaderResourcePath()};
    return searchPaths;
}

NdrNodeDiscoveryResultVec
UsdShadersDiscoveryPlugin::DiscoverNodes(const Context &context)
{
    NdrNodeDiscoveryResultVec result;

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
    auto rootPrims = stage->GetPseudoRoot().GetChildren();
    for (const auto &shaderDef : rootPrims) {
        UsdShadeShader shader(shaderDef);
        if (!shader) {
            continue;
        }

        auto discoveryResults = UsdShadeShaderDefUtils::GetNodeDiscoveryResults(
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

NDR_REGISTER_DISCOVERY_PLUGIN(UsdShadersDiscoveryPlugin);


PXR_NAMESPACE_CLOSE_SCOPE
