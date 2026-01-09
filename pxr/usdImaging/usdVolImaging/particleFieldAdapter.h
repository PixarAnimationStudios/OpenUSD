//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HDPARTICLEFIELD_PARTICLEFIELDADAPTER_H
#define HDPARTICLEFIELD_PARTICLEFIELDADAPTER_H

#include <pxr/pxr.h>
#include <pxr/usdImaging/usdImaging/api.h>
#include <pxr/usdImaging/usdImaging/gprimAdapter.h>
#include <pxr/usdImaging/usdImaging/primAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingPointsAdapter
///
/// Delegate support for UsdGeomPoints.
class UsdImaging_3DGaussianSplatAdapter : public UsdImagingGprimAdapter {
  public:
    using BaseAdapter = UsdImagingGprimAdapter;

    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //

    SdfPath Populate(const UsdPrim& usdPrim, UsdImagingIndexProxy* index,
                     const UsdImagingInstancerContext* instancerContext = nullptr) override;

    bool IsSupported(const UsdImagingIndexProxy* index) const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    void TrackVariability(const UsdPrim& usdPrim, const SdfPath& cachePath, HdDirtyBits* timeVaryingBits,
                          const UsdImagingInstancerContext* i_instancerContext = nullptr) const override;

    /// Thread Safe.
    void UpdateForTime(UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time, HdDirtyBits requestedBits,
                       UsdImagingInstancerContext const* instancerContext = nullptr) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change processing
    // ---------------------------------------------------------------------- //

    HdDirtyBits ProcessPropertyChange(const UsdPrim& usdPrim, const SdfPath& cachePath,
                                      const TfToken& propertyName) override;

    // ---------------------------------------------------------------------- //
    /// \name Data access
    // ---------------------------------------------------------------------- //

    VtValue Get(UsdPrim const& prim, SdfPath const& cachePath, TfToken const& key, UsdTimeCode time,
                VtIntArray* outIndices) const override;

    UsdImaging_3DGaussianSplatAdapter() : UsdImagingGprimAdapter() {}

    ~UsdImaging_3DGaussianSplatAdapter() override;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    TfToken GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim) override;

    HdContainerDataSourceHandle GetImagingSubprimData(UsdPrim const& prim, TfToken const& subprim,
                                                      const UsdImagingDataSourceStageGlobals& stageGlobals) override;

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(UsdPrim const& prim, TfToken const& subprim,
                                                    TfTokenVector const& properties,
                                                    UsdImagingPropertyInvalidationType invalidationType) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_PARTICLEFIELD_ADAPTER_H
