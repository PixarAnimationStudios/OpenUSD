//
// Copyright 2017 Pixar
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
#ifndef USDIMAGING_LIGHT_ADAPTER_H
#define USDIMAGING_LIGHT_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE


class UsdPrim;

/// \class UsdImagingLightAdapter
///
/// Base class for all lights.
///
class UsdImagingLightAdapter : public UsdImagingPrimAdapter {
public:
    typedef UsdImagingPrimAdapter BaseAdapter;


    UsdImagingLightAdapter()
        : UsdImagingPrimAdapter()
    {}

    USDIMAGING_API
    virtual ~UsdImagingLightAdapter();

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
    
    USDIMAGING_API
    virtual void TrackVariabilityPrep(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      HdDirtyBits requestedBits,
                                      UsdImagingInstancerContext const* 
                                          instancerContext = NULL);

    /// Thread Safe.
    USDIMAGING_API
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits requestedBits,
                                  HdDirtyBits* dirtyBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL);


    /// Thread Safe.
    USDIMAGING_API
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL);

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    /// Returns a bit mask of attributes to be udpated, or
    /// HdChangeTracker::AllDirty if the entire prim must be resynchronized.
    virtual int ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName);

    /// When a PrimResync event occurs, the prim may have been deleted entirely,
    /// adapter plug-ins should override this method to free any per-prim state
    /// that was accumulated in the adapter.
    virtual void ProcessPrimResync(SdfPath const& primPath,
                                   UsdImagingIndexProxy* index);

    /// Removes all associated Rprims and dependencies from the render index
    /// without scheduling them for repopulation. 
    virtual void ProcessPrimRemoval(SdfPath const& primPath,
                                   UsdImagingIndexProxy* index);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGING_LIGHT_ADAPTER_H
