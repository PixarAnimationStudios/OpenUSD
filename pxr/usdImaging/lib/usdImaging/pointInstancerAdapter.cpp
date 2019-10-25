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
        GetInstancerCachePath(prim, instancerContext);
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
    if (!protosRel.GetForwardedTargets(&usdProtoPaths) 
        || usdProtoPaths.empty()) {
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
    instrData.prototypes.resize(usdProtoPaths.size());
    instrData.visible = true;
    instrData.dirtyBits = HdChangeTracker::AllDirty;
    instrData.parentInstancerCachePath = parentInstancerCachePath;
    instrData.visibleTime = std::numeric_limits<double>::infinity();
    instrData.indicesTime = std::numeric_limits<double>::infinity();

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

    // Make sure we populate instancer data to the value cache the first time
    // through UpdateForTime.
    index->MarkInstancerDirty(instancerCachePath,
        HdChangeTracker::DirtyTransform |
        HdChangeTracker::DirtyPrimvar);
    if (!parentInstancerCachePath.IsEmpty()) {
        index->MarkInstancerDirty(instancerCachePath,
            HdChangeTracker::DirtyInstanceIndex);
    }

    // ---------------------------------------------------------------------- //
    // Main Prototype allocation loop.
    // ---------------------------------------------------------------------- //

    // Iterate over all prototypes to allocate the Rprims in the Hydra
    // RenderIndex.
    size_t prototypeCount = instrData.prototypes.size();

    // For each prototype, allocate the Rprims.
    for (size_t protoIndex = 0; protoIndex < prototypeCount; ++protoIndex) {

        // -------------------------------------------------------------- //
        // Initialize this prototype.
        // -------------------------------------------------------------- //
        _PrototypeSharedPtr &prototype = instrData.prototypes[protoIndex];
        prototype.reset(new _Prototype());
        prototype->enabled = false;        // initialize as disabled.
        prototype->requiresUpdate = true;
        prototype->protoRootPath = usdProtoPaths[protoIndex];
        prototype->indices = VtIntArray(1); // overwritten in _UpdateInstanceMap

        UsdPrim protoRootPrim = _GetPrim(prototype->protoRootPath);
        if (!protoRootPrim) {
            TF_WARN("Targeted prototype was not found <%s>\n",
                    prototype->protoRootPath.GetText());
            continue;
        }

        // -------------------------------------------------------------- //
        // Traverse the subtree and allocate the Rprims
        // -------------------------------------------------------------- //
        UsdImagingInstancerContext ctx = { instancerCachePath,
                                           /*childName=*/TfToken(),
                                           SdfPath(),
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

    _PrototypeSharedPtr &prototype = instrData.prototypes[protoIndex];

    std::vector<UsdPrimRange> treeStack;
    treeStack.push_back(UsdPrimRange(protoRootPrim));
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
            UsdPrimRange masterRange(master);
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
                UsdImagingInstancerContext ctx = {
                    instancerContext->instancerCachePath,
                    /*childName=*/protoName,
                    materialId,
                    drawMode,
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
            instrData.usdToCacheMap[iter->GetPath()].push_back(protoPath);
            _ProtoRprim& rproto = instrData.protoRprimMap[protoPath];
            rproto.adapter = adapter;
            rproto.prototype = prototype;
            rproto.paths = instancerChain;

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
    UsdImagingValueCache* valueCache = _GetValueCache();

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
        _ProtoRprim& rproto =
            const_cast<_ProtoRprim&>(_GetProtoRprim(prim.GetPath(), cachePath));
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(rproto.prototype, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(rproto.paths.size() > 0, "%s", cachePath.GetText())) {
            return;
        }

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

        // XXX: We should never pull purpose directly from the prototype's
        // adapter, since we must compute purpose relative to the model root,
        // however we have no way of communicating that currently.
        UsdPrim protoRootPrim = _GetPrim(rproto.prototype->protoRootPath);
        UsdPrim protoPrim = _GetProtoUsdPrim(rproto);
        rproto.adapter->TrackVariability(protoPrim, cachePath,
                                        &rproto.variabilityBits);
        *timeVaryingBits |= rproto.variabilityBits;

        // XXX: We need to override the purpose computed by the adapter for the
        // same reason noted above.
        valueCache->GetPurpose(cachePath) = UsdGeomTokens->default_;

        // Compute the purpose.
        // protoPrim may be across an instance boundary from protoRootPrim, so
        // compute purpose for each master subtree, and then for the final
        // path relative to the proto root.
        for (size_t i = 0; i < rproto.paths.size()-1;++i) {
            _ComputeProtoPurpose(_GetPrim(rproto.paths[i+1]).GetMaster(),
                                 _GetPrim(rproto.paths[i+0]),
                                 &valueCache->GetPurpose(cachePath));
        }
        _ComputeProtoPurpose(protoRootPrim, _GetPrim(rproto.paths.back()),
                             &valueCache->GetPurpose(cachePath));

        if (!(rproto.variabilityBits & HdChangeTracker::DirtyVisibility)) {
            // Pre-cache visibility, because we now know that it is static for
            // the rprim prototype over all time.
            // protoPrim may be across an instance boundary from protoRootPrim,
            // so compute purpose for each master subtree, and then for the
            // final path relative to the proto root.
            for (size_t i = 0; i < rproto.paths.size()-1; ++i) {
                _ComputeProtoVisibility(_GetPrim(rproto.paths[i+1]).GetMaster(),
                                        _GetPrim(rproto.paths[i+0]),
                                        time, &rproto.visible);
            }
            _ComputeProtoVisibility(protoRootPrim, _GetPrim(rproto.paths.back()),
                                    time, &rproto.visible);
        }

        // If the instancer varies over time, we should flag the DirtyInstancer
        // bits on the Rprim on every frame, to be sure the instancer data
        // associated with the Rprim gets updated.
        int instancerBits = _UpdateDirtyBits(prim);
        *timeVaryingBits |=  (instancerBits & HdChangeTracker::DirtyInstancer);
        _IsVarying(prim,
            UsdGeomTokens->visibility,
            HdChangeTracker::DirtyVisibility,
            UsdImagingTokens->usdVaryingVisibility,
            timeVaryingBits,
            true);

        return;
    } else {
        TfToken purpose = GetPurpose(prim);
        // Empty purpose means there is no opinion, fall back to default.
        if (purpose.IsEmpty())
            purpose = UsdGeomTokens->default_;
        valueCache->GetPurpose(cachePath) = purpose;

        // Check to see if this point instancer is also being instanced, 
        // if so, we need to set dirty bits on the instance index. 
        // For instancers, we could probably update the 
        // instance index only once, since
        // currently subsequent updates are redundant.
        _InstancerDataMap::const_iterator instr =
            _instancerData.find(cachePath);
        if (instr != _instancerData.end()) {
            SdfPath parentInstancerCachePath =
                instr->second.parentInstancerCachePath;
            UsdPrim parentInstancer = _GetPrim(
                    parentInstancerCachePath.GetAbsoluteRootOrPrimPath());
            if (parentInstancer) {
                // Mark instance indices as time varying if any of the following
                // is time varying : protoIndices, invisibleIds
                _IsVarying(parentInstancer,
                    UsdGeomTokens->invisibleIds,
                    HdChangeTracker::DirtyInstanceIndex,
                    _tokens->instancer,
                    timeVaryingBits,
                    false) || _IsVarying(parentInstancer,
                        UsdGeomTokens->protoIndices,
                        HdChangeTracker::DirtyInstanceIndex,
                        _tokens->instancer,
                        timeVaryingBits,
                        false);
            }
        }

        // this is for instancer transform.
        _IsTransformVarying(prim,
                            HdChangeTracker::DirtyTransform,
                            UsdImagingTokens->usdVaryingXform,
                            timeVaryingBits);

        // to update visibility
        _UpdateDirtyBits(prim);

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
            UsdGeomPointInstancer instancer(prim);
            UsdGeomPrimvarsAPI primvars(instancer);
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
        _ProtoRprim const& rproto = _GetProtoRprim(instancerPath, cachePath);
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(rproto.prototype, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(rproto.paths.size() > 0, "%s", cachePath.GetText())) {
            return;
        }

        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            _UpdateInstanceMap(instancerPath, time);
            valueCache->GetInstanceIndices(cachePath) = 
                rproto.prototype->indices;
        }

        // Never pull visibility directly from the prototype, since we will
        // need to compute visibility relative to the model root anyway.
        // Similarly, the InstanceIndex was already updated, if needed.
        int protoReqBits = requestedBits 
            & ~HdChangeTracker::DirtyInstanceIndex
            & ~HdChangeTracker::DirtyVisibility;

        // Allow the prototype's adapter to update, if there's anything left
        // to do.
        UsdPrim protoPrim = _GetProtoUsdPrim(rproto);
        if (protoReqBits != HdChangeTracker::Clean)
            rproto.adapter->UpdateForTime(protoPrim,
                                          cachePath, time, protoReqBits);

        if (requestedBits & HdChangeTracker::DirtyVisibility) {
            // Apply the instancer visibility at the current time to the
            // instance. Notice that the instance will also pickup the instancer
            // visibility at the time offset.
            bool& vis = valueCache->GetVisible(cachePath);
            bool protoHasFixedVis = !(rproto.variabilityBits
                    & HdChangeTracker::DirtyVisibility);

            _UpdateInstancerVisibility(instancerPath, time);

            _InstancerDataMap::const_iterator it
                = _instancerData.find(instancerPath);
            if (TF_VERIFY(it != _instancerData.end())) {
                vis = it->second.visible;
            }
            if (protoHasFixedVis) {
                // The instancer is visible and the proto prim has fixed
                // visibility (it does not vary over time), we can use the
                // pre-cached visibility.
                vis = vis && rproto.visible;
            } else if (vis) {
                // The instancer is visible and the prototype has varying
                // visibility, we must compute visibility from the proto
                // prim to the model instance root.
                for (size_t i = 0; i < rproto.paths.size()-1; ++i) {
                    _ComputeProtoVisibility(
                        _GetPrim(rproto.paths[i+1]).GetMaster(),
                        _GetPrim(rproto.paths[i+0]),
                        time, &vis);
                }
                _ComputeProtoVisibility(
                    _GetPrim(rproto.prototype->protoRootPath),
                    _GetPrim(rproto.paths.back()),
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
            _CorrectTransform(prim, _GetPrim(rproto.prototype->protoRootPath),
                              cachePath, rproto.paths, time);
        }
    } else {
        // Nested Instancer (instancer has instanceIndex)
        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            // For nested instancers, we must update the instance index.
            _InstancerDataMap::const_iterator inst = 
                                            _instancerData.find(cachePath);
            if (inst != _instancerData.end()) {
                // Because the instancer itself has been setup as a prototype of
                // the parent instancer, we can use the same pattern as gprims
                // and pull the instance indices from the prototype. That data
                // is setup in _UpdateInstanceMap() by the parent instancer.
                SdfPath parentInstancerCachePath =
                    inst->second.parentInstancerCachePath;
                if (!parentInstancerCachePath.IsEmpty()) {
                    SdfPath parentInstancerUsdPath =
                        parentInstancerCachePath.GetAbsoluteRootOrPrimPath();
                    UsdPrim parentInstancerUsdPrim =
                        _GetPrim(parentInstancerUsdPath);
                    UsdImagingPrimAdapterSharedPtr adapter =
                        _GetPrimAdapter(parentInstancerUsdPrim);

                    valueCache->GetInstanceIndices(cachePath) =
                        adapter->GetInstanceIndices(parentInstancerCachePath,
                                                    cachePath, time);
                }
            } else {
                TF_CODING_ERROR("PI: %s is not found in _instancerData\n",
                                cachePath.GetText());
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
            // into instance-rate primvars.
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
            _InstancerDataMap::iterator inst = _instancerData.find(cachePath);
            if (!TF_VERIFY(inst != _instancerData.end(),
                              "Unknown instancer %s", cachePath.GetText())) {
                return;
            }

            SdfPath parentInstancerCachePath =
                inst->second.parentInstancerCachePath;
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
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(), cachePath);
        if (!rproto.adapter || (rproto.paths.size() <= 0)) {
            // It's possible we'll get multiple USD edits for the same
            // prototype, one of which will cause a resync.  On resync,
            // we immediately remove the instancer data, but primInfo
            // deletion is deferred until the end of the edit batch.
            // That means, if GetProtoRprim fails we've already
            // queued the prototype for resync and we can safely
            // return AllDirty.
            return HdChangeTracker::AllDirty;
        }

        // XXX: Specifically disallow visibility and transform updates: in
        // these cases, it's hard to tell which prims we should dirty but
        // probably we need to dirty both prototype & instancer. This is a
        // project for later. In the meantime, returning AllDirty causes
        // a re-sync.
        HdDirtyBits dirtyBits = rproto.adapter->ProcessPropertyChange(
            _GetProtoUsdPrim(rproto), cachePath, propertyName);

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

        if (_PrimvarChangeRequiresResync(
                prim, cachePath, propertyName, primvarName)) {
            return HdChangeTracker::AllDirty;
        } else {
            return HdChangeTracker::DirtyPrimvar;
        }
    }

    // XXX: Treat indices & transform changes as re-sync. In theory, we
    // should only need to re-sync for changes to "prototypes", but we're a
    // ways off...
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

    // Scan all instancers for dependencies
    if (instancersToUnload.empty()) {
        TF_FOR_ALL(instIt, _instancerData) {
            SdfPath const& instancerPath = instIt->first;
            _InstancerData& inst = instIt->second;

            if (inst.parentInstancerCachePath == cachePath) {
                instancersToUnload.push_back(instancerPath);
                continue;
            }

            // Check if this is a new prim under an existing proto root.
            // Once the prim is found, we know the entire instancer will be
            // unloaded so we can stop searching.
            bool foundPrim = false;
            TF_FOR_ALL(protoIt, inst.prototypes) {
                if (cachePath.HasPrefix((*protoIt)->protoRootPath)) {
                    // Append this instancer to the unload list (we can't modify
                    // the structure while iterating).
                    instancersToUnload.push_back(instancerPath);
                    foundPrim = true;
                    break;
                }
            }
            if (foundPrim) {
                continue;
            }

            // Check for a dependency on this UsdPrim.
            // XXX(UsdImagingPaths): since we have a cachePath and not a
            // usdPath, it's not clear what the following is doing?
            SdfPath const& usdPath = cachePath;
            _UsdToCacheMap::iterator it = inst.usdToCacheMap.find(usdPath);
            if (it != inst.usdToCacheMap.end()) {
                instancersToUnload.push_back(instancerPath);
            }
        }
    }

    // Propagate changes from the parent instancers down to the children.
    SdfPathVector moreToUnload;
    TF_FOR_ALL(i, instancersToUnload) {
        TF_FOR_ALL(instIt, _instancerData) {
            SdfPath const& instancerPath = instIt->first;
            _InstancerData& inst = instIt->second;
            if (inst.parentInstancerCachePath == *i) {
                moreToUnload.push_back(instancerPath);
            }
        }
    }
    instancersToUnload.insert(instancersToUnload.end(), moreToUnload.begin(),
            moreToUnload.end());
    moreToUnload.clear();

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
        _ProtoRprim const& rproto = _GetProtoRprim(instancerPath, cachePath);

        rproto.adapter->MarkDirty(prim, cachePath, dirty, index);
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
        _ProtoRprim const& rproto = _GetProtoRprim(instancerPath, cachePath);

        rproto.adapter->MarkRefineLevelDirty(prim, cachePath, index);
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
        _ProtoRprim const& rproto = _GetProtoRprim(instancerPath, cachePath);

        rproto.adapter->MarkReprDirty(prim, cachePath, index);
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
        _ProtoRprim const& rproto = _GetProtoRprim(instancerPath, cachePath);

        rproto.adapter->MarkCullStyleDirty(prim, cachePath, index);
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
        _ProtoRprim const& rproto = _GetProtoRprim(instancerPath, cachePath);

        rproto.adapter->MarkRenderTagDirty(prim, cachePath, index);
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
        _ProtoRprim const& rproto = _GetProtoRprim(instancerPath, cachePath);

        rproto.adapter->MarkTransformDirty(prim, cachePath, index);
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
        _ProtoRprim const& rproto = _GetProtoRprim(instancerPath, cachePath);

        rproto.adapter->MarkVisibilityDirty(prim, cachePath, index);
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
    const _ProtoRPrimMap protoPrimMap = instIt->second.protoRprimMap;
    _instancerData.erase(instIt);

    // First, we need to make sure all proto rprims are removed.
    TF_FOR_ALL(protoRprimIt, protoPrimMap) {
        SdfPath     const& cachePath = protoRprimIt->first;
        _ProtoRprim const& proto     = protoRprimIt->second;

        proto.adapter->ProcessPrimRemoval(cachePath, index);
    }

    // Blow away the instancer and the associated local data.
    index->RemoveInstancer(instancerPath);
}

// -------------------------------------------------------------------------- //
// Private IO Helpers
// -------------------------------------------------------------------------- //

UsdImagingPointInstancerAdapter::_ProtoRprim const&
UsdImagingPointInstancerAdapter::_GetProtoRprim(SdfPath const& instrPath, 
                                                 SdfPath const& cachePath) const
{
    static _ProtoRprim const EMPTY;
    SdfPath const& instancerPath =
        (cachePath.GetParentPath().IsPrimVariantSelectionPath())
            ? cachePath.GetParentPath()
            : instrPath;

    _InstancerDataMap::const_iterator it = _instancerData.find(instancerPath);
    if (it == _instancerData.end()) {
        return EMPTY;
    }
    _ProtoRPrimMap::const_iterator protoRprimIt
        = it->second.protoRprimMap.find(cachePath);
    if (protoRprimIt == it->second.protoRprimMap.end()) {
        return EMPTY;
    }
    return protoRprimIt->second;
}

const UsdPrim
UsdImagingPointInstancerAdapter::_GetProtoUsdPrim(
    _ProtoRprim const& proto) const
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

void
UsdImagingPointInstancerAdapter::_UpdateInstanceMap(
                    SdfPath const& instancerPath,
                    UsdTimeCode time) const
{
    UsdPrim instancerPrim = _GetPrim(instancerPath.GetPrimPath());

    TF_DEBUG(USDIMAGING_INSTANCER).Msg(
        "[PointInstancer::_UpdateInstanceMap] %s\n",
        instancerPath.GetText());

    UsdGeomPointInstancer instancer(instancerPrim);
    if (!instancer) {
        TF_WARN("Instancer prim <%s> is not a valid PointInstancer\n",
                instancerPath.GetText());
        return;
    }

    // We expect the instancerData entry for this instancer to be established
    // before this method is called. This map should also never be accessed and
    // mutated at the same time, so doing this lookup from multiple threads is
    // safe.
    _InstancerDataMap::iterator it = _instancerData.find(instancerPath);

    if (it == _instancerData.end()) {
        TF_CODING_ERROR("Instancer prim <%s> had no associated instancerData "
                "entry\n",
                instancerPrim.GetPath().GetText());
        return;
    }
    _InstancerData& instrData = it->second;

    // It's tempting to scan through the protoPools here and attempt to avoid
    // grabbing the lock, but it's not thread safe.
    std::lock_guard<std::mutex> lock(instrData.mutex);

    // Don't recompute the indices if they're already up to date (for example,
    // if a different prototype requested them).
    bool upToDate = instrData.indicesTime == time;
    if (upToDate)
        return;
    instrData.indicesTime = time;

    std::vector<_PrototypeSharedPtr>& prototypes = instrData.prototypes;

    // Reset any indices that were previously accumulated
    TF_FOR_ALL(pg, prototypes) {
        (*pg)->indices.resize(0);
    }

    UsdAttribute indicesAttr = instancer.GetProtoIndicesAttr();
    VtIntArray indices;

    if (!indicesAttr.Get(&indices, time)) {
        TF_RUNTIME_ERROR("Failed to read point cloud indices");
        return;
    }

    // Fetch the "mask", a bit array of enabled/disabled state per instance.
    // If no value is available, mask will be ignored below.
    std::vector<bool> mask = instancer.ComputeMaskAtTime(time);

    for (size_t instanceId = 0; instanceId < indices.size(); ++instanceId) {
        size_t protoIndex = indices[instanceId];

        if (protoIndex > prototypes.size() - 1) {
            TF_WARN("Invalid index (%lu) found in <%s.%s> for time (%s)\n",
                    protoIndex, instancer.GetPath().GetText(), 
                    indicesAttr.GetName().GetText(),
                    TfStringify(time).c_str());
            continue;
        }

        if (mask.size() == 0 || mask[instanceId]) {
            prototypes[protoIndex]->indices.push_back(instanceId);
        }
    }

    TF_DEBUG(USDIMAGING_POINT_INSTANCER_PROTO_CREATED).Msg(
        "[Instancer Updated]: <%s>\n",
        instancerPrim.GetPath().GetText());
}

void
UsdImagingPointInstancerAdapter::_UpdateInstancerVisibility(
        SdfPath const& instancerPath,
        UsdTimeCode time) const
{
    UsdPrim instancerPrim = _GetPrim(instancerPath.GetPrimPath());

    TF_DEBUG(USDIMAGING_INSTANCER).Msg(
        "[PointInstancer::_UpdateInstancerVisibility] %s\n",
        instancerPath.GetText());

    UsdGeomPointInstancer instancer(instancerPrim);
    if (!instancer) {
        TF_WARN("Instancer prim <%s> is not a valid PointInstancer\n",
                instancerPath.GetText());
        return;
    }

    // We expect the instancerData entry for this instancer to be established
    // before this method is called. This map should also never be accessed and
    // mutated at the same time, so doing this lookup from multiple threads is
    // safe.
    _InstancerDataMap::iterator it = _instancerData.find(instancerPath);

    if (it == _instancerData.end()) {
        TF_CODING_ERROR("Instancer prim <%s> had no associated instancerData "
                "entry\n",
                instancerPrim.GetPath().GetText());
        return;
    }
    _InstancerData& instrData = it->second;

    // It's tempting to scan through the protoPools here and attempt to avoid
    // grabbing the lock, but it's not thread safe.
    std::lock_guard<std::mutex> lock(instrData.mutex);

    // Grab the instancer visibility, if it varies over time.
    if (instrData.dirtyBits & HdChangeTracker::DirtyVisibility) {
        bool upToDate = instrData.visibleTime == time;
        if (!upToDate) {
            instrData.visible = _GetInstancerVisible(instancerPath, time);
            instrData.visibleTime = time;
        }
    }
}

int
UsdImagingPointInstancerAdapter::_UpdateDirtyBits(UsdPrim const& instancerPrim)
    const
{
    // We expect the instancerData entry for this instancer to be established
    // before this method is called. This map should also never be accessed and
    // mutated at the same time, so doing this lookup from multiple threads is
    // safe.
    _InstancerDataMap::iterator it = 
                                  _instancerData.find(instancerPrim.GetPath());

    if (it == _instancerData.end()) {
        TF_CODING_ERROR("Instancer prim <%s> had no associated instancerData "
                "entry\n",
                instancerPrim.GetPath().GetText());
        return HdChangeTracker::Clean;
    }
    _InstancerData& instrData = it->second;

    // It's tempting to peek at the dirtyBits here and attempt to avoid grabbing
    // the lock, but it's not thread safe.
    std::lock_guard<std::mutex> lock(instrData.mutex);
 
    UsdGeomPointInstancer instancer(instancerPrim);
    if (!instancer) {
        TF_CODING_ERROR("Instancer prim <%s> is not a valid instancer\n",
                instancerPrim.GetPath().GetText());
        return HdChangeTracker::Clean;
    }

    // If another thread already initialized the dirty bits, we can bail.
    if (instrData.dirtyBits != 
            static_cast<HdDirtyBits>(HdChangeTracker::AllDirty)) {
        return instrData.dirtyBits;
    }

    instrData.dirtyBits = HdChangeTracker::Clean;
    HdDirtyBits* dirtyBits = &instrData.dirtyBits;

    if (!_IsVarying(instancerPrim, 
                       UsdGeomTokens->visibility, 
                       HdChangeTracker::DirtyVisibility,
                       UsdImagingTokens->usdVaryingVisibility,
                       dirtyBits,
                       true)) 
    {
        // When the instancer visibility doesn't vary over time, pre-cache
        // visibility to avoid fetching it on frame change.
        // XXX: The usage of _GetTimeWithOffset here is super-sketch, but
        // it avoids blowing up the inherited visibility cache... We should let
        // this be initialized by the first UpdateForTime instead.
        instrData.visible = _GetInstancerVisible(instancerPrim.GetPath(),
            _GetTimeWithOffset(0.0));
    }

    // These _IsVarying calls are chained to short circuit as soon as we find
    // the instancer to be varying; this is a little hacky, but seemed
    // better than a crazy nested if-statement.
    _IsVarying(instancerPrim,
               UsdGeomTokens->positions,
               HdChangeTracker::DirtyInstancer,
               _tokens->instancer,
               dirtyBits,
               false)
        || _IsVarying(instancerPrim,
               UsdGeomTokens->orientations,
               HdChangeTracker::DirtyInstancer,
               _tokens->instancer,
               dirtyBits,
               false)
        || _IsVarying(instancerPrim,
               UsdGeomTokens->scales,
               HdChangeTracker::DirtyInstancer,
               _tokens->instancer,
               dirtyBits,
               false)
        || _IsVarying(instancerPrim,
               UsdGeomTokens->protoIndices,
               HdChangeTracker::DirtyInstancer,
               _tokens->instancer,
               dirtyBits,
               false)
        || _IsVarying(instancerPrim, 
               UsdGeomTokens->invisibleIds,
               HdChangeTracker::DirtyInstancer,
               _tokens->instancer,
               dirtyBits,
               false);
    return instrData.dirtyBits;
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

void
UsdImagingPointInstancerAdapter::_ComputeProtoPurpose(
                                 UsdPrim const& protoRoot,
                                 UsdPrim const& protoGprim,
                                 TfToken* purpose) const
{
    if (!TF_VERIFY(purpose)) { return; }
    if (!protoGprim.GetPath().HasPrefix(protoRoot.GetPath())) {
        TF_CODING_ERROR("Prototype <%s> is not prefixed under "
                "proto root <%s>\n",
                protoGprim.GetPath().GetText(),
                protoRoot.GetPath().GetText());
        return;
    }

    // Recurse until we get to the protoRoot. With this recursion, we'll
    // process the protoRoot first, then a child, down to the protoGprim.
    if (!protoGprim.IsMaster()  &&
        protoRoot != protoGprim &&
        protoGprim.GetParent()) {
        _ComputeProtoPurpose(protoRoot, protoGprim.GetParent(), purpose);
    }

    // If an ancestor has a purpose, we need not check other prims (bail
    // here at every child recursion after the first parent purpose is found).
    if (*purpose != UsdGeomTokens->default_) {
        return;
    }

    // Fetch the value for this prim, intentionally only reading the default
    // sample, as purpose is not time varying.
    UsdGeomImageable(protoGprim).GetPurposeAttr().Get(purpose);
}

/*virtual*/
SdfPath 
UsdImagingPointInstancerAdapter::GetPathForInstanceIndex(
    SdfPath const &protoCachePath,
    int protoIndex,
    int *instanceCountForThisLevel,
    int *instancerIndex,
    SdfPath *masterCachePath,
    SdfPathVector *instanceContext)
{
    // if the protoCachePath is a prim path, protoCachePath is a point instancer
    // and it may have a parent instancer.
    // if the parent instancer is a native instancer, it could be a
    // variant selection path.
    // e.g.
    //     /path/pointInstancer
    //     /path/pointInstancer{instance=1}
    //
    if (protoCachePath.IsPrimOrPrimVariantSelectionPath()) {
        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "PI: Look for instancer %s [%d]\n",
            protoCachePath.GetText(), protoIndex);

        _InstancerDataMap::iterator it = _instancerData.find(protoCachePath);
        if (it != _instancerData.end()) {
            SdfPath parentInstancerCachePath =
                it->second.parentInstancerCachePath;
            if (!parentInstancerCachePath.IsEmpty()) {
                SdfPath parentInstancerUsdPath =
                    parentInstancerCachePath.GetAbsoluteRootOrPrimPath();
                UsdPrim parentInstancerUsdPrim =
                    _GetPrim(parentInstancerUsdPath);
                UsdImagingPrimAdapterSharedPtr adapter =
                    _GetPrimAdapter(parentInstancerUsdPrim);
                if (adapter) {
                    adapter->GetPathForInstanceIndex(
                        parentInstancerCachePath, protoCachePath, protoIndex,
                        instanceCountForThisLevel, instancerIndex,
                        masterCachePath, instanceContext);
                } else {
                    TF_CODING_ERROR("PI: adapter not found for %s\n",
                                    parentInstancerCachePath.GetText());
                }

                // next parent
                return parentInstancerCachePath;
            }
        }
        // end of recursion.
        if (instanceCountForThisLevel) {
            *instanceCountForThisLevel = 0;
        }
        // don't touch instancerIndex.
        return protoCachePath;
    }

    // extract instancerPath from protoCachePath.
    //
    // protoCachePath = /path/pointInstancer{instance=1}.proto_*
    // instancerPath  = /path/pointInstancer{instance=1}
    //
    SdfPath instancerPath = protoCachePath.GetPrimOrPrimVariantSelectionPath();

    return GetPathForInstanceIndex(instancerPath,
                                   protoCachePath, protoIndex,
                                   instanceCountForThisLevel,
                                   instancerIndex, masterCachePath,
                                   instanceContext);
}

/*virtual*/
SdfPath 
UsdImagingPointInstancerAdapter::GetPathForInstanceIndex(
    SdfPath const &instancerCachePath,
    SdfPath const &protoCachePath,
    int protoIndex,
    int *instanceCountForThisLevel,
    int *instancerIndex,
    SdfPath *masterCachePath,
    SdfPathVector *instanceContext)
{
    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "PI: Look for %s [%d]\n", protoCachePath.GetText(), protoIndex);

    _InstancerDataMap::iterator it = _instancerData.find(instancerCachePath);
    if (it != _instancerData.end()) {
        _InstancerData& instancerData = it->second;

        // find protoCachePath
        TF_FOR_ALL (protoRprimIt, instancerData.protoRprimMap) {
            if (protoRprimIt->first == protoCachePath) {
                // found.
                int count = (int)protoRprimIt->second.prototype->indices.size();
                TF_DEBUG(USDIMAGING_SELECTION).Msg(
                    "  found %s at %d/%d\n",
                    protoRprimIt->first.GetText(), protoIndex, count);

                if (instanceCountForThisLevel) {
                    *instanceCountForThisLevel = count;
                }

                //
                // for individual instance selection, returns absolute index of
                // this instance.
                int absIndex = protoRprimIt->second.prototype->indices[protoIndex % count];
                if (instancerIndex) {
                    *instancerIndex = absIndex;
                }

                // return the instancer
                return instancerCachePath;
            }
        }
    }
    // not found. prevent infinite recursion
    if (instanceCountForThisLevel) {
        *instanceCountForThisLevel = 0;
    }
    return instancerCachePath;
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
        _ProtoRprim const& rproto = _GetProtoRprim(usdPrim.GetPath(),
                                                   cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(rproto);
        return rproto.adapter->SamplePrimvar(
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
        _ProtoRprim const& rproto = _GetProtoRprim(usdPrim.GetPath(),
                                                   cachePath);
        UsdPrim protoPrim = _GetProtoUsdPrim(rproto);
        return rproto.adapter->GetSubdivTags(protoPrim, cachePath, time);
    }
    return UsdImagingPrimAdapter::GetSubdivTags(usdPrim, cachePath, time);
}

/*virtual*/
bool
UsdImagingPointInstancerAdapter::PopulateSelection(
    HdSelection::HighlightMode const& highlightMode,
    SdfPath const &path,
    VtIntArray const &instanceIndices,
    HdSelectionSharedPtr const &result)
{
    // XXX(UsdImagingPaths): Is this a Hydra ID? Cache Path? Or UsdPath?
    // primAdapter.h calls it a usdPath, but clients pass in an rprimPath.
    //
    SdfPath indexPath = _ConvertCachePathToIndexPath(path);
    SdfPathVector const& ids = _GetRprimSubtree(indexPath);

    bool added = false;
    TF_FOR_ALL (it, ids){
        result->AddInstance(highlightMode, *it, instanceIndices);
        added = true;
    }
    return added;
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
        _ProtoRprim const& rproto = _GetProtoRprim(usdPrim.GetPath(), id);
        UsdPrim protoPrim = _GetProtoUsdPrim(rproto);
        return rproto.adapter->GetVolumeFieldDescriptors(
            protoPrim, id, time);
    } else {
        return UsdImagingPrimAdapter::GetVolumeFieldDescriptors(
            usdPrim, id, time);
    }
}

/*virtual*/
SdfPathVector
UsdImagingPointInstancerAdapter::GetDependPaths(SdfPath const &instancerPath) const
{
    _InstancerDataMap::const_iterator it = _instancerData.find(instancerPath);

    SdfPathVector result;
    if (it != _instancerData.end()) {
        _InstancerData const & instancerData = it->second;

        // if the proto path is property path, that should be in the subtree
        // and no need to return.
        TF_FOR_ALL (protoRprimIt, instancerData.protoRprimMap) {
            SdfPath const &protoPath = protoRprimIt->first;
            if (protoPath.IsPrimOrPrimVariantSelectionPath()) {
                if (!protoPath.HasPrefix(instancerPath)) {
                    result.push_back(protoPath);
                }
            }
        }
    }
    // XXX: we may want to cache this result in _instancerData.
    return result;
}

/*virtual*/
void
UsdImagingPointInstancerAdapter::_RemovePrim(SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("Should use overidden ProcessPrimResync/ProcessPrimRemoval");
}

/*virtual*/
VtIntArray
UsdImagingPointInstancerAdapter::GetInstanceIndices(
    SdfPath const &instancerPath, SdfPath const &protoRprim, UsdTimeCode time)
{
    if (!instancerPath.IsEmpty()) {
        _ProtoRprim const &rproto =
            _GetProtoRprim(instancerPath, protoRprim);
        if (!rproto.prototype) {
            TF_CODING_ERROR("PI: No prototype found for parent <%s> of <%s>\n",
                    instancerPath.GetText(),
                    protoRprim.GetText());
        } else {
            _UpdateInstanceMap(instancerPath, time);
            return rproto.prototype->indices;
        }
    }
    return VtIntArray();
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
        _ProtoRprim const& rproto
            = _GetProtoRprim(parentInstancerCachePath, cachePath);
        if (rproto.prototype) {
            if (rproto.prototype->protoRootPath == cachePath) {
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
                            _GetPrim(rproto.prototype->protoRootPath),
                            time);
                }
            }
        } else {
            // parent instancer is a native instancer.
            // to avoid double transform of this instancer and native
            // instancer, set target transform to root transform.
            transformRoot = GetRootTransform();
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

