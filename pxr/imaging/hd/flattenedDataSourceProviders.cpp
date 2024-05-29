//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/flattenedDataSourceProviders.h"

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
