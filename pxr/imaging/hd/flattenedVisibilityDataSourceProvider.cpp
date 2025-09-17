//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/flattenedVisibilityDataSourceProvider.h"

#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/imaging/hd/visibilitySchema.h"

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceHandle
HdFlattenedVisibilityDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    // Note: this resolves the visibility not according to USD spec.
    // That is, if a parent is invis'd, we should never be vis'd.

    HdVisibilitySchema inputVisibility(ctx.GetInputDataSource());
    if (inputVisibility.GetVisibility()) {
        return inputVisibility.GetContainer();
    }

    HdVisibilitySchema parentVisibility(
        ctx.GetFlattenedDataSourceFromParentPrim());
    if (parentVisibility.GetVisibility()) {
        return parentVisibility.GetContainer();
    }

    static const HdContainerDataSourceHandle identityVisibility =
        HdVisibilitySchema::Builder()
            .SetVisibility(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .Build();

    return identityVisibility;
}

void
HdFlattenedVisibilityDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    *locators = HdDataSourceLocatorSet::UniversalSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
