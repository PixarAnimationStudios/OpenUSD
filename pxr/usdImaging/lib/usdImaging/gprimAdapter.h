//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USDIMAGING_GPRIM_ADAPTER_H
#define USDIMAGING_GPRIM_ADAPTER_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/usd/usdGeom/xformCache.h"

class UsdGeomGprim;

/// \class UsdImagingGprimAdapter
///
/// Delegate support for UsdGeomGrims.
///
/// This adapter is provided as a base class for all adapters that want basic
/// Gprim data support, such as visibility, doubleSided, extent, displayColor,
/// purpose, and transform.
///
class UsdImagingGprimAdapter : public UsdImagingPrimAdapter {
public:
    typedef UsdImagingPrimAdapter BaseAdapter;

    UsdImagingGprimAdapter()
        : UsdImagingPrimAdapter()
    {}

    virtual ~UsdImagingGprimAdapter();

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
    
    virtual void TrackVariabilityPrep(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      int requestedBits,
                                      UsdImagingInstancerContext const* 
                                          instancerContext = NULL);

    /// Thread Safe.
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  int requestedBits,
                                  int* dirtyBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL);

    virtual void UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath, 
                                   UsdTimeCode time,
                                   int requestedBits,
                                   UsdImagingInstancerContext const* 
                                       instancerContext = NULL);

    /// Thread Safe.
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               int requestedBits,
                               int* dirtyBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL);

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    virtual int ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& property);

    /// Returns the color and opacity for a given prim, taking into account
    /// surface shader colors and explicitly authored color on the prim.
    USDIMAGING_API
    static VtValue GetColorAndOpacity(UsdPrim const& prim, 
                        UsdImagingValueCache::PrimvarInfo* primvarInfo,
                        UsdTimeCode time);
   
protected:

    /// This function can be overridden if the gprim adapter wants to have
    /// control over the primvar discovery.
    virtual void _DiscoverPrimvars(
            UsdGeomGprim const& gprim,
            SdfPath const& cachePath,
            SdfPath const& shaderPath,
            UsdTimeCode time,
            UsdImagingValueCache* valueCache);

private:

    /// Discover required primvars by searching for primvar inputs connected to
    /// the shader network.
    void _DiscoverPrimvarsFromShaderNetwork(UsdGeomGprim const& gprim,
                           SdfPath const& cachePath, 
                           UsdShadeShader const& shader,
                           UsdTimeCode time,
                           UsdImagingValueCache* valueCache);

    // Deprecated shader discovery.
    void _DiscoverPrimvarsDeprecated(UsdGeomGprim const& gprim,
                           SdfPath const& cachePath, 
                           UsdPrim const& shaderPrim,
                           UsdTimeCode time,
                           UsdImagingValueCache* valueCache);

    /// Reads the extent from the given prim. If the extent is not authored,
    /// an empty GfRange3d is returned, the extent will not be computed.
    GfRange3d _GetExtent(UsdPrim const& prim, UsdTimeCode time);

    /// Returns the doubleSided state for a given prim.
    bool _GetDoubleSided(UsdPrim const& prim);

    /// Returns the UsdGeomImagable "purpose" for this prim, including any
    /// inherited purpose. Inherited values are strongest.
    TfToken _GetPurpose(UsdPrim const & prim, UsdTimeCode time);

    /// Returns the surface shader for this prim
    SdfPath _GetSurfaceShader(UsdPrim const& prim);
};

#endif //USDIMAGING_GPRIM_ADAPTER_H
