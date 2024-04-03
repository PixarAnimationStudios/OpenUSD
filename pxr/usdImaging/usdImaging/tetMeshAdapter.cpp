//
// Copyright 2023 Pixar
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
#include "pxr/usdImaging/usdImaging/tetMeshAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceMesh.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingTetMeshAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingTetMeshAdapter::~UsdImagingTetMeshAdapter() = default;

SdfPath
UsdImagingTetMeshAdapter::Populate(UsdPrim const& prim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

bool
UsdImagingTetMeshAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

void
UsdImagingTetMeshAdapter::TrackVariability(UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits* timeVaryingBits,
        UsdImagingInstancerContext const* instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    // Discover time-varying points.
    _IsVarying(prim,
               UsdGeomTokens->points,
               HdChangeTracker::DirtyPoints,
               UsdImagingTokens->usdVaryingPrimvar,
               timeVaryingBits,
               /*isInherited*/false);

    _IsVarying(prim,
            UsdGeomTokens->tetVertexIndices,
            HdChangeTracker::DirtyTopology,
            UsdImagingTokens->usdVaryingTopology,
            timeVaryingBits,
            /*isInherited*/false);
}

/*virtual*/ 
VtValue
UsdImagingTetMeshAdapter::GetTopology(UsdPrim const& prim,
                                       SdfPath const& cachePath,
                                       UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Compute the surfaceFaceIndices for the Tet Mesh
    VtVec3iArray surfaceFaceIndices;
    UsdGeomTetMesh::ComputeSurfaceFaces(
        UsdGeomTetMesh(prim), &surfaceFaceIndices, time);

    // Compute faceVertexIndices and faceVertexCounts for the HdMeshTopology
    const size_t numCounts = surfaceFaceIndices.size();
    VtIntArray faceVertexIndices;
    VtIntArray faceVertexCounts(numCounts);
    for (size_t i = 0; i < numCounts; ++i) {
        faceVertexIndices.push_back(surfaceFaceIndices[i][0]);
        faceVertexIndices.push_back(surfaceFaceIndices[i][1]);
        faceVertexIndices.push_back(surfaceFaceIndices[i][2]);
        faceVertexCounts[i] = 3;
    }

    // Create and return the HdMeshTopology
    HdMeshTopology tetMeshTopology(
        PxOsdOpenSubdivTokens->catmullClark,
        _Get<TfToken>(prim, UsdGeomTokens->orientation, time),
        faceVertexCounts,
        faceVertexIndices);

    return VtValue(tetMeshTopology);
}


PXR_NAMESPACE_CLOSE_SCOPE