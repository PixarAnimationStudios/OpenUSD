//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_REPRESENTED_BY_ANCESTOR_PRIM_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_REPRESENTED_BY_ANCESTOR_PRIM_ADAPTER_H

/// \file usdImaging/representedByAncestorPrimAdapter.h

#include "pxr/usdImaging/usdImaging/primAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingRepresentedByAncestorPrimAdapter
///
/// Base class for all prim adapters which only want to indicate that an
/// ancestor prim is responsible for them.
/// 
/// Because Hydra 1.0 prim adapter methods are still present, their pure
/// virtuals must be implemented here (even if they will won't be called).
/// 
class UsdImagingRepresentedByAncestorPrimAdapter : public UsdImagingPrimAdapter
{
public:

    using BaseAdapter = UsdImagingPrimAdapter;

    UsdImagingRepresentedByAncestorPrimAdapter()
    : UsdImagingPrimAdapter()
    {}

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;
    PopulationMode GetPopulationMode() override;

    // ---------------------------------------------------------------------- //
    /// \name Overrides for Pure Virtual Legacy Methods
    // ---------------------------------------------------------------------- //

    SdfPath Populate(
        UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const*
            instancerContext = nullptr) override;

    void TrackVariability(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits* timeVaryingBits,
        UsdImagingInstancerContext const* instancerContext = nullptr)
            const override
    {}

    void UpdateForTime(
        UsdPrim const& prim,
        SdfPath const& cachePath, 
        UsdTimeCode time,
        HdDirtyBits requestedBits,
        UsdImagingInstancerContext const* instancerContext = nullptr)
            const override
    {}

    HdDirtyBits ProcessPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName) override;

    void MarkDirty(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits dirty,
        UsdImagingIndexProxy* index) override
    {}

protected:
    void _RemovePrim(
        SdfPath const& cachePath,
        UsdImagingIndexProxy* index) override
    {}

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_REPRESENTED_BY_ANCESTOR_PRIM_ADAPTER_H
