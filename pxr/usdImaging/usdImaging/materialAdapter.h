//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_MATERIAL_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_MATERIAL_ADAPTER_H

/// \file usdImaging/materialAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/representedByAncestorPrimAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdMaterialNetworkMap;


/// \class UsdImagingMaterialAdapter
/// \brief Provides information that can be used to generate a material.
class UsdImagingMaterialAdapter : public UsdImagingPrimAdapter
{
public:
    using BaseAdapter = UsdImagingPrimAdapter;

    UsdImagingMaterialAdapter()
        : UsdImagingPrimAdapter()
    {}

    USDIMAGING_API
    ~UsdImagingMaterialAdapter() override;



    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const& prim) override;

    USDIMAGING_API
    TfToken GetImagingSubprimType(
            UsdPrim const& prim,
            TfToken const& subprim) override;

    USDIMAGING_API
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

    /// Returns RepresentsSelfAndDescendents to suppress population of child
    /// Shader prims and receive notice when their properties change/
    USDIMAGING_API
    PopulationMode GetPopulationMode() override;

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprimFromDescendent(
            UsdPrim const& prim,
            UsdPrim const& descendentPrim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;

    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    SdfPath Populate(UsdPrim const& prim,
                     UsdImagingIndexProxy* index,
                     UsdImagingInstancerContext const* instancerContext = NULL) override;

    USDIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDIMAGING_API
    void TrackVariability(UsdPrim const& prim,
                          SdfPath const& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          UsdImagingInstancerContext const* 
                              instancerContext = NULL) const override;


    /// Thread Safe.
    USDIMAGING_API
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
    USDIMAGING_API
    HdDirtyBits ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      TfToken const& propertyName) override;

    USDIMAGING_API
    void MarkDirty(UsdPrim const& prim,
                   SdfPath const& cachePath,
                   HdDirtyBits dirty,
                   UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkMaterialDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void ProcessPrimResync(SdfPath const& cachePath,
                           UsdImagingIndexProxy* index) override;

    // ---------------------------------------------------------------------- //
    /// \name Utilities 
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    VtValue GetMaterialResource(UsdPrim const &prim,
                                SdfPath const& cachePath, 
                                UsdTimeCode time) const override;

protected:
    USDIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
                     UsdImagingIndexProxy* index) final;
};

/// \class UsdImagingShaderAdapter
/// \brief Delegates invalidation responsibility of a Shader prim to an
/// ancestor Material prim
class UsdImagingShaderAdapter :
        public UsdImagingRepresentedByAncestorPrimAdapter
{
public:
    using BaseAdapter = UsdImagingRepresentedByAncestorPrimAdapter;

    UsdImagingShaderAdapter()
        : UsdImagingRepresentedByAncestorPrimAdapter() {}
};

/// \class UsdImagingNodeGraphAdapter
/// \brief Delegates invalidation responsibility of a Noge Graph prim to an
/// ancestor Material prim
class UsdImagingNodeGraphAdapter :
        public UsdImagingRepresentedByAncestorPrimAdapter
{
public:
    using BaseAdapter = UsdImagingRepresentedByAncestorPrimAdapter;

    UsdImagingNodeGraphAdapter()
        : UsdImagingRepresentedByAncestorPrimAdapter() {}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_MATERIAL_ADAPTER_H
