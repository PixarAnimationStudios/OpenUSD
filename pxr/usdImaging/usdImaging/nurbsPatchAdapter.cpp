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
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/nurbsPatch.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingNurbsPatchAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingNurbsPatchAdapter::~UsdImagingNurbsPatchAdapter() 
{
}

bool
UsdImagingNurbsPatchAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingNurbsPatchAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingNurbsPatchAdapter::TrackVariability(UsdPrim const& prim,
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

    // Discover time-varying points.
    _IsVarying(prim,
               UsdGeomTokens->points,
               HdChangeTracker::DirtyPoints,
               UsdImagingTokens->usdVaryingPrimvar,
               timeVaryingBits,
               /*isInherited*/false);

    // Discover time-varying topology.
    _IsVarying(prim, UsdGeomTokens->curveVertexCounts,
                       HdChangeTracker::DirtyTopology,
                       UsdImagingTokens->usdVaryingTopology,
                       timeVaryingBits,
                       /*isInherited*/false);
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingNurbsPatchAdapter::UpdateForTime(UsdPrim const& prim,
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
        valueCache->GetTopology(cachePath) = GetMeshTopology(prim, time);
    }
}

/*virtual*/
VtValue
UsdImagingNurbsPatchAdapter::GetPoints(UsdPrim const& prim,
                                       SdfPath const& cachePath,
                                       UsdTimeCode time) const
{
    TF_UNUSED(cachePath);
    return GetMeshPoints(prim, time);   
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

PXR_NAMESPACE_CLOSE_SCOPE

