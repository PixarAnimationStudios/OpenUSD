//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
