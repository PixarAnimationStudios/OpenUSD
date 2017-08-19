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
#ifndef USDIMAGING_POINTS_ADAPTER_H
#define USDIMAGING_POINTS_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingPointsAdapter
///
/// Delegate support for UsdGeomPoints.
///
class UsdImagingPointsAdapter : public UsdImagingGprimAdapter {
public:
    typedef UsdImagingGprimAdapter BaseAdapter;

    UsdImagingPointsAdapter()
        : UsdImagingGprimAdapter()
    {}
    USDIMAGING_API
    virtual ~UsdImagingPointsAdapter();

    USDIMAGING_API
    virtual SdfPath Populate(UsdPrim const& prim,
                     UsdImagingIndexProxy* index,
                     UsdImagingInstancerContext const* instancerContext = NULL);

    USDIMAGING_API
    virtual bool IsSupported(HdRenderIndex* renderIndex);

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

private:
    void _GetPoints(UsdPrim const&, VtValue* value, UsdTimeCode time);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGING_POINTS_ADAPTER_H
