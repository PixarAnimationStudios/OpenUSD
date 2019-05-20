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
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/geomSubset.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/subset.h"
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
    // Check for any UsdGeomSubset children and record this adapter as
    // the delegate for their paths.
    if (UsdGeomImageable imageable = UsdGeomImageable(prim)) {
        for (const UsdGeomSubset &subset:
             UsdGeomSubset::GetAllGeomSubsets(imageable)) {
            index->AddPrimInfo(subset.GetPath(),
                               subset.GetPrim().GetParent(),
                               shared_from_this());
            // Ensure the bound material has been populated.
            if (UsdPrim materialPrim =
                prim.GetStage()->GetPrimAtPath(
                GetMaterialId(subset.GetPrim()))) {
                UsdImagingPrimAdapterSharedPtr materialAdapter =
                    index->GetMaterialAdapter(materialPrim);
                if (materialAdapter) {
                    materialAdapter->Populate(materialPrim, index, nullptr);
                }
            }
        }
    }
    return _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialId(prim), instancerContext);
}


void
UsdImagingMeshAdapter::TrackVariability(UsdPrim const& prim,
                                        SdfPath const& cachePath,
                                        HdDirtyBits* timeVaryingBits,
                                        UsdImagingInstancerContext const* 
                                            instancerContext) const
{
    // Early return when called on behalf of a UsdGeomSubset.
    if (UsdGeomSubset(prim)) {
        return;
    }

    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.

    // Discover time-varying points.
    _IsVarying(prim,
               UsdGeomTokens->points,
               HdChangeTracker::DirtyPoints,
               UsdImagingTokens->usdVaryingPrimvar,
               timeVaryingBits,
               /*isInherited*/false);

    // Discover time-varying primvars:normals, and if that attribute
    // doesn't exist also check for time-varying normals.
    // Only do this for polygonal meshes.

    TfToken schemeToken;
    _GetPtr(prim, UsdGeomTokens->subdivisionScheme,
            UsdTimeCode::EarliestTime(), &schemeToken);

    if (schemeToken == PxOsdOpenSubdivTokens->none) {
        bool normalsExists = false;
        _IsVarying(prim,
                UsdImagingTokens->primvarsNormals,
                HdChangeTracker::DirtyNormals,
                UsdImagingTokens->usdVaryingNormals,
                timeVaryingBits,
                /*isInherited*/false,
                &normalsExists);
        if (!normalsExists) {
            _IsVarying(prim,
                    UsdGeomTokens->normals,
                    HdChangeTracker::DirtyNormals,
                    UsdImagingTokens->usdVaryingNormals,
                    timeVaryingBits,
                    /*isInherited*/false);
        }
    }

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

    // Discover time-varying UsdGeomSubset children.
    if (UsdGeomImageable imageable = UsdGeomImageable(prim)) {
        for (const UsdGeomSubset &subset:
             UsdGeomSubset::GetAllGeomSubsets(imageable)) {
            _IsVarying(subset.GetPrim(),
                       UsdGeomTokens->elementType,
                       HdChangeTracker::DirtyTopology,
                       UsdImagingTokens->usdVaryingPrimvar,
                       timeVaryingBits,
                       /*isInherited*/false);
            _IsVarying(subset.GetPrim(),
                       UsdGeomTokens->indices,
                       HdChangeTracker::DirtyTopology,
                       UsdImagingTokens->usdVaryingPrimvar,
                       timeVaryingBits,
                       /*isInherited*/false);
        }
    }
}

void
UsdImagingMeshAdapter::MarkDirty(UsdPrim const& prim,
                                 SdfPath const& cachePath,
                                 HdDirtyBits dirty,
                                 UsdImagingIndexProxy* index)
{
    // Check if this is invoked on behalf of a UsdGeomSubset of
    // a parent mesh; if so, dirty the parent instead.
    if (cachePath.IsPrimPath() && cachePath.GetParentPath() == prim.GetPath()) {
        index->MarkRprimDirty(cachePath.GetParentPath(), dirty);
    } else {
        index->MarkRprimDirty(cachePath, dirty);
    }
}

void
UsdImagingMeshAdapter::MarkRefineLevelDirty(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
    // Check if this is invoked on behalf of a UsdGeomSubset of
    // a parent mesh; if so, there's nothing to do.
    if (cachePath.IsPrimPath() && cachePath.GetParentPath() == prim.GetPath()) {
        return;
    }
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyDisplayStyle);
}


void
UsdImagingMeshAdapter::_RemovePrim(SdfPath const& cachePath,
                                   UsdImagingIndexProxy* index)
{
    // Check if this is invoked on behalf of a UsdGeomSubset,
    // in which case there will be no rprims associated with
    // the cache path.  If so, dirty parent topology.
    if (index->HasRprim(cachePath)) {
        index->RemoveRprim(cachePath);
    } else {
        index->MarkRprimDirty(cachePath.GetParentPath(),
                              HdChangeTracker::DirtyTopology);
    }
}

bool
UsdImagingMeshAdapter::_IsBuiltinPrimvar(TfToken const& primvarName) const
{
    return (primvarName == HdTokens->normals) ||
        UsdImagingGprimAdapter::_IsBuiltinPrimvar(primvarName);
}

void
UsdImagingMeshAdapter::UpdateForTime(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdTimeCode time,
                                     HdDirtyBits requestedBits,
                                     UsdImagingInstancerContext const*
                                         instancerContext) const
{
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[UpdateForTime] Mesh path: <%s>\n",
                                     prim.GetPath().GetText());

    // Check if invoked on behalf of a UsdGeomSubset; if so, do nothing.
    if (cachePath.GetParentPath() == prim.GetPath()) {
        return;
    }

    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);

    UsdImagingValueCache* valueCache = _GetValueCache();
    HdPrimvarDescriptorVector& primvars = valueCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        VtValue& topology = valueCache->GetTopology(cachePath);
        _GetMeshTopology(prim, &topology, time);
    }

    if (requestedBits & HdChangeTracker::DirtyNormals) {
        TfToken schemeToken;
        _GetPtr(prim, UsdGeomTokens->subdivisionScheme, time, &schemeToken);
        // Only populate normals for polygonal meshes.
        if (schemeToken == PxOsdOpenSubdivTokens->none) {
            // First check for "primvars:normals"
            UsdGeomPrimvarsAPI primvarsApi(prim);
            UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
                    UsdImagingTokens->primvarsNormals);
            if (pv) {
                _ComputeAndMergePrimvar(prim, cachePath, pv, time, valueCache);
            } else {
                UsdGeomMesh mesh(prim);
                VtVec3fArray normals;
                if (mesh.GetNormalsAttr().Get(&normals, time)) {
                    _MergePrimvar(&primvars,
                        UsdGeomTokens->normals,
                        _UsdToHdInterpolation(mesh.GetNormalsInterpolation()),
                        HdPrimvarRoleTokens->normal);
                    valueCache->GetNormals(cachePath) = VtValue(normals);
                }
            }
        }
    }

    // Subdiv tags are only needed if the mesh is refined.  So
    // there's no need to fetch the data if the prim isn't refined.
    if (_IsRefined(cachePath)) {
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

    // Check for UsdGeomSubset changes.
    // Do the cheaper property name filtering first.
    if ((propertyName == UsdGeomTokens->elementType ||
         propertyName == UsdGeomTokens->indices) &&
         cachePath.GetPrimPath().GetParentPath() == prim.GetPath()) {
        return HdChangeTracker::DirtyTopology;
    }

    // TODO: support sparse topology and subdiv tag changes
    // (Note that a change in subdivision scheme means we need to re-track
    // the variability of the normals...)

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

// -------------------------------------------------------------------------- //
// Private IO Helpers
// -------------------------------------------------------------------------- //

void
UsdImagingMeshAdapter::_GetMeshTopology(UsdPrim const& prim,
                                         VtValue* topo,
                                         UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    TfToken schemeToken;
    _GetPtr(prim, UsdGeomTokens->subdivisionScheme, time, &schemeToken);

    HdMeshTopology meshTopo(
        schemeToken,
        _Get<TfToken>(prim, UsdGeomTokens->orientation, time),
        _Get<VtIntArray>(prim, UsdGeomTokens->faceVertexCounts, time),
        _Get<VtIntArray>(prim, UsdGeomTokens->faceVertexIndices, time),
        _Get<VtIntArray>(prim, UsdGeomTokens->holeIndices, time));

    // Convert UsdGeomSubsets to HdGeomSubsets.
    if (UsdGeomImageable imageable = UsdGeomImageable(prim)) {
        HdGeomSubsets geomSubsets;
        for (const UsdGeomSubset &subset:
             UsdGeomSubset::GetAllGeomSubsets(imageable)) {
             VtIntArray indices;
             TfToken elementType;
             if (subset.GetElementTypeAttr().Get(&elementType) &&
                 subset.GetIndicesAttr().Get(&indices)) {
                 if (elementType == UsdGeomTokens->face) {
                     geomSubsets.emplace_back(
                        HdGeomSubset {
                            HdGeomSubset::TypeFaceSet,
                            subset.GetPath(),
                            GetMaterialId(subset.GetPrim()),
                            indices });
                 }
             }
        }
        if (!geomSubsets.empty()) {
            meshTopo.SetGeomSubsets(geomSubsets);
        }
    }

    topo->Swap(meshTopo);
}

void
UsdImagingMeshAdapter::_GetSubdivTags(UsdPrim const& prim,
                                       SubdivTags* tags,
                                       UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if(!prim.IsA<UsdGeomMesh>())
        return;

    TfToken interpolationRule =
        _Get<TfToken>(prim, UsdGeomTokens->interpolateBoundary, time);
    if (interpolationRule.IsEmpty()) {
        interpolationRule = UsdGeomTokens->edgeAndCorner;
    }
    tags->SetVertexInterpolationRule(interpolationRule);

    TfToken faceVaryingRule = _Get<TfToken>(
        prim, UsdGeomTokens->faceVaryingLinearInterpolation, time);
    if (faceVaryingRule.IsEmpty()) {
        faceVaryingRule = UsdGeomTokens->cornersPlus1;
    }
    tags->SetFaceVaryingInterpolationRule(faceVaryingRule);

    // XXX uncomment after fixing USD schema
    // TfToken creaseMethod =
    //     _Get<TfToken>(prim, UsdGeomTokens->creaseMethod, time);
    // tags->SetCreaseMethod(creaseMethod);

    TfToken triangleRule =
        _Get<TfToken>(prim, UsdGeomTokens->triangleSubdivisionRule, time);
    if (triangleRule.IsEmpty()) {
        triangleRule = UsdGeomTokens->catmullClark;
    }
    tags->SetTriangleSubdivision(triangleRule);

    VtIntArray creaseIndices =
        _Get<VtIntArray>(prim, UsdGeomTokens->creaseIndices, time);
    tags->SetCreaseIndices(creaseIndices);

    VtIntArray creaseLengths =
        _Get<VtIntArray>(prim, UsdGeomTokens->creaseLengths, time);
    tags->SetCreaseLengths(creaseLengths);

    VtFloatArray creaseSharpnesses =
        _Get<VtFloatArray>(prim, UsdGeomTokens->creaseSharpnesses, time);
    tags->SetCreaseWeights(creaseSharpnesses);

    VtIntArray cornerIndices =
        _Get<VtIntArray>(prim, UsdGeomTokens->cornerIndices, time);
    tags->SetCornerIndices(cornerIndices);

    VtFloatArray cornerSharpnesses =
        _Get<VtFloatArray>(prim, UsdGeomTokens->cornerSharpnesses, time);
    tags->SetCornerWeights(cornerSharpnesses);
}


PXR_NAMESPACE_CLOSE_SCOPE

