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

#include "pxr/usdImaging/usdImaging/dataSourceImplicits-Impl.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/geomUtil/sphereMeshGenerator.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/sphereSchema.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
using _PrimSource = UsdImagingDataSourceImplicitsPrim<UsdGeomSphere, HdSphereSchema>;
}

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingSphereAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingSphereAdapter::~UsdImagingSphereAdapter() = default;

TfTokenVector
UsdImagingSphereAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingSphereAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->sphere;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingSphereAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return _PrimSource::New(
            prim.GetPath(),
            prim,
            stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingSphereAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return _PrimSource::Invalidate(
            prim, subprim,properties, invalidationType);
    }
    
    return HdDataSourceLocatorSet();
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
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingSphereAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const* 
                                              instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    // Check DirtyPoints before doing variability checks, to see if we can skip.
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->radius,
                      HdChangeTracker::DirtyPoints,
                      UsdImagingTokens->usdVaryingPrimvar,
                      timeVaryingBits, /*inherited*/false);
    }
}

HdDirtyBits
UsdImagingSphereAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->radius) {
        return HdChangeTracker::DirtyPoints;
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue
UsdImagingSphereAdapter::GetPoints(UsdPrim const& prim,
                                   UsdTimeCode time) const
{
    UsdGeomSphere sphere(prim);
    double radius = 1.0;
    if (!sphere.GetRadiusAttr().Get(&radius, time)) {
        TF_WARN("Could not evaluate double-valued radius attribute on prim %s",
            prim.GetPath().GetText());
    }

    const size_t numPoints =
        GeomUtilSphereMeshGenerator::ComputeNumPoints(numRadial, numAxial);

    VtVec3fArray points(numPoints);
        
    GeomUtilSphereMeshGenerator::GeneratePoints(
        points.begin(),
        numRadial,
        numAxial,
        radius
    );

    return VtValue(points);
}

/*virtual*/ 
VtValue
UsdImagingSphereAdapter::GetTopology(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // All spheres share the same topology.
    static const HdMeshTopology topology =
        HdMeshTopology(GeomUtilSphereMeshGenerator::GenerateTopology(
                            numRadial, numAxial));

    return VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE

