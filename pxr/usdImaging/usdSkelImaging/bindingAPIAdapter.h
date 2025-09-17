//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_BINDING_API_ADAPTER_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_BINDING_API_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdSkelImagingBindingAPIAdapter
///
/// API Schema adapter for UsdSkel's SkelBindingAPI.
///
class UsdSkelImagingBindingAPIAdapter : public UsdImagingAPISchemaAdapter
{
public:
    using BaseAdapter = UsdImagingAPISchemaAdapter;

    USDSKELIMAGING_API
    UsdSkelImagingBindingAPIAdapter();

    USDSKELIMAGING_API
    ~UsdSkelImagingBindingAPIAdapter() override;

    USDSKELIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDSKELIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
