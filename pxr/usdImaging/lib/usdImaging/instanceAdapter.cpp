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
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/imageable.h"

#include "pxr/base/gf/matrix4f.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE


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
    return _Populate(prim, index, instancerContext,
                     SdfPath::AbsoluteRootPath());
}

SdfPath
UsdImagingInstanceAdapter::_Populate(UsdPrim const& prim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext,
                            SdfPath const& parentProxyPath)
{
    TF_DEBUG(USDIMAGING_INSTANCER).Msg(
        "[Populate NI] prim=%s\n", prim.GetPath().GetText());

    SdfPath instancePath = prim.GetPath();
    if (!TF_VERIFY(
            prim.IsInstance(), 
            "Cannot populate instance adapter from <%s>, "
            "which is not an instance prim.",
            instancePath.GetString().c_str())) {
        return SdfPath();
    }

    const UsdPrim& masterPrim = prim.GetMaster();
    if (!TF_VERIFY(
            masterPrim, "Cannot get master prim for <%s>",
            instancePath.GetString().c_str())) {
        return SdfPath();
    }

    // This is a shared_ptr to ourself. The InstancerContext requires the
    // adapter shared_ptr.
    UsdImagingPrimAdapterSharedPtr const& instancerAdapter = 
                        _GetSharedFromThis();

    const SdfPath& instanceMaterialId = instancerAdapter->GetMaterialId(prim);

    // Construct the instance proxy path for "instancePath" to look up the
    // draw mode for this instance.  If this is a nested instance (meaning
    // "prim" is part of a master), parentProxyPath contains the instance
    // proxy path for the master we're currently in, so we can stitch the full
    // proxy path together.
    SdfPathVector instancerChain;
    instancerChain = { instancePath };
    if (prim.IsInMaster()) {
        instancerChain.push_back(parentProxyPath);
    }
    const TfToken& instanceDrawMode = GetModelDrawMode(_GetPrim(
        _GetPrimPathFromInstancerChain(instancerChain)));

    // Check if there's an instance in use as a hydra instancer for this master
    // with the appropriate inherited attributes.
    SdfPath instancerPath;
    auto range = _masterToInstancerMap.equal_range(masterPrim.GetPath());
    for (auto it = range.first; it != range.second; ++it) {
        _InstancerData& instancerData = _instancerData[it->second];
        // If material ID or draw mode differ, split the instance.
        if (instancerData.materialId == instanceMaterialId &&
            instancerData.drawMode == instanceDrawMode) {
            instancerPath = it->second;
            break;
        }
    }

    // If we didn't find a suitable hydra instancer for this master,
    // add a new one.
    if (instancerPath.IsEmpty()) {
        _masterToInstancerMap.insert(
            std::make_pair(masterPrim.GetPath(), instancePath));
        instancerPath = instancePath;
    }

    _InstancerData& instancerData = _instancerData[instancerPath];
    instancerData.dirtyBits = HdChangeTracker::AllDirty;

    // Compute the instancer proxy path (which might be different than the
    // one computed above, if instancePath and instancerPath are different...)
    instancerChain = { instancerPath };
    if (_GetPrim(instancerPath).IsInMaster()) {
        instancerChain.push_back(parentProxyPath);
    }
    SdfPath instancerProxyPath = _GetPrimPathFromInstancerChain(instancerChain);

    std::vector<UsdPrim> nestedInstances;

    if(instancerData.instancePaths.empty()) {
        instancerData.masterPath = masterPrim.GetPath();
        instancerData.materialId = instanceMaterialId;
        instancerData.drawMode = instanceDrawMode;

        // Add this instancer into the render index.
        UsdImagingInstancerContext ctx = { SdfPath(),
                                           TfToken(),
                                           SdfPath(),
                                           TfToken(),
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

        UsdPrimRange range(masterPrim);
        int protoID = 0;
        int primCount = 0;

        for (auto iter = range.begin(); iter != range.end(); ++iter) {
            // If we encounter an instance in this master, save it aside
            // for a subsequent population pass since we'll need to populate
            // its master once we're done with this one.
            if (iter->IsInstance()) {
                nestedInstances.push_back(*iter);
                continue;
            }

            // Stitch the current prim-in-master path to the instancer proxy
            // path to get a full scene-scoped path that we can pass to
            // _GetPrimAdapter (since _GetPrimAdapter needs the instance proxy
            // path for inherited value resolution).
            instancerChain = { iter->GetPath(), instancerProxyPath };
            UsdPrim instanceProxyPrim =
                _GetPrim(_GetPrimPathFromInstancerChain(instancerChain));
            if (!instanceProxyPrim) {
                continue;
            }

            UsdImagingPrimAdapterSharedPtr const& primAdapter =
                _GetPrimAdapter(instanceProxyPrim, /*ignoreInstancing=*/ true);
            if (!primAdapter || primAdapter->IsPopulatedIndirectly()) {
                continue;
            }

            // If we're processing the master prim, it's normally not allowed
            // to be imageable.  We can't instance a gprim (or instancer)
            // directly since we don't derive any scalability benefit from
            // mesh-to-mesh instancing.
            //
            // Exceptions (like cards mode) will be flagged by the function
            // CanPopulateMaster() on their prim adapter.
            //
            // If the master prim has an adapter and shouldn't, generate a
            // warning and continue.
            if (iter->IsMaster() && primAdapter &&
                !primAdapter->CanPopulateMaster()) {
                TF_WARN("The gprim at path <%s> was directly instanced. "
                        "In order to instance this prim, put the prim under an "
                        "Xform, and instance the Xform parent.",
                        iter->GetPath().GetText());
                continue;
            }
                
            // 
            // prototype allocation. 
            //
            const TfToken protoName(TfStringPrintf(
                "proto_%s_id%d", iter->GetName().GetText(), protoID++));
            bool isLeafInstancer;

            // Inherited attribute resolution...
            SdfPath protoMaterialId = primAdapter->GetMaterialId(prim);
            if (protoMaterialId.IsEmpty()) {
                protoMaterialId = instanceMaterialId;
            }
            TfToken protoDrawMode = GetModelDrawMode(instanceProxyPrim);

            const SdfPath protoPath = 
                _InsertProtoRprim(&iter, protoName, protoMaterialId,
                                  protoDrawMode, instancerPath,
                                  primAdapter, instancerAdapter, index,
                                  &isLeafInstancer);
                    
            // 
            // Update instancer data.
            //
            _ProtoRprim& rproto = instancerData.primMap[protoPath];
            if (iter->IsMaster()) {
                // If the prototype we're populating is the master prim,
                // our prim handle should be to the instance, since the master
                // prim doesn't have attributes.
                rproto.path = instancerPath;
            } else {
                rproto.path = iter->GetPath();
            }
            rproto.adapter = primAdapter;
            rproto.protoGroup = grp;
            ++primCount;

            if (!isLeafInstancer) {
                instancerData.childInstancers.insert(protoPath);
            }

            TF_DEBUG(USDIMAGING_INSTANCER).Msg(
                "[Add Instance NI] <%s>  %s (%s), adapter = %s\n",
                instancerPath.GetText(), protoPath.GetText(),
                iter->GetName().GetText(),
                primAdapter ?
                    TfType::GetCanonicalTypeName(typeid(*primAdapter)).c_str() :
                    "none");
        }
        if (primCount > 0) {
            index->InsertInstancer(instancerPath,
                                   /*parentPath=*/ctx.instancerId,
                                   _GetPrim(instancerPath),
                                   ctx.instancerAdapter);

            // Ensure that the instance transforms are computed on the first
            // call to UpdateForTime.
            index->MarkInstancerDirty(instancerPath,
                HdChangeTracker::DirtyPrimVar);
        } else if (nestedInstances.empty()) {
            // if this instance path ends up to have no prims in subtree
            // and not an instance itself , we don't need to track this path
            // any more.

            instancePath = SdfPath();
        }
    }

    if (!instancePath.IsEmpty()) {
        // Add an entry to the instancer data for the given instance. Keep
        // the vector sorted for faster lookups during change processing.
        std::vector<SdfPath>& instancePaths = instancerData.instancePaths;
        std::vector<SdfPath>::iterator it = std::lower_bound(
            instancePaths.begin(), instancePaths.end(), instancePath);

        // We may repopulate instances we've already seen during change
        // processing when nested instances are involved. Rather than do
        // some complicated filtering in ProcessPrimResync to avoid this,
        // we just silently ignore duplicate instances here.
        if (it == instancePaths.end() || *it != instancePath) {
            instancePaths.insert(it, instancePath);

            TF_DEBUG(USDIMAGING_INSTANCER).Msg(
                "[Add Instance NI] <%s>  %s\n",
                instancerPath.GetText(), instancePath.GetText());

            _instanceToInstancerMap[instancePath] = instancerPath;

            // Make sure we add a dependency for this instance on this adapter,
            // so that changes to the instance are handled properly.
            index->AddPrimInfo(instancePath,
                               _GetPrim(instancePath),
                               instancerAdapter);

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
        _Populate(*nestedInstanceIt, index, instancerContext,
                  instancerProxyPath);
    }

    return instancerPath;
}

SdfPath
UsdImagingInstanceAdapter::_InsertProtoRprim(
    UsdPrimRange::iterator *it,
    TfToken const& protoName,
    SdfPath materialId,
    TfToken drawMode,
    SdfPath instancerPath,
    UsdImagingPrimAdapterSharedPtr const& primAdapter,
    UsdImagingPrimAdapterSharedPtr const& instancerAdapter,
    UsdImagingIndexProxy* index,
    bool *isLeafInstancer)
{
    UsdPrim prim = **it;
    if ((*it)->IsMaster()) {
        // If the prototype we're populating is the master prim,
        // our prim handle should be to the instance, since the master
        // prim doesn't have attributes.
        prim = _GetPrim(instancerPath);
    }

    UsdImagingInstancerContext ctx = { instancerPath,
                                       protoName,
                                       materialId,
                                       drawMode,
                                       instancerAdapter };

    SdfPath protoPath = primAdapter->Populate(prim, index, &ctx);

    if (primAdapter->ShouldCullChildren(prim)) {
        it->PruneChildren();
    }

    *isLeafInstancer = !(primAdapter->IsInstancerAdapter());
    return protoPath;
}

bool
UsdImagingInstanceAdapter::_IsChildPrim(UsdPrim const& prim, 
                                        SdfPath const& cachePath) const
{
    // Child paths are the instancer path with a property appended. For leaf
    // gprims (mesh, points, etc) we use child paths, but for adapters which
    // prune children (and therefore likely want to manage their namespace), we
    // use the prim path of the original prim, thus IsChildPath fails.
    //
    // We can distinguish between child prims (e.g. children in primMap, from
    // recursive populate calls) and instances (in _instanceToInstancerMap
    // and the primInfo table) by checking whether the prim shows up in
    // _instanceToInstancerMap.  If it's not there, it must be a child
    // prim which we did not relocate during population.
    return IsChildPath(cachePath) 
        // We could make this less ad-hoc by storing a list of valid non-child
        // paths, in exchange for the overhead of maintaining that list.
        || nullptr == TfMapLookupPtr(_instanceToInstancerMap, prim.GetPath());
}

void 
UsdImagingInstanceAdapter::TrackVariabilityPrep(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingInstancerContext const* 
                                          instancerContext)
{
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        rproto.adapter->TrackVariabilityPrep(
            _GetPrim(rproto.path), cachePath, 
            &instancerContext);
    } 
}

void 
UsdImagingInstanceAdapter::TrackVariability(UsdPrim const& prim,
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

    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim & rproto = const_cast<_ProtoRprim&>(
                                     _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext));
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }

        // If requested, we will always mark indices dirty and update them
        // lazily.
        *timeVaryingBits |= HdChangeTracker::DirtyInstanceIndex;
        // Initializing to an empty value is OK here because either this
        // prototype will be invisible or it will be visible and the indices
        // will be updated.
        VtIntArray a;
        valueCache->GetInstanceIndices(cachePath) = a;
        
        UsdPrim protoPrim = _GetPrim(rproto.path);
        rproto.adapter->TrackVariability(
            protoPrim, cachePath, &rproto.variabilityBits,
            &instancerContext);
        *timeVaryingBits |= rproto.variabilityBits;

        if (!(rproto.variabilityBits & HdChangeTracker::DirtyVisibility)) {
            // Pre-cache visibility, because we now know that it is static for
            // the rprim prototype over all time.
            rproto.visible = GetVisible(protoPrim, time);
        }

        // If any of the instances varies over time, we should flag the 
        // DirtyInstancer bits on the Rprim on every frame, to be sure the 
        // instancer data associated with the Rprim gets updated.
        int instancerBits = _UpdateDirtyBits(
                _GetPrim(instancerContext.instancerId));
        *timeVaryingBits |=  (instancerBits & HdChangeTracker::DirtyInstancer);
        *timeVaryingBits |= HdChangeTracker::DirtyVisibility;

    } else if (TfMapLookupPtr(_instancerData, prim.GetPath()) != nullptr) {
        // In this case, prim is an instance master. Master prims provide
        // no data of their own, so we fall back to the default purpose.
        valueCache->GetPurpose(cachePath) = UsdGeomTokens->default_;

        int instancerBits = _UpdateDirtyBits(prim);

        // If any of the instance transforms vary over time, the
        // instancer will have the DirtyInstancer bit set. Translate
        // that to DirtyPrimVar so that Hd will note that the
        // instance transform primvar is varying over time.
        if (instancerBits & HdChangeTracker::DirtyInstancer) {
            *timeVaryingBits |= HdChangeTracker::DirtyPrimVar;
        }
    }
}

template <typename Functor>
void
UsdImagingInstanceAdapter::_RunForAllInstancesToDraw(
    UsdPrim const& instancer,
    Functor* fn) const
{
    const _InstancerData* instancerData = 
        TfMapLookupPtr(_instancerData, instancer.GetPath());
    if (!TF_VERIFY(instancerData, "Can't find instancerData for %s",
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
    Functor* fn) const
{
    // NOTE: This logic is almost exactly similar to the logic in
    // _CountAllInstancesToDrawImpl. If you're updating this function, you
    // may need to update that function as well.

    const _InstancerData* instancerData = 
        TfMapLookupPtr(_instancerData, instancer.GetPath());
    if (!TF_VERIFY(instancerData)) {
        return false;
    }

    TF_FOR_ALL(pathIt, instancerData->instancePaths) {
        const UsdPrim instancePrim = _GetPrim(*pathIt);
        if (!TF_VERIFY(instancePrim, 
                "Invalid instance <%s> for master <%s>",
                pathIt->GetText(),
                instancerData->masterPath.GetText())) {
            break;
        }
        
        instanceContext->push_back(instancePrim);

        bool continueIteration = true;
        if (!instancePrim.IsInMaster()) {
            continueIteration = (*fn)(*instanceContext, (*instanceIdx)++);
        }
        else {
            // In this case, instancePrim is a descendent of a master prim.
            // Walk up the parent chain to find the master prim.
            UsdPrim parentMaster = instancePrim;
            while (!parentMaster.IsMaster()) {
                parentMaster = parentMaster.GetParent();
            }

            // Iterate over all instancers corresponding to different
            // variations of this master prim, since each instancer
            // will cause another copy of this master prim to be drawn.
            auto range =
                _masterToInstancerMap.equal_range(parentMaster.GetPath());
            for (auto it = range.first; it != range.second; ++it) {
                const UsdPrim instancer = _GetPrim(it->second);
                if (TF_VERIFY(instancer)) {
                    continueIteration = _RunForAllInstancesToDrawImpl(
                        instancer, instanceContext, instanceIdx, fn);
                    if (!continueIteration) {
                        break;
                    }
                }
            }
        }

        instanceContext->pop_back();

        if (!continueIteration) {
            return false;
        }
    }

    return true;
}

size_t 
UsdImagingInstanceAdapter::_CountAllInstancesToDraw(
    UsdPrim const& instancer) const
{
    // Memoized table of instancer path to the total number of 
    // times that instancer will be drawn.
    TfHashMap<SdfPath, size_t, SdfPath::Hash> numInstancesToDraw;
    return _CountAllInstancesToDrawImpl(instancer, &numInstancesToDraw);
}

size_t 
UsdImagingInstanceAdapter::_CountAllInstancesToDrawImpl(
    UsdPrim const& instancer,
    _InstancerDrawCounts* drawCounts) const
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
    if (!TF_VERIFY(instancerData)) {
        return 0;
    }
    
    size_t drawCount = 0;

    TF_FOR_ALL(pathIt, instancerData->instancePaths) {
        const UsdPrim instancePrim = _GetPrim(*pathIt);
        if (!TF_VERIFY(instancePrim, 
                "Invalid instance <%s> for master <%s>",
                pathIt->GetText(),
                instancerData->masterPath.GetText())) {
            return 0;
        }

        if (!instancePrim.IsInMaster()) {
            ++drawCount;
        }
        else {
            UsdPrim parentMaster = instancePrim;
            while (!parentMaster.IsMaster()) {
                parentMaster = parentMaster.GetParent();
            }

            auto range =
                _masterToInstancerMap.equal_range(parentMaster.GetPath());
            for (auto it = range.first; it != range.second; ++it) {
                const UsdPrim instancer = _GetPrim(it->second);
                if (TF_VERIFY(instancer)) {
                    drawCount += _CountAllInstancesToDrawImpl(
                        instancer, drawCounts);
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
        const UsdImagingInstanceAdapter* adapter_, const UsdTimeCode& time_) 
        : adapter(adapter_), time(time_) 
    { }

    void Initialize(size_t numInstances)
    { 
        result.resize(numInstances);
    }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        if (!TF_VERIFY(instanceIdx < result.size())) {
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

    const UsdImagingInstanceAdapter* adapter;
    UsdTimeCode time;
    VtMatrix4dArray result;
};

bool
UsdImagingInstanceAdapter::_ComputeInstanceTransforms(
    UsdPrim const& instancer,
    VtMatrix4dArray* outTransforms,
    UsdTimeCode time) const
{
    _ComputeInstanceTransformFn computeXform(this, time);
    _RunForAllInstancesToDraw(instancer, &computeXform);
    outTransforms->swap(computeXform.result);
    return true;
}

struct UsdImagingInstanceAdapter::_IsInstanceTransformVaryingFn
{
    _IsInstanceTransformVaryingFn(const UsdImagingInstanceAdapter* adapter_)
        : adapter(adapter_), result(false) { }

    void Initialize(size_t numInstances)
    { }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        TF_FOR_ALL(primIt, instanceContext) {
            if (_GetIsTransformVarying(*primIt)) {
                result = true;
                break;
            }
        }
        return !result;
    }

    bool _GetIsTransformVarying(const UsdPrim& prim) {
        bool transformVarying;
        HdDirtyBits dirtyBits;

        // Cache any _IsTransformVarying calls.
        auto cacheIt = cache.find(prim);
        if (cacheIt == cache.end()) {
            transformVarying = adapter->_IsTransformVarying(
                prim,
                HdChangeTracker::DirtyTransform,
                HdTokens->instancer,
                &dirtyBits);
            cache[prim] = transformVarying;
        } else {
            transformVarying = cacheIt->second;
        }

        return transformVarying;
    }

    const UsdImagingInstanceAdapter* adapter;
    bool result;

    // We keep a simple cache directly on _IsInstanceTransformVaryingFn because
    // we only need it during initialization and resyncs (not in UpdateForTime).
    boost::unordered_map<UsdPrim, bool, boost::hash<UsdPrim>> cache;
};

bool 
UsdImagingInstanceAdapter::_IsInstanceTransformVarying(UsdPrim const& instancer)
    const 
{
    _IsInstanceTransformVaryingFn isTransformVarying(this);
    _RunForAllInstancesToDraw(instancer, &isTransformVarying);
    return isTransformVarying.result;
}

void 
UsdImagingInstanceAdapter::UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath, 
                                   UsdTimeCode time,
                                   HdDirtyBits requestedBits,
                                   UsdImagingInstancerContext const* 
                                       instancerContext)
{
    if (_IsChildPrim(prim, cachePath)) {
        // Note that the proto group in this rproto has not yet been
        // updated with new instances at this point.
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        rproto.adapter->UpdateForTimePrep(
            _GetPrim(rproto.path), cachePath, time, requestedBits,
            &instancerContext);
    }
}

void 
UsdImagingInstanceAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const*) const
{
    UsdImagingValueCache* valueCache = _GetValueCache();

    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return;
        }
        if (!TF_VERIFY(rproto.protoGroup, "%s", cachePath.GetText()))
            return;

        _UpdateInstanceMap(_GetPrim(instancerContext.instancerId), time);

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
        UsdPrim protoPrim = _GetPrim(rproto.path);

        if (protoReqBits != HdChangeTracker::Clean) {
            rproto.adapter->UpdateForTime(protoPrim, cachePath, 
                rproto.protoGroup->time, protoReqBits, &instancerContext);
        }

        // Make sure we always query and return visibility. This is done
        // after the adapter update to ensure we get our specialized view of
        // visibility.

        // Apply the instancer visibility at the current time to the
        // instance. Notice that the instance will also pickup the instancer
        // visibility at the time offset.
        bool& vis = valueCache->GetVisible(cachePath);
        bool protoHasFixedVis = !(rproto.variabilityBits
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

        if (requestedBits & HdChangeTracker::DirtyTransform) {
            GfMatrix4d& childXf = _GetValueCache()->GetTransform(cachePath);
            if (protoPrim.IsInstance()) {
                // If the prototype we're processing is a master, protoPrim is
                // a pointer to the instance for attribute lookup; but the
                // instance transform for that instance is already part of the
                // instanceTransform primvar.  Masters don't have any transform,
                // aside from the root transform, so we can set the rprim
                // transform to identity.
                childXf.SetIdentity();
            } else {
                // Inverse out the root transform to avoid a double
                // transformation when applying the instancer transform.
                childXf = childXf * GetRootTransform().GetInverse();
            }
        }

    } else if (TfMapLookupPtr(_instancerData, prim.GetPath()) != nullptr) {
        // For the instancer itself, we only send the instance transforms
        // back as primvars, which falls into the DirtyPrimVar bucket
        // currently.
        if (requestedBits & HdChangeTracker::DirtyPrimVar) {
            VtMatrix4dArray instanceXforms;
            if (_ComputeInstanceTransforms(prim, &instanceXforms, time)) {
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

HdDirtyBits
UsdImagingInstanceAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    // Blast everything. This will trigger a prim resync; see ProcessPrimResync.
    return HdChangeTracker::AllDirty;
}

void
UsdImagingInstanceAdapter::_ResyncPath(SdfPath const& usdPath,
                                       UsdImagingIndexProxy* index,
                                       bool reload)
{
    // If prim data exists at this path, we'll drop it now.
    _InstancerDataMap::iterator instIt = _instancerData.find(usdPath);
    if (instIt != _instancerData.end()) {
        // Nuke the entire instancer.
        _ResyncInstancer(usdPath, index, reload);
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
            if (pathIt != instances.end() &&
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
        _ResyncInstancer(*pathIt, index, reload);
    }
}

void UsdImagingInstanceAdapter::ProcessPrimResync(SdfPath const& primPath,
                                                  UsdImagingIndexProxy* index)
{
    _ResyncPath(primPath, index, /*reload=*/true);
}

void UsdImagingInstanceAdapter::ProcessPrimRemoval(SdfPath const& primPath,
                                                   UsdImagingIndexProxy* index)
{
    _ResyncPath(primPath, index, /*reload=*/false);
}

void
UsdImagingInstanceAdapter::MarkDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     HdDirtyBits dirty,
                                     UsdImagingIndexProxy* index)
{
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            rproto.adapter->MarkDirty(prim, cachePath, dirty, index);
        }
    } else if (TfMapLookupPtr(_instancerData, prim.GetPath()) != nullptr) {
        index->MarkInstancerDirty(cachePath, dirty);
    }
}

void
UsdImagingInstanceAdapter::MarkRefineLevelDirty(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         UsdImagingIndexProxy* index)
{
    // The instancer isn't interested in this, but it's children maybe
    // so make sure the message gets forwarded
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            rproto.adapter->MarkRefineLevelDirty(prim, cachePath, index);
        }
    }
}

void
UsdImagingInstanceAdapter::MarkReprDirty(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  UsdImagingIndexProxy* index)
{
    // The instancer isn't interested in this, but it's children maybe
    // so make sure the message gets forwarded
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            rproto.adapter->MarkReprDirty(prim, cachePath, index);
        }
    }
}

void
UsdImagingInstanceAdapter::MarkCullStyleDirty(UsdPrim const& prim,
                                       SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index)
{
    // The instancer isn't interested in this, but it's children maybe
    // so make sure the message gets forwarded
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            rproto.adapter->MarkCullStyleDirty(prim, cachePath, index);
        }
    }
}

void
UsdImagingInstanceAdapter::MarkTransformDirty(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              UsdImagingIndexProxy* index)
{
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            rproto.adapter->MarkTransformDirty(prim, cachePath, index);
        }
    } else if (TfMapLookupPtr(_instancerData, prim.GetPath()) != nullptr) {
        // For the instancer itself, the instance transforms are sent back
        // as primvars, so we need to augment the DirtyTransform bit with
        // DirtyPrimVar.
        static const HdDirtyBits transformDirty =
                                                HdChangeTracker::DirtyPrimVar  |
                                                HdChangeTracker::DirtyTransform;

        index->MarkInstancerDirty(cachePath, transformDirty);
    }
}

void
UsdImagingInstanceAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               UsdImagingIndexProxy* index)
{
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            rproto.adapter->MarkVisibilityDirty(prim, cachePath, index);
        }
    } else if (TfMapLookupPtr(_instancerData, prim.GetPath()) != nullptr) {
        static const HdDirtyBits visibilityDirty =
                                               HdChangeTracker::DirtyVisibility;

        index->MarkInstancerDirty(cachePath, visibilityDirty);
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

/*virtual*/
size_t
UsdImagingInstanceAdapter::SampleInstancerTransform(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerPath,
    UsdTimeCode time,
    const std::vector<float>& configuredSampleTimes,
    size_t maxSampleCount,
    float *times,
    GfMatrix4d *samples)
{
    // This code must match UpdateForTime(), which says:
    // the instancer transform can only be the root transform.
    if (maxSampleCount > 0) {
        times[0] = 0.0;
        samples[0] = GetRootTransform();
        return 1;
    }
    return 0;
}

size_t
UsdImagingInstanceAdapter::SamplePrimvar(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time, const std::vector<float>& configuredSampleTimes,
    size_t maxNumSamples, float *times, VtValue *samples)
{
    if (_IsChildPrim(usdPrim, cachePath)) {
        // Note that the proto group in this rproto has not yet been
        // updated with new instances at this point.
        UsdImagingInstancerContext instancerContext;
        _ProtoRprim const& rproto = _GetProtoRprim(usdPrim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (!TF_VERIFY(rproto.adapter, "%s", cachePath.GetText())) {
            return 0;
        }
        return rproto.adapter->SamplePrimvar(
            _GetPrim(rproto.path), cachePath,
            key, time, configuredSampleTimes, maxNumSamples, times, samples);
    }
    if (key == HdTokens->instanceTransform) {
        size_t numSamples = std::min(maxNumSamples,
                                     configuredSampleTimes.size());
        for (size_t i=0; i < numSamples; ++i) {
            UsdTimeCode sceneTime =
                _GetTimeWithOffset(configuredSampleTimes[i]);
            times[i] = configuredSampleTimes[i];
            VtMatrix4dArray xf;
            _ComputeInstanceTransforms(usdPrim, &xf, sceneTime);
            samples[i] = xf;
        }
        return numSamples;
    } else {
        return UsdImagingPrimAdapter::SamplePrimvar(
            usdPrim, cachePath, key, time, configuredSampleTimes,
            maxNumSamples, times, samples);
    }
}

void
UsdImagingInstanceAdapter::_ResyncInstancer(SdfPath const& instancerPath,
                                            UsdImagingIndexProxy* index,
                                            bool repopulate)
{
    _InstancerDataMap::iterator instIt = _instancerData.find(instancerPath);
    if (!TF_VERIFY(instIt != _instancerData.end())) {
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
        index->RemovePrimInfo(*instanceIt);
    }

    // Remove this instancer's entry from the master -> instancer map.
    auto range = _masterToInstancerMap.equal_range(instIt->second.masterPath);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == instancerPath) {
            _masterToInstancerMap.erase(it);
            break;
        }
    }

    // Blow away the instancer and the associated local data.
    index->RemoveInstancer(instancerPath);
    index->RemovePrimInfo(instancerPath);
    _instancerData.erase(instIt);

    // Repopulate the instancer's previous instances. Those that don't exist
    // anymore will be ignored, while those that still exist will be
    // pushed back into this adapter and refreshed.
    if (repopulate) {
        TF_FOR_ALL(pathIt, instancePaths) {
            if (_GetPrim(*pathIt) && _GetPrim(*pathIt).IsActive()) {
                index->Repopulate(*pathIt);
            }
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
                                          UsdImagingInstancerContext* ctx) const
{
    static _ProtoRprim const EMPTY;

    _InstancerDataMap::const_iterator it = _instancerData.find(instancerPath);
    _ProtoRprim const* r = nullptr;
    SdfPath instancerId;
    SdfPath materialId;
    TfToken drawMode;

    if (it != _instancerData.end()) {
        _PrimMap::const_iterator primIt = it->second.primMap.find(cachePath);
        if (primIt == it->second.primMap.end()) {
            return EMPTY;
        }
        instancerId = instancerPath;
        materialId = it->second.materialId;
        drawMode = it->second.drawMode;
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
                materialId = 
                    pathInstancerDataPair.second.materialId;
                drawMode =
                    pathInstancerDataPair.second.drawMode;
                r = &protoIt->second;
                break;
            }
        }
    }
    if (!TF_VERIFY(r, "instancer = %s, cachePath = %s",
                      instancerPath.GetText(), cachePath.GetText())) {
        return EMPTY;
    }

    ctx->instancerId = instancerId;
    ctx->instanceMaterialId = materialId;
    ctx->instanceDrawMode = drawMode;
    ctx->childName = TfToken();
    // Note: use a null adapter here.  The UsdImagingInstancerContext is
    // not really used outside of population.  We should clean this up and
    // remove these contexts from everything outside of population.
    ctx->instancerAdapter = UsdImagingPrimAdapterSharedPtr();

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
        const UsdImagingInstanceAdapter* adapter_, const UsdTimeCode& time_,
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
        if (!TF_VERIFY(instanceIdx < visibility->size())) {
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
            if (!adapter->GetVisible(*primIt, time)) {
                return false;
            }
        }
        return true;
    }

    bool IsVisibilityVarying(const std::vector<UsdPrim>& instanceContext)
    {
        TF_FOR_ALL(primIt, instanceContext) {
            if (_GetIsVisibilityVarying(*primIt)) {
                return true;
            }
        }
        return false;
    }

    bool _GetIsVisibilityVarying(const UsdPrim& prim) {
        bool visibilityVarying;

        auto cacheIt = varyingCache.find(prim);
        if (cacheIt == varyingCache.end()) {
            HdDirtyBits dirtyBits;
            visibilityVarying = adapter->_IsVarying(
                prim,
                UsdGeomTokens->visibility,
                HdChangeTracker::DirtyVisibility,
                UsdImagingTokens->usdVaryingVisibility,
                &dirtyBits,
                true);
            varyingCache[prim] = visibilityVarying;
        } else {
            visibilityVarying = cacheIt->second;
        }

        return visibilityVarying;
    }

    const UsdImagingInstanceAdapter* adapter;
    UsdTimeCode time;
    std::vector<_InstancerData::Visibility>* visibility;
    VtIntArray* indices;

    // We keep a simple cache of visibility varying states directly on
    // _UpdateInstanceMapFn because we only need it for the first draw and
    // during resyncs.
    boost::unordered_map<UsdPrim, bool, boost::hash<UsdPrim>> varyingCache;
};

void
UsdImagingInstanceAdapter::_UpdateInstanceMap(
                    UsdPrim const& instancerPrim, 
                    UsdTimeCode time) const
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
                    UsdPrim const& instancerPrim) const
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
    if (!prim) {
        TF_CODING_ERROR("Invalid prim");
        return SdfPath();
    }

    if (prim.IsInstance() && !_PrimIsInstancer(prim)) {
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

            if (inst.childInstancers.find(instancerPath) !=
                inst.childInstancers.end()) {
                    return GetPathForInstanceIndex(instIt->first,
                                                   instanceIndex,
                                                   instanceCount,
                                                   absoluteInstanceIndex,
                                                   rprimPath,
                                                   instanceContext);
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
        if (!TF_VERIFY(static_cast<size_t>(instanceIndex) < instanceIndices.size())) {
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
        HdxSelectionHighlightMode const& highlightMode_,
        HdxSelectionSharedPtr const &result_)
        : adapter(adapter_)
        , instancerPath(instancerPath_)
        , instancePath(instancePath_)
        , instanceIndices(instanceIndices_)
        , highlightMode(highlightMode_)
        , result(result_)
        , found(false)
    {}

    void Initialize(size_t numInstances)
    {}

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        SdfPath path = instanceContext.back().GetPath();
        // When we don't have instanceIndices we might be looking for a subtree
        // in that case we can add everything under that path
        // Otherwise, we're only interested in the instanceContext 
        // which has instancePath
        if (!instanceIndices.empty()) {
            if (path != instancePath) {
                return true;
            }
        } else {
            if (!path.HasPrefix(instancePath)) {
                return true;
            }
        }

        const _InstancerData* instancerData = 
            TfMapLookupPtr(adapter->_instancerData, instancerPath);
        if (!TF_VERIFY(instancerData, "%s not found", instancerPath.GetText())) {
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
            // XXX: this seems redundant, but needed for point instancer 
            // highlighting for now. Ideally we should communicate back to point 
            // instancer adapter to not use renderIndex
            SdfPathVector const &ids
                = adapter->_delegate->GetRenderIndex().GetRprimSubtree(indexPath);
            TF_FOR_ALL (protoIt, ids) {
                result->AddInstance(highlightMode, *protoIt, niInstanceIndices);

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
    HdxSelectionHighlightMode highlightMode;
    HdxSelectionSharedPtr result;
    bool found;
};

/*virtual*/
bool
UsdImagingInstanceAdapter::PopulateSelection(
    HdxSelectionHighlightMode const& highlightMode,
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
            this, it->first, instancePath, instanceIndices,
            highlightMode, result);

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
UsdImagingInstanceAdapter::_RemovePrim(SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("Should use overidden ProcessPrimResync/ProcessPrimRemoval");
}


/*virtual*/
VtIntArray
UsdImagingInstanceAdapter::GetInstanceIndices(SdfPath const &instancerPath,
                                              SdfPath const &protoRprimPath)
{
    if (!instancerPath.IsEmpty()) {
        UsdImagingInstancerContext ctx;
        _ProtoRprim const& rproto =
            _GetProtoRprim(instancerPath, protoRprimPath, &ctx);
        if (!rproto.protoGroup) {
            TF_CODING_ERROR("NI: No prototype found for parent <%s> of <%s>\n",
                    instancerPath.GetText(),
                    protoRprimPath.GetText());
        } else {
            return rproto.protoGroup->indices;
        }
    }

    return VtIntArray();
}

/*virtual*/
GfMatrix4d
UsdImagingInstanceAdapter::GetRelativeInstancerTransform(
    SdfPath const &parentInstancerPath,
    SdfPath const &instancerPath, UsdTimeCode time) const
{
    // regardless the parentInstancerPath is empty or not,
    // we subtract the root transform.
    UsdPrim prim = _GetPrim(instancerPath.GetPrimPath());
    return GetTransform(prim, time) * GetRootTransform().GetInverse();
}

PXR_NAMESPACE_CLOSE_SCOPE

