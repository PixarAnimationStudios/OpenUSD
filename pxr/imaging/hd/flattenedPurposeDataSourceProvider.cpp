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
    // If there is a purpose on this prim, use it.
    HdPurposeSchema inputPurpose(ctx.GetInputDataSource());
    if (inputPurpose.GetPurpose()) {
        return inputPurpose.GetContainer();
    }

    // If there is a parent purpose we can inherit, use that.
    HdPurposeSchema parentPurpose(ctx.GetFlattenedDataSourceFromParentPrim());
    if (HdBoolDataSourceHandle inheritableDs = parentPurpose.GetInheritable()) {
        if (inheritableDs->GetTypedValue(0.0f)) {
            // Parent purpose is inheritable.
            return parentPurpose.GetContainer();
        }
    }

    // If there is a fallback purpose, use that.
    if (HdTokenDataSourceHandle fallbackDs = inputPurpose.GetFallback()) {
        // Fallback purposes are not inheritable.
        return HdPurposeSchema::Builder()
            .SetPurpose(fallbackDs)
            .Build();
    }

    // Pass through the existing data untouched.
    return inputPurpose.GetContainer();
}

void
HdFlattenedPurposeDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    *locators = HdDataSourceLocatorSet::UniversalSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
