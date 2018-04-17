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
#include "pxr/usdImaging/usdSkelImaging/skelRootAdapter.h"

#include "pxr/usdImaging/usdSkelImaging/utils.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdSkelImagingSkelRootAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}


UsdSkelImagingSkelRootAdapter::~UsdSkelImagingSkelRootAdapter()
{}


bool
UsdSkelImagingSkelRootAdapter::IsSupported(
    const UsdImagingIndexProxy* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
}


SdfPath
UsdSkelImagingSkelRootAdapter::Populate(
    const UsdPrim& prim,
    UsdImagingIndexProxy* index,
    const UsdImagingInstancerContext* instancerContext)
{
    if(!TF_VERIFY(prim.IsA<UsdSkelRoot>())) {
        return SdfPath();
    }

    UsdSkelRoot skelRoot(prim);
    _skelCache.Populate(skelRoot);

    int instanceNum = 0;
    for(UsdPrim p : UsdPrimRange(prim)) {

        if(UsdSkelSkeletonQuery skelQuery = _skelCache.GetSkelQuery(p)) {

            SdfPath cachePath =
                prim.GetPath().AppendVariantSelection(
                    "instance", TfStringify(instanceNum++));

            SdfPath instancer = instancerContext ?
                instancerContext->instancerId : SdfPath();

            index->InsertRprim(HdPrimTypeTokens->mesh, cachePath,
                               instancer, prim, shared_from_this());
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

            auto skelInstance = std::make_shared<_SkelInstance>();
            skelInstance->skelRoot = skelRoot;
            skelInstance->skelQuery = skelQuery;

            _instanceCache[cachePath] = skelInstance;
        }
    }
    return prim.GetPath();
}

void
UsdSkelImagingSkelRootAdapter::TrackVariability(
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

    UsdImagingValueCache* valueCache = _GetValueCache();

    _SkelInstance* instance = _GetSkelInstance(cachePath);
    if(!TF_VERIFY(instance)) {
        return;
    }

    // Discover time-varying extent.
    _IsVarying(instance->skelRoot.GetPrim(),
               UsdGeomTokens->extent,
               HdChangeTracker::DirtyExtent,
               UsdImagingTokens->usdVaryingExtent,
               timeVaryingBits,
               false);

    // Discover time-varying transforms.
    if(!_IsTransformVarying(prim, HdChangeTracker::DirtyTransform,
                            UsdImagingTokens->usdVaryingXform,
                            timeVaryingBits)) {
        if(const UsdSkelAnimQuery& animQuery =
           instance->skelQuery.GetAnimQuery()) {
            
            if(animQuery.TransformMightBeTimeVarying()) {
                (*timeVaryingBits) |= HdChangeTracker::DirtyTransform;
                HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingExtent);
            }
        }
    }

    valueCache->GetVisible(cachePath) = GetVisible(prim, time);
    // Discover time-varying visibility.
    _IsVarying(prim,
               UsdGeomTokens->visibility,
               HdChangeTracker::DirtyVisibility,
               UsdImagingTokens->usdVaryingVisibility,
               timeVaryingBits,
               true);

    // Currently only drawing bones, which are implicitly guides.
    valueCache->GetPurpose(cachePath) = UsdGeomTokens->default_;
    
    // Discover time-varying points.
    if(const UsdSkelAnimQuery& animQuery = instance->skelQuery.GetAnimQuery()) {
        if(animQuery.JointTransformsMightBeTimeVarying()) {
            (*timeVaryingBits) |= HdChangeTracker::DirtyPoints;
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingPrimvar);
        }
    }
}


void
UsdSkelImagingSkelRootAdapter::UpdateForTime(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.

    UsdImagingValueCache* valueCache = _GetValueCache();

    _SkelInstance* instance = _GetSkelInstance(cachePath);
    if(!TF_VERIFY(instance)) {
        return;
    }

    // Extents comes from the skel root.
    if (requestedBits & HdChangeTracker::DirtyExtent) {
        // XXX: We can only reason about extents at the UsdSkelRoot.
        // This should give extents that encompass all skeletons contained
        // therein, but the extents may not be a tight fit.
        valueCache->GetExtent(cachePath) =
            _GetExtent(instance->skelRoot, time);
    }

    // Basic imaging properties.
    if (requestedBits & HdChangeTracker::DirtyDoubleSided) {
        valueCache->GetDoubleSided(cachePath) = true;
    }

    if (requestedBits & HdChangeTracker::DirtyVisibility) {
        valueCache->GetVisible(cachePath) = GetVisible(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyMaterialId) {
        // XXX: The bone mesh does not need a material.
        valueCache->GetMaterialId(cachePath) = SdfPath();
    }

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        valueCache->GetTopology(cachePath) =
            instance->ComputeTopologyAndRestState();
    }

    if (requestedBits & HdChangeTracker::DirtyPoints) {
        valueCache->GetPoints(cachePath) = instance->ComputePoints(time);
    }

    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        // Expose points as a primvar.
        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = HdTokens->points;
        primvar.interpolation = UsdGeomTokens->vertex;

        PrimvarInfoVector& primvars = valueCache->GetPrimvars(cachePath);
        _MergePrimvar(primvar, &primvars);
    }

    if (requestedBits & HdChangeTracker::DirtyTransform) {

        // World transform of a skeleton instance is:
        //    animTransform * primLocalToWorldTransform
        GfMatrix4d xform;
        if(instance->skelQuery.ComputeAnimTransform(&xform, time)) {
            xform *= GetTransform(prim, time);
        } else {
            xform = GetTransform(prim, time);
        }
        valueCache->GetTransform(cachePath) = xform;
    }
}


HdDirtyBits
UsdSkelImagingSkelRootAdapter::ProcessPropertyChange(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    const TfToken& propertyName)
{
    // TODO: Can we perform granular tracking?
    // The problem with this is that our Rprims are bound to the path of the
    // SkelRoot, but some of the changes we want to track happen on several
    // other prims, some of which may not even be descendants.
    return HdChangeTracker::AllDirty;;
}


void
UsdSkelImagingSkelRootAdapter::MarkDirty(const UsdPrim& prim,
                                         const SdfPath& cachePath,
                                         HdDirtyBits dirty,
                                         UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, dirty);
}


void
UsdSkelImagingSkelRootAdapter::_RemovePrim(const SdfPath& cachePath,
                                           UsdImagingIndexProxy* index)
{
    index->RemoveRprim(cachePath);

    _instanceCache.erase(cachePath);

    // TODO: Clearing the entire cache is excessive, but correct.
    _skelCache.Clear();
}


GfRange3d 
UsdSkelImagingSkelRootAdapter::_GetExtent(
    const UsdSkelRoot& skelRoot, UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtVec3fArray extent;
    if (skelRoot.GetExtentAttr().Get(&extent, time)) {
        // Note:
        // Usd stores extent as 2 float vecs. We do an implicit 
        // conversion to doubles
        return GfRange3d(extent[0], extent[1]);
    } else {
        // Return empty range if no value was found.
        return GfRange3d();
    }
}


UsdSkelImagingSkelRootAdapter::_SkelInstance*
UsdSkelImagingSkelRootAdapter::_GetSkelInstance(const SdfPath& cachePath) const
{
    auto it = _instanceCache.find(cachePath);
    return it != _instanceCache.end() ? it->second.get() : nullptr;
}


HdMeshTopology
UsdSkelImagingSkelRootAdapter::_SkelInstance::ComputeTopologyAndRestState()
{
    HdMeshTopology meshTopology;

    size_t numPoints = 0;
    UsdSkelImagingComputeBoneTopology(skelQuery.GetTopology(),
                                      &meshTopology,
                                      &numPoints);

    // While computing topology, we also compute the 'rest pose'
    // of the bone mesh, along with joint influences.
    VtMatrix4dArray xforms; 
    skelQuery.ComputeJointSkelTransforms(&xforms, UsdTimeCode::Default(),
                                         /*atRest*/ true);

    UsdSkelImagingComputeBonePoints(skelQuery.GetTopology(), xforms,
                                    numPoints, &_boneMeshPoints);

    UsdSkelImagingComputeBoneJointIndices(skelQuery.GetTopology(),
                                          &_boneMeshJointIndices, numPoints);
    return meshTopology;
}


VtVec3fArray
UsdSkelImagingSkelRootAdapter::_SkelInstance::ComputePoints(
    UsdTimeCode time) const
{
    VtVec3fArray skinnedPoints(_boneMeshPoints);

    VtMatrix4dArray xforms;
    if(skelQuery.ComputeSkinningTransforms(&xforms, time)) {

        if(TF_VERIFY(_boneMeshPoints.size() == _boneMeshJointIndices.size())) {

            // This is a simplified form of UsdSkelSkinPointsLBS,
            // We make the following simplifications:
            // - numInfluencesPerPoint = 1
            // - all weight values are 1

            const int* jointIndices = _boneMeshJointIndices.cdata();
            const GfMatrix4d* jointXforms = xforms.cdata();

            GfVec3f* points = skinnedPoints.data();
            for(size_t pi = 0; pi < skinnedPoints.size(); ++pi) {
                int jointIdx = jointIndices[pi];

                if(jointIdx >= 0 &&
                   static_cast<size_t>(jointIdx) < xforms.size()) {

                    // XXX: Joint transforms in UsdSkel are required to be
                    // affine, so this is safe!
                    points[pi] =
                        jointXforms[jointIdx].TransformAffine(points[pi]);
                } else {
                    TF_WARN("Out of range joint index %d at index %zu"
                            " (num joints = %zu).",
                            jointIdx, pi, xforms.size());
                    // Fallback to the rest points
                    return _boneMeshPoints;
                }
            }
            return skinnedPoints;
        }
    }
    return _boneMeshPoints;
}


PXR_NAMESPACE_CLOSE_SCOPE
