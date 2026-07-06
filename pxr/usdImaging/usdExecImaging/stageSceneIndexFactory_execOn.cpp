//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/stageSceneIndexFactory.h"

#include "pxr/usdImaging/usdExecImaging/stageSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

// This implementation is used when building with PXR_BUILD_EXEC=ON.
UsdExecImagingStageSceneIndexInterfaceRefPtr
UsdExecImagingCreateStageSceneIndex()
{
    return UsdExecImaging_StageSceneIndex::New();
}

PXR_NAMESPACE_CLOSE_SCOPE
