//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DOME_LIGHT_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_DOME_LIGHT_ADAPTER_H

/// \file usdImaging/domeLightAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/lightAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdPrim;

/// \class UsdImagingDomeLightAdapter
///
/// Adapter class for lights of type DomeLight
///
class UsdImagingDomeLightAdapter : public UsdImagingLightAdapter {
public:
    typedef UsdImagingLightAdapter BaseAdapter;

    UsdImagingDomeLightAdapter()
        : UsdImagingLightAdapter()
    {}

    USDIMAGING_API
    virtual ~UsdImagingDomeLightAdapter();

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDIMAGING_API
    TfToken GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim)
        override;

    USDIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
        UsdPrim const& prim, TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;

    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    SdfPath Populate(UsdPrim const& prim,
         UsdImagingIndexProxy* index,
         UsdImagingInstancerContext const* instancerContext = NULL) override;

    USDIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;
    
protected:
    USDIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) override final ;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DOME_LIGHT_ADAPTER_H
