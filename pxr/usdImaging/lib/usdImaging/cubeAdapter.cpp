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
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCubeAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCubeAdapter::~UsdImagingCubeAdapter() 
{
}

bool
UsdImagingCubeAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingCubeAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialId(prim), instancerContext);
}

void 
UsdImagingCubeAdapter::TrackVariability(UsdPrim const& prim,
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
    
    // The base adapter may already be setting that transform dirty bit.
    // _IsVarying will clear it, so check it isn't already marked as
    // varying before checking for additional set cases.
     if ((*timeVaryingBits & HdChangeTracker::DirtyTransform) == 0) {
        _IsVarying(prim, UsdGeomTokens->size,
                      HdChangeTracker::DirtyTransform,
                      UsdImagingTokens->usdVaryingXform,
                      timeVaryingBits, /*inherited*/false);
    }
}

void 
UsdImagingCubeAdapter::UpdateForTime(UsdPrim const& prim,
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
        _MergePrimvar(&valueCache->GetPrimvars(cachePath),
                      HdTokens->points,
                      HdInterpolationVertex,
                      HdPrimvarRoleTokens->point);
    }

    if (_IsRefined(cachePath)) {
        if (requestedBits & HdChangeTracker::DirtySubdivTags) {
            valueCache->GetSubdivTags(cachePath);
        }
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

PXR_NAMESPACE_CLOSE_SCOPE

