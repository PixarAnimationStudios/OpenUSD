//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_FLATTENED_DATA_SOURCE_PROVIDERS_H
#define PXR_USD_IMAGING_USD_IMAGING_FLATTENED_DATA_SOURCE_PROVIDERS_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Contains all HdFlattenedDataSourceProviders needed for flattening
/// the output of UsdImagingStageSceneIndex.
///
/// Can be given as inputArgs to HdFlatteningSceneIndex.
///
USDIMAGING_API
HdContainerDataSourceHandle
UsdImagingFlattenedDataSourceProviders();

PXR_NAMESPACE_CLOSE_SCOPE

#endif
