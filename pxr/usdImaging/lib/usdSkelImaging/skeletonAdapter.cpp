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
#include "pxr/usdImaging/usdSkelImaging/package.h"
#include "pxr/usdImaging/usdSkelImaging/utils.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/tokens.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/imaging/hd/extComputation.h" // dirtyBits
#include "pxr/imaging/hd/extComputationContext.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // computation inputs
    (geomBindXform)
    (hasConstantInfluences)
    (influences)
    (numInfluencesPerComponent)
    (primWorldToLocal)
    (restPoints)
    
    (skelLocalToWorld)
    (skinningXforms)

    // computation output
    (skinnedPoints)

    // computation(s)
    (skinningComputation)
    (skinningInputAggregatorComputation)

    // gpu compute kernels
    (skinPointsLBSKernel)
    (skinPointsSimpleKernel)

    // skel primvar names
    ((skelJointIndices,  "skel:jointIndices"))
    ((skelJointWeights,   "skel:jointWeights"))
    ((skelGeomBindXform, "skel:geomBindTransform"))

);

TF_DEFINE_ENV_SETTING(USDSKELIMAGING_FORCE_CPU_COMPUTE, 0,
                      "Use Hydra ExtCPU computations for skinning.");

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdSkelImagingSkeletonAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

// XXX: Temporary way to force CPU comps. Ideally, this is a render delegate
// opinion, or should be handled in Hydra ExtComputation.
static bool
_IsEnabledCPUComputations()
{
    static bool enabled
        = (TfGetEnvSetting(USDSKELIMAGING_FORCE_CPU_COMPUTE) == 1);
    return enabled;
}

static bool
_IsEnabledAggregatorComputation()
{
    // XXX: Aggregated comps don't work with CPU comps yet.
    static bool enabled = !_IsEnabledCPUComputations();
    return enabled;
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
    // We expect Populate to be called ONLY on a UsdSkelSkeleton prim.
    if(!TF_VERIFY(prim.IsA<UsdSkelSkeleton>())) {
        return SdfPath();
    }

    auto skelData = std::make_shared<_SkelData>();
    skelData->skelQuery = _skelCache.GetSkelQuery(UsdSkelSkeleton(prim));
    SdfPath const& skelPath = prim.GetPath();
    _skelDataCache[skelPath] = skelData;

    SdfPath instancer = instancerContext ?
        instancerContext->instancerId : SdfPath();

    // Insert mesh prim to visualize the bone mesh for the skeleton.
    // Note: This uses the "rest" pose of the skeleton.
    // Also, since the bone mesh isn't backed by the UsdStage, we register the
    // skeleton prim on its behalf.
    index->InsertRprim(HdPrimTypeTokens->mesh, prim.GetPath(),
                       instancer, prim, shared_from_this());
    
    
    // Insert a computation for each skinned prim targeted by this
    // skeleton. We know this because the SkelRootAdapter populated all the
    // "skeleton -> skinned prims" during Populate.
    // Note: The SkeletonAdapter registers itself as "responsible" for
    // the computation, and we pass the skinnedPrim as the usdPrim,
    // argument and _not_ the skel prim.
    const auto bindingIt = _skelBindingMap.find(skelPath);
    
    if (bindingIt != _skelBindingMap.end()) {
        UsdSkelBinding const& binding = bindingIt->second;

        for (UsdSkelSkinningQuery const& query : binding.GetSkinningTargets()) {
            UsdPrim const& skinnedPrim = query.GetPrim();
            SdfPath skinnedPrimPath = UsdImagingGprimAdapter::_ResolveCachePath(
                                    skinnedPrim.GetPath(), instancerContext);

            _skinnedPrimToSkelMap[skinnedPrimPath] = skelPath;
            
            SdfPath compPath = _GetSkinningComputationPath(skinnedPrimPath);

            TF_DEBUG(USDIMAGING_COMPUTATIONS).Msg(
                "[SkeletonAdapter::Populate] Inserting "
                "computation %s for skinned prim %s\n",
                compPath.GetText(), skinnedPrimPath.GetText());

            index->InsertSprim(
                    HdPrimTypeTokens->extComputation,
                    compPath,
                    skinnedPrim,
                    shared_from_this());

            if (_IsEnabledAggregatorComputation()) {
                SdfPath aggrCompPath =
                    _GetSkinningInputAggregatorComputationPath(skinnedPrimPath);

                TF_DEBUG(USDIMAGING_COMPUTATIONS).Msg(
                    "[SkeletonAdapter::Populate] Inserting "
                    "computation %s for skinned prim %s\n",
                    aggrCompPath.GetText(), skinnedPrimPath.GetText());

                index->InsertSprim(
                    HdPrimTypeTokens->extComputation,
                    aggrCompPath,
                    skinnedPrim,
                    shared_from_this());
            }
        }
    } else {
        // Do nothing. This isn't an error. We can have skeletons that
        // don't affect any skinned prims. One example is using variants.
    }

    return prim.GetPath();
}

// ---------------------------------------------------------------------- //
/// Parallel Setup and Resolve
// ---------------------------------------------------------------------- //

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

    if (_IsCallbackForSkeleton(prim)) {
        _TrackBoneMeshVariability(  prim,
                                    cachePath,
                                    timeVaryingBits,
                                    instancerContext);
        return;
    }

    if (_IsSkinnedPrimPath(cachePath)) {
        _TrackSkinnedPrimVariability(   prim,
                                        cachePath,
                                        timeVaryingBits,
                                        instancerContext);
        return;
    }

    if (_IsSkinningComputationPath(cachePath)) {
         _TrackSkinningComputationVariability(  prim,
                                                cachePath,
                                                timeVaryingBits,
                                                instancerContext);
        return;
    }

    if (_IsSkinningInputAggregatorComputationPath(cachePath)) {
        // Nothing to do; these are not expected to be time varying.
        // XXX: Check if inputs from the skinned prim are time-varying and
        // issue a warning.
        return;
    }

    TF_CODING_ERROR("UsdSkelImagingSkeletonAdapter::TrackVariability : Received"
                    " unknown prim %s ", cachePath.GetText());
}


void
UsdSkelImagingSkeletonAdapter::UpdateForTime(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    if (_IsCallbackForSkeleton(prim)) {
        return _UpdateBoneMeshForTime(  prim,
                                        cachePath,
                                        time,
                                        requestedBits,
                                        instancerContext);
    }

    if (_IsSkinnedPrimPath(cachePath)) {
        return _UpdateSkinnedPrimForTime(   prim,
                                            cachePath,
                                            time,
                                            requestedBits,
                                            instancerContext);
    }

    if (_IsSkinningComputationPath(cachePath)) {
        return _UpdateSkinningComputationForTime(   
                    prim,
                    cachePath,
                    time,
                    requestedBits,
                    instancerContext);
    }

    if (_IsSkinningInputAggregatorComputationPath(cachePath)) {
        return _UpdateSkinningInputAggregatorComputationForTime(
                    prim,
                    cachePath,
                    time,
                    requestedBits,
                    instancerContext);
    }

    TF_CODING_ERROR("UsdSkelImagingSkeletonAdapter::UpdateForTime : Received"
                    " unknown prim %s ", cachePath.GetText());
}

// ---------------------------------------------------------------------- //
/// Change Processing
// ---------------------------------------------------------------------- //

HdDirtyBits
UsdSkelImagingSkeletonAdapter::ProcessPropertyChange(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    const TfToken& propertyName)
{
    if (_IsCallbackForSkeleton(prim)) {
        if (propertyName == UsdGeomTokens->visibility ||
            propertyName == UsdGeomTokens->purpose)
            return HdChangeTracker::DirtyVisibility;
        else if (propertyName == UsdGeomTokens->extent)
            return HdChangeTracker::DirtyExtent;
        else if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(
                propertyName))
            return HdChangeTracker::DirtyTransform;

        // XXX: Changes to properties on the skeleton (e.g., the joint 
        // hierarchy) should propagate to the computations.
        // We don't have access to the UsdImagingIndexProxy here, so we cannot
        // use the property name to propagate dirtyness.

        // TODO: Perform granular tracking of only the relevant properties.
        // The main problem with this is that we can't easily track changes
        // related to the inherited SkelAnimation.
        return HdChangeTracker::AllDirty;
    }
    
    if (_IsSkinnedPrimPath(cachePath)) {
        // Since The SkeletonAdapter hijacks skinned prims (see SkelRootAdapter),
        // make sure to delegate to the actual adapter registered for the prim.
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
        HdDirtyBits dirtyBits =
            adapter->ProcessPropertyChange(prim, cachePath, propertyName);
        
        // XXX: We need to handle UsdSkel-related primvars manually here, since
        // they're ignored in GprimAdapter.
        if (propertyName == UsdSkelTokens->primvarsSkelJointIndices || 
            propertyName == UsdSkelTokens->primvarsSkelJointWeights ||
            propertyName == UsdSkelTokens->primvarsSkelGeomBindTransform) {
            
            if (dirtyBits == HdChangeTracker::AllDirty) {
                // XXX: We don't have access to the UsdImagingIndexProxy here,
                // so We propagate the dirtyness to the computations in
                // MarkDirty(..)
                dirtyBits = HdChangeTracker::DirtyPrimvar;
            } else {
                TF_WARN("Skinned prim %s needs to be resync'd because of a"
                        "property change. Hijacking doesn't work in this "
                        "scenario.\n", cachePath.GetText());
            }
        }

        return dirtyBits;
    }
    
    // We don't expect to get callbacks on behalf of any other prims on
    // the USD stage.
    TF_WARN("Unhandled ProcessPropertyChange callback for cachePath <%s> "
                "in UsdSkelImagingSkelAdapter.", cachePath.GetText());
    return HdChangeTracker::Clean;
}


void
UsdSkelImagingSkeletonAdapter::MarkDirty(const UsdPrim& prim,
                                         const SdfPath& cachePath,
                                         HdDirtyBits dirty,
                                         UsdImagingIndexProxy* index)
{
    if (_IsCallbackForSkeleton(prim)) {

        // Mark the bone mesh dirty
        index->MarkRprimDirty(cachePath, dirty);

    } else if (_IsSkinnedPrimPath(cachePath)) {

        // Since The SkeletonAdapter hijacks skinned prims (see SkelRootAdapter),
        // make sure to delegate to the actual adapter registered for the prim.
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
        adapter->MarkDirty(prim, cachePath, dirty, index);

        // Propagate dirtyness to the computations.
        // See related comment in ProcessPropertyChange(..)
        if (dirty & HdChangeTracker::DirtyPoints ||
            dirty & HdChangeTracker::DirtyTransform ||
            dirty & HdChangeTracker::DirtyPrimvar) {
            
            index->MarkSprimDirty(_GetSkinningComputationPath(cachePath),
                                  HdExtComputation::DirtySceneInput);
            
            if (_IsEnabledAggregatorComputation()) {
                index->MarkSprimDirty(
                    _GetSkinningInputAggregatorComputationPath(cachePath),
                    HdExtComputation::DirtySceneInput);
            }
        }
    
    } else if (_IsSkinningComputationPath(cachePath) ||
              _IsSkinningInputAggregatorComputationPath(cachePath)) {

         TF_DEBUG(USDIMAGING_COMPUTATIONS).Msg(
                "[SkeletonAdapter::Populate] Marking "
                "computation %s for skinned prim %s as Dirty.\n",
                cachePath.GetText(), prim.GetPath().GetText());

        index->MarkSprimDirty(cachePath, dirty);
    
    } else {
        // We don't expect to get callbacks on behalf of any other prims on
        // the USD stage.
         TF_WARN("Unhandled MarkDirty callback for cachePath <%s> "
                 "in UsdSkelImagingSkelAdapter.", cachePath.GetText());
    }
}

void
UsdSkelImagingSkeletonAdapter::MarkRefineLevelDirty(const UsdPrim& prim,
                                                    const SdfPath& cachePath,
                                                    UsdImagingIndexProxy* index)
{
    if (_IsCallbackForSkeleton(prim) ||
        _IsSkinnedPrimPath(cachePath)) {

        // Since The SkeletonAdapter hijacks the bone mesh and any skinned prim,
        // make sure to delegate to the actual adapter registered for the prim.
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
        adapter->MarkRefineLevelDirty(prim, cachePath, index);

    }
    // Nothing to do otherwise.
}

void
UsdSkelImagingSkeletonAdapter::MarkReprDirty(const UsdPrim& prim,
                                             const SdfPath& cachePath,
                                             UsdImagingIndexProxy* index)
{
    if (_IsCallbackForSkeleton(prim) ||
        _IsSkinnedPrimPath(cachePath)) {

        // Since The SkeletonAdapter hijacks the bone mesh and any skinned prim,
        // make sure to delegate to the actual adapter registered for the prim.
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
        adapter->MarkReprDirty(prim, cachePath, index);

    }
    // Nothing to do otherwise.
}

void
UsdSkelImagingSkeletonAdapter::MarkCullStyleDirty(const UsdPrim& prim,
                                                  const SdfPath& cachePath,
                                                  UsdImagingIndexProxy* index)
{
    if (_IsCallbackForSkeleton(prim) ||
        _IsSkinnedPrimPath(cachePath)) {

        // Since The SkeletonAdapter hijacks the bone mesh and any skinned prim,
        // make sure to delegate to the actual adapter registered for the prim.
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
        adapter->MarkCullStyleDirty(prim, cachePath, index);

    }
    // Nothing to do otherwise.
}

void
UsdSkelImagingSkeletonAdapter::MarkTransformDirty(const UsdPrim& prim,
                                                  const SdfPath& cachePath,
                                                  UsdImagingIndexProxy* index)
{
    if (_IsCallbackForSkeleton(prim) ||
        _IsSkinnedPrimPath(cachePath)) {

        // Since The SkeletonAdapter hijacks the bone mesh and any skinned prim,
        // make sure to delegate to the actual adapter registered for the prim.
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
        adapter->MarkTransformDirty(prim, cachePath, index);

    } else if (_IsSkinningComputationPath(cachePath) ||
              _IsSkinningInputAggregatorComputationPath(cachePath)) {

        // XXX: See comments in ProcessPropertyChange about dirtyness
        // propagation to the computations.
    
    } else {
        // We don't expect to get callbacks on behalf of any other prims on
        // the USD stage.
         TF_WARN("Unhandled MarkDirty callback for cachePath <%s> "
                 "in UsdSkelImagingSkelAdapter.", cachePath.GetText());
    }
}


void
UsdSkelImagingSkeletonAdapter::MarkVisibilityDirty(const UsdPrim& prim,
                                                   const SdfPath& cachePath,
                                                   UsdImagingIndexProxy* index)
{
    if (_IsCallbackForSkeleton(prim) ||
        _IsSkinnedPrimPath(cachePath)) {

        // Since The SkeletonAdapter hijacks the bone mesh and any skinned prim,
        // make sure to delegate to the actual adapter registered for the prim.
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
        adapter->MarkVisibilityDirty(prim, cachePath, index);

        // Note:
        // (1) If a skeleton is invis'd, it continues to affect skinned prims.
        
        // (2) The computations are executed as a result of the Rprim sync step.
        // We skip syncing Rprims that are invis'd (note: if a prim is invisible
        // at the start, we do sync once), and thus won't trigger the
        // computations.

    } else if (_IsSkinningComputationPath(cachePath) ||
              _IsSkinningInputAggregatorComputationPath(cachePath)) {

        // Nothing to do. See comment above.
    
    } else {
        // We don't expect to get callbacks on behalf of any other prims on
        // the USD stage.
         TF_WARN("Unhandled MarkDirty callback for cachePath <%s> "
                 "in UsdSkelImagingSkelAdapter.", cachePath.GetText());
    }
}


void
UsdSkelImagingSkeletonAdapter::MarkMaterialDirty(const UsdPrim& prim,
                                                 const SdfPath& cachePath,
                                                 UsdImagingIndexProxy* index)
{
    if (_IsCallbackForSkeleton(prim) ||
        _IsSkinnedPrimPath(cachePath)) {

        // Since The SkeletonAdapter hijacks the bone mesh and any skinned prim,
        // make sure to delegate to the actual adapter registered for the prim.
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
        adapter->MarkMaterialDirty(prim, cachePath, index);

    } else {
        // We don't expect to get callbacks on behalf of computations or
        // any other prims on the USD stage 
         TF_WARN("Unhandled MarkDirty callback for cachePath <%s> "
                 "in UsdSkelImagingSkelAdapter.", cachePath.GetText());
    }
}


namespace {

void
_TransformPoints(TfSpan<GfVec3f> points, const GfMatrix4d& xform)
{
    WorkParallelForN(
        points.size(),
        [&](size_t start, size_t end)
        {
            for (size_t i = start; i < end; ++i) {
                points[i] = xform.Transform(points[i]);
            }
        }, /*grainSize*/ 1000);
}

} // namespace

// ---------------------------------------------------------------------- //
/// Computation API
// ---------------------------------------------------------------------- //
void
UsdSkelImagingSkeletonAdapter::InvokeComputation(
    SdfPath const& computationPath,
    HdExtComputationContext* context)
{
    HD_TRACE_FUNCTION();

    VtValue restPoints
        = context->GetInputValue(_tokens->restPoints);
    VtValue geomBindXform
        = context->GetInputValue(_tokens->geomBindXform);
    VtValue influences
        = context->GetInputValue(_tokens->influences);
    VtValue numInfluencesPerComponent
        = context->GetInputValue(_tokens->numInfluencesPerComponent);
    VtValue hasConstantInfluences
        = context->GetInputValue(_tokens->hasConstantInfluences);
    VtValue primWorldToLocal
        = context->GetInputValue(_tokens->primWorldToLocal);

    VtValue skinningXforms
        = context->GetInputValue(_tokens->skinningXforms);
    VtValue skelLocalToWorld
        = context->GetInputValue(_tokens->skelLocalToWorld);

    // Ensure inputs are holding the right value types.
    if (!restPoints.IsHolding<VtVec3fArray>() ||
        !geomBindXform.IsHolding<GfMatrix4f>() ||
        !influences.IsHolding<VtVec2fArray>() ||
        !numInfluencesPerComponent.IsHolding<int>() ||
        !hasConstantInfluences.IsHolding<bool>() ||
        !primWorldToLocal.IsHolding<GfMatrix4d>() ||

        !skinningXforms.IsHolding<VtMatrix4fArray>() ||
        !skelLocalToWorld.IsHolding<GfMatrix4d>()) {
            
        TF_DEBUG(USDIMAGING_COMPUTATIONS).Msg(
                "[SkeletonAdapter::InvokeComputation] Error invoking CPU "
                "computation %s\n", computationPath.GetText());
        context->RaiseComputationError();
        return;
    }

    VtVec3fArray skinnedPoints = 
        restPoints.UncheckedGet<VtVec3fArray>();

    if (!hasConstantInfluences.UncheckedGet<bool>()) {
        
        if (UsdSkelSkinPointsLBS(
            geomBindXform.UncheckedGet<GfMatrix4f>(),
            skinningXforms.UncheckedGet<VtMatrix4fArray>(),
            influences.UncheckedGet<VtVec2fArray>(),
            numInfluencesPerComponent.UncheckedGet<int>(),
            skinnedPoints)) {

            // The points returned above are in skel space, and need to be
            // xformed to prim local space.
            const GfMatrix4d skelToPrimLocal =
                skelLocalToWorld.UncheckedGet<GfMatrix4d>() *
                primWorldToLocal.UncheckedGet<GfMatrix4d>();

            _TransformPoints(skinnedPoints, skelToPrimLocal);

        } else {
            // Return the restPoints as a fallback.
            // Note that we set skinnedPoints to restPoints here even
            // though skinnedPoints was initialized to restPoints.
            // This ensures that we don't end up with scrambled meshes if an
            // error was encountered midway during skinning application.
            skinnedPoints = restPoints.UncheckedGet<VtVec3fArray>();
        }

    } else {
        // Have constant influences. Compute a rigid deformation.
        GfMatrix4f skinnedTransform;
        if (UsdSkelSkinTransformLBS(
                geomBindXform.UncheckedGet<GfMatrix4f>(),
                skinningXforms.UncheckedGet<VtMatrix4fArray>(),
                influences.UncheckedGet<VtVec2fArray>(),
                &skinnedTransform)) {
            
            // The computed skinnedTransform is the transform which, when
            // applied to the points of the skinned prim, results in skinned
            // points in *skel* space, and need to be xformed to prim
            // local space.

            const GfMatrix4d restToPrimLocalSkinnedXf =
                GfMatrix4d(skinnedTransform)*
                skelLocalToWorld.UncheckedGet<GfMatrix4d>()*
                primWorldToLocal.UncheckedGet<GfMatrix4d>();

            // XXX: Ideally we would modify the xform of the skinned prim,
            // rather than its underlying points (which is particularly
            // important if we want to preserve instancing!).
            // For now, bake the rigid deformation into the points.
            _TransformPoints(skinnedPoints, restToPrimLocalSkinnedXf);

        } else {
            // Nothing to do. We initialized skinnedPoints to the restPoints,
            // so just return that.
        }
    }

    context->SetOutputValue(_tokens->skinnedPoints, VtValue(skinnedPoints));
}

// ---------------------------------------------------------------------- //
/// Non-virtual public API
// ---------------------------------------------------------------------- //

void
UsdSkelImagingSkeletonAdapter::RegisterSkelBinding(
    UsdSkelBinding const& binding)
{
    _skelBindingMap[binding.GetSkeleton().GetPath()] = binding;
}

// ---------------------------------------------------------------------- //
/// Utility methods
// ---------------------------------------------------------------------- //

void
UsdSkelImagingSkeletonAdapter::_RemovePrim(const SdfPath& cachePath,
                                           UsdImagingIndexProxy* index)
{
    // XXX: We could just delegate to the registered adapter if _RemovePrim
    // wasn't protected
    if (index->HasRprim(cachePath)) {
        index->RemoveRprim(cachePath);
    }

    bool isSkelPath = _skelBindingMap.find(cachePath) != _skelBindingMap.end();
    if (isSkelPath) {
        _skelBindingMap.erase(cachePath);
    } else if (_IsSkinnedPrimPath(cachePath)) {
        // Remove the computations as well.
        index->RemoveSprim(HdPrimTypeTokens->extComputation,
                           _GetSkinningComputationPath(cachePath));
        
        if (_IsEnabledAggregatorComputation()) {
            index->RemoveSprim(HdPrimTypeTokens->extComputation,
                        _GetSkinningInputAggregatorComputationPath(cachePath));

        }

        _skinnedPrimToSkelMap.erase(cachePath);
    }

    // Remove internal bookkeeping entries.
    _skelDataCache.erase(cachePath);

    // TODO: Clearing the entire cache is excessive, but correct.
    _skelCache.Clear();
}

// ---------------------------------------------------------------------- //
/// Handlers for the Bone Mesh
// ---------------------------------------------------------------------- //
bool
UsdSkelImagingSkeletonAdapter::_IsCallbackForSkeleton(const UsdPrim& prim) const
{
    // The Skeleton prim is registered against the bone mesh. See Populate(..)
    return prim.IsA<UsdSkelSkeleton>();
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


GfVec3f
UsdSkelImagingSkeletonAdapter::_GetSkeletonDisplayColor(
        const UsdPrim& prim,
        UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    UsdGeomPrimvarsAPI primvars(prim);

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
UsdSkelImagingSkeletonAdapter::_GetSkeletonDisplayOpacity(
        const UsdPrim& prim,
        UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    UsdGeomPrimvarsAPI primvars(prim);

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


void
UsdSkelImagingSkeletonAdapter::_TrackBoneMeshVariability(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    HdDirtyBits* timeVaryingBits,
    const UsdImagingInstancerContext* instancerContext) const
{
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

    // Discover time-varying visibility.
    _IsVarying(prim,
               UsdGeomTokens->visibility,
               HdChangeTracker::DirtyVisibility,
               UsdImagingTokens->usdVaryingVisibility,
               timeVaryingBits,
               true);

    // Tie the bone mesh to the 'proxy' purpose, so we have a way to disable
    // rendering of ALL bone meshes.
    valueCache->GetPurpose(cachePath) = UsdGeomTokens->proxy;
}


void
UsdSkelImagingSkeletonAdapter::_UpdateBoneMeshForTime(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    const UsdImagingInstancerContext* instancerContext) const
{
     _SkelData* skelData = _GetSkelData(cachePath);
    if (!TF_VERIFY(skelData)) {
        return;
    }

    TF_DEBUG(USDIMAGING_CHANGES).Msg("[UpdateForTime] Skeleton path: <%s>\n",
                                     prim.GetPath().GetText());
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[UpdateForTime] Cache path: <%s>\n",
                                     cachePath.GetText());

    // Act as the mesh adapter for the non-existent bone mesh, and populate
    // the value cache with the necessary info.
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
            _GetSkeletonDisplayColor(prim, time);
        valueCache->GetOpacity(cachePath) =
            _GetSkeletonDisplayOpacity(prim, time);

        _MergePrimvar(&valueCache->GetPrimvars(cachePath),
                      HdTokens->displayColor,
                      HdInterpolationConstant,
                      HdPrimvarRoleTokens->color);
        _MergePrimvar(&valueCache->GetPrimvars(cachePath),
                      HdTokens->displayOpacity,
                      HdInterpolationConstant);
    }

    if (requestedBits & HdChangeTracker::DirtyDoubleSided) {
        valueCache->GetDoubleSided(cachePath) = true;
    }

    if (requestedBits & HdChangeTracker::DirtyMaterialId) {
        // The bone mesh does not need a material.
        valueCache->GetMaterialId(cachePath) = SdfPath();
    }
}


// ---------------------------------------------------------------------- //
/// Common utitily methods for skinning computations & skinned prims
// ---------------------------------------------------------------------- //
bool
UsdSkelImagingSkeletonAdapter::_IsAffectedByTimeVaryingJointXforms(
    const SdfPath& skinnedPrimPath) const
{
    SdfPath const& skelPath = _skinnedPrimToSkelMap.at(skinnedPrimPath);

    _SkelData* skelData = _GetSkelData(skelPath);
    if (!TF_VERIFY(skelData)) {
        return false;
    }
    // Discover time-varying points.
    if (const UsdSkelAnimQuery& animQuery =
        skelData->skelQuery.GetAnimQuery()) {

        if(animQuery.JointTransformsMightBeTimeVarying()) {
            return true;
        }
    }
    return false;
}


// ---------------------------------------------------------------------- //
/// Handlers for the skinning computations
// ---------------------------------------------------------------------- //
SdfPath
UsdSkelImagingSkeletonAdapter::_GetSkinningComputationPath(
    const SdfPath& skinnedPrimPath) const
{
    return skinnedPrimPath.AppendChild(_tokens->skinningComputation);
}


SdfPath
UsdSkelImagingSkeletonAdapter::_GetSkinningInputAggregatorComputationPath(
    const SdfPath& skinnedPrimPath) const
{
    return skinnedPrimPath.AppendChild(_tokens->skinningInputAggregatorComputation);
}


bool
UsdSkelImagingSkeletonAdapter::_IsSkinningComputationPath(
    const SdfPath& cachePath) const
{
    return cachePath.GetName() == _tokens->skinningComputation;
}


bool
UsdSkelImagingSkeletonAdapter::_IsSkinningInputAggregatorComputationPath(
    const SdfPath& cachePath) const
{
    return cachePath.GetName() == _tokens->skinningInputAggregatorComputation;
}


void
UsdSkelImagingSkeletonAdapter::_TrackSkinningComputationVariability(
    const UsdPrim& skinnedPrim,
    const SdfPath& computationPath,
    HdDirtyBits* timeVaryingBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    // XXX: We don't receive the "cachePath" for the skinned prim, and so
    // the method below won't work when using multiple UsdImagingDelgate's.
    SdfPath skinnedPrimCachePath = UsdImagingGprimAdapter::_ResolveCachePath(
            skinnedPrim.GetPath(), instancerContext);
    
    if (_IsAffectedByTimeVaryingJointXforms(skinnedPrimCachePath)) {
        (*timeVaryingBits) |= HdExtComputation::DirtySceneInput;
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingPrimvar);
    }

    // XXX: Issue warnings for computation inputs that we don't expect to be 
    // time varying.
}


VtVec3fArray
UsdSkelImagingSkeletonAdapter::_GetSkinnedPrimPoints(
    const UsdPrim& skinnedPrim,
    const SdfPath& skinnedPrimCachePath,
    UsdTimeCode time) const
{
    // Since only UsdGeomBased-type prims can be targeted by a skeleton,
    // we expect the skinned prim adapter to derive from GprimAdapter.
    UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(skinnedPrim);
    auto gprimAdapter =
        boost::dynamic_pointer_cast<UsdImagingGprimAdapter> (adapter);
    if (!TF_VERIFY(gprimAdapter)) {
        return VtVec3fArray();
    }

    VtValue points = 
        gprimAdapter->GetPoints(skinnedPrim, skinnedPrimCachePath, time);
    if (!TF_VERIFY(points.IsHolding<VtVec3fArray>())) {
        return VtVec3fArray();
    }
    return points.UncheckedGet<VtVec3fArray>();
}


namespace {


bool
_GetInfluences(const UsdSkelBindingAPI& binding,
               UsdTimeCode time,
               VtVec2fArray* influences,
               int* numInfluencesPerComponent,
               bool* isConstant)
{
    const UsdGeomPrimvar ji = binding.GetJointIndicesPrimvar();
    const UsdGeomPrimvar jw = binding.GetJointWeightsPrimvar();
    
    const int indicesElementSize = ji.GetElementSize();
    const int weightsElementSize = jw.GetElementSize();
    if (indicesElementSize != weightsElementSize) {
        TF_WARN("%s -- jointIndices element size (%d) != "
                "jointWeights element size (%d).",
                binding.GetPrim().GetPath().GetText(),
                indicesElementSize, weightsElementSize);
        return false;
    }
    
    if (indicesElementSize <= 0) {
        TF_WARN("%s -- Invalid element size for skel:jointIndices and "
                "skel:jointWeights primvars (%d): element size must greater "
                "than zero.", binding.GetPrim().GetPath().GetText(),
                indicesElementSize);
        return false;
    }
    const TfToken indicesInterpolation = ji.GetInterpolation();
    const TfToken weightsInterpolation = jw.GetInterpolation();
    if (indicesInterpolation != weightsInterpolation) {
        TF_WARN("%s -- jointIndices interpolation (%s) != "
                "jointWeights interpolation (%s).",
                binding.GetPrim().GetPath().GetText(),
                indicesInterpolation.GetText(),
                weightsInterpolation.GetText());
        return false;
    }
    
    VtIntArray vji;
    VtFloatArray vjw;
    if (ji.ComputeFlattened(&vji, time) &&
        jw.ComputeFlattened(&vjw, time)) {

        influences->resize(vji.size());
        if (UsdSkelInterleaveInfluences(vji, vjw, *influences)) {
            *numInfluencesPerComponent = indicesElementSize;
            *isConstant = indicesInterpolation == UsdGeomTokens->constant;
            return true;
        }
    }
    return false;
}
               


} // namespace


void
UsdSkelImagingSkeletonAdapter::_UpdateSkinningComputationForTime(
    const UsdPrim& skinnedPrim,
    const SdfPath& computationPath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    TF_DEBUG(USDIMAGING_CHANGES).Msg(
        "[_UpdateSkinningComputationForTime] "
         "skinnedPrim path: <%s> , computation path: <%s>\n",
        skinnedPrim.GetPath().GetText(),
        computationPath.GetText());
    
    UsdImagingValueCache* valueCache = _GetValueCache();

    // XXX: We don't receive the "cachePath" for the skinned prim, and so
    // the method below won't work when using multiple UsdImagingDelgate's.
    SdfPath skinnedPrimCachePath = UsdImagingGprimAdapter::_ResolveCachePath(
            skinnedPrim.GetPath(), instancerContext);
    
    // For dispatchCount, elementCount and outputDesc, we need to know the 
    // number of points on the skinned prim. Pull only when required.
    VtVec3fArray restPoints;
    size_t numPoints = 0;
    if (requestedBits & 
            (HdExtComputation::DirtyDispatchCount |
             HdExtComputation::DirtyElementCount |
             HdExtComputation::DirtySceneInput)) {
        restPoints =
            _GetSkinnedPrimPoints(skinnedPrim, skinnedPrimCachePath, time);
        numPoints = restPoints.size();
    }
    
    if (requestedBits & HdExtComputation::DirtyDispatchCount) {
        valueCache->GetExtComputationInput(
            computationPath,HdTokens->dispatchCount) = VtValue(numPoints);
    }

    if (requestedBits & HdExtComputation::DirtyElementCount) {
        valueCache->GetExtComputationInput(
            computationPath, HdTokens->elementCount) = VtValue(numPoints);
    }

    if (requestedBits & HdExtComputation::DirtyInputDesc) {
        if (_IsEnabledAggregatorComputation()) {

            // Scene inputs
            TfTokenVector sceneInputNames({
                // From the skinned prim
                    _tokens->primWorldToLocal,
                // From the skeleton
                    _tokens->skinningXforms, // only this is time-varying
                    _tokens->skelLocalToWorld
            });
            valueCache->GetExtComputationSceneInputNames(computationPath)
                = sceneInputNames;

            // Computation inputs
            TfTokenVector compInputNames({
                    _tokens->restPoints,
                    _tokens->geomBindXform,
                    _tokens->influences,
                    _tokens->numInfluencesPerComponent,
                    _tokens->hasConstantInfluences,
            });
            SdfPath skinnedPrimPath =
                UsdImagingGprimAdapter::_ResolveCachePath(
                            skinnedPrim.GetPath(), instancerContext);
            SdfPath aggrCompId =
                _GetSkinningInputAggregatorComputationPath(skinnedPrimPath);
            
            HdExtComputationInputDescriptorVector compInputDescs;
            for (auto const& input : compInputNames) {
                compInputDescs.emplace_back(
                    HdExtComputationInputDescriptor(input, aggrCompId, input));
            }
            valueCache->GetExtComputationInputs(computationPath)
                = compInputDescs;

        } else {
            // Scene inputs
            TfTokenVector sceneInputNames({
                // From the skinned prim
                    _tokens->restPoints,
                    _tokens->geomBindXform,
                    _tokens->influences,
                    _tokens->numInfluencesPerComponent,
                    _tokens->hasConstantInfluences,
                    _tokens->primWorldToLocal,

                // From the skeleton
                    _tokens->skinningXforms, // only this is time-varying
                    _tokens->skelLocalToWorld
            });
            valueCache->GetExtComputationSceneInputNames(computationPath) =
                sceneInputNames;

            // No computation inputs
            valueCache->GetExtComputationInputs(computationPath)
                = HdExtComputationInputDescriptorVector();
        }
    }

    if (requestedBits & HdExtComputation::DirtySceneInput) {
        // XXX: The only time varying input here is the skinning xforms.
        // However, we don't have fine-grained tracking to tell which
        // scene input is "dirty". Hence, fetch all these values and update
        // the value cache.
        // Note: With CPU computations, this is necessary. We don't use
        //       persistent buffer sources to cache the inputs.
        //       With GPU computations, we can use an "input aggregation"
        //       computations to remove the non-varying inputs into its own
        //       computation.
        
        UsdSkelBindingAPI binding(skinnedPrim);

        // TODO: Handle inherited primvars for jointIndices, jointWeights
        // and geomBindTransform.

        // restPoints, geomBindXform
        if (!_IsEnabledAggregatorComputation()) {
            valueCache->GetExtComputationInput(
                computationPath, _tokens->restPoints) = restPoints;

            // read (optional) geomBindTransform property.
            // If unauthored, it is identity.
            GfMatrix4d geomBindXform(1);
            if (UsdAttribute attr = binding.GetGeomBindTransformAttr()) {
                attr.Get(&geomBindXform);
            }
            // Skinning computations use float precision.
            valueCache->GetExtComputationInput(
                computationPath, _tokens->geomBindXform) =
                    GfMatrix4f(geomBindXform);
        }

        // influences, numInfluencesPerComponent, hasConstantInfluences
        if (!_IsEnabledAggregatorComputation()) {

            VtVec2fArray influences;
            int numInfluencesPerComponent = 0;
            bool usesConstantJointPrimvar = false;
            if (_GetInfluences(binding, time, &influences,
                               &numInfluencesPerComponent,
                               &usesConstantJointPrimvar)) {

                valueCache->GetExtComputationInput(
                    computationPath, _tokens->influences) = influences;
                valueCache->GetExtComputationInput(
                    computationPath, _tokens->numInfluencesPerComponent)
                        = numInfluencesPerComponent;
                valueCache->GetExtComputationInput(
                    computationPath, _tokens->hasConstantInfluences)
                        = usesConstantJointPrimvar;
            }
        }

        // primWorldToLocal
        {
            UsdGeomXformCache xformCache(time);
            GfMatrix4d primWorldToLocal =
                xformCache.GetLocalToWorldTransform(skinnedPrim).GetInverse();
            valueCache->GetExtComputationInput(
                computationPath,_tokens->primWorldToLocal)
                    = VtValue(primWorldToLocal);
        }

        // skinningXforms, skelLocalToWorld
        {
            SdfPath skinnedPrimPath =
                UsdImagingGprimAdapter::_ResolveCachePath(
                            skinnedPrim.GetPath(), instancerContext);
            SdfPath const& skelPath = _skinnedPrimToSkelMap.at(skinnedPrimPath);

            _SkelData* skelData = _GetSkelData(skelPath);
            if (!TF_VERIFY(skelData)) {
                return;
            }
            VtMatrix4fArray skinningXforms;
            if (skelData->skelQuery.ComputeSkinningTransforms(&skinningXforms,
                                                           time)) {
                valueCache->GetExtComputationInput(
                    computationPath, _tokens->skinningXforms) = skinningXforms;
            }

            UsdGeomXformCache xformCache(time);
            UsdPrim const& skelPrim = skelData->skelQuery.GetPrim();
            GfMatrix4d skelLocalToWorld =
                xformCache.GetLocalToWorldTransform(skelPrim);
            valueCache->GetExtComputationInput(
                computationPath, _tokens->skelLocalToWorld) = skelLocalToWorld;
        }
    }
    
    if (requestedBits & HdExtComputation::DirtyOutputDesc) {
        HdTupleType pointsType;
        pointsType.type = HdTypeFloatVec3;
        pointsType.count = 1;
        
        HdExtComputationOutputDescriptorVector& outputsEntry =
            valueCache->GetExtComputationOutputs(computationPath);
        outputsEntry.clear();
        outputsEntry.emplace_back(_tokens->skinnedPoints, pointsType);
    }

    if (requestedBits & HdExtComputation::DirtyKernel) {
        if (_IsEnabledCPUComputations()) {
            valueCache->GetExtComputationKernel(computationPath) = 
                std::string();
        } else {
            valueCache->GetExtComputationKernel(computationPath)
                = _GetSkinningComputeKernel();
        }
    }
}


void
UsdSkelImagingSkeletonAdapter::_UpdateSkinningInputAggregatorComputationForTime(
    const UsdPrim& skinnedPrim,
    const SdfPath& computationPath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    TF_DEBUG(USDIMAGING_CHANGES).Msg(
        "[_UpdateSkinningInputAggregatorComputationForTime] "
        "skinnedPrim path: <%s> , computation path: <%s>\n",
        skinnedPrim.GetPath().GetText(),
        computationPath.GetText());

    // Note: We expect this to run only when the skeleton prim is added/resync'd
    UsdImagingValueCache* valueCache = _GetValueCache();

    // XXX: We don't receive the "cachePath" for the skinned prim, and so
    // the method below won't work when using multiple UsdImagingDelgate's.
    SdfPath skinnedPrimCachePath = UsdImagingGprimAdapter::_ResolveCachePath(
            skinnedPrim.GetPath(), instancerContext);
 
    // DispatchCount, ElementCount aren't relevant for an input aggregator
    // computation. However, it will be pulled on during sprim sync, so
    // update the value cache.
    if (requestedBits & HdExtComputation::DirtyDispatchCount) {
        valueCache->GetExtComputationInput(
            computationPath, HdTokens->dispatchCount) = size_t(0);
    }

    if (requestedBits & HdExtComputation::DirtyElementCount) {
        valueCache->GetExtComputationInput(
            computationPath, HdTokens->elementCount) = size_t(0);
    }

    if (requestedBits & HdExtComputation::DirtyInputDesc) {
        // ExtComputation inputs
        TfTokenVector inputNames({
            // Data authored on the skinned prim as primvars.
                _tokens->restPoints,
                _tokens->geomBindXform,
                _tokens->influences,
                _tokens->numInfluencesPerComponent,
                _tokens->hasConstantInfluences,
        });
        valueCache->GetExtComputationSceneInputNames(computationPath)
            = inputNames;

        valueCache->GetExtComputationInputs(computationPath)
            = HdExtComputationInputDescriptorVector();
    }

    if (requestedBits & HdExtComputation::DirtySceneInput) {
        UsdSkelBindingAPI binding(skinnedPrim);

        // TODO: Handle inherited primvars for jointIndices, jointWeights
        // and geomBindTransform.

        // restPoints, geomBindXform
        {
            VtVec3fArray restPoints =
                _GetSkinnedPrimPoints(skinnedPrim, skinnedPrimCachePath, time);
            valueCache->GetExtComputationInput(
                computationPath, _tokens->restPoints) = restPoints;
            
            // read (optional) geomBindTransform property.
            // If unauthored, it is identity.
            GfMatrix4d geomBindXform(1);
            if (UsdAttribute attr = binding.GetGeomBindTransformAttr()) {
                attr.Get(&geomBindXform);
            }
            // Skinning computations use float precision.
            valueCache->GetExtComputationInput(
                computationPath, _tokens->geomBindXform) =
                GfMatrix4f(geomBindXform);
        }

        // influences, numInfluencesPerComponent, hasConstantInfluences
        {
            VtVec2fArray influences;
            int numInfluencesPerComponent = 0;
            bool usesConstantJointPrimvar = false;
            if (_GetInfluences(binding, time, &influences,
                               &numInfluencesPerComponent,
                               &usesConstantJointPrimvar)) {

                valueCache->GetExtComputationInput(
                    computationPath, _tokens->influences) = influences;
                valueCache->GetExtComputationInput(
                    computationPath, _tokens->numInfluencesPerComponent)
                        = numInfluencesPerComponent;
                valueCache->GetExtComputationInput(
                    computationPath, _tokens->hasConstantInfluences)
                        = usesConstantJointPrimvar;
            }
        }
    }
    
    if (requestedBits & HdExtComputation::DirtyOutputDesc) {
        valueCache->GetExtComputationOutputs(computationPath)
            = HdExtComputationOutputDescriptorVector();
    }

    if (requestedBits & HdExtComputation::DirtyKernel) {
        valueCache->GetExtComputationKernel(computationPath)
            = std::string();
    }
}


/* static */
std::string
UsdSkelImagingSkeletonAdapter::_LoadSkinningComputeKernel()
{
    TRACE_FUNCTION();
    HioGlslfx gfx(UsdSkelImagingPackageSkinningShader());

    if (!gfx.IsValid()) {
        TF_CODING_ERROR("Couldn't load UsdImagingGLPackageSkinningShader");
        return std::string();
    }

    //const TfToken& kernelKey = _tokens->skinPointsSimpleKernel;
    const TfToken& kernelKey = _tokens->skinPointsLBSKernel;
    
    std::string shaderSource = gfx.GetSource(kernelKey);
    if (!TF_VERIFY(!shaderSource.empty())) {
        TF_WARN("Skinning compute shader is missing kernel '%s'",
                kernelKey.GetText());
        return std::string();
    }

    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg(
        "Kernel for skinning is :\n%s\n", shaderSource.c_str());

    return shaderSource;
}


/* static */
const std::string&
UsdSkelImagingSkeletonAdapter::_GetSkinningComputeKernel()
{
    static const std::string shaderSource(_LoadSkinningComputeKernel());
    return shaderSource;
}

// ---------------------------------------------------------------------- //
/// Handlers for the skinned prim
// ---------------------------------------------------------------------- //

bool
UsdSkelImagingSkeletonAdapter::_IsSkinnedPrimPath(
    const SdfPath& cachePath) const
{
    if (_skinnedPrimToSkelMap.find(cachePath) != _skinnedPrimToSkelMap.end()) {
        return true;
    }
    return false;
}


void
UsdSkelImagingSkeletonAdapter::_TrackSkinnedPrimVariability(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    HdDirtyBits* timeVaryingBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    // Since The SkeletonAdapter hijacks skinned prims (see SkelRootAdapter),
    // make sure to delegate to the actual adapter registered for the prim.
    UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(prim);
    adapter->TrackVariability(prim, cachePath,
                              timeVaryingBits, instancerContext);

    if (_IsAffectedByTimeVaryingJointXforms(cachePath)) {
        (*timeVaryingBits) |= HdChangeTracker::DirtyPoints;
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingPrimvar);
    }
}


void
UsdSkelImagingSkeletonAdapter::_UpdateSkinnedPrimForTime(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    const UsdImagingInstancerContext* instancerContext) const
{
    // For readability
    UsdPrim const& skinnedPrim = prim;
    SdfPath const& skinnedPrimPath = cachePath;

    TF_DEBUG(USDIMAGING_CHANGES).Msg(
        "[UpdateForTime] Skinned prim path: <%s>\n", prim.GetPath().GetText());
    TF_DEBUG(USDIMAGING_CHANGES).Msg
        ("[UpdateForTime] Cache path: <%s>\n", cachePath.GetText());

    // Register points as a computed primvar on the skinned prim.
    if (requestedBits & HdChangeTracker::DirtyPoints) {
        UsdImagingValueCache* valueCache = _GetValueCache();

        HdExtComputationPrimvarDescriptorVector& computedPrimvarsEntry =
            valueCache->GetExtComputationPrimvars(skinnedPrimPath);

        // Note: We don't specify the # of points, since the prim already knows
        // how many to expect for a given topology.
        // The count field below indicates that we have one vec3f per point.
        HdTupleType pointsType;
        pointsType.type = HdTypeFloatVec3;
        pointsType.count = 1;
        
        TF_DEBUG(USDIMAGING_COMPUTATIONS).Msg(
                "[SkeletonAdapter::_UpdateSkinnedPrimForTime] Adding "
                " points as a computed primvar for prim %s\n",
                skinnedPrimPath.GetText());

        HdExtComputationPrimvarDescriptorVector compPrimvars;
        compPrimvars.emplace_back(
                        HdTokens->points,
                        HdInterpolationVertex,
                        HdPrimvarRoleTokens->point,
                        _GetSkinningComputationPath(skinnedPrimPath),
                        _tokens->skinnedPoints,
                        pointsType);
        
        // Overwrite the entire entry (i.e., don't use emplace_back)
        computedPrimvarsEntry = compPrimvars;
    }

    // Suppress the dirtybit for points, so we don't publish 'points' as a
    // primvar.
    requestedBits &= ~HdChangeTracker::DirtyPoints;

    // Since The SkeletonAdapter hijacks skinned prims (see SkelRootAdapter),
    // make sure to delegate to the actual adapter registered for the prim.
    UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(skinnedPrim);
    adapter->UpdateForTime(skinnedPrim, skinnedPrimPath,
                        time, requestedBits, instancerContext);

    
    // Don't publish skinning related primvars since they're consumed only by
    // the computations.
    // XXX: The usage of elementSize for jointWeights/Indices primvars to have
    // multiple values per-vertex is not supported yet in Hydra.
    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        UsdImagingValueCache* valueCache = _GetValueCache();
        HdPrimvarDescriptorVector& primvars =
            valueCache->GetPrimvars(skinnedPrimPath);
        for (auto it = primvars.begin(); it != primvars.end(); ) {
            if (it->name == _tokens->skelJointIndices ||
                it->name == _tokens->skelJointWeights  ||
                it->name == _tokens->skelGeomBindXform) {
                it = primvars.erase(it);
            } else {
                ++it;
            }
        }
    }
}

// ---------------------------------------------------------------------- //
/// _SkelData
// ---------------------------------------------------------------------- //
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
