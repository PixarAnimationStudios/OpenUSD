//
// Copyright 2019 Pixar
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
#include "pxr/usdImaging/usdImaging/nurbsCurvesAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurves.h" 
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingNurbsCurvesAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingNurbsCurvesAdapter::~UsdImagingNurbsCurvesAdapter() 
{
}

bool
UsdImagingNurbsCurvesAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->basisCurves);
}

SdfPath
UsdImagingNurbsCurvesAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->basisCurves,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingNurbsCurvesAdapter::TrackVariability(UsdPrim const& prim,
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
    // Do not check the time-varying NURBS curve attributes since we are 
    // creating a Basis Curve and just drawing the linear segments between 
    // the control points  
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
        UsdGeomPrimvar pv = _GetInheritedPrimvar(prim, HdTokens->widths);
        if (pv && pv.ValueMightBeTimeVarying()) {
            *timeVaryingBits |= HdChangeTracker::DirtyWidths;
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingWidths);
            widthsExists = true;
        }
    }
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
        UsdGeomPrimvar pv = _GetInheritedPrimvar(prim, HdTokens->normals);
        if (pv && pv.ValueMightBeTimeVarying()) {
            *timeVaryingBits |= HdChangeTracker::DirtyNormals;
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingNormals);
            normalsExists = true;
        }
    }
    if (!normalsExists) {
        _IsVarying(prim, UsdGeomTokens->normals,
                HdChangeTracker::DirtyNormals,
                UsdImagingTokens->usdVaryingNormals,
                timeVaryingBits,
                /*isInherited*/false);
    }
}

bool
UsdImagingNurbsCurvesAdapter::_IsBuiltinPrimvar(
                                TfToken const& primvarName) const
{
    return (primvarName == HdTokens->normals ||
            primvarName == HdTokens->widths) ||
        UsdImagingGprimAdapter::_IsBuiltinPrimvar(primvarName);
}

void 
UsdImagingNurbsCurvesAdapter::UpdateForTime(UsdPrim const& prim,
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

        // Currently drawing the cage for NURBS curves so this creates
        // a Basis Curves Topology with pre-set parameters
        _GetBasisCurvesTopology(prim, &topology, time);
    }

    if (requestedBits & HdChangeTracker::DirtyWidths) {
        // First check for "primvars:widths"
        UsdGeomPrimvarsAPI primvarsApi(prim);
        UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
            UsdImagingTokens->primvarsWidths);
        if (!pv) {
            // If it's not found locally, see if it's inherited
            pv = _GetInheritedPrimvar(prim, HdTokens->widths);
        }
        if (pv) {
            _ComputeAndMergePrimvar(prim, cachePath, pv, time, valueCache);
        } else {
            UsdGeomNurbsCurves curves(prim);            
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
        if (!pv) {
            // If it's not found locally, see if it's inherited
            pv = _GetInheritedPrimvar(prim, HdTokens->normals);
        }
        if (pv) {
            _ComputeAndMergePrimvar(prim, cachePath, pv, time, valueCache);
        } else {
            UsdGeomNurbsCurves curves(prim);
            VtVec3fArray normals;
            if (curves.GetNormalsAttr().Get(&normals, time)) {
                _MergePrimvar(&primvars,
                        UsdGeomTokens->normals,
                        _UsdToHdInterpolation(curves.GetNormalsInterpolation()),
                        HdPrimvarRoleTokens->normal);
                valueCache->GetNormals(cachePath) = VtValue(normals);
            } else {
                _RemovePrimvar(&primvars, UsdGeomTokens->normals);
            }
        }
    }
}

HdDirtyBits
UsdImagingNurbsCurvesAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                        SdfPath const& cachePath,
                                        TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->points) {
        return HdChangeTracker::DirtyPoints;
    }
    else if (propertyName == UsdGeomTokens->curveVertexCounts) {
        return HdChangeTracker::DirtyTopology;
    }
    // Handle attributes that are treated as "built-in" primvars.
    else if (propertyName == UsdGeomTokens->widths) {
        UsdGeomCurves curves(prim);
        return UsdImagingPrimAdapter::_ProcessNonPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdTokens->widths,
            _UsdToHdInterpolation(curves.GetWidthsInterpolation()),
            HdChangeTracker::DirtyWidths);
    
    } else if (propertyName == UsdGeomTokens->normals) {
        UsdGeomPointBased pb(prim);
        return UsdImagingPrimAdapter::_ProcessNonPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdTokens->normals,
            _UsdToHdInterpolation(pb.GetNormalsInterpolation()),
            HdChangeTracker::DirtyNormals);
    }
    // Handle prefixed primvars that use special dirty bits.
    else if (propertyName == UsdImagingTokens->primvarsWidths) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdChangeTracker::DirtyWidths);
    
    } else if (propertyName == UsdImagingTokens->primvarsNormals) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
                prim, cachePath, propertyName, HdChangeTracker::DirtyNormals);
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

void
UsdImagingNurbsCurvesAdapter::_GetBasisCurvesTopology(UsdPrim const& prim, 
                                         VtValue* topo, 
                                         UsdTimeCode time) const
{
    // Currently drawing the cage for NURBS curves so this creates 
    // a Basis Curves Topology with the following parameters:
    //      linear, linear, and nonperiodic
    const TfToken& topoCurveBasis = HdTokens->linear;
    const TfToken& topoCurveType = HdTokens->linear;
    const TfToken& topoCurveWrap = HdTokens->nonperiodic;

    HdBasisCurvesTopology topology(
        topoCurveType, topoCurveBasis, topoCurveWrap,
        _Get<VtIntArray>(prim, UsdGeomTokens->curveVertexCounts, time),
        VtIntArray());
    *topo = VtValue(topology);
}

PXR_NAMESPACE_CLOSE_SCOPE
