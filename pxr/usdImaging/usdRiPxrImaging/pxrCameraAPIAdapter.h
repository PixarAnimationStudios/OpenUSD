//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_RI_PXR_IMAGING_PXR_CAMERA_API_ADAPTER_H
#define PXR_USD_IMAGING_USD_RI_PXR_IMAGING_PXR_CAMERA_API_ADAPTER_H

/// \file

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdRiPxrImaging/api.h"
#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdRiPxrImagingCameraAPIAdapter
///
/// Scene index support for PxrCameraAPI applied USD schema.
///
/// The attributes of the schema will be available under
/// HdCameraSchema::GetNamespacedProperties().
///
class UsdRiPxrImagingCameraAPIAdapter : public UsdImagingAPISchemaAdapter
{
public:

    using BaseAdapter = UsdImagingAPISchemaAdapter;

    USDRIPXRIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const &appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDRIPXRIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const &appliedInstanceName,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
