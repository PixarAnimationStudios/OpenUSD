//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/cubeAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceImplicits-Impl.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/geomUtil/cuboidMeshGenerator.h"
#include "pxr/imaging/hd/cubeSchema.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
using _PrimSource = UsdImagingDataSourceImplicitsPrim<UsdGeomCube, HdCubeSchema>;
}

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingCubeAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCubeAdapter::~UsdImagingCubeAdapter() = default;

TfTokenVector
UsdImagingCubeAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingCubeAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->cube;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingCubeAdapter::GetImagingSubprimData(
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
UsdImagingCubeAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return _PrimSource::Invalidate(
            prim, subprim, properties, invalidationType);
    }
    
    return HdDataSourceLocatorSet();
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
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
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
    
    // The base adapter may already be setting that points dirty bit.
    // _IsVarying will clear it, so check it isn't already marked as
    // varying before checking for additional set cases.
     if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->size,
                      HdChangeTracker::DirtyPoints,
                      UsdImagingTokens->usdVaryingPrimvar,
                      timeVaryingBits, /*inherited*/false);
    }
}

HdDirtyBits
UsdImagingCubeAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->size) {
        return HdChangeTracker::DirtyPoints;
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue
UsdImagingCubeAdapter::GetPoints(UsdPrim const& prim,
                                 UsdTimeCode time) const
{
    UsdGeomCube cube(prim);

    double size = 2.0;
    if (!cube.GetSizeAttr().Get(&size, time)) {
        TF_WARN("Could not evaluate double-valued size attribute on prim %s",
            prim.GetPath().GetText());
    }
    
    const size_t numPoints =
        GeomUtilCuboidMeshGenerator::ComputeNumPoints();

    VtVec3fArray points(numPoints);
        
    GeomUtilCuboidMeshGenerator::GeneratePoints(
        points.begin(),
        /* xLength = */ size,
        /* yLength = */ size,
        /* zLength = */ size);

    return VtValue(points);
}

/*virtual*/ 
VtValue
UsdImagingCubeAdapter::GetTopology(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // All cubes share the same topology.
    static const HdMeshTopology topology =
        HdMeshTopology(GeomUtilCuboidMeshGenerator::GenerateTopology());

    return VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE

