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
#include "pxr/usdImaging/usdImaging/capsuleAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/capsule.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCapsuleAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCapsuleAdapter::~UsdImagingCapsuleAdapter() 
{
}

SdfPath
UsdImagingCapsuleAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)

{
    index->InsertMesh(prim.GetPath(),
                      GetShaderBinding(prim),
                      instancerContext);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return prim.GetPath();
}

void 
UsdImagingCapsuleAdapter::TrackVariabilityPrep(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              int requestedBits,
                                              UsdImagingInstancerContext const* 
                                                  instancerContext)
{
    // Let the base class track what it needs.
    BaseAdapter::TrackVariabilityPrep(
        prim, cachePath, requestedBits, instancerContext);
}

void 
UsdImagingCapsuleAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          int requestedBits,
                                          int* dirtyBits,
                                          UsdImagingInstancerContext const* 
                                              instancerContext)
{
    BaseAdapter::TrackVariability(
        prim, cachePath, requestedBits, dirtyBits, instancerContext);
    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.

    if (requestedBits & HdChangeTracker::DirtyPoints) {
        if (!_IsVarying(prim, 
                           UsdGeomTokens->radius,
                           HdChangeTracker::DirtyPoints,
                           UsdImagingTokens->usdVaryingPrimVar,
                           dirtyBits, 
                           /*isInherited*/false)) {
            _IsVarying(prim, 
                       UsdGeomTokens->height,
                       HdChangeTracker::DirtyPoints,
                       UsdImagingTokens->usdVaryingPrimVar,
                       dirtyBits,
                       /*isInherited*/false);
        }
    }
}

void 
UsdImagingCapsuleAdapter::UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath, 
                                   UsdTimeCode time,
                                   int requestedBits,
                                   UsdImagingInstancerContext const* 
                                       instancerContext)
{
    BaseAdapter::UpdateForTimePrep(
        prim, cachePath, time, requestedBits, instancerContext);
    // This adapter will never mark these as dirty, however the client may
    // explicitly ask for them, after the initial cached value is gone.
    
    UsdImagingValueCache* valueCache = _GetValueCache();
    if (requestedBits & HdChangeTracker::DirtyTopology)
        valueCache->GetTopology(cachePath);

    if (requestedBits & HdChangeTracker::DirtyPoints)
        valueCache->GetPoints(cachePath);
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingCapsuleAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               int requestedBits,
                               int* resultBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext)
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, resultBits, instancerContext);
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

// capsules have additional stacks along the spine for each capping hemisphere
static const int _hemisphereStacks = 4;

static VtVec3fArray
_GenerateCapsuleMeshPoints(float radius, float height, TfToken const & axis)
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

    int numPoints = _slices * (_stacks + 1)                   // cylinder
                  + 2 * _slices * (_hemisphereStacks-1)       // hemispheres
                  + 2;                                        // end points

    // populate points
    VtVec3fArray pointsArray(numPoints);
    GfVec3f * p = pointsArray.data();

    // base hemisphere
    *p++ = spine * (-height/2-radius);
    for (int i=0; i<_hemisphereStacks-1; ++i) {
        float a = float(M_PI / 2) * (1.0f - float(i+1) / _hemisphereStacks);
        float r = radius * cosf(a);
        float w = radius * sinf(a);

        for (int j=0; j<_slices; ++j) {
            *p++ = r * ring[j] + spine * (-height/2-w);
        }
    }

    // middle
    for (int i=0; i<=_stacks; ++i) {
        float t = float(i) / _stacks;
        float w = height * (t - 0.5f);

        for (int j=0; j<_slices; ++j) {
            *p++ = radius * ring[j] + spine * w;
        }
    }

    // top hemisphere
    for (int i=0; i<_hemisphereStacks-1; ++i) {
        float a = float(M_PI / 2) * (float(i+1) / _hemisphereStacks);
        float r = radius * cosf(a);
        float w = radius * sinf(a);

        for (int j=0; j<_slices; ++j) {
            *p++ = r *  ring[j] + spine * (height/2+w);
        }
    }
    *p++ = spine * (height/2.0f+radius);

    TF_VERIFY(p - pointsArray.data() == numPoints);

    return pointsArray;
}

/*static*/
VtValue
UsdImagingCapsuleAdapter::GetMeshPoints(UsdPrim const& prim, 
                                        UsdTimeCode time)
{
    UsdGeomCapsule capsule(prim);
    double radius = 0.5;
    double height = 1.0;
    TfToken axis = UsdGeomTokens->z;
    TF_VERIFY(capsule.GetRadiusAttr().Get(&radius, time));
    TF_VERIFY(capsule.GetHeightAttr().Get(&height, time));
    TF_VERIFY(capsule.GetAxisAttr().Get(&axis, time));

    // We can't express varying radius and height via a non-uniform
    // scaling transformation and maintain spherical end caps.
    return VtValue(_GenerateCapsuleMeshPoints(float(radius),
                                              float(height),
                                              axis));
}

static HdMeshTopology
_GenerateCapsuleMeshTopology()
{
    int numCounts = _slices * (_stacks + 2 * _hemisphereStacks);
    int numIndices = 4 * _slices * _stacks                   // cylinder quads
                   + 4 * 2 * _slices * (_hemisphereStacks-1) // hemisphere quads
                   + 3 * 2 * _slices;                        // end cap tris

    VtIntArray countsArray(numCounts);
    int * counts = countsArray.data();

    VtIntArray indicesArray(numIndices);
    int * indices = indicesArray.data();

    // populate face counts and face indices
    int face = 0, index = 0, p = 0;

    // base hemisphere end cap triangles
    int base = p++;
    for (int i=0; i<_slices; ++i) {
        counts[face++] = 3;
        indices[index++] = p + (i+1)%_slices;
        indices[index++] = p + i;
        indices[index++] = base;
    }

    // middle and hemisphere quads
    for (int i=0; i<_stacks+2*(_hemisphereStacks-1); ++i) {
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

    // top hemisphere end cap triangles
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
UsdImagingCapsuleAdapter::GetMeshTopology()
{
    // topology is identical for all capsules
    static HdMeshTopology capsuleTopo = _GenerateCapsuleMeshTopology();

    return VtValue(capsuleTopo);
}

PXR_NAMESPACE_CLOSE_SCOPE

