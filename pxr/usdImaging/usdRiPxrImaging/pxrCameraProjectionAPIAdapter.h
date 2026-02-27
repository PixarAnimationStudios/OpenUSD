//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_RI_PXR_IMAGING_PXR_CAMERA_PROJECTION_API_ADAPTER_H
#define PXR_USD_IMAGING_USD_RI_PXR_IMAGING_PXR_CAMERA_PROJECTION_API_ADAPTER_H

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/imaging/hd/dataSource.h"

#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/usdImaging/usdImaging/types.h"
#include "pxr/usdImaging/usdRiPxrImaging/api.h"
#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdRiPxrImagingCameraProjectionAPIAdapter
  : public UsdImagingAPISchemaAdapter
{
public:
    using BaseAdapter = UsdImagingAPISchemaAdapter;

    USDRIPXRIMAGING_API
    HdContainerDataSourceHandle
    GetImagingSubprimData(
        const UsdPrim& prim,
        const TfToken& subprim,
        const TfToken& appliedInstanceName,
        const UsdImagingDataSourceStageGlobals& stageGlobals) override;

    USDRIPXRIMAGING_API
    HdDataSourceLocatorSet
    InvalidateImagingSubprim(
        const UsdPrim& prim,
        const TfToken& subprim,
        const TfToken& appliedInstanceName,
        const TfTokenVector& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;
};

// For PxrCameraProjectionAPI schema applied on UsdGeomCamera that
// allow specification of projection, if the schema expects relationships, 
// perform the following actions for each value of the environment variable.
// true : Produce a warning message if connections are used.
// false : Disallow the use of attribute connections for the purposes of
//         connecting projection.
USDIMAGING_API
extern TfEnvSetting<bool>
        LEGACY_PXR_CAMERA_PROJECTION_TERMINAL_ALLOWED_AND_WARN;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
