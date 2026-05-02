//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/instanceProxyViewSceneIndex.h"

#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/instanceProxySchema.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/trace/trace.h"

#include <optional>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (UsdNiPropagatedPrototypes)
);

// Types used in the implementation of HdInstanceProxyViewSceneIndex.
namespace
{

using _InstancerChain = TfSmallVector<SdfPath, 4>;
struct _InstancingContext {
    SdfPath prototypePath;
    HdSceneIndexPrim prototypePrim;
    _InstancerChain instancers; // Ordered from outermost to innermost.
};

} // anon

///////////////////////////////////////////////////////////////////////////////
//
//                HdInstanceProxyViewSceneIndex::_Impl
//
///////////////////////////////////////////////////////////////////////////////

struct HdInstanceProxyViewSceneIndex::_Impl
{
    _Impl(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    : inputSi(inputSceneIndex)
    {
        // Update tracking by traversing the input scene to find all instancer
        // prims.
        HdSceneIndexPrimView view(inputSceneIndex, SdfPath::AbsoluteRootPath());
        for (const SdfPath &primPath : view) {
            const HdSceneIndexPrim prim = inputSceneIndex->GetPrim(primPath);
            if (prim.primType == HdPrimTypeTokens->instancer) {
                UpdateTracking(primPath);
            }
        }
    }

    // ------------------------------------------------------------------------
    // Public API
    //
    bool
    IsInstanceProxy(
        const SdfPath &primPath,
        std::optional<_InstancingContext> *_InstancingContext = nullptr) const;

    bool
    IsOutermostInstance(
        const SdfPath &primPath,
        SdfPath *prototypePath = nullptr) const;
    
    bool
    IsInstance(
        const SdfPath &primPath,
        SdfPath *prototypePath = nullptr);
    
    bool
    IsInstancer(const SdfPath &primPath) const;

    SdfPathVector
    ComputeRemappedChildPrimPaths(
        const SdfPath &prototypePath,
        const SdfPath &prefixToReplaceWith) const;

    SdfPathVector
    ComputeChildPrimPathsForInstanceProxy(const SdfPath &primPath) const;

    void UpdateTracking(const SdfPath &instancerPath);

    void RemoveTracking(const SdfPath &instancerPath);
    
private:
    // ------------------------------------------------------------------------
    // Private API
    //
    bool
    _IsDescendantOfOuterInstance(
        const SdfPath &primPath, SdfPath *outerInstancePath) const;
    
    bool
    _IsDescendantOfInstancer(const SdfPath &primPath) const;
    
    std::pair<SdfPath, _InstancerChain>
    _ComputePrototypePathAndInstancers(
        const SdfPath &instanceProxyPrimPath,
        const SdfPath &outerInstancePath) const;

private:
    // ------------------------------------------------------------------------
    // Data
    //
    struct InstanceInfo {
        SdfPath instancerPath;
        SdfPath prototypePath;
        int instanceIndex;
    };
    using InstanceInfoMap = std::unordered_map<SdfPath, InstanceInfo, TfHash>;
    
    const HdSceneIndexBaseRefPtr inputSi;

    // Map from any Hydra instance prim (i.e. prim with `instance` data source)
    // to its instancer, prototype and instance index.
    // Note that this includes nested instance prims that are descendants of
    // instancer prim(s).
    InstanceInfoMap instanceInfoMap;

    // Track leaf prims with an instance data source that aren't under an
    // instancer prim to aid query processing.
    SdfPathSet outerInstancePrimPaths;

    // Track all prims of type `instancer` to aid notice processing.
    SdfPathSet instancerPrimPaths;
};

bool
HdInstanceProxyViewSceneIndex::_Impl::IsInstanceProxy(
    const SdfPath &primPath,
    std::optional<_InstancingContext> *_instancingContext /* = nullptr */) const
{
    SdfPath outerInstancePath;
    if (!_IsDescendantOfOuterInstance(primPath, &outerInstancePath)) {
        return false;
    }

    const auto [prototypePath, instancers] =
        _ComputePrototypePathAndInstancers(primPath, outerInstancePath);

    const HdSceneIndexPrim prototypePrim = inputSi->GetPrim(prototypePath);
    if (!prototypePrim) {
        TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
            "Path <%s> is not a valid instance proxy prim path because "
            "the computed prototype path <%s> does not exist.\n",
            primPath.GetText(), prototypePath.GetText());
        return false;
    }

    if (_instancingContext) {
        *_instancingContext =
            _InstancingContext{prototypePath, prototypePrim, instancers};
    }

    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "Path <%s> is an instance proxy prim with prototype <%s> and "
        "%zu instancer(s).\n",
        primPath.GetText(), prototypePath.GetText(), instancers.size());
    return true;
}

bool
HdInstanceProxyViewSceneIndex::_Impl::IsOutermostInstance(
    const SdfPath &primPath,
    SdfPath *prototypePath /* = nullptr */) const
{
    if (outerInstancePrimPaths.count(primPath) == 0) {
        return false;
    }

    if (prototypePath) {
        const auto it = instanceInfoMap.find(primPath);
        if (it == instanceInfoMap.end()) {
            TF_CODING_ERROR(
                "Outer instance prim path <%s> is missing from instance info "
                "map. This indicates a bug in tracking logic.",
                primPath.GetText());
            *prototypePath = SdfPath();
        } else {
            *prototypePath = it->second.prototypePath;
        }
    }
    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "Path <%s> is an outer instance prim.\n", primPath.GetText());
    return true;
}

bool
HdInstanceProxyViewSceneIndex::_Impl::IsInstance(
    const SdfPath &primPath,
    SdfPath *prototypePath /* = nullptr */)
{
    const auto it = instanceInfoMap.find(primPath);
    if (it == instanceInfoMap.end()) {
        return false;
    }
    
    if (prototypePath) {
        *prototypePath = it->second.prototypePath;
    }
    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "Path <%s> is an instance prim.\n", primPath.GetText());
    return true;
}

bool
HdInstanceProxyViewSceneIndex::_Impl::IsInstancer(const SdfPath &primPath) const
{
    return instancerPrimPaths.count(primPath) > 0;
}

SdfPathVector
HdInstanceProxyViewSceneIndex::_Impl::ComputeRemappedChildPrimPaths(
    const SdfPath &prototypePath, const SdfPath &prefixToReplaceWith) const
{
    const SdfPathVector childPaths =
        inputSi->GetChildPrimPaths(prototypePath);

    SdfPathVector remappedChildPaths;
    remappedChildPaths.reserve(childPaths.size());

    for (const auto &childPath : childPaths) {
        // XXX Is there a better way to exclude these prims?
        if (childPath.GetNameToken() == _tokens->UsdNiPropagatedPrototypes) {
            continue;
        }

        // Remap the prototype child path to the instance proxy child path by
        // replacing the prototype path prefix with the instance proxy path.
        remappedChildPaths.push_back(
            prefixToReplaceWith.AppendChild(childPath.GetNameToken()));
    }
    return remappedChildPaths;
}

void
HdInstanceProxyViewSceneIndex::_Impl::UpdateTracking(
    const SdfPath &instancerPath)
{
    TRACE_FUNCTION();
    const auto instancerPrim = inputSi->GetPrim(instancerPath);
    const HdInstancerTopologySchema topologySchema =
        HdInstancerTopologySchema::GetFromParent(instancerPrim.dataSource);
    const HdPathArrayDataSourceHandle instanceLocationsDs =
        topologySchema.GetInstanceLocations();

    if (!instanceLocationsDs) {
        RemoveTracking(instancerPath);
        return;
    }

    instancerPrimPaths.insert(instancerPath);
    
    const VtArray<SdfPath> instanceLocations =
        instanceLocationsDs->GetTypedValue(0.0);

    for (const auto &instancePath : instanceLocations) {
        const auto instancePrim = inputSi->GetPrim(instancePath);
        const HdInstanceSchema instanceSchema =
            HdInstanceSchema::GetFromParent(instancePrim.dataSource);
        const HdIntDataSourceHandle prototypeIndexDs =
            instanceSchema.GetPrototypeIndex();
        const HdIntDataSourceHandle instanceIndexDs =
            instanceSchema.GetInstanceIndex();

        if (!prototypeIndexDs || !instanceIndexDs) {
            instanceInfoMap.erase(instancePath);
            outerInstancePrimPaths.erase(instancePath);
            continue;
        }

        const int prototypeIndex = prototypeIndexDs->GetTypedValue(0);
        const int instanceIndex = instanceIndexDs->GetTypedValue(0);
        const auto prototypePathsDs = topologySchema.GetPrototypes();
        const VtArray<SdfPath> prototypePaths =
            prototypeIndexDs
            ? prototypePathsDs->GetTypedValue(0.0)
            : VtArray<SdfPath>();
        const SdfPath prototypePath =
            (prototypeIndex >= 0 && prototypeIndex < (int)prototypePaths.size())
            ? prototypePaths[prototypeIndex]
            : SdfPath();

        TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
            "Tracking instance prim <%s> with instancer <%s>, "
            "prototype <%s>, instance index %d\n",
            instancePath.GetText(), instancerPath.GetText(),
            prototypePath.GetText(), instanceIndex);

        instanceInfoMap[instancePath] =
            {instancerPath, prototypePath, instanceIndex};
        
        if (!_IsDescendantOfInstancer(instancePath)) {
            // This is an outer instance.
            outerInstancePrimPaths.insert(instancePath);
        }
        
        // XXX instancedBy doesn't seem to be a reliable way to determine if
        // a hydra instance prim is "outside" an instancer. In the nested
        // instancing case, the prototype prim for the outer instancer is
        // itself an instance prim, but it doesn't have the instancedBy schema.
        // Unclear why. For now, using the "not descendant of any instancer"
        // heuristic to determine if an instance prim is an outer instance.
        //
        // const HdInstancedBySchema instancedBySchema =
        //     HdInstancedBySchema::GetFromParent(instancePrim.dataSource);
        // if (!instancedBySchema) {
        //     // This is an outer instance.
        //     outerInstancePrimPaths.insert(instancePath);
        // }
    }
}

void
HdInstanceProxyViewSceneIndex::_Impl::RemoveTracking(
    const SdfPath &instancerPath)
{
    // Nuke any existing tracking for instances tied to this instancer...
    auto it = instanceInfoMap.begin();
    while (it != instanceInfoMap.end()) {
        if (it->second.instancerPath == instancerPath) {
            outerInstancePrimPaths.erase(it->first);
            it = instanceInfoMap.erase(it);
        } else {
            ++it;
        }
    }

    // ...and then remove the instancer from the tracked set.
    instancerPrimPaths.erase(instancerPath);
}

bool
HdInstanceProxyViewSceneIndex::_Impl::_IsDescendantOfOuterInstance(
    const SdfPath &primPath,
    SdfPath *outerInstancePath) const
{
    // Since we track the outer instance prim paths, if the path is a
    // descendant of any of those, then it is an instance proxy prim.
    // Lower bound gives us the first path that is >= primPath.
    // The element before that is the largest path that is < primPath.
    auto it = std::lower_bound(
        outerInstancePrimPaths.begin(), outerInstancePrimPaths.end(),
        primPath);
    
    if (it == outerInstancePrimPaths.begin()) {
        return false;
    }

    --it;

    // By decrementing the iterator, we don't need special case handling for
    // the scenario where `primPath` is an outer instance prim.
    TF_AXIOM(primPath != *it);

    if (primPath.HasPrefix(*it)) {
        if (outerInstancePath) {
            *outerInstancePath = *it;
        }
        return true;
    }
    return false;
}

bool
HdInstanceProxyViewSceneIndex::_Impl::_IsDescendantOfInstancer(
    const SdfPath &primPath) const
{
    auto it = std::lower_bound(
        instancerPrimPaths.begin(), instancerPrimPaths.end(),
        primPath);
    
    if (it == instancerPrimPaths.begin()) {
        return false;
    }

    --it;

    // By decrementing the iterator, we don't need special case handling for
    // the scenario where `primPath` is an instancer prim.
    TF_AXIOM(primPath != *it);

    return primPath.HasPrefix(*it);
}

std::pair<SdfPath, _InstancerChain>
HdInstanceProxyViewSceneIndex::_Impl::_ComputePrototypePathAndInstancers(
    const SdfPath &instanceProxyPath,
    const SdfPath &outerInstancePath) const
{
    if (!instanceProxyPath.HasPrefix(outerInstancePath)) {
        // This function should not be invoked with an instance proxy path that 
        // isn't a descendant of the outer instance path.
        // Note that we don't validate that the instance proxy path is a valid
        // one here. We just perform the path manipulation to compute the
        // mapped prototype path in the instancing context.
        TF_CODING_ERROR(
            "Instance proxy path <%s> is not a descendant of "
            "outer instance path <%s>",
            instanceProxyPath.GetText(), outerInstancePath.GetText());
        return {};
    }

    // Build the instancing context starting from the first prefix path after 
    // the outermost instance prim (i.e. an instance proxy child prim of the
    // outer instance prim) up to the instance proxy path itself.
    // Start with the prototype path being the outer instance prim path.
    // Iterate through each prefix path, replacing the current prototype path
    // if it is an instance prim with the corresponding prototype path, and then
    // appending the name token of the current prefix path to the prototype
    // path.
    //
    const size_t numPrefixes =
        instanceProxyPath.GetPathElementCount() - 
        outerInstancePath.GetPathElementCount();
    const SdfPathVector prefixPaths =
        instanceProxyPath.GetPrefixes(numPrefixes);

    SdfPath prototypePath = outerInstancePath;
    _InstancerChain instancers;

    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "Computing prototype path for instance proxy path <%s> with outer "
        "instance path <%s>.\n",
        instanceProxyPath.GetText(), outerInstancePath.GetText());

    for (const auto &prefixPath : prefixPaths) {
        TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
            "   Current prefix path <%s>, prototype path <%s>...\n",
            prefixPath.GetText(), prototypePath.GetText());

        const auto it = instanceInfoMap.find(prototypePath);
        if (it != instanceInfoMap.end()) {
            TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
                "   Prototype path <%s> is an instance prim. Remapping to "
                "prototype path <%s> and recording instancer <%s>.\n",
                prototypePath.GetText(), it->second.prototypePath.GetText(),
                it->second.instancerPath.GetText());
            const auto &instanceInfo = it->second;
            prototypePath = instanceInfo.prototypePath;
            instancers.push_back(instanceInfo.instancerPath);
        }
        TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
            "   Appending name token <%s>. Prototype path is now <%s>. \n",
            prefixPath.GetNameToken().GetText(),
            prototypePath.AppendChild(prefixPath.GetNameToken()).GetText());

        prototypePath =
            prototypePath.AppendChild(prefixPath.GetNameToken());
    }

    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "RESULT: Computed prototype path <%s> for instance proxy path <%s>."
        "\n\n", prototypePath.GetText(), instanceProxyPath.GetText());

    return {prototypePath, instancers};
}

namespace {

// -----------------------------------------------------------------------------
// Data source overrides.
// -----------------------------------------------------------------------------

HdContainerDataSourceHandle
_BuildInstanceProxyDataSource(const SdfPath &prototypePath)
{
    return HdRetainedContainerDataSource::New(
        HdInstanceProxySchema::GetSchemaToken(),
        HdInstanceProxySchema::Builder()
            .SetPrototypePath(
                HdRetainedTypedSampledDataSource<SdfPath>::New(prototypePath))
            .Build());
}

} // anon

///////////////////////////////////////////////////////////////////////////////
//
// Scene index implementation
//
///////////////////////////////////////////////////////////////////////////////

HdInstanceProxyViewSceneIndexRefPtr
HdInstanceProxyViewSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
{
    HdInstanceProxyViewSceneIndexRefPtr sceneIndex =
        TfCreateRefPtr(
            new HdInstanceProxyViewSceneIndex(inputSceneIndex));

    sceneIndex->SetDisplayName("Instance Proxy View Scene Index");

    return sceneIndex;
}

HdInstanceProxyViewSceneIndex::HdInstanceProxyViewSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _impl(std::make_unique<_Impl>(inputSceneIndex))
{
}

bool
HdInstanceProxyViewSceneIndex::IsOutermostInstance(const SdfPath &primPath) const
{
    return _impl->IsOutermostInstance(primPath);
}

bool
HdInstanceProxyViewSceneIndex::IsInstanceProxy(const SdfPath &primPath) const
{
    return _impl->IsInstanceProxy(primPath);
}

HdSceneIndexPrim
HdInstanceProxyViewSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    // Two scenarios to handle for `primPath` here:
    // 1. Instance proxy prim path: Manufacture a prim that mirrors the
    //    corresponding prototype prim with the instance proxy schema overlaid.
    //
    // 2. Any other prim path. Simply pass through to the input scene index.
    //    This includes invalid prim paths that may be a descendant of an
    //    outer instance prim but aren't valid instance proxy paths.
    //
    {
        std::optional<_InstancingContext> optCtx;
    
        if (_impl->IsInstanceProxy(primPath, &optCtx)) {
            const SdfPath &prototypePath = optCtx->prototypePath;
            const HdSceneIndexPrim &prototypePrim = optCtx->prototypePrim;
    
            TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
                "Getting prim for instance proxy path: <%s>\n",
                primPath.GetText());
            
            return {
                prototypePrim.primType,
                HdOverlayContainerDataSource::OverlayedContainerDataSources(
                    prototypePrim.dataSource,
                    _BuildInstanceProxyDataSource(prototypePath))
            };
        }
    }

    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdInstanceProxyViewSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    // Three possibilities for `primPath` here:
    // 1. Outer instance prim (a leaf hydra prim that isn't under an instancer)
    //    in the input scene index:
    //    Return the remapped child prim paths of the prototype prim.
    //
    // 2. Instance proxy prim that this scene index provides.
    //    Here, we need to account for the possibility that the prototype prim
    //    corresponding to the instance proxy prim is an instance, in which
    //    case we need to obtain its prototype path.
    //    Return the remapped child prim paths of the resolved prototype
    //    prim.
    //
    // 3. Any other prim path. Simply pass through to the input scene index.
    //
    {
        SdfPath prototypePath;
        if (_impl->IsOutermostInstance(primPath, &prototypePath)) {
            const auto childPaths =
                _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    
            if (!TF_VERIFY(childPaths.empty(),
                    "Outer instance path <%s> should not have any child prim "
                    "paths in the input scene index", primPath.GetText())) {
                return childPaths;
            }
    
            TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
                "Getting child prim paths for outer instance path: <%s>\n",
                primPath.GetText());
            
            return _impl->ComputeRemappedChildPrimPaths(prototypePath, primPath);
        }
    }

    {
        std::optional<_InstancingContext> optCtx;
        if (_impl->IsInstanceProxy(primPath, &optCtx)) {
            const auto &ctx = *optCtx;
            const auto childPaths =
                _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    
            if (!TF_VERIFY(childPaths.empty(),
                    "Instance proxy path <%s> should not have any child prim "
                    "paths in the input scene index. Found %zu child paths.",
                    primPath.GetText(), childPaths.size())) {
                return childPaths;
            }
    
            TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
                "Getting child prim paths for instance proxy path: <%s>"
                " that corresponds to prototype path: <%s>\n",
                primPath.GetText(), ctx.prototypePath.GetText());
            
            // XXX This bit feels unfortunate. Is there a better way to
            // express this?
            SdfPath resolvedPrototypePath = ctx.prototypePath;
            if (_impl->IsInstance(ctx.prototypePath, &resolvedPrototypePath)) {
                TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
                    "Mapped prototype path <%s> corresponds to an instance prim. "
                    "Using prototype path <%s> from instance info map for "
                    "computing child prim paths.\n",
                    ctx.prototypePath.GetText(), resolvedPrototypePath.GetText());
            }

            return _impl->ComputeRemappedChildPrimPaths(
                resolvedPrototypePath, primPath);
        }
    }

    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdInstanceProxyViewSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    for (const auto &entry : entries) {
        if (entry.primType == HdPrimTypeTokens->instancer) {
            const auto &instancerPath = entry.primPath;
            _impl->UpdateTracking(instancerPath);
        } else if (_impl->IsInstancer(entry.primPath)) {
            // Handle resync of a tracked instancer prim that is no longer one.
            _impl->RemoveTracking(entry.primPath);
        }
    }

    // XXX This scene index does not currently send notices for instance proxy
    //     prims currently.
    _SendPrimsAdded(entries);
}

void
HdInstanceProxyViewSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    for (const auto &entry : entries) {
        if (_impl->IsInstancer(entry.primPath)) {
            _impl->RemoveTracking(entry.primPath);
        }
    }

    // XXX This scene index does not currently send notices for instance proxy
    //     prims currently.
    _SendPrimsRemoved(entries);
}

void
HdInstanceProxyViewSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    for (const auto &entry : entries) {
        if (_impl->IsInstancer(entry.primPath)) {
            _impl->UpdateTracking(entry.primPath);
        }
    }

    // XXX This scene index does not currently send notices for instance proxy
    //     prims currently.
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
