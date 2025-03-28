//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_PROC_IMAGING_GENERATIVE_PROCEDURAL_ADAPTER_H
#define PXR_USD_IMAGING_USD_PROC_IMAGING_GENERATIVE_PROCEDURAL_ADAPTER_H

#include "pxr/usdImaging/usdProcImaging/api.h"
#include "pxr/usdImaging/usdImaging/instanceablePrimAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdProcImagingGenerativeProceduralAdapter
    : public UsdImagingInstanceablePrimAdapter
{
public:
    using BaseAdapter = UsdImagingInstanceablePrimAdapter;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDPROCIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDPROCIMAGING_API
    TfToken GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim)
        override;

    USDPROCIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDPROCIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;

    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //


    USDPROCIMAGING_API
    SdfPath Populate(
        UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const*
            instancerContext = nullptr) override;

    USDPROCIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;

    USDPROCIMAGING_API
    void UpdateForTime(
        UsdPrim const& prim,
        SdfPath const& cachePath, 
        UsdTimeCode time,
        HdDirtyBits requestedBits,
        UsdImagingInstancerContext const* 
            instancerContext = nullptr) const override;

    USDPROCIMAGING_API
    VtValue Get(UsdPrim const& prim,
                SdfPath const& cachePath,
                TfToken const& key,
                UsdTimeCode time,
                VtIntArray *outIndices) const override;

    USDPROCIMAGING_API
    HdDirtyBits ProcessPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName) override;


    USDPROCIMAGING_API
    void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) override;

    USDPROCIMAGING_API
    void MarkTransformDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDPROCIMAGING_API
    void MarkVisibilityDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index) override;

    USDPROCIMAGING_API
    void TrackVariability(UsdPrim const& prim,
                          SdfPath const& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          UsdImagingInstancerContext const* 
                              instancerContext = nullptr) const override;

protected:
    USDPROCIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
        UsdImagingIndexProxy* index) override;

private:
    TfToken _GetHydraPrimType(UsdPrim const& prim);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_PROC_IMAGING_GENERATIVE_PROCEDURAL_ADAPTER_H
