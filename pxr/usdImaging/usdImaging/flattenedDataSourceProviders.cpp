//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/flattenedDataSourceProviders.h"

#include "pxr/usdImaging/usdImaging/directMaterialBindingsSchema.h"
#include "pxr/usdImaging/usdImaging/flattenedGeomModelDataSourceProvider.h"
#include "pxr/usdImaging/usdImaging/flattenedDirectMaterialBindingsDataSourceProvider.h"
#include "pxr/usdImaging/usdImaging/geomModelSchema.h"
#include "pxr/usdImaging/usdImaging/modelSchema.h"

#include "pxr/imaging/hd/flattenedDataSourceProviders.h"
#include "pxr/imaging/hd/flattenedOverlayDataSourceProvider.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdContainerDataSourceHandle
UsdImagingFlattenedDataSourceProviders()
{
    using namespace HdMakeDataSourceContainingFlattenedDataSourceProvider;

    static HdContainerDataSourceHandle const result =
        HdOverlayContainerDataSource::New(
            {
            HdRetainedContainerDataSource::New(
                UsdImagingDirectMaterialBindingsSchema::GetSchemaToken(),
                Make<UsdImagingFlattenedDirectMaterialBindingsDataSourceProvider>()),

            HdRetainedContainerDataSource::New(
                UsdImagingGeomModelSchema::GetSchemaToken(),
                Make<UsdImagingFlattenedGeomModelDataSourceProvider>()),

            HdRetainedContainerDataSource::New(
                UsdImagingModelSchema::GetSchemaToken(),
                Make<HdFlattenedOverlayDataSourceProvider>()),

            HdFlattenedDataSourceProviders()
            });

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE

