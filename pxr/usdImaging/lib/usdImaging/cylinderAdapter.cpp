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
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/cylinder.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

#include <cmath>

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
                     prim, index, GetMaterialId(prim), instancerContext);
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
    
    if (!_IsVarying(prim,
                       UsdGeomTokens->radius,
                       HdChangeTracker::DirtyPoints,
                       UsdImagingTokens->usdVaryingPrimvar,
                       timeVaryingBits,
                       /*isInherited*/false)) {
        _IsVarying(prim,
                   UsdGeomTokens->height,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits,
                   /*isInherited*/false);
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
    if (requestedBits & HdChangeTracker::DirtyTopology) {
        valueCache->GetTopology(cachePath) = GetMeshTopology();
    }
    if (requestedBits & HdChangeTracker::DirtyPoints) {
        valueCache->GetPoints(cachePath) = GetMeshPoints(prim, time);

        // Expose points as a primvar.
        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = HdTokens->points;
        primvar.interpolation = UsdGeomTokens->vertex;
        _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
    }
}


// -------------------------------------------------------------------------- //

// slices are segments around the mesh
static const int _slices = 10;

// stacks are segments along the spine axis
static const int _stacks = 1;

static VtVec3fArray
_GenerateCylinderMeshPoints(float baseRadius,
                            float topRadius,
                            float height,
                            TfToken const & axis)
{
    // choose basis vectors aligned with the spine axis
    GfVec3f u, v, spine;
    if (axis == UsdGeomTokens->x) {
        u = GfVec3f::YAxis();
        v = GfVec3f::ZAxis();
        spine = GfVec3f::XAxis();
    } else if (axis == UsdGeomTokens->y) {
        u = GfVec3f::ZAxis();
        v = GfVec3f::XAxis();
        spine = GfVec3f::YAxis();
    } else { // (axis == UsdGeomTokens->z)
        u = GfVec3f::XAxis();
        v = GfVec3f::YAxis();
        spine = GfVec3f::ZAxis();
    }

    // compute a ring of points with unit radius in the uv plane
    std::vector<GfVec3f> ring(_slices);
    for (int i=0; i<_slices; ++i) {
        float a = float(2 * M_PI * i) / _slices;
        ring[i] = u * cosf(a) + v * sinf(a);
    }

    int numPoints = _slices * (_stacks + 1 + 2) + 2;

    // populate points
    VtVec3fArray pointsArray(numPoints);
    GfVec3f * p = pointsArray.data();

    // base cap
    *p++ = spine * -height/2.0f;
    for (int j=0; j<_slices; ++j) {
        *p++ = baseRadius * ring[j] + spine * -height/2.0f;
    }

    // cylinder
    for (int i=0; i<=_stacks; ++i) {
        float t = float(i) / _stacks;
        float r = t * (topRadius-baseRadius) + baseRadius;
        float w = height * (t - 0.5f);

        for (int j=0; j<_slices; ++j) {
            *p++ = r * ring[j] + spine * w;
        }
    }

    // top cap
    for (int j=0; j<_slices; ++j) {
        *p++ = topRadius * ring[j] + spine * height/2.0f;
    }
    *p++ = spine * height/2.0f;

    TF_VERIFY(p - pointsArray.data() == numPoints);

    return pointsArray;
}

/*static*/
VtValue
UsdImagingCylinderAdapter::GetMeshPoints(UsdPrim const& prim, 
                                         UsdTimeCode time)
{
    UsdGeomCylinder geoSchema(prim);

    double radius = 1.0;
    TF_VERIFY(geoSchema.GetRadiusAttr().Get(&radius, time));
    double height = 2.0;
    TF_VERIFY(geoSchema.GetHeightAttr().Get(&height, time));
    TfToken axis = UsdGeomTokens->z;
    TF_VERIFY(geoSchema.GetAxisAttr().Get(&axis, time));

    // We could express radius and height via a
    // (potentially non-uniform) scaling transformation.
    return VtValue(_GenerateCylinderMeshPoints(float(radius),
                                               float(radius),
                                               float(height),
                                               axis));
}

static HdMeshTopology
_GenerateCylinderMeshTopology()
{
    int numCounts = _slices * _stacks + 2 * _slices;
    int numIndices = 4 * _slices * _stacks      // cylinder quads
                   + 3 * 2 * _slices;           // end cap triangles

    VtIntArray countsArray(numCounts);
    int * counts = countsArray.data();

    VtIntArray indicesArray(numIndices);
    int * indices = indicesArray.data();

    // populate face counts and face indices
    int face = 0, index = 0, p = 0;

    // base end cap triangles
    int base = p++;
    for (int i=0; i<_slices; ++i) {
        counts[face++] = 3;
        indices[index++] = p + (i+1)%_slices;
        indices[index++] = p + i;
        indices[index++] = base;
    }
    p += _slices;

    // cylinder quads
    for (int i=0; i<_stacks; ++i) {
        for (int j=0; j<_slices; ++j) {
            float x0 = 0;
            float x1 = x0 + _slices;
            float y0 = j;
            float y1 = (j + 1) % _slices;
            counts[face++] = 4;
            indices[index++] = p + x0 + y0;
            indices[index++] = p + x0 + y1;
            indices[index++] = p + x1 + y1;
            indices[index++] = p + x1 + y0;
        }
        p += _slices;
    }
    p += _slices;

    // top end cap triangles
    int top = p + _slices;
    for (int i=0; i<_slices; ++i) {
        counts[face++] = 3;
        indices[index++] = p + i;
        indices[index++] = p + (i+1)%_slices;
        indices[index++] = top;
    }

    TF_VERIFY(face == numCounts && index == numIndices);

    return HdMeshTopology(PxOsdOpenSubdivTokens->catmark,
                          HdTokens->rightHanded,
                          countsArray, indicesArray);
}

/*static*/
VtValue
UsdImagingCylinderAdapter::GetMeshTopology()
{
    // topology is identical for all cylinders
    static HdMeshTopology cylinderTopo = _GenerateCylinderMeshTopology();

    return VtValue(cylinderTopo);
}

PXR_NAMESPACE_CLOSE_SCOPE

