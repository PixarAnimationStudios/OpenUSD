//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/hermiteCurvesAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/hermiteCurves.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingHermiteCurvesAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingHermiteCurvesAdapter::~UsdImagingHermiteCurvesAdapter() 
{
}

bool
UsdImagingHermiteCurvesAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->basisCurves);
}

SdfPath
UsdImagingHermiteCurvesAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->basisCurves,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingHermiteCurvesAdapter::TrackVariability(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               HdDirtyBits* timeVaryingBits,
                                               UsdImagingInstancerContext const*
                                                   instancerContext) const
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

    // Discover time-varying topology.
    //
    // Note that basis, wrap and type are all uniform attributes, so they can't
    // vary over time.
    _IsVarying(prim, UsdGeomTokens->curveVertexCounts,
                       HdChangeTracker::DirtyTopology,
                       UsdImagingTokens->usdVaryingTopology,
                       timeVaryingBits,
                       /*isInherited*/false);
}

bool
UsdImagingHermiteCurvesAdapter::_IsBuiltinPrimvar(
    TfToken const& primvarName) const
{
    return (primvarName == HdTokens->normals ||
            primvarName == HdTokens->widths) ||
        UsdImagingGprimAdapter::_IsBuiltinPrimvar(primvarName);
}

HdDirtyBits
UsdImagingHermiteCurvesAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->points) {
        return HdChangeTracker::DirtyPoints;
    }

    else if (propertyName == UsdGeomTokens->curveVertexCounts) {
        return HdChangeTracker::DirtyTopology;
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

// -------------------------------------------------------------------------- //

VtValue
UsdImagingHermiteCurvesAdapter::GetTopology(UsdPrim const& prim, 
                                            SdfPath const& cachePath,
                                            UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdBasisCurvesTopology topology(
        HdTokens->linear, HdTokens->bezier, HdTokens->nonperiodic,
        _Get<VtIntArray>(prim, UsdGeomTokens->curveVertexCounts, time),
        VtIntArray());
    return VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE

