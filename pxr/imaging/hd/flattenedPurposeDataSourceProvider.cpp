//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/flattenedPurposeDataSourceProvider.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceHandle
HdFlattenedPurposeDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    HdPurposeSchema inputPurpose(ctx.GetInputDataSource());
    if (inputPurpose.GetPurpose()) {
        return inputPurpose.GetContainer();
    }

    HdPurposeSchema parentPurpose(ctx.GetFlattenedDataSourceFromParentPrim());
    if (parentPurpose.GetPurpose()) {
        return parentPurpose.GetContainer();
    }

    static const HdContainerDataSourceHandle identityPurpose =
        HdPurposeSchema::Builder()
            .SetPurpose(
                HdRetainedTypedSampledDataSource<TfToken>::New(
                    HdRenderTagTokens->geometry))
            .Build();

    return identityPurpose;
}

void
HdFlattenedPurposeDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    *locators = HdDataSourceLocatorSet::UniversalSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
