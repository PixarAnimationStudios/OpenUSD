//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_STAGE_SCENE_INDEX_FACTORY_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_STAGE_SCENE_INDEX_FACTORY_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/api.h"
#include "pxr/usdImaging/usdExecImaging/stageSceneIndexInterface.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Constructs a concrete instance of UsdExecImagingStageSceneIndexInterface.
///
/// Construction of the scene index is hidden behind a factory function, because
/// usdExecImaging can be built with exec disabled. When pxr is built with
/// PXR_BUILD_EXEC=OFF, this method returns a null pointer.
///
USDEXECIMAGING_API
UsdExecImagingStageSceneIndexInterfaceRefPtr UsdExecImagingCreateStageSceneIndex();

PXR_NAMESPACE_CLOSE_SCOPE

#endif