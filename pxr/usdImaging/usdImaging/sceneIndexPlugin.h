//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_SCENE_INDEX_PLUGIN_H
#define PXR_USD_IMAGING_USD_IMAGING_SCENE_INDEX_PLUGIN_H

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/type.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSceneIndexBase);
using UsdImagingSceneIndexPluginUniquePtr =
    std::unique_ptr<class UsdImagingSceneIndexPlugin>;

/// \class UsdImagingSceneIndexPlugin
///
/// A base class for scene index plugins that can insert filtering
/// scene indices into UsdImaging, see sceneIndices.cpp for details.
///
/// This is intended for UsdSkelImaging inserting scene indices implementing
/// the UsdSkel behaviors.
///
/// Usage:
///
/// class MyPlugin : public UsdImagingSceneIndexPlugin {
/// public:
///     HdSceneIndexBaseRefPtr
///     AppendSceneIndex(HdSceneIndexBaseRefPtr const &inputScene) override {
///         ...
///     }
/// };
///
/// TF_REGISTRY_FUNCTION(UsdImagingSceneIndexPlugin)
/// {
///    // Also add to plugInfo.json.
///    UsdImagingSceneIndexPlugin::Define<MyPlugin>();
/// }
///
class UsdImagingSceneIndexPlugin
{
public:
    /// Override by client. Similar to HdSceneIndexPlugin::AppendSceneIndex.
    virtual HdSceneIndexBaseRefPtr AppendSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene) = 0;

    USDIMAGING_API
    virtual ~UsdImagingSceneIndexPlugin();

    class FactoryBase : public TfType::FactoryBase
    {
    public:
        virtual UsdImagingSceneIndexPluginUniquePtr Create() = 0;

        USDIMAGING_API
        virtual ~FactoryBase();
    };

    template<typename T>
    class Factory : public FactoryBase
    {
    public:
        UsdImagingSceneIndexPluginUniquePtr Create() override {
            return std::make_unique<T>();
        }
    };

    /// Call within TF_REGISTRY_FUNCTION(UsdImagingSceneIndexPlugin) to ensure
    /// that UsdImaging can instantiate the client's subclass of
    /// UsdImagingSceneIndexPlugin.
    template<typename T>
    static
    void Define()
    {
        TfType::Define<T, TfType::Bases<UsdImagingSceneIndexPlugin>>()
            .template SetFactory<Factory<T>>();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
