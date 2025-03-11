//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_SCENE_INDEX_PRIM_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_SCENE_INDEX_PRIM_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingSceneIndexPrimAdapter
///
/// The base class prim adapter for scene index support. Note that while the
/// plugin system uses UsdImagingPrimAdapter, that class is overloaded with
/// functionality for both UsdImagingStageSceneIndex and UsdImagingDelegate,
/// and (e.g.) has a bunch of pure virtual functions that are only relevant
/// to UsdImagingDelegate.
///
/// Pure scene index prim types should consider this the base class, and only
/// use API defined here.
///
class UsdImagingSceneIndexPrimAdapter : public UsdImagingPrimAdapter
{
public:
    using BaseAdapter = UsdImagingPrimAdapter;

    USDIMAGING_API
    UsdImagingSceneIndexPrimAdapter();

    USDIMAGING_API
    ~UsdImagingSceneIndexPrimAdapter() override;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    TfTokenVector GetImagingSubprims(UsdPrim const &prim) override = 0;

    TfToken GetImagingSubprimType(
        UsdPrim const &prim, TfToken const& subprim) override = 0;

    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override = 0;

    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override = 0;

    /*
     * A simple prim could implement this as:
     * GetImagingSubprims() = { "" }
     * GetImagingSubprimType("") = "Sphere"
     * GetImagingSubprimData("") = SphereDataSource(prim, globals)
     * ... while a more complicated prim could implement this as:
     * GetImagingSubprims() = { "", "guide" }
     * GetImagingSubprimType("") = "Light"
     * GetImagingSubprimType("guide") = "Mesh"
     * ... etc.  Typically the main prim is named "" (empty token).
     */

    // ---------------------------------------------------------------------- //
    /// \name Legacy
    ///
    /// Implementation of deprecated pure-virtuals; these functions
    /// don't participate in the stage scene index.
    // ---------------------------------------------------------------------- //
    
    USDIMAGING_API
    SdfPath Populate(UsdPrim const& prim,
                     UsdImagingIndexProxy* index,
                     UsdImagingInstancerContext const*
                         instancerContext = nullptr) override final;

    USDIMAGING_API
    void TrackVariability(UsdPrim const& prim,
                          SdfPath const& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          UsdImagingInstancerContext const* 
                              instancerContext = nullptr) const override final;

    USDIMAGING_API
    void UpdateForTime(UsdPrim const& prim,
                       SdfPath const& cachePath, 
                       UsdTimeCode time,
                       HdDirtyBits requestedBits,
                       UsdImagingInstancerContext const* 
                           instancerContext = nullptr) const override final;

    USDIMAGING_API
    HdDirtyBits ProcessPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName) override final;

    USDIMAGING_API
    void MarkDirty(UsdPrim const& prim,
                   SdfPath const& cachePath,
                   HdDirtyBits dirty,
                   UsdImagingIndexProxy* index) override final;

    USDIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
                     UsdImagingIndexProxy* index) override final;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_PRIM_DATASOURCE_ADAPTER_H
