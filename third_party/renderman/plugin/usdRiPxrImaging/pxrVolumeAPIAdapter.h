//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_PXR_VOLUME_API_ADAPTER_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_PXR_VOLUME_API_ADAPTER_H

/// \file

#include "usdRiPxrImaging/api.h"

#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdRiPxrImagingVolumeAPIAdapter : public UsdImagingAPISchemaAdapter
{
public:
    using BaseAdapter = UsdImagingAPISchemaAdapter;

    USDRIPXRIMAGING_API
    TfToken GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfToken const& appliedInstanceName) override;

    USDRIPXRIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfToken const& appliedInstanceName,
        const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDRIPXRIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfToken const& appliedInstanceName,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
