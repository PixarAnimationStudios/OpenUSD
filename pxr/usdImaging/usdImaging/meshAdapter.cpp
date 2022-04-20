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

#include "pxr/usdImaging/usdImaging/dataSourceMesh.h"
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

TfTokenVector
UsdImagingMeshAdapter::GetImagingSubprims()
{
    return { TfToken() };
}

TfToken
UsdImagingMeshAdapter::GetImagingSubprimType(TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->mesh;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingMeshAdapter::GetImagingSubprimData(
        TfToken const& subprim,
        UsdPrim const& prim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceMeshPrim::New(
            prim.GetPath(),
            prim,
            stageGlobals);
    }
    return nullptr;
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
    SdfPath cachePath = _AddRprim(HdPrimTypeTokens->mesh, prim, index,
                                  GetMaterialUsdPath(prim), instancerContext);

    // Check for any UsdGeomSubset children of familyType 
    // UsdShadeTokens->materialBind and record dependencies for them.
    for (const UsdGeomSubset &subset:
         UsdShadeMaterialBindingAPI(prim).GetMaterialBindSubsets()) {
        index->AddDependency(cachePath, subset.GetPrim());

        // Ensure the bound material has been populated.
        UsdPrim materialPrim = prim.GetStage()->GetPrimAtPath(
                GetMaterialUsdPath(subset.GetPrim()));
        if (materialPrim) {
            UsdImagingPrimAdapterSharedPtr materialAdapter =
                index->GetMaterialAdapter(materialPrim);
            if (materialAdapter) {
                materialAdapter->Populate(materialPrim, index, nullptr);
                // We need to register a dependency on the material prim so
                // that geometry is updated when the material is
                // (specifically, DirtyMaterialId).
                // XXX: Eventually, it would be great to push this into hydra.
                index->AddDependency(cachePath, materialPrim);
            }
        }
    }

    return cachePath;
}


void
UsdImagingMeshAdapter::TrackVariability(UsdPrim const& prim,
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
            UsdGeomPrimvar pv = _GetInheritedPrimvar(prim, HdTokens->normals);
            if (pv && pv.ValueMightBeTimeVarying()) {
                *timeVaryingBits |= HdChangeTracker::DirtyNormals;
                HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingNormals);
                normalsExists = true;
            }
        }
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

    // Discover time-varying UsdGeomSubset children, which indicates that the
    // topology must be treated as time varying. Only do these checks until
    // we determine the topology is time varying, because each call to
    // _IsVarying will clear the topology dirty bit if it's already set.
    if (((*timeVaryingBits) & HdChangeTracker::DirtyTopology) == 0) {
        for (const UsdGeomSubset &subset:
             UsdShadeMaterialBindingAPI(prim).GetMaterialBindSubsets()) {

            // If the topology dirty flag is already set, exit the loop.
            if (((*timeVaryingBits) & HdChangeTracker::DirtyTopology) != 0){
                break;
            }

            if (!_IsVarying(subset.GetPrim(),
                            UsdGeomTokens->elementType,
                            HdChangeTracker::DirtyTopology,
                            UsdImagingTokens->usdVaryingPrimvar,
                            timeVaryingBits,
                            /*isInherited*/false)) {
                // Only do this check if the elementType is not already
                // known to be varying.
                _IsVarying(subset.GetPrim(),
                           UsdGeomTokens->indices,
                           HdChangeTracker::DirtyTopology,
                           UsdImagingTokens->usdVaryingPrimvar,
                           timeVaryingBits,
                           /*isInherited*/false);
            }
        }
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

    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);

    if (requestedBits & HdChangeTracker::DirtyNormals) {
        UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();
        HdPrimvarDescriptorVector& primvars = 
            primvarDescCache->GetPrimvars(cachePath);

        TfToken schemeToken;
        _GetPtr(prim, UsdGeomTokens->subdivisionScheme, time, &schemeToken);
        // Only populate normals for polygonal meshes.
        if (schemeToken == PxOsdOpenSubdivTokens->none) {
            // First check for "primvars:normals"
            UsdGeomPrimvarsAPI primvarsApi(prim);
            UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
                    UsdImagingTokens->primvarsNormals);
            if (!pv) {
                // If it's not found locally, see if it's inherited
                pv = _GetInheritedPrimvar(prim, HdTokens->normals);
            }

            if (pv) {
                _ComputeAndMergePrimvar(prim, pv, time, &primvars);
            } else {
                UsdGeomMesh mesh(prim);
                VtVec3fArray normals;
                if (mesh.GetNormalsAttr().Get(&normals, time)) {
                    _MergePrimvar(&primvars,
                        UsdGeomTokens->normals,
                        _UsdToHdInterpolation(mesh.GetNormalsInterpolation()),
                        HdPrimvarRoleTokens->normal);
                } else {
                    _RemovePrimvar(&primvars, UsdGeomTokens->normals);
                }
            }
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
    // XXX: Verify that this is being called on a GeomSubset prim?
    if (propertyName == UsdGeomTokens->elementType ||
        propertyName == UsdGeomTokens->indices) {
        return HdChangeTracker::DirtyTopology;
    }

    if (propertyName == UsdGeomTokens->subdivisionScheme) {
        // (Note that a change in subdivision scheme means we need to re-track
        // the variability of the normals...)
        return HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyNormals;
    }

    if (propertyName == UsdGeomTokens->faceVertexCounts ||
        propertyName == UsdGeomTokens->faceVertexIndices ||
        propertyName == UsdGeomTokens->holeIndices ||
        propertyName == UsdGeomTokens->orientation) {
        return HdChangeTracker::DirtyTopology;
    }

    if (propertyName == UsdGeomTokens->interpolateBoundary ||
        propertyName == UsdGeomTokens->faceVaryingLinearInterpolation ||
        propertyName == UsdGeomTokens->triangleSubdivisionRule ||
        propertyName == UsdGeomTokens->creaseIndices ||
        propertyName == UsdGeomTokens->creaseLengths ||
        propertyName == UsdGeomTokens->creaseSharpnesses ||
        propertyName == UsdGeomTokens->cornerIndices ||
        propertyName == UsdGeomTokens->cornerSharpnesses) {
        // XXX UsdGeomTokens->creaseMethod, when we add support for that.
        return HdChangeTracker::DirtySubdivTags;
    }

    if (propertyName == UsdGeomTokens->normals) {
        UsdGeomPointBased pb(prim);
        return UsdImagingPrimAdapter::_ProcessNonPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdTokens->normals,
            _UsdToHdInterpolation(pb.GetNormalsInterpolation()),
            HdChangeTracker::DirtyNormals);
    }
    if (propertyName == UsdImagingTokens->primvarsNormals) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
                prim, cachePath, propertyName, HdChangeTracker::DirtyNormals);
    }

    // Handle attributes that are treated as "built-in" primvars.
    if (propertyName == UsdGeomTokens->normals) {
        UsdGeomMesh mesh(prim);
        return UsdImagingPrimAdapter::_ProcessNonPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdTokens->normals,
            _UsdToHdInterpolation(mesh.GetNormalsInterpolation()),
            HdChangeTracker::DirtyNormals);
    }
    // Handle prefixed primvars that use special dirty bits.
    else if (propertyName == UsdImagingTokens->primvarsNormals) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
                prim, cachePath, propertyName, HdChangeTracker::DirtyNormals);
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/ 
VtValue
UsdImagingMeshAdapter::GetTopology(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdTimeCode time) const
{
    TRACE_FUNCTION();
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
    HdGeomSubsets geomSubsets;
    for (const UsdGeomSubset &subset:
         UsdShadeMaterialBindingAPI(prim).GetMaterialBindSubsets()) {
        VtIntArray indices;
        TfToken elementType;
        if (subset.GetElementTypeAttr().Get(&elementType) &&
            subset.GetIndicesAttr().Get(&indices, time)) {
            if (elementType == UsdGeomTokens->face) {
                geomSubsets.emplace_back(
                   HdGeomSubset {
                       HdGeomSubset::TypeFaceSet,
                       subset.GetPath(),
                       GetMaterialUsdPath(subset.GetPrim()),
                       indices });
            }
        }
    }
    if (!geomSubsets.empty()) {
        meshTopo.SetGeomSubsets(geomSubsets);
    }

    return VtValue(meshTopo);
}

PxOsdSubdivTags
UsdImagingMeshAdapter::GetSubdivTags(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    PxOsdSubdivTags tags;

    if(!prim.IsA<UsdGeomMesh>()) {
        return tags;
    }

    TfToken interpolationRule =
        _Get<TfToken>(prim, UsdGeomTokens->interpolateBoundary, time);
    if (interpolationRule.IsEmpty()) {
        interpolationRule = UsdGeomTokens->edgeAndCorner;
    }
    tags.SetVertexInterpolationRule(interpolationRule);

    TfToken faceVaryingRule = _Get<TfToken>(
        prim, UsdGeomTokens->faceVaryingLinearInterpolation, time);
    if (faceVaryingRule.IsEmpty()) {
        faceVaryingRule = UsdGeomTokens->cornersPlus1;
    }
    tags.SetFaceVaryingInterpolationRule(faceVaryingRule);

    // XXX uncomment after fixing USD schema
    // TfToken creaseMethod =
    //     _Get<TfToken>(prim, UsdGeomTokens->creaseMethod, time);
    // tags->SetCreaseMethod(creaseMethod);

    TfToken triangleRule =
        _Get<TfToken>(prim, UsdGeomTokens->triangleSubdivisionRule, time);
    if (triangleRule.IsEmpty()) {
        triangleRule = UsdGeomTokens->catmullClark;
    }
    tags.SetTriangleSubdivision(triangleRule);

    VtIntArray creaseIndices =
        _Get<VtIntArray>(prim, UsdGeomTokens->creaseIndices, time);
    tags.SetCreaseIndices(creaseIndices);

    VtIntArray creaseLengths =
        _Get<VtIntArray>(prim, UsdGeomTokens->creaseLengths, time);
    tags.SetCreaseLengths(creaseLengths);

    VtFloatArray creaseSharpnesses =
        _Get<VtFloatArray>(prim, UsdGeomTokens->creaseSharpnesses, time);
    tags.SetCreaseWeights(creaseSharpnesses);

    VtIntArray cornerIndices =
        _Get<VtIntArray>(prim, UsdGeomTokens->cornerIndices, time);
    tags.SetCornerIndices(cornerIndices);

    VtFloatArray cornerSharpnesses =
        _Get<VtFloatArray>(prim, UsdGeomTokens->cornerSharpnesses, time);
    tags.SetCornerWeights(cornerSharpnesses);

    return tags;
}

/*virtual*/
VtValue
UsdImagingMeshAdapter::Get(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           TfToken const &key,
                           UsdTimeCode time,
                           VtIntArray *outIndices) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (key == HdTokens->normals) {
        TfToken schemeToken;
        _GetPtr(prim, UsdGeomTokens->subdivisionScheme, time, &schemeToken);

        // Only populate normals for polygonal meshes.
        if (schemeToken == PxOsdOpenSubdivTokens->none) {
            // First check for "primvars:normals"
            UsdGeomPrimvarsAPI primvarsApi(prim);
            UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
                    UsdImagingTokens->primvarsNormals);
            if (!pv) {
                // If it's not found locally, see if it's inherited
                pv = _GetInheritedPrimvar(prim, HdTokens->normals);
            }

            VtValue value;

            if (outIndices) {
                if (pv && pv.Get(&value, time)) {
                    pv.GetIndices(outIndices, time);
                    return value;
                }
            } else if (pv && pv.ComputeFlattened(&value, time)) {
                return value;
            }

            // If there's no "primvars:normals",
            // fall back to UsdGeomMesh's "normals" attribute. 
            UsdGeomMesh mesh(prim);
            VtVec3fArray normals;
            if (mesh && mesh.GetNormalsAttr().Get(&normals, time)) {
                value = normals;
                return value;
            }
        }
    }

    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

PXR_NAMESPACE_CLOSE_SCOPE
