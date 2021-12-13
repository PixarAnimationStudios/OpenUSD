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
#include "pxr/usdImaging/usdImaging/pointInstancerAdapter.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/version.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/xformable.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/gf/quath.h"

#include <limits>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE


// XXX: These should come from Hd or UsdImaging
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (instance)
    (instancer)
    (rotate)
    (scale)
    (translate)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingPointInstancerAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingPointInstancerAdapter::~UsdImagingPointInstancerAdapter() 
{
}

bool
UsdImagingPointInstancerAdapter::ShouldCullChildren() const
{
    return true;
}

bool
UsdImagingPointInstancerAdapter::IsInstancerAdapter() const
{
    return true;
}

SdfPath
UsdImagingPointInstancerAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _Populate(prim, index, instancerContext);
}

SdfPath
UsdImagingPointInstancerAdapter::_Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    SdfPath const& parentInstancerCachePath =
        instancerContext ? instancerContext->instancerCachePath : SdfPath();
    SdfPath instancerCachePath = prim.GetPath();
    UsdGeomPointInstancer inst(prim);

    if (!inst) {
        TF_WARN("Invalid instancer prim <%s>, instancer scheme was not valid\n",
                instancerCachePath.GetText());
        return SdfPath();
    }

    // for the case we happen to process the same instancer more than once,
    // use variant selection path to make a unique index path (e.g. NI-PI)
    if (_instancerData.find(instancerCachePath) != _instancerData.end()) {
        static std::atomic_int ctr(0);
        std::string name = TfStringify(++ctr);
        instancerCachePath = instancerCachePath.AppendVariantSelection(
                                                "instance",name);
    }

    // ---------------------------------------------------------------------- //
    // Init instancer and fetch authored data needed to drive population
    // ---------------------------------------------------------------------- //

    // Get the prototype target paths. These paths target subgraphs that are to
    // be instanced. As a result, a single path here may result in many rprims
    // for a single declared "prototype".
    SdfPathVector usdProtoPaths;
    UsdRelationship protosRel = inst.GetPrototypesRel();
    if (!protosRel.GetForwardedTargets(&usdProtoPaths)) {
        TF_WARN("Point instancer %s does not have a valid 'prototypes' "
                "relationship. Not adding it to the render index."
                , instancerCachePath.GetText());
        return SdfPath();
    }

    // protoIndices is a required property; it is allowed to be empty if
    // time-varying data is provided via protoIndices.timeSamples. we only
    // check for its definition  since USD doesn't have a cheap mechanism to
    // check if an attribute has data
    UsdAttribute protoIndicesAttr = inst.GetProtoIndicesAttr();
    if (!protoIndicesAttr.HasValue()) {
        TF_WARN("Point instancer %s does not have a 'protoIndices'"
                "attribute. Not adding it to the render index.",
                instancerCachePath.GetText());
        return SdfPath();
    }

    // positions is a required property; it is allowed to be empty if
    // time-varying data is provided via positions.timeSamples. we only
    // check for its definition  since USD doesn't have a cheap mechanism to
    // check if an attribute has data
    UsdAttribute positionsAttr = inst.GetPositionsAttr();
    if (!positionsAttr.HasValue()) {
        TF_WARN("Point instancer %s does not have a 'positions' attribute. "
                "Not adding it to the render index.",
                instancerCachePath.GetText());
        return SdfPath();
    }

    // Erase any data that we may have accumulated for a previous instancer at
    // the same path (given that we should get a PrimResync notice before
    // population, perhaps this is unnecessary?).
    if (!TF_VERIFY(_instancerData.find(instancerCachePath) 
                        == _instancerData.end(), "<%s>\n",
                        instancerCachePath.GetText())) {
        _UnloadInstancer(instancerCachePath, index); 
    }

    // Init instancer data for this point instancer.
    _InstancerData& instrData = _instancerData[instancerCachePath];
    // myself. we want to grab PI adapter even if the PI itself is NI
    // so that the children are bound to the PI adapter.
    UsdImagingPrimAdapterSharedPtr instancerAdapter
        = _GetPrimAdapter(prim, /*ignoreInstancing=*/true);

    // PERFORMANCE: We may allocate more pools than are actually used, so if
    // we're squeezing memory in the future, we could be a little more efficient
    // here.
    instrData.prototypePaths.resize(usdProtoPaths.size());
    instrData.prototypePathIndices.clear();
    instrData.visible = true;
    instrData.variableVisibility = true;
    instrData.parentInstancerCachePath = parentInstancerCachePath;
    instrData.visibleTime = std::numeric_limits<double>::infinity();

    TF_DEBUG(USDIMAGING_INSTANCER)
        .Msg("[Add PI] %s, parentInstancerCachePath <%s>\n",
             instancerCachePath.GetText(), parentInstancerCachePath.GetText());

    // Need to use GetAbsoluteRootOrPrimPath() on instancerCachePath to drop
    // {instance=X} from the path, so usd can find the prim.
    index->InsertInstancer(
        instancerCachePath,
        _GetPrim(instancerCachePath.GetAbsoluteRootOrPrimPath()),
        instancerContext ? instancerContext->instancerAdapter
                         : UsdImagingPrimAdapterSharedPtr());

    // ---------------------------------------------------------------------- //
    // Main Prototype allocation loop.
    // ---------------------------------------------------------------------- //

    // Iterate over all prototypes to allocate the Rprims in the Hydra
    // RenderIndex.
    size_t prototypeCount = instrData.prototypePaths.size();

    // For each prototype, allocate the Rprims.
    for (size_t protoIndex = 0; protoIndex < prototypeCount; ++protoIndex) {

        // -------------------------------------------------------------- //
        // Initialize this prototype.
        // -------------------------------------------------------------- //
        const SdfPath & prototypePath = usdProtoPaths[protoIndex];
        instrData.prototypePaths[protoIndex] = prototypePath;
        instrData.prototypePathIndices[prototypePath] = protoIndex;
        UsdPrim protoRootPrim = _GetPrim(instrData.prototypePaths[protoIndex]);
        if (!protoRootPrim) {
            TF_WARN("Targeted prototype was not found <%s>\n",
                    instrData.prototypePaths[protoIndex].GetText());
            continue;
        }

        // -------------------------------------------------------------- //
        // Traverse the subtree and allocate the Rprims
        // -------------------------------------------------------------- //
        UsdImagingInstancerContext ctx = { instancerCachePath,
                                           /*childName=*/TfToken(),
                                           SdfPath(),
                                           TfToken(),
                                           TfToken(),
                                           instancerAdapter};
        _PopulatePrototype(protoIndex, instrData, protoRootPrim, index, &ctx);
    }

    return instancerCachePath;
}

void
UsdImagingPointInstancerAdapter::_PopulatePrototype(
    int protoIndex,
    _InstancerData& instrData,
    UsdPrim const& protoRootPrim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const *instancerContext)
{
    int protoID = 0;
    size_t primCount = 0;
    size_t instantiatedPrimCount = 0;

    std::vector<UsdPrimRange> treeStack;
    treeStack.push_back(
        UsdPrimRange(protoRootPrim, _GetDisplayPredicateForPrototypes()));
    while (!treeStack.empty()) {
        if (!treeStack.back()) {
            treeStack.pop_back();
            if (!treeStack.empty() && treeStack.back()) {
                // whenever we push a new tree iterator, we leave the
                // last one un-incremented intentionally so we have the
                // residual path. That also means that whenever we pop,
                // must increment the last iterator.
                treeStack.back().increment_begin();
            }
            if (treeStack.empty() || !treeStack.back()) {
                continue;
            }
        }
        UsdPrimRange &range = treeStack.back();
        UsdPrimRange::iterator iter = range.begin();

        // If we encounter native instances, continue traversing inside them.
        // XXX: Should we delegate to instanceAdapter here?
        if (iter->IsInstance()) {
            UsdPrim prototype = iter->GetPrototype();
            UsdPrimRange prototypeRange(
                prototype, _GetDisplayPredicateForPrototypes());
            treeStack.push_back(prototypeRange);

            // Make sure to register a dependency on this instancer with the
            // parent PI.
            index->AddDependency(instancerContext->instancerCachePath, *iter);
            continue;
        }

        // construct instance chain
        // note: paths is stored in the backward of treeStack
        //       (prototype, prototype, ... , instance path)
        //       to get the UsdPrim, use paths.front()
        //
        // for example:
        //
        // ProtoCube   <----+
        //   +-- cube       | (native instance)
        // ProtoA           |  <--+
        //   +-- ProtoCube--+     | (native instance)
        // PointInstancer         |
        //   +-- ProtoA ----------+
        //
        // paths = 
        //    /__Prototype_1/cube
        //    /__Prototype_2/ProtoCube
        //    /PointInstancer/ProtoA

        SdfPathVector instancerChain;
        for (int i = treeStack.size()-1; i >= 0; i--) {
            instancerChain.push_back(treeStack[i].front().GetPath());
        }
        // make sure instancerChain is not empty
        TF_VERIFY(instancerChain.size() > 0);

        // _GetPrimAdapter requires the instance proxy prim path, so:
        UsdPrim instanceProxyPrim = _GetPrim(_GetPrimPathFromInstancerChain(
                instancerChain));

        if (!instanceProxyPrim) {
            range.set_begin(++iter);
            continue;
        }

        // Skip population of non-imageable prims.
        if (UsdImagingPrimAdapter::ShouldCullSubtree(instanceProxyPrim)) {
            TF_DEBUG(USDIMAGING_INSTANCER).Msg("[Instance PI] Discovery of new "
                "prims at or below <%s> pruned by prim type (%s)\n",
                iter->GetPath().GetText(), iter->GetTypeName().GetText());
            iter.PruneChildren();
            range.set_begin(++iter);
            continue;
        }

        UsdImagingPrimAdapterSharedPtr adapter =
            _GetPrimAdapter(instanceProxyPrim, /* ignoreInstancing = */ true);

        // Usd prohibits directly instancing gprims so if the current prim is
        // an instance and has an adapter, warn and skip the prim. Prim types
        // (such as cards) that can be directly instanced can opt out of this
        // via CanPopulateUsdInstance().
        if (instanceProxyPrim.IsInstance() && adapter &&
            !adapter->CanPopulateUsdInstance()) {
            TF_WARN("The gprim at path <%s> was directly instanced. "
                    "In order to instance this prim, put the prim under an "
                    "Xform, and instance the Xform parent.",
                    iter->GetPath().GetText());
            range.set_begin(++iter);
            continue;
        }

        if (adapter) {
            primCount++;

            //
            // prototype allocation.
            //
            
            SdfPath protoPath;
            if (adapter->IsInstancerAdapter()) {
                // if the prim is handled by some kind of multiplexing adapter
                // (e.g. another nested PointInstancer)
                // we'll relocate its children to itself, then no longer need to
                // traverse for this instancer.
                //
                // note that this condition should be tested after IsInstance()
                // above, since UsdImagingInstanceAdapter also returns true for
                // IsInstancerAdapter but it could be instancing something else.
                UsdImagingInstancerContext ctx = {
                    instancerContext->instancerCachePath,
                    instancerContext->childName,
                    instancerContext->instancerMaterialUsdPath,
                    instancerContext->instanceDrawMode,
                    instancerContext->instanceInheritablePurpose,
                    UsdImagingPrimAdapterSharedPtr() };
                protoPath = adapter->Populate(*iter, index, &ctx);
            } else {
                TfToken protoName(
                    TfStringPrintf(
                        "proto%d_%s_id%d", protoIndex,
                        iter->GetPath().GetName().c_str(), protoID++));

                UsdPrim populatePrim = *iter;
                if (iter->IsPrototype() && 
                    TF_VERIFY(instancerChain.size() > 1)) {
                    populatePrim = _GetPrim(instancerChain.at(1));
                }

                SdfPath const& materialId = GetMaterialUsdPath(instanceProxyPrim);
                TfToken const& drawMode = GetModelDrawMode(instanceProxyPrim);
                TfToken const& inheritablePurpose = 
                    GetInheritablePurpose(instanceProxyPrim);
                UsdImagingInstancerContext ctx = {
                    instancerContext->instancerCachePath,
                    /*childName=*/protoName,
                    materialId,
                    drawMode,
                    inheritablePurpose,
                    instancerContext->instancerAdapter };
                protoPath = adapter->Populate(populatePrim, index, &ctx);
            }

            if (adapter->ShouldCullChildren()) {
                iter.PruneChildren();
            }

            if (protoPath.IsEmpty()) {
                // Dont track this prototype if it wasn't actually
                // added.
                range.set_begin(++iter);
                continue;
            }

            TF_DEBUG(USDIMAGING_INSTANCER).Msg(
                "[Add Instance PI] <%s>  %s\n",
                instancerContext->instancerCachePath.GetText(),
                protoPath.GetText());

            //
            // Update instancer data.
            //
            _ProtoPrim& proto = instrData.protoPrimMap[protoPath];
            proto.adapter = adapter;
            proto.protoRootPath = instrData.prototypePaths[protoIndex];
            proto.paths = instancerChain;

            // Book keeping, for debugging.
            instantiatedPrimCount++;
        }
        range.set_begin(++iter);
    }

    TF_DEBUG(USDIMAGING_POINT_INSTANCER_PROTO_CREATED).Msg(
        "Prototype[%d]: <%s>, primCount: %lu, instantiatedPrimCount: %lu\n",
        protoIndex,
        protoRootPrim.GetPath().GetText(),
        primCount,
        instantiatedPrimCount);
}

void 
UsdImagingPointInstancerAdapter::TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext) const
{
    // XXX: This is no good: if an attribute has exactly one time sample, the
    // default value will get cached and never updated. However, if we use an
    // arbitrary time here, attributes which have valid default values and 1
    // time sample will get cached with the wrong result. The solution is to
    // stop guessing about what time to read, which will be done in a future
    // change, which requires a much larger structure change to UsdImaging.
    //
    // Here we choose to favor correctness of the time sample, since we must
    // ensure the correct image is produced for final render.
    UsdTimeCode time(1.0);

    if (IsChildPath(cachePath)) {
        _ProtoPrim& proto =
            const_cast<_ProtoPrim&>(_GetProtoPrim(prim.GetPath(), cachePath));
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(proto.paths.size() > 0, "%s", cachePath.GetText())) {
            return;
        }

        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        proto.adapter->TrackVariability(protoPrim, cachePath,
                                        &proto.variabilityBits);
        *timeVaryingBits |= proto.variabilityBits;

        if (!(proto.variabilityBits & HdChangeTracker::DirtyVisibility)) {
            // Pre-cache visibility, because we now know that it is static for
            // the populated prototype over all time.  protoPrim may be across
            // an instance boundary from protoRootPrim, so compute visibility
            // for each prototype subtree, and then for the final path relative
            // to the proto root.
            UsdPrim protoRootPrim = _GetPrim(proto.protoRootPath);
            for (size_t i = 0; i < proto.paths.size()-1; ++i) {
                _ComputeProtoVisibility(
                    _GetPrim(proto.paths[i+1]).GetPrototype(),
                    _GetPrim(proto.paths[i+0]),
                    time, &proto.visible);
            }
            _ComputeProtoVisibility(protoRootPrim, _GetPrim(proto.paths.back()),
                                    time, &proto.visible);
        }

        // XXX: We handle PI visibility by pushing it onto the prototype;
        // we should fix this.
        _IsVarying(prim,
            UsdGeomTokens->visibility,
            HdChangeTracker::DirtyVisibility,
            UsdImagingTokens->usdVaryingVisibility,
            timeVaryingBits,
            true);

        return;
    } else  if (_InstancerData const* instrData =
                TfMapLookupPtr(_instancerData, cachePath)) {

        // Mark instance indices as time varying if any of the following is 
        // time varying : protoIndices, invisibleIds
        _IsVarying(prim,
            UsdGeomTokens->invisibleIds,
            HdChangeTracker::DirtyInstanceIndex,
            _tokens->instancer,
            timeVaryingBits,
            false) || _IsVarying(prim,
                UsdGeomTokens->protoIndices,
                HdChangeTracker::DirtyInstanceIndex,
                _tokens->instancer,
                timeVaryingBits,
                false);

        // this is for instancer transform.
        _IsTransformVarying(prim,
                            HdChangeTracker::DirtyTransform,
                            UsdImagingTokens->usdVaryingXform,
                            timeVaryingBits);

        // instancer visibility
        HdDirtyBits visBits;
        if (!_IsVarying(prim, 
                    UsdGeomTokens->visibility, 
                    HdChangeTracker::DirtyVisibility,
                    UsdImagingTokens->usdVaryingVisibility,
                    &visBits,
                    true)) 
        {
            // When the instancer visibility doesn't vary over time, pre-cache
            // visibility to avoid fetching it on frame change.
            // XXX: The usage of _GetTimeWithOffset here is super-sketch, but
            // it avoids blowing up the inherited visibility cache...
            // We should let this be initialized by the first UpdateForTime...
            instrData->visible = _GetInstancerVisible(cachePath,
                    _GetTimeWithOffset(0.0));
            instrData->variableVisibility = false;
        } else {
            instrData->variableVisibility = true;
        }

        // Check per-instance transform primvars
        _IsVarying(prim,
                UsdGeomTokens->positions,
                HdChangeTracker::DirtyPrimvar,
                _tokens->instancer,
                timeVaryingBits,
                false) ||
            _IsVarying(prim,
                    UsdGeomTokens->orientations,
                    HdChangeTracker::DirtyPrimvar,
                    _tokens->instancer,
                    timeVaryingBits,
                    false) ||
            _IsVarying(prim,
                    UsdGeomTokens->scales,
                    HdChangeTracker::DirtyPrimvar,
                    _tokens->instancer,
                    timeVaryingBits,
                    false) ||
            _IsVarying(prim,
                    UsdGeomTokens->velocities,
                    HdChangeTracker::DirtyPrimvar,
                    _tokens->instancer,
                    timeVaryingBits,
                    false) ||
            _IsVarying(prim,
                    UsdGeomTokens->accelerations,
                    HdChangeTracker::DirtyPrimvar,
                    _tokens->instancer,
                    timeVaryingBits,
                    false);

        if (!(*timeVaryingBits & HdChangeTracker::DirtyPrimvar)) {
            UsdGeomPrimvarsAPI primvars(prim);
            for (auto const &pv: primvars.GetPrimvarsWithValues()) {
                TfToken const& interp = pv.GetInterpolation();
                if (interp != UsdGeomTokens->uniform &&
                    pv.ValueMightBeTimeVarying()) {
                    *timeVaryingBits |= HdChangeTracker::DirtyPrimvar;
                    HD_PERF_COUNTER_INCR(_tokens->instancer);
                    break;
                }
            }
        }
    }
}

void 
UsdImagingPointInstancerAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();

    if (IsChildPath(cachePath)) {
        // Allow the prototype's adapter to update, if there's anything left
        // to do.
        if (requestedBits != HdChangeTracker::Clean) {
            // cachePath : /path/instancerPath.proto_*
            // instancerPath : /path/instancerPath
            SdfPath instancerPath = cachePath.GetParentPath();
            _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);
            if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
                return;
            }
            if (!TF_VERIFY(proto.paths.size() > 0, "%s", cachePath.GetText())) {
                return;
            }

            UsdPrim protoPrim = _GetProtoUsdPrim(proto);
            proto.adapter->UpdateForTime(
                protoPrim, cachePath, time, requestedBits);
        }
    } else if (_instancerData.find(cachePath) != _instancerData.end()) {
        // For the instancer itself, we only send translate, rotate, scale,
        // velocities, and accelerations back as primvars, which all fall into
        // the DirtyPrimvar bucket currently.
        if (requestedBits & HdChangeTracker::DirtyPrimvar) {
            UsdGeomPointInstancer instancer(prim);

            HdPrimvarDescriptorVector& vPrimvars = 
                primvarDescCache->GetPrimvars(cachePath);

            // PERFORMANCE: It would be nice to track variability of individual
            // primvars separately, since uniform values will  needlessly be
            // sent to the GPU on every frame.
            VtVec3fArray positions;
            if (instancer.GetPositionsAttr().Get(&positions, time)) {
                _MergePrimvar(
                    &vPrimvars,
                    _tokens->translate,
                    HdInterpolationInstance,
                    HdPrimvarRoleTokens->vector);
            }

            VtQuathArray orientations;
            if (instancer.GetOrientationsAttr().Get(&orientations, time)) {
                _MergePrimvar(
                    &vPrimvars,
                    _tokens->rotate,
                    HdInterpolationInstance);
            }

            VtVec3fArray scales;
            if (instancer.GetScalesAttr().Get(&scales, time)) {
                _MergePrimvar(
                    &vPrimvars,
                    _tokens->scale,
                    HdInterpolationInstance);
            }

            VtVec3fArray velocities;
            if (instancer.GetVelocitiesAttr().Get(&velocities, time)) {
                _MergePrimvar(
                    &vPrimvars,
                    HdTokens->velocities,
                    HdInterpolationInstance);
            }

            VtVec3fArray accelerations;
            if (instancer.GetAccelerationsAttr().Get(&accelerations, time)) {
                _MergePrimvar(
                    &vPrimvars,
                    HdTokens->accelerations,
                    HdInterpolationInstance);
            }

            // Convert non-uniform primvars on UsdGeomPointInstancer into
            // instance-rate primvars. Note: this only gets local primvars.
            // Inherited primvars don't vary per-instance, so we let the
            // prototypes pick them up. Include constant-rate local primvars
            // to let the renderer query settings on the point instancer
            // itself, as not all settings can be applied at the prototype
            // level.
            UsdGeomPrimvarsAPI primvars(instancer);
            HdInterpolation interpOverride = HdInterpolationInstance;
            for (auto const &pv: primvars.GetPrimvarsWithValues()) {
                TfToken const& interp = pv.GetInterpolation();
                if (interp != UsdGeomTokens->uniform) {
                    _ComputeAndMergePrimvar(prim, pv, time, &vPrimvars,
                        interp == UsdGeomTokens->constant
                            ? nullptr : &interpOverride);
                }
            }
        }
    }
}

HdDirtyBits
UsdImagingPointInstancerAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    if (IsChildPath(cachePath)) {
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(), cachePath);
        if (!proto.adapter || (proto.paths.size() <= 0)) {
            // It's possible we'll get multiple USD edits for the same
            // prototype, one of which will cause a resync.  On resync,
            // we immediately remove the instancer data, but primInfo
            // deletion is deferred until the end of the edit batch.
            // That means, if GetProtoPrim fails we've already
            // queued the prototype for resync and we can safely
            // return clean (no-work).
            return HdChangeTracker::Clean;
        }

        UsdPrim protoUsdPrim = _GetProtoUsdPrim(proto);
        if (!protoUsdPrim) {
            // It's possible that we will get a property change that was
            // actually directed at a parent primitive for an inherited
            // primvar. We need to verify that the prototype UsdPrim still
            // exists, as it may have been deactivated or otherwise removed,
            // in which case we can return clean (no-work).
            return HdChangeTracker::Clean;
        }

        // XXX: Specifically disallow visibility and transform updates: in
        // these cases, it's hard to tell which prims we should dirty but
        // probably we need to dirty both prototype & instancer. This is a
        // project for later. In the meantime, returning AllDirty causes
        // a re-sync.
        HdDirtyBits dirtyBits = proto.adapter->ProcessPropertyChange(
            protoUsdPrim, cachePath, propertyName);

        if (dirtyBits & (HdChangeTracker::DirtyTransform |
                         HdChangeTracker::DirtyVisibility)) {
            return HdChangeTracker::AllDirty;
        }
        return dirtyBits;
    }

    if (propertyName == UsdGeomTokens->positions ||
        propertyName == UsdGeomTokens->orientations ||
        propertyName == UsdGeomTokens->scales ||
        propertyName == UsdGeomTokens->velocities ||
        propertyName == UsdGeomTokens->accelerations) {

        TfToken primvarName = propertyName;
        if (propertyName == UsdGeomTokens->positions) {
            primvarName = _tokens->translate;
        } else if (propertyName == UsdGeomTokens->orientations) {
            primvarName = _tokens->rotate;
        } else if (propertyName == UsdGeomTokens->scales) {
            primvarName = _tokens->scale;
        } else if (propertyName == UsdGeomTokens->velocities) {
            primvarName = HdTokens->velocities;
        } else if (propertyName == UsdGeomTokens->accelerations) {
            primvarName = HdTokens->accelerations;
        }

        return _ProcessNonPrefixedPrimvarPropertyChange(
                prim, cachePath, propertyName, primvarName,
                HdInterpolationInstance);
    }

    if (propertyName == UsdGeomTokens->protoIndices ||
        propertyName == UsdGeomTokens->invisibleIds) {
        return HdChangeTracker::DirtyInstanceIndex;
    }

    // Is the property a primvar?
    if (UsdGeomPrimvarsAPI::CanContainPropertyName(propertyName)) {
        // Ignore local constant/uniform primvars.
        UsdGeomPrimvar pv = UsdGeomPrimvarsAPI(prim).GetPrimvar(propertyName);
        if (pv && pv.GetInterpolation() == UsdGeomTokens->uniform) {
            return HdChangeTracker::Clean;
        }

        
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName,
            /*valueChangeDirtyBit*/HdChangeTracker::DirtyPrimvar,
            /*inherited=*/false);
    }

    // XXX: Treat transform & visibility changes as re-sync, until we untangle
    // instancer vs proto data.
    if (propertyName == UsdGeomTokens->visibility ||
        UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName)) {
        return HdChangeTracker::AllDirty;
    }

    return HdChangeTracker::Clean;
}

void
UsdImagingPointInstancerAdapter::_ProcessPrimRemoval(SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index,
                                             SdfPathVector* instancersToReload)
{
    SdfPath affectedInstancer;

    // cachePath is from the _dependencyInfo map in the delegate, and points to
    // either a hydra instancer or a hydra prototype (the latter in the case of
    // adapter forwarding).  For hydra prototypes, their name is mangled by the
    // immediate instancer parent: if /World/PI/PI2 has cache path
    // /World/PI/PI2{0}, then /World/PI/PI2/cube will have cache path
    // /World/PI/PI2{0}.proto0_cube_id0 (see _PopulatePrototype). This, then,
    // gives us an easy route to the affected instancer.
    if (IsChildPath(cachePath)) {
        affectedInstancer = cachePath.GetParentPath();
    } else {
        affectedInstancer = cachePath;
    }

    // If the affected instancer is populated, delete it by finding the
    // top-level instancer and calling _UnloadInstancer on that.
    // XXX: It would be nice if we could just remove *this* prim and rely on
    // the resync code to propertly resync it with the right parent instancer.

    _InstancerDataMap::iterator instIt = _instancerData.find(affectedInstancer);

    if (instIt == _instancerData.end()) {
        // Invalid cache path.
        return;
    }

    while (instIt != _instancerData.end()) {
        affectedInstancer = instIt->first;
        SdfPath parentInstancerCachePath =
            instIt->second.parentInstancerCachePath;
        if (parentInstancerCachePath.IsEmpty()) {
            break;
        }
        instIt = _instancerData.find(parentInstancerCachePath);
    }

    // Should we reload affected instancer?
    if (instancersToReload) {
        UsdPrim p = _GetPrim(affectedInstancer.GetPrimPath());
        if (p && p.IsActive()) {
            instancersToReload->push_back(affectedInstancer);
        }
    }
    _UnloadInstancer(affectedInstancer, index);
}

void
UsdImagingPointInstancerAdapter::ProcessPrimResync(SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index)
{
    // _ProcesPrimRemoval does the heavy lifting, returning a set of instancers
    // to repopulate. Note that the child/prototype prims need not be in the
    // "toReload" list, as they will be discovered in the process of reloading
    // the root instancer prim.
    SdfPathVector toReload;
    _ProcessPrimRemoval(cachePath, index, &toReload);
    for (SdfPath const& instancerRootPath : toReload) {
        index->Repopulate(instancerRootPath);
    }
}

/*virtual*/
void
UsdImagingPointInstancerAdapter::ProcessPrimRemoval(SdfPath const& cachePath,
                                                    UsdImagingIndexProxy* index)
{
    // Process removals, but do not repopulate.
    _ProcessPrimRemoval(cachePath, index, /*instancersToRepopulate*/nullptr);
}

void
UsdImagingPointInstancerAdapter::MarkDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           HdDirtyBits dirty,
                                           UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        // cachePath : /path/instancerPath.proto_*
        // instancerPath : /path/instancerPath
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);

        proto.adapter->MarkDirty(prim, cachePath, dirty, index);
    } else {
        index->MarkInstancerDirty(cachePath, dirty);
    }
}

void
UsdImagingPointInstancerAdapter::MarkRefineLevelDirty(
                                                   UsdPrim const& prim,
                                                   SdfPath const& cachePath,
                                                   UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        // cachePath : /path/instancerPath.proto_*
        // instancerPath : /path/instancerPath
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);

        proto.adapter->MarkRefineLevelDirty(prim, cachePath, index);
    }
}

void
UsdImagingPointInstancerAdapter::MarkReprDirty(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        // cachePath : /path/instancerPath.proto_*
        // instancerPath : /path/instancerPath
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);

        proto.adapter->MarkReprDirty(prim, cachePath, index);
    }
}

void
UsdImagingPointInstancerAdapter::MarkCullStyleDirty(UsdPrim const& prim,
                                                    SdfPath const& cachePath,
                                                    UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        // cachePath : /path/instancerPath.proto_*
        // instancerPath : /path/instancerPath
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);

        proto.adapter->MarkCullStyleDirty(prim, cachePath, index);
    }
}

void
UsdImagingPointInstancerAdapter::MarkRenderTagDirty(UsdPrim const& prim,
                                                    SdfPath const& cachePath,
                                                    UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        // cachePath : /path/instancerPath.proto_*
        // instancerPath : /path/instancerPath
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);

        proto.adapter->MarkRenderTagDirty(prim, cachePath, index);
    }
}

void
UsdImagingPointInstancerAdapter::MarkTransformDirty(UsdPrim const& prim,
                                                    SdfPath const& cachePath,
                                                    UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        // cachePath : /path/instancerPath.proto_*
        // instancerPath : /path/instancerPath
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);

        proto.adapter->MarkTransformDirty(prim, cachePath, index);
    } else {
        static const HdDirtyBits transformDirty =
                                                HdChangeTracker::DirtyTransform;

        index->MarkInstancerDirty(cachePath, transformDirty);
    }
}

void
UsdImagingPointInstancerAdapter::MarkVisibilityDirty(
                                                    UsdPrim const& prim,
                                                    SdfPath const& cachePath,
                                                    UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        // cachePath : /path/instancerPath.proto_*
        // instancerPath : /path/instancerPath
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);

        proto.adapter->MarkVisibilityDirty(prim, cachePath, index);
    } else {
        static const HdDirtyBits visibilityDirty =
                                               HdChangeTracker::DirtyVisibility;

        index->MarkInstancerDirty(cachePath, visibilityDirty);
    }
}

void
UsdImagingPointInstancerAdapter::_UnloadInstancer(SdfPath const& instancerPath,
                                            UsdImagingIndexProxy* index)
{
    _InstancerDataMap::iterator instIt = _instancerData.find(instancerPath);

    // Note: If any of the prototype children is a point instancer, their
    // ProcessPrimRemoval will try to forward the removal call to the
    // top-level instancer that has an entry in _instancerData.  This means,
    // to avoid infinite loops, that we need to remove the _instancerData
    // entry for this instancer before removing prototypes.

    const _ProtoPrimMap protoPrimMap = instIt->second.protoPrimMap;
    _instancerData.erase(instIt);

    // First, we need to make sure all proto rprims are removed.
    for (auto const& pair : protoPrimMap) {
        // pair: <cache path, _ProtoPrim>
        pair.second.adapter->ProcessPrimRemoval(pair.first, index);
    }

    // Blow away the instancer and the associated local data.
    index->RemoveInstancer(instancerPath);
}

// -------------------------------------------------------------------------- //
// Private IO Helpers
// -------------------------------------------------------------------------- //

UsdImagingPointInstancerAdapter::_ProtoPrim const&
UsdImagingPointInstancerAdapter::_GetProtoPrim(SdfPath const& instrPath, 
                                                 SdfPath const& cachePath) const
{
    static _ProtoPrim const EMPTY;
    SdfPath const& instancerPath =
        (cachePath.GetParentPath().IsPrimVariantSelectionPath())
            ? cachePath.GetParentPath()
            : instrPath;

    _InstancerDataMap::const_iterator it = _instancerData.find(instancerPath);
    if (it == _instancerData.end()) {
        return EMPTY;
    }
    _ProtoPrimMap::const_iterator protoPrimIt
        = it->second.protoPrimMap.find(cachePath);
    if (protoPrimIt == it->second.protoPrimMap.end()) {
        return EMPTY;
    }
    return protoPrimIt->second;
}

bool 
UsdImagingPointInstancerAdapter::_GetProtoPrimForChild(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    _ProtoPrim const** proto,
    UsdImagingInstancerContext* ctx) const
{
    if (IsChildPath(cachePath)) {
        *proto = &_GetProtoPrim(usdPrim.GetPath(), cachePath);
        if (!TF_VERIFY(*proto)) {
            return false;
        }
        UsdPrim protoPrim = _GetProtoUsdPrim(**proto);

        // The instancer path since IsChildPath is true
        const SdfPath instancerPath = cachePath.GetParentPath();

        ctx->instancerCachePath = instancerPath;
        ctx->childName = cachePath.GetNameToken();
        ctx->instancerMaterialUsdPath = SdfPath();
        ctx->instanceDrawMode = TfToken();
        ctx->instanceInheritablePurpose = TfToken();
        ctx->instancerAdapter = const_cast<UsdImagingPointInstancerAdapter *>
            (this)->shared_from_this();
        return true;
    } else {
        return false;
    }
}

const UsdPrim
UsdImagingPointInstancerAdapter::_GetProtoUsdPrim(
    _ProtoPrim const& proto) const
{
    // proto.paths.front() is the most local path for the rprim.
    // If it's not native-instanced, proto.paths will be size 1.
    // If it is native-instanced, proto.paths may look like
    //   /__Prototype_1/prim
    //   /Instance
    // where /__Prototype_1/prim is the pointer to the actual prim in question.
    UsdPrim prim = _GetPrim(proto.paths.front());

    // One exception: if the prototype is an instance, proto.paths looks like
    //   /__Prototype_1
    //   /Instance
    // ... in which case, we want to return /Instance since prototypes drop all
    // attributes.
    if (prim && prim.IsPrototype() && TF_VERIFY(proto.paths.size() > 1)) {
        prim = _GetPrim(proto.paths.at(1));
    }
    return prim;
}

bool
UsdImagingPointInstancerAdapter::_GetInstancerVisible(
    SdfPath const &instancerPath, UsdTimeCode time) const
{
    bool visible = UsdImagingPrimAdapter::GetVisible(
        _GetPrim(instancerPath.GetPrimPath()), 
        instancerPath,
        time);

    if (visible) {
        _InstancerDataMap::const_iterator it
            = _instancerData.find(instancerPath);
        if (it != _instancerData.end()) {
            // note that parent instancer may not be a namespace parent
            // (e.g. prototype -> instance)
            SdfPath const &parentInstancerCachePath =
                it->second.parentInstancerCachePath;
            if (!parentInstancerCachePath.IsEmpty()) {
                return _GetInstancerVisible(parentInstancerCachePath, time);
            }
        }
    }

    return visible;
}

void
UsdImagingPointInstancerAdapter::_UpdateInstancerVisibility(
        SdfPath const& instancerPath,
        _InstancerData const& instrData,
        UsdTimeCode time) const
{
    TF_DEBUG(USDIMAGING_INSTANCER).Msg(
        "[PointInstancer::_UpdateInstancerVisibility] %s\n",
        instancerPath.GetText());

    if (instrData.variableVisibility) {
        // If visibility is variable, each prototype will be updating the
        // instancer visibility here, so make sure we lock before retrieving
        // or computing it.
        std::lock_guard<std::mutex> lock(instrData.mutex);

        // Grab the instancer visibility, if it varies over time.
        bool upToDate = instrData.visibleTime == time;
        if (!upToDate) {
            instrData.visible = _GetInstancerVisible(instancerPath, time);
            instrData.visibleTime = time;
        }
    }
}

GfMatrix4d
UsdImagingPointInstancerAdapter::_CorrectTransform(
    UsdPrim const& instancer,
    UsdPrim const& protoRoot,
    SdfPath const& cachePath,
    SdfPathVector const& protoPathChain,
    GfMatrix4d const& inTransform,
    UsdTimeCode time) const
{
    // Subtract out the parent transform from prototypes (in prototype time).
    //
    // Need to track instancer transform variability (this should be
    // fine, as long as the prototypes live under the instancer).

    // - delegate-root-transform
    //      root transform applied to entire prims in a delegate.
    // - proto-root-transform
    //      transform of the each prototype root usd-prim
    // - proto-gprim-transform
    //      transform of the each prototype Rprim

    // Our Hydra convention applies the delegate-root-transform to instancer,
    // not to a prototype (required for nested instancing).
    // Compute inverse to extract root transform from prototypes too.
    GfMatrix4d inverseRootTransform = GetRootTransform().GetInverse();

    // First, GprimAdapter has already populated transform of the protoPrim into
    // value cache, including the delegate-root-transform, because GprimAdapter
    // doesn't know if it's a prototype of point instancer or not.
    //
    //  (see UsdImagingGprimAdapter::UpdateForTime. GetTransform() doesn't
    //   specify ignoreRootTransform parameter)
    //
    // We want to store the relative transform for each prototype rprim.
    // Subtract the delegate-root-transform.
    GfMatrix4d protoGprimToWorld = inTransform;
    protoGprimToWorld = protoGprimToWorld * inverseRootTransform;

    // If this is nested instancer (has parent),
    for (size_t i = 1; i < protoPathChain.size(); ++i) {
        // ignore root transform of nested instancer chain
        //
        // PI ---(protoRoot)--- NI:XFM
        //                          ^
        //                       This matrix, we're applying
        protoGprimToWorld *= BaseAdapter::GetTransform(
            _GetPrim(protoPathChain[i]), 
            protoPathChain[i],
            time,
            /*ignoreRootTransform=*/true);
    }

    // Then, we also need to subtract transform above the proto root to avoid
    // double transform of instancer and prototypes.
    // Compute the transform of the proto root, 
    // excluding delegate-root-transform.
    //
    // PI(or whatever):XFM---(protoRoot)--- NI (or whatever)
    //                 ^
    //      This matrix, we're subtracting
    UsdPrim parent = protoRoot.GetParent();
    if (parent) {
        GfMatrix4d parentToWorld = GetTransform(
            parent, parent.GetPath(), time, /*ignoreRootTransform=*/true);

        // protoRootToWorld includes its own transform AND root transform,
        // GetInverse() extracts both transforms.
        protoGprimToWorld = protoGprimToWorld * parentToWorld.GetInverse();
    }
    return protoGprimToWorld;
}

void
UsdImagingPointInstancerAdapter::_ComputeProtoVisibility(
                                 UsdPrim const& protoRoot,
                                 UsdPrim const& protoGprim,
                                 UsdTimeCode time,
                                 bool* vis) const
{
    if (!TF_VERIFY(vis)) { return; }
    if (!protoGprim.GetPath().HasPrefix(protoRoot.GetPath())) {
        TF_CODING_ERROR("Prototype <%s> is not prefixed under "
                "proto root <%s>\n",
                protoGprim.GetPath().GetText(),
                protoRoot.GetPath().GetText());
        return;
    }

    // if it's in invised list, set vis to false
    if (_IsInInvisedPaths(protoGprim.GetPath())) {
        *vis = false;
        return;
    }

    // Recurse until we get to the protoRoot. With this recursion, we'll
    // process the protoRoot first, then a child, down to the protoGprim.
    //
    // Skip all prototypes, since they can't have an opinion.
    if (!protoGprim.IsPrototype() &&
        protoRoot != protoGprim && protoGprim.GetParent()) {
        _ComputeProtoVisibility(protoRoot, protoGprim.GetParent(), time, vis);
    }

    // If an ancestor set vis to false, we need not check any other prims.
    if (!*vis)
        return;

    // Check visibility of this prim.
    TfToken visToken;
    if (UsdGeomImageable(protoGprim).GetVisibilityAttr().Get(&visToken, time) 
                                    && visToken == UsdGeomTokens->invisible) {
        *vis = false;
        return;
    }
}

/* virtual */
SdfPath
UsdImagingPointInstancerAdapter::GetScenePrimPath(
    SdfPath const& cachePath,
    int instanceIndex,
    HdInstancerContext *instancerContext) const
{
    HD_TRACE_FUNCTION();

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "GetScenePrimPath: proto = %s\n", cachePath.GetText());

    SdfPath instancerPath;
    SdfPath protoPath = cachePath;

    // If the prototype is an rprim, the instancer path is just the parent path.
    if (IsChildPath(cachePath)) {
        instancerPath = cachePath.GetParentPath();
    } else {
        // If the prototype is a PI, then cache path will be in the prim map
        // of one of the instancers.  If it's a UsdGeomPointInstancer, we can
        // look it up directly, and get the parent path that way. Otherwise,
        // we need to loop all instancers.
        _InstancerDataMap::const_iterator it =
            _instancerData.find(cachePath);
        if (it != _instancerData.end()) {
            instancerPath = it->second.parentInstancerCachePath;
        } else {
            for (auto const& pair : _instancerData) {
                if (pair.second.protoPrimMap.count(cachePath) > 0) {
                    instancerPath = pair.first;
                    break;
                }
            }
        }
    }

    // If we couldn't determine instancerPath, bail.
    if (instancerPath == SdfPath()) {
        return SdfPath();
    }

    _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);
    if (!proto.adapter) {
        // If proto.adapter is null, _GetProtoPrim failed.
        return SdfPath();
    }
    SdfPath primPath = _GetPrimPathFromInstancerChain(proto.paths);

    // If the prim path is in prototype, we need the help of the parent
    // instancer to figure out what the right instance is.  We assume:
    // 1.) primPath and instancerPath are inside the same prototype.
    // 2.) recursing gives us the fully-qualified version of instancerPath.
    //
    // If:
    // - primPath is /__Prototype_1/Instancer/protos/A
    // - instancerPath is /__Prototype_1/Instancer
    // - parentPath is /World/Foo/Instance
    //
    // Let fqInstancerPath = parentAdapter->GetScenePrimPath(instancerPath);
    // - fqInstancerPath = /World/Bar/Instance/Instancer
    // Then instancePath = /World/Bar/Instance
    // instancePath = fqInstancerPath - instancerPath, or traversing up
    // until we hit the first instance.
    //
    // Finally:
    // - fqPrimPath = instancePath + primPath
    //   = /World/Bar/Instance/Instancer/protos/A
    //
    // We also recurse here to fill in instancerContext.

    // Look up the parent instancer of this instancer.
    _InstancerDataMap::const_iterator it = _instancerData.find(instancerPath);
    if (it == _instancerData.end()) {
        return SdfPath();
    }
    SdfPath parentPath = it->second.parentInstancerCachePath;

    // Compute the local & parent instance index.
    VtValue indicesValue = GetInstanceIndices(_GetPrim(instancerPath),
            instancerPath, cachePath, _GetTimeWithOffset(0.0));

    if (!indicesValue.IsHolding<VtIntArray>()) {
        return SdfPath();
    }
    VtIntArray const & indices = indicesValue.UncheckedGet<VtIntArray>();

    // instanceIndex = parentIndex * indices.size() + i.
    int parentIndex = instanceIndex / indices.size();
    // indices[i] gives the offset into the index buffers (i.e. protoIndices).
    int localIndex = indices[instanceIndex % indices.size()];

    // Find out the fully-qualified parent path. If there is none, the
    // one we have is fully qualified.
    SdfPath fqInstancerPath = instancerPath;
    UsdPrim parentInstancerUsdPrim =
        _GetPrim(parentPath.GetAbsoluteRootOrPrimPath());
    if (parentInstancerUsdPrim) {
        UsdImagingPrimAdapterSharedPtr parentAdapter =
            _GetPrimAdapter(parentInstancerUsdPrim);
        if (!TF_VERIFY(parentAdapter, "%s",
                       parentPath.GetAbsoluteRootOrPrimPath().GetText())) {
            return SdfPath();
        }
        fqInstancerPath =
            parentAdapter->GetScenePrimPath(instancerPath, parentIndex,
                                            instancerContext);
    }

    // Append to the instancer context.
    if (instancerContext != nullptr) {
        instancerContext->push_back(
            std::make_pair(fqInstancerPath, localIndex));
    }

    // Check if primPath is in prototype, and if so check if the instancer
    // is in the same prototype...
    UsdPrim prim = _GetPrim(primPath);
    if (!prim || !prim.IsInPrototype()) {
        return primPath;
    }
    UsdPrim instancer = _GetPrim(instancerPath.GetAbsoluteRootOrPrimPath());
    if (!instancer || !instancer.IsInPrototype() ||
        prim.GetPrototype() != instancer.GetPrototype()) {
        TF_CODING_ERROR("primPath <%s> and instancerPath <%s> are not in "
                        "the same prototype", primPath.GetText(),
                        instancerPath.GetText());
        return SdfPath();
    }

    // Stitch the paths together.
    UsdPrim fqInstancer = _GetPrim(fqInstancerPath);
    if (!fqInstancer || !fqInstancer.IsInstanceProxy()) {
        return SdfPath();
    }
    while (fqInstancer && !fqInstancer.IsInstance()) {
        fqInstancer = fqInstancer.GetParent();
    }
    SdfPath instancePath = fqInstancer.GetPath();

    SdfPathVector paths = { primPath, instancePath };
    return _GetPrimPathFromInstancerChain(paths);
}

static 
size_t
_GatherAuthoredTransformTimeSamples(
    UsdPrim const& prim,
    GfInterval interval,
    std::vector<double>* timeSamples) 
{
    UsdPrim p = prim;
    while (p) {
        // XXX We could do some caching here
        if (UsdGeomXformable xf = UsdGeomXformable(p)) {
            std::vector<double> localTimeSamples;
            xf.GetTimeSamplesInInterval(interval, &localTimeSamples);

            // Join timesamples 
            timeSamples->insert(
                timeSamples->end(), 
                localTimeSamples.begin(), 
                localTimeSamples.end());
        }
        p = p.GetParent();
    }

    // Sort here
    std::sort(timeSamples->begin(), timeSamples->end());
    timeSamples->erase(
        std::unique(timeSamples->begin(), 
            timeSamples->end()), 
            timeSamples->end());

    return timeSamples->size();
}

/*virtual*/
GfMatrix4d
UsdImagingPointInstancerAdapter::GetInstancerTransform(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerPath,
    UsdTimeCode time) const
{
    TRACE_FUNCTION();

    _InstancerDataMap::const_iterator inst = _instancerData.find(instancerPath);
    if (!TF_VERIFY(inst != _instancerData.end(),
                   "Unknown instancer %s", instancerPath.GetText())) {
        return GfMatrix4d(1);
    }

    SdfPath parentInstancerCachePath = inst->second.parentInstancerCachePath;
    if (!parentInstancerCachePath.IsEmpty()) {
        // If nested, double transformation should be avoided.
        SdfPath parentInstancerUsdPath =
            parentInstancerCachePath.GetAbsoluteRootOrPrimPath();
        UsdPrim parentInstancerUsdPrim = _GetPrim(parentInstancerUsdPath);
        UsdImagingPrimAdapterSharedPtr adapter =
            _GetPrimAdapter(parentInstancerUsdPrim);

        // ParentInstancer doesn't necessarily be UsdGeomPointInstancer.
        // lookup and delegate adapter to compute the instancer 
        // transform.
        return adapter->GetRelativeInstancerTransform(
                parentInstancerCachePath, 
                instancerPath, 
                time);
    } else {
        // If not nested, simply output the transform of the instancer.
        return GetRelativeInstancerTransform(parentInstancerCachePath, 
                                             instancerPath, 
                                             time);
    }
}

/*virtual*/
SdfPath
UsdImagingPointInstancerAdapter::GetInstancerId(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath) const
{
    if (IsChildPath(cachePath)) {
        // If this is called on behalf of an rprim, the rprim's name will be
        // /path/to/instancer.name_of_proto, so just take the parent path.
        return cachePath.GetParentPath();
    } else if (_InstancerData const* instrData =
            TfMapLookupPtr(_instancerData, cachePath)) {
        // Otherwise, look up the parent in the instancer data.
        return instrData->parentInstancerCachePath;
    } else {
        TF_CODING_ERROR("Unexpected path <%s>", cachePath.GetText());
        return SdfPath::EmptyPath();
    }
}

/*virtual*/
SdfPathVector
UsdImagingPointInstancerAdapter::GetInstancerPrototypes(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const
{
    HD_TRACE_FUNCTION();

    if (IsChildPath(cachePath)) {
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->GetInstancerPrototypes(protoPrim, cachePath);
    } else {
        SdfPathVector prototypes;
        if (const _InstancerData* instancerData =
                TfMapLookupPtr(_instancerData, cachePath)) {
            for (_ProtoPrimMap::const_iterator i =
                    instancerData->protoPrimMap.cbegin();
                    i != instancerData->protoPrimMap.cend(); ++i) {
                prototypes.push_back(i->first);
            }
        }
        return prototypes;
    }
}

/*virtual*/
size_t
UsdImagingPointInstancerAdapter::SampleInstancerTransform(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerPath,
    UsdTimeCode time,
    size_t maxNumSamples,
    float *sampleTimes,
    GfMatrix4d *sampleValues)
{
    HD_TRACE_FUNCTION();

    if (maxNumSamples == 0) {
        return 0;
    }

    // This code must match how UpdateForTime() computes instancerTransform.
    _InstancerDataMap::iterator inst = _instancerData.find(instancerPath);
    if (!TF_VERIFY(inst != _instancerData.end(),
                   "Unknown instancer %s", instancerPath.GetText())) {
        return 0;
    }
    SdfPath parentInstancerCachePath = inst->second.parentInstancerCachePath;
    GfInterval interval = _GetCurrentTimeSamplingInterval();

    // Add time samples at the boudary conditions
    size_t numSamples = 0;
    std::vector<double> timeSamples;
    timeSamples.push_back(interval.GetMin());
    timeSamples.push_back(interval.GetMax());

    if (!parentInstancerCachePath.IsEmpty()) {
        // if nested, double transformation should be avoided.
        SdfPath parentInstancerUsdPath =
            parentInstancerCachePath.GetAbsoluteRootOrPrimPath();
        UsdPrim parentInstancerUsdPrim = _GetPrim(parentInstancerUsdPath);
        UsdImagingPrimAdapterSharedPtr adapter =
            _GetPrimAdapter(parentInstancerUsdPrim);

        numSamples = _GatherAuthoredTransformTimeSamples(
            parentInstancerUsdPrim, 
            interval, 
            &timeSamples);

        size_t numSamplesToEvaluate = std::min(maxNumSamples, numSamples);
        for (size_t i=0; i < numSamplesToEvaluate; ++i) {
            sampleTimes[i] = timeSamples[i] - time.GetValue();
            sampleValues[i] = adapter->GetRelativeInstancerTransform(
                parentInstancerCachePath, 
                instancerPath,
                timeSamples[i]);
        }
    } else {
        numSamples = _GatherAuthoredTransformTimeSamples(
            _GetPrim(instancerPath), 
            interval, 
            &timeSamples);

        size_t numSamplesToEvaluate = std::min(maxNumSamples, numSamples);
        for (size_t i=0; i < numSamplesToEvaluate; ++i) {
            sampleTimes[i] = timeSamples[i] - time.GetValue();
            sampleValues[i] = GetRelativeInstancerTransform(
                parentInstancerCachePath, 
                instancerPath,
                timeSamples[i]);
        }
    }
    return numSamples;
}

GfMatrix4d 
UsdImagingPointInstancerAdapter::GetTransform(UsdPrim const& prim, 
                                              SdfPath const& cachePath,
                                              UsdTimeCode time,
                                              bool ignoreRootTransform) const
{
    GfMatrix4d output(1.0);

    if (!IsChildPath(cachePath)) {
        return BaseAdapter::GetTransform(prim, 
                                        cachePath, 
                                        time, 
                                        ignoreRootTransform);
    }

    // cachePath : /path/instancerPath.proto_*
    // instancerPath : /path/instancerPath
    SdfPath instancerPath = cachePath.GetParentPath();
    _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);
    if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
        return output;
    }
    if (!TF_VERIFY(proto.paths.size() > 0, "%s", cachePath.GetText())) {
        return output;
    }

    UsdPrim protoPrim = _GetProtoUsdPrim(proto);

    output = proto.adapter->GetTransform(
        protoPrim, 
        cachePath, 
        time,
        ignoreRootTransform);

    // If the prototype we're processing is a prototype, _GetProtoUsdPrim
    // will return us the instance for attribute lookup; but the
    // instance transform for that instance is already accounted for in
    // _CorrectTransform.  Prototypes don't have any transform aside from
    // the root transform, so override the result of UpdateForTime.
    if (protoPrim.IsInstance()) {
        output = GetRootTransform();
    }

    // Correct the transform for various shenanigans: NI transforms,
    // delegate root transform, proto root transform.
    return _CorrectTransform(prim, _GetPrim(proto.protoRootPath), cachePath, 
        proto.paths, output, time);
}

size_t
UsdImagingPointInstancerAdapter::SampleTransform(
    UsdPrim const& usdPrim, 
    SdfPath const& cachePath,
    UsdTimeCode time,
    size_t maxNumSamples,
    float *sampleTimes,
    GfMatrix4d *sampleValues)
{
    if (maxNumSamples == 0) {
        return 0;
    }

    // Pull a single sample from the value-cached transform.
    // This makes the (hopefully safe) assumption that we do not need
    // motion blur on the underlying prototypes.
    sampleTimes[0] = 0.0;
    sampleValues[0] = GetTransform(usdPrim, cachePath, time);
    return 1;
}

size_t
UsdImagingPointInstancerAdapter::SamplePrimvar(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time, 
    size_t maxNumSamples, 
    float *sampleTimes, 
    VtValue *sampleValues,
    VtIntArray *sampleIndices)
{
    HD_TRACE_FUNCTION();

    if (maxNumSamples == 0) {
        return 0;
    }

    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(),
                                                   cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->SamplePrimvar(
            protoPrim, cachePath, key, time,
            maxNumSamples, sampleTimes, sampleValues, sampleIndices);
    } else {
        // Map Hydra-PI transform keys to their USD equivalents.
        TfToken usdKey = key;
        if (key == _tokens->translate) {
            usdKey = UsdGeomTokens->positions;
        } else if (key == _tokens->scale) {
            usdKey = UsdGeomTokens->scales;
        } else if (key == _tokens->rotate) {
            usdKey = UsdGeomTokens->orientations;
        } else if (key == HdTokens->velocities) {
            usdKey = UsdGeomTokens->velocities;
        } else if (key == HdTokens->accelerations) {
            usdKey = UsdGeomTokens->accelerations;
        }
        return UsdImagingPrimAdapter::SamplePrimvar(
            usdPrim, cachePath, usdKey, time,
            maxNumSamples, sampleTimes, sampleValues, sampleIndices);
    }
}

/*virtual*/
bool 
UsdImagingPointInstancerAdapter::GetVisible(UsdPrim const& prim, 
                                            SdfPath const& cachePath,
                                            UsdTimeCode time) const
{
    // Apply the instancer visibility at the current time to the
    // instance. Notice that the instance will also pickup the instancer
    // visibility at the time offset.

    if (IsChildPath(cachePath)) {
        bool vis = false;

        // cachePath : /path/instancerPath.proto_*
        // instancerPath : /path/instancerPath
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return vis;
        }
        if (!TF_VERIFY(proto.paths.size() > 0, "%s", cachePath.GetText())) {
            return vis;
        }

        bool protoHasFixedVis = !(proto.variabilityBits
                & HdChangeTracker::DirtyVisibility);
        _InstancerDataMap::const_iterator it = 
            _instancerData.find(instancerPath);
        if (TF_VERIFY(it != _instancerData.end())) {
            _InstancerData const& instrData = it->second;
            _UpdateInstancerVisibility(instancerPath, instrData, time);
            vis = instrData.visible;
        }

        if (protoHasFixedVis) {
            // The instancer is visible and the proto prim has fixed
            // visibility (it does not vary over time), we can use the
            // pre-cached visibility.
            vis = vis && proto.visible;
        } else if (vis) {
            // The instancer is visible and the prototype has varying
            // visibility, we must compute visibility from the proto
            // prim to the model instance root.
            for (size_t i = 0; i < proto.paths.size()-1; ++i) {
                _ComputeProtoVisibility(
                    _GetPrim(proto.paths[i+1]).GetPrototype(),
                    _GetPrim(proto.paths[i+0]),
                    time, &vis);
            }
            _ComputeProtoVisibility(
                _GetPrim(proto.protoRootPath),
                _GetPrim(proto.paths.back()),
                time, &vis);
        }

        return vis;
    }

    return BaseAdapter::GetVisible(prim, cachePath, time);
}

/*virtual*/
TfToken 
UsdImagingPointInstancerAdapter::GetPurpose(
    UsdPrim const& usdPrim, 
    SdfPath const& cachePath,
    TfToken const& instanceInheritablePurpose) const
{
    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoUsdPrim = _GetProtoUsdPrim(proto);

        UsdPrim instanceProxyPrim = _GetPrim(_GetPrimPathFromInstancerChain(
                proto.paths));

        TfToken const& inheritablePurpose = 
                    GetInheritablePurpose(instanceProxyPrim);

        return proto.adapter->GetPurpose(protoUsdPrim, cachePath, 
                                         inheritablePurpose);
    }
    return BaseAdapter::GetPurpose(usdPrim, cachePath, TfToken());

}

/*virtual*/
PxOsdSubdivTags
UsdImagingPointInstancerAdapter::GetSubdivTags(UsdPrim const& usdPrim,
                                               SdfPath const& cachePath,
                                               UsdTimeCode time) const
{
    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->GetSubdivTags(protoPrim, cachePath, time);
    }
    return BaseAdapter::GetSubdivTags(usdPrim, cachePath, time);
}

/*virtual*/
VtValue
UsdImagingPointInstancerAdapter::GetTopology(UsdPrim const& usdPrim,
                                             SdfPath const& cachePath,
                                             UsdTimeCode time) const
{
    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->GetTopology(protoPrim, cachePath, time);
    }
    return BaseAdapter::GetTopology(usdPrim, cachePath, time);
}

/*virtual*/
HdCullStyle 
UsdImagingPointInstancerAdapter::GetCullStyle(UsdPrim const& usdPrim,
                                             SdfPath const& cachePath,
                                             UsdTimeCode time) const
{
    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->GetCullStyle(protoPrim, cachePath, time);
    }
    return BaseAdapter::GetCullStyle(usdPrim, cachePath, time);
}

/*virtual*/
GfRange3d 
UsdImagingPointInstancerAdapter::GetExtent(UsdPrim const& usdPrim, 
                                           SdfPath const& cachePath, 
                                           UsdTimeCode time) const
{
    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->GetExtent(protoPrim, cachePath, time);
    }
    return BaseAdapter::GetExtent(usdPrim, cachePath, time);
}


/*virtual*/
bool 
UsdImagingPointInstancerAdapter::GetDoubleSided(UsdPrim const& usdPrim, 
                                                SdfPath const& cachePath, 
                                                UsdTimeCode time) const
{
    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->GetDoubleSided(protoPrim, cachePath, time);
    }
    return BaseAdapter::GetDoubleSided(usdPrim, cachePath, time);
}

/*virtual*/
SdfPath
UsdImagingPointInstancerAdapter::GetMaterialId(UsdPrim const& usdPrim, 
                      SdfPath const& cachePath, 
                      UsdTimeCode time) const
{
    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        SdfPath materialId =
            proto.adapter->GetMaterialId(protoPrim, cachePath, time);
        if (!materialId.IsEmpty()) {
            return materialId;
        }
        // If the child prim doesn't have a material ID, see if there's
        // an instancer material path...
        UsdPrim instanceProxyPrim = _GetPrim(_GetPrimPathFromInstancerChain(
                    proto.paths));
        return GetMaterialUsdPath(instanceProxyPrim);
    }
    return BaseAdapter::GetMaterialId(usdPrim, cachePath, time);
}

/*virtual*/
VtValue
UsdImagingPointInstancerAdapter::Get(UsdPrim const& usdPrim,
                                     SdfPath const& cachePath,
                                     TfToken const& key,
                                     UsdTimeCode time,
                                     VtIntArray *outIndices) const
{
    TRACE_FUNCTION();

    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->Get(protoPrim, cachePath, key, time, outIndices);

    } else  if (_InstancerData const* instrData =
                TfMapLookupPtr(_instancerData, cachePath)) {
        TF_UNUSED(instrData);

        if (key == _tokens->translate) {
            UsdGeomPointInstancer instancer(usdPrim);
            VtVec3fArray positions;
            if (instancer.GetPositionsAttr().Get(&positions, time)) {
                return VtValue(positions);
            }

        } else if (key == _tokens->rotate) {
            UsdGeomPointInstancer instancer(usdPrim);
            VtQuathArray orientations;
            if (instancer.GetOrientationsAttr().Get(&orientations, time)) {
                return VtValue(orientations);
            }

        } else if (key == _tokens->scale) {
            UsdGeomPointInstancer instancer(usdPrim);
            VtVec3fArray scales;
            if (instancer.GetScalesAttr().Get(&scales, time)) {
                return VtValue(scales);
            }

        } else if (key == HdTokens->velocities) {
            UsdGeomPointInstancer instancer(usdPrim);
            VtVec3fArray velocities;
            if (instancer.GetVelocitiesAttr().Get(&velocities, time)) {
                return VtValue(velocities);
            }

        } else if (key == HdTokens->accelerations) {
            UsdGeomPointInstancer instancer(usdPrim);
            VtVec3fArray accelerations;
            if (instancer.GetAccelerationsAttr().Get(&accelerations, time)) {
                return VtValue(accelerations);
            }

        } else {
            UsdGeomPrimvarsAPI primvars(usdPrim);
            if (UsdGeomPrimvar pv = primvars.GetPrimvar(key)) {
                VtValue value;
                if (outIndices) {
                    if (pv && pv.Get(&value, time)) {
                        pv.GetIndices(outIndices, time);
                        return value;
                    }
                } else if (pv && pv.ComputeFlattened(&value, time)) {
                    return value;
                }
            }
        }
    }

    return BaseAdapter::Get(usdPrim, cachePath, key, time, outIndices);
}

/*virtual*/
HdExtComputationInputDescriptorVector
UsdImagingPointInstancerAdapter::GetExtComputationInputs(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext ctx;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &ctx)) {

        return proto->adapter->GetExtComputationInputs(
                _GetProtoUsdPrim(*proto), cachePath, &ctx);
    }
    return BaseAdapter::GetExtComputationInputs(usdPrim, cachePath, nullptr);
}

/*virtual*/
HdExtComputationOutputDescriptorVector
UsdImagingPointInstancerAdapter::GetExtComputationOutputs(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* /*unused*/) const

{
    UsdImagingInstancerContext ctx;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &ctx)) {

        return proto->adapter->GetExtComputationOutputs(
                _GetProtoUsdPrim(*proto), cachePath, &ctx);
    }
    return BaseAdapter::GetExtComputationOutputs(usdPrim, cachePath, nullptr);
}

/*virtual*/
HdExtComputationPrimvarDescriptorVector
UsdImagingPointInstancerAdapter::GetExtComputationPrimvars(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    HdInterpolation interpolation,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext ctx;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &ctx)) {

        return proto->adapter->GetExtComputationPrimvars(
                _GetProtoUsdPrim(*proto), cachePath, interpolation, &ctx);
    }
    return BaseAdapter::GetExtComputationPrimvars(usdPrim, cachePath, 
            interpolation, nullptr);
}

/*virtual*/
VtValue 
UsdImagingPointInstancerAdapter::GetExtComputationInput(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    TfToken const& name,
    UsdTimeCode time,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext ctx;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &ctx)) {

        return proto->adapter->GetExtComputationInput(
                _GetProtoUsdPrim(*proto), cachePath, name, time, &ctx);
    }
    return BaseAdapter::GetExtComputationInput(usdPrim, cachePath, name, time,
                nullptr);
}

/*virtual*/
VtValue
UsdImagingPointInstancerAdapter::GetInstanceIndices(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerCachePath,
    SdfPath const& prototypeCachePath,
    UsdTimeCode time) const
{
    if (IsChildPath(instancerCachePath)) {
        UsdImagingInstancerContext ctx;
        _ProtoPrim const *proto;
        if (_GetProtoPrimForChild(
                instancerPrim, instancerCachePath, &proto, &ctx)) {
             return proto->adapter->GetInstanceIndices(
                    _GetProtoUsdPrim(*proto), instancerCachePath,
                            prototypeCachePath, time);
        }
    }

    if (_InstancerData const* instrData =
                TfMapLookupPtr(_instancerData, instancerCachePath)) {

        // need to find the prototypeRootPath for this prototypeCachePath
        const auto protoPrimIt =
                instrData->protoPrimMap.find(prototypeCachePath);
        if (protoPrimIt != instrData->protoPrimMap.end()) {
            const SdfPath & prototypeRootPath =
                    protoPrimIt->second.protoRootPath;

            // find index of prototypeRootPath within expected array-of-arrays
            const auto pathIndexIt =
                    instrData->prototypePathIndices.find(prototypeRootPath);
            if (pathIndexIt != instrData->prototypePathIndices.end()) {
                size_t pathIndex = (*pathIndexIt).second;

                UsdPrim instancerPrim = _GetPrim(
                        instancerCachePath.GetPrimPath());
                VtArray<VtIntArray> indices = GetPerPrototypeIndices(
                        instancerPrim, time);

                if (pathIndex >= indices.size()) {
                    TF_WARN("ProtoIndex %lu out of bounds "
                            "(prototypes size = %lu) for (%s, %s)",
                                    pathIndex,
                                    indices.size(),
                                    instancerCachePath.GetText(),
                                    prototypeCachePath.GetText());
                    
                    return VtValue();
                }
                return VtValue(indices[pathIndex]);
            }
        }

        TF_WARN("No matching ProtoRootPath found for (%s, %s)",
                instancerCachePath.GetText(), prototypeCachePath.GetText());
    }

    return VtValue();
}

/*virtual*/
std::string 
UsdImagingPointInstancerAdapter::GetExtComputationKernel(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext ctx;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &ctx)) {

        return proto->adapter->GetExtComputationKernel(
                _GetProtoUsdPrim(*proto), cachePath, &ctx);
    }
    return BaseAdapter::GetExtComputationKernel(usdPrim, cachePath, nullptr);
}


/*virtual*/
bool
UsdImagingPointInstancerAdapter::PopulateSelection(
    HdSelection::HighlightMode const& highlightMode,
    SdfPath const &cachePath,
    UsdPrim const &usdPrim,
    int const hydraInstanceIndex,
    VtIntArray const &parentInstanceIndices,
    HdSelectionSharedPtr const &result) const
{
    HD_TRACE_FUNCTION();

    if (IsChildPath(cachePath)) {
        SdfPath instancerPath = cachePath.GetParentPath();
        _ProtoPrim const& proto = _GetProtoPrim(instancerPath, cachePath);
        if (!proto.adapter) {
            return false;
        }

        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "PopulateSelection: proto = %s pi = %s\n",
            cachePath.GetText(), instancerPath.GetText());

        // Make sure one of the prototype paths is a suffix of "usdPrim".
        // "paths" has the location of the populated proto; for native
        // instancing, it will have N-1 paths to instances and
        // 1 path to the prototype.
        // If there's no native instancing, paths will have size 1.
        // If "usdPrim" is a parent of any of these paths, that counts
        // as a selection of this prototype.  e.g.
        // - /World/Instancer/protos (-> /__Prototype_1)
        // - /__Prototype_1/trees/tree_1 (a gprim)
        bool foundPrefix = false;
        SdfPath usdPath = usdPrim.GetPath();
        for (auto const& path : proto.paths) {
            if (path.HasPrefix(usdPath)) {
                foundPrefix = true;
                break;
            }
        }
        if (!foundPrefix) {
            return false;
        }

        // Compose instance indices, if we don't have an explicit index.
        // UsdImaging only adds instance indices in the case of usd instances,
        // so if we don't have an explicit index we want to add all PI
        // instances of the given prototype.
        VtIntArray instanceIndices;
        if (hydraInstanceIndex == -1 && parentInstanceIndices.size() != 0) {
            _InstancerData const* instrData =
                TfMapLookupPtr(_instancerData, instancerPath);
            if (instrData == nullptr) {
                return false;
            }
            
            // XXX: Using _GetTimeWithOffset here is a bit of a hack?
            VtValue indicesValue = GetInstanceIndices(_GetPrim(instancerPath),
                    instancerPath, cachePath, _GetTimeWithOffset(0.0));
            if (!indicesValue.IsHolding<VtIntArray>()) {
                return false;
            }

            VtIntArray const & indices =
                    indicesValue.UncheckedGet<VtIntArray>();

            for (const int pi : parentInstanceIndices) {
                for (size_t i = 0; i < indices.size(); ++i) {
                    instanceIndices.push_back(pi * indices.size() + i);
                }
            }
        }

        // We want the path comparison in PopulateSelection to always succeed
        // (since we've verified the path above), so take the prim at
        // cachePath.GetPrimPath, since that's guaranteed to exist and be a
        // prefix...
        UsdPrim prefixPrim = _GetPrim(cachePath.GetAbsoluteRootOrPrimPath());
        return proto.adapter->PopulateSelection(
            highlightMode, cachePath, prefixPrim,
            hydraInstanceIndex, instanceIndices, result);
    } else {
        _InstancerData const* instrData =
            TfMapLookupPtr(_instancerData, cachePath);
        if (instrData == nullptr) {
            return false;
        }

        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "PopulateSelection: pi = %s\n", cachePath.GetText());

        // For non-gprim selections, selectionPath might be pointing to a
        // point instancer, or it might be pointing to a usd native instance
        // (which is a dependency of the PI, used for calculating prototype
        // transform).  If there's native instancing involved, we need to
        // break the selection path down to a path context, and zipper compare
        // it against the proto paths of each proto prim.  This is a very
        // similar implementation to the one in instanceAdapter.cpp...
        std::deque<SdfPath> selectionPathVec;
        UsdPrim p = usdPrim;
        while (p.IsInstanceProxy()) {
            selectionPathVec.push_front(p.GetPrimInPrototype().GetPath());
            do {
                p = p.GetParent();
            } while (!p.IsInstance());
        }
        selectionPathVec.push_front(p.GetPath());

        // If "cachePath" and "usdPrim" are equal, and hydraInstanceIndex
        // has a value, we're responding to "AddSelected(/World/PI, N)";
        // we can treat it as an instance index for this PI, rather than
        // treating it as an absolute instance index for an rprim.
        // (/World/PI, -1) still corresponds to select-all-instances.
        if (usdPrim.GetPath() == cachePath.GetAbsoluteRootOrPrimPath() &&
            hydraInstanceIndex != -1) {
            // "N" here refers to the instance index in the protoIndices array,
            // which may be different than the actual hydra index, so we need
            // to find the correct prototype/instance pair.

            bool added = false;
            for (auto const& pair : instrData->protoPrimMap) {
                VtValue indicesValue =  GetInstanceIndices(_GetPrim(cachePath),
                        cachePath, pair.first, _GetTimeWithOffset(0.0));

                if (!indicesValue.IsHolding<VtIntArray>()) {
                    continue;
                }
                VtIntArray const & indices =
                        indicesValue.UncheckedGet<VtIntArray>();

                int foundIndex = -1;
                for (size_t i = 0; i < indices.size(); ++i) {
                    if (indices[i] == hydraInstanceIndex) {
                        foundIndex = int(i);
                        break;
                    }
                }
                if (foundIndex == -1) {
                    continue;
                }
                VtIntArray instanceIndices;
                if (parentInstanceIndices.size() > 0) {
                    for (const int pi : parentInstanceIndices) {
                        instanceIndices.push_back(pi * indices.size() +
                            foundIndex);
                    }
                } else {
                    instanceIndices.push_back(foundIndex);
                }
                UsdPrim selectionPrim =
                    _GetPrim(pair.first.GetAbsoluteRootOrPrimPath());

                added |= pair.second.adapter->PopulateSelection(
                    highlightMode, pair.first, selectionPrim,
                    -1, instanceIndices, result);
            }
            return added;
        }

        bool added = false;
        for (auto const& pair : instrData->protoPrimMap) {

            // Zipper compare the instance paths and the selection paths.
            size_t instanceCount, selectionCount;
            for (instanceCount = 0, selectionCount = 0;
                 instanceCount < pair.second.paths.size() &&
                 selectionCount < selectionPathVec.size();
                 ++instanceCount) {
                // pair.second.paths is innermost-first, and selectionPathVec
                // outermost-first, so we need to flip the paths index.
                size_t instanceIdx =
                    pair.second.paths.size() - instanceCount - 1;
                if (pair.second.paths[instanceIdx].HasPrefix(
                        selectionPathVec[selectionCount])) {
                    ++selectionCount;
                } else if (selectionPathVec[selectionCount].HasPrefix(
                            pair.second.paths[instanceIdx])) {
                    // If the selection path is a suffix of the prototype path,
                    // leave the rest of the selection path to be used as
                    // a selection prim for the child adapter.
                    ++instanceCount;
                    break;
                } else if (selectionCount != 0) {
                    // The paths don't match; setting "selectionCount = 0"
                    // is telling the below code "no match", which is an
                    // ok proxy for "partial match, partial mismatch".
                    selectionCount = 0;
                    break;
                }
            }

            UsdPrim selectionPrim;
            if (selectionCount == selectionPathVec.size()) {
                // If we've accounted for the whole selection path, fully
                // populate this prototype.
                selectionPrim =
                    _GetPrim(pair.first.GetAbsoluteRootOrPrimPath());
            }
            else if (selectionCount != 0 &&
                     instanceCount == pair.second.paths.size()) {
                // If the selection path goes past the end of the instance path,
                // compose the remainder of the selection path into a
                // (possibly instance proxy) usd prim and use that as the
                // selection prim.
                SdfPathVector residualPathVec(
                    selectionPathVec.rbegin(),
                    selectionPathVec.rend() - selectionCount);
                SdfPath residualPath =
                    _GetPrimPathFromInstancerChain(residualPathVec);
                selectionPrim = _GetPrim(residualPath);
            } else {
                continue;
            }

            // Compose instance indices.
            VtIntArray instanceIndices;

            VtValue indicesValue = 
                    GetInstanceIndices(_GetPrim(cachePath), cachePath,
                            pair.first, _GetTimeWithOffset(0.0));

            if (!indicesValue.IsHolding<VtIntArray>()) {
                continue;
            }
            VtIntArray const & indices =
                    indicesValue.UncheckedGet<VtIntArray>();


            if (parentInstanceIndices.size() > 0) {
                for (const int pi : parentInstanceIndices) {
                    for (size_t i = 0; i < indices.size(); ++i) {
                        instanceIndices.push_back(pi * indices.size() + i);
                    }
                }
            } else {
                for (size_t i = 0; i < indices.size(); ++i) {
                    instanceIndices.push_back(i);
                }
            }

            added |= pair.second.adapter->PopulateSelection(
                highlightMode, pair.first, selectionPrim,
                hydraInstanceIndex, instanceIndices, result);
        }

        return added;
    }

    return false;
}

/*virtual*/
HdVolumeFieldDescriptorVector
UsdImagingPointInstancerAdapter::GetVolumeFieldDescriptors(
    UsdPrim const& usdPrim,
    SdfPath const &id,
    UsdTimeCode time) const
{
    if (IsChildPath(id)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(), id);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->GetVolumeFieldDescriptors(
            protoPrim, id, time);
    } else {
        return UsdImagingPrimAdapter::GetVolumeFieldDescriptors(
            usdPrim, id, time);
    }
}

/*virtual*/
void
UsdImagingPointInstancerAdapter::_RemovePrim(SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("Should use overidden ProcessPrimResync/ProcessPrimRemoval");
}

/*virtual*/
GfMatrix4d
UsdImagingPointInstancerAdapter::GetRelativeInstancerTransform(
    SdfPath const &parentInstancerCachePath, 
    SdfPath const &cachePath,
    UsdTimeCode time) const
{
    GfMatrix4d transformRoot(1); // target to world.

    // XXX: isProtoRoot detection shouldn't be needed since UsdGeomPointInstaner
    // doesn't have a convention of ignoring protoRoot transform unlike the ones
    // in PxUsdGeomGL.
    // 2 test cases in testUsdImagingGLPointInstancer
    //   pi_pi_usda, time=1 and 2
    // are wrongly configured, and we need to be updated together when fixing.
    //
    bool isProtoRoot = false;
    UsdPrim prim = _GetPrim(cachePath.GetPrimPath());
    bool inPrototype = prim.IsInPrototype();

    if (!parentInstancerCachePath.IsEmpty()) {
        // this instancer has a parent instancer. see if this instancer 
        // is a protoRoot or not.
        _ProtoPrim const& proto
            = _GetProtoPrim(parentInstancerCachePath, cachePath);
        if (proto.protoRootPath == cachePath) {
            // this instancer is a proto root.
            isProtoRoot = true;
        } else {
            // this means instancer(cachePath) is a member of a
            // prototype of the parent instacer, but not a proto root.
            //
            // we need to extract relative transform to root.
            //
            if (inPrototype) {
                // if the instancer is in prototype, set the target
                // root transform to world, since the parent
                // instancer (if the parent is also in prototype,
                // native instancer which instances that parent) 
                // has delegate's root transform.
                transformRoot = GetRootTransform();
            } else {
                // set the target root to proto root.
                transformRoot
                    = BaseAdapter::GetTransform(
                        _GetPrim(proto.protoRootPath),
                        proto.protoRootPath,
                        time);
            }
        }
    }

    if (isProtoRoot) {
        // instancer is a protoroot of parent instancer.
        // ignore instancer transform.
        return GfMatrix4d(1);
    } else {
        // set protoRoot-to-instancer relative transform

        // note that GetTransform() includes GetRootTransform().
        //   GetTransform(prim) : InstancerXfm * RootTransform
        //
        // 1. If the instancer doesn't have a parent,
        //    transformRoot is identity.
        //
        //    val = InstancerXfm * RootTransform * 1^-1
        //        = InstancerXfm * RootTransform
        //
        // 2. If the instancer has a parent and in prototype,
        //    transformRoot is RootTransform.
        //
        //    val = InstancerXfm * RootTransform * (RootTransform)^-1
        //        = InstancerXfm
        //
        // 3. If the instaner has a parent but not in prototype,
        //    transformRoot is (ProtoRoot * RootTransform).
        //
        //    val = InstancerXfm * RootTransform * (ProtoRoot * RootTransform)^-1
        //        = InstancerXfm * (ProtoRoot)^-1
        //
        // in case 2 and 3, RootTransform will be applied on the parent
        // instancer.
        //
        return BaseAdapter::GetTransform(prim, prim.GetPath(), time) * 
            transformRoot.GetInverse();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

