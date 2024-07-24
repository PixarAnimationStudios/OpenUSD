//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_GPRIM_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_GPRIM_ADAPTER_H

/// \file usdImaging/gprimAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/instanceablePrimAdapter.h"

#include "pxr/usd/usdGeom/xformCache.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdGeomGprim;

/// \class UsdImagingGprimAdapter
///
/// Delegate support for UsdGeomGrims.
///
/// This adapter is provided as a base class for all adapters that want basic
/// Gprim data support, such as visibility, doubleSided, extent, displayColor,
/// displayOpacity, purpose, and transform.
///
class UsdImagingGprimAdapter : public UsdImagingInstanceablePrimAdapter
{
public:
    using BaseAdapter = UsdImagingInstanceablePrimAdapter;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;
    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDIMAGING_API
    void TrackVariability(UsdPrim const& prim,
                          SdfPath const& cachePath,
                          HdDirtyBits* timeVaryingBits,
                          UsdImagingInstancerContext const* 
                              instancerContext = nullptr) const override;

    /// Thread Safe.
    USDIMAGING_API
    void UpdateForTime(UsdPrim const& prim,
                       SdfPath const& cachePath, 
                       UsdTimeCode time,
                       HdDirtyBits requestedBits,
                       UsdImagingInstancerContext const* 
                           instancerContext = nullptr) const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    HdDirtyBits ProcessPropertyChange(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              TfToken const& property) override;

    USDIMAGING_API
    void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkRefineLevelDirty(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkReprDirty(UsdPrim const& prim,
                               SdfPath const& cachePath,
                               UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkCullStyleDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkRenderTagDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkTransformDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkVisibilityDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkMaterialDirty(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdImagingIndexProxy* index) override;

    USDIMAGING_API
    void MarkCollectionsDirty(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingIndexProxy* index) override;

    // ---------------------------------------------------------------------- //
    /// \name Utility methods
    // ---------------------------------------------------------------------- //
    /// Give derived classes an opportunity to override how we get points for
    /// a prim. This is useful for implicit primitives.
    USDIMAGING_API
    virtual VtValue GetPoints(UsdPrim const& prim,
                              UsdTimeCode time) const;

    /// Returns color, Usd interpolation token, and optionally color indices for
    /// a given prim, taking into account surface shader colors and explicitly 
    /// authored color on the prim. If indices is not nullptr and the color 
    /// value has indices, color will be set to the unflattened color value and 
    /// indices set to the color value's indices.
    USDIMAGING_API
    static bool GetColor(UsdPrim const& prim, 
                         UsdTimeCode time,
                         TfToken *interpolation,
                         VtValue *color,
                         VtIntArray *indices);

    /// Returns opacity, Usd interpolation token, and optionally opacity indices
    /// for a given prim, taking into account surface shader opacity and 
    /// explicitly authored opacity on the prim. If indices is not nullptr and 
    /// the opacity value has indices, opacity will be set to the unflattened 
    /// opacity value and indices set to the opacity value's indices.
    USDIMAGING_API
    static bool GetOpacity(UsdPrim const& prim, 
                           UsdTimeCode time,
                           TfToken *interpolation,
                           VtValue *opacity,
                           VtIntArray *indices);

    // Helper function: add a given type of rprim, potentially with instancer
    // name mangling, and add any bound shader.
    USDIMAGING_API
    SdfPath _AddRprim(TfToken const& primType,
                      UsdPrim const& usdPrim,
                      UsdImagingIndexProxy* index,
                      SdfPath const& materialUsdPath,
                      UsdImagingInstancerContext const* instancerContext);

    /// Reads the extent from the given prim. If the extent is not authored,
    /// an empty GfRange3d is returned, the extent will not be computed.
    USDIMAGING_API
    GfRange3d GetExtent(UsdPrim const& prim, 
                        SdfPath const& cachePath, 
                        UsdTimeCode time) const override;

    /// Reads double-sided from the given prim. If not authored, returns false
    USDIMAGING_API
    bool GetDoubleSided(UsdPrim const& prim, 
                        SdfPath const& cachePath, 
                        UsdTimeCode time) const override;

    USDIMAGING_API
    SdfPath GetMaterialId(UsdPrim const& prim, 
                          SdfPath const& cachePath, 
                          UsdTimeCode time) const override;
    /// Gets the value of the parameter named key for the given prim (which
    /// has the given cache path) and given time. If outIndices is not nullptr 
    /// and the value has indices, it will return the unflattened value and set 
    /// outIndices to the value's associated indices.
    USDIMAGING_API
    VtValue Get(UsdPrim const& prim,
                SdfPath const& cachePath,
                TfToken const& key,
                UsdTimeCode time,
                VtIntArray *outIndices) const override;

    // For implicit prims such as capsules, cones, cylinders and planes, the
    // "spine" axis along or about which the surface is aligned may be
    // specified. This utility method returns a basis matrix that transforms
    // points generated using "Z" as the spine axis to the desired axis.
    USDIMAGING_API
    static GfMatrix4d GetImplicitBasis(TfToken const &spineAxis);

protected:

    USDIMAGING_API
    void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) override;

    // Give derived classes an opportunity to block GprimAdapter processing
    // of certain primvars.
    USDIMAGING_API
    virtual bool _IsBuiltinPrimvar(TfToken const& primvarName) const;

    // Utility for gathering the names of primvars used by the gprim's 
    // materials, used in primvar filtering.
    USDIMAGING_API 
    virtual TfTokenVector _CollectMaterialPrimvars(
        SdfPathVector const& materialUsdPaths, 
        UsdTimeCode time) const;

    /// Returns the primvar names known to be supported for the rprims 
    /// this adapter produces.  These primvar names are excepted from primvar
    /// filtering.
    USDIMAGING_API
    virtual TfTokenVector const& _GetRprimPrimvarNames() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_IMAGING_USD_IMAGING_GPRIM_ADAPTER_H
