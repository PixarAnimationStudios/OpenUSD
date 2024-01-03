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

#include "pxr/imaging/hd/flattenedDataSourceProviders.h"

#include "pxr/imaging/hd/flattenedMaterialBindingsDataSourceProvider.h"
#include "pxr/imaging/hd/flattenedOverlayDataSourceProvider.h"
#include "pxr/imaging/hd/flattenedPrimvarsDataSourceProvider.h"
#include "pxr/imaging/hd/flattenedPurposeDataSourceProvider.h"
#include "pxr/imaging/hd/flattenedVisibilityDataSourceProvider.h"
#include "pxr/imaging/hd/flattenedXformDataSourceProvider.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceHandle
HdFlattenedDataSourceProviders()
{
    using namespace HdMakeDataSourceContainingFlattenedDataSourceProvider;

    static HdContainerDataSourceHandle const result =
        HdRetainedContainerDataSource::New(
            HdCoordSysBindingSchema::GetSchemaToken(),
            Make<HdFlattenedOverlayDataSourceProvider>(),
            HdMaterialBindingsSchema::GetSchemaToken(),
            Make<HdFlattenedMaterialBindingsDataSourceProvider>(),
            HdPrimvarsSchema::GetSchemaToken(),
            Make<HdFlattenedPrimvarsDataSourceProvider>(),
            HdPurposeSchema::GetSchemaToken(),
            Make<HdFlattenedPurposeDataSourceProvider>(),
            HdVisibilitySchema::GetSchemaToken(),
            Make<HdFlattenedVisibilityDataSourceProvider>(),
            HdXformSchema::GetSchemaToken(),
            Make<HdFlattenedXformDataSourceProvider>());

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
