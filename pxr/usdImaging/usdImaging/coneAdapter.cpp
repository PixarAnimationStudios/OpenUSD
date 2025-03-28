//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/coneAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceImplicits-Impl.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/geomUtil/coneMeshGenerator.h"
#include "pxr/imaging/hd/coneSchema.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdGeom/cone.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
using _PrimSource = UsdImagingDataSourceImplicitsPrim<UsdGeomCone, HdConeSchema>;
}

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingConeAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingConeAdapter::~UsdImagingConeAdapter() 
{
}

TfTokenVector
UsdImagingConeAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingConeAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->cone;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingConeAdapter::GetImagingSubprimData(
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
UsdImagingConeAdapter::InvalidateImagingSubprim(
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
UsdImagingConeAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingConeAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingConeAdapter::TrackVariability(UsdPrim const& prim,
                                        SdfPath const& cachePath,
                                        HdDirtyBits* timeVaryingBits,
                                        UsdImagingInstancerContext const* 
                                            instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    // Check DirtyPoints before doing variability checks, in case we can skip
    // any of them...
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->height,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits, /*inherited*/false);
    }
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->radius,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits, /*inherited*/false);
    }
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->axis,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits, /*inherited*/false);
    }
}

HdDirtyBits
UsdImagingConeAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->height ||
        propertyName == UsdGeomTokens->radius ||
        propertyName == UsdGeomTokens->axis) {
        return HdChangeTracker::DirtyPoints;
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue
UsdImagingConeAdapter::GetPoints(UsdPrim const& prim,
                                 UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    UsdGeomCone cone(prim);

    double height = 2.0;
    if (!cone.GetHeightAttr().Get(&height, time)) {
        TF_WARN("Could not evaluate double-valued height attribute on prim %s",
            prim.GetPath().GetText());
    }
    double radius = 1.0;
    if (!cone.GetRadiusAttr().Get(&radius, time)) {
        TF_WARN("Could not evaluate double-valued radius attribute on prim %s",
            prim.GetPath().GetText());
    }
    TfToken axis = UsdGeomTokens->z;
    if (!cone.GetAxisAttr().Get(&axis, time)) {
        TF_WARN("Could not evaluate token-valued axis attribute on prim %s",
            prim.GetPath().GetText());
    }

    // The cone point generator computes points such that the "rings" of the
    // cone lie on a plane parallel to the XY plane, with the Z-axis being
    // the "spine" of the cone. These need to be transformed to the right
    // basis when a different spine axis is used.
    const GfMatrix4d basis = UsdImagingGprimAdapter::GetImplicitBasis(axis);

    const size_t numPoints =
        GeomUtilConeMeshGenerator::ComputeNumPoints(numRadial);

    VtVec3fArray points(numPoints);
        
    GeomUtilConeMeshGenerator::GeneratePoints(
        points.begin(),
        numRadial,
        radius,
        height,
        &basis
    );

    return VtValue(points);
}

/*virtual*/ 
VtValue
UsdImagingConeAdapter::GetTopology(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // All cones share the same topology.
    static const HdMeshTopology topology =
        HdMeshTopology(GeomUtilConeMeshGenerator::GenerateTopology(
                            numRadial));

    return VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE

