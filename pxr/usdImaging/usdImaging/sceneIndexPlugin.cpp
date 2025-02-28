//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/sceneIndexPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdImagingSceneIndexPlugin>();
}

UsdImagingSceneIndexPlugin::~UsdImagingSceneIndexPlugin() = default;

UsdImagingSceneIndexPlugin::FactoryBase::~FactoryBase() = default;

PXR_NAMESPACE_CLOSE_SCOPE
