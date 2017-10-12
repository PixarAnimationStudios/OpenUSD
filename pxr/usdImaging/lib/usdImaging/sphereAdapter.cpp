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
#include "pxr/usdImaging/usdImaging/sphereAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingSphereAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingSphereAdapter::~UsdImagingSphereAdapter() 
{
}

bool
UsdImagingSphereAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingSphereAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialId(prim), instancerContext);
}

void 
UsdImagingSphereAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const* 
                                              instancerContext)
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
        _IsVarying(prim, UsdGeomTokens->radius,
                      HdChangeTracker::DirtyTransform,
                      UsdImagingTokens->usdVaryingXform,
                      timeVaryingBits, /*inherited*/false);
    }
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingSphereAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext)
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
UsdImagingSphereAdapter::GetMeshPoints(UsdPrim const& prim, 
                                       UsdTimeCode time)
{
    static GfVec3f points[] = {
        GfVec3f( 0.2384,  0.1483, -0.9511) ,GfVec3f( 0.0839,  0.2606, -0.9511) ,GfVec3f(-0.1071,  0.2606, -0.9511),
        GfVec3f(-0.2616,  0.1483, -0.9511) ,GfVec3f(-0.3206, -0.0333, -0.9511) ,GfVec3f(-0.2616, -0.2149, -0.9511),
        GfVec3f(-0.1071, -0.3272, -0.9511) ,GfVec3f( 0.0839, -0.3272, -0.9511) ,GfVec3f( 0.2384, -0.2149, -0.9511),
        GfVec3f( 0.2975, -0.0333, -0.9511) ,GfVec3f( 0.4640,  0.3122, -0.8090) ,GfVec3f( 0.1701,  0.5257, -0.8090),
        GfVec3f(-0.1932,  0.5257, -0.8090) ,GfVec3f(-0.4871,  0.3122, -0.8090) ,GfVec3f(-0.5993, -0.0333, -0.8090),
        GfVec3f(-0.4871, -0.3788, -0.8090) ,GfVec3f(-0.1932, -0.5923, -0.8090) ,GfVec3f( 0.1701, -0.5923, -0.8090),
        GfVec3f( 0.4640, -0.3788, -0.8090) ,GfVec3f( 0.5762, -0.0333, -0.8090) ,GfVec3f( 0.6429,  0.4422, -0.5878),
        GfVec3f( 0.2384,  0.7361, -0.5878) ,GfVec3f(-0.2616,  0.7361, -0.5878) ,GfVec3f(-0.6661,  0.4422, -0.5878),
        GfVec3f(-0.8206, -0.0333, -0.5878) ,GfVec3f(-0.6661, -0.5088, -0.5878) ,GfVec3f(-0.2616, -0.8027, -0.5878),
        GfVec3f( 0.2384, -0.8027, -0.5878) ,GfVec3f( 0.6429, -0.5088, -0.5878) ,GfVec3f( 0.7975, -0.0333, -0.5878),
        GfVec3f( 0.7579,  0.5257, -0.3090) ,GfVec3f( 0.2823,  0.8712, -0.3090) ,GfVec3f(-0.3055,  0.8712, -0.3090),
        GfVec3f(-0.7810,  0.5257, -0.3090) ,GfVec3f(-0.9626, -0.0333, -0.3090) ,GfVec3f(-0.7810, -0.5923, -0.3090),
        GfVec3f(-0.3055, -0.9378, -0.3090) ,GfVec3f( 0.2823, -0.9378, -0.3090) ,GfVec3f( 0.7579, -0.5923, -0.3090),
        GfVec3f( 0.9395, -0.0333, -0.3090) ,GfVec3f( 0.7975,  0.5545, -0.0000) ,GfVec3f( 0.2975,  0.9178, -0.0000),
        GfVec3f(-0.3206,  0.9178, -0.0000) ,GfVec3f(-0.8206,  0.5545, -0.0000) ,GfVec3f(-1.0116, -0.0333,  0.0000),
        GfVec3f(-0.8206, -0.6211,  0.0000) ,GfVec3f(-0.3206, -0.9844,  0.0000) ,GfVec3f( 0.2975, -0.9844,  0.0000),
        GfVec3f( 0.7975, -0.6211,  0.0000) ,GfVec3f( 0.9884, -0.0333,  0.0000) ,GfVec3f( 0.7579,  0.5257,  0.3090),
        GfVec3f( 0.2823,  0.8712,  0.3090) ,GfVec3f(-0.3055,  0.8712,  0.3090) ,GfVec3f(-0.7810,  0.5257,  0.3090),
        GfVec3f(-0.9626, -0.0333,  0.3090) ,GfVec3f(-0.7810, -0.5923,  0.3090) ,GfVec3f(-0.3055, -0.9378,  0.3090),
        GfVec3f( 0.2823, -0.9378,  0.3090) ,GfVec3f( 0.7579, -0.5923,  0.3090) ,GfVec3f( 0.9395, -0.0333,  0.3090),
        GfVec3f( 0.6429,  0.4422,  0.5878) ,GfVec3f( 0.2384,  0.7361,  0.5878) ,GfVec3f(-0.2616,  0.7361,  0.5878),
        GfVec3f(-0.6661,  0.4422,  0.5878) ,GfVec3f(-0.8206, -0.0333,  0.5878) ,GfVec3f(-0.6661, -0.5088,  0.5878),
        GfVec3f(-0.2616, -0.8027,  0.5878) ,GfVec3f( 0.2384, -0.8027,  0.5878) ,GfVec3f( 0.6429, -0.5088,  0.5878),
        GfVec3f( 0.7975, -0.0333,  0.5878) ,GfVec3f( 0.4640,  0.3122,  0.8090) ,GfVec3f( 0.1701,  0.5257,  0.8090),
        GfVec3f(-0.1932,  0.5257,  0.8090) ,GfVec3f(-0.4871,  0.3122,  0.8090) ,GfVec3f(-0.5993, -0.0333,  0.8090),
        GfVec3f(-0.4871, -0.3788,  0.8090) ,GfVec3f(-0.1932, -0.5923,  0.8090) ,GfVec3f( 0.1701, -0.5923,  0.8090),
        GfVec3f( 0.4640, -0.3788,  0.8090) ,GfVec3f( 0.5762, -0.0333,  0.8090) ,GfVec3f( 0.2384,  0.1483,  0.9511),
        GfVec3f( 0.0839,  0.2606,  0.9511) ,GfVec3f(-0.1071,  0.2606,  0.9511) ,GfVec3f(-0.2616,  0.1483,  0.9511),
        GfVec3f(-0.3206, -0.0333,  0.9511) ,GfVec3f(-0.2616, -0.2149,  0.9511) ,GfVec3f(-0.1071, -0.3272,  0.9511),
        GfVec3f( 0.0839, -0.3272,  0.9511) ,GfVec3f( 0.2384, -0.2149,  0.9511) ,GfVec3f( 0.2975, -0.0333,  0.9511),
        GfVec3f(-0.0116, -0.0333, -1.0000) ,GfVec3f(-0.0116, -0.0333,  1.0000),
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
UsdImagingSphereAdapter::GetMeshTopology()
{
    static int numVerts[] = {
        4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4,
        4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4,
        4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4,
        4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4 ,4,
        3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3 ,3,
    };
    static int verts[] = {
        0 ,1 ,11 ,10 ,1 ,2 ,12 ,11 ,2 ,3 ,13 ,12 ,3 ,4 ,14 ,13 ,4 ,5 ,15 ,14, 5
            ,6 ,16 ,15 ,6 ,7 ,17 ,16 ,7 ,8 ,18 ,17 ,8 ,9 ,19 ,18 ,9 ,0 ,10 ,19,
        10 ,11 ,21 ,20 ,11 ,12 ,22 ,21 ,12 ,13 ,23 ,22 ,13 ,14 ,24 ,23 ,14 ,15
            ,25 ,24, 15 ,16 ,26 ,25 ,16 ,17 ,27 ,26 ,17 ,18 ,28 ,27 ,18 ,19 ,29
            ,28 ,19 ,10 ,20 ,29, 20 ,21 ,31 ,30 ,21 ,22 ,32 ,31 ,22 ,23 ,33 ,32
            ,23 ,24 ,34 ,33 ,24 ,25 ,35 ,34, 25 ,26 ,36 ,35 ,26 ,27 ,37 ,36 ,27
            ,28 ,38 ,37 ,28 ,29 ,39 ,38 ,29 ,20 ,30 ,39, 30 ,31 ,41 ,40 ,31 ,32
            ,42 ,41 ,32 ,33 ,43 ,42 ,33 ,34 ,44 ,43 ,34 ,35 ,45 ,44, 35 ,36 ,46
            ,45 ,36 ,37 ,47 ,46 ,37 ,38 ,48 ,47 ,38 ,39 ,49 ,48 ,39 ,30 ,40 ,49,
        40 ,41 ,51 ,50 ,41 ,42 ,52 ,51 ,42 ,43 ,53 ,52 ,43 ,44 ,54 ,53 ,44 ,45
            ,55 ,54, 45 ,46 ,56 ,55 ,46 ,47 ,57 ,56 ,47 ,48 ,58 ,57 ,48 ,49 ,59
            ,58 ,49 ,40 ,50 ,59, 50 ,51 ,61 ,60 ,51 ,52 ,62 ,61 ,52 ,53 ,63 ,62
            ,53 ,54 ,64 ,63 ,54 ,55 ,65 ,64, 55 ,56 ,66 ,65 ,56 ,57 ,67 ,66 ,57
            ,58 ,68 ,67 ,58 ,59 ,69 ,68 ,59 ,50 ,60 ,69, 60 ,61 ,71 ,70 ,61 ,62
            ,72 ,71 ,62 ,63 ,73 ,72 ,63 ,64 ,74 ,73 ,64 ,65 ,75 ,74, 65 ,66 ,76
            ,75 ,66 ,67 ,77 ,76 ,67 ,68 ,78 ,77 ,68 ,69 ,79 ,78 ,69 ,60 ,70 ,79,
        70 ,71 ,81 ,80 ,71 ,72 ,82 ,81 ,72 ,73 ,83 ,82 ,73 ,74 ,84 ,83 ,74 ,75
            ,85 ,84, 75 ,76 ,86 ,85 ,76 ,77 ,87 ,86 ,77 ,78 ,88 ,87 ,78 ,79 ,89
            ,88 ,79 ,70 ,80 ,89, 1 ,0 ,90 ,2 ,1 ,90 ,3 ,2 ,90 ,4 ,3 ,90 ,5 ,4
            ,90 ,6 ,5 ,90 ,7 ,6, 90 ,8 ,7 ,90 ,9 ,8 ,90 ,0 ,9 ,90 ,80 ,81 ,91
            ,81 ,82 ,91 ,82 ,83 ,91 ,83, 84 ,91 ,84 ,85 ,91 ,85 ,86 ,91 ,86 ,87
            ,91 ,87 ,88 ,91 ,88 ,89 ,91 ,89 ,80 ,91,
    };
    static HdMeshTopology sphereTopo(PxOsdOpenSubdivTokens->catmark,
                                   HdTokens->rightHanded,
               _BuildVtArray(numVerts, sizeof(numVerts) / sizeof(numVerts[0])),
               _BuildVtArray(verts, sizeof(verts) / sizeof(verts[0])));
    return VtValue(sphereTopo);
}

/*static*/
GfMatrix4d
UsdImagingSphereAdapter::GetMeshTransform(UsdPrim const& prim, 
                                          UsdTimeCode time)
{
    double radius = 1.0;
    UsdGeomSphere sphere(prim);
    TF_VERIFY(sphere.GetRadiusAttr().Get(&radius, time));
    GfMatrix4d xf(GfVec4d(radius, radius, radius, 1.0));   
    return xf;
}

PXR_NAMESPACE_CLOSE_SCOPE

