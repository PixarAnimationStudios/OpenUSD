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
