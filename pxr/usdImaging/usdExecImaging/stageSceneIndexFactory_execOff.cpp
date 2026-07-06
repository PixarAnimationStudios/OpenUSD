//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/stageSceneIndexFactory.h"

PXR_NAMESPACE_OPEN_SCOPE

// This implementation is used if built with PXR_BUILD_EXEC=OFF.
UsdExecImagingStageSceneIndexInterfaceRefPtr
UsdExecImagingCreateStageSceneIndex()
{
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
