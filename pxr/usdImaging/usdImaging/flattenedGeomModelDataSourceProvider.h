//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_USD_IMAGING_USD_IMAGING_FLATTENED_GEOM_MODEL_DATA_SOURCE_PROVIDER_H
#define PXR_USD_IMAGING_USD_IMAGING_FLATTENED_GEOM_MODEL_DATA_SOURCE_PROVIDER_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/flattenedDataSourceProvider.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingFlattenedGeomModelDataSourceProvider
                        : public HdFlattenedDataSourceProvider
{
    USDIMAGING_API
    HdContainerDataSourceHandle GetFlattenedDataSource(
        const Context&) const override;

    USDIMAGING_API
    void ComputeDirtyLocatorsForDescendants(
        HdDataSourceLocatorSet * locators) const override;

public:

    USDIMAGING_API
    virtual ~UsdImagingFlattenedGeomModelDataSourceProvider();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_FLATTENED_GEOM_MODEL_DATA_SOURCE_PROVIDER_H

