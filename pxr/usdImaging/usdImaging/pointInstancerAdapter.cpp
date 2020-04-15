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
    instrData.visible = true;
    instrData.variableVisibility = true;
    instrData.parentInstancerCachePath = parentInstancerCachePath;
    instrData.visibleTime = std::numeric_limits<double>::infinity();

    // Store the reverse mapping from prototype paths back to this new
    // instancer that uses the prototype.
    for (auto usdProtoPath : usdProtoPaths) {
        _PrototypeData& protoData = _prototypeData[usdProtoPath];
        protoData.instancerCachePaths.insert(instancerCachePath);
    }

    // Store the reverse mapping from the parent instancer path back to
    // this new instancer.
    if (!parentInstancerCachePath.IsEmpty()) {
        _ParentInstancerData& parentData =
            _parentInstancerData[parentInstancerCachePath];
        parentData.childCachePaths.insert(instancerCachePath);
    }

    TF_DEBUG(USDIMAGING_INSTANCER)
        .Msg("[Add PI] %s, parentInstancerCachePath <%s>\n",
             instancerCachePath.GetText(), parentInstancerCachePath.GetText());

    // Need to use GetAbsoluteRootOrPrimPath() on instancerCachePath to drop
    // {instance=X} from the path, so usd can find the prim.
    index->InsertInstancer(
        instancerCachePath,
        parentInstancerCachePath,
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
        instrData.prototypePaths[protoIndex] = usdProtoPaths[protoIndex];
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
        _PopulatePrototype(protoIndex, instancerCachePath,
                instrData, protoRootPrim, index, &ctx);
    }

    // Make sure we populate instancer data to the value cache the first time
    // through UpdateForTime.
    index->MarkInstancerDirty(instancerCachePath,
        HdChangeTracker::DirtyTransform |
        HdChangeTracker::DirtyPrimvar |
        HdChangeTracker::DirtyInstanceIndex);

    return instancerCachePath;
}

void
UsdImagingPointInstancerAdapter::_PopulatePrototype(
    int protoIndex,
    SdfPath const& instancerCachePath,
    _InstancerData& instrData,
    UsdPrim const& protoRootPrim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const *instancerContext)
{
    int protoID = 0;
    size_t primCount = 0;
    size_t instantiatedPrimCount = 0;

    std::vector<UsdPrimRange> treeStack;
    treeStack.push_back(UsdPrimRange(protoRootPrim, _GetDisplayPredicate()));
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
            UsdPrim master = iter->GetMaster();
            UsdPrimRange masterRange(master, _GetDisplayPredicate());
            treeStack.push_back(masterRange);

            // Make sure to register a dependency on this instancer with the
            // parent PI.
            index->AddDependency(instancerContext->instancerCachePath, *iter);
            continue;
        }

        // construct instance chain
        // note: paths is stored in the backward of treeStack
        //       (master, master, ... , instance path)
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
        //    /__Master__1/cube
        //    /__Master__2/ProtoCube
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
        // via CanPopulateMaster().
        if (instanceProxyPrim.IsInstance() && adapter &&
            !adapter->CanPopulateMaster()) {
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
                if (iter->IsMaster() && TF_VERIFY(instancerChain.size() > 1)) {
                    populatePrim = _GetPrim(instancerChain.at(1));
                }

                SdfPath const& materialId = GetMaterialUsdPath(populatePrim);
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

            _PrototypeData& protoData = _prototypeData[protoPath];
            protoData.instancerCachePaths.insert(instancerCachePath);

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
            // the populated prototype over all time.
            // protoPrim may be across an instance boundary from protoRootPrim,
            // so compute visibility for each master subtree, and then for the
            // final path relative to the proto root.
            UsdPrim protoRootPrim = _GetPrim(proto.protoRootPath);
            for (size_t i = 0; i < proto.paths.size()-1; ++i) {
                _ComputeProtoVisibility(_GetPrim(proto.paths[i+1]).GetMaster(),
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
                    false);

        if (!(*timeVaryingBits & HdChangeTracker::DirtyPrimvar)) {
            UsdGeomPrimvarsAPI primvars(prim);
            for (auto const &pv: primvars.GetPrimvarsWithValues()) {
                TfToken const& interp = pv.GetInterpolation();
                if (interp != UsdGeomTokens->constant &&
                    interp != UsdGeomTokens->uniform &&
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
    UsdImagingValueCache* valueCache = _GetValueCache();
    if (IsChildPath(cachePath)) {
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

        // Never pull visibility directly from the prototype, since we will
        // need to compute visibility relative to the model root anyway.
        int protoReqBits = requestedBits 
            & ~HdChangeTracker::DirtyVisibility;

        // Allow the prototype's adapter to update, if there's anything left
        // to do.
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        if (protoReqBits != HdChangeTracker::Clean) {
            proto.adapter->UpdateForTime(protoPrim,
                    cachePath, time, protoReqBits);
        }

        if (requestedBits & HdChangeTracker::DirtyVisibility) {
            // Apply the instancer visibility at the current time to the
            // instance. Notice that the instance will also pickup the instancer
            // visibility at the time offset.
            bool& vis = valueCache->GetVisible(cachePath);
            bool protoHasFixedVis = !(proto.variabilityBits
                    & HdChangeTracker::DirtyVisibility);

            _InstancerDataMap::const_iterator it
                = _instancerData.find(instancerPath);
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
                        _GetPrim(proto.paths[i+1]).GetMaster(),
                        _GetPrim(proto.paths[i+0]),
                        time, &vis);
                }
                _ComputeProtoVisibility(
                    _GetPrim(proto.protoRootPath),
                    _GetPrim(proto.paths.back()),
                    time, &vis);
            }
        }

        if (requestedBits & HdChangeTracker::DirtyTransform) {
            // If the prototype we're processing is a master, _GetProtoUsdPrim
            // will return us the instance for attribute lookup; but the
            // instance transform for that instance is already accounted for in
            // _CorrectTransform.  Masters don't have any transform aside from
            // the root transform, so override the result of UpdateForTime.
            if (protoPrim.IsInstance()) {
                _GetValueCache()->GetTransform(cachePath) = GetRootTransform();
            }

            // Correct the transform for various shenanigans: NI transforms,
            // delegate root transform, proto root transform.
            _CorrectTransform(prim, _GetPrim(proto.protoRootPath),
                              cachePath, proto.paths, time);
        }
    } else  if (_InstancerData const* instrData =
                TfMapLookupPtr(_instancerData, cachePath)) {

        // On DirtyInstanceIndex, recompute the per-prototype index map.
        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            _InstanceMap instanceMap =
                _ComputeInstanceMap(cachePath, *instrData, time);

            // XXX: See UsdImagingDelegate::GetInstanceIndices;
            // the change-tracking is on the instancer prim, but for simplicity
            // we store each prototype's index buffer in that prototype's value
            // cache (since each prototype can have only one instancer).
            for (auto const& pair : instrData->protoPrimMap) {
                valueCache->GetInstanceIndices(pair.first) =
                    instanceMap[pair.second.protoRootPath];
            }
        }

        // For the instancer itself, we only send translate, rotate and scale
        // back as primvars, which all fall into the DirtyPrimvar bucket
        // currently.
        if (requestedBits & HdChangeTracker::DirtyPrimvar) {
            UsdGeomPointInstancer instancer(prim);

            // PERFORMANCE: It would be nice to track variability of individual
            // primvars separately, since uniform values will  needlessly be
            // sent to the GPU on every frame.
            VtVec3fArray positions;
            if (instancer.GetPositionsAttr().Get(&positions, time)) {
                valueCache->GetPrimvar(cachePath, _tokens->translate) = 
                                                                    positions;
                _MergePrimvar(
                    &valueCache->GetPrimvars(cachePath),
                    _tokens->translate,
                    HdInterpolationInstance,
                    HdPrimvarRoleTokens->vector);
            }

            VtQuathArray orientations;
            if (instancer.GetOrientationsAttr().Get(&orientations, time)) {
                // convert to Vec4Array that hydra instancer requires.
                // Also note that hydra's instancer takes GfQuaterion layout
                // (real, imaginary) which differs from GfQuath's
                // (imaginary, real)
                VtVec4fArray rotations;
                rotations.reserve(orientations.size());
                for (const GfQuath& orientation : orientations) {
                    rotations.push_back(
                        GfVec4f(orientation.GetReal(),
                                orientation.GetImaginary()[0],
                                orientation.GetImaginary()[1],
                                orientation.GetImaginary()[2]));
                }

                valueCache->GetPrimvar(cachePath, _tokens->rotate) = rotations;
                _MergePrimvar(
                    &valueCache->GetPrimvars(cachePath),
                    _tokens->rotate,
                    HdInterpolationInstance);
            }

            VtVec3fArray scales;
            if (instancer.GetScalesAttr().Get(&scales, time)) {
                valueCache->GetPrimvar(cachePath, _tokens->scale) = scales;
                _MergePrimvar(
                    &valueCache->GetPrimvars(cachePath),
                    _tokens->scale,
                    HdInterpolationInstance);
            }

            // Convert non-constant primvars on UsdGeomPointInstancer
            // into instance-rate primvars. Note: this only gets local primvars.
            // Inherited primvars don't vary per-instance, so we let the
            // prototypes pick them up.
            UsdGeomPrimvarsAPI primvars(instancer);
            for (auto const &pv: primvars.GetPrimvarsWithValues()) {
                TfToken const& interp = pv.GetInterpolation();
                if (interp != UsdGeomTokens->constant &&
                    interp != UsdGeomTokens->uniform) {
                    HdInterpolation interp = HdInterpolationInstance;
                    _ComputeAndMergePrimvar(
                        prim, cachePath, pv, time, valueCache, &interp);
                }
            }
        }

        // update instancer transform.
        if (requestedBits & HdChangeTracker::DirtyTransform) {
            SdfPath parentInstancerCachePath =
                instrData->parentInstancerCachePath;
            if (!parentInstancerCachePath.IsEmpty()) {
                // if nested, double transformation should be avoided.
                SdfPath parentInstancerUsdPath =
                    parentInstancerCachePath.GetAbsoluteRootOrPrimPath();
                UsdPrim parentInstancerUsdPrim =
                    _GetPrim(parentInstancerUsdPath);
                UsdImagingPrimAdapterSharedPtr adapter =
                    _GetPrimAdapter(parentInstancerUsdPrim);

                // parentInstancer doesn't necessarily be UsdGeomPointInstancer.
                // lookup and delegate adapter to compute the instancer 
                // transform.
                _GetValueCache()->GetInstancerTransform(cachePath) =
                    adapter->GetRelativeInstancerTransform(
                        parentInstancerCachePath, cachePath, time);
            } else {
                // if not nested, simply put the transform of the instancer.
                _GetValueCache()->GetInstancerTransform(cachePath) =
                    this->GetRelativeInstancerTransform(
                        parentInstancerCachePath, cachePath, time);
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
            // return AllDirty.
            return HdChangeTracker::AllDirty;
        }

        // XXX: Specifically disallow visibility and transform updates: in
        // these cases, it's hard to tell which prims we should dirty but
        // probably we need to dirty both prototype & instancer. This is a
        // project for later. In the meantime, returning AllDirty causes
        // a re-sync.
        HdDirtyBits dirtyBits = proto.adapter->ProcessPropertyChange(
            _GetProtoUsdPrim(proto), cachePath, propertyName);

        if (dirtyBits & (HdChangeTracker::DirtyTransform |
                         HdChangeTracker::DirtyVisibility)) {
            return HdChangeTracker::AllDirty;
        }
        return dirtyBits;
    }

    if (propertyName == UsdGeomTokens->positions ||
        propertyName == UsdGeomTokens->orientations ||
        propertyName == UsdGeomTokens->scales) {

        TfToken primvarName = propertyName;
        if (propertyName == UsdGeomTokens->positions) {
            primvarName = _tokens->translate;
        } else if (propertyName == UsdGeomTokens->orientations) {
            primvarName = _tokens->rotate;
        } else if (propertyName == UsdGeomTokens->scales) {
            primvarName = _tokens->scale;
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
    if (UsdImagingPrimAdapter::_HasPrimvarsPrefix(propertyName)) {
        // Ignore local constant/uniform primvars.
        UsdGeomPrimvar pv = UsdGeomPrimvarsAPI(prim).GetPrimvar(propertyName);
        if (pv && (pv.GetInterpolation() == UsdGeomTokens->constant ||
                   pv.GetInterpolation() == UsdGeomTokens->uniform)) {
            return HdChangeTracker::Clean;
        }

        
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName,
            /*valueChangeDirtyBit*/HdChangeTracker::DirtyPrimvar,
            /*inherited=*/false);
    }

    // XXX: Treat transform & visibility changes as re-sync, until we untangle
    // instancer vs proto data.
    return HdChangeTracker::AllDirty;
}

void
UsdImagingPointInstancerAdapter::_ProcessPrimRemoval(SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index,
                                             SdfPathVector* instancersToReload)
{
    // If prim data exists at this path, we'll drop it now.
    _InstancerDataMap::iterator instIt = _instancerData.find(cachePath);
    SdfPathVector instancersToUnload;

    if (instIt != _instancerData.end()) {
        while (instIt != _instancerData.end()) {
            SdfPath parentInstancerCachePath =
                instIt->second.parentInstancerCachePath;
            instancersToUnload.push_back(instIt->first);

            // Setup the next iteration.
            if (parentInstancerCachePath.IsEmpty()) {
                break;
            }

            // Note that the parent may be owned by a different adapter, so we
            // might not find it here.
            instIt = _instancerData.find(parentInstancerCachePath);
        }
    } else {
        if (!IsChildPath(cachePath)) {
            // This is a path that is neither an instancer or a child path,
            // which means it was only tracked for change processing at an
            // instance root.
            return;
        }
    }

    // Otherwise, the cachePath must be a path to one of the prototype rprims.

    // The prim in the Usd scenegraph could be shared among many instancers, so
    // we search each instancer for the presence of the given cachePath. Any
    // instancer that references this prim must be rebuilt, we don't currently
    // support incrementally rebuilding an instancer.

    if (instancersToUnload.empty()) {
        // Check the reverse lookup of prototypes to instancers. This will
        // catch any instancers that have this prototype or any ancestor of
        // this prototype in its prototypePaths or protoPrimMap.
        SdfPath cachePathAncestor = cachePath;
        while (!cachePathAncestor.IsAbsoluteRootPath()) {
            _PrototypeDataMap::iterator protoIt =
                _prototypeData.find(cachePathAncestor);
            if (protoIt != _prototypeData.end()) {
                for (auto instancerPath : protoIt->second.instancerCachePaths) {
                    instancersToUnload.push_back(instancerPath);
                }
            }
            cachePathAncestor = cachePathAncestor.GetParentPath();
        }

        // Check if the cache path indicates a parent instancer.
        _ParentInstancerDataMap::iterator parentIt =
            _parentInstancerData.find(cachePath);
        if (parentIt != _parentInstancerData.end()) {
            for (auto instancerPath : parentIt->second.childCachePaths) {
                instancersToUnload.push_back(instancerPath);
            }
        }
    }

    // Propagate changes from the parent instancers down to the children.
    SdfPathVector moreToUnload;
    TF_FOR_ALL(i, instancersToUnload) {
        _ParentInstancerDataMap::iterator parentIt =
            _parentInstancerData.find(cachePath);
        if (parentIt != _parentInstancerData.end()) {
            for (auto instancerPath : parentIt->second.childCachePaths) {
                moreToUnload.push_back(instancerPath);
            }
        }
    }
    instancersToUnload.insert(instancersToUnload.end(), moreToUnload.begin(),
            moreToUnload.end());

    if (instancersToReload) {
        instancersToReload->reserve(instancersToUnload.size());
    }

    TF_FOR_ALL(i, instancersToUnload) {
        _InstancerDataMap::iterator instIt = _instancerData.find(*i);
        // we expect duplicated instancer entries in instacersToUnload.
        // continue if it's already removed.
        if (instIt == _instancerData.end()) continue;
        SdfPath parentInstancerCachePath =
            instIt->second.parentInstancerCachePath;

        _UnloadInstancer(*i, index);

        // If the caller doesn't need to know what to reload, we're done in this
        // loop.
        if (!instancersToReload) {
            continue;
        }

        // Never repopulate child instancers directly, they are only repopulated
        // by populating the parent.
        if (!parentInstancerCachePath.IsEmpty()) {
            continue;
        }

        // It's an error to request an invalid prim to be Repopulated, so be
        // sure the prim still exists before requesting Repopulation.
        if (UsdPrim p = _GetPrim(*i)) {
            if (p.IsActive()) {
                instancersToReload->push_back(*i);
            }
        }
    }
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

    // Remove the reverse mapping from the prototype paths to the instancer.
    for (auto usdProtoPath : instIt->second.prototypePaths) {
        _PrototypeDataMap::iterator protoIt = _prototypeData.find(usdProtoPath);
        if (protoIt != _prototypeData.end()) {
            protoIt->second.instancerCachePaths.erase(instancerPath);
            if (protoIt->second.instancerCachePaths.empty()) {
                _prototypeData.erase(protoIt);
            }
        }
    }

    // Remove the reverse mapping from the parent instancer to the instancer.
    if (!instIt->second.parentInstancerCachePath.IsEmpty()) {
        _ParentInstancerDataMap::iterator parentIt =
            _parentInstancerData.find(instIt->second.parentInstancerCachePath);
        if (parentIt != _parentInstancerData.end()) {
            parentIt->second.childCachePaths.erase(instancerPath);
            if (parentIt->second.childCachePaths.empty()) {
                _parentInstancerData.erase(parentIt);
            }
        }
    }

    // XXX: There's a nasty catch-22 where PI's ProcessPrimRemoval tries to
    // remove that point instancer as well as any parents (since we don't have
    // good invalidation for a parent PI when a child PI is removed/resynced,
    // we resync the whole tree); and _UnloadInstancer tries to remove
    // children.  This would cause an infinite loop, except that calling
    // ProcessPrimRemoval on a child a second time is a no-op.  However,
    // if a parent PI has multiple child PIs, the parent PI will be removed
    // several times (usually resulting in a segfault).
    //
    // To guard against that, we remove instancerPath from _instancerData
    // before traversing children, so that the parent PI is only removed once.
    const _ProtoPrimMap protoPrimMap = instIt->second.protoPrimMap;
    _instancerData.erase(instIt);

    // First, we need to make sure all proto rprims are removed.
    TF_FOR_ALL(protoPrimIt, protoPrimMap) {
        SdfPath const& cachePath = protoPrimIt->first;
        _ProtoPrim const& proto = protoPrimIt->second;
        _PrototypeDataMap::iterator protoIt = _prototypeData.find(cachePath);

        // Remove the proto prim cache path from the reverse map of
        // prototypes paths to instancer paths as well.
        if (protoIt != _prototypeData.end()) {
            protoIt->second.instancerCachePaths.erase(instancerPath);
            if (protoIt->second.instancerCachePaths.empty()) {
                _prototypeData.erase(protoIt);
            }
        }
        proto.adapter->ProcessPrimRemoval(cachePath, index);
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

const UsdPrim
UsdImagingPointInstancerAdapter::_GetProtoUsdPrim(
    _ProtoPrim const& proto) const
{
    // proto.paths.front() is the most local path for the rprim.
    // If it's not native-instanced, proto.paths will be size 1.
    // If it is native-instanced, proto.paths may look like
    //   /__Master_1/prim
    //   /Instance
    // where /__Master_1/prim is the pointer to the actual prim in question.
    UsdPrim prim = _GetPrim(proto.paths.front());

    // One exception: if the prototype is an instance, proto.paths looks like
    //   /__Master_1
    //   /Instance
    // ... in which case, we want to return /Instance since masters drop all
    // attributes.
    if (prim.IsMaster() && TF_VERIFY(proto.paths.size() > 1)) {
        prim = _GetPrim(proto.paths.at(1));
    }
    return prim;
}

bool
UsdImagingPointInstancerAdapter::_GetInstancerVisible(
    SdfPath const &instancerPath, UsdTimeCode time) const
{
    bool visible = UsdImagingPrimAdapter::GetVisible(
        _GetPrim(instancerPath.GetPrimPath()), time);

    if (visible) {
        _InstancerDataMap::const_iterator it
            = _instancerData.find(instancerPath);
        if (it != _instancerData.end()) {
            // note that parent instancer may not be a namespace parent
            // (e.g. master -> instance)
            SdfPath const &parentInstancerCachePath =
                it->second.parentInstancerCachePath;
            if (!parentInstancerCachePath.IsEmpty()) {
                return _GetInstancerVisible(parentInstancerCachePath, time);
            }
        }
    }

    return visible;
}

UsdImagingPointInstancerAdapter::_InstanceMap
UsdImagingPointInstancerAdapter::_ComputeInstanceMap(
                    SdfPath const& instancerPath,
                    _InstancerData const& instrData,
                    UsdTimeCode time) const
{
    UsdPrim instancerPrim = _GetPrim(instancerPath.GetPrimPath());

    TF_DEBUG(USDIMAGING_INSTANCER).Msg(
        "[PointInstancer::_ComputeInstanceMap] %s\n",
        instancerPath.GetText());

    UsdGeomPointInstancer instancer(instancerPrim);
    if (!instancer) {
        TF_WARN("Instancer prim <%s> is not a valid PointInstancer\n",
                instancerPath.GetText());
        return _InstanceMap();
    }

    UsdAttribute indicesAttr = instancer.GetProtoIndicesAttr();
    VtIntArray indices;

    if (!indicesAttr.Get(&indices, time)) {
        TF_RUNTIME_ERROR("Failed to read point cloud indices");
        return _InstanceMap();
    }

    // Initialize all of the indices to empty.
    _InstanceMap instanceMap;
    for (SdfPath const& proto : instrData.prototypePaths) {
        instanceMap[proto] = VtIntArray();
    }

    // Fetch the "mask", a bit array of enabled/disabled state per instance.
    // If no value is available, mask will be ignored below.
    std::vector<bool> mask = instancer.ComputeMaskAtTime(time);

    for (size_t instanceId = 0; instanceId < indices.size(); ++instanceId) {
        size_t protoIndex = indices[instanceId];

        if (protoIndex >= instrData.prototypePaths.size()) {
            TF_WARN("Invalid index (%lu) found in <%s.%s> for time (%s)\n",
                    protoIndex, instancer.GetPath().GetText(), 
                    indicesAttr.GetName().GetText(),
                    TfStringify(time).c_str());
            continue;
        }
        SdfPath const& protoPath = instrData.prototypePaths[protoIndex];

        if (mask.size() == 0 || mask[instanceId]) {
            instanceMap[protoPath].push_back(instanceId);
        }
    }

    TF_DEBUG(USDIMAGING_POINT_INSTANCER_PROTO_CREATED).Msg(
        "[Instancer Updated]: <%s>\n",
        instancerPrim.GetPath().GetText());

    return instanceMap;
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

void
UsdImagingPointInstancerAdapter::_CorrectTransform(UsdPrim const& instancer,
                                                   UsdPrim const& protoRoot,
                                                   SdfPath const& cachePath,
                                                   SdfPathVector const& protoPathChain,
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

    // Our hd convention applies the delegate-root-transform to instancer,
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
    GfMatrix4d& protoGprimToWorld = _GetValueCache()->GetTransform(cachePath);
    protoGprimToWorld = protoGprimToWorld * inverseRootTransform;

    // If this is nested instancer (has parent),
    for (size_t i = 1; i < protoPathChain.size(); ++i) {
        // ignore root transform of nested instancer chain
        //
        // PI ---(protoRoot)--- NI:XFM
        //                          ^
        //                       This matrix, we're applying
        protoGprimToWorld *= GetTransform(_GetPrim(protoPathChain[i]), time,
                                          /*ignoreRootTransform=*/true);
    }

    // Then, we also need to subtract transform above the proto root to avoid
    // double transform of instancer and prototypes.
    // Compute the transform of the proto root, excluding delegate-root-transform.
    //
    // PI(or whatever):XFM---(protoRoot)--- NI (or whatever)
    //                 ^
    //      This matrix, we're subtracting
    UsdPrim parent = protoRoot.GetParent();
    if (parent) {
        GfMatrix4d parentToWorld =
            GetTransform(parent, time, /*ignoreRootTransform=*/true);

        // protoRootToWorld includes its own transform AND root transform,
        // GetInverse() extracts both transforms.
        protoGprimToWorld = protoGprimToWorld * parentToWorld.GetInverse();
    }

    // Instancer transform is computed and stored at the instancer.
    // see UpdateForTime()
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
    // Skip all masters, since they can't have an opinion.
    if (!protoGprim.IsMaster() &&
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
    int instanceIndex) const
{
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
        // XXX: A prim adapter "GetInstancerId()" function would be super
        // useful here.
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

    // If the prim path is in master, we need the help of the parent
    // instancer to figure out what the right instance is.  We assume:
    // 1.) primPath and instancerPath are inside the same master.
    // 2.) recursing gives us the fully-qualified version of instancerPath.
    //
    // If:
    // - primPath is /_Master_1/Instancer/protos/A
    // - instancerPath is /_Master_1/Instancer
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

    // Check if primPath is in master, and if so check if the instancer
    // is in the same master...
    UsdPrim prim = _GetPrim(primPath);
    if (!prim || !prim.IsInMaster()) {
        return primPath;
    }
    UsdPrim instancer = _GetPrim(instancerPath.GetAbsoluteRootOrPrimPath());
    if (!instancer || !instancer.IsInMaster() ||
        prim.GetMaster() != instancer.GetMaster()) {
        TF_CODING_ERROR("primPath <%s> and instancerPath <%s> are not in "
                        "the same master", primPath.GetText(),
                        instancerPath.GetText());
        return SdfPath();
    }

    // Look up the parent instancer of this instancer.
    _InstancerDataMap::const_iterator it =
        _instancerData.find(instancerPath);
    if (it == _instancerData.end()) {
        return SdfPath();
    }
    SdfPath parentPath = it->second.parentInstancerCachePath;

    // Compute the parent instance index.
    _InstanceMap instanceMap = _ComputeInstanceMap(
            cachePath, it->second, _GetTimeWithOffset(0.0));
    VtIntArray const& indices = instanceMap[proto.protoRootPath];
    // instanceIndex = parentIndex * indices.size() + i,
    // so parentIndex = instanceIndex / indices.size().
    int parentIndex = instanceIndex / indices.size();

    // Find out the fully-qualified parent path.
    UsdPrim parentInstancerUsdPrim =
        _GetPrim(parentPath.GetAbsoluteRootOrPrimPath());
    UsdImagingPrimAdapterSharedPtr parentAdapter =
        _GetPrimAdapter(parentInstancerUsdPrim);
    SdfPath fqInstancerPath =
        parentAdapter->GetScenePrimPath(instancerPath, parentIndex);

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
    sampleValues[0] = _GetValueCache()->GetTransform(cachePath);
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
    VtValue *sampleValues)
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
            maxNumSamples, sampleTimes, sampleValues);
    } else {
        // Map Hydra-PI transform keys to their USD equivalents.
        TfToken usdKey = key;
        if (key == _tokens->translate) {
            usdKey = UsdGeomTokens->positions;
        } else if (key == _tokens->scale) {
            usdKey = UsdGeomTokens->scales;
        } else if (key == _tokens->rotate) {
            usdKey = UsdGeomTokens->orientations;
        }
        return UsdImagingPrimAdapter::SamplePrimvar(
            usdPrim, cachePath, usdKey, time,
            maxNumSamples, sampleTimes, sampleValues);
    }
}

PxOsdSubdivTags
UsdImagingPointInstancerAdapter::GetSubdivTags(UsdPrim const& usdPrim,
                                               SdfPath const& cachePath,
                                               UsdTimeCode time) const
{
    if (IsChildPath(cachePath)) {
        // Delegate to prototype adapter and USD prim.
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(),
                                                   cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(proto);
        return proto.adapter->GetSubdivTags(protoPrim, cachePath, time);
    }
    return UsdImagingPrimAdapter::GetSubdivTags(usdPrim, cachePath, time);
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
        // - /World/Instancer/protos (-> /Master_1)
        // - /Master_1/trees/tree_1 (a gprim)
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
            _InstanceMap instanceMap = _ComputeInstanceMap(
                cachePath, *instrData, _GetTimeWithOffset(0.0));
            VtIntArray const& indices = instanceMap[proto.protoRootPath];
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
            selectionPathVec.push_front(p.GetPrimInMaster().GetPath());
            do {
                p = p.GetParent();
            } while (!p.IsInstance());
        }
        selectionPathVec.push_front(p.GetPath());

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

            // Compose instance indices, if we don't have an explicit index.
            VtIntArray instanceIndices;
            if (hydraInstanceIndex == -1 && parentInstanceIndices.size() != 0) {
                _InstanceMap instanceMap = _ComputeInstanceMap(
                    cachePath, *instrData, _GetTimeWithOffset(0.0));
                VtIntArray const& indices =
                    instanceMap[pair.second.protoRootPath];
                for (const int pi : parentInstanceIndices) {
                    for (size_t i = 0; i < indices.size(); ++i) {
                        instanceIndices.push_back(pi * indices.size() + i);
                    }
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
    SdfPath const &parentInstancerCachePath, SdfPath const &cachePath,
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
    bool inMaster = prim.IsInMaster();

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
            if (inMaster) {
                // if the instancer is in master, set the target
                // root transform to world, since the parent
                // instancer (if the parent is also in master,
                // native instancer which instances that parent) 
                // has delegate's root transform.
                transformRoot = GetRootTransform();
            } else {
                // set the target root to proto root.
                transformRoot
                    = GetTransform(
                        _GetPrim(proto.protoRootPath),
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
        // 2. If the instancer has a parent and in master,
        //    transformRoot is RootTransform.
        //
        //    val = InstancerXfm * RootTransform * (RootTransform)^-1
        //        = InstancerXfm
        //
        // 3. If the instaner has a parent but not in master,
        //    transformRoot is (ProtoRoot * RootTransform).
        //
        //    val = InstancerXfm * RootTransform * (ProtoRoot * RootTransform)^-1
        //        = InstancerXfm * (ProtoRoot)^-1
        //
        // in case 2 and 3, RootTransform will be applied on the parent
        // instancer.
        //
        return GetTransform(prim, time) * transformRoot.GetInverse();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

