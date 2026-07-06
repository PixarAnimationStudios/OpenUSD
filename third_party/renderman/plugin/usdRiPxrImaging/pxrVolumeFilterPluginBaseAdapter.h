//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_PXR_VOLUME_FILTER_PLUGIN_BASE_ADAPTER_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_PXR_VOLUME_FILTER_PLUGIN_BASE_ADAPTER_H

/// \file usdRiPxrImaging/volumeFilterAdapter.h

#include "usdRiPxrImaging/api.h"

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;

/// \class UsdRiPxrImagingVolumeFilterAdapter
///
/// Adapter class for volume filters (PxrVolumeFilterPluginBase prims).
///
class UsdRiPxrImagingVolumeFilterAdapter : public UsdImagingPrimAdapter {
public:
    typedef UsdImagingPrimAdapter BaseAdapter;

    UsdRiPxrImagingVolumeFilterAdapter()
        : UsdImagingPrimAdapter()
    {}

    USDRIPXRIMAGING_API
    ~UsdRiPxrImagingVolumeFilterAdapter() override;

    USDRIPXRIMAGING_API
    SdfPath Populate(UsdPrim const& prim,
                     UsdImagingIndexProxy* index,
                     UsdImagingInstancerContext const*
                         instancerContext = NULL) override;

    USDRIPXRIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDRIPXRIMAGING_API
    void TrackVariability(UsdPrim const& prim,
                          SdfPath const& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          UsdImagingInstancerContext const*
                              instancerContext = NULL) const override;

    /// Thread Safe.
    USDRIPXRIMAGING_API
    void UpdateForTime(UsdPrim const& prim,
                       SdfPath const& cachePath,
                       UsdTimeCode time,
                       HdDirtyBits requestedBits,
                       UsdImagingInstancerContext const*
                           instancerContext = NULL) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing
    // ---------------------------------------------------------------------- //

    /// Returns a bit mask of attributes to be updated, or
    /// HdChangeTracker::AllDirty if the entire prim must be resynchronized.
    USDRIPXRIMAGING_API
    HdDirtyBits ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      TfToken const& propertyName) override;

    USDRIPXRIMAGING_API
    void MarkDirty(UsdPrim const& prim,
                   SdfPath const& cachePath,
                   HdDirtyBits dirty,
                   UsdImagingIndexProxy* index) override;

    USDRIPXRIMAGING_API
    void MarkTransformDirty(UsdPrim const& prim,
                            SdfPath const& cachePath,
                            UsdImagingIndexProxy* index) override;

    USDRIPXRIMAGING_API
    void MarkVisibilityDirty(UsdPrim const& prim,
                             SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) override;

    // ---------------------------------------------------------------------- //
    /// \name Utilities
    // ---------------------------------------------------------------------- //

    USDRIPXRIMAGING_API
    VtValue GetMaterialResource(UsdPrim const &prim,
                                SdfPath const& cachePath,
                                UsdTimeCode time) const override;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDRIPXRIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDRIPXRIMAGING_API
    TfToken GetImagingSubprimType(
            UsdPrim const& prim, TfToken const& subprim) override;

    USDRIPXRIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDRIPXRIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;

    USDRIPXRIMAGING_API
    PopulationMode GetPopulationMode() override;

    USDRIPXRIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprimFromDescendent(
            UsdPrim const& prim,
            UsdPrim const& descendentPrim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;

protected:
    USDRIPXRIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
                     UsdImagingIndexProxy* index) override;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_USD_RI_PXR_IMAGING_PXR_VOLUME_FILTER_PLUGIN_BASE_ADAPTER_H
