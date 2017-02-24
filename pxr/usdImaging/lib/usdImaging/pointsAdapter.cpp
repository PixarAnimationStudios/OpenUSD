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
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/points.h"

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

SdfPath
UsdImagingPointsAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    index->InsertPoints(prim.GetPath(),
                        GetShaderBinding(prim),
                        instancerContext);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return prim.GetPath();
}

void 
UsdImagingPointsAdapter::TrackVariabilityPrep(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              HdDirtyBits requestedBits,
                                              UsdImagingInstancerContext const* 
                                                  instancerContext)
{
    // Let the base class track what it needs.
    BaseAdapter::TrackVariabilityPrep(
        prim, cachePath, requestedBits, instancerContext);
}

void 
UsdImagingPointsAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits requestedBits,
                                          HdDirtyBits* dirtyBits,
                                          UsdImagingInstancerContext const* 
                                              instancerContext)
{
    BaseAdapter::TrackVariability(
        prim, cachePath, requestedBits, dirtyBits, instancerContext);

    if (requestedBits & HdChangeTracker::DirtyPoints) {
        // Discover time-varying points.
        _IsVarying(prim, 
                   UsdGeomTokens->points, 
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimVar,
                   dirtyBits,
                   /*isInherited*/false);
    }
    if (requestedBits & HdChangeTracker::DirtyWidths) {
        _IsVarying(prim, UsdGeomTokens->widths,
                           HdChangeTracker::DirtyWidths,
                           UsdImagingTokens->usdVaryingWidths,
                           dirtyBits, 
                           /*isInherited*/false);
    }
}

void 
UsdImagingPointsAdapter::UpdateForTimePrep(UsdPrim const& prim,
                                           SdfPath const& cachePath, 
                                           UsdTimeCode time,
                                           HdDirtyBits requestedBits,
                                           UsdImagingInstancerContext const* 
                                               instancerContext)
{
    BaseAdapter::UpdateForTimePrep(
        prim, cachePath, time, requestedBits, instancerContext);
    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdChangeTracker::DirtyPoints)
        valueCache->GetPoints(cachePath);

    if (requestedBits & HdChangeTracker::DirtyWidths)
        valueCache->GetWidths(cachePath);
}

void 
UsdImagingPointsAdapter::UpdateForTime(UsdPrim const& prim,
                                       SdfPath const& cachePath, 
                                       UsdTimeCode time,
                                       HdDirtyBits requestedBits,
                                       HdDirtyBits* resultBits,
                                       UsdImagingInstancerContext const* 
                                           instancerContext)
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, resultBits, instancerContext);
    UsdImagingValueCache* valueCache = _GetValueCache();

    PrimvarInfoVector& primvars = valueCache->GetPrimvars(cachePath);

    VtValue& pointsValues = valueCache->GetPoints(cachePath);

    if (requestedBits & HdChangeTracker::DirtyPoints) {
        _GetPoints(prim, &pointsValues, time);
        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = HdTokens->points;
        primvar.interpolation = UsdGeomTokens->vertex;
        _MergePrimvar(primvar, &primvars);
    }

    if (requestedBits & HdChangeTracker::DirtyWidths) {
        UsdImagingValueCache::PrimvarInfo primvar;
        UsdGeomPoints points(prim);
        VtFloatArray widths;
        primvar.name = UsdGeomTokens->widths;

        // XXX Add support for real constant interpolation
        primvar.interpolation = UsdGeomTokens->vertex;

        // Read the widths, if there is no widths create a buffer
        // and fill it with default widths of 1.0f
        if (!points.GetWidthsAttr().Get(&widths, time)) {

            // Check if we have just updated the points because in that
            // case we don't need to read the points again
            if (!(requestedBits & HdChangeTracker::DirtyPoints)) {
                _GetPoints(prim, &pointsValues, time);
            }

            for(size_t i = 0; i < pointsValues.Get<VtVec3fArray>().size() ; i ++) {
                widths.push_back(1.0f);
            }
        }
        _MergePrimvar(primvar, &primvars);
        valueCache->GetWidths(cachePath) = VtValue(widths);
    }
}


// -------------------------------------------------------------------------- //

void
UsdImagingPointsAdapter::_GetPoints(UsdPrim const& prim, 
                                   VtValue* value, 
                                   UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    if (!prim.GetAttribute(UsdGeomTokens->points).Get(value, time)) {
        *value = VtVec3fArray();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

