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
#include "pxr/usdImaging/usdImaging/meshAdapter.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingMeshAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingMeshAdapter::~UsdImagingMeshAdapter()
{
}

bool
UsdImagingMeshAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}

SdfPath
UsdImagingMeshAdapter::Populate(UsdPrim const& prim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialId(prim), instancerContext);
}


void
UsdImagingMeshAdapter::TrackVariability(UsdPrim const& prim,
                                        SdfPath const& cachePath,
                                        HdDirtyBits* timeVaryingBits,
                                        UsdImagingInstancerContext const* 
                                            instancerContext)
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);
    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.

    // Discover time-varying points.
    _IsVarying(prim,
               UsdGeomTokens->points,
               HdChangeTracker::DirtyPoints,
               UsdImagingTokens->usdVaryingPrimVar,
               timeVaryingBits,
               /*isInherited*/false);

    // Discover time-varying topology.
    if (!_IsVarying(prim,
                       UsdGeomTokens->faceVertexCounts,
                       HdChangeTracker::DirtyTopology,
                       UsdImagingTokens->usdVaryingTopology,
                       timeVaryingBits,
                       /*isInherited*/false)) {
        // Only do this check if the faceVertexCounts is not already known
        // to be varying.
        if (!_IsVarying(prim,
                           UsdGeomTokens->faceVertexIndices,
                           HdChangeTracker::DirtyTopology,
                           UsdImagingTokens->usdVaryingTopology,
                           timeVaryingBits,
                           /*isInherited*/false)) {
            // Only do this check if both faceVertexCounts and
            // faceVertexIndices are not known to be varying.
            _IsVarying(prim,
                       UsdGeomTokens->holeIndices,
                       HdChangeTracker::DirtyTopology,
                       UsdImagingTokens->usdVaryingTopology,
                       timeVaryingBits,
                       /*isInherited*/false);
        }
    }
}

void
UsdImagingMeshAdapter::UpdateForTime(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdTimeCode time,
                                     HdDirtyBits requestedBits,
                                     UsdImagingInstancerContext const*
                                         instancerContext)
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);

    UsdImagingValueCache* valueCache = _GetValueCache();
    PrimvarInfoVector& primvars = valueCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        VtValue& topology = valueCache->GetTopology(cachePath);
        _GetMeshTopology(prim, &topology, time);
    }

    if (requestedBits & HdChangeTracker::DirtyPoints) {
        VtValue& points = valueCache->GetPoints(cachePath);
        _GetPoints(prim, &points, time);
        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = HdTokens->points;
        primvar.interpolation = UsdGeomTokens->vertex;
        _MergePrimvar(primvar, &primvars);
    }

    // Subdiv tags are only needed if the mesh is refined.  So
    // there's no need to fetch the data if the prim isn't refined.
    if (_delegate->IsRefined(cachePath)) {
        if (requestedBits & HdChangeTracker::DirtySubdivTags) {
            SubdivTags& tags = valueCache->GetSubdivTags(cachePath);
            _GetSubdivTags(prim, &tags, time);
        }
    }
}

HdDirtyBits
UsdImagingMeshAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      TfToken const& propertyName)
{
    if(propertyName == UsdGeomTokens->points)
        return HdChangeTracker::DirtyPoints;

    // TODO: support sparse topology and subdiv tag changes

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

// -------------------------------------------------------------------------- //
// Private IO Helpers
// -------------------------------------------------------------------------- //

void
UsdImagingMeshAdapter::_GetMeshTopology(UsdPrim const& prim,
                                         VtValue* topo,
                                         UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    TfToken schemeToken;
    _GetPtr(prim, UsdGeomTokens->subdivisionScheme, time, &schemeToken);

    *topo = HdMeshTopology(
        schemeToken,
        _Get<TfToken>(prim, UsdGeomTokens->orientation, time),
        _Get<VtIntArray>(prim, UsdGeomTokens->faceVertexCounts, time),
        _Get<VtIntArray>(prim, UsdGeomTokens->faceVertexIndices, time),
        _Get<VtIntArray>(prim, UsdGeomTokens->holeIndices, time));
}

void
UsdImagingMeshAdapter::_GetPoints(UsdPrim const& prim,
                                   VtValue* value,
                                   UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    if (!prim.GetAttribute(UsdGeomTokens->points).Get(value, time)) {
        *value = VtVec3fArray();
    }
}


void
UsdImagingMeshAdapter::_GetSubdivTags(UsdPrim const& prim,
                                       SubdivTags* tags,
                                       UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if(!prim.IsA<UsdGeomMesh>())
        return;

    TfToken token; VtIntArray iarray; VtFloatArray farray;

    _GetPtr(prim, UsdGeomTokens->interpolateBoundary, time, &token);
    tags->SetVertexInterpolationRule(token);

    auto meshPrim = UsdGeomMesh(prim);
    auto fvLinearInterpAttr = meshPrim.GetFaceVaryingLinearInterpolationAttr();
    fvLinearInterpAttr.Get(&token, time); 

    tags->SetFaceVaryingInterpolationRule(token);

    // XXX uncomment after fixing USD schema

    //_GetPtr(prim, UsdGeomTokens->creaseMethod, time, &token);
    //tags->SetCreaseMethod(token);

    _GetPtr(prim, UsdGeomTokens->triangleSubdivisionRule, time, &token);
    tags->SetTriangleSubdivision(token);

    _GetPtr(prim, UsdGeomTokens->creaseIndices, time, &iarray);
    tags->SetCreaseIndices(iarray);

    _GetPtr(prim, UsdGeomTokens->creaseLengths, time, &iarray);
    tags->SetCreaseLengths(iarray);

    _GetPtr(prim, UsdGeomTokens->creaseSharpnesses, time, &farray);
    tags->SetCreaseWeights(farray);

    _GetPtr(prim, UsdGeomTokens->cornerIndices, time, &iarray);
    tags->SetCornerIndices(iarray);

    _GetPtr(prim, UsdGeomTokens->cornerSharpnesses, time, &farray);
    tags->SetCornerWeights(farray);

    _GetPtr(prim, UsdGeomTokens->holeIndices, time, &iarray);
    tags->SetHoleIndices(iarray);
}


PXR_NAMESPACE_CLOSE_SCOPE

