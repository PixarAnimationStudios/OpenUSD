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
#include "pxr/usdImaging/usdImaging/nurbsPatchAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/nurbsPatch.h"

#include "pxr/base/tf/type.h"

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingNurbsPatchAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingNurbsPatchAdapter::~UsdImagingNurbsPatchAdapter() 
{
}

SdfPath
UsdImagingNurbsPatchAdapter::Populate(UsdPrim const& prim, 
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
UsdImagingNurbsPatchAdapter::TrackVariabilityPrep(UsdPrim const& prim,
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
UsdImagingNurbsPatchAdapter::TrackVariability(UsdPrim const& prim,
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
        // Discover time-varying points.
        _IsVarying(prim, 
                   UsdGeomTokens->points, 
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimVar,
                   dirtyBits,
                   /*isInherited*/false);
    }

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        // Discover time-varying topology.
        _IsVarying(prim, UsdGeomTokens->curveVertexCounts,
                           HdChangeTracker::DirtyTopology,
                           UsdImagingTokens->usdVaryingTopology,
                           dirtyBits, 
                           /*isInherited*/false);
    }
}

void 
UsdImagingNurbsPatchAdapter::UpdateForTimePrep(UsdPrim const& prim,
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
UsdImagingNurbsPatchAdapter::UpdateForTime(UsdPrim const& prim,
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
        valueCache->GetTopology(cachePath) = GetMeshTopology(prim, time);
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

/*static*/
VtValue
UsdImagingNurbsPatchAdapter::GetMeshPoints(UsdPrim const& prim, 
                                       UsdTimeCode time)
{
    VtArray<GfVec3f> points;

    if (!prim.GetAttribute(UsdGeomTokens->points).Get(&points, time)) {
        TF_WARN("Points could not be read from prim: <%s>",
                prim.GetPath().GetText());
        points = VtVec3fArray();
    }

    return VtValue(points);
}

/*static*/
VtValue
UsdImagingNurbsPatchAdapter::GetMeshTopology(UsdPrim const& prim, 
                                       UsdTimeCode time)
{
    UsdGeomNurbsPatch nurbsPatch(prim);

    // Obtain the number of CP in each surface direction to be able to calculate
    // quads out of the patches
    int nUVertexCount = 0, nVVertexCount = 0;
    
    if (!nurbsPatch.GetUVertexCountAttr().Get(&nUVertexCount, time)) {
        TF_WARN("UVertexCount could not be read from prim: <%s>",
                prim.GetPath().GetText());
        return VtValue(HdMeshTopology());
    }

    if (!nurbsPatch.GetVVertexCountAttr().Get(&nVVertexCount, time)) {
        TF_WARN("VVertexCount could not be read from prim: <%s>",
                prim.GetPath().GetText());
        return VtValue(HdMeshTopology());
    }

    if (nUVertexCount == 0 || nVVertexCount == 0) {
        TF_WARN("NurbsPatch skipped <%s>, VVertexCount or UVertexCount is 0",
                prim.GetPath().GetText());
        return VtValue(HdMeshTopology());
    }

    // Calculate the number of quads/faces needed
    // as well as the number of indices
    int nFaces = (nUVertexCount -1) * (nVVertexCount - 1);
    int nIndices = nFaces * 4;

    // Prepare the array of vertices per face required for rendering
    VtArray<int> vertsPerFace(nFaces);
    for(int i = 0 ; i < nFaces; i++) {
        vertsPerFace[i] = 4;
    }

    // Prepare the array of indices required for rendering
    // Basically, we create one quad per vertex, except for the ones in the
    // last column and last row.
    int uid = 0;
    VtArray<int> indices(nIndices);
    for(int row = 0 ; row < nVVertexCount - 1 ; row ++) {
        for(int col = 0 ; col < nUVertexCount - 1 ; col ++) {
            int idx = row * nUVertexCount + col;

            indices[uid++] = idx;
            indices[uid++] = idx + 1;
            indices[uid++] = idx + nUVertexCount + 1;
            indices[uid++] = idx + nUVertexCount;
        }
    } 

    // Obtain the orientation
    TfToken orientation;
    if (!prim.GetAttribute(UsdGeomTokens->orientation).Get(&orientation, time)) {
        TF_WARN("Orientation could not be read from prim, using right handed: <%s>",
                prim.GetPath().GetText());
        orientation = HdTokens->rightHanded;
    }

    // Create the mesh topology
    HdMeshTopology topo = HdMeshTopology(
        PxOsdOpenSubdivTokens->catmark, 
        orientation,
        vertsPerFace,
        indices);

    return VtValue(topo);
}
