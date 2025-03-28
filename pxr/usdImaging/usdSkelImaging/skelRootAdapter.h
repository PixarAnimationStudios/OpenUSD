//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_SKEL_ROOT_ADAPTER_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_SKEL_ROOT_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdSkelImaging/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingSkelRootAdapter
///
/// The SkelRoot adapter exists for two reasons:
/// (a) Registering the SkeletonAdapter to handle processing of any skinned
///     prim under a SkelRoot prim.
///     The UsdSkel schema requires that ANY skinned prim lives under a 
///     SkelRoot.
/// (b) Getting the skeleton that deforms each skinned prim, which is stored
///     in the SkeletonAdapter (the latter is stateful).
/// Both of these happen during Populate(..)
///
class UsdSkelImagingSkelRootAdapter : public UsdImagingPrimAdapter {
public:
    using BaseAdapter = UsdImagingPrimAdapter;

    UsdSkelImagingSkelRootAdapter()
        : BaseAdapter()
    {}

    USDSKELIMAGING_API
    virtual ~UsdSkelImagingSkelRootAdapter();

    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //
    USDSKELIMAGING_API
    SdfPath
    Populate(const UsdPrim& prim,
             UsdImagingIndexProxy* index,
             const UsdImagingInstancerContext*
                 instancerContext=nullptr) override;

    USDSKELIMAGING_API
    bool CanPopulateUsdInstance() const override { return true; }

    bool ShouldIgnoreNativeInstanceSubtrees() const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDSKELIMAGING_API
    void TrackVariability(const UsdPrim& prim,
                          const SdfPath& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          const UsdImagingInstancerContext* 
                             instancerContext = nullptr) const override;

    /// Thread Safe.
    USDSKELIMAGING_API
    void UpdateForTime(const UsdPrim& prim,
                       const SdfPath& cachePath, 
                       UsdTimeCode time,
                       HdDirtyBits requestedBits,
                       const UsdImagingInstancerContext*
                           instancerContext=nullptr) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing
    // ---------------------------------------------------------------------- //

    USDSKELIMAGING_API
    HdDirtyBits ProcessPropertyChange(const UsdPrim& prim,
                                      const SdfPath& cachePath,
                                      const TfToken& propertyName) override;

    USDSKELIMAGING_API
    void MarkDirty(const UsdPrim& prim,
                   const SdfPath& cachePath,
                   HdDirtyBits dirty,
                   UsdImagingIndexProxy* index) override;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDSKELIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const &prim) override;

    USDSKELIMAGING_API
    TfToken GetImagingSubprimType(
            UsdPrim const &prim,
            TfToken const &subprim) override;

    USDSKELIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDSKELIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;

    
protected:
    USDSKELIMAGING_API
    void _RemovePrim(const SdfPath& cachePath,
                     UsdImagingIndexProxy* index) override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_SKEL_IMAGING_SKEL_ROOT_ADAPTER_H
