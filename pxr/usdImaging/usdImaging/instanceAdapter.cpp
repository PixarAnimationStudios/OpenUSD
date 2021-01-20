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

#include <boost/unordered_map.hpp>

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

    const UsdPrim& prototypePrim = prim.GetPrototype();
    if (!TF_VERIFY(
            prototypePrim, "Cannot get prototype prim for <%s>",
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
    // nested instance (meaning "prim" is part of a prototype), parentProxyPath
    // contains the instance proxy path for the instance we're currently in,
    // so we can stitch the full proxy path together.
    TfToken instanceDrawMode;
    TfToken instanceInheritablePurpose;
    std::vector<_InstancerData::PrimvarInfo> inheritedPrimvars;
    {
        instancerChain = { instancePath };
        if (prim.IsInPrototype()) {
            instancerChain.push_back(parentProxyPath);
        }
        SdfPath instanceChainPath =
            _GetPrimPathFromInstancerChain(instancerChain);
        if (UsdPrim instanceUsdPrim = _GetPrim(instanceChainPath)) {
            instanceDrawMode = GetModelDrawMode(instanceUsdPrim);
            instanceInheritablePurpose = GetInheritablePurpose(instanceUsdPrim);
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
                            "parentProxyPath <%s>; isInPrototype %i",
                            instanceChainPath.GetText(),
                            instancePath.GetText(),
                            parentProxyPath.GetText(),
                            int(prim.IsInPrototype()));
        }
    }

    // Check if there's an instance of this prototype with the appropriate
    // inherited attributes that already has an associated hydra instancer.
    SdfPath instancerPath;
    auto range = _prototypeToInstancerMap.equal_range(prototypePrim.GetPath());
    for (auto it = range.first; it != range.second; ++it) {
        _InstancerData& instancerData = _instancerData[it->second];
        // If material ID, draw mode, or inherited primvar set differ,
        // split the instance.
        if (instancerData.materialUsdPath == instancerMaterialUsdPath &&
            instancerData.drawMode == instanceDrawMode &&
            instancerData.inheritedPrimvars == inheritedPrimvars &&
            instancerData.inheritablePurpose == instanceInheritablePurpose) {
            instancerPath = it->second;
            break;
        }
    }

    // If we didn't find a suitable hydra instancer for this prototype,
    // add a new one.
    if (instancerPath.IsEmpty()) {
        _prototypeToInstancerMap.insert(
            std::make_pair(prototypePrim.GetPath(), instancePath));
        instancerPath = instancePath;
    }

    _InstancerData& instancerData = _instancerData[instancerPath];

    // Compute the instancer proxy path (which might be different than the
    // one computed above, if instancePath and instancerPath are different...)
    instancerChain = { instancerPath };
    if (_GetPrim(instancerPath).IsInPrototype()) {
        instancerChain.push_back(parentProxyPath);
    }
    SdfPath instancerProxyPath = _GetPrimPathFromInstancerChain(instancerChain);

    std::vector<UsdPrim> nestedInstances;

    if(instancerData.instancePaths.empty()) {
        instancerData.prototypePath = prototypePrim.GetPath();
        instancerData.materialUsdPath = instancerMaterialUsdPath;
        instancerData.drawMode = instanceDrawMode;
        instancerData.inheritablePurpose = instanceInheritablePurpose;
        instancerData.inheritedPrimvars = inheritedPrimvars;

        UsdImagingInstancerContext ctx = { SdfPath(),
                                           TfToken(),
                                           SdfPath(),
                                           TfToken(),
                                           TfToken(),
                                           instancerAdapter };

        // -------------------------------------------------------------- //
        // Allocate hydra prototype prims for the prims in the USD prototype.
        // -------------------------------------------------------------- //

        UsdPrimRange range(prototypePrim, _GetDisplayPredicate());
        int protoID = 0;
        int primCount = 0;

        for (auto iter = range.begin(); iter != range.end(); ++iter) {
            // If we encounter an instance in this USD prototype, save it aside
            // for a subsequent population pass since we'll need to populate
            // its USD prototype once we're done with this one.
            if (iter->IsInstance()) {
                nestedInstances.push_back(*iter);
                continue;
            }

            // Stitch the current prim-in-prototype path to the instancer proxy
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

            // If we're processing the root prim of the USD prototype, we
            // normally don't allow it to be imageable.  If you directly
            // instance a gprim, the gprim attributes can vary per-instance,
            // meaning you'd need to add one hydra prototype per instance and
            // you'd lose any scalability benefit.
            //
            // Normally we skip this prim and warn (if it's of imageable type),
            // but a few exceptions (like cards mode) will be flagged by the
            // function CanPopulateUsdInstance(), in which case we allow them
            // to be populated.
            //
            // (Note: any prim type that implements CanPopulateUsdInstance will
            // need extensive code support in this adapter as well).
            if (iter->IsPrototype() && primAdapter &&
                !primAdapter->CanPopulateUsdInstance()) {
                TF_WARN("The gprim at path <%s> was directly instanced. "
                        "In order to instance this prim, put the prim under an "
                        "Xform, and instance the Xform parent.",
                        iter->GetPath().GetText());
                continue;
            }
                
            // 
            // Hydra prototype allocation. 
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
            TfToken protoInheritablePurpose = 
                GetInheritablePurpose(instanceProxyPrim);

            const SdfPath protoPath = 
                _InsertProtoPrim(&iter, protoName, protoMaterialId,
                                 protoDrawMode, protoInheritablePurpose, 
                                 instancerPath, primAdapter, instancerAdapter, 
                                 index, &isLeafInstancer);
                    
            // 
            // Update instancer data.
            //
            _ProtoPrim& proto = instancerData.primMap[protoPath];
            if (iter->IsPrototype()) {
                // If the hydra prim we're populating is the root prim of the
                // usd prototype, our usd prim handle should be to the instance,
                // since the prototype root prim doesn't have attributes.
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
        // Add this instancer into the render index
        index->InsertInstancer(instancerPath,
                               _GetPrim(instancerPath),
                               ctx.instancerAdapter);

        // Mark this instancer as having a TrackVariability queued, since
        // we automatically queue it in InsertInstancer.
        instancerData.refreshVariability = true;
    }

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

        // Add this instance's parent path to the instancerData's list of all
        // parent native instances.
        //
        // Note: instead of getting the parent "instancer" path, we get the
        // instance proxy path. So for:
        //     /World/A -> /Prototype_1/B -> /Prototype_2/C,
        // 
        // we have instancer = /World/A, parentProxy = /;
        // instancer = /Prototype_1/B, parentProxy = /World/A;
        // instancer = /Prototype_2/C, parentProxy = /World/A/B.
        // If parentProxy is an instance proxy, take the prim in prototype..
        if (parentProxyPath != SdfPath::AbsoluteRootPath()) {
            UsdPrim parent = _GetPrim(parentProxyPath);
            if (parent.IsInstanceProxy()) {
                parent = parent.GetPrimInPrototype();
            }
            SdfPath parentPath = parent.GetPath();

            SdfPathVector::iterator it = std::lower_bound(
                instancerData.parentInstances.begin(),
                instancerData.parentInstances.end(),
                parentPath);
            if (it == instancerData.parentInstances.end() ||
                *it != parentPath) {
                instancerData.parentInstances.insert(it, parentPath);
            }
        }
    }

    // We're done modifying data structures for the passed in instance,
    // so now it's safe to re-enter this function to populate the
    // nested instances we discovered.
    for (UsdPrim &prim : nestedInstances) {
        _Populate(prim, index, instancerContext, instancerProxyPath);
        instancerData.nestedInstances.push_back(prim.GetPath());
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

        _InstancerData& depInstancerData =
            _instancerData[depInstancerPath];

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

            // Tell UsdImaging to re-run TrackVariability.
            if (!depInstancerData.refreshVariability) {
                depInstancerData.refreshVariability = true;
                index->Refresh(depInstancerPath);
            }
        }

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
    TfToken inheritablePurpose,
    SdfPath instancerPath,
    UsdImagingPrimAdapterSharedPtr const& primAdapter,
    UsdImagingPrimAdapterSharedPtr const& instancerAdapter,
    UsdImagingIndexProxy* index,
    bool *isLeafInstancer)
{
    UsdPrim prim = **it;
    if ((*it)->IsPrototype()) {
        // If the hydra prim we're populating is the prototype root prim,
        // our prim handle should be to the instance, since the prototype
        // root prim doesn't have attributes.
        prim = _GetPrim(instancerPath);
    }

    UsdImagingInstancerContext ctx = { instancerPath,
                                       protoName,
                                       materialUsdPath,
                                       drawMode,
                                       inheritablePurpose,
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

        instrData->refreshVariability = false;
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

    for (SdfPath const& path : instancerData->instancePaths) {
        const UsdPrim instancePrim = _GetPrim(path);
        if (!TF_VERIFY(instancePrim, 
                "Invalid instance <%s> for prototype <%s>",
                path.GetText(),
                instancerData->prototypePath.GetText())) {
            break;
        }
        
        instanceContext->push_back(instancePrim);

        bool continueIteration = true;
        if (!instancePrim.IsInPrototype()) {
            continueIteration = (*fn)(*instanceContext, (*instanceIdx)++);
        }
        else {
            // In this case, instancePrim is a descendent of a prototype prim.
            // Walk up the parent chain to find the prototype prim.
            UsdPrim parentPrototype = instancePrim;
            while (!parentPrototype.IsPrototype()) {
                parentPrototype = parentPrototype.GetParent();
            }

            // Iterate over all instancers corresponding to different
            // variations of this prototype prim, since each instancer
            // will cause another copy of this prototype prim to be drawn.
            auto range =
                _prototypeToInstancerMap.equal_range(parentPrototype.GetPath());
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

    for (SdfPath const& path : instancerData->instancePaths) {
        const UsdPrim instancePrim = _GetPrim(path);
        if (!TF_VERIFY(instancePrim, 
                "Invalid instance <%s> for prototype <%s>",
                path.GetText(),
                instancerData->prototypePath.GetText())) {
            return 0;
        }

        if (!instancePrim.IsInPrototype()) {
            ++drawCount;
        }
        else {
            UsdPrim parentPrototype = instancePrim;
            while (!parentPrototype.IsPrototype()) {
                parentPrototype = parentPrototype.GetParent();
            }

            auto range =
                _prototypeToInstancerMap.equal_range(parentPrototype.GetPath());
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
        for (UsdPrim const& prim : instanceContext) {
            xform = xform * adapter->GetTransform(
                prim, prim.GetPath(), time, ignoreRootTransform);
        }

        // The transform of the USD prototype root will have the scene root
        // transform incorporated, so we need to negate that.
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
        for (UsdPrim const& prim : instanceContext) {
             if (UsdGeomXformable xf = UsdGeomXformable(prim)) {
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
        for (UsdPrim const& prim : instanceContext) {
            if (_GetIsTransformVarying(prim)) {
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
    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();

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
        // Per-instance transforms and inherited primvars are handled by
        // DirtyPrimvar.
        if (requestedBits & HdChangeTracker::DirtyPrimvar) {
            VtMatrix4dArray instanceXforms;
            if (_ComputeInstanceTransforms(prim, &instanceXforms, time)) {
                _MergePrimvar(
                    &primvarDescCache->GetPrimvars(cachePath),
                    HdInstancerTokens->instanceTransform,
                    HdInterpolationInstance);
            }
            for (auto const& ipv : instrData->inheritedPrimvars) {
                VtValue val;
                if (_ComputeInheritedPrimvar(prim, ipv.name, ipv.type,
                                             &val, time)) {
                    _MergePrimvar(&primvarDescCache->GetPrimvars(cachePath),
                                  ipv.name, HdInterpolationInstance,
                                  _UsdToHdRole(ipv.type.GetRole()));
                }
            }
        }
    }
}

HdDirtyBits
UsdImagingInstanceAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    // If this is called on behalf of a hydra prototype (a child prim of
    // a native instancing prim), pass the call through.
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

    if (UsdGeomPrimvarsAPI::CanContainPropertyName(propertyName)) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
                prim, cachePath, propertyName);
    }

    return HdChangeTracker::Clean;
}

void
UsdImagingInstanceAdapter::_ResyncPath(SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index,
                                       bool reload)
{
    // Cache path corresponds to a hydra instancer path that we want to remove
    // or reload.  While we only create one hydra instancer per native instance
    // group, we keep instancerData entries for each level of USD prototype.
    // So we need to traverse up to the top-level native instances, and then
    // go back down through all of the nested native instances, resyncing each.
    //
    // We do this with a breadth first search. instancersToUnload marks where
    // we've been already; instancersToTraverse marks where we still need to
    // visit.  When we visit a node, add it to the unload set and also add
    // any dependencies to the traversal list (such as parent instancers
    // and child instancers).

    SdfPathSet instancersToUnload;
    SdfPathVector instancersToTraverse;

    instancersToTraverse.push_back(cachePath);
    while (!instancersToTraverse.empty()) {
        SdfPath instancePath = instancersToTraverse.back();
        instancersToTraverse.pop_back();

        SdfPath instancerPath;
        auto it = _instanceToInstancerMap.find(instancePath);
        if (it == _instanceToInstancerMap.end()) {
            continue;
        }
        instancerPath = it->second;

        // If this is a new instancer to unload...
        if (instancersToUnload.insert(instancerPath).second)  {
            _InstancerDataMap::iterator instIt =
                _instancerData.find(instancerPath);
            TF_VERIFY(instIt != _instancerData.end());

            // Make sure to visit parents/children!
            instancersToTraverse.insert(instancersToTraverse.end(),
                instIt->second.nestedInstances.begin(),
                instIt->second.nestedInstances.end());
            instancersToTraverse.insert(instancersToTraverse.end(),
                instIt->second.parentInstances.begin(),
                instIt->second.parentInstances.end());
        }
    }

    // Actually resync everything.
    for (SdfPath const& instancer : instancersToUnload) {
        _ResyncInstancer(instancer, index, reload);
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
GfMatrix4d
UsdImagingInstanceAdapter::GetInstancerTransform(UsdPrim const& instancerPrim,
                                                 SdfPath const& instancerPath,
                                                 UsdTimeCode time) const
{
    TRACE_FUNCTION();
    return GetRootTransform();
}

/*virtual*/
SdfPath
UsdImagingInstanceAdapter::GetInstancerId(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const
{
    // If this is called on behalf of an instanced Rprim, return the
    // instancer cache path we've stored for that prim.
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return instancerContext.instancerCachePath;
    }

    // If this is called on behalf of an instancer prim representing a native
    // instancer, return the empty path: native instancers can't have parents.
    return SdfPath::EmptyPath();
}

/*virtual*/
SdfPathVector
UsdImagingInstanceAdapter::GetInstancerPrototypes(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const
{
    HD_TRACE_FUNCTION();

    if (_IsChildPrim(usdPrim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(),
                cachePath, &instancerContext);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return SdfPathVector();
        }
        return proto.adapter->GetInstancerPrototypes(_GetPrim(proto.path), cachePath);
    } else {
        SdfPathVector prototypes;
        if (const _InstancerData* instancerData =
            TfMapLookupPtr(_instancerData, usdPrim.GetPath())) {
            for (_PrimMap::const_iterator i = instancerData->primMap.cbegin();
                 i != instancerData->primMap.cend(); ++i) {
                prototypes.push_back(i->first);
            }
        }
        return prototypes;
    }
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

TfToken 
UsdImagingInstanceAdapter::GetPurpose(
    UsdPrim const& usdPrim, 
    SdfPath const& cachePath,
    TfToken const& instanceInheritablePurpose) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetPurpose(_GetPrim(proto->path), cachePath, 
                                instancerContext.instanceInheritablePurpose);
    }
    return UsdImagingPrimAdapter::GetPurpose(usdPrim, cachePath, TfToken());
}

PxOsdSubdivTags
UsdImagingInstanceAdapter::GetSubdivTags(UsdPrim const& usdPrim,
                                         SdfPath const& cachePath,
                                         UsdTimeCode time) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        // Note that the proto group in this proto has not yet been
        // updated with new instances at this point.
        return proto->adapter->GetSubdivTags(
                _GetPrim(proto->path), cachePath, time);
    }
    return BaseAdapter::GetSubdivTags(usdPrim, cachePath, time);
}

VtValue
UsdImagingInstanceAdapter::GetTopology(UsdPrim const& usdPrim,
                                       SdfPath const& cachePath,
                                       UsdTimeCode time) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetTopology(
                _GetPrim(proto->path), cachePath, time);
    }
    return BaseAdapter::GetTopology(usdPrim, cachePath, time);
}

/*virtual*/
HdCullStyle 
UsdImagingInstanceAdapter::GetCullStyle(UsdPrim const& usdPrim,
                                        SdfPath const& cachePath,
                                        UsdTimeCode time) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        // Note that the proto group in this proto has not yet been
        // updated with new instances at this point.
        return proto->adapter->GetCullStyle(
                _GetPrim(proto->path), cachePath, time);
    }
    return BaseAdapter::GetCullStyle(usdPrim, cachePath, time);
}

/*virtual*/
GfRange3d 
UsdImagingInstanceAdapter::GetExtent(UsdPrim const& usdPrim, 
                                     SdfPath const& cachePath, 
                                     UsdTimeCode time) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetExtent(
                _GetPrim(proto->path), cachePath, time);
    }
    return BaseAdapter::GetExtent(usdPrim, cachePath, time);
}

/*virtual*/
bool 
UsdImagingInstanceAdapter::GetVisible(UsdPrim const& usdPrim, 
                                      SdfPath const& cachePath,
                                      UsdTimeCode time) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetVisible(
                _GetPrim(proto->path), cachePath, time);
    }
    return BaseAdapter::GetVisible(usdPrim, cachePath, time);
}

/*virtual*/
bool
UsdImagingInstanceAdapter::GetDoubleSided(UsdPrim const& usdPrim, 
                                          SdfPath const& cachePath, 
                                          UsdTimeCode time) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetDoubleSided(
                _GetPrim(proto->path), cachePath, time);
    }

    return BaseAdapter::GetDoubleSided(usdPrim, cachePath, time);
}

/*virtual*/
GfMatrix4d 
UsdImagingInstanceAdapter::GetTransform(UsdPrim const& prim, 
                                        SdfPath const& cachePath,
                                        UsdTimeCode time,
                                        bool ignoreRootTransform) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(prim, cachePath, &proto, &instancerContext)) {
        // Note that the proto group in this proto has not yet been
        // updated with new instances at this point.
        return proto->adapter->GetTransform(
            _GetPrim(proto->path), cachePath, time, ignoreRootTransform);
    }

    return BaseAdapter::GetTransform(prim, cachePath, time, ignoreRootTransform);
}

/*virtual*/
SdfPath
UsdImagingInstanceAdapter::GetMaterialId(UsdPrim const& usdPrim, 
                                      SdfPath const& cachePath, 
                                      UsdTimeCode time) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {

        const SdfPath materialId = proto->adapter->GetMaterialId(
                _GetPrim(proto->path), cachePath, time);
        if (!materialId.IsEmpty()) {
            return materialId;
        } else {
            // child prim have doesn't one? fall back on instancerContext's
            // value
            return instancerContext.instancerMaterialUsdPath;
        }
    }

    return BaseAdapter::GetMaterialId(usdPrim, cachePath, time);
}

/*virtual*/
HdExtComputationInputDescriptorVector
UsdImagingInstanceAdapter::GetExtComputationInputs(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetExtComputationInputs(
                _GetPrim(proto->path), cachePath, &instancerContext);
    }
    return BaseAdapter::GetExtComputationInputs(usdPrim, cachePath, nullptr);
}

/*virtual*/
HdExtComputationOutputDescriptorVector
UsdImagingInstanceAdapter::GetExtComputationOutputs(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetExtComputationOutputs(
                _GetPrim(proto->path), cachePath, &instancerContext);
    }
    return BaseAdapter::GetExtComputationOutputs(usdPrim, cachePath, nullptr);
}

/*virtual*/
HdExtComputationPrimvarDescriptorVector
UsdImagingInstanceAdapter::GetExtComputationPrimvars(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    HdInterpolation interpolation,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetExtComputationPrimvars(
                _GetPrim(proto->path), cachePath, interpolation,
                &instancerContext);
    }
    return BaseAdapter::GetExtComputationPrimvars(usdPrim, cachePath, 
            interpolation, nullptr);
}

/*virtual*/
VtValue 
UsdImagingInstanceAdapter::GetExtComputationInput(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    TfToken const& name,
    UsdTimeCode time,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetExtComputationInput(
                _GetPrim(proto->path), cachePath, name, time,
                &instancerContext);
    }
    return BaseAdapter::GetExtComputationInput(usdPrim, cachePath, name, time,
            nullptr);
}

/*virtual*/
std::string 
UsdImagingInstanceAdapter::GetExtComputationKernel(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    const UsdImagingInstancerContext* /*unused*/) const
{
    UsdImagingInstancerContext instancerContext;
    _ProtoPrim const *proto;
    if (_GetProtoPrimForChild(usdPrim, cachePath, &proto, &instancerContext)) {
        return proto->adapter->GetExtComputationKernel(
                _GetPrim(proto->path), cachePath, &instancerContext);
    }
    return BaseAdapter::GetExtComputationKernel(usdPrim, cachePath, nullptr);
}

/*virtual*/
VtValue
UsdImagingInstanceAdapter::GetInstanceIndices(
    UsdPrim const& instancerPrim,
    SdfPath const& instancerCachePath,
    SdfPath const& prototypeCachePath,
    UsdTimeCode time) const
{
    if (_IsChildPrim(instancerPrim, instancerCachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const *proto;
        if (_GetProtoPrimForChild(instancerPrim, instancerCachePath, &proto,
                &instancerContext)) {
            return proto->adapter->GetInstanceIndices(_GetPrim(proto->path),
                instancerCachePath, prototypeCachePath, time);
        }

        return BaseAdapter::GetInstanceIndices(
                instancerPrim, instancerCachePath, prototypeCachePath, time);
    }

    // XXX: This is called once per hydra prototype, since each prototype needs
    // a full set of indices at each level.  This is wasteful since the indices
    // here are the same for all prototypes.  The previous behavior cached the
    // indices in the value cache; we might want to investigate caching here.
    if (_InstancerData const* instrData =
               TfMapLookupPtr(_instancerData, instancerCachePath)) {
        VtIntArray indices = _ComputeInstanceMap(
                instancerPrim, *instrData, time);
        return VtValue(indices);
    }

    return BaseAdapter::GetInstanceIndices(
                instancerPrim, instancerCachePath, prototypeCachePath, time);
}

/*virtual*/
VtValue 
UsdImagingInstanceAdapter::Get(UsdPrim const& usdPrim, 
                               SdfPath const& cachePath,
                               TfToken const &key,
                               UsdTimeCode time) const
{
    TRACE_FUNCTION();

    if (_IsChildPrim(usdPrim, cachePath)) {
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(usdPrim.GetPath(),
                                                cachePath,
                                                &instancerContext);
        if (!TF_VERIFY(proto.adapter, "%s", cachePath.GetText())) {
            return VtValue();
        }
        return proto.adapter->Get(
                _GetPrim(proto.path), cachePath, key, time);
    } else if (_InstancerData const* instrData =
        TfMapLookupPtr(_instancerData, usdPrim.GetPath())) {

        if (key == HdInstancerTokens->instanceTransform) {
            VtMatrix4dArray instanceXforms;
            if (_ComputeInstanceTransforms(usdPrim, &instanceXforms, time)) {
                return VtValue(instanceXforms);
            }
        }
        
        for (auto const& ipv : instrData->inheritedPrimvars) {
            VtValue val;
            if (ipv.name == key && 
                _ComputeInheritedPrimvar(usdPrim, ipv.name, ipv.type,
                                         &val, time)) {
                return val;
            }
        }
            
    }
    return BaseAdapter::Get(usdPrim, cachePath, key, time);
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
    for (auto const& pair : instIt->second.primMap) {
        // Call ProcessRemoval here because we don't want them to reschedule for
        // resync, that will happen when the instancer is resync'd.
        pair.second.adapter->ProcessPrimRemoval(pair.first, index);
    }

    // Remove this instancer's entry from the USD prototype -> instancer map.
    auto range =_prototypeToInstancerMap.equal_range(
        instIt->second.prototypePath);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == instancerPath) {
            _prototypeToInstancerMap.erase(it);
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

    for (SdfPath const& path : instancePaths) {
        auto it = _instanceToInstancerMap.find(path);
        _instanceToInstancerMap.erase(it);
    }

    // Repopulate the instancer's previous instances. Those that don't exist
    // anymore will be ignored, while those that still exist will be
    // pushed back into this adapter and refreshed.
    if (repopulate) {
        for (SdfPath const &path : instancePaths) {
            UsdPrim prim = _GetPrim(path);
            if (prim && prim.IsActive() && !prim.IsInPrototype()) {
                index->Repopulate(path);
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
    TfToken inheritablePurpose;

    if (it != _instancerData.end()) {
        _PrimMap::const_iterator primIt = it->second.primMap.find(cachePath);
        if (primIt == it->second.primMap.end()) {
            return EMPTY;
        }
        instancerCachePath = instancerPath;
        materialUsdPath = it->second.materialUsdPath;
        drawMode = it->second.drawMode;
        inheritablePurpose = it->second.inheritablePurpose;
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
                inheritablePurpose =
                    pathInstancerDataPair.second.inheritablePurpose;
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
    ctx->instanceInheritablePurpose = inheritablePurpose;
    ctx->childName = TfToken();
    // Note: use a null adapter here.  The UsdImagingInstancerContext is
    // not really used outside of population.  We should clean this up and
    // remove these contexts from everything outside of population.
    ctx->instancerAdapter = UsdImagingPrimAdapterSharedPtr();

    return *r;
}

bool 
UsdImagingInstanceAdapter::_GetProtoPrimForChild(
    UsdPrim const& usdPrim,
    SdfPath const& cachePath,
    _ProtoPrim const** proto,
    UsdImagingInstancerContext* ctx) const
{
    if (!_IsChildPrim(usdPrim, cachePath)) {
        return false;
    }

    *proto = &_GetProtoPrim(usdPrim.GetPath(),cachePath, ctx);

    if (!TF_VERIFY((*proto)->adapter, "%s", cachePath.GetText())) {
        return false;
    }

    return true;
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
        for (UsdPrim const& prim : instanceContext) {
            if (!adapter->GetVisible(prim, prim.GetPath(), time)) {
                return false;
            }
        }
        return true;
    }

    bool IsVisibilityVarying(const std::vector<UsdPrim>& instanceContext)
    {
        for (UsdPrim const& prim : instanceContext) {
            if (_GetIsVisibilityVarying(prim)) {
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
        for (UsdPrim const& prim : instanceContext) {
            if (!adapter->GetVisible(prim, prim.GetPath(), time)) {
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
    TRACE_FUNCTION();

    VtIntArray indices(0);

    _ComputeInstanceMapFn computeInstanceMapFn(
        this, time, &instrData.visibility, &indices);
    _RunForAllInstancesToDraw(instancerPrim, &computeInstanceMapFn);
    return indices;
}

struct UsdImagingInstanceAdapter::_GetScenePrimPathFn
{
    _GetScenePrimPathFn(
        const UsdImagingInstanceAdapter* adapter_,
        int instanceIndex_,
        const SdfPath &protoPath_)
        : adapter(adapter_)
        , instanceIndex(instanceIndex_)
        , protoPath(protoPath_)
    { }

    void Initialize(size_t numInstances)
    {
    }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        // If this iteration is the right instance index, compose all the USD
        // prototype paths together to get the instance proxy path.  Include the
        // proto path (of the child prim), if one was provided.
        if (instanceIdx == instanceIndex) {
            SdfPathVector instanceChain;
            // To get the correct prim-in-prototype, we need to add the
            // prototype path to the instance chain.  However, there's a case in
            // _Populate where we populate prims that are just a USD prototype
            // (used by e.g. cards).  In this case, the hydra proto path is
            // overridden to be the path of the USD instance, and we don't want
            // to add it to the instance chain since instanceContext.front
            // would duplicate it.
            UsdPrim p = adapter->_GetPrim(protoPath);
            if (p && !p.IsInstance()) {
                instanceChain.push_back(protoPath);
            }
            for (UsdPrim const& prim : instanceContext) {
                instanceChain.push_back(prim.GetPath());
            }
            primPath = adapter->_GetPrimPathFromInstancerChain(instanceChain);
            return false;
        }
        return true;
    }

    const UsdImagingInstanceAdapter* adapter;
    const size_t instanceIndex;
    const SdfPath& protoPath;
    SdfPath primPath;
};

/* virtual */
SdfPath
UsdImagingInstanceAdapter::GetScenePrimPath(
    SdfPath const& cachePath,
    int instanceIndex,
    HdInstancerContext *instancerContext) const
{
    HD_TRACE_FUNCTION();

    // For child prims (hydra prototypes) and USD instances, the process is
    // the same: find the associated hydra instancer, and use the instance
    // index to look up the composed instance path.  They differ based on
    // whether you append a hydra proto path, and how you find the
    // hydra instancer.
    UsdPrim usdPrim = _GetPrim(cachePath.GetAbsoluteRootOrPrimPath());
    if (_IsChildPrim(usdPrim, cachePath)) {

        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "GetScenePrimPath: instance proto = %s\n", cachePath.GetText());

        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(
            cachePath.GetAbsoluteRootOrPrimPath(),
            cachePath, &instancerContext);

        _InstancerData const* instrData =
            TfMapLookupPtr(_instancerData, instancerContext.instancerCachePath);
        if (!instrData) {
            return SdfPath();
        }

        UsdPrim instancerPrim = _GetPrim(instancerContext.instancerCachePath);

        // Translate from hydra instance index to USD (since hydra filters out
        // invisible instances).
        VtIntArray indices = _ComputeInstanceMap(instancerPrim, *instrData, 
            _GetTimeWithOffset(0.0));

        instanceIndex = indices[instanceIndex];

        _GetScenePrimPathFn primPathFn(this, instanceIndex, proto.path);
        _RunForAllInstancesToDraw(instancerPrim, &primPathFn);
        return primPathFn.primPath;
    } else {

        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "GetScenePrimPath: instance = %s\n", cachePath.GetText());

        SdfPath const* instancerPath =
            TfMapLookupPtr(_instanceToInstancerMap, cachePath);
        if (instancerPath == nullptr) {
            return SdfPath();
        }
        _InstancerData const* instrData =
            TfMapLookupPtr(_instancerData, *instancerPath);
        if (instrData == nullptr) {
            return SdfPath();
        }
        _GetScenePrimPathFn primPathFn(this, instanceIndex,
            SdfPath::EmptyPath());
        _RunForAllInstancesToDraw(_GetPrim(*instancerPath), &primPathFn);
        return primPathFn.primPath;
    }

    return SdfPath();
}

struct UsdImagingInstanceAdapter::_PopulateInstanceSelectionFn
{
    _PopulateInstanceSelectionFn(
            UsdPrim const& usdPrim_,
            int const hydraInstanceIndex_,
            VtIntArray const& parentInstanceIndices_,
            _InstancerData const* instrData_,
            VtIntArray const& drawnIndices_,
            UsdImagingInstanceAdapter const* adapter_,
            HdSelection::HighlightMode const& highlightMode_,
            HdSelectionSharedPtr const& result_)
        : usdPrim(usdPrim_)
        , hydraInstanceIndex(hydraInstanceIndex_)
        , parentInstanceIndices(parentInstanceIndices_)
        , instrData(instrData_)
        , drawnIndices(drawnIndices_)
        , adapter(adapter_)
        , highlightMode(highlightMode_)
        , result(result_)
        , added(false)
    {}

    void Initialize(size_t numInstances)
    {
        // In order to check selectionPath against the instance context,
        // we need to decompose selectionPath into a path vector.  We can't
        // just assemble instanceContext into a proxy prim because
        // selectionPath might point to something inside a USD prototype.
        // See comment in operator().
        UsdPrim p = usdPrim;
        while (p.IsInstanceProxy()) {
            selectionPathVec.push_front(p.GetPrimInPrototype().GetPath());
            do {
                p = p.GetParent();
            } while (!p.IsInstance());
        }
        selectionPathVec.push_front(p.GetPath());
    }

    bool operator()(
        const std::vector<UsdPrim>& instanceContext, size_t instanceIdx)
    {
        // To illustrate the below algorithm, imagine the following scene:
        // /World/A, /World/A2 -> /__Prototype_1
        // /__Prototype_1/B -> /__Prototype_2
        // /__Prototype_2/C,D are gprims.
        // We want to be able to select /World/A/B as well as /__Prototype_1/B.
        // ... to do this, we break the selection path down into components
        // in PopulateSelection: /World/A, /__Prototype_1/B.
        //
        // The matrix of things we can select:
        // 1.) One instance, one gprim (e.g. /World/A/B/C):
        //     - selection context [/World/A, /__Prototype_1/B, /__Prototype_2/C]
        //     - instance context [/World/A, /__Prototype_1/B]
        //     /__Prototype_2/C needs to be checked against primMap.
        // 2.) One instance, multiple gprims (e.g. /World/A/B):
        //     - selection context [/World/A, /__Prototype_1/B]
        //     - instance context [/World/A, /__Prototype_1/B]
        // 3.) Multiple instances, one gprim (e.g. /__Prototype_1/B/C)
        //     - selection context [/__Prototype_1/B, /__Prototype_2/C]
        //     - instance context [/World/A, /__Prototype_1/B]
        //     - instance context [/World/A2, /__Prototype_1/B]
        //     /__Prototype_2/C needs to be checked against primMap.
        // 4.) Multiple instances, multiple gprims (e.g. /__Prototype_1/B)
        //     - selection context [/__Prototype_1/B]
        //     - instance context [/World/A, /__Prototype_1/B]
        //     - instance context [/World/A2, /__Prototype_1/B]
        //
        // The algorithm, then:
        // - If selectionContext[0] is not in instanceContext, continue.
        // - Define start as selectionContext[0] = instanceContext[start]
        // - If selectionContext[1...N] = instanceContext[start+1 ... start+N],
        //   highlight all protos of this instance.
        // - If selectionContext[1...X] = instanceContext[start+1 ... start+X],
        //   and len(instanceContext) = start+X+1, selectionContext[X+1...N]
        //   is a residual path: probably a gprim path, but possibly an
        //   instance proxy path in the case of nested PI.  The residual path
        //   will select a certain proto/set of protos to highlight, for this
        //   instance.
        // - Otherwise, highlight nothing.

        // Zipper compare instance and selection paths.
        size_t instanceCount, selectionCount;
        for (instanceCount = 0, selectionCount = 0;
             instanceCount < instanceContext.size() &&
             selectionCount < selectionPathVec.size();
             ++instanceCount) {
                // instanceContext is innermost-first, and selectionPathVec
                // outermost-first, so we need to flip the paths index.
                size_t instanceContextIdx =
                    instanceContext.size() - instanceCount - 1;
            if (instanceContext[instanceContextIdx].GetPath().HasPrefix(
                  selectionPathVec[selectionCount])) {
                ++selectionCount;
            } else if (selectionCount != 0) {
                return true;
            }
        }

        // Create an instanceIndices that selects this instance, for use if the
        // paths match. Ignore parentInstanceIndices since instanceAdapter can't
        // have a parent.
        // Note: "instanceIdx" is an index into the list of USD instances, but
        // hydra's index buffer filters out invisible instances.  This means
        // we need to translate here for the correct hydra encoding.
        VtIntArray instanceIndices;
        for (size_t i=0; i < drawnIndices.size(); i++) {
            if (drawnIndices[i] == (int)instanceIdx) {
                instanceIndices.push_back(i);
                break;
            }
        }

        if (selectionCount == selectionPathVec.size()) {
            for (auto const& pair : instrData->primMap) {
                UsdPrim prefixPrim =
                    adapter->_GetPrim(pair.first.GetAbsoluteRootOrPrimPath());
                added |= pair.second.adapter->PopulateSelection(
                    highlightMode, pair.first, prefixPrim,
                    hydraInstanceIndex, instanceIndices, result);
            }
        }
        else if (selectionCount != 0 &&
                 instanceCount == instanceContext.size()) {
            // Compose the remainder of the selection path into a (possibly
            // instance proxy) usd prim, and use that as the selection prim.
            // This prim can either be a parent of any given proto, or a child
            // (in the case of a selection inside a point instancer scope).
            SdfPathVector residualPathVec(
                selectionPathVec.rbegin(),
                selectionPathVec.rend() - selectionCount);
            SdfPath residualPath =
                adapter->_GetPrimPathFromInstancerChain(residualPathVec);
            UsdPrim selectionPrim = adapter->_GetPrim(residualPath);

            for (auto const& pair : instrData->primMap) {
                if (pair.second.path.HasPrefix(
                    selectionPathVec[selectionCount])) {
                    // If the selection path is a prefix of this proto,
                    // use a prefix prim to fully select the proto, in case
                    // it's a gprim with name mangling.
                    selectionPrim = adapter->_GetPrim(
                        pair.first.GetAbsoluteRootOrPrimPath());
                } else if (!selectionPathVec[selectionCount].HasPrefix(
                           pair.second.path)) {
                    // If the selection path isn't a prefix of the proto,
                    // we need the proto to be a prefix of the selection path
                    // (in which case we pass the residualPath selection prim,
                    // below, to support sub-object selection of PI prims).
                    //
                    // If the latter is *not* the case, skip this iteration.
                    continue;
                }
                added |= pair.second.adapter->PopulateSelection(
                    highlightMode, pair.first, selectionPrim,
                    hydraInstanceIndex, instanceIndices, result);
            }
        }
        return true;
    }

    UsdPrim const& usdPrim;
    int const hydraInstanceIndex;
    VtIntArray const& parentInstanceIndices;
    _InstancerData const* instrData;
    VtIntArray const& drawnIndices;
    UsdImagingInstanceAdapter const* adapter;
    HdSelection::HighlightMode const& highlightMode;
    HdSelectionSharedPtr const& result;
    std::deque<SdfPath> selectionPathVec;
    bool added;
};

/*virtual*/
bool
UsdImagingInstanceAdapter::PopulateSelection(
    HdSelection::HighlightMode const& highlightMode,
    SdfPath const &cachePath,
    UsdPrim const &usdPrim,
    int const hydraInstanceIndex,
    VtIntArray const &parentInstanceIndices,
    HdSelectionSharedPtr const &result) const
{
    HD_TRACE_FUNCTION();

    // cachePath will either point to a gprim-in-prototype (which ends up here
    // because of adapter hijacking), or a USD native instance prim. We can
    // distinguish between the two with _IsChildPrim.
    if (_IsChildPrim(
            _GetPrim(cachePath.GetAbsoluteRootOrPrimPath()), cachePath)) {
        // If cachePath points to a gprim, name mangling dictates the instancer
        // path is the prim path above it.  If cachePath points to a child
        // point instancer, there's not a good way to recover the instancer
        // path; this is reflected in the fact that _GetProtoPrim has a case
        // for this to walk all of the instancer datas looking for a match.
        UsdImagingInstancerContext instancerContext;
        _ProtoPrim const& proto = _GetProtoPrim(
            cachePath.GetAbsoluteRootOrPrimPath(),
            cachePath, &instancerContext);

        if (!proto.adapter) {
            return false;
        }
        _InstancerData const* instrData =
            TfMapLookupPtr(_instancerData, instancerContext.instancerCachePath);
        if (instrData == nullptr) {
            return false;
        }

        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "PopulateSelection: proto = %s instancer = %s\n",
            cachePath.GetText(), instancerContext.instancerCachePath.GetText());

        // If we're getting called on behalf of a child prim, we're inside a
        // USD prototype and need to add a selection for that child prim for all
        // USD instances of the prototype.  (If we're called on behalf of an
        // instance proxy, we fall into the else case below; and if we're called
        // on an un-instanced prim something has gone wrong). If the selection
        // path is a prefix of the proto path inside the USD prototype, we can
        // highlight the whole proto; otherwise, we should pass the full
        // selection path to the child adapter (e.g. to process partial
        // PI selection).

        UsdPrim selectionPrim;
        if (proto.path.HasPrefix(usdPrim.GetPath())) {
            // Since we're doing a full highlight anyway, we override the
            // selection prim to something we know will always succeed for
            // gprims (despite name mangling).
            selectionPrim = _GetPrim(cachePath.GetAbsoluteRootOrPrimPath());
        } else if (usdPrim.GetPath().HasPrefix(proto.path)) {
            selectionPrim = usdPrim;
        } else {
            return false;
        }

        // Compose the instance indices.
        // Add the native instances to the parentInstanceIndices we pass
        // down.  We're ignoring parentInstanceIndices here since we
        // know the instance adapter can't have a parent.
        VtIntArray instanceIndices;
        for (size_t i = 0; i < instrData->numInstancesToDraw; ++i) {
            instanceIndices.push_back(i);
        }

        // Populate selection.
        return proto.adapter->PopulateSelection(
            highlightMode, cachePath, selectionPrim,
            hydraInstanceIndex, instanceIndices, result);
    } else {
        SdfPath const* instancerPath =
            TfMapLookupPtr(_instanceToInstancerMap, cachePath);
        if (instancerPath == nullptr) {
            return false;
        }
        _InstancerData const* instrData =
            TfMapLookupPtr(_instancerData, *instancerPath);
        if (instrData == nullptr) {
            return false;
        }
        TF_DEBUG(USDIMAGING_SELECTION).Msg(
            "PopulateSelection: instance = %s instancer = %s\n",
            cachePath.GetText(), instancerPath->GetText());

        UsdPrim instancerPrim = _GetPrim(*instancerPath);

        VtIntArray indices = _ComputeInstanceMap(instancerPrim, *instrData, 
            _GetTimeWithOffset(0.0));

        _PopulateInstanceSelectionFn populateFn(usdPrim, 
                hydraInstanceIndex,
            parentInstanceIndices, instrData, indices, 
            this, highlightMode, result);
        _RunForAllInstancesToDraw(instancerPrim, &populateFn);

        return populateFn.added;
    }

    return false;
}

/*virtual*/
HdVolumeFieldDescriptorVector
UsdImagingInstanceAdapter::GetVolumeFieldDescriptors(
    UsdPrim const& usdPrim,
    SdfPath const &id,
    UsdTimeCode time) const
{
    if (IsChildPath(id)) {
        // Delegate to child adapter and USD prim.
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
    SdfPath const &instancerPath, 
    UsdTimeCode time) const
{
    // This API doesn't do anything for native instancers.
    UsdPrim prim = _GetPrim(instancerPath.GetPrimPath());
    return BaseAdapter::GetTransform(prim, prim.GetPath(), time);
}

PXR_NAMESPACE_CLOSE_SCOPE

