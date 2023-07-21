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
#include "pxr/imaging/hd/flattenedXformDataSourceProvider.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceHandle
HdFlattenedXformDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    static const HdContainerDataSourceHandle identityXform =
        HdXformSchema::Builder()
            .SetMatrix(
                HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                    GfMatrix4d().SetIdentity()))
            .Build();

    HdXformSchema inputXform(ctx.GetInputDataSource());

    // If this xform is fully composed, early out.
    if (HdBoolDataSourceHandle const resetXformStack =
                    inputXform.GetResetXformStack()) {
        if (resetXformStack->GetTypedValue(0.0f)) {
            // Only use the local transform, or identity if no matrix was
            // provided...
            if (inputXform.GetMatrix()) {
                return inputXform.GetContainer();
            } else {
                return identityXform;
            }
        }
    }

    // Otherwise, we need to look at the parent value.
    HdXformSchema parentXform(ctx.GetFlattenedDataSourceFromParentPrim());

    // Attempt to compose the local matrix with the parent matrix;
    // note that since we got the parent matrix from _prims instead of
    // _inputDataSource, the parent matrix should be flattened already.
    // If either of the local or parent matrix are missing, they are
    // interpreted to be identity.
    HdMatrixDataSourceHandle const parentMatrixDataSource =
        parentXform.GetMatrix();
    HdMatrixDataSourceHandle const inputMatrixDataSource =
        inputXform.GetMatrix();

    if (inputMatrixDataSource && parentMatrixDataSource) {
        const GfMatrix4d parentMatrix =
            parentMatrixDataSource->GetTypedValue(0.0f);
        const GfMatrix4d inputMatrix =
            inputMatrixDataSource->GetTypedValue(0.0f);

        return HdXformSchema::Builder()
            .SetMatrix(
                    HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                        inputMatrix * parentMatrix))
            .Build();
    }
    if (inputMatrixDataSource) {
        return inputXform.GetContainer();
    }
    if (parentMatrixDataSource) {
        return parentXform.GetContainer();
    }

    return identityXform;
}

void
HdFlattenedXformDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    *locators = HdDataSourceLocatorSet::UniversalSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
