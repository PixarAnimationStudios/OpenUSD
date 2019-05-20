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
#include "pxr/usdImaging/usdImaging/basisCurvesAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/basisCurves.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingBasisCurvesAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingBasisCurvesAdapter::~UsdImagingBasisCurvesAdapter() 
{
}

bool
UsdImagingBasisCurvesAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->basisCurves);
}

SdfPath
UsdImagingBasisCurvesAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->basisCurves,
                     prim, index, GetMaterialId(prim), instancerContext);
}

void 
UsdImagingBasisCurvesAdapter::TrackVariability(UsdPrim const& prim,
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

    // Check for time-varying primvars:widths, and if that attribute
    // doesn't exist also check for time-varying widths.
    bool widthsExists = false;
    _IsVarying(prim,
               UsdImagingTokens->primvarsWidths,
               HdChangeTracker::DirtyWidths,
               UsdImagingTokens->usdVaryingWidths,
               timeVaryingBits,
               /*isInherited*/false,
               &widthsExists);
    if (!widthsExists) {
        _IsVarying(prim, UsdGeomTokens->widths,
                HdChangeTracker::DirtyWidths,
                UsdImagingTokens->usdVaryingWidths,
                timeVaryingBits,
                /*isInherited*/false);
    }

    // Check for time-varying primvars:normals, and if that attribute
    // doesn't exist also check for time-varying normals.
    bool normalsExists = false;
    _IsVarying(prim,
               UsdImagingTokens->primvarsNormals,
               HdChangeTracker::DirtyNormals,
               UsdImagingTokens->usdVaryingNormals,
               timeVaryingBits,
               /*isInherited*/false,
               &normalsExists);
    if (!normalsExists) {
        _IsVarying(prim, UsdGeomTokens->normals,
                HdChangeTracker::DirtyNormals,
                UsdImagingTokens->usdVaryingNormals,
                timeVaryingBits,
                /*isInherited*/false);
    }
}

bool
UsdImagingBasisCurvesAdapter::_IsBuiltinPrimvar(TfToken const& primvarName) const
{
    return (primvarName == HdTokens->normals ||
            primvarName == HdTokens->widths) ||
        UsdImagingGprimAdapter::_IsBuiltinPrimvar(primvarName);
}

void 
UsdImagingBasisCurvesAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);
    UsdImagingValueCache* valueCache = _GetValueCache();

    HdPrimvarDescriptorVector& primvars = valueCache->GetPrimvars(cachePath);
    if (requestedBits & HdChangeTracker::DirtyTopology) {
        VtValue& topology = valueCache->GetTopology(cachePath);
        _GetBasisCurvesTopology(prim, &topology, time);
    }

    if (requestedBits & HdChangeTracker::DirtyWidths) {
        // First check for "primvars:widths"
        UsdGeomPrimvarsAPI primvarsApi(prim);
        UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
            UsdImagingTokens->primvarsWidths);
        if (pv) {
            _ComputeAndMergePrimvar(prim, cachePath, pv, time, valueCache);
        } else {
            UsdGeomBasisCurves curves(prim);
            HdInterpolation interpolation;
            VtFloatArray widths;
            if (curves.GetWidthsAttr().Get(&widths, time)) {
                interpolation = _UsdToHdInterpolation(
                    curves.GetWidthsInterpolation());
            } else {
                widths = VtFloatArray(1);
                widths[0] = 1.0f;
                interpolation = HdInterpolationConstant;
            }
            _MergePrimvar(&primvars, UsdGeomTokens->widths, interpolation);
            valueCache->GetWidths(cachePath) = VtValue(widths);
        }
    }
    if (requestedBits & HdChangeTracker::DirtyNormals) {
        // First check for "primvars:normals"
        UsdGeomPrimvarsAPI primvarsApi(prim);
        UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
            UsdImagingTokens->primvarsNormals);
        if (pv) {
            _ComputeAndMergePrimvar(prim, cachePath, pv, time, valueCache);
        } else {
            UsdGeomBasisCurves curves(prim);
            VtVec3fArray normals;
            if (curves.GetNormalsAttr().Get(&normals, time)) {
                _MergePrimvar(&primvars,
                        UsdGeomTokens->normals,
                        _UsdToHdInterpolation(curves.GetNormalsInterpolation()),
                        HdPrimvarRoleTokens->normal);
                valueCache->GetNormals(cachePath) = VtValue(normals);
            }
        }
    }
}


// -------------------------------------------------------------------------- //

void
UsdImagingBasisCurvesAdapter::_GetBasisCurvesTopology(UsdPrim const& prim, 
                                         VtValue* topo,
                                         UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // These are uniform attributes and can't vary over time.
    UsdTimeCode unvarying = UsdTimeCode::Default();
    TfToken topoCurveBasis, topoCurveType, topoCurveWrap;
    TfToken curveBasis = _Get<TfToken>(prim, UsdGeomTokens->basis, unvarying);
    TfToken curveType = _Get<TfToken>(prim, UsdGeomTokens->type, unvarying);
    TfToken curveWrap = _Get<TfToken>(prim, UsdGeomTokens->wrap, unvarying);

    if(curveBasis == UsdGeomTokens->bezier) {
        topoCurveBasis = HdTokens->bezier;
    }
    else if(curveBasis == UsdGeomTokens->bspline) {
        topoCurveBasis = HdTokens->bSpline;
    }
    else if(curveBasis == UsdGeomTokens->catmullRom) {
        topoCurveBasis = HdTokens->catmullRom;
    }
    else if(curveBasis == UsdGeomTokens->hermite) {
        topoCurveBasis = HdTokens->hermite;
    }
    else if(curveBasis == UsdGeomTokens->power) {
        topoCurveBasis = HdTokens->power;
    }
    else {
        topoCurveBasis = HdTokens->bezier;
        if (!curveBasis.IsEmpty()) {
            TF_WARN("Unknown curve basis '%s', using '%s'", 
                curveBasis.GetText(), topoCurveBasis.GetText());
        }
    }

    if(curveType == UsdGeomTokens->linear) {
        topoCurveType = HdTokens->linear;
    }
    else if(curveType == UsdGeomTokens->cubic) {
        topoCurveType = HdTokens->cubic;
    }
    else {
        topoCurveType = HdTokens->cubic;
        if (!curveType.IsEmpty()) {
            TF_WARN("Unknown curve type '%s', using '%s'", 
                curveType.GetText(), topoCurveType.GetText());
        }
    }

    if(curveWrap == UsdGeomTokens->periodic) {
        topoCurveWrap = HdTokens->periodic;
    }
    else if(curveWrap == UsdGeomTokens->nonperiodic) {
        topoCurveWrap = HdTokens->nonperiodic;
    }
    else {
        topoCurveWrap = HdTokens->nonperiodic;
        if (!curveWrap.IsEmpty()) {
            TF_WARN("Unknown curve wrap '%s', using '%s'", 
                curveWrap.GetText(), topoCurveWrap.GetText());
        }
    }

    HdBasisCurvesTopology topology(
        topoCurveType, topoCurveBasis, topoCurveWrap,
        _Get<VtIntArray>(prim, UsdGeomTokens->curveVertexCounts, time),
        VtIntArray());
    *topo = VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE

