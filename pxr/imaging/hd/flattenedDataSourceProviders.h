//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_FLATTENED_DATA_SOURCE_PROVIDERS_H
#define PXR_IMAGING_HD_FLATTENED_DATA_SOURCE_PROVIDERS_H

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Contains all flattened data source providers implemented in hd.
///
/// Can be given as inputArgs to the HdFlatteningSceneIndex.
///
HD_API
HdContainerDataSourceHandle HdFlattenedDataSourceProviders();

PXR_NAMESPACE_CLOSE_SCOPE

#endif

