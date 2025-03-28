//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_VOL_IMAGING_FIELD3D_ASSET_ADAPTER_H
#define PXR_USD_IMAGING_USD_VOL_IMAGING_FIELD3D_ASSET_ADAPTER_H

/// \file usdImaging/field3dAssetAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/fieldAdapter.h"
#include "pxr/usdImaging/usdVolImaging/api.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdPrim;

/// \class UsdImagingField3DAssetAdapter
///
/// Adapter class for fields of type Field3DAsset
///
class UsdImagingField3DAssetAdapter : public UsdImagingFieldAdapter {
public:
    using BaseAdapter = UsdImagingFieldAdapter;

    UsdImagingField3DAssetAdapter()
        : UsdImagingFieldAdapter()
    {}

    USDVOLIMAGING_API
    ~UsdImagingField3DAssetAdapter() override;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDVOLIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDVOLIMAGING_API
    TfToken GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim)
        override;

    USDVOLIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;

    // ---------------------------------------------------------------------- //
    /// \name Data access
    // ---------------------------------------------------------------------- //

    USDVOLIMAGING_API
    VtValue Get(UsdPrim const& prim,
                SdfPath const& cachePath,
                TfToken const& key,
                UsdTimeCode time,
                VtIntArray *outIndices) const override;

    USDVOLIMAGING_API
    TfToken GetPrimTypeToken() const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_VOL_IMAGING_FIELD3D_ASSET_ADAPTER_H
