//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_SPHERE_LIGHT_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_SPHERE_LIGHT_ADAPTER_H

/// \file usdImaging/sphereLightAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/lightAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdPrim;

/// \class UsdImagingSphereLightAdapter
///
/// Adapter class for lights of type SphereLight
///
class UsdImagingSphereLightAdapter : public UsdImagingLightAdapter {
public:
    typedef UsdImagingLightAdapter BaseAdapter;

    UsdImagingSphereLightAdapter()
        : UsdImagingLightAdapter()
    {}

    USDIMAGING_API
    virtual ~UsdImagingSphereLightAdapter();


    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDIMAGING_API
    TfToken GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim)
        override;

    // ---------------------------------------------------------------------- //


    USDIMAGING_API
    SdfPath Populate(UsdPrim const& prim,
         UsdImagingIndexProxy* index,
         UsdImagingInstancerContext const* instancerContext = NULL) override;

    USDIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;

protected:
    void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) override final;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_SPHERE_LIGHT_ADAPTER_H
