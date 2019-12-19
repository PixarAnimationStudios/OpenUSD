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
#include "pxr/usdImaging/usdImaging/indexProxy.h"
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

#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/vec4h.h"

#include "pxr/base/tf/type.h"

#include <limits>
#include <queue>

PXR_NAMESPACE_OPEN_SCOPE

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

bool
UsdImagingInstanceAdapter::ShouldCullChildren() const
{
    return true;
}

bool
UsdImagingInstanceAdapter::IsInstancerAdapter() const
{
    return true;
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
    UsdImagingPrimAdapterSharedPtr const& instancerAdapter = shared_from_this();

    const SdfPath& instancerMaterialUsdPath =
        instancerAdapter->GetMaterialUsdPath(prim);

    // Storage for various instancer chains built up below.
    SdfPathVector instancerChain;

    // Construct the instance proxy path for "instancePath" to look up the
    // draw mode and inherited primvars for this instance.  If this is a
    // nested instance (meaning "prim" is part of a master), parentProxyPath
    // contains the instance proxy path for the master we're currently in,
    // so we can stitch the full proxy path together.
    TfToken instanceDrawMode;
    std::vector<_InstancerData::PrimvarInfo> inheritedPrimvars;
    {
        instancerChain = { instancePath };
        if (prim.IsInMaster()) {
            instancerChain.push_back(parentProxyPath);
        }
        SdfPath instanceChainPath =
            _GetPrimPathFromInstancerChain(instancerChain);
        if (UsdPrim instanceUsdPrim = _GetPrim(instanceChainPath)) {
            instanceDrawMode = GetModelDrawMode(instanceUsdPrim);
            UsdImaging_InheritedPrimvarStrategy::value_type
                inheritedPrimvarRecord = _GetInheritedPrimvars(instanceUsdPrim);
            if (inheritedPrimvarRecord) {
                for (auto const& pv : inheritedPrimvarRecord->primvars) {
                    inheritedPrimvars.push_back({
                        pv.GetPrimvarName(),
                        pv.GetTypeName()});
                }
                std::sort(inheritedPrimvars.begin(), inheritedPrimvars.end());
            }
        } else {
            TF_CODING_ERROR("Could not find USD instance prim at "
                            "instanceChainPath <%s> given instancePath <%s>, "
                            "parentProxyPath <%s>; isInMaster %i",
                            instanceChainPath.GetText(),
                            instancePath.GetText(),
                            parentProxyPath.GetText(),
                            int(prim.IsInMaster()));
        }
    }

    // Check if there's an instance in use as a hydra instancer for this master
    // with the appropriate inherited attributes.
    SdfPath instancerPath;
    auto range = _masterToInstancerMap.equal_range(masterPrim.GetPath());
    for (auto it = range.first; it != range.second; ++it) {
        _InstancerData& instancerData = _instancerData[it->second];
        // If material ID, draw mode, or inherited primvar set differ,
        // split the instance.
        if (instancerData.materialUsdPath == instancerMaterialUsdPath &&
            instancerData.drawMode == instanceDrawMode &&
            instancerData.inheritedPrimvars == inheritedPrimvars) {
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
        instancerData.materialUsdPath = instancerMaterialUsdPath;
        instancerData.drawMode = instanceDrawMode;
        instancerData.inheritedPrimvars = inheritedPrimvars;

        UsdImagingInstancerContext ctx = { SdfPath(),
                                           TfToken(),
                                           SdfPath(),
                                           TfToken(),
                                           instancerAdapter };

        // -------------------------------------------------------------- //
        // Allocate prototype prims.
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

            if (UsdImagingPrimAdapter::ShouldCullSubtree(instanceProxyPrim)) {
                iter.PruneChildren();
                continue;
            }

            UsdImagingPrimAdapterSharedPtr const& primAdapter =
                _GetPrimAdapter(instanceProxyPrim, /*ignoreInstancing=*/ true);
            if (!primAdapter) {
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
            SdfPath protoMaterialId =
                primAdapter->GetMaterialUsdPath(*iter);
            if (protoMaterialId.IsEmpty()) {
                protoMaterialId = instancerMaterialUsdPath;
            }
            TfToken protoDrawMode = GetModelDrawMode(instanceProxyPrim);

            const SdfPath protoPath = 
                _InsertProtoPrim(&iter, protoName, protoMaterialId,
                                 protoDrawMode, instancerPath,
                                 primAdapter, instancerAdapter, index,
                                 &isLeafInstancer);
                    
            // 
            // Update instancer data.
            //
            _ProtoPrim& proto = instancerData.primMap[protoPath];
            if (iter->IsMaster()) {
                // If the prototype we're populating is the master prim,
                // our prim handle should be to the instance, since the master
                // prim doesn't have attributes.
                proto.path = instancerPath;
            } else {
                proto.path = iter->GetPath();
            }
            proto.adapter = primAdapter;
            ++primCount;

            if (!isLeafInstancer) {
                instancerData.childPointInstancers.insert(protoPath);
            }

            TF_DEBUG(USDIMAGING_INSTANCER).Msg(
                "[Add Instance NI] <%s>  %s (%s), adapter = %s\n",
                instancerPath.GetText(), protoPath.GetText(),
                iter->GetName().GetText(),
                primAdapter ?
                    TfType::GetCanonicalTypeName(typeid(*primAdapter)).c_str() :
                    "none");
        }
        // Add this instancer into the render index if it has any prototypes.
        if (primCount > 0) {
            index->InsertInstancer(instancerPath,
                                   /*parentPath=*/ctx.instancerCachePath,
                                   _GetPrim(instancerPath),
                                   ctx.instancerAdapter);
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
        }
    }

    // We're done modifying data structures for the passed in instance,
    // so now it's safe to re-enter this function to populate the
    // nested instances we discovered.
    TF_FOR_ALL(nestedInstanceIt, nestedInstances) {
        _Populate(*nestedInstanceIt, index, instancerContext,
                  instancerProxyPath);
        instancerData.nestedInstances.push_back(nestedInstanceIt->GetPath());
    }

    // Add a dependency on any associated hydra instancers (instancerPath, if
    // this instance wasn't added to hydra, and any nested instancers); also
    // make sure to mark all hydra instancers dirty.
    std::queue<SdfPath> depInstancePaths;
    depInstancePaths.push(instancePath);
    std::set<SdfPath> visited;
    while (!depInstancePaths.empty()) {
        SdfPath depInstancePath = depInstancePaths.front();
        depInstancePaths.pop();

        if (depInstancePath.IsEmpty()) {
            continue;
        }

        SdfPath depInstancerPath = _instanceToInstancerMap[depInstancePath];

        auto result = visited.insert(depInstancerPath);
        if (!result.second) {
            continue;
        }

        if (index->IsPopulated(depInstancerPath)) {
            // If we've found a populated instancer, register a dependency,
            // unless depInstancerPath == prim.GetPath, in which case the
            // dependency was automatically added by InsertInstancer.
            if(depInstancerPath != prim.GetPath()) {
                index->AddDependency(depInstancerPath, prim);
            }

            // Ask hydra to do a full refresh on this instancer.
            index->MarkInstancerDirty(depInstancerPath,
                    HdChangeTracker::DirtyPrimvar |
                    HdChangeTracker::DirtyTransform |
                    HdChangeTracker::DirtyInstanceIndex);
        }

        _InstancerData& depInstancerData =
            _instancerData[depInstancerPath];
        for (SdfPath const& nestedInstance :
                depInstancerData.nestedInstances) {
            depInstancePaths.push(nestedInstance);
        }
    }

    return instancerPath;
}

SdfPath
UsdImagingInstanceAdapter::_InsertProtoPrim(
    UsdPrimRange::iterator *it,
    TfToken const& protoName,
    SdfPath materialUsdPath,
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
                                       materialUsdPath,
                                       drawMode,
                                       instancerAdapter };

    SdfPath protoPath = primAdapter->Populate(prim, index, &ctx);

    if (primAdapter->ShouldCullChildren()) {
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
UsdImagingInstanceAdapter::TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext) const
{
    UsdImagingValueCache* valueCache = _GetValueCache();

    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                cachePath,
                                                &instancerContext);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return;
        }

        UsdPrim protoPrim = _GetPrim(proto.path);
        proto.adapter->TrackVariability(protoPrim, cachePath,
            timeVaryingBits, &instancerContext);
    } else if (_InstancerData const* instrData =
               TfMapLookupPtr(_instancerData, prim.GetPath())) {
        // In this case, prim is an instance master. Master prims provide
        // no data of their own, so we fall back to the default purpose.
        // XXX: This seems incorrect?
        valueCache->GetPurpose(cachePath) = UsdGeomTokens->default_;

        // Count how many instances there are in total (used for the loop
        // counter of _RunForAllInstancesToDraw).
        instrData->numInstancesToDraw = _CountAllInstancesToDraw(prim);

        if (_IsInstanceTransformVarying(prim)) {
            // Instance transforms are stored as instance-rate primvars.
            *timeVaryingBits |= HdChangeTracker::DirtyPrimvar;
        }
        if (!instrData->inheritedPrimvars.empty() &&
                _IsInstanceInheritedPrimvarVarying(prim)) {
            *timeVaryingBits |= HdChangeTracker::DirtyPrimvar;
        }
        if (_ComputeInstanceMapVariability(prim, *instrData)) {
            *timeVaryingBits |= HdChangeTracker::DirtyInstanceIndex;
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
        inverseRoot = adapter->GetRootTransform().GetInverse();
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

        // The prototype transform will have the root transform, so we need
        // to negate that.
        xform = inverseRoot * xform;

        result[instanceIdx] = xform;
        return true;
    }

    const UsdImagingInstanceAdapter* adapter;
    UsdTimeCode time;
    VtMatrix4dArray result;
    GfMatrix4d inverseRoot;
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

struct UsdImagingInstanceAdapter::_GatherInstanceTransformTimeSamplesFn
{
    _GatherInstanceTransformTimeSamplesFn(
        const UsdImagingInstanceAdapter* adapter_, const GfInterval& interval_) 
        : adapter(adapter_), interval(interval_) 
    { }

    void Initialize(size_t numInstances)
    { }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        TF_FOR_ALL(primIt, instanceContext) {
             if (UsdGeomXformable xf = UsdGeomXformable(*primIt)) {
                std::vector<double> localTimeSamples;
                xf.GetTimeSamplesInInterval(interval, &localTimeSamples);

                // Join timesamples 
                result.insert(
                    result.end(), 
                    localTimeSamples.begin(), 
                    localTimeSamples.end());
            }
        }
        return true;
    }

    const UsdImagingInstanceAdapter* adapter;
    GfInterval interval;
    std::vector<double> result;
};

bool
UsdImagingInstanceAdapter::_GatherInstanceTransformsTimeSamples(
    UsdPrim const& instancer,
    GfInterval interval,
    std::vector<double>* outTimes) const
{
    HD_TRACE_FUNCTION();

    _GatherInstanceTransformTimeSamplesFn gatherSamples(this, interval);
    _RunForAllInstancesToDraw(instancer, &gatherSamples);
    outTimes->swap(gatherSamples.result);
    return true;
}

struct UsdImagingInstanceAdapter::_GatherInstancePrimvarTimeSamplesFn
{
    _GatherInstancePrimvarTimeSamplesFn(
        const UsdImagingInstanceAdapter* adapter_,
        const GfInterval& interval_,
        TfToken const& key_)
        : adapter(adapter_), interval(interval_), key(key_)
    { }

    void Initialize(size_t numInstances)
    { }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        SdfPathVector instanceChain;
        for (UsdPrim const& prim : instanceContext) {
            instanceChain.push_back(prim.GetPath());
        }
        SdfPath instanceChainPath =
            adapter->_GetPrimPathFromInstancerChain(instanceChain);
        if (UsdPrim instanceProxyPrim =
                adapter->_GetPrim(instanceChainPath)) {
            UsdImaging_InheritedPrimvarStrategy::value_type
                inheritedPrimvarRecord =
                    adapter->_GetInheritedPrimvars(instanceProxyPrim);
            if (inheritedPrimvarRecord) {
                for (auto const& pv : inheritedPrimvarRecord->primvars) {
                    if (pv.GetPrimvarName() == key) {
                        // At this point, pv is the actual primvar attribute
                        // for this instantiation of instanceContext.
                        std::vector<double> localTimeSamples;
                        pv.GetTimeSamplesInInterval(interval, &localTimeSamples);

                        // Join timesamples
                        result.insert(
                            result.end(), 
                            localTimeSamples.begin(), 
                            localTimeSamples.end());
                    }
                }
            }
        }
        return true;
    }

    const UsdImagingInstanceAdapter* adapter;
    GfInterval interval;
    TfToken key;
    std::vector<double> result;
};

bool
UsdImagingInstanceAdapter::_GatherInstancePrimvarTimeSamples(
    UsdPrim const& instancer,
    TfToken const& key,
    GfInterval interval,
    std::vector<double>* outTimes) const
{
    HD_TRACE_FUNCTION();

    _GatherInstancePrimvarTimeSamplesFn gatherSamples(this, interval, key);
    _RunForAllInstancesToDraw(instancer, &gatherSamples);
    outTimes->swap(gatherSamples.result);
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
                HdInstancerTokens->instancer,
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

template<typename T>
struct UsdImagingInstanceAdapter::_ComputeInheritedPrimvarFn
{
    _ComputeInheritedPrimvarFn(
        const UsdImagingInstanceAdapter* adapter_,
        TfToken const& name_,
        UsdTimeCode const& time_)
        : adapter(adapter_), name(name_), time(time_) { }

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

        SdfPathVector instanceChain;
        for (UsdPrim const& prim : instanceContext) {
            instanceChain.push_back(prim.GetPath());
        }
        SdfPath instanceChainPath =
            adapter->_GetPrimPathFromInstancerChain(instanceChain);
        if (UsdPrim instanceProxyPrim =
                adapter->_GetPrim(instanceChainPath)) {
            UsdImaging_InheritedPrimvarStrategy::value_type
                inheritedPrimvarRecord =
                    adapter->_GetInheritedPrimvars(instanceProxyPrim);
            if (inheritedPrimvarRecord) {
                for (auto const& pv : inheritedPrimvarRecord->primvars) {
                    if (pv.GetPrimvarName() == name) {
                        VtValue v;
                        pv.ComputeFlattened(&v, time);
                        if (v.IsHolding<T>()) {
                            result[instanceIdx] = v.Get<T>();
                        } else if (v.IsHolding<VtArray<T>>()) {
                            VtArray<T> a = v.Get<VtArray<T>>();
                            if (a.size() > 0) {
                                result[instanceIdx] = a[0];
                            }
                            if (a.size() != 1) {
                                sampleSizeErrorPaths.push_back(
                                    pv.GetAttr().GetPath());
                            }
                        } else {
                            TF_CODING_ERROR("Unexpected VtValue type %s "
                                "for primvar %s (expected %s)",
                                v.GetTypeName().c_str(),
                                pv.GetAttr().GetPath().GetText(),
                                TfType::Find<T>().GetTypeName().c_str());
                        }
                    }
                }
            }
        }
        return true;
    }

    const UsdImagingInstanceAdapter* adapter;
    TfToken name;
    UsdTimeCode time;
    VtArray<T> result;
    SdfPathVector sampleSizeErrorPaths;
};

bool
UsdImagingInstanceAdapter::_ComputeInheritedPrimvar(UsdPrim const& instancer,
                                                    TfToken const& primvarName,
                                                    SdfValueTypeName const& type,
                                                    VtValue *result,
                                                    UsdTimeCode time) const
{
    // Unfortunately, we have the type info as the run-time SdfValueTypeName
    // object, but not the compile-time T.  If we put a dispatch hook in
    // Sdf or VtValue, we wouldn't need this table.
    //
    // This set of types was chosen to match HdGetValueData(), e.g. the set
    // of types hydra can reliably transport through primvars.
    VtValue dv = type.GetScalarType().GetDefaultValue();
    if (dv.IsHolding<GfHalf>()) {
        return _ComputeInheritedPrimvar<GfHalf>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfMatrix3d>()) {
        return _ComputeInheritedPrimvar<GfMatrix3d>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfMatrix3f>()) {
        return _ComputeInheritedPrimvar<GfMatrix3f>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfMatrix4d>()) {
        return _ComputeInheritedPrimvar<GfMatrix4d>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfMatrix4f>()) {
        return _ComputeInheritedPrimvar<GfMatrix4f>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec2d>()) {
        return _ComputeInheritedPrimvar<GfVec2d>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec2f>()) {
        return _ComputeInheritedPrimvar<GfVec2f>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec2h>()) {
        return _ComputeInheritedPrimvar<GfVec2h>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec2i>()) {
        return _ComputeInheritedPrimvar<GfVec2i>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec3d>()) {
        return _ComputeInheritedPrimvar<GfVec3d>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec3f>()) {
        return _ComputeInheritedPrimvar<GfVec3f>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec3h>()) {
        return _ComputeInheritedPrimvar<GfVec3h>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec3i>()) {
        return _ComputeInheritedPrimvar<GfVec3i>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec4d>()) {
        return _ComputeInheritedPrimvar<GfVec4d>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec4f>()) {
        return _ComputeInheritedPrimvar<GfVec4f>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec4h>()) {
        return _ComputeInheritedPrimvar<GfVec4h>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<GfVec4i>()) {
        return _ComputeInheritedPrimvar<GfVec4i>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<bool>()) {
        return _ComputeInheritedPrimvar<bool>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<char>()) {
        return _ComputeInheritedPrimvar<char>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<double>()) {
        return _ComputeInheritedPrimvar<double>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<float>()) {
        return _ComputeInheritedPrimvar<float>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<int16_t>()) {
        return _ComputeInheritedPrimvar<int16_t>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<int32_t>()) {
        return _ComputeInheritedPrimvar<int32_t>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<uint16_t>()) {
        return _ComputeInheritedPrimvar<uint16_t>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<uint32_t>()) {
        return _ComputeInheritedPrimvar<uint32_t>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<unsigned char>()) {
        return _ComputeInheritedPrimvar<unsigned char>(
                instancer, primvarName, result, time);
    } else if (dv.IsHolding<std::string>()) {
        return _ComputeInheritedPrimvar<std::string>(
                instancer, primvarName, result, time);
    } else {
        TF_WARN("Native instancing: unrecognized inherited primvar type '%s' "
                "for primvar '%s'", 
                type.GetAsToken().GetText(),
                primvarName.GetText());
        return false;
    }
}

template<typename T>
bool
UsdImagingInstanceAdapter::_ComputeInheritedPrimvar(UsdPrim const& instancer,
                                                    TfToken const& primvarName,
                                                    VtValue *result,
                                                    UsdTimeCode time) const
{
    _ComputeInheritedPrimvarFn<T> computeInheritedPrimvar(
        this, primvarName, time);
    _RunForAllInstancesToDraw(instancer, &computeInheritedPrimvar);
    *result = VtValue(computeInheritedPrimvar.result);
    for (SdfPath const& errorPath :
            computeInheritedPrimvar.sampleSizeErrorPaths) {
        TF_WARN("Instance inherited primvar %s doesn't define the right "
                "number of samples (only 1 sample is supported)",
                errorPath.GetText());
    }
    return true;
}

struct UsdImagingInstanceAdapter::_IsInstanceInheritedPrimvarVaryingFn
{
    _IsInstanceInheritedPrimvarVaryingFn(
        const UsdImagingInstanceAdapter* adapter_)
        : adapter(adapter_), result(false) { }

    void Initialize(size_t numInstances)
    { }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        SdfPathVector instanceChain;
        for (UsdPrim const& prim : instanceContext) {
            instanceChain.push_back(prim.GetPath());
        }
        SdfPath instanceChainPath =
            adapter->_GetPrimPathFromInstancerChain(instanceChain);
        if (UsdPrim instanceProxyPrim =
                adapter->_GetPrim(instanceChainPath)) {
            UsdImaging_InheritedPrimvarStrategy::value_type
                inheritedPrimvarRecord =
                    adapter->_GetInheritedPrimvars(instanceProxyPrim);
            if (inheritedPrimvarRecord && inheritedPrimvarRecord->variable) {
                result = true;
            }
        }
        return !result;
    }

    const UsdImagingInstanceAdapter* adapter;
    bool result;
};

bool
UsdImagingInstanceAdapter::_IsInstanceInheritedPrimvarVarying(
                                UsdPrim const& instancer) const
{
    _IsInstanceInheritedPrimvarVaryingFn isPrimvarVarying(this);
    _RunForAllInstancesToDraw(instancer, &isPrimvarVarying);
    return isPrimvarVarying.result;
}

bool
UsdImagingInstanceAdapter::_InstancerData::PrimvarInfo::operator<
    (const UsdImagingInstanceAdapter::_InstancerData::PrimvarInfo &rhs) const {
    // This is the logic from std::pair, except for the GetAsToken calls.
    if (name < rhs.name) { return true; }
    else if (rhs.name < name) { return false; }
    else if (type.GetAsToken() < rhs.type.GetAsToken()) { return true; }
    else { return false; }
}

bool
UsdImagingInstanceAdapter::_InstancerData::PrimvarInfo::operator==
    (const UsdImagingInstanceAdapter::_InstancerData::PrimvarInfo &rhs) const {
    return (name == rhs.name && type == rhs.type);
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
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                cachePath,
                                                &instancerContext);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return;
        }

        UsdPrim protoPrim = _GetPrim(proto.path);
        proto.adapter->UpdateForTime(protoPrim, cachePath, 
            time, requestedBits, &instancerContext);
    } else if (_InstancerData const* instrData =
               TfMapLookupPtr(_instancerData, prim.GetPath())) {

        // We handle per-instance visibility by pulling stuff out of the
        // instance indices, so DirtyInstanceIndex means we need to recompute
        // that map.
        if (requestedBits & HdChangeTracker::DirtyInstanceIndex) {
            VtIntArray indices = _ComputeInstanceMap(prim, *instrData, time);
            // XXX: See UsdImagingDelegate::GetInstanceIndices;
            // the change-tracking is on the instancer prim, but for simplicity
            // we store each prototype's index buffer in that prototype's value
            // cache (since each prototype can only have one instancer).
            for (auto const& pair : instrData->primMap) {
                valueCache->GetInstanceIndices(pair.first) = indices;
            }
        }

        // Per-instance transforms and inherited primvars are handled by
        // DirtyPrimvar.
        if (requestedBits & HdChangeTracker::DirtyPrimvar) {
            VtMatrix4dArray instanceXforms;
            if (_ComputeInstanceTransforms(prim, &instanceXforms, time)) {
                valueCache->GetPrimvar(
                    cachePath,
                    HdInstancerTokens->instanceTransform) = instanceXforms;
                _MergePrimvar(
                    &valueCache->GetPrimvars(cachePath),
                    HdInstancerTokens->instanceTransform,
                    HdInterpolationInstance);
            }
            for (auto const& ipv : instrData->inheritedPrimvars) {
                VtValue val;
                if (_ComputeInheritedPrimvar(prim, ipv.name, ipv.type,
                                             &val, time)) {
                    valueCache->GetPrimvar(cachePath, ipv.name) = val;
                    _MergePrimvar(&valueCache->GetPrimvars(cachePath),
                                  ipv.name, HdInterpolationInstance,
                                  _UsdToHdRole(ipv.type.GetRole()));
                }
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
    // If this is called on behalf of a prototype prim, pass the call through.
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return HdChangeTracker::AllDirty;
        }

        UsdPrim protoPrim = _GetPrim(proto.path);
        HdDirtyBits dirtyBits = proto.adapter->ProcessPropertyChange(
            protoPrim, cachePath, propertyName);

        return dirtyBits;
    }

    // Transform changes to instance prims end up getting folded into the
    // "instanceTransform" instance-rate primvar.
    if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName)) {
        return HdChangeTracker::DirtyPrimvar;
    }

    // Visibility changes to instance prims end up getting folded into the
    // instance map.
    if (propertyName == UsdGeomTokens->visibility) {
        return HdChangeTracker::DirtyInstanceIndex;
    }

    // Is the property a primvar?
    static std::string primvarsNS = "primvars:";
    if (TfStringStartsWith(propertyName.GetString(), primvarsNS)) {
        TfToken primvarName = TfToken(
            propertyName.GetString().substr(primvarsNS.size()));
        if (_PrimvarChangeRequiresResync(
                prim, cachePath, propertyName, primvarName)) {
            return HdChangeTracker::AllDirty;
        } else {
            return HdChangeTracker::DirtyPrimvar;
        }
    }

    // For other property changes, blast everything.  This will trigger a
    // prim resync.
    return HdChangeTracker::AllDirty;
}

void
UsdImagingInstanceAdapter::_ResyncPath(SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index,
                                       bool reload)
{
    // Either the prim was fundamentally modified or removed.
    // Regenerate instancer data if an instancer depends on the
    // resync'd prim. 
    SdfPathVector instancersToUnload;

    TF_FOR_ALL(instIt, _instancerData) {
        SdfPath const& instancerPath = instIt->first;
        _InstancerData& inst = instIt->second;

        // The resync'd prim is a dependency if it is a descendent of
        // the instancer master prim.
        if (cachePath.HasPrefix(inst.masterPath)) {
            instancersToUnload.push_back(instancerPath);
            continue;
        }

        // The resync'd prim is a dependency if it is an instance of
        // the instancer master prim.
        const SdfPathVector& instances = inst.instancePaths;
        if (std::binary_search(instances.begin(), instances.end(), cachePath)) {
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

void UsdImagingInstanceAdapter::ProcessPrimResync(SdfPath const& cachePath,
                                                  UsdImagingIndexProxy* index)
{
    _ResyncPath(cachePath, index, /*reload=*/true);
}

void UsdImagingInstanceAdapter::ProcessPrimRemoval(SdfPath const& cachePath,
                                                   UsdImagingIndexProxy* index)
{
    _ResyncPath(cachePath, index, /*reload=*/false);
}

void
UsdImagingInstanceAdapter::MarkDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     HdDirtyBits dirty,
                                     UsdImagingIndexProxy* index)
{
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            proto.adapter->MarkDirty(prim, cachePath, dirty, index);
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
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            proto.adapter->MarkRefineLevelDirty(prim, cachePath, index);
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
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            proto.adapter->MarkReprDirty(prim, cachePath, index);
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
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            proto.adapter->MarkCullStyleDirty(prim, cachePath, index);
        }
    }
}

void
UsdImagingInstanceAdapter::MarkRenderTagDirty(UsdPrim const& prim,
                                       SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index)
{
    // The instancer isn't interested in this, but it's children maybe
    // so make sure the message gets forwarded
    if (_IsChildPrim(prim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            proto.adapter->MarkRenderTagDirty(prim, cachePath, index);
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
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            proto.adapter->MarkTransformDirty(prim, cachePath, index);
        }
    } else if (TfMapLookupPtr(_instancerData, prim.GetPath()) != nullptr) {
        // For the instancer itself, the instance transforms are sent back
        // as primvars, so we need to augment the DirtyTransform bit with
        // DirtyPrimvar.
        static const HdDirtyBits transformDirty =
                                                HdChangeTracker::DirtyPrimvar  |
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
        _ProtoPrim const& proto = _GetProtoPrim(prim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);

        if (TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            proto.adapter->MarkVisibilityDirty(prim, cachePath, index);
        }
    } else if (TfMapLookupPtr(_instancerData, prim.GetPath()) != nullptr) {
        // For the instancer itself, the visibility of each instance affects
        // whether or not it gets added to the instance indices array, so we
        // need to return DirtyInstanceIndex.
        static const HdDirtyBits visibilityDirty =
                                            HdChangeTracker::DirtyVisibility |
                                            HdChangeTracker::DirtyInstanceIndex;

        index->MarkInstancerDirty(cachePath, visibilityDirty);
    }
}

/*virtual*/
std::vector<VtArray<TfToken>>
UsdImagingInstanceAdapter::GetInstanceCategories(UsdPrim const& prim) 
{
    HD_TRACE_FUNCTION();
    std::vector<VtArray<TfToken>> categories;
    if (const _InstancerData* instancerData = 
        TfMapLookupPtr(_instancerData, prim.GetPath())) {
        UsdImaging_CollectionCache& cc = _GetCollectionCache();
        categories.reserve(instancerData->instancePaths.size());
        for (SdfPath const& p: instancerData->instancePaths) {
            categories.push_back(cc.ComputeCollectionsContainingPath(p));
        }
    }
    return categories;
}

/*virtual*/
size_t
UsdImagingInstanceAdapter::SampleInstancerTransform(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerPath,
    UsdTimeCode time,
    size_t maxSampleCount,
    float *sampleTimes,
    GfMatrix4d *sampleValues)
{
    // This code must match UpdateForTime(), which says:
    // the instancer transform can only be the root transform.
    if (maxSampleCount > 0) {
        sampleTimes[0] = 0.0f;
        sampleValues[0] = GetRootTransform();
        return 1;
    }
    return 0;
}

size_t
UsdImagingInstanceAdapter::SampleTransform(
    UsdPrim const& usdPrim, 
    SdfPath const& cachePath,
    UsdTimeCode time, 
    size_t maxNumSamples, 
    float *sampleTimes,
    GfMatrix4d *sampleValues)
{
    HD_TRACE_FUNCTION();

    if (maxNumSamples == 0) {
        return 0;
    }

    if (_IsChildPrim(usdPrim, cachePath)) {
        // Note that the proto group in this proto has not yet been
        // updated with new instances at this point.
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return 0;
        }
        return proto.adapter->SampleTransform(
            _GetPrim(proto.path), cachePath,
            time, maxNumSamples, sampleTimes, sampleValues);
    } else {
        return UsdImagingPrimAdapter::SampleTransform(
            usdPrim, cachePath, time, maxNumSamples,
            sampleTimes, sampleValues);
    }
}

size_t
UsdImagingInstanceAdapter::SamplePrimvar(
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

    if (_IsChildPrim(usdPrim, cachePath)) {
        // Note that the proto group in this proto has not yet been
        // updated with new instances at this point.
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(),
                                                    cachePath,
                                                    &instancerContext);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return 0;
        }
        return proto.adapter->SamplePrimvar(
            _GetPrim(proto.path), cachePath, key, time,  
            maxNumSamples, sampleTimes, sampleValues);
    }

    GfInterval interval = _GetCurrentTimeSamplingInterval();
    std::vector<double> timeSamples;
    SdfValueTypeName type;

    if (key != HdInstancerTokens->instanceTransform) {
        // "instanceTransform" is built-in and synthesized, but other primvars
        // need to be in the inherited primvar list. Loop through to check
        // existence and find the correct type.
        _InstancerData const* instrData =
            TfMapLookupPtr(_instancerData, usdPrim.GetPath());
        if (!instrData) {
            return 0;
        }
        bool found = false;
        for (auto const& ipv : instrData->inheritedPrimvars) {
            if (ipv.name == key) {
                type = ipv.type;
                found = true;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }

    if (key == HdInstancerTokens->instanceTransform) {
        _GatherInstanceTransformsTimeSamples(usdPrim, interval, &timeSamples);
    } else {
        _GatherInstancePrimvarTimeSamples(usdPrim, key, interval, &timeSamples);
    }

    timeSamples.push_back(interval.GetMin());
    timeSamples.push_back(interval.GetMax());

    // Sort here
    std::sort(timeSamples.begin(), timeSamples.end());
    timeSamples.erase(
        std::unique(timeSamples.begin(), 
            timeSamples.end()), 
            timeSamples.end());
    size_t numSamples = timeSamples.size();
    size_t numSamplesToEvaluate = std::min(maxNumSamples, numSamples);

    for (size_t i=0; i < numSamplesToEvaluate; ++i) {
        sampleTimes[i] = timeSamples[i] - time.GetValue();
        if (key == HdInstancerTokens->instanceTransform) {
            VtMatrix4dArray xf;
            _ComputeInstanceTransforms(usdPrim, &xf, timeSamples[i]);
            sampleValues[i] = xf;
        } else {
            VtValue val;
            _ComputeInheritedPrimvar(usdPrim, key, type, &val, timeSamples[i]);
            sampleValues[i] = val;
        }
    }
    return numSamples;
}

PxOsdSubdivTags
UsdImagingInstanceAdapter::GetSubdivTags(UsdPrim const& usdPrim,
                                         SdfPath const& cachePath,
                                         UsdTimeCode time) const
{
    if (_IsChildPrim(usdPrim, cachePath)) {
        // Note that the proto group in this proto has not yet been
        // updated with new instances at this point.
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(),
                                                   cachePath,
                                                   &instancerContext);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return PxOsdSubdivTags();
        }
        return proto.adapter->GetSubdivTags(
                _GetPrim(proto.path), cachePath, time);
    }
    return UsdImagingPrimAdapter::GetSubdivTags(usdPrim, cachePath, time);
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

    // Remove this instancer's entry from the master -> instancer map.
    auto range = _masterToInstancerMap.equal_range(instIt->second.masterPath);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == instancerPath) {
            _masterToInstancerMap.erase(it);
            break;
        }
    }

    // Remove the instancer, if it's an actual hydra prim. In nested instancing
    // cases, we might have an _instancerData entry but no hydra instancer.
    if (index->IsPopulated(instancerPath)) {
        index->RemoveInstancer(instancerPath);
    }

    // Keep a copy of the instancer's instances so we can repopulate them below.
    const SdfPathVector instancePaths = instIt->second.instancePaths;

    // Remove local instancer data.
    _instancerData.erase(instIt);

    // Repopulate the instancer's previous instances. Those that don't exist
    // anymore will be ignored, while those that still exist will be
    // pushed back into this adapter and refreshed.
    if (repopulate) {
        TF_FOR_ALL(pathIt, instancePaths) {
            UsdPrim prim = _GetPrim(*pathIt);
            if (prim && prim.IsActive() && !prim.IsInMaster()) {
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

UsdImagingInstanceAdapter::_ProtoPrim const&
UsdImagingInstanceAdapter::_GetProtoPrim(SdfPath const& instancerPath,
                                         SdfPath const& cachePath,
                                         UsdImagingInstancerContext* ctx) const
{
    static _ProtoPrim const EMPTY;

    _InstancerDataMap::const_iterator it = _instancerData.find(instancerPath);
    _ProtoPrim const* r = nullptr;
    SdfPath instancerCachePath;
    SdfPath materialUsdPath;
    TfToken drawMode;

    if (it != _instancerData.end()) {
        _PrimMap::const_iterator primIt = it->second.primMap.find(cachePath);
        if (primIt == it->second.primMap.end()) {
            return EMPTY;
        }
        instancerCachePath = instancerPath;
        materialUsdPath = it->second.materialUsdPath;
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
                instancerCachePath = pathInstancerDataPair.first;
                materialUsdPath = 
                    pathInstancerDataPair.second.materialUsdPath;
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

    ctx->instancerCachePath = instancerCachePath;
    ctx->instancerMaterialUsdPath = materialUsdPath;
    ctx->instanceDrawMode = drawMode;
    ctx->childName = TfToken();
    // Note: use a null adapter here.  The UsdImagingInstancerContext is
    // not really used outside of population.  We should clean this up and
    // remove these contexts from everything outside of population.
    ctx->instancerAdapter = UsdImagingPrimAdapterSharedPtr();

    return *r;
}

struct UsdImagingInstanceAdapter::_ComputeInstanceMapVariabilityFn
{
    _ComputeInstanceMapVariabilityFn(
        const UsdImagingInstanceAdapter* adapter_,
        std::vector<_InstancerData::Visibility>* visibility_)
        : adapter(adapter_), visibility(visibility_)
    { }

    void Initialize(size_t numInstances)
    {
        visibility->resize(numInstances, _InstancerData::Unknown);
    }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        if (!TF_VERIFY(instanceIdx < visibility->size())) {
            return true;
        }

        _InstancerData::Visibility& instanceVis =
            (*visibility)[instanceIdx];

        if (IsVisibilityVarying(instanceContext)) {
            instanceVis = _InstancerData::Varying;
        } else {
            bool vis = GetVisible(instanceContext);
            instanceVis = (vis ? _InstancerData::Visible :
                                 _InstancerData::Invisible);
        }

        return true;
    }

    bool GetVisible(const std::vector<UsdPrim>& instanceContext)
    {
        // XXX: The usage of _GetTimeWithOffset here is super-sketch, but
        // it avoids blowing up the inherited visibility cache... The value
        // doesn't matter since we only call this function when visibility
        // is not variable.
        UsdTimeCode time = adapter->_GetTimeWithOffset(0.0);
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

    // We keep a simple cache of visibility varying states directly on
    // _ComputeInstanceMapVariabilityFn because we only need it for the
    // variability calculation and during resyncs.
    boost::unordered_map<UsdPrim, bool, boost::hash<UsdPrim>> varyingCache;
    const UsdImagingInstanceAdapter* adapter;
    std::vector<_InstancerData::Visibility>* visibility;
};

bool
UsdImagingInstanceAdapter::_ComputeInstanceMapVariability(
        UsdPrim const& instancerPrim,
        _InstancerData const& instrData) const
{
    _ComputeInstanceMapVariabilityFn computeInstanceMapVariabilityFn(
            this, &instrData.visibility);
    _RunForAllInstancesToDraw(instancerPrim,
            &computeInstanceMapVariabilityFn); 

    return (std::find(instrData.visibility.begin(),
                      instrData.visibility.end(),
                      _InstancerData::Varying) != instrData.visibility.end());
}

struct UsdImagingInstanceAdapter::_ComputeInstanceMapFn
{
    _ComputeInstanceMapFn(
        const UsdImagingInstanceAdapter* adapter_, const UsdTimeCode& time_,
        const std::vector<_InstancerData::Visibility>* visibility_, 
        VtIntArray* indices_)
        : adapter(adapter_), time(time_), 
          visibility(visibility_), indices(indices_)
    { }

    void Initialize(size_t numInstances)
    {
    }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        if (!TF_VERIFY(instanceIdx < visibility->size())) {
            return true;
        }

        bool vis = true;
        const _InstancerData::Visibility& instanceVis =
            (*visibility)[instanceIdx];

        TF_VERIFY(instanceVis != _InstancerData::Unknown);

        if (instanceVis == _InstancerData::Varying) {
            vis = GetVisible(instanceContext);
        } else {
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

    const UsdImagingInstanceAdapter* adapter;
    UsdTimeCode time;
    const std::vector<_InstancerData::Visibility>* visibility;
    VtIntArray* indices;
};

VtIntArray
UsdImagingInstanceAdapter::_ComputeInstanceMap(
                    UsdPrim const& instancerPrim, 
                    _InstancerData const& instrData,
                    UsdTimeCode time) const
{
    VtIntArray indices(0);

    _ComputeInstanceMapFn computeInstanceMapFn(
        this, time, &instrData.visibility, &indices);
    _RunForAllInstancesToDraw(instancerPrim, &computeInstanceMapFn);
    return indices;
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
                    SdfPath const &protoCachePath,
                    int protoIndex,
                    int *instanceCount,
                    int *instancerIndex,
                    SdfPath *masterCachePath,
                    SdfPathVector *instanceContext)
{
    UsdPrim const &prim = _GetPrim(protoCachePath.GetAbsoluteRootOrPrimPath());
    if (!prim) {
        TF_CODING_ERROR("Invalid prim");
        return SdfPath();
    }

    if (prim.IsInstance() && !_PrimIsInstancer(prim)) {
        // This instance prim is handled by this adapter, but it's not
        // an instancer so the instance index doesn't apply.
        return SdfPath();
    }

    SdfPath instancerPath = protoCachePath.GetPrimPath();
    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "NI: Look for %s [%d]\n", instancerPath.GetText(), protoIndex);

    _InstancerDataMap::iterator instIt = _instancerData.find(instancerPath);
    if (instIt == _instancerData.end()) {
        // if it's not found, it may be an instance of other instancer.
        TF_FOR_ALL(instIt, _instancerData) {
            _InstancerData& inst = instIt->second;

            if (inst.childPointInstancers.find(instancerPath) !=
                inst.childPointInstancers.end()) {
                    return GetPathForInstanceIndex(instIt->first,
                                                   protoIndex,
                                                   instanceCount,
                                                   instancerIndex,
                                                   masterCachePath,
                                                   instanceContext);
            }
        }
        TF_CODING_ERROR("Unknown instancer %s", instancerPath.GetText());
        return SdfPath();
    }

    // remap protoIndex
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
    // if we pick 3, this function takes protoIndex = 2.
    // we need to map 2 back to 3 by instanceIndices[protoIndex]

    // XXX: The usage of _GetTimeWithOffset here feels weird...
    VtIntArray instanceIndices = _ComputeInstanceMap(
        _GetPrim(instancerPath), instIt->second, _GetTimeWithOffset(0.0));

    int instanceIndex = protoIndex;
    if (!TF_VERIFY(static_cast<size_t>(instanceIndex) <
            instanceIndices.size())) {
        return SdfPath();
    }
    instanceIndex = instanceIndices[instanceIndex];

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

    if (masterCachePath) {
        const auto rprimPathIt = instIt->second.primMap.find(protoCachePath);
        if (rprimPathIt != instIt->second.primMap.end()) {
            *masterCachePath = rprimPathIt->second.path;

            TF_DEBUG(USDIMAGING_SELECTION).Msg(
                "NI: masterCachePath %s\n", masterCachePath->GetText());
        }
    }

    // intentionally leave instancerIndex as it is, so that
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
        HdSelection::HighlightMode const& highlightMode_,
        HdSelectionSharedPtr const &result_)
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
            // convert to indexPath (add prefix)
            SdfPath indexPath =
                adapter->_ConvertCachePathToIndexPath(it->first);

            // highlight all subtree with instanceIndices.
            // XXX: this seems redundant, but needed for point instancer 
            // highlighting for now. Ideally we should communicate back to point
            // instancer adapter to not use renderIndex
            SdfPathVector const &ids = adapter->_GetRprimSubtree(indexPath);

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
    HdSelection::HighlightMode highlightMode;
    HdSelectionSharedPtr result;
    bool found;
};

/*virtual*/
bool
UsdImagingInstanceAdapter::PopulateSelection(
    HdSelection::HighlightMode const& highlightMode,
    SdfPath const &cachePath,
    UsdPrim const &usdPrim,
    VtIntArray const &instanceIndices,
    HdSelectionSharedPtr const &result)
{
    HD_TRACE_FUNCTION();

    // cachePath points to an actual hydra instancer, so we use it as
    // instancerPath.  The instance path is usdPrim.
    SdfPath instancePath = usdPrim.GetPath();

    TF_DEBUG(USDIMAGING_SELECTION).Msg(
        "PopulateSelection: instance = %s\n", instancePath.GetText());

    _PopulateInstanceSelectionFn populateFn(
        this, cachePath, instancePath, instanceIndices,
            highlightMode, result);

    _RunForAllInstancesToDraw(_GetPrim(cachePath), &populateFn);

    return populateFn.found;
}

/*virtual*/
HdVolumeFieldDescriptorVector
UsdImagingInstanceAdapter::GetVolumeFieldDescriptors(
    UsdPrim const& usdPrim,
    SdfPath const &id,
    UsdTimeCode time) const
{
    if (IsChildPath(id)) {
        // Delegate to prototype adapter and USD prim.
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(),
                                                    id, &instancerContext);
        return proto.adapter->GetVolumeFieldDescriptors(
            _GetPrim(proto.path), id, time);
    }
    return UsdImagingPrimAdapter::GetVolumeFieldDescriptors(
        usdPrim, id, time);
}

/*virtual*/
void
UsdImagingInstanceAdapter::_RemovePrim(SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("Should use overidden ProcessPrimResync/ProcessPrimRemoval");
}

/*virtual*/
GfMatrix4d
UsdImagingInstanceAdapter::GetRelativeInstancerTransform(
    SdfPath const &parentInstancerPath,
    SdfPath const &instancerPath, UsdTimeCode time) const
{
    // This API doesn't do anything for native instancers.
    UsdPrim prim = _GetPrim(instancerPath.GetPrimPath());
    return GetTransform(prim, time);
}

PXR_NAMESPACE_CLOSE_SCOPE

