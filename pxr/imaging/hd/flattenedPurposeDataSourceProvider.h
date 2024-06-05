//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_FLATTENED_PURPOSE_DATA_SOURCE_PROVIDER_H
#define PXR_IMAGING_HD_FLATTENED_PURPOSE_DATA_SOURCE_PROVIDER_H

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/flattenedDataSourceProvider.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdFlattenedPurposeDataSourceProvider : public HdFlattenedDataSourceProvider
{
    HD_API
    HdContainerDataSourceHandle GetFlattenedDataSource(
        const Context&) const override;

    HD_API
    void ComputeDirtyLocatorsForDescendants(
        HdDataSourceLocatorSet * locators) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_FLATTENED_PURPOSE_DATA_SOURCE_PROVIDER_H
