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
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/version.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/imageable.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

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
    SdfPath const& parentInstancerPath =
                                    GetInstancerBinding(prim, instancerContext);
    SdfPath instancerPath = prim.GetPath();
    UsdGeomPointInstancer inst(prim);

    if (!inst) {
        TF_WARN("Invalid instancer prim <%s>, instancer scheme was not valid\n",
                instancerPath.GetText());
        return SdfPath();
    }

    // for the case we happen to process the same instancer more than once,
    // use variant selection path to make a unique index path (e.g. NI-PI)
    if (_instancerData.find(instancerPath) != _instancerData.end()) {
        static std::atomic_int ctr(0);
        std::string name = TfStringify(++ctr);
        instancerPath = instancerPath.AppendVariantSelection(
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
        return SdfPath();
    }

    // Erase any data that we may have accumulated for a previous instancer at
    // the same path (given that we should get a PrimResync notice before
    // population, perhaps this is unnecessary?).
    if (!TF_VERIFY(_instancerData.find(instancerPath) 
                        == _instancerData.end(), "<%s>\n",
                        instancerPath.GetText())) {
        _UnloadInstancer(instancerPath, index); 
    }

    // Init instancer data for this point instancer.
    _InstancerData& instrData = _instancerData[instancerPath];
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
    instrData.parentInstancerPath = parentInstancerPath;

    TF_DEBUG(USDIMAGING_INSTANCER).Msg("[Add PI] %s\n", instancerPath.GetText());
    index->InsertInstancer(instancerPath, instancerContext);

    UsdAttribute indicesAttr = inst.GetProtoIndicesAttr();

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
        if (!TF_VERIFY(protoIndex < usdProtoPaths.size())) {
            continue;
        }

        _PrototypeSharedPtr &prototype = instrData.prototypes[protoIndex];
        prototype.reset(new _Prototype());
        prototype->enabled = false;        // initialize as disabled.
        prototype->requiresUpdate = true;
        prototype->protoRootPath = usdProtoPaths[protoIndex];
        prototype->indices = VtIntArray(1);

        UsdPrim protoRootPrim = _GetPrim(prototype->protoRootPath);
        if (!protoRootPrim) {
            TF_WARN("Targeted prototype was not found <%s>\n",
                    prototype->protoRootPath.GetText());
            continue;
        }

        // -------------------------------------------------------------- //
        // Traverse the subtree and allocate the Rprims
        // -------------------------------------------------------------- //
        UsdImagingInstancerContext ctx = { instancerPath,
                                           /*childName=*/TfToken(),
                                           SdfPath(),
                                           instancerAdapter};
        _PopulatePrototype(protoIndex, instrData, protoRootPrim, index, &ctx);
    }

    return instancerPath;
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
    SdfPath instancerPath = instancerContext->instancerId;

    _PrototypeSharedPtr &prototype = instrData.prototypes[protoIndex];

    std::vector<UsdTreeIterator> treeStack;
    treeStack.push_back(UsdTreeIterator(protoRootPrim));
    for (; !treeStack.empty();) {
        if (!treeStack.back()) {
            treeStack.pop_back();
            if (!treeStack.empty() && treeStack.back()) {
                // whenever we push a new tree iterator, we leave the
                // last one un-incremented intentionally so we have the
                // residual path. That also means that whenever we pop,
                // must increment the last iterator.
                ++treeStack.back();
            }
            if (treeStack.empty() || !treeStack.back()) {
                continue;
            }
        }
        UsdTreeIterator& treeIt = treeStack.back();
        if (UsdImagingPrimAdapterSharedPtr adapter = 
            _GetPrimAdapter(*treeIt)){
            primCount++;

            //
            // Rprim allocation.
            //
            std::stringstream ss;
            ss << "proto" << protoIndex << "_"
               << treeIt->GetPath().GetName() 
               << "_id" << protoID++;
            TfToken protoName(ss.str());

            SdfPath protoPath;
            if (treeIt->IsInstance()) {
                UsdPrim master = treeIt->GetMaster();
                treeStack.push_back(UsdTreeIterator(master));
                continue;
            } else if (treeIt->IsMaster()) {
                // ignore master root (redirected from IsInstance condition)
                // note that this is not IsInMaster.
                ++treeIt;
                continue;
            } else if (adapter->ShouldCullChildren(*treeIt)) {
                // if the prim is handled by some kind of multiplexing adapter
                // (e.g. another nested PointInstancer)
                // we'll relocate its children to itself, then no longer need to
                // traverse for this instancer.
                //
                // note that this condition should be tested after IsInstance()
                // above, since UsdImagingInstanceAdapter also returns true for
                // ShouldCullChildren but it could be instancing something else.
                UsdImagingInstancerContext ctx = {
                    instancerContext->instancerId,
                    instancerContext->childName,
                    instancerContext->instanceSurfaceShaderPath,
                    UsdImagingPrimAdapterSharedPtr() };
                protoPath = adapter->Populate(*treeIt, index, &ctx);
                treeIt.PruneChildren();
            } else {
                SdfPath const& shader = GetShaderBinding(*treeIt);
                UsdImagingInstancerContext ctx = {
                    instancerPath,
                    /*childName=*/protoName,
                    shader,
                    instancerContext->instancerAdapter };
                protoPath = instancerPath.AppendProperty(protoName);
                adapter->Populate(*treeIt, index, &ctx);
            }

            if (protoPath.IsEmpty()) {
                // Dont track this instancer if it wasn't actually
                // added.
                ++treeIt;
                continue;
            }

            TF_DEBUG(USDIMAGING_INSTANCER).Msg(
                "[Add Instance PI] <%s>  %s\n",
                instancerPath.GetText(), protoPath.GetText());

            //
            // Update instancer data.
            //
            instrData.usdToCacheMap[
                treeIt->GetPath()].push_back(protoPath);
            _ProtoRprim& rproto = instrData.protoRprimMap[protoPath];
            rproto.adapter = adapter;
            rproto.prototype = prototype;

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
            //
            for (int i = treeStack.size()-1; i >= 0; i--) {
                rproto.paths.push_back(treeStack[i]->GetPath());
            }

            // make sure paths is not empty
            TF_VERIFY(rproto.paths.size() > 0);

            // Book keeping, for debugging.
            instantiatedPrimCount++;
        }
        ++treeIt;
    }

    TF_DEBUG(USDIMAGING_POINT_INSTANCER_PROTO_CREATED).Msg(
        "Prototype[%d]: <%s>, primCount: %lu, instantiatedPrimCount: %lu\n",
        protoIndex,
        protoRootPrim.GetPath().GetText(),
        primCount,
        instantiatedPrimCount);
}

void 
UsdImagingPointInstancerAdapter::TrackVariabilityPrep(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      int requestedBits,
                                      UsdImagingInstancerContext const* 
                                          instancerContext)
{
    UsdImagingValueCache* valueCache = _GetValueCache();
    // We will always cache purpose.
    // XXX: this is an indicator of a problem, we should have some flag to
    // drive purpose, just as all other values.
    valueCache->GetPurpose(cachePath);

    if (IsChildPath(cachePath)) {
        // We must ensure InstanceIndices gets populated with some value;
        // perhaps we should reconsider the way the instancing code works in
        // Hydra.
        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            valueCache->GetInstanceIndices(cachePath);
        }

        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(), cachePath);
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(rproto.paths.size() > 0, "%s", cachePath.GetText())) {
            return;
        }

        rproto.adapter->TrackVariabilityPrep(_GetPrim(rproto.paths.front()),
                                             cachePath,
                                                requestedBits);
        return;
    } else {
        // XXX: We must continue to fetch primvars here, see note in
        // UpdateForTimePrep.
        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            UsdGeomPointInstancer instancer(prim);
            if (instancer.GetPositionsAttr().HasValue()) {
                valueCache->GetPrimvar(cachePath, _tokens->translate);
                valueCache->GetPrimvars(cachePath);
            }
            if (instancer.GetOrientationsAttr().HasValue()) {
                valueCache->GetPrimvar(cachePath, _tokens->rotate);
               valueCache->GetPrimvars(cachePath);
            }
            if (instancer.GetScalesAttr().HasValue()) {
                valueCache->GetPrimvar(cachePath, _tokens->scale);
                valueCache->GetPrimvars(cachePath);
            }
        }
    }
}

void 
UsdImagingPointInstancerAdapter::TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  int requestedBits,
                                  int* dirtyBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext)
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

        // If requested, we will always mark indices dirty and update them
        // lazily.
        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            *dirtyBits |= HdChangeTracker::DirtyInstanceIndex;
            // Initializing to an empty value is OK here because either this
            // prototype will be invisible or it will be visible and the indices
            // will be updated.
            VtIntArray a;
            valueCache->GetInstanceIndices(cachePath) = a;
        }

        // XXX: We should never pull purpose directly from the prototype's
        // adapter, since we must compute purpose relative to the model root,
        // however we have no way of communicating that currently.
        UsdPrim protoRootPrim = _GetPrim(rproto.prototype->protoRootPath);
        UsdPrim protoPrim = _GetPrim(rproto.paths.front());
        rproto.adapter->TrackVariability(protoPrim, cachePath,
                                        requestedBits,
                                        &rproto.variabilityBits);
        *dirtyBits |= rproto.variabilityBits;

        // XXX: We need to override the purpose computed by the adapter for the
        // same reason noted above.
        valueCache->GetPurpose(cachePath) = UsdGeomTokens->default_;

        // Compute the purpose.
        {
            _ComputeProtoPurpose(protoRootPrim,
                                 protoPrim,
                             &valueCache->GetPurpose(cachePath));
            // We may have hopped across several instance boundaries, we need to
            // walk back up the stack from instance to master until we get back
            // to the first instance. In the event that there are no instances,
            // the paths vector will not be sufficiently large to enter this
            // loop.
            for (size_t i = 0; i < rproto.paths.size()-1; ++i) {
                _ComputeProtoPurpose(_GetPrim(rproto.paths[i+1]),
                                     _GetPrim(rproto.paths[i+0]),
                                    &valueCache->GetPurpose(cachePath));
            }
        }

        if (!(rproto.variabilityBits & HdChangeTracker::DirtyVisibility)) {
            // Pre-cache visibility, because we now know that it is static for
            // the rprim prototype over all time.
            _ComputeProtoVisibility(protoRootPrim,
                                    protoPrim,
                                    time,
                                    &rproto.visible);
            // We may have hopped across several instance boundaries, we need to
            // walk back up the stack from instance to master until we get back
            // to the first instance. In the event that there are no instances,
            // the paths vector will not be sufficiently large to enter this
            // loop.
            for (size_t i = 0; i < rproto.paths.size()-1; ++i) {
                _ComputeProtoVisibility(
                                    _GetPrim(rproto.paths[i+1]),
                                    _GetPrim(rproto.paths[i+0]),
                                    time,
                                    &rproto.visible);
            }
        }

        // If the instancer varies over time, we should flag the DirtyInstancer
        // bits on the Rprim on every frame, to be sure the instancer data
        // associated with the Rprim gets updated.
        int instancerBits = _UpdateDirtyBits(prim);
        *dirtyBits |=  (instancerBits & HdChangeTracker::DirtyInstancer);

        if (requestedBits & HdChangeTracker::DirtyVisibility) {
            *dirtyBits |= HdChangeTracker::DirtyVisibility;
        }
        return;
    } else {
        TfToken purpose = UsdGeomImageable(prim).ComputePurpose();
        // Empty purpose means there is no opinion, fall back to default.
        if (purpose.IsEmpty())
            purpose = UsdGeomTokens->default_;
        valueCache->GetPurpose(cachePath) = purpose;

        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            // Check to see if this instancer is also being instanced, if so, we
            // need to set dirty bits on the instance index. For instancers, we
            // could probably update the instance index only once, since
            // currently subsequent updates are redundant.
            _InstancerDataMap::const_iterator instr =
                                            _instancerData.find(prim.GetPath());
            if (instr != _instancerData.end()) {
                SdfPath parentInstancerPath = instr->second.parentInstancerPath;
                if (!parentInstancerPath.IsEmpty()) {
                    *dirtyBits |= HdChangeTracker::DirtyInstanceIndex;
                }
            }
        }

        if (requestedBits & HdChangeTracker::DirtyTransform) {
            // this is for instancer transform.
            // _IsTransformVarying(prim, 
            //                     HdChangeTracker::DirtyTransform,
            //                     UsdImagingTokens->usdVaryingXform,
            //                     dirtyBits);

            // XXX: hack to support nested instancer and multi-threaded sync;
            //      we always populate instancer transform into value cache
            *dirtyBits |=  HdChangeTracker::DirtyTransform;
        }

        // to update visibility
        _UpdateDirtyBits(prim);

        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            UsdGeomPointInstancer instancer(prim);

            // TODO: verify lengths of arrays vs. the actual instance count for
            // this frame.

            bool anyVarying = false;

            VtVec3fArray positions;
            if (instancer.GetPositionsAttr().Get(&positions, time)) {
                if (!positions.empty()) {
                    valueCache->GetPrimvar(cachePath, _tokens->translate) =
                        positions;
                    UsdImagingValueCache::PrimvarInfo primvar;
                    primvar.name = _tokens->translate;
                    primvar.interpolation = _tokens->instance;
                    _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
                }

                anyVarying = _IsVarying(prim,
                                        UsdGeomTokens->positions,
                                        HdChangeTracker::DirtyPrimVar,
                                        _tokens->instancer,
                                        dirtyBits,
                                        false);
            }

            VtVec4fArray rotations;
            if (instancer.GetOrientationsAttr().Get(&rotations, time)) {
                if (!rotations.empty()) {
                    valueCache->GetPrimvar(cachePath, _tokens->rotate) =
                        rotations;
                    UsdImagingValueCache::PrimvarInfo primvar;
                    primvar.name = _tokens->rotate;
                    primvar.interpolation = _tokens->instance;
                    _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
                }

                anyVarying = anyVarying || 
                    _IsVarying(prim,
                               UsdGeomTokens->orientations,
                               HdChangeTracker::DirtyPrimVar,
                               _tokens->instancer,
                               dirtyBits,
                               false);
            }

            VtVec3fArray scales(1);
            if (instancer.GetScalesAttr().Get(&scales, time)) {
                if (!scales.empty()) {
                    valueCache->GetPrimvar(cachePath, _tokens->scale) = scales;
                    UsdImagingValueCache::PrimvarInfo primvar;
                    primvar.name = _tokens->scale;
                    primvar.interpolation = _tokens->instance;
                    _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
                }

                anyVarying = anyVarying || 
                    _IsVarying(prim,
                               UsdGeomTokens->scales,
                               HdChangeTracker::DirtyPrimVar,
                               _tokens->instancer,
                               dirtyBits,
                               false);
            }
        }
    }
}

static
bool
_ShouldFetchAttr(UsdAttribute attr, UsdTimeCode time)
{
    return (!time.IsDefault() && attr.HasValue())
            || (time.IsDefault() && attr.HasMetadata(SdfFieldKeys->Default));
}
 
void
UsdImagingPointInstancerAdapter::UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdTimeCode time,
                                   int requestedBits,
                                   UsdImagingInstancerContext const*
                                       instancerContext)
{
    UsdImagingValueCache* valueCache = _GetValueCache();
    if (IsChildPath(cachePath)) {
        // extract instancerPath from cachePath.
        //
        // cachePath:/path/pointInstancer.proto_*
        // instancerPath:/path/pointInstancer
        //
        _UpdateInstanceMap(cachePath.GetParentPath(), time);

        // All Update code paths will update visibility.
        valueCache->GetVisible(cachePath);
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(), cachePath);

        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            valueCache->GetInstanceIndices(cachePath);
        }
        if (requestedBits & HdChangeTracker::DirtyTransform) {
            valueCache->GetInstancerTransform(cachePath);
        }

        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(rproto.prototype, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(rproto.paths.size() > 0, "%s", cachePath.GetText())) {
            return;
        }

        rproto.adapter->UpdateForTimePrep(_GetPrim(rproto.paths.front()),
                                          cachePath,
                                          time, requestedBits);
    } else {
        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            // If this is a nested instancer, we need to prime the
            // InstanceIndices in the value cache and make sure the parent
            // instancer has been updated for the current time.
            _InstancerDataMap::const_iterator instr =
                                            _instancerData.find(prim.GetPath());
            if (instr != _instancerData.end()) {
                SdfPath parentInstancerPath = instr->second.parentInstancerPath;
                if (!parentInstancerPath.IsEmpty()) {
                    // if this instancer has a parent instancer, make sure
                    // the parent instancer is up to date too.
                    // note that the parent instancer doesn't necessarily be
                    // UsdGeomPointInstancer, we delegate to the adapter
                    UsdImagingPrimAdapterSharedPtr adapter =
                        _GetPrimAdapter(_GetPrim(parentInstancerPath));

                    if (adapter) {
                        adapter->UpdateForTimePrep(
                            _GetPrim(parentInstancerPath), parentInstancerPath,
                            time, requestedBits, instancerContext);
                    } else {
                        TF_CODING_ERROR("PI: adapter not found for %s\n",
                                        cachePath.GetText());
                    }
                }
                // Update instancer data, prepare an entry in valueCache
                valueCache->GetInstanceIndices(cachePath);
                _UpdateInstanceMap(cachePath, time);
            } else {
                TF_CODING_ERROR("PI: %s is not found in _instancerData\n",
                                cachePath.GetText());
            }
        }

        if (requestedBits & HdChangeTracker::DirtyTransform) {
            valueCache->GetInstancerTransform(cachePath);
        }

        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            // TODO: Stop querying existance here.
            // 
            // The problem is all primvars are specified as a single dirty bit,
            // so the render index can only ask for all or nothing. If any one
            // primvar varies over time, we end up paying the cost to fetch all
            // of them on every frame.
            //
            // Worse, if we stop reading these values on during track
            // variability, the render index will never get any values for
            // non-time-varying primvars, since we won't fetch them on the first
            // pass and we wont fetch them here since _ShouldFetchAttr is
            // designed to filter out all non-time-varying attributes.
            //
            // Solution: introduce named dirty bits for primvars (and shader
            // parameters). Then we don't need to filter here and everything
            // works as expected.
            //
            // Until then, we must filter here and fetch initial values in track
            // variability.
            UsdGeomPointInstancer instancer(prim);
            if (_ShouldFetchAttr(instancer.GetPositionsAttr(), time)) {
                valueCache->GetPrimvar(cachePath, _tokens->translate);
                valueCache->GetPrimvars(cachePath);
            }
            if (_ShouldFetchAttr(instancer.GetOrientationsAttr(), time)) {
                valueCache->GetPrimvar(cachePath, _tokens->rotate);
                valueCache->GetPrimvars(cachePath);
            }
            if (_ShouldFetchAttr(instancer.GetScalesAttr(), time)) {
                valueCache->GetPrimvar(cachePath, _tokens->scale);
                valueCache->GetPrimvars(cachePath);
            }
        }
    }
}

void 
UsdImagingPointInstancerAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               int requestedBits,
                               int* resultBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext)
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

        // At this point we can flip the initialized flag, only
        // _FilterRequestedBits requires it.
        if (!rproto.initialized) {
            _ProtoRprim& rprotoRW = const_cast<_ProtoRprim&>(rproto);
            rprotoRW.initialized = true;
        }

        // Uphold the original precondition: result == requested bits.
        *resultBits = requestedBits;

        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
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
        if (protoReqBits != HdChangeTracker::Clean)
            rproto.adapter->UpdateForTime(_GetPrim(rproto.paths.front()), cachePath,
                                          time, protoReqBits, resultBits);

        // Make sure we always query and return visibility. This is done
        // after the adapter update to ensure we get our specialized view of
        // visibility.
        requestedBits |= HdChangeTracker::DirtyVisibility;
        *resultBits |= HdChangeTracker::DirtyVisibility;

        // Apply the instancer visibility at the current time to the
        // instance. Notice that the instance will also pickup the instancer
        // visibility at the time offset.
        if (requestedBits & HdChangeTracker::DirtyVisibility) {
            bool& vis = valueCache->GetVisible(cachePath);
            bool protoHasFixedVis = !(rproto.variabilityBits 
                                      & HdChangeTracker::DirtyVisibility);

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
                _ComputeProtoVisibility(
                    _GetPrim(rproto.prototype->protoRootPath), 
                    _GetPrim(rproto.paths.front()), 
                    time,
                    &vis); 
            }
        }

        if (*resultBits & HdChangeTracker::DirtyTransform) {
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
                SdfPath parentInstancerPath = inst->second.parentInstancerPath;
                UsdImagingPrimAdapterSharedPtr adapter =
                    _GetPrimAdapter(_GetPrim(parentInstancerPath));

                valueCache->GetInstanceIndices(cachePath) =
                    adapter->GetInstanceIndices(parentInstancerPath, cachePath);
            } else {
                TF_CODING_ERROR("PI: %s is not found in _instancerData\n",
                                cachePath.GetText());
            }
        }

        // For the instancer itself, we only send translate, rotate and scale
        // back as primvars, which all fall into the DirtyPrimVar bucket
        // currently.
        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            UsdGeomPointInstancer instancer(prim);

            // PERFORMANCE: It would be nice to track variability of individual
            // primvars separately, since uniform values will  needlessly be
            // sent to the GPU on every frame.
            VtVec3fArray positions;
            if (instancer.GetPositionsAttr().Get(&positions, time)) {
                valueCache->GetPrimvar(cachePath, _tokens->translate) = 
                                                                    positions;
                UsdImagingValueCache::PrimvarInfo primvar;
                primvar.name = _tokens->translate;
                primvar.interpolation = _tokens->instance;
                _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
            }

            VtVec4fArray rotations;
            if (instancer.GetOrientationsAttr().Get(&rotations, time)) {
                valueCache->GetPrimvar(cachePath, _tokens->rotate) = rotations;
                UsdImagingValueCache::PrimvarInfo primvar;
                primvar.name = _tokens->rotate;
                primvar.interpolation = _tokens->instance;
                _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
            }

            VtVec3fArray scales;
            if (instancer.GetScalesAttr().Get(&scales, time)) {
                valueCache->GetPrimvar(cachePath, _tokens->scale) = scales;
                UsdImagingValueCache::PrimvarInfo primvar;
                primvar.name = _tokens->scale;
                primvar.interpolation = _tokens->instance;
                _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
            }
        }

        // update instancer transform.
        if (requestedBits & HdChangeTracker::DirtyTransform) {
            _InstancerDataMap::iterator inst = _instancerData.find(cachePath);
            if (!TF_VERIFY(inst != _instancerData.end(),
                              "Unknown instancer %s", cachePath.GetText())) {
                return;
            }

            SdfPath parentInstancerPath = inst->second.parentInstancerPath;
            if (!parentInstancerPath.IsEmpty()) {
                // if nested, double transformation should be avoided.
                UsdImagingPrimAdapterSharedPtr adapter =
                    _GetPrimAdapter(_GetPrim(parentInstancerPath));

                // parentInstancer doesn't necessarily be UsdGeomPointInstancer.
                // lookup and delegate adapter to compute the instancer transform.
                _GetValueCache()->GetInstancerTransform(cachePath) =
                    adapter->GetRelativeInstancerTransform(parentInstancerPath,
                                                           cachePath, time);
            } else {
                // if not nested, simply put the transform of the instancer.
                _GetValueCache()->GetInstancerTransform(cachePath) =
                    this->GetRelativeInstancerTransform(parentInstancerPath,
                                                        cachePath, time);
            }

        }
    }
}

int
UsdImagingPointInstancerAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    // Blast everything.
    return HdChangeTracker::AllDirty;
    // XXX: Change processing needs to be routed through the adapter earlier,
    // currently we don't see changes to the prototypes.
    if (IsChildPath(cachePath)) {
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(), cachePath);
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return HdChangeTracker::AllDirty;
        }
        if (!TF_VERIFY(rproto.paths.size() > 0, "%s", cachePath.GetText())) {
            return HdChangeTracker::AllDirty;
        }
        return rproto.adapter->ProcessPropertyChange(
            _GetPrim(rproto.paths.front()),
                                                   cachePath, propertyName);
    }

    // Blast everything. This will trigger a prim resync; see ProcessPrimResync.
    return HdChangeTracker::AllDirty;
}

void
UsdImagingPointInstancerAdapter::_ProcessPrimRemoval(SdfPath const& usdPath,
                                             UsdImagingIndexProxy* index,
                                             SdfPathVector* instancersToReload)
{
    // If prim data exists at this path, we'll drop it now.
    _InstancerDataMap::iterator instIt = _instancerData.find(usdPath);
    SdfPathVector instancersToUnload;

    if (instIt != _instancerData.end()) {
        while (instIt != _instancerData.end()) {
            SdfPath parentInstancerPath = instIt->second.parentInstancerPath;
            instancersToUnload.push_back(instIt->first);

            // Setup the next iteration.
            if (parentInstancerPath.IsEmpty()) {
                break;
            }

            // Note that the parent may be owned by a different adapter, so we
            // might not find it here.
            instIt = _instancerData.find(parentInstancerPath);
        }
    } else {
        if (!IsChildPath(usdPath)) {
            // This is a path that is neither an instancer or a child path,
            // which means it was only tracked for change processing at an
            // instance root.
            return;
        }
    }

    // Otherwise, the usdPath must be a path to one of the prototype rprims.

    // The prim in the Usd scenegraph could be shared among many instancers, so
    // we search each instancer for the presence of the given usdPath. Any
    // instancer that references this prim must be rebuilt, we don't currently
    // support incrementally rebuilding an instancer.

    // Scan all instancers for dependencies
    if (instancersToUnload.empty()) {
        TF_FOR_ALL(instIt, _instancerData) {
            SdfPath const& instancerPath = instIt->first;
            _InstancerData& inst = instIt->second;

            if (inst.parentInstancerPath == usdPath) {
                instancersToUnload.push_back(instancerPath);
                continue;
            }

            // Check if this is a new prim under an existing proto root.
            // Once the prim is found, we know the entire instancer will be
            // unloaded so we can stop searching.
            bool foundPrim = false;
            TF_FOR_ALL(protoIt, inst.prototypes) {
                if (usdPath.HasPrefix((*protoIt)->protoRootPath)) {
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
            if (inst.parentInstancerPath == *i) {
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
        SdfPath parentInstancerPath = instIt->second.parentInstancerPath;

        _UnloadInstancer(*i, index);

        // If the caller doesn't need to know what to reload, we're done in this
        // loop.
        if (!instancersToReload) {
            continue;
        }

        // Never repopulate child instancers directly, they are only repopulated
        // by populating the parent.
        if (!parentInstancerPath.IsEmpty()) {
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
UsdImagingPointInstancerAdapter::ProcessPrimResync(SdfPath const& usdPath,
                                             UsdImagingIndexProxy* index)
{
    // _ProcesPrimRemoval does the heavy lifting, returing a set of instancers
    // to repopulate. Note that the child/prototype prims need not be in the
    // "toReload" list, as they will be discovered in the process of reloading
    // the root instancer prim.
    SdfPathVector toReload;
    _ProcessPrimRemoval(usdPath, index, &toReload);
    for (SdfPath const& instancerRootPath : toReload) {
        index->Repopulate(instancerRootPath);
    }
}

/*virtual*/
void
UsdImagingPointInstancerAdapter::ProcessPrimRemoval(SdfPath const& usdPath,
                                                    UsdImagingIndexProxy* index)
{
    // Process removals, but do not repopulate.
    _ProcessPrimRemoval(usdPath, index, /*instancersToRepopulate*/nullptr);
}

bool
UsdImagingPointInstancerAdapter::ShouldCullChildren(UsdPrim const& prim)
{
    return true;
}

void
UsdImagingPointInstancerAdapter::_UnloadInstancer(SdfPath const& instancerPath,
                                            UsdImagingIndexProxy* index)
{
    _InstancerDataMap::iterator instIt = _instancerData.find(instancerPath);
    // First, we need to make sure all proto rprims are removed.
    TF_FOR_ALL(protoRprimIt, instIt->second.protoRprimMap) {
        SdfPath const& cachePath = protoRprimIt->first;
        index->RemoveRprim(cachePath);
        index->RemoveDependency(cachePath);
    }

    // Blow away the instancer and the associated local data.
    index->RemoveInstancer(instancerPath);
    index->RemoveDependency(instancerPath);
    _instancerData.erase(instIt);
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

bool
UsdImagingPointInstancerAdapter::_GetInstancerVisible(
    SdfPath const &instancerPath, UsdTimeCode time)
{
    bool visible = UsdImagingPrimAdapter::GetVisible(
        _GetPrim(instancerPath.GetPrimPath()), time);

    if (visible) {
        _InstancerDataMap::const_iterator it
            = _instancerData.find(instancerPath);
        if (it != _instancerData.end()) {
            // note that parent instancer may not be a namespace parent
            // (e.g. master -> instance)
            SdfPath const &parentInstancerPath = it->second.parentInstancerPath;
            if (!parentInstancerPath.IsEmpty()) {
                return _GetInstancerVisible(parentInstancerPath, time);
            }
        }
    }

    return visible;
}

void
UsdImagingPointInstancerAdapter::_UpdateInstanceMap(
                    SdfPath const& instancerPath,
                    UsdTimeCode time)
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

    // Grab the instancer visiblity, if it varies over time.
    if (instrData.dirtyBits & HdChangeTracker::DirtyVisibility) {
        instrData.visible = _GetInstancerVisible(instancerPath, time);
    }

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

int
UsdImagingPointInstancerAdapter::_UpdateDirtyBits(
                    UsdPrim const& instancerPrim)
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
    if (instrData.dirtyBits != HdChangeTracker::AllDirty)
        return instrData.dirtyBits;

    instrData.dirtyBits = HdChangeTracker::Clean;
    int* dirtyBits = &instrData.dirtyBits;

    if (!_IsVarying(instancerPrim, 
                       UsdGeomTokens->visibility, 
                       HdChangeTracker::DirtyVisibility,
                       UsdImagingTokens->usdVaryingVisibility,
                       dirtyBits,
                       true)) 
    {
        // When the instancer visibility doesn't vary over time, pre-cache
        // visibility to avoid fetching it on frame change.
        instrData.visible = _GetInstancerVisible(instancerPrim.GetPath(),
                                                 /*time doesn't matter*/1.0);
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
                                                   UsdTimeCode time)
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
        GfMatrix4d parentToWorld = GetTransform(parent, time,
                                                   /*ignoreRootTransform=*/true);

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
                                 bool* vis)
{
    if (!TF_VERIFY(vis)) { return; }
    if (!protoGprim.GetPath().HasPrefix(protoGprim.GetPath())) {
        TF_CODING_ERROR("Prototype <%s> is not prefixed under "
                "proto root <%s>\n",
                protoGprim.GetPath().GetText(),
                protoRoot.GetPath().GetText());
        return;
    }

    // if it's in invised list, set vis to false
    if (_delegate->IsInInvisedPaths(protoGprim.GetPath())) {
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
                                 TfToken* purpose)
{
    if (!TF_VERIFY(purpose)) { return; }
    if (!protoGprim.GetPath().HasPrefix(protoGprim.GetPath())) {
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
    SdfPath const &protoPath, int instanceIndex, int *instanceCount,
    int *absoluteInstanceIndex, SdfPath *rprimPath,
    SdfPathVector *instanceContext)
{
    // if the protoPath is a prim path, protoPath is a point instancer
    // and it may have a parent instancer.
    // if the parent instancer is a native instancer, it could be a
    // variant selection path.
    // e.g.
    //     /path/pointInstancer
    //     /path/pointInstancer{instance=1}
    //
    if (protoPath.IsPrimOrPrimVariantSelectionPath()) {
        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "PI: Look for instancer %s [%d]\n", protoPath.GetText(), instanceIndex);

        _InstancerDataMap::iterator it = _instancerData.find(protoPath);
        if (it != _instancerData.end()) {
            SdfPath parentInstancerPath = it->second.parentInstancerPath;
            if (not parentInstancerPath.IsEmpty()) {

                UsdImagingPrimAdapterSharedPtr adapter =
                    _GetPrimAdapter(_GetPrim(parentInstancerPath));
                if (adapter) {
                    adapter->GetPathForInstanceIndex(
                        parentInstancerPath, protoPath, instanceIndex,
                        instanceCount, absoluteInstanceIndex,
                        rprimPath, instanceContext);
                } else {
                    TF_CODING_ERROR("PI: adapter not found for %s\n",
                                    parentInstancerPath.GetText());
                }

                // next parent
                return parentInstancerPath;
            }
        }
        // end of recursion.
        if (instanceCount) {
            *instanceCount = 0;
        }
        // don't touch absoluteInstanceIndex.
        return protoPath;
    }

    // extract instancerPath from protoPath.
    //
    // protoPath     = /path/pointInstancer{instance=1}.proto_*
    // instancerPath = /path/pointInstancer{instance=1}
    //
    SdfPath instancerPath = protoPath.GetPrimOrPrimVariantSelectionPath();

    return GetPathForInstanceIndex(instancerPath,
                                   protoPath, instanceIndex, instanceCount,
                                   absoluteInstanceIndex, rprimPath,
                                   instanceContext);
}

/*virtual*/
SdfPath 
UsdImagingPointInstancerAdapter::GetPathForInstanceIndex(
    SdfPath const &instancerPath, SdfPath const &protoPath,
    int instanceIndex, int *instanceCount,
    int *absoluteInstanceIndex, SdfPath *rprimPath,
    SdfPathVector *instanceContext)
{
    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "PI: Look for %s [%d]\n", protoPath.GetText(), instanceIndex);

    _InstancerDataMap::iterator it = _instancerData.find(instancerPath);
    if (it != _instancerData.end()) {
        _InstancerData& instancerData = it->second;

        // find protoPath
        TF_FOR_ALL (protoRprimIt, instancerData.protoRprimMap) {
            if (protoRprimIt->first == protoPath) {
                // found.
                int count = (int)protoRprimIt->second.prototype->indices.size();
                TF_DEBUG(USDIMAGING_SELECTION).Msg(
                    "  found %s at %d/%d\n",
                    protoRprimIt->first.GetText(), instanceIndex, count);

                if (instanceCount) {
                    *instanceCount = count;
                }

                //
                // for individual instance selection, returns absolute index of
                // this instance.
                int absIndex = protoRprimIt->second.prototype->indices[instanceIndex % count];
                if (absoluteInstanceIndex) {
                    *absoluteInstanceIndex = absIndex;
                }

                // return the instancer
                return instancerPath;
            }
        }
    }
    // not found. prevent infinite recursion
    if (instanceCount) {
        *instanceCount = 0;
    }
    return instancerPath;
}

/*virtual*/
bool
UsdImagingPointInstancerAdapter::PopulateSelection(
    SdfPath const &path,
    VtIntArray const &instanceIndices,
    HdxSelectionSharedPtr const &result)
{
    SdfPath indexPath = _delegate->GetPathForIndex(path);
    SdfPathVector const& ids =
        _delegate->GetRenderIndex().GetRprimSubtree(indexPath);

    bool added = false;
    TF_FOR_ALL (it, ids){
        result->AddInstance(*it, instanceIndices);
        added = true;
    }
    return added;
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
VtIntArray
UsdImagingPointInstancerAdapter::GetInstanceIndices(
    SdfPath const &instancerPath, SdfPath const &protoRprim)
{
    if (!instancerPath.IsEmpty()) {
        _ProtoRprim const &rproto =
            _GetProtoRprim(instancerPath, protoRprim);
        if (!rproto.prototype) {
            TF_CODING_ERROR("PI: No prototype found for parent <%s> of <%s>\n",
                    instancerPath.GetText(),
                    protoRprim.GetText());
        } else {
            return rproto.prototype->indices;
        }
    }
    return VtIntArray();
}

/*virtual*/
GfMatrix4d
UsdImagingPointInstancerAdapter::GetRelativeInstancerTransform(
    SdfPath const &parentInstancerPath, SdfPath const &cachePath,
    UsdTimeCode time)
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

    if (!parentInstancerPath.IsEmpty()) {
        // this instancer has a parent instancer. see if this instancer 
        // is a protoRoot or not.
        _ProtoRprim const& rproto
            = _GetProtoRprim(parentInstancerPath, cachePath);
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

