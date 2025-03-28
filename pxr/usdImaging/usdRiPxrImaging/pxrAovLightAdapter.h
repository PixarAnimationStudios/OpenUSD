//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_RI_PXR_IMAGING_PXR_AOV_LIGHT_ADAPTER_H
#define PXR_USD_IMAGING_USD_RI_PXR_IMAGING_PXR_AOV_LIGHT_ADAPTER_H

/// \file

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdRiPxrImaging/api.h"
#include "pxr/usdImaging/usdImaging/lightAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;

/// \class UsdRiPxrImagingAovLightAdapter
///
/// Adapter class for lights of type PxrAovLight.
///
class UsdRiPxrImagingAovLightAdapter : public UsdImagingLightAdapter
{
public:
    using BaseAdapter = UsdImagingLightAdapter;

    UsdRiPxrImagingAovLightAdapter()
        : UsdImagingLightAdapter()
    {}

    USDRIPXRIMAGING_API
    virtual ~UsdRiPxrImagingAovLightAdapter();

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDRIPXRIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDRIPXRIMAGING_API
    TfToken GetImagingSubprimType(UsdPrim const& prim,
                                  TfToken const& subprim) override;

    // ---------------------------------------------------------------------- //

    USDRIPXRIMAGING_API
    SdfPath Populate(
        UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const* instancerContext = NULL) override;

    USDRIPXRIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;
    
protected:
    void _RemovePrim(SdfPath const& cachePath,
                     UsdImagingIndexProxy* index) override final;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_RI_PXR_IMAGING_PXR_AOV_LIGHT_ADAPTER_H
