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
#include "pxr/usdImaging/usdImaging/instanceAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/imageable.h"

#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

#include <limits>

// // XXX: These should come from Hd or UsdImaging
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (instance)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingInstanceAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

// ------------------------------------------------------------

UsdImagingInstanceAdapter::UsdImagingInstanceAdapter() 
    : BaseAdapter()
{
}

UsdImagingInstanceAdapter::~UsdImagingInstanceAdapter() 
{
}

SdfPath
UsdImagingInstanceAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    TF_DEBUG(USDIMAGING_INSTANCER).Msg(
        "[Populate NI] prim=%s\n", prim.GetPath().GetText());

    SdfPath instancePath = prim.GetPath();
    if (not TF_VERIFY(
            prim.IsInstance(), 
            "Cannot populate instance adapter from <%s>, "
            "which is not an instance prim.",
            instancePath.GetString().c_str())) {
        return SdfPath();
    }

    const UsdPrim& masterPrim = prim.GetMaster();
    if (not TF_VERIFY(
            masterPrim, "Cannot get master prim for <%s>",
            instancePath.GetString().c_str())) {
        return SdfPath();
    }

    // This is a shared_ptr to the prim adapter for the current instancePrim.
    const UsdImagingPrimAdapterSharedPtr instancedPrimAdapter = 
        _GetPrimAdapter(prim, /* ignoreInstancing = */ true);

    // This is a shared_ptr to ourself. The InstancerContext requires the
    // adapter shared_ptr.
    UsdImagingPrimAdapterSharedPtr const& instancerAdapter = 
                        _GetSharedFromThis();

    // If the current prim is drawable (instancedPrimAdapter is non-NULL), we
    // ask it to give us the shader binding.  Otherwise, this
    // (instancerAdapter) will provide the shader binding.
    const SdfPath& instanceShaderBinding = (instancedPrimAdapter
            ? instancedPrimAdapter : instancerAdapter)->GetShaderBinding(prim);

    // Store away the path of the given instance prim to use as the
    // instancer for Hydra if this is the first time we've seen this 
    // (master, shader binding) pair.
    const SdfPath& instancerPath = 
        _masterToInstancerMap[masterPrim.GetPath()].insert(
            std::make_pair(instanceShaderBinding, instancePath)).first->second;

    _InstancerData& instancerData = _instancerData[instancerPath];
    instancerData.dirtyBits = HdChangeTracker::AllDirty;

    std::vector<UsdPrim> nestedInstances;

    if (instancerData.instancePaths.empty()) {
        instancerData.masterPath = masterPrim.GetPath();
        instancerData.shaderBindingPath = instanceShaderBinding;

        // Add this instancer into the render index.
        UsdImagingInstancerContext ctx = { SdfPath(),
                                           TfToken(),
                                           SdfPath(),
                                           instancerAdapter };

        // -------------------------------------------------------------- //
        // Initialize this protoGroup.
        // -------------------------------------------------------------- //
        _ProtoGroupPtr& grp = instancerData.protoGroup;
        grp.reset(new _ProtoGroup());
        // Initialize to inf. to avoid collisions in our initial time
        // and the first time the client attempts to draw. (inf. = no data
        // loaded yet). RequiresUpdate=true is not enough.
        grp->time = std::numeric_limits<double>::infinity();
        grp->indices = VtIntArray(1);

        // -------------------------------------------------------------- //
        // Allocate the Rprims
        // -------------------------------------------------------------- //
        int protoID = 0;

        // The master is a typeless stub for instancing and should
        // never itself be a renderable gprim, so we can skip it initially
        // and just iterate over its children;
        UsdTreeIterator treeIt(masterPrim);
        ++treeIt;

        int primCount = 0;
        for (; treeIt; ++treeIt) {
            // If we encounter an instance in this master, save it aside
            // for a subsequent population pass since we'll need to populate
            // its master once we're done with this one.
            if (treeIt->IsInstance()) {
                nestedInstances.push_back(*treeIt);
                continue;
            }
            
            UsdImagingPrimAdapterSharedPtr const& adapter = 
                _GetPrimAdapter(*treeIt);
            if (not adapter) {
                continue;
            }
                
            // 
            // Rprim allocation. 
            //
            const TfToken protoName(TfStringPrintf(
                "proto_%s_id%d", treeIt->GetName().GetText(), protoID++));
            const SdfPath protoPath = 
                _InsertProtoRprim(&treeIt, protoName,
                                  instanceShaderBinding,
                                  instancerPath, instancerAdapter, index);
                    
            // 
            // Update instancer data.
            //
            _ProtoRprim& rproto = instancerData.primMap[protoPath];
            rproto.path =  treeIt->GetPath();
            rproto.adapter = adapter;
            rproto.protoGroup = grp;
            ++primCount;

            TF_DEBUG(USDIMAGING_INSTANCER).Msg(
                "[Add Instance NI] <%s>  %s (%s), adapter = %p\n",
                instancerPath.GetText(), protoPath.GetText(), treeIt->GetName().GetText(),
                adapter.get());
        }
        if (primCount > 0) {
            index->InsertInstancer(instancerPath, &ctx); 
        } else if (nestedInstances.empty()) {
            // if this instance path ends up to have no prims in subtree
            // and not an instance itself , we don't need to track this path
            // any more.

            instancePath = SdfPath();
        }
    }

    // If this instance prim itself is a drawable gprim, we need to add an
    // rprim for it to the render index. This won't happen when processing 
    // the instance's master, since the master is never a drawable prim.
    // 
    // We need to ensure this rprim has a different path from the instancer,
    // so we follow the same scheme as above: we insert a new proto rprim
    // for this instance under a child path with a unique suffix.
    //
    // In this case, we dispatch to the underlying PrimAdapter and disable
    // instancing.
    if (instancedPrimAdapter) {
        UsdTreeIterator treeIt(prim);
        const SdfPath protoPath = 
            _InsertProtoRprim(&treeIt, TfToken(),
                              instanceShaderBinding,
                              SdfPath(), instancedPrimAdapter, index);
        instancePath = SdfPath();
    }

    if (not instancePath.IsEmpty()) {
        // Add an entry to the instancer data for the given instance. Keep
        // the vector sorted for faster lookups during change processing.
        std::vector<SdfPath>& instancePaths = instancerData.instancePaths;
        std::vector<SdfPath>::iterator it = std::lower_bound(
            instancePaths.begin(), instancePaths.end(), instancePath);

        // We may repopulate instances we've already seen during change
        // processing when nested instances are involved. Rather than do
        // some complicated filtering in ProcessPrimResync to avoid this,
        // we just silently ignore duplicate instances here.
        if (it == instancePaths.end() or *it != instancePath) {
            instancePaths.insert(it, instancePath);

            TF_DEBUG(USDIMAGING_INSTANCER).Msg(
                "[Add Instance NI] <%s>  %s\n",
                instancerPath.GetText(), instancePath.GetText());

            _instanceToInstancerMap[instancePath] = instancerPath;

            // Make sure we add a dependency for this instance on this adapter,
            // so that changes to the instance are handled properly.
            index->AddDependency(instancePath, instancerAdapter);

            // If we're adding an instance to an instancer that had already
            // been drawn, we need to ensure it and its rprims are marked
            // dirty to ensure the new instance will be drawn.
            if (instancerData.protoGroup->time !=
                std::numeric_limits<double>::infinity()) {

                instancerData.protoGroup->time = 
                    std::numeric_limits<double>::infinity();
                index->RefreshInstancer(instancerPath);
                for (const auto& cachePathAndRprim : instancerData.primMap) {
                    index->Refresh(cachePathAndRprim.first);
                }
            }
        }
    }

    // We're done modifying data structures for the passed in instance,
    // so now it's safe to re-enter this function to populate the
    // nested instances we discovered.
    TF_FOR_ALL(nestedInstanceIt, nestedInstances) {
        Populate(*nestedInstanceIt, index, instancerContext);
    }

    return instancerPath;
}

SdfPath
UsdImagingInstanceAdapter::_InsertProtoRprim(UsdTreeIterator* it,
                        TfToken const& protoName,
                        SdfPath instanceShaderBinding,
                        SdfPath instancerPath,
                        UsdImagingPrimAdapterSharedPtr const& instancerAdapter,
                        UsdImagingIndexProxy* index)
{
    UsdPrim const& prim = **it;
    SdfPath protoPath;

    // Talk to the prim's native adapter to do population and ShaderBinding
    // queries on our behalf.
    UsdImagingPrimAdapterSharedPtr const& adapter = 
                        _GetPrimAdapter(prim, /* ignoreInstancing = */ true);

    // If this prim is an instance, we can use the given instanceShaderBinding
    // Otherwise, this is a prim in a master; we need to see if there's any
    // applicable bindings authored and only fallback to the instance binding
    // if there isn't one.
    auto getShaderForPrim = [adapter, instanceShaderBinding](UsdPrim const& prim) {
        if (prim.IsInstance()) { 
            return instanceShaderBinding;
        }
        SdfPath shaderId = adapter->GetShaderBinding(prim);
        return shaderId.IsEmpty() ? instanceShaderBinding : shaderId;
    };

    // If this prim is an instance, we don't want Hydra to treat its rprim
    // as a prototype to be shared with all other instances. Only prims in
    // masters should be treated as prototypes for the instancer.
    auto getInstancerPathForPrim = [instancerPath](UsdPrim const& prim) {
        return prim.IsInstance() ? SdfPath() : instancerPath;
    };

    // Here we use the instancerAdapter so when AddDependency is called, the
    // the InstanceAdapter will be registered to handle change processing and
    // data access.
    UsdImagingInstancerContext ctx = { instancerPath,
                                       protoName,
                                       getShaderForPrim(prim),
                                       instancerAdapter };

    // There is no need to call AddDependency, as it will be picked up via the
    // instancer context.
    SdfPath populatedPath = adapter->Populate(prim, index, &ctx);

    if (adapter->ShouldCullChildren(prim)) {
        // If the prim's adapter wants to prune children, it's likely some sort
        // of multiplexing adapter, in which case we wont attempt to relocate it
        // under the instancer (this happens in the case of recursive
        // instancers).
        it->PruneChildren();

        // we use populatedPath instead of prim.GetPath() so that prim adapter
        // can clone the prim if necessary (see PointInstancer)
        protoPath = populatedPath;
    } else {
        protoPath = instancerPath.AppendProperty(protoName);
    }

    return protoPath;
}

bool
UsdImagingInstanceAdapter::_IsChildPrim(UsdPrim const& prim, 
                                        SdfPath const& cachePath) const
{
    // Child paths are the instancer path with a property appended. For leaf
    // gprims (mesh, points, etc) we use child paths, but for adapters which
    // prune childern (and therefor likely want to manage their namespace), we
    // use the prim path of the original prim, thus IsChildPath fails.
    return IsChildPath(cachePath) 
        // When we have a non-child path that is not in the instancer data, it
        // must be an instanced prim which we did not relocate during
        // population.
        //
        // We could make this less ad-hoc by storing a list of valid non-child
        // paths, in exchange for the overhead of maintaining that list.
        or nullptr == TfMapLookupPtr(_instancerData, prim.GetPath());
}

void 
UsdImagingInstanceAdapter::TrackVariabilityPrep(UsdPrim const& prim,
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

    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (not TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }

        // We must ensure InstanceIndices gets populated with some value;
        // perhaps we should reconsider the way the instancing code works in
        // Hydra.
        if (requestedBits & HdChangeTracker::DirtyInstanceIndex)
            valueCache->GetInstanceIndices(cachePath);

        rproto.adapter->TrackVariabilityPrep(
            _GetPrim(rproto.path), cachePath, requestedBits, 
            &instancerContext);
    } 
    else {
        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            valueCache->GetPrimvar(cachePath, HdTokens->instanceTransform);
            valueCache->GetPrimvars(cachePath);
        }
    }
}

void 
UsdImagingInstanceAdapter::TrackVariability(UsdPrim const& prim,
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

    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim & rproto = const_cast<_ProtoRprim&>(
                                     _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext));
        if (not TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
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
        
        UsdPrim protoPrim = _GetPrim(rproto.path);
        rproto.adapter->TrackVariability(
            protoPrim, cachePath, requestedBits, &rproto.variabilityBits,
            &instancerContext);
        *dirtyBits |= rproto.variabilityBits;

        if (not (rproto.variabilityBits & HdChangeTracker::DirtyVisibility)) {
            // Pre-cache visibility, because we now know that it is static for
            // the rprim prototype over all time.
            rproto.visible = GetVisible(protoPrim, time);
        }

        // If any of the instances varies over time, we should flag the 
        // DirtyInstancer bits on the Rprim on every frame, to be sure the 
        // instancer data associated with the Rprim gets updated.
        int instancerBits = _UpdateDirtyBits(prim.GetStage()->
                                 GetPrimAtPath(instancerContext.instancerId));
        *dirtyBits |=  (instancerBits & HdChangeTracker::DirtyInstancer);

        if (requestedBits & HdChangeTracker::DirtyVisibility) {
            *dirtyBits |= HdChangeTracker::DirtyVisibility;
        }

    } else {
        // In this case, prim is an instance master. Master prims provide
        // no data of their own, so we fall back to the default purpose.
        valueCache->GetPurpose(cachePath) = UsdGeomTokens->default_;

        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            int instancerBits = _UpdateDirtyBits(prim);

            // If any of the instance transforms vary over time, the
            // instancer will have the DirtyInstancer bit set. Translate
            // that to DirtyPrimVar so that Hd will note that the 
            // instance transform primvar is varying over time.
            if (instancerBits & HdChangeTracker::DirtyInstancer) {
                *dirtyBits |= HdChangeTracker::DirtyPrimVar;
            }

            VtMatrix4dArray instanceXforms;
            if (_ComputeInstanceTransform(prim, &instanceXforms, time)) {
                valueCache->GetPrimvar(
                    cachePath, HdTokens->instanceTransform) = instanceXforms;
                UsdImagingValueCache::PrimvarInfo primvar;
                primvar.name = HdTokens->instanceTransform;
                primvar.interpolation = _tokens->instance;
                _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
            }
        }
    }
}

template <typename Functor>
void
UsdImagingInstanceAdapter::_RunForAllInstancesToDraw(
    UsdPrim const& instancer,
    Functor* fn)
{
    const _InstancerData* instancerData = 
        TfMapLookupPtr(_instancerData, instancer.GetPath());
    if (not TF_VERIFY(instancerData, "Can't find instancerData for %s",
                      instancer.GetPath().GetText())) {
        return;
    }

    fn->Initialize(instancerData->numInstancesToDraw);

    size_t instanceCount = 0;
    std::vector<UsdPrim> instanceContext;
    _RunForAllInstancesToDrawImpl(
        instancer, &instanceContext, &instanceCount, fn);
}

template <typename Functor>
bool
UsdImagingInstanceAdapter::_RunForAllInstancesToDrawImpl(
    UsdPrim const& instancer, 
    std::vector<UsdPrim> *instanceContext,
    size_t* instanceIdx,
    Functor* fn)
{
    // NOTE: This logic is almost exactly similar to the logic in
    // _CountAllInstancesToDrawImpl. If you're updating this function, you
    // may need to update that function as well.

    const _InstancerData* instancerData = 
        TfMapLookupPtr(_instancerData, instancer.GetPath());
    if (not TF_VERIFY(instancerData)) {
        return false;
    }

    TF_FOR_ALL(pathIt, instancerData->instancePaths) {
        const UsdPrim instancePrim = _GetPrim(*pathIt);
        if (not TF_VERIFY(instancePrim, 
                "Invalid instance <%s> for master <%s>",
                pathIt->GetText(),
                instancerData->masterPath.GetText())) {
            break;
        }
        
        instanceContext->push_back(instancePrim);

        bool continueIteration = true;
        if (not instancePrim.IsInMaster()) {
            continueIteration = (*fn)(*instanceContext, (*instanceIdx)++);
        }
        else {
            // In this case, instancePrim is a descendent of a master prim.
            // Walk up the parent chain to find the master prim.
            UsdPrim parentMaster = instancePrim;
            while (not parentMaster.IsMaster()) {
                parentMaster = parentMaster.GetParent();
            }

            // Iterate over all instancers corresponding to different
            // shader variations of this master prim, since each instancer
            // will cause another copy of this master prim to be drawn.
            const _ShaderBindingToInstancerMap* bindingToInstancerMap =
                TfMapLookupPtr(_masterToInstancerMap, parentMaster.GetPath());
            if (TF_VERIFY(bindingToInstancerMap)) {
                for (const auto& i : *bindingToInstancerMap) {
                    const UsdPrim instancerForShader = _GetPrim(i.second);
                    if (TF_VERIFY(instancerForShader)) {
                        continueIteration = _RunForAllInstancesToDrawImpl(
                            instancerForShader, instanceContext, instanceIdx, 
                            fn);
                        if (not continueIteration) {
                            break;
                        }
                    }
                }
            }
        }

        instanceContext->pop_back();

        if (not continueIteration) {
            return false;
        }
    }

    return true;
}

size_t 
UsdImagingInstanceAdapter::_CountAllInstancesToDraw(
    UsdPrim const& instancer)
{
    // Memoized table of instancer path to the total number of 
    // times that instancer will be drawn.
    TfHashMap<SdfPath, size_t, SdfPath::Hash> numInstancesToDraw;
    return _CountAllInstancesToDrawImpl(instancer, &numInstancesToDraw);
}

size_t 
UsdImagingInstanceAdapter::_CountAllInstancesToDrawImpl(
    UsdPrim const& instancer,
    _InstancerDrawCounts* drawCounts)
{
    // NOTE: This logic is almost exactly similar to the logic in
    // _RunForAllInstancesToDrawImpl. If you're updating this function, you
    // may need to update that function as well.

    // See if we've already compute the total draw count for this instancer
    // in our memoized table.
    {
        _InstancerDrawCounts::const_iterator it = 
            drawCounts->find(instancer.GetPath());
        if (it != drawCounts->end()) {
            return it->second;
        }
    }

    // Otherwise, we have to compute it. Go find the instances for this
    // instancer.
    const _InstancerData* instancerData = 
        TfMapLookupPtr(_instancerData, instancer.GetPath());
    if (not TF_VERIFY(instancerData)) {
        return 0;
    }
    
    size_t drawCount = 0;

    TF_FOR_ALL(pathIt, instancerData->instancePaths) {
        const UsdPrim instancePrim = _GetPrim(*pathIt);
        if (not TF_VERIFY(instancePrim, 
                "Invalid instance <%s> for master <%s>",
                pathIt->GetText(),
                instancerData->masterPath.GetText())) {
            return 0;
        }

        if (not instancePrim.IsInMaster()) {
            ++drawCount;
        }
        else {
            UsdPrim parentMaster = instancePrim;
            while (not parentMaster.IsMaster()) {
                parentMaster = parentMaster.GetParent();
            }

            const _ShaderBindingToInstancerMap* bindingToInstancerMap =
                TfMapLookupPtr(_masterToInstancerMap, parentMaster.GetPath());
            if (TF_VERIFY(bindingToInstancerMap)) {
                for (const auto& i : *bindingToInstancerMap) {
                    const UsdPrim instancerForShader = _GetPrim(i.second);
                    if (TF_VERIFY(instancerForShader)) {
                        drawCount += _CountAllInstancesToDrawImpl(
                            instancerForShader, drawCounts);
                    }
                }
            }
        }
    }

    (*drawCounts)[instancer.GetPath()] = drawCount;
    return drawCount;
}

struct UsdImagingInstanceAdapter::_ComputeInstanceTransformFn
{
    _ComputeInstanceTransformFn(
        UsdImagingInstanceAdapter* adapter_, const UsdTimeCode& time_) 
        : adapter(adapter_), time(time_) 
    { }

    void Initialize(size_t numInstances)
    { 
        result.resize(numInstances);
    }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        if (not TF_VERIFY(instanceIdx < result.size())) {
            result.resize(instanceIdx + 1);
        }

        // Ignore root transform when computing each instance's transform
        // to avoid a double transformation when applying the instancer 
        // transform.
        static const bool ignoreRootTransform = true;

        GfMatrix4d xform(1.0);
        TF_FOR_ALL(instanceIt, instanceContext) {
            xform = xform 
                * adapter->GetTransform(*instanceIt, time, ignoreRootTransform);
        }

        result[instanceIdx] = xform;
        return true;
    }

    UsdImagingInstanceAdapter* adapter;
    UsdTimeCode time;
    VtMatrix4dArray result;
};

bool
UsdImagingInstanceAdapter::_ComputeInstanceTransform(
    UsdPrim const& instancer,
    VtMatrix4dArray* outTransforms,
    UsdTimeCode time)
{
    _ComputeInstanceTransformFn computeXform(this, time);
    _RunForAllInstancesToDraw(instancer, &computeXform);
    outTransforms->swap(computeXform.result);
    return true;
}

struct UsdImagingInstanceAdapter::_IsInstanceTransformVaryingFn
{
    _IsInstanceTransformVaryingFn(UsdImagingInstanceAdapter* adapter_)
        : adapter(adapter_), result(false) { }

    void Initialize(size_t numInstances)
    { }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        int dirtyBits;
        TF_FOR_ALL(primIt, instanceContext) {
            if (adapter->_IsTransformVarying(
                    *primIt, 
                    HdChangeTracker::DirtyTransform,
                    HdTokens->instancer,
                    &dirtyBits)) {
                result = true;
                break;
            }
        }
        return not result;
    }

    UsdImagingInstanceAdapter* adapter;
    bool result;
};

bool 
UsdImagingInstanceAdapter::_IsInstanceTransformVarying(UsdPrim const& instancer)
{
    _IsInstanceTransformVaryingFn isTransformVarying(this);
    _RunForAllInstancesToDraw(instancer, &isTransformVarying);
    return isTransformVarying.result;
}

void 
UsdImagingInstanceAdapter::UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath, 
                                   UsdTimeCode time,
                                   int requestedBits,
                                   UsdImagingInstancerContext const* 
                                       instancerContext)
{
    UsdImagingValueCache* valueCache = _GetValueCache();

    if (_IsChildPrim(prim, cachePath)) {
        // Note that the proto group in this rproto has not yet been
        // updated with new instances at this point.
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (not TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }

        // All Update code paths will update visibility.
        valueCache->GetVisible(cachePath);

        if (requestedBits & HdChangeTracker::DirtyInstanceIndex)
            valueCache->GetInstanceIndices(cachePath);
        if (requestedBits & HdChangeTracker::DirtyTransform)
            valueCache->GetInstancerTransform(cachePath);
        if (requestedBits & HdChangeTracker::DirtySurfaceShader)
            valueCache->GetSurfaceShader(cachePath);

        rproto.adapter->UpdateForTimePrep(
            _GetPrim(rproto.path), cachePath, time, requestedBits,
            &instancerContext);
    } else {
        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            valueCache->GetPrimvar(cachePath, HdTokens->instanceTransform);
            valueCache->GetPrimvars(cachePath);
        }
    }
}

void 
UsdImagingInstanceAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               int requestedBits,
                               int* resultBits,
                               UsdImagingInstancerContext const*)
{
    UsdImagingValueCache* valueCache = _GetValueCache();

    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (not TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        if (not TF_VERIFY(rproto.protoGroup, "%s", cachePath.GetText()))
            return;

        _UpdateInstanceMap(_GetPrim(instancerContext.instancerId), time);

        // Uphold the original precondition: result == requested bits.
        *resultBits = requestedBits;

        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            valueCache->GetInstanceIndices(cachePath) = 
                                          rproto.protoGroup->indices;
        }

        // Never pull visibility directly from the prototype, since we will
        // need to compute visibility relative to the model root anyway.
        // Similarly, the InstanceIndex was already updated, if needed.
        int protoReqBits = requestedBits 
                         & ~HdChangeTracker::DirtyInstanceIndex
                         & ~HdChangeTracker::DirtyVisibility;

        // Allow the prototype's adapter to update, if there's anything left
        // to do.
        if (protoReqBits != HdChangeTracker::Clean) {
            rproto.adapter->UpdateForTime(
                _GetPrim(rproto.path), cachePath, 
                rproto.protoGroup->time, protoReqBits, resultBits,
                &instancerContext);
        }

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
            bool protoHasFixedVis = not (rproto.variabilityBits 
                                        & HdChangeTracker::DirtyVisibility);
            if (protoHasFixedVis) { 
                // The proto prim has fixed visibility (it does not vary over time), 
                // we can use the pre-cached visibility.
                vis = rproto.visible;
            } 
            else {
                // The instancer is visible and the prototype has varying
                // visibility, we must compute visibility.
                vis = GetVisible(_GetPrim(instancerContext.instancerId), time);
            }
        }

        if (*resultBits & HdChangeTracker::DirtyTransform) {
            // Inverse out the root transform to avoid a double transformation
            // when applying the instancer transform.
            GfMatrix4d& childXf = _GetValueCache()->GetTransform(cachePath);
            childXf = childXf * GetRootTransform().GetInverse();
        }

        if (requestedBits & HdChangeTracker::DirtySurfaceShader) {
            // First try to get the shader binded in the instance, if no shader
            // is bound then access the shader bound to the master prim.
            SdfPath p = GetShaderBinding(prim);
            if (p.IsEmpty()) {
                p = GetShaderBinding(_GetPrim(rproto.path));
            }
            valueCache->GetSurfaceShader(cachePath) = p;
        }

    } else {
        // For the instancer itself, we only send the instance transforms
        // back as primvars, which falls into the DirtyPrimVar bucket
        // currently.
        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            VtMatrix4dArray instanceXforms;
            if (_ComputeInstanceTransform(prim, &instanceXforms, time)) {
                valueCache->GetPrimvar(
                    cachePath, HdTokens->instanceTransform) = instanceXforms;
                UsdImagingValueCache::PrimvarInfo primvar;
                primvar.name = HdTokens->instanceTransform;
                primvar.interpolation = _tokens->instance;
                _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
            }
        }

        // instancer transform can only be the root transform.
        if (requestedBits & HdChangeTracker::DirtyTransform) {
            _GetValueCache()->GetInstancerTransform(cachePath) =
                GetRootTransform();
        }
    }
}

int
UsdImagingInstanceAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    // Blast everything. This will trigger a prim resync; see ProcessPrimResync.
    return HdChangeTracker::AllDirty;
}

void
UsdImagingInstanceAdapter::ProcessPrimResync(SdfPath const& usdPath,
                                             UsdImagingIndexProxy* index)
{
    // If prim data exists at this path, we'll drop it now.
    _InstancerDataMap::iterator instIt = _instancerData.find(usdPath);
    if (instIt != _instancerData.end()) {
        // Nuke the entire instancer.
        _ReloadInstancer(usdPath, index);
        return;
    }

    // Either the prim was fundamentally modified or removed.
    // Regenerate instancer data if an instancer depends on the
    // resync'd prim. 
    SdfPathVector instancersToUnload;

    TF_FOR_ALL(instIt, _instancerData) {
        SdfPath const& instancerPath = instIt->first;
        _InstancerData& inst = instIt->second;

        // The resync'd prim is a dependency if it is a descendent of
        // the instancer master prim.
        if (usdPath.HasPrefix(inst.masterPath)) {
            instancersToUnload.push_back(instancerPath);
            continue;
        }

        // The resync'd prim is a dependency if it is an instance of
        // the instancer master prim.
        const SdfPathVector& instances = inst.instancePaths;
        if (std::binary_search(instances.begin(), instances.end(), usdPath)) {
            instancersToUnload.push_back(instancerPath);
            continue;
        }
    }

    // If there are nested instances beneath the instancer we're about to
    // reload, we need to reload the instancers for those instances as well,
    // and so on if those instancers also have nested instances.
    for (size_t i = 0; i < instancersToUnload.size(); ++i) {
        // Make sure to take a copy since we are intentionally mutating
        // the vector as we're iterating over it.
        const SdfPath instancerToUnload = instancersToUnload[i];
        TF_FOR_ALL(instIt, _instancerData) {
            const SdfPathVector& instances = instIt->second.instancePaths;
            const SdfPathVector::const_iterator pathIt = std::lower_bound(
                instances.begin(), instances.end(), instancerToUnload);
            if (pathIt != instances.end() and 
                pathIt->HasPrefix(instancerToUnload)) {
                // Since we use one of the Usd instances as the Hydra
                // instancer, we need to do this check to ensure we don't
                // add the same prim to instancersToUnload and wind up
                // in an infinite loop.
                if (*pathIt != instancerToUnload) {
                    instancersToUnload.push_back(instIt->first);
                }
            }
        }
    }

    TF_FOR_ALL(pathIt, instancersToUnload) {
        _ReloadInstancer(*pathIt, index);
    }
}

/*virtual*/
SdfPath
UsdImagingInstanceAdapter::GetInstancer(SdfPath const &cachePath)
{
    _InstanceToInstancerMap::iterator it
        = _instanceToInstancerMap.find(cachePath);
    if (it != _instanceToInstancerMap.end()) {
        return it->second;
    }
    return SdfPath();
}

bool
UsdImagingInstanceAdapter::ShouldCullChildren(UsdPrim const& prim)
{
    return true;
}

void
UsdImagingInstanceAdapter::_ReloadInstancer(SdfPath const& instancerPath,
                                            UsdImagingIndexProxy* index)
{
    _InstancerDataMap::iterator instIt = _instancerData.find(instancerPath);
    if (not TF_VERIFY(instIt != _instancerData.end())) {
        return;
    }

    // First, we need to make sure all proto rprims are removed.
    TF_FOR_ALL(primIt, instIt->second.primMap) {
        // Call ProcessRemoval here because we don't want them to reschedule for
        // resync, that will happen when the instancer is resync'd.
        primIt->second.adapter->ProcessPrimRemoval(primIt->first, index);
    }

    // Remove all dependencies on the instancer's instances, but keep a
    // copy of them around so we can repopulate them below.
    const SdfPathVector instancePaths = instIt->second.instancePaths;
    TF_FOR_ALL(instanceIt, instancePaths) {
        index->RemoveDependency(*instanceIt);
    }

    // Remove this instancer's entry from the master -> instancer map.
    auto shaderBindingIt = 
        _masterToInstancerMap.find(instIt->second.masterPath);
    if (TF_VERIFY(shaderBindingIt != _masterToInstancerMap.end())) {
        _ShaderBindingToInstancerMap& bindingMap = shaderBindingIt->second;
        auto instancerIt = bindingMap.find(instIt->second.shaderBindingPath);
        if (TF_VERIFY(instancerIt != bindingMap.end())) {
            bindingMap.erase(instancerIt);
        }

        if (bindingMap.empty()) {
            _masterToInstancerMap.erase(shaderBindingIt);
        }
    }

    // Blow away the instancer and the associated local data.
    index->RemoveInstancer(instancerPath);
    index->RemoveDependency(instancerPath);
    _instancerData.erase(instIt);

    // Repopulate the instancer's previous instances. Those that don't exist
    // anymore will be ignored, while those that still exist will be
    // pushed back into this adapter and refreshed.
    TF_FOR_ALL(pathIt, instancePaths) {
        if (_GetPrim(*pathIt) and _GetPrim(*pathIt).IsActive()) {
            index->Repopulate(*pathIt);
        }
    }
}

bool 
UsdImagingInstanceAdapter::_PrimIsInstancer(UsdPrim const& prim) const
{
    return _instancerData.find(prim.GetPath()) != _instancerData.end();
}

// -------------------------------------------------------------------------- //
// Private IO Helpers 
// -------------------------------------------------------------------------- //

UsdImagingInstanceAdapter::_ProtoRprim const&
UsdImagingInstanceAdapter::_GetProtoRprim(SdfPath const& instancerPath,
                                          SdfPath const& cachePath,
                                          UsdImagingInstancerContext* ctx)
{
    static _ProtoRprim const EMPTY;

    _InstancerDataMap::const_iterator it = _instancerData.find(instancerPath);
    _ProtoRprim const* r = nullptr;
    SdfPath instancerId;
    SdfPath shaderBinding;

    if (it != _instancerData.end()) {
        _PrimMap::const_iterator primIt = it->second.primMap.find(cachePath);
        if (primIt == it->second.primMap.end()) {
            return EMPTY;
        }
        instancerId = instancerPath;
        shaderBinding = it->second.shaderBindingPath;
        r = &(primIt->second);
    } else {
        // If we didn't find an instancerData entry, it's likely because the
        // prim is not nested under the instancer, which causes the
        // instancerPath to be invalid in this context.
        //
        // Tracking the non-child prims in a separate map would remove the need
        // for this loop.
        for (auto const& pathInstancerDataPair : _instancerData) {
            _InstancerData const& instancer = pathInstancerDataPair.second;
            _PrimMap::const_iterator protoIt =
                                    instancer.primMap.find(cachePath);
            if (protoIt != instancer.primMap.end()) {
                // This is the correct instancer path for this prim.
                instancerId = pathInstancerDataPair.first;
                shaderBinding = pathInstancerDataPair.second.shaderBindingPath;
                r = &protoIt->second;
                break;
            }
        }
    }
    if (not TF_VERIFY(r, "instancer = %s, cachePath = %s",
                      instancerPath.GetText(), cachePath.GetText())) {
        return EMPTY;
    }

    ctx->instancerId = instancerId;
    ctx->instanceSurfaceShaderPath = shaderBinding;
    ctx->childName = TfToken();
    ctx->instancerAdapter = _GetSharedFromThis();

    return *r;
}

UsdImagingPrimAdapterSharedPtr
UsdImagingInstanceAdapter::_GetSharedFromThis()
{
    boost::shared_ptr<class UsdImagingPrimAdapter> a = shared_from_this();
    return a;
}

struct UsdImagingInstanceAdapter::_UpdateInstanceMapFn
{
    _UpdateInstanceMapFn(
        UsdImagingInstanceAdapter* adapter_, const UsdTimeCode& time_,
        std::vector<_InstancerData::Visibility>* visibility_, 
        VtIntArray* indices_)
        : adapter(adapter_), time(time_), 
          visibility(visibility_), indices(indices_)
    { }

    void Initialize(size_t numInstances)
    {
        visibility->resize(numInstances, _InstancerData::Unknown);
    }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        if (not TF_VERIFY(instanceIdx < visibility->size())) {
            visibility->resize(instanceIdx + 1);
        }

        bool vis = true;
        _InstancerData::Visibility& instanceVis = (*visibility)[instanceIdx];
        if (instanceVis == _InstancerData::Unknown) {
            vis = GetVisible(instanceContext);
            if (IsVisibilityVarying(instanceContext)) {
                instanceVis = _InstancerData::Varying;
            }
            else {
                instanceVis = 
                    (vis ? _InstancerData::Visible : _InstancerData::Invisible);
            }
        }
        else if (instanceVis == _InstancerData::Varying) {
            vis = GetVisible(instanceContext);
        }
        else {
            vis = (instanceVis == _InstancerData::Visible);
        }
        
        if (vis) {
            indices->push_back(instanceIdx);
        }

        return true;
    }

    bool GetVisible(const std::vector<UsdPrim>& instanceContext)
    {
        TF_FOR_ALL(primIt, instanceContext) {
            if (not adapter->GetVisible(*primIt, time)) {
                return false;
            }
        }
        return true;
    }

    bool IsVisibilityVarying(const std::vector<UsdPrim>& instanceContext)
    {
        TF_FOR_ALL(primIt, instanceContext) {
            int dirtyBits;
            if (adapter->_IsVarying(*primIt, 
                           UsdGeomTokens->visibility, 
                           HdChangeTracker::DirtyVisibility,
                           UsdImagingTokens->usdVaryingVisibility,
                           &dirtyBits,
                           true)) {
                return true;
            }
        }
        return false;
    }

    UsdImagingInstanceAdapter* adapter;
    UsdTimeCode time;
    std::vector<_InstancerData::Visibility>* visibility;
    VtIntArray* indices;
};

void
UsdImagingInstanceAdapter::_UpdateInstanceMap(
                    UsdPrim const& instancerPrim, 
                    UsdTimeCode time)
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
        return;
    }
    _InstancerData& instrData = it->second;

    // It's tempting to scan through the protoGroup here and attempt to avoid
    // grabbing the lock, but it's not thread safe.
    std::lock_guard<std::mutex> lock(instrData.mutex);

    _ProtoGroupPtr& group = instrData.protoGroup;

    // Early exit if another thread already updated all the groups we care about
    // for the current time sample.
    bool allUpToDate = group->time == time;
    if (allUpToDate)
        return;

    group->indices.resize(0);
    group->time = time;

    _UpdateInstanceMapFn updateInstanceMapFn(
        this, time, &instrData.visibility, &group->indices);
    _RunForAllInstancesToDraw(instancerPrim, &updateInstanceMapFn); 
}

int
UsdImagingInstanceAdapter::_UpdateDirtyBits(
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
                "entry",
                instancerPrim.GetPath().GetText());
        return HdChangeTracker::Clean;
    }
    _InstancerData& instrData = it->second;

    // It's tempting to peek at the dirtyBits here and attempt to avoid grabbing
    // the lock, but it's not thread safe.
    std::lock_guard<std::mutex> lock(instrData.mutex);

    // If another thread already initialized the dirty bits, we can bail.
    if (instrData.dirtyBits != HdChangeTracker::AllDirty)
        return instrData.dirtyBits;

    instrData.numInstancesToDraw = _CountAllInstancesToDraw(instancerPrim);

    instrData.dirtyBits = HdChangeTracker::Clean;
    if (_IsInstanceTransformVarying(instancerPrim)) {
        instrData.dirtyBits |= HdChangeTracker::DirtyInstancer;
    }

    return instrData.dirtyBits;
}

struct UsdImagingInstanceAdapter::_GetPathForInstanceIndexFn
{
    _GetPathForInstanceIndexFn(
        UsdImagingInstanceAdapter* adapter_,
        SdfPath const &usdPath_,
        int instanceIndex_,
        SdfPathVector *instanceContextPaths_)
        : adapter(adapter_), usdPath(usdPath_), instanceIndex(instanceIndex_),
          instanceContextPaths(instanceContextPaths_)
    { }

    void Initialize(size_t numInstances)
    {
    }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        if (instanceIdx == static_cast<size_t>(instanceIndex) && instanceContext.size() > 0) {
            instancePath = instanceContext.back().GetPath();

            if (instanceContextPaths) {
                for (const UsdPrim &p: instanceContext) {
                    instanceContextPaths->push_back(p.GetPath());
                }
            }

            return false;
        }
        return true;
    }

    UsdImagingInstanceAdapter* adapter;
    SdfPath usdPath;
    SdfPath instancePath;
    int instanceIndex;
    SdfPathVector *instanceContextPaths;
};

/*virtual*/
SdfPath 
UsdImagingInstanceAdapter::GetPathForInstanceIndex(
    SdfPath const &path, int instanceIndex, int *instanceCount,
    int *absoluteInstanceIndex,
    SdfPath * rprimPath,
    SdfPathVector *instanceContext)
{
    UsdPrim const &prim = _GetPrim(path.GetAbsoluteRootOrPrimPath());
    if (not prim) {
        TF_CODING_ERROR("Invalid prim");
        return SdfPath();
    }

    if (prim.IsInstance() and not _PrimIsInstancer(prim)) {
        // This instance prim is handled by this adapter, but it's not
        // an instancer so the instance index doesn't apply.
        return SdfPath();
    }

    SdfPath instancerPath = path.GetPrimPath();
    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "NI: Look for %s [%d]\n", instancerPath.GetText(), instanceIndex);

    _InstancerDataMap::iterator instIt = _instancerData.find(instancerPath);
    if (instIt == _instancerData.end()) {
        // if it's not found, it may be an instance of other instancer.
        TF_FOR_ALL(instIt, _instancerData) {
            _InstancerData& inst = instIt->second;
            TF_FOR_ALL (i, inst.primMap) {
                if (i->first.GetPrimPath() == instancerPath) {
                    // found.
                    return GetPathForInstanceIndex(instIt->first,
                                                   instanceIndex,
                                                   instanceCount,
                                                   absoluteInstanceIndex,
                                                   rprimPath,
                                                   instanceContext);
                }
            }
        }
        TF_CODING_ERROR("Unknown instancer %s", instancerPath.GetText());
        return SdfPath();
    }

    // remap instanceIndex
    //
    // lookup instanceIndices to get the absolute index to
    // instancePaths.
    //
    // for example:
    //    if a prototype is instanced into 4 instances,
    //
    //         instanceIndices = [0, 1, 2, 3]
    //
    //    and say the second instance gets invised,
    //
    //         instanceIndices = [0, 2, 3]
    //
    // if we pick 3, this function takes instanceIndex = 2.
    // we need to map 2 back to 3 by instanceIndices[instanceIndex]
    //
    TF_FOR_ALL (it, instIt->second.primMap) {
        // pick the first proto
        VtIntArray const &instanceIndices = it->second.protoGroup->indices;
        if (not TF_VERIFY(static_cast<size_t>(instanceIndex) < instanceIndices.size())) {
            return SdfPath();
        }
        instanceIndex = instanceIndices[instanceIndex];
        break;
    }
        
    _GetPathForInstanceIndexFn getPathForInstanceIndexFn(
        this, instancerPath, instanceIndex, instanceContext);

    _RunForAllInstancesToDraw(prim, &getPathForInstanceIndexFn);

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "NI: Found %s\n", getPathForInstanceIndexFn.instancePath.GetText());

    // stop recursion, since we know instanceAdapter doesn't create an instacer
    // which has a parent instancer.
    // (actually it can be retrieved at the functor initialization if we like,
    //  for future extension)
    if (instanceCount) {
        *instanceCount = 0;
    }

    if (rprimPath) {
        const auto rprimPathIt = instIt->second.primMap.find(path);
        if (rprimPathIt != instIt->second.primMap.end()) {
            *rprimPath = rprimPathIt->second.path;

            TF_DEBUG(USDIMAGING_SELECTION).Msg(
                "NI: rprimPath %s\n", rprimPath->GetText());
        }
    }

    // intentionally leave absoluteInstanceIndex as it is, so that
    // partial selection of point instancer can be passed through.

    return getPathForInstanceIndexFn.instancePath;
}

struct UsdImagingInstanceAdapter::_PopulateInstanceSelectionFn
{
    _PopulateInstanceSelectionFn(
        UsdImagingInstanceAdapter* adapter_,
        SdfPath const &instancerPath_,
        SdfPath const &instancePath_,
        VtIntArray const &instanceIndices_,
        HdxSelectionSharedPtr const &result_)
        : adapter(adapter_)
        , instancerPath(instancerPath_)
        , instancePath(instancePath_)
        , instanceIndices(instanceIndices_)
        , result(result_)
        , found(false)
    {}

    void Initialize(size_t numInstances)
    {}

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        SdfPath path = instanceContext.back().GetPath();
        // we're only interested in the instanceContext which has instancePath
        if (path != instancePath) {
            return true;
            }

        const _InstancerData* instancerData = 
            TfMapLookupPtr(adapter->_instancerData, instancerPath);
        if (not TF_VERIFY(instancerData, "%s not found", instancerPath.GetText())) {
            return true;
        }

        // To highlight individual instances of NI-PI, compose instanceIndices
        VtIntArray niInstanceIndices;
        niInstanceIndices.reserve(instanceIndices.size()+1);
        TF_FOR_ALL(it, instanceIndices) {
            niInstanceIndices.push_back(*it);
        }
        niInstanceIndices.push_back((int)instanceIdx);

        // add all protos.
        TF_FOR_ALL (it, instancerData->primMap) {
            SdfPath protoRprim = it->first;
            // convert to indexPath (add prefix)
            SdfPath indexPath = adapter->_delegate->GetPathForIndex(it->first);

            // highlight all subtree with instanceIndices.
            // XXX: this seems redundant, but needed for point instancer highlighting for now.
            // ideally we should communicate back to point instancer adapter
            // to not use renderIndex
            SdfPathVector const &ids
                = adapter->_delegate->GetRenderIndex().GetRprimSubtree(indexPath);
            TF_FOR_ALL (protoIt, ids) {
                result->AddInstance(*protoIt, niInstanceIndices);

                TF_DEBUG(USDIMAGING_SELECTION).Msg(
                    "PopulateSelection: (instance) %s - %s : %ld\n",
                    indexPath.GetText(), protoIt->GetText(), instanceIdx);
            }


            found = true;
        }
        return true;
    }

    UsdImagingInstanceAdapter* adapter;
    SdfPath instancerPath;
    SdfPath instancePath;
    VtIntArray instanceIndices;
    HdxSelectionSharedPtr result;
    bool found;
};

/*virtual*/
bool
UsdImagingInstanceAdapter::PopulateSelection(
    SdfPath const &instancePath,
    VtIntArray const &instanceIndices,
    HdxSelectionSharedPtr const &result)
{
    HD_TRACE_FUNCTION();

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "PopulateSelection: instance = %s\n", instancePath.GetText());

    // look for instancePath
    //
    // XXX: do we still need to iterate over all instancer?
    //
    bool found = false;
    TF_FOR_ALL (it, _instancerData) {
        _PopulateInstanceSelectionFn populateFn(
            this, it->first, instancePath, instanceIndices, result);

        _RunForAllInstancesToDraw(_GetPrim(it->first), &populateFn);

        found |= populateFn.found;
    }

    return found;
}

/*virtual*/
SdfPathVector
UsdImagingInstanceAdapter::GetDependPaths(SdfPath const &instancerPath) const
{
    _InstancerDataMap::const_iterator it = _instancerData.find(instancerPath);

    SdfPathVector result;
    if (it != _instancerData.end()) {
        _InstancerData const & instancerData = it->second;

        // if the proto path is property path, that should be in the subtree
        // and no need to return.
        TF_FOR_ALL (protoIt, instancerData.primMap) {
            SdfPath const &protoPath = protoIt->first;
            if (protoPath.IsPrimOrPrimVariantSelectionPath()) {
                if (not protoPath.HasPrefix(instancerPath)) {
                    result.push_back(protoPath);
                }
            }
        }
    }
    // XXX: we may want to cache this result in _instancerData.
    return result;
}


