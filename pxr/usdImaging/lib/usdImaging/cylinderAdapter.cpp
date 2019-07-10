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
#include "pxr/usdImaging/usdImaging/cylinderAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/implicitSurfaceMeshUtils.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdGeom/cylinder.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCylinderAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCylinderAdapter::~UsdImagingCylinderAdapter() 
{
}

bool
UsdImagingCylinderAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingCylinderAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)

{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingCylinderAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const* 
                                              instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);
    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.

    // IMPORTANT: Calling _IsVarying will clear the specified bit if the given
    // attribute is _not_ varying.  Since we have multiple attributes (and the
    // base adapter invocation) that might result in the bit being set, we need
    // to be careful not to reset it.  Translation: only check _IsVarying for a
    // given cause IFF the bit wasn't already set by a previous invocation.
    if ((*timeVaryingBits & HdChangeTracker::DirtyTransform) == 0) {
        _IsVarying(prim, UsdGeomTokens->height,
                   HdChangeTracker::DirtyTransform,
                   UsdImagingTokens->usdVaryingXform,
                   timeVaryingBits, /*inherited*/false);
    }
    if ((*timeVaryingBits & HdChangeTracker::DirtyTransform) == 0) {
        _IsVarying(prim, UsdGeomTokens->radius,
                   HdChangeTracker::DirtyTransform,
                   UsdImagingTokens->usdVaryingXform,
                   timeVaryingBits, /*inherited*/false);
    }
    if ((*timeVaryingBits & HdChangeTracker::DirtyTransform) == 0) {
        _IsVarying(prim, UsdGeomTokens->axis,
                   HdChangeTracker::DirtyTransform,
                   UsdImagingTokens->usdVaryingXform,
                   timeVaryingBits, /*inherited*/false);
    }
}


// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingCylinderAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);
    UsdImagingValueCache* valueCache = _GetValueCache();
    if (requestedBits & HdChangeTracker::DirtyTransform) {
        // Update the transform with the size authored for the sphere.
        GfMatrix4d& ctm = valueCache->GetTransform(cachePath);
        GfMatrix4d xf = GetMeshTransform(prim, time);
        ctm = xf * ctm;
    }
    if (requestedBits & HdChangeTracker::DirtyTopology) {
        valueCache->GetTopology(cachePath) = GetMeshTopology();
    }
}

/*virtual*/
VtValue
UsdImagingCylinderAdapter::GetPoints(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdTimeCode time) const
{
    TF_UNUSED(cachePath);
    return GetMeshPoints(prim, time);   
}

/*static*/
VtValue
UsdImagingCylinderAdapter::GetMeshPoints(UsdPrim const& prim,
                                         UsdTimeCode time)
{
    // The points are constant; the prim's attributes are accomodated by
    // manipulating the transform (see GetMeshTransform() below).
    return VtValue(UsdImagingGetUnitCylinderMeshPoints());
}

/*static*/
VtValue
UsdImagingCylinderAdapter::GetMeshTopology()
{
    // Like the points, topology is constant and identical for all cylinders.
    return VtValue(HdMeshTopology(UsdImagingGetUnitCylinderMeshTopology()));
}

/*static*/
GfMatrix4d
UsdImagingCylinderAdapter::GetMeshTransform(UsdPrim const& prim, 
                                            UsdTimeCode time)
{
    UsdGeomCylinder cylinder(prim);

    double height = 2.0;
    UsdGeomSphere sphere(prim);
    if (!cylinder.GetHeightAttr().Get(&height, time)) {
        TF_WARN("Could not evaluate double-valued height attribute on prim %s",
            prim.GetPath().GetText());
    }
    double radius = 1.0;
    if (!cylinder.GetRadiusAttr().Get(&radius, time)) {
        TF_WARN("Could not evaluate double-valued radius attribute on prim %s",
            prim.GetPath().GetText());
    }
    TfToken axis = UsdGeomTokens->z;
    if (!cylinder.GetAxisAttr().Get(&axis, time)) {
        TF_WARN("Could not evaluate token-valued axis attribute on prim %s",
            prim.GetPath().GetText());
    }

    return UsdImagingGenerateConeOrCylinderTransform(height, radius, axis);
}

size_t
UsdImagingCylinderAdapter::SampleTransform(
    UsdPrim const& prim, SdfPath const& cachePath,
    const std::vector<float>& configuredSampleTimes,
    size_t maxNumSamples, float *sampleTimes,
    GfMatrix4d *sampleValues)
{
    const size_t numSamples = BaseAdapter::SampleTransform(
        prim, cachePath, configuredSampleTimes, maxNumSamples,
        sampleTimes, sampleValues);

    // Apply modeling transformation (which may be time-varying)
    for (size_t i=0; i < numSamples; ++i) {
        UsdTimeCode usdTime = _GetTimeWithOffset(sampleTimes[i]);
        GfMatrix4d xf = GetMeshTransform(prim, usdTime);
        sampleValues[i] = xf * sampleValues[i];
    }

    return numSamples;
}

PXR_NAMESPACE_CLOSE_SCOPE

