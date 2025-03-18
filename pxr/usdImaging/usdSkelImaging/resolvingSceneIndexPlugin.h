//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_RESOLVING_SCENE_INDEX_PLUGIN_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_RESOLVING_SCENE_INDEX_PLUGIN_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/usdImaging/usdImaging/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class UsdSkelImagingResolvingSceneIndexPlugin
///
/// Registers scene indices to resolve the Skeleton prim and points-based prim
/// skinned by a Skeleton prim.
///
class UsdSkelImagingResolvingSceneIndexPlugin
    : public UsdImagingSceneIndexPlugin
{
public:
    USDSKELIMAGING_API
    HdSceneIndexBaseRefPtr AppendSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene) override;

    USDSKELIMAGING_API
    HdContainerDataSourceHandle FlattenedDataSourceProviders() override;

    USDSKELIMAGING_API
    TfTokenVector InstanceDataSourceNames() override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
