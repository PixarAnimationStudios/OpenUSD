//
// Copyright 2018 Pixar
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
#ifndef PXR_USD_IMAGING_USD_NPR_IMAGING_CONTOUR_ADAPTER_H
#define PXR_USD_IMAGING_USD_NPR_IMAGING_CONTOUR_ADAPTER_H

/// \file usdImaging/contourAdapter.h

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"
#include "pxr/usdImaging/usdNprImaging/api.h"
#include "pxr/usdImaging/usdNprImaging/dualMesh.h"
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class UsdNprDualMesh;

struct UsdNprOutputBuffer {
  VtArray<GfVec3f> points;
  VtArray<int> faceVertexCounts;
  VtArray<int> faceVertexIndices;
};

typedef std::vector<UsdNprOutputBuffer> UsdNprOutputBufferVector;
typedef TfHashMap<SdfPath, UsdNprDualMeshSharedPtr, SdfPath::Hash> UsdNprDualMeshMap;


/// \class UsdImagingContourAdapter
///
/// Delegate support for UsdNprContour.
///
class UsdImagingContourAdapter : public UsdImagingGprimAdapter {
public:
    typedef UsdImagingGprimAdapter BaseAdapter;

    UsdImagingContourAdapter()
        : UsdImagingGprimAdapter()
    {}
    USDNPRIMAGING_API
    virtual ~UsdImagingContourAdapter();

    USDNPRIMAGING_API
    SdfPath Populate(
        UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const* instancerContext = nullptr) override;

    USDNPRIMAGING_API
    bool IsSupported(UsdImagingIndexProxy const* index) const override;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //

    /// Thread Safe.
    USDNPRIMAGING_API
    void TrackVariability(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits* timeVaryingBits,
        UsdImagingInstancerContext const* instancerContext = nullptr) 
            const override;

    /// Thread Safe.
    USDNPRIMAGING_API
    void UpdateForTime(
        UsdPrim const& prim,
        SdfPath const& cachePath, 
        UsdTimeCode time,
        HdDirtyBits requestedBits,
        UsdImagingInstancerContext const* instancerContext = nullptr) 
            const override;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing API (public)
    // ---------------------------------------------------------------------- //
    USDNPRIMAGING_API
    HdDirtyBits ProcessPropertyChange(const UsdPrim& prim,
                                      const SdfPath& cachePath,
                                      const TfToken& propertyName) override;

    USDNPRIMAGING_API
    void ProcessPrimResync(SdfPath const& primPath,
                           UsdImagingIndexProxy* index) override;

    USDNPRIMAGING_API
    void ProcessPrimRemoval(SdfPath const& primPath,
                            UsdImagingIndexProxy* index) override;

    USDNPRIMAGING_API
    void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) override;        
       
private:
  void _ComputeOutputGeometry(const UsdNprOutputBufferVector& buffers,
    UsdImagingValueCache* valueCache, SdfPath const& cachePath) const;

  UsdNprDualMeshMap       _dualMeshes;
  mutable std::mutex      _mutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_NPR_IMAGING_CONTOUR_ADAPTER_H
