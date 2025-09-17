//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/flattenedDataSourceProviders.h"

#include "pxr/usdImaging/usdImaging/flattenedGeomModelDataSourceProvider.h"
#include "pxr/usdImaging/usdImaging/flattenedMaterialBindingsDataSourceProvider.h"
#include "pxr/usdImaging/usdImaging/geomModelSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingsSchema.h"
#include "pxr/usdImaging/usdImaging/modelSchema.h"
#include "pxr/usdImaging/usdImaging/sceneIndexPlugin.h"

#include "pxr/imaging/hd/flattenedDataSourceProviders.h"
#include "pxr/imaging/hd/flattenedOverlayDataSourceProvider.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

static
HdContainerDataSourceHandle
_UsdFlattenedDataSourceProviders()
{
    using namespace HdMakeDataSourceContainingFlattenedDataSourceProvider;

    return
        HdRetainedContainerDataSource::New(
            UsdImagingMaterialBindingsSchema::GetSchemaToken(),
            Make<UsdImagingFlattenedMaterialBindingsDataSourceProvider>(),

            UsdImagingGeomModelSchema::GetSchemaToken(),
            Make<UsdImagingFlattenedGeomModelDataSourceProvider>(),

            UsdImagingModelSchema::GetSchemaToken(),
            Make<HdFlattenedOverlayDataSourceProvider>());
}

static
HdContainerDataSourceHandle
_FlattenedDataSourceProviders()
{
    TRACE_FUNCTION();
    
    std::vector<HdContainerDataSourceHandle> result;

    // Usd-specific flattening
    result.push_back(_UsdFlattenedDataSourceProviders());

    // Flattening from UsdImaging scene index plugins.
    for (const UsdImagingSceneIndexPluginUniquePtr &sceneIndexPlugin :
             UsdImagingSceneIndexPlugin::GetAllSceneIndexPlugins()) {
        if (HdContainerDataSourceHandle ds =
                sceneIndexPlugin->FlattenedDataSourceProviders()) {
            result.push_back(std::move(ds));
        }
    }

    // Basic flattening from Hydra.
    result.push_back(HdFlattenedDataSourceProviders());

    return HdOverlayContainerDataSource::New(result.size(), result.data());
}


HdContainerDataSourceHandle
UsdImagingFlattenedDataSourceProviders()
{
    static HdContainerDataSourceHandle const result =
        _FlattenedDataSourceProviders();
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
