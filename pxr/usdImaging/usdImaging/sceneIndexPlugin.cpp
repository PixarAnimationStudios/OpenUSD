//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/sceneIndexPlugin.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdImagingSceneIndexPlugin>();
}

UsdImagingSceneIndexPlugin::~UsdImagingSceneIndexPlugin() = default;

HdContainerDataSourceHandle
UsdImagingSceneIndexPlugin::FlattenedDataSourceProviders()
{
    return nullptr;
}

TfTokenVector
UsdImagingSceneIndexPlugin::InstanceDataSourceNames()
{
    return {};
}

std::vector<UsdImagingSceneIndexPluginUniquePtr>
UsdImagingSceneIndexPlugin::GetAllSceneIndexPlugins()
{
    TRACE_FUNCTION();

    std::vector<UsdImagingSceneIndexPluginUniquePtr> result;

    PlugRegistry &plugRegistry = PlugRegistry::GetInstance();

    std::set<TfType> pluginTypes;
    PlugRegistry::GetAllDerivedTypes(
        TfType::Find<UsdImagingSceneIndexPlugin>(), &pluginTypes);

    for (const TfType &pluginType : pluginTypes) {
        PlugPluginPtr const plugin = plugRegistry.GetPluginForType(pluginType);
        if (!plugin) {
            TF_CODING_ERROR(
                "Could not get plugin for type %s.",
                pluginType.GetTypeName().c_str());
            continue;
        }
        if (!plugin->Load()) {
            TF_CODING_ERROR(
                "Could not load plugin %s.",
                plugin->GetName().c_str());
            continue;
        }

        UsdImagingSceneIndexPlugin::FactoryBase * const factory =
            pluginType.GetFactory<UsdImagingSceneIndexPlugin::FactoryBase>();
        if (!factory) {
            TF_CODING_ERROR(
                "No factory for UsdImagingSceneIndexPlugin %s.",
                plugin->GetName().c_str());
            continue;
        }
        UsdImagingSceneIndexPluginUniquePtr sceneIndexPlugin =
            factory->Create();
        if (!sceneIndexPlugin) {
            TF_CODING_ERROR(
                "Could not create UsdImagingSceneIndexPlugin %s.",
                plugin->GetName().c_str());
            continue;
        }

        result.push_back(std::move(sceneIndexPlugin));
    }
    return result;
}

UsdImagingSceneIndexPlugin::FactoryBase::~FactoryBase() = default;

PXR_NAMESPACE_CLOSE_SCOPE
