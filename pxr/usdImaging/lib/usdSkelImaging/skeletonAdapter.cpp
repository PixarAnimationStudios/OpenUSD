//
// Copyright 2018 Pixar
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
#include "pxr/usdImaging/usdSkelImaging/skeletonAdapter.h"

#include "pxr/usdImaging/usdSkelImaging/utils.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdSkelImagingSkeletonAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}


UsdSkelImagingSkeletonAdapter::~UsdSkelImagingSkeletonAdapter()
{}


bool
UsdSkelImagingSkeletonAdapter::IsSupported(
    const UsdImagingIndexProxy* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}


SdfPath
UsdSkelImagingSkeletonAdapter::Populate(
    const UsdPrim& prim,
    UsdImagingIndexProxy* index,
    const UsdImagingInstancerContext* instancerContext)
{
    if(!TF_VERIFY(prim.IsA<UsdSkelSkeleton>())) {
        return SdfPath();
    }

    auto skelData = std::make_shared<_SkelData>();
    skelData->skelQuery = _skelCache.GetSkelQuery(UsdSkelSkeleton(prim));

    _skelDataCache[prim.GetPath()] = skelData;

    SdfPath instancer = instancerContext ?
        instancerContext->instancerId : SdfPath();

    index->InsertRprim(HdPrimTypeTokens->mesh, prim.GetPath(),
                       instancer, prim, shared_from_this());
    
    return prim.GetPath();
}

void
UsdSkelImagingSkeletonAdapter::TrackVariability(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    HdDirtyBits* timeVaryingBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.

    // Why is this OK? 
    // Either the value is unvarying, in which case the time ordinate doesn't
    // matter; or the value is varying, in which case we will update it upon
    // first call to Delegate::SetTime(). 
    UsdTimeCode time(1.0);

    _SkelData* skelData = _GetSkelData(cachePath);
    if (!TF_VERIFY(skelData)) {
        return;
    }

    UsdImagingValueCache* valueCache = _GetValueCache();

    if (!_IsVarying(prim,
                    UsdGeomTokens->primvarsDisplayColor,
                    HdChangeTracker::DirtyPrimvar,
                    UsdImagingTokens->usdVaryingPrimvar,
                    timeVaryingBits,
                    false)) {
        // Only do this second check if the displayColor isn't already known
        // to be varying.
        _IsVarying(prim,
                   UsdGeomTokens->primvarsDisplayOpacity,
                   HdChangeTracker::DirtyPrimvar,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits,
                   false);
    }

    // Discover time-varying extent.
    _IsVarying(prim,
               UsdGeomTokens->extent,
               HdChangeTracker::DirtyExtent,
               UsdImagingTokens->usdVaryingExtent,
               timeVaryingBits,
               false);

    // Discover time-varying points.
    if (const UsdSkelAnimQuery& animQuery =
        skelData->skelQuery.GetAnimQuery()) {

        if(animQuery.JointTransformsMightBeTimeVarying()) {
            (*timeVaryingBits) |= HdChangeTracker::DirtyPoints;
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingPrimvar);
        }
    }

    // Discover time-varying transforms.
    _IsTransformVarying(prim,
                        HdChangeTracker::DirtyTransform,
                        UsdImagingTokens->usdVaryingXform,
                        timeVaryingBits);

    valueCache->GetVisible(cachePath) = GetVisible(prim, time);
    // Discover time-varying visibility.
    _IsVarying(prim,
               UsdGeomTokens->visibility,
               HdChangeTracker::DirtyVisibility,
               UsdImagingTokens->usdVaryingVisibility,
               timeVaryingBits,
               true);

    TfToken purpose = _GetPurpose(prim, time);
    // Empty purpose means there is no opintion, fall back to default.
    if (purpose.IsEmpty())
        purpose = UsdGeomTokens->default_;
    valueCache->GetPurpose(cachePath) = purpose;
}


void
UsdSkelImagingSkeletonAdapter::UpdateForTime(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[UpdateForTime] Skeleton path: <%s>\n",
                                     prim.GetPath().GetText());

    _SkelData* skelData = _GetSkelData(cachePath);
    if (!TF_VERIFY(skelData)) {
        return;
    }

    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        valueCache->GetTopology(cachePath) =
            skelData->ComputeTopologyAndRestState();
    }

    if (requestedBits & HdChangeTracker::DirtyPoints) {

        valueCache->GetPoints(cachePath) = skelData->ComputePoints(time);
    }

    if (requestedBits & HdChangeTracker::DirtyTransform) {
        valueCache->GetTransform(cachePath) = GetTransform(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyExtent) {
        valueCache->GetExtent(cachePath) = _GetExtent(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyVisibility) {
        valueCache->GetVisible(cachePath) = GetVisible(prim, time);
    }
    
    if (requestedBits & HdChangeTracker::DirtyPrimvar) {

        // Expose points as a primvar.
        _MergePrimvar(&valueCache->GetPrimvars(cachePath),
                      HdTokens->points,
                      HdInterpolationVertex,
                      HdPrimvarRoleTokens->point);
        
        valueCache->GetColor(cachePath) =
            VtValue(_GetColorAndOpacity(prim, time));
        _MergePrimvar(&valueCache->GetPrimvars(cachePath),
                      HdTokens->color,
                      HdInterpolationConstant,
                      HdPrimvarRoleTokens->color);
    }

    if (requestedBits & HdChangeTracker::DirtyDoubleSided) {
        valueCache->GetDoubleSided(cachePath) = true;
    }

    if (requestedBits & HdChangeTracker::DirtyMaterialId) {
        // XXX: The bone mesh does not need a material.
        valueCache->GetMaterialId(cachePath) = SdfPath();
    }
}


HdDirtyBits
UsdSkelImagingSkeletonAdapter::ProcessPropertyChange(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    const TfToken& propertyName)
{
    if (propertyName == UsdGeomTokens->visibility ||
        propertyName == UsdGeomTokens->purpose)
        return HdChangeTracker::DirtyVisibility;
    else if (propertyName == UsdGeomTokens->extent)
        return HdChangeTracker::DirtyExtent;
    else if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName))
        return HdChangeTracker::DirtyTransform;

    // TODO: Perform granular tracking of only the relevant properties.
    // The main problem with this is that we can't easily track changes
    // related to the inherited SkelAnimation.
    return HdChangeTracker::AllDirty;
}


void
UsdSkelImagingSkeletonAdapter::MarkDirty(const UsdPrim& prim,
                                          const SdfPath& cachePath,
                                          HdDirtyBits dirty,
                                          UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, dirty);
}


void
UsdSkelImagingSkeletonAdapter::MarkTransformDirty(const UsdPrim& prim,
                                                   const SdfPath& cachePath,
                                                   UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyTransform);
}


void
UsdSkelImagingSkeletonAdapter::MarkVisibilityDirty(const UsdPrim& prim,
                                                    const SdfPath& cachePath,
                                                    UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyVisibility);
}


void
UsdSkelImagingSkeletonAdapter::MarkMaterialDirty(const UsdPrim& prim,
                                                  const SdfPath& cachePath,
                                                  UsdImagingIndexProxy* index)
{
    // If the Usd material changed, it could mean the primvar set also changed
    // Hydra doesn't currently manage detection and propagation of these
    // changes, so we must mark the rprim dirty.
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyMaterialId);
}


void
UsdSkelImagingSkeletonAdapter::_RemovePrim(const SdfPath& cachePath,
                                           UsdImagingIndexProxy* index)
{
    index->RemoveRprim(cachePath);

    _skelDataCache.erase(cachePath);

    // TODO: Clearing the entire cache is excessive, but correct.
    _skelCache.Clear();
}


GfRange3d
UsdSkelImagingSkeletonAdapter::_GetExtent(const UsdPrim& prim,
                                          UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    UsdGeomBoundable boundable(prim);
    VtVec3fArray extent;
    if (boundable.GetExtentAttr().Get(&extent, time)) {
        // Note:
        // Usd stores extent as 2 float vecs. We do an implicit 
        // conversion to doubles
        return GfRange3d(extent[0], extent[1]);
    } else {
        // Return empty range if no value was found.
        return GfRange3d();
    }
}


TfToken
UsdSkelImagingSkeletonAdapter::_GetPurpose(const UsdPrim& prim,
                                           UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    // PERFORMANCE: Make this more efficient, see http://bug/90497
    return UsdGeomImageable(prim).ComputePurpose();
}


namespace {


GfVec3f
_GetSkeletonDisplayColor(const UsdGeomPrimvarsAPI& primvars,
                         UsdTimeCode time)
{
    if (UsdGeomPrimvar pv = primvars.GetPrimvar(
            UsdGeomTokens->primvarsDisplayColor)) {
        // May be stored as a constant.
        GfVec3f color;
        if (pv.Get(&color, time))
            return color;

        // May be stored as an array holding a single elem.
        VtVec3fArray colors;
        if (pv.Get(&colors, time) && colors.size() == 1)
            return colors[0];
    }
    return GfVec3f(0.5f);
}


float
_GetSkeletonDisplayOpacity(const UsdGeomPrimvarsAPI& primvars,
                           UsdTimeCode time)
{
    if (UsdGeomPrimvar pv = primvars.GetPrimvar(
            UsdGeomTokens->primvarsDisplayOpacity)) {
        // May be stored as a constant.
        float opacity;
        if (pv.Get(&opacity, time))
            return opacity;

        // May be stored as an array holding a single elem.
        VtFloatArray opacities;
        if (pv.Get(&opacities, time) && opacities.size() == 1)
            return opacities[0];
    }
    return 1.0f;
}


} // namespace


GfVec4f
UsdSkelImagingSkeletonAdapter::_GetColorAndOpacity(const UsdPrim& prim,
                                                   UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    UsdGeomPrimvarsAPI primvars(prim);

    GfVec3f color = _GetSkeletonDisplayColor(primvars, time);
    float opacity = _GetSkeletonDisplayOpacity(primvars, time);
    
    return GfVec4f(color[0], color[1], color[2], opacity);
}


UsdSkelImagingSkeletonAdapter::_SkelData*
UsdSkelImagingSkeletonAdapter::_GetSkelData(const SdfPath& cachePath) const
{
    auto it = _skelDataCache.find(cachePath);
    return it != _skelDataCache.end() ? it->second.get() : nullptr;
}


HdMeshTopology
UsdSkelImagingSkeletonAdapter::_SkelData::ComputeTopologyAndRestState()
{
    HdMeshTopology meshTopology;

    size_t numPoints = 0;
    UsdSkelImagingComputeBoneTopology(skelQuery.GetTopology(),
                                      &meshTopology,
                                      &numPoints);

    // While computing topology, we also compute the 'rest pose'
    // of the bone mesh, along with joint influences.
    VtMatrix4dArray xforms; 
    skelQuery.GetJointWorldBindTransforms(&xforms);

    _numJoints = xforms.size();

    UsdSkelImagingComputeBonePoints(skelQuery.GetTopology(), xforms,
                                    numPoints, &_boneMeshPoints);

    UsdSkelImagingComputeBoneJointIndices(skelQuery.GetTopology(),
                                          &_boneMeshJointIndices, numPoints);

    // Transform points by their inverse bind transforms. This puts bone points
    // in the right space so that when we compute bone points on frame changes,
    // we only need to consider joint transforms (and can disregard bind
    // transforms). This is only possible since each point of the mesh is 
    // influenced by only one joint.
    if (TF_VERIFY(_boneMeshPoints.size() == _boneMeshJointIndices.size())) {

        for (auto& xf : xforms) {
            xf = xf.GetInverse();
        }

        const GfMatrix4d* invBindXforms = xforms.cdata();
        const int* jointIndices = _boneMeshJointIndices.cdata();
        GfVec3f* points = _boneMeshPoints.data();
        for (size_t i = 0; i < _boneMeshPoints.size(); ++i) {
            int jointIdx = jointIndices[i];
            TF_DEV_AXIOM(jointIdx >= 0 &&
                         static_cast<size_t>(jointIdx) < xforms.size());
            points[i] = invBindXforms[jointIdx].Transform(points[i]);
        }
    }

    return meshTopology;
}


VtVec3fArray
UsdSkelImagingSkeletonAdapter::_SkelData::ComputePoints(
    UsdTimeCode time) const
{
    // Initial bone points were stored pre-transformed by the *inverse* world
    // bind transforms. To correctly position/orient them, we simply need to
    // transform each bone point by the corresponding skel-space joint
    // transform.
    VtMatrix4dArray xforms;
    if (skelQuery.ComputeJointSkelTransforms(&xforms, time)) {
        if (xforms.size() != _numJoints) {
            TF_WARN("Size of computed xforms [%zu] != expected num "
                    "joints [%zu].", xforms.size(), _numJoints);
            return _boneMeshPoints;
        }

        if(TF_VERIFY(_boneMeshPoints.size() == _boneMeshJointIndices.size())) {

            VtVec3fArray skinnedPoints(_boneMeshPoints);

            const int* jointIndices = _boneMeshJointIndices.cdata();
            const GfMatrix4d* jointXforms = xforms.cdata();
            GfVec3f* points = skinnedPoints.data();

            for(size_t pi = 0; pi < skinnedPoints.size(); ++pi) {
                int jointIdx = jointIndices[pi];

                TF_DEV_AXIOM(jointIdx >= 0 &&
                             static_cast<size_t>(jointIdx) < xforms.size());

                // XXX: Joint transforms in UsdSkel are required to be
                // affine, so this is safe!
                points[pi] = jointXforms[jointIdx].TransformAffine(points[pi]);
            }
            return skinnedPoints;
        }
    }
    return _boneMeshPoints;
}


PXR_NAMESPACE_CLOSE_SCOPE
