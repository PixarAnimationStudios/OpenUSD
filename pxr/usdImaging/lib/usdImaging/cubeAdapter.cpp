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
#include "pxr/usdImaging/usdImaging/cubeAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCubeAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCubeAdapter::~UsdImagingCubeAdapter() 
{
}

SdfPath
UsdImagingCubeAdapter::Populate(UsdPrim const& prim, 
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
UsdImagingCubeAdapter::TrackVariabilityPrep(UsdPrim const& prim,
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
UsdImagingCubeAdapter::TrackVariability(UsdPrim const& prim,
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
    
    UsdTimeCode time(1.0);
    if (requestedBits & HdChangeTracker::DirtyTransform) {
        if (!(*dirtyBits & HdChangeTracker::DirtyTransform)) {
            _IsVarying(prim, UsdGeomTokens->size,
                          HdChangeTracker::DirtyTransform,
                          UsdImagingTokens->usdVaryingXform,
                          dirtyBits, /*inherited*/false);
        }
    }
}

void 
UsdImagingCubeAdapter::UpdateForTimePrep(UsdPrim const& prim,
                                         SdfPath const& cachePath, 
                                         UsdTimeCode time,
                                         int requestedBits,
                                         UsdImagingInstancerContext const* 
                                             instancerContext)
{
    BaseAdapter::UpdateForTimePrep(prim, cachePath, time, requestedBits);
    UsdImagingValueCache* valueCache = _GetValueCache();
    // This adapter will never mark these as dirty, however the client may
    // explicitly ask for them, after the initial cached value is gone.
    if (requestedBits & HdChangeTracker::DirtyPoints)
        valueCache->GetPoints(cachePath);
    if (requestedBits & HdChangeTracker::DirtyTopology)
        valueCache->GetTopology(cachePath);
}

void 
UsdImagingCubeAdapter::UpdateForTime(UsdPrim const& prim,
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

    if (requestedBits & HdChangeTracker::DirtyTransform) {
        // Update the transform with the size authored for the cube.
        GfMatrix4d& ctm = valueCache->GetTransform(cachePath);
        GfMatrix4d xf = GetMeshTransform(prim, time);
        ctm = xf * ctm;
    }
    if (requestedBits & HdChangeTracker::DirtyTopology) {
        valueCache->GetTopology(cachePath) = GetMeshTopology();
    }
    if (requestedBits & HdChangeTracker::DirtyPoints) {
        valueCache->GetPoints(cachePath)= GetMeshPoints(prim, time);

        // Expose points as a primvar.
        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = HdTokens->points;
        primvar.interpolation = UsdGeomTokens->vertex;
        PrimvarInfoVector& primvars = valueCache->GetPrimvars(cachePath);
        _MergePrimvar(primvar, &primvars);
    }
}

// -------------------------------------------------------------------------- //

/*static*/
VtValue
UsdImagingCubeAdapter::GetMeshPoints(UsdPrim const& prim, 
                                     UsdTimeCode time)
{
    static GfVec3f points[] = {
        GfVec3f( 0.5f, 0.5f, 0.5f ),
        GfVec3f(-0.5f, 0.5f, 0.5f ),
        GfVec3f(-0.5f,-0.5f, 0.5f ),
        GfVec3f( 0.5f,-0.5f, 0.5f ),
        GfVec3f(-0.5f,-0.5f,-0.5f ),
        GfVec3f(-0.5f, 0.5f,-0.5f ),
        GfVec3f( 0.5f, 0.5f,-0.5f ),
        GfVec3f( 0.5f,-0.5f,-0.5f ),
    };

    size_t numPoints = sizeof(points) / sizeof(points[0]);
    VtArray<GfVec3f> output(numPoints);
    std::copy(points, points + numPoints, output.begin());
    return VtValue(output);
}

template <typename T>
static VtArray<T>
_BuildVtArray(T values[], int numValues)
{
    VtArray<T> result(numValues);
    std::copy(values, values+numValues, result.begin());
    return result;
}

/*static*/
VtValue
UsdImagingCubeAdapter::GetMeshTopology()
{
    static int numVerts[] = { 4, 4, 4, 4, 4, 4 };
    static int verts[] = {
        0, 1, 2, 3,
        4, 5, 6, 7,
        0, 6, 5, 1,
        4, 7, 3, 2,
        0, 3, 7, 6,
        4, 2, 1, 5,
    };
    static HdMeshTopology cubeTopo(PxOsdOpenSubdivTokens->bilinear,
               HdTokens->rightHanded,
               _BuildVtArray(numVerts, sizeof(numVerts) / sizeof(numVerts[0])),
               _BuildVtArray(verts, sizeof(verts) / sizeof(verts[0])));
    return VtValue(cubeTopo);
}

/*static*/
GfMatrix4d
UsdImagingCubeAdapter::GetMeshTransform(UsdPrim const& prim, 
                                        UsdTimeCode time)
{
    double size = 2.0;
    UsdGeomCube cube(prim);
    TF_VERIFY(cube.GetSizeAttr().Get(&size, time));
    GfMatrix4d xf(GfVec4d(size, size, size, 1.0));
    return xf;
}
