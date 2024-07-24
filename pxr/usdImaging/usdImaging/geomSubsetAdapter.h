//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_GEOM_SUBSET_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_GEOM_SUBSET_ADAPTER_H

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/types.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/base/tf/token.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingGeomSubsetAdapter
  : public UsdImagingPrimAdapter
{
public:
    using BaseAdapter = UsdImagingPrimAdapter;
    
    UsdImagingGeomSubsetAdapter()
      : BaseAdapter()
    { }
    
    USDIMAGING_API
    ~UsdImagingGeomSubsetAdapter() override;
    
    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    TfTokenVector GetImagingSubprims(const UsdPrim& prim) override;

    USDIMAGING_API 
    TfToken GetImagingSubprimType(
        const UsdPrim& prim,
        const TfToken& subprim) override;

    USDIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
        const UsdPrim& prim,
        const TfToken& subprim,
        const UsdImagingDataSourceStageGlobals& stageGlobals) override;

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
        const UsdPrim& prim,
        const TfToken& subprim,
        const TfTokenVector& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;

    // ---------------------------------------------------------------------- //
    /// \name Overrides for Pure Virtual Legacy Methods
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    SdfPath Populate(
        const UsdPrim& prim,
        UsdImagingIndexProxy* index,
        const UsdImagingInstancerContext* instancerCtx = nullptr) override;

    USDIMAGING_API
    void TrackVariability(
        const UsdPrim& prim,
        const SdfPath& cachePath,
        HdDirtyBits* timeVaryingBits,
        const UsdImagingInstancerContext* instancerCtx = nullptr) const override
    { }

    USDIMAGING_API
    void UpdateForTime(
        const UsdPrim& prim,
        const SdfPath& cachePath, 
        UsdTimeCode time,
        HdDirtyBits requestedBits,
        const UsdImagingInstancerContext* instancerCtx = nullptr) const override
    { }

    USDIMAGING_API
    HdDirtyBits ProcessPropertyChange(
        const UsdPrim& prim,
        const SdfPath& cachePath,
        const TfToken& propertyName) override;

    USDIMAGING_API
    void MarkDirty(
        const UsdPrim& prim,
        const SdfPath& cachePath,
        HdDirtyBits dirty,
        UsdImagingIndexProxy* index) override
    { }

protected:
    USDIMAGING_API
    void _RemovePrim(
        const SdfPath& cachePath,
        UsdImagingIndexProxy* index) override
    { }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_GEOM_SUBSET_ADAPTER_H
