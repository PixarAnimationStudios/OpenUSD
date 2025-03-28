//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_INVALIDATABLE_CONTAINER_DATA_SOURCE_H
#define PXR_IMAGING_HD_INVALIDATABLE_CONTAINER_DATA_SOURCE_H

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdInvalidatableContainerDataSource
///
/// Base class for container data sources that cache data but provide
/// a locator to invalidate the cached data.
///
class HdInvalidatableContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE_ABSTRACT(HdInvalidatableContainerDataSource)

    virtual bool Invalidate(const HdDataSourceLocatorSet &dirtyLocators) = 0;
};

HD_DECLARE_DATASOURCE_HANDLES(HdInvalidatableContainerDataSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
