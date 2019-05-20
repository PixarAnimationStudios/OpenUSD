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
#include "pxr/usdImaging/usdImaging/pointsAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingPointsAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingPointsAdapter::~UsdImagingPointsAdapter() 
{
}

bool
UsdImagingPointsAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->points);
}

SdfPath
UsdImagingPointsAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->points,
                     prim, index, GetMaterialId(prim), instancerContext);
}

void 
UsdImagingPointsAdapter::TrackVariability(UsdPrim const& prim,
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
UsdImagingPointsAdapter::_IsBuiltinPrimvar(TfToken const& primvarName) const
{
    return (primvarName == HdTokens->normals ||
            primvarName == HdTokens->widths) ||
        UsdImagingGprimAdapter::_IsBuiltinPrimvar(primvarName);
}

void 
UsdImagingPointsAdapter::UpdateForTime(UsdPrim const& prim,
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

    if (requestedBits & HdChangeTracker::DirtyWidths) {
        // First check for "primvars:widths"
        UsdGeomPrimvarsAPI primvarsApi(prim);
        UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
            UsdImagingTokens->primvarsWidths);
        if (pv) {
            _ComputeAndMergePrimvar(prim, cachePath, pv, time, valueCache);
        } else {
            UsdGeomPoints points(prim);
            HdInterpolation interpolation;
            VtFloatArray widths;
            if (points.GetWidthsAttr().Get(&widths, time)) {
                interpolation = _UsdToHdInterpolation(
                    points.GetWidthsInterpolation());
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
            UsdGeomPoints points(prim);
            VtVec3fArray normals;
            if (points.GetNormalsAttr().Get(&normals, time)) {
                _MergePrimvar(&primvars,
                        UsdGeomTokens->normals,
                        _UsdToHdInterpolation(points.GetNormalsInterpolation()),
                        HdPrimvarRoleTokens->normal);
                valueCache->GetNormals(cachePath) = VtValue(normals);
            }
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

