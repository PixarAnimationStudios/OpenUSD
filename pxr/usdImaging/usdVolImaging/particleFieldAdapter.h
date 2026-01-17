//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_VOL_IMAGING_PARTICLE_FIELD_ADAPTER_H
#define PXR_USD_IMAGING_USD_VOL_IMAGING_PARTICLE_FIELD_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdVolImaging/api.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingParticleFieldAdapter
///
/// Adapter class for prims of type ParticleField
class UsdImagingParticleFieldAdapter : public UsdImagingGprimAdapter {
  public:
    using BaseAdapter = UsdImagingGprimAdapter;

    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //

    USDVOLIMAGING_API
    SdfPath Populate(const UsdPrim& usdPrim, UsdImagingIndexProxy* index,
        const UsdImagingInstancerContext* instancerContext = nullptr) override;

    USDVOLIMAGING_API
    bool IsSupported(const UsdImagingIndexProxy* index) const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    USDVOLIMAGING_API
    void TrackVariability(const UsdPrim& usdPrim, const SdfPath& cachePath,
        HdDirtyBits* timeVaryingBits,
        const UsdImagingInstancerContext* i_instancerContext = nullptr)
        const override;

    /// Thread Safe.
    USDVOLIMAGING_API
    void UpdateForTime(UsdPrim const& prim, SdfPath const& cachePath,
        UsdTimeCode time, HdDirtyBits requestedBits,
        UsdImagingInstancerContext const* instancerContext = nullptr)
        const override;

    // ---------------------------------------------------------------------- //
    /// \name Change processing
    // ---------------------------------------------------------------------- //

    USDVOLIMAGING_API
    HdDirtyBits ProcessPropertyChange(const UsdPrim& usdPrim,
        const SdfPath& cachePath, const TfToken& propertyName) override;

    // ---------------------------------------------------------------------- //
    /// \name Data access
    // ---------------------------------------------------------------------- //

    USDVOLIMAGING_API
    VtValue Get(UsdPrim const& prim, SdfPath const& cachePath,
        TfToken const& key, UsdTimeCode time,
        VtIntArray* outIndices) const override;

    UsdImagingParticleFieldAdapter() : UsdImagingGprimAdapter() {}

    USDVOLIMAGING_API
    ~UsdImagingParticleFieldAdapter() override;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDVOLIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDVOLIMAGING_API
    TfToken GetImagingSubprimType(
        UsdPrim const& prim, TfToken const& subprim) override;

    USDVOLIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
        UsdPrim const& prim, TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals& stageGlobals) override;

    USDVOLIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
        UsdPrim const& prim, TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_PARTICLEFIELD_ADAPTER_H
