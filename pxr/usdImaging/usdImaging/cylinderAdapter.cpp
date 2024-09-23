//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/cylinderAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceImplicits-Impl.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/geomUtil/cylinderMeshGenerator.h"
#include "pxr/imaging/hd/cylinderSchema.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdGeom/cylinder.h"
#include "pxr/usd/usdGeom/cylinder_1.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
using _PrimSource_0 = UsdImagingDataSourceImplicitsPrim<UsdGeomCylinder,
      HdCylinderSchema>;
using _PrimSource_1 = UsdImagingDataSourceImplicitsPrim<UsdGeomCylinder_1,
      HdCylinderSchema>;
}

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingCylinderAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCylinderAdapter::~UsdImagingCylinderAdapter() = default;

TfTokenVector
UsdImagingCylinderAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingCylinderAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->cylinder;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingCylinderAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        if (prim.IsA<UsdGeomCylinder>()) {
            return _PrimSource_0::New(
                prim.GetPath(),
                prim,
                stageGlobals);
        } else { // IsA<UsdGeomCylinder_1>()
            return _PrimSource_1::New(
                prim.GetPath(),
                prim,
                stageGlobals);
        }
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingCylinderAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        if (prim.IsA<UsdGeomCylinder>()) {
            return _PrimSource_0::Invalidate(
				prim, subprim, properties, invalidationType);
        } else { // IsA<UsdGeomCylinder_1>()
            return _PrimSource_1::Invalidate(
				prim, subprim, properties, invalidationType);
        }
    }

    return HdDataSourceLocatorSet();
}

bool
UsdImagingCylinderAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingCylinderAdapter::Populate(UsdPrim const& prim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)

{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void
UsdImagingCylinderAdapter::TrackVariability(UsdPrim const& prim,
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

    // IMPORTANT: Calling _IsVarying will clear the specified bit if the given
    // attribute is _not_ varying.  Since we have multiple attributes (and the
    // base adapter invocation) that might result in the bit being set, we need
    // to be careful not to reset it.  Translation: only check _IsVarying for a
    // given cause IFF the bit wasn't already set by a previous invocation.
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->height,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits, /*inherited*/false);
    }
    if (prim.IsA<UsdGeomCylinder>()) {
        if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
            _IsVarying(prim, UsdGeomTokens->radius,
                       HdChangeTracker::DirtyPoints,
                       UsdImagingTokens->usdVaryingPrimvar,
                       timeVaryingBits, /*inherited*/false);
        }
    } else { // IsA<UsdGeomCylinder_1>()
        if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
            _IsVarying(prim, UsdGeomTokens->radiusBottom,
                       HdChangeTracker::DirtyPoints,
                       UsdImagingTokens->usdVaryingPrimvar,
                       timeVaryingBits, /*inherited*/false);
        }
        if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
            _IsVarying(prim, UsdGeomTokens->radiusTop,
                       HdChangeTracker::DirtyPoints,
                       UsdImagingTokens->usdVaryingPrimvar,
                       timeVaryingBits, /*inherited*/false);
        }
    }
    if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
        _IsVarying(prim, UsdGeomTokens->axis,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits, /*inherited*/false);
    }
}

HdDirtyBits
UsdImagingCylinderAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                                 SdfPath const& cachePath,
                                                 TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->height ||
        propertyName == UsdGeomTokens->radius ||
        propertyName == UsdGeomTokens->radiusBottom ||
        propertyName == UsdGeomTokens->radiusTop ||
        propertyName == UsdGeomTokens->axis) {
        return HdChangeTracker::DirtyPoints;
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

static void extractRadii(UsdGeomCylinder const& cylinder,
                         UsdTimeCode time,
                         double& radiusBottomOut,
                         double& radiusTopOut)
{
    if (!cylinder.GetRadiusAttr().Get(&radiusBottomOut, time)) {
        TF_WARN("Could not evaluate double-valued radius attribute on prim %s",
            cylinder.GetPath().GetText());
    } else {
        radiusTopOut = radiusBottomOut;
    }
}


static void extractRadii(UsdGeomCylinder_1 const& cylinder,
                         UsdTimeCode time,
                         double& radiusBottomOut,
                         double& radiusTopOut)
{
    if (!cylinder.GetRadiusBottomAttr().Get(&radiusBottomOut, time)) {
        TF_WARN("Could not evaluate double-valued bottom radius attribute on " \
                "prim %s", cylinder.GetPath().GetText());
    }
    if (!cylinder.GetRadiusTopAttr().Get(&radiusTopOut, time)) {
        TF_WARN("Could not evaluate double-valued top radius attribute on " \
                "prim %s", cylinder.GetPath().GetText());
    }
}

template<typename CylinderType>
static void extractCylinderParameters(UsdPrim const& prim,
                                      UsdTimeCode time,
                                      double& heightOut,
                                      double& radiusBottomOut,
                                      double& radiusTopOut,
                                      TfToken& axisOut)
{
    if (!prim.IsA<CylinderType>()) {
        return;
    }

    CylinderType cylinder(prim);

    if (!cylinder.GetHeightAttr().Get(&heightOut, time)) {
        TF_WARN("Could not evaluate double-valued height attribute on prim %s",
            cylinder.GetPath().GetText());
    }

    extractRadii(cylinder, time, radiusBottomOut, radiusTopOut);

    if (!cylinder.GetAxisAttr().Get(&axisOut, time)) {
        TF_WARN("Could not evaluate token-valued axis attribute on prim %s",
            cylinder.GetPath().GetText());
    }
}

/*virtual*/
VtValue
UsdImagingCylinderAdapter::GetPoints(UsdPrim const& prim,
                                     UsdTimeCode time) const
{
    double height = 2.0;
    double radiusBottom = 1.0;
    double radiusTop = 1.0;
    TfToken axis = UsdGeomTokens->z;
    extractCylinderParameters<UsdGeomCylinder>(prim, time, height, radiusBottom,
        radiusTop, axis);
    extractCylinderParameters<UsdGeomCylinder_1>(prim, time, height,
        radiusBottom, radiusTop, axis);


    const GfMatrix4d basis = UsdImagingGprimAdapter::GetImplicitBasis(axis);

    const size_t numPoints =
        GeomUtilCylinderMeshGenerator::ComputeNumPoints(numRadial);

    VtVec3fArray points(numPoints);

    GeomUtilCylinderMeshGenerator::GeneratePoints(
        points.begin(),
        numRadial,
        radiusBottom,
        radiusTop,
        height,
        &basis
    );

    return VtValue(points);
}

/*virtual*/
VtValue
UsdImagingCylinderAdapter::GetTopology(UsdPrim const& prim,
                                       SdfPath const& cachePath,
                                       UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // All cylinders share the same topology.
    static const HdMeshTopology topology =
        HdMeshTopology(GeomUtilCylinderMeshGenerator::GenerateTopology(
                            numRadial));

    return VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE

