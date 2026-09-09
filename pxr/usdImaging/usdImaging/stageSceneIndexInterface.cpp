//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/stageSceneIndexInterface.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdImagingStageSceneIndexInterface>();
}

UsdImagingStageSceneIndexInterface::
~UsdImagingStageSceneIndexInterface() = default;

PXR_NAMESPACE_CLOSE_SCOPE
