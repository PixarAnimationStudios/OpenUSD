//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/capsuleAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceImplicits-Impl.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/geomUtil/capsuleMeshGenerator.h"
#include "pxr/imaging/hd/capsuleSchema.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdGeom/capsule.h"
#include "pxr/usd/usdGeom/capsule_1.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
using _PrimSource_0 = UsdImagingDataSourceImplicitsPrim<UsdGeomCapsule, HdCapsuleSchema>;
using _PrimSource_1 = UsdImagingDataSourceImplicitsPrim<UsdGeomCapsule_1, HdCapsuleSchema>;
}

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingCapsuleAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingCapsuleAdapter::~UsdImagingCapsuleAdapter()
{
}

TfTokenVector
UsdImagingCapsuleAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingCapsuleAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->capsule;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingCapsuleAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        if (prim.IsA<UsdGeomCapsule>()) {
            return _PrimSource_0::New(
                prim.GetPath(),
                prim,
                stageGlobals);
        } else { // IsA<UsdGeomCapsule_1>()
            return _PrimSource_1::New(
                prim.GetPath(),
                prim,
                stageGlobals);
        }

    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingCapsuleAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        if (prim.IsA<UsdGeomCapsule>()) {
            return _PrimSource_0::Invalidate(
				prim, subprim, properties, invalidationType);
        } else {  // IsA<UsdGeomCapsule_1>()
            return _PrimSource_1::Invalidate(
				prim, subprim, properties, invalidationType);
        }
    }

    return HdDataSourceLocatorSet();
}

bool
UsdImagingCapsuleAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingCapsuleAdapter::Populate(UsdPrim const& prim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)

{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void
UsdImagingCapsuleAdapter::TrackVariability(UsdPrim const& prim,
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
    if (prim.IsA<UsdGeomCapsule>()) {
        if ((*timeVaryingBits & HdChangeTracker::DirtyPoints) == 0) {
            _IsVarying(prim, UsdGeomTokens->radius,
                       HdChangeTracker::DirtyPoints,
                       UsdImagingTokens->usdVaryingPrimvar,
                       timeVaryingBits, /*inherited*/false);
        }
    } else { // IsA<UsdGeomCapsule_1>()
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
UsdImagingCapsuleAdapter::ProcessPropertyChange(UsdPrim const& prim,
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

static void extractRadii(UsdGeomCapsule const& capsule, UsdTimeCode time, double& radiusBottomOut, double& radiusTopOut)
{
    if (!capsule.GetRadiusAttr().Get(&radiusBottomOut, time)) {
        TF_WARN("Could not evaluate double-valued radius attribute on prim %s",
            capsule.GetPath().GetText());
    } else {
        radiusTopOut = radiusBottomOut;
    }
}


static void extractRadii(UsdGeomCapsule_1 const& capsule, UsdTimeCode time, double& radiusBottomOut, double& radiusTopOut)
{
    if (!capsule.GetRadiusBottomAttr().Get(&radiusBottomOut, time)) {
        TF_WARN("Could not evaluate double-valued bottom radius attribute on prim %s",
            capsule.GetPath().GetText());
    }
    if (!capsule.GetRadiusTopAttr().Get(&radiusTopOut, time)) {
        TF_WARN("Could not evaluate double-valued top radius attribute on prim %s",
            capsule.GetPath().GetText());
    }
}

template<typename CapsuleType>
static void extractCapsuleParameters(UsdPrim const& prim, UsdTimeCode time, double& heightOut, double& radiusBottomOut,
        double& radiusTopOut, TfToken& axisOut)
{
    if (!prim.IsA<CapsuleType>()) {
        return;
    }

    CapsuleType capsule(prim);

    if (!capsule.GetHeightAttr().Get(&heightOut, time)) {
        TF_WARN("Could not evaluate double-valued height attribute on prim %s",
            capsule.GetPath().GetText());
    }

    extractRadii(capsule, time, radiusBottomOut, radiusTopOut);

    if (!capsule.GetAxisAttr().Get(&axisOut, time)) {
        TF_WARN("Could not evaluate token-valued axis attribute on prim %s",
            capsule.GetPath().GetText());
    }
}


/*virtual*/
VtValue
UsdImagingCapsuleAdapter::GetPoints(UsdPrim const& prim,
                                    UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    double height = 2.0;
    double radiusBottom = 0.5;
    double radiusTop = 0.5;
    TfToken axis = UsdGeomTokens->z;
    extractCapsuleParameters<UsdGeomCapsule>(prim, time, height, radiusBottom,
            radiusTop, axis);
    extractCapsuleParameters<UsdGeomCapsule_1>(prim, time, height, radiusBottom,
            radiusTop, axis);

    // The capsule point generator computes points such that the "rings" of the
    // capsule lie on a plane parallel to the XY plane, with the Z-axis being
    // the "spine" of the capsule. These need to be transformed to the right
    // basis when a different spine axis is used.
    const GfMatrix4d basis = UsdImagingGprimAdapter::GetImplicitBasis(axis);

    const size_t numPoints =
        GeomUtilCapsuleMeshGenerator::ComputeNumPoints(numRadial, numCapAxial);

    VtVec3fArray points(numPoints);
    const double sweep = 360;
    GeomUtilCapsuleMeshGenerator::GeneratePoints(
        points.begin(),
        numRadial,
        numCapAxial,
        radiusBottom,
        radiusTop,
        height,
        sweep,
        &basis);

    return VtValue(points);
}

/*virtual*/
VtValue
UsdImagingCapsuleAdapter::GetTopology(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // All capsules share the same topology.
    static const HdMeshTopology topology =
        HdMeshTopology(GeomUtilCapsuleMeshGenerator::GenerateTopology(
                            numRadial, numCapAxial));

    return VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE

