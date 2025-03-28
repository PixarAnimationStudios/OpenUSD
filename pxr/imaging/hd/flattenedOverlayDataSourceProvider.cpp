//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/flattenedOverlayDataSourceProvider.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceHandle
HdFlattenedOverlayDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    return
        HdOverlayContainerDataSource::OverlayedContainerDataSources(
            ctx.GetInputDataSource(),
            ctx.GetFlattenedDataSourceFromParentPrim());
}

void
HdFlattenedOverlayDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
}

PXR_NAMESPACE_CLOSE_SCOPE

