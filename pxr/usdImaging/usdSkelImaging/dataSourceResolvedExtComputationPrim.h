//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_RESOLVED_EXT_COMPUTATION_PRIM_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_RESOLVED_EXT_COMPUTATION_PRIM_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

using UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle =
    std::shared_ptr<class UsdSkelImagingDataSourceResolvedPointsBasedPrim>;

/// Returns a data source for an ext computation prim of a skinned prim.
///
/// Used by the points resolving scene index. That scene index adds the ext
/// computations as children of the skinned prim with name \a computationName.
///
USDSKELIMAGING_API
HdContainerDataSourceHandle
UsdSkelImagingDataSourceResolvedExtComputationPrim(
    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle resolvedPrimSource,
    const TfToken &computationName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
