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
// Maps to HdInstanceSchema with the addition of the prototype (root) path.
struct _InstanceInfo {
    SdfPath instancerPath;
    SdfPath prototypeRootPath;
    int prototypeIndex;
    int instanceIndex;
};

using _InstancingContext = TfSmallVector<_InstanceInfo, 4>;

struct _InstanceProxyContext {
    // `primInPrototype` refers to the prim under the (propagated) prototype
    // root that the instance proxy prim corresponds to.
    SdfPath pathToPrimInPrototype;
    HdSceneIndexPrim primInPrototype;

    _InstancingContext instancingCtx; // Ordered from outermost to innermost.
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
    : _inputSi(inputSceneIndex)
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
        std::optional<_InstanceProxyContext> *instanceProxyCtx = nullptr) const;

    bool
    IsOutermostInstance(
        const SdfPath &primPath,
        SdfPath *prototypeRootPath = nullptr) const;
    
    bool
    IsInstance(
        const SdfPath &primPath,
        SdfPath *prototypeRootPath = nullptr);
    
    bool
    IsInstancer(const SdfPath &primPath) const;

    SdfPathVector
    ComputeRemappedChildPrimPaths(
        const SdfPath &sourcePath,
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
    
    std::pair<SdfPath, _InstancingContext>
    _ComputeMappedPrimPathAndInstancingContext(
        const SdfPath &instanceProxyPrimPath,
        const SdfPath &outerInstancePath) const;

private:
    // ------------------------------------------------------------------------
    // Data
    //
    
    const HdSceneIndexBaseRefPtr _inputSi;
    
    // Map from any Hydra instance prim (i.e. prim with `instance` data source)
    // to its instancer, prototype and instance index.
    // Note that this includes nested instance prims that are descendants of
    // instancer prim(s).
    using _InstanceInfoMap = std::unordered_map<SdfPath, _InstanceInfo, TfHash>;
    _InstanceInfoMap _instanceInfoMap;

    // Track leaf prims with an instance data source that aren't under an
    // instancer prim to aid query processing.
    SdfPathSet _outerInstancePrimPaths;

    // Track all prims of type `instancer` to aid notice processing.
    SdfPathSet _instancerPrimPaths;
};

bool
HdInstanceProxyViewSceneIndex::_Impl::IsInstanceProxy(
    const SdfPath &primPath,
    std::optional<
        _InstanceProxyContext> *instanceProxyCtx /* = nullptr */) const
{
    SdfPath outerInstancePath;
    if (!_IsDescendantOfOuterInstance(primPath, &outerInstancePath)) {
        return false;
    }

    const auto [pathToPrimInPrototype, instancingContext] =
        _ComputeMappedPrimPathAndInstancingContext(
            primPath, outerInstancePath);

    const auto primInPrototype = _inputSi->GetPrim(pathToPrimInPrototype);
    if (!primInPrototype) {
        TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
            "Path <%s> is not a valid instance proxy prim path because "
            "the computed prototype path <%s> does not exist.\n",
            primPath.GetText(), pathToPrimInPrototype.GetText());
        return false;
    }

    if (instanceProxyCtx) {
        *instanceProxyCtx = _InstanceProxyContext{
            pathToPrimInPrototype, primInPrototype, instancingContext};
    }

    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "Path <%s> is an instance proxy prim mapping to the prim <%s> and "
        "%zu instancer(s).\n", primPath.GetText(),
        pathToPrimInPrototype.GetText(), instancingContext.size());
    return true;
}

bool
HdInstanceProxyViewSceneIndex::_Impl::IsOutermostInstance(
    const SdfPath &primPath,
    SdfPath *prototypeRootPath /* = nullptr */) const
{
    if (_outerInstancePrimPaths.count(primPath) == 0) {
        return false;
    }

    if (prototypeRootPath) {
        const auto it = _instanceInfoMap.find(primPath);
        if (it == _instanceInfoMap.end()) {
            TF_CODING_ERROR(
                "Outer instance prim path <%s> is missing from instance info "
                "map. This indicates a bug in tracking logic.",
                primPath.GetText());
            *prototypeRootPath = SdfPath();
        } else {
            *prototypeRootPath = it->second.prototypeRootPath;
        }
    }
    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "Path <%s> is an outer instance prim.\n", primPath.GetText());
    return true;
}

bool
HdInstanceProxyViewSceneIndex::_Impl::IsInstance(
    const SdfPath &primPath,
    SdfPath *prototypeRootPath /* = nullptr */)
{
    const auto it = _instanceInfoMap.find(primPath);
    if (it == _instanceInfoMap.end()) {
        return false;
    }
    
    if (prototypeRootPath) {
        *prototypeRootPath = it->second.prototypeRootPath;
    }
    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "Path <%s> is an instance prim.\n", primPath.GetText());
    return true;
}

bool
HdInstanceProxyViewSceneIndex::_Impl::IsInstancer(const SdfPath &primPath) const
{
    return _instancerPrimPaths.count(primPath) > 0;
}

SdfPathVector
HdInstanceProxyViewSceneIndex::_Impl::ComputeRemappedChildPrimPaths(
    const SdfPath &sourcePath, const SdfPath &prefixToReplaceWith) const
{
    const SdfPathVector childPaths =
        _inputSi->GetChildPrimPaths(sourcePath);

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
    const auto instancerPrim = _inputSi->GetPrim(instancerPath);
    const HdInstancerTopologySchema topologySchema =
        HdInstancerTopologySchema::GetFromParent(instancerPrim.dataSource);
    const HdPathArrayDataSourceHandle instanceLocationsDs =
        topologySchema.GetInstanceLocations();

    if (!instanceLocationsDs) {
        RemoveTracking(instancerPath);
        return;
    }

    _instancerPrimPaths.insert(instancerPath);
    
    const VtArray<SdfPath> instanceLocations =
        instanceLocationsDs->GetTypedValue(0.0);

    for (const auto &instancePath : instanceLocations) {
        const auto instancePrim = _inputSi->GetPrim(instancePath);
        const HdInstanceSchema instanceSchema =
            HdInstanceSchema::GetFromParent(instancePrim.dataSource);
        const HdIntDataSourceHandle prototypeIndexDs =
            instanceSchema.GetPrototypeIndex();
        const HdIntDataSourceHandle instanceIndexDs =
            instanceSchema.GetInstanceIndex();

        if (!prototypeIndexDs || !instanceIndexDs) {
            _instanceInfoMap.erase(instancePath);
            _outerInstancePrimPaths.erase(instancePath);
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

        _instanceInfoMap[instancePath] =
            {instancerPath, prototypePath, prototypeIndex, instanceIndex};
        
        if (!_IsDescendantOfInstancer(instancePath)) {
            // This is an outer instance.
            _outerInstancePrimPaths.insert(instancePath);
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
        //     _outerInstancePrimPaths.insert(instancePath);
        // }
    }
}

void
HdInstanceProxyViewSceneIndex::_Impl::RemoveTracking(
    const SdfPath &instancerPath)
{
    // Nuke any existing tracking for instances tied to this instancer...
    auto it = _instanceInfoMap.begin();
    while (it != _instanceInfoMap.end()) {
        if (it->second.instancerPath == instancerPath) {
            _outerInstancePrimPaths.erase(it->first);
            it = _instanceInfoMap.erase(it);
        } else {
            ++it;
        }
    }

    // ...and then remove the instancer from the tracked set.
    _instancerPrimPaths.erase(instancerPath);
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
        _outerInstancePrimPaths.begin(), _outerInstancePrimPaths.end(),
        primPath);
    
    if (it == _outerInstancePrimPaths.begin()) {
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
        _instancerPrimPaths.begin(), _instancerPrimPaths.end(),
        primPath);
    
    if (it == _instancerPrimPaths.begin()) {
        return false;
    }

    --it;

    // By decrementing the iterator, we don't need special case handling for
    // the scenario where `primPath` is an instancer prim.
    TF_AXIOM(primPath != *it);

    return primPath.HasPrefix(*it);
}

std::pair<SdfPath, _InstancingContext>
HdInstanceProxyViewSceneIndex::_Impl::
_ComputeMappedPrimPathAndInstancingContext(
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
    // Start with the mapped path being the outer instance prim path.
    // Iterate through each prefix path, replacing the current mapped path
    // if it is an instance prim with the corresponding prototype root path, and
    // then appending the name token of the current prefix path to the mapped 
    // path.
    //
    const size_t numPrefixes =
        instanceProxyPath.GetPathElementCount() - 
        outerInstancePath.GetPathElementCount();
    const SdfPathVector prefixPaths =
        instanceProxyPath.GetPrefixes(numPrefixes);

    SdfPath mappedPrimPath = outerInstancePath;
    _InstancingContext instancingCtx;

    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "Computing mapped prim path for instance proxy path <%s> with outer "
        "instance path <%s>.\n",
        instanceProxyPath.GetText(), outerInstancePath.GetText());

    for (const auto &prefixPath : prefixPaths) {
        TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
            "   Current prefix path <%s>, mapped prim path <%s>...\n",
            prefixPath.GetText(), mappedPrimPath.GetText());

        const auto it = _instanceInfoMap.find(mappedPrimPath);
        if (it != _instanceInfoMap.end()) {
            const auto &instanceInfo = it->second;
            TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
                "   Path <%s> is an instance prim. Remapping to "
                "prototype root path <%s> and recording instancer <%s>.\n",
                mappedPrimPath.GetText(),
                instanceInfo.prototypeRootPath.GetText(),
                instanceInfo.instancerPath.GetText());

            mappedPrimPath = instanceInfo.prototypeRootPath;
            instancingCtx.push_back(instanceInfo);
        }
        TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
            "   Appending name token <%s>. Mapped path is now <%s>. \n",
            prefixPath.GetNameToken().GetText(),
            mappedPrimPath.AppendChild(prefixPath.GetNameToken()).GetText());

        mappedPrimPath =
            mappedPrimPath.AppendChild(prefixPath.GetNameToken());
    }

    TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
        "RESULT: Computed mapped path <%s> for instance proxy path <%s>."
        "\n\n", mappedPrimPath.GetText(), instanceProxyPath.GetText());

    return {mappedPrimPath, instancingCtx};
}

namespace {

// -----------------------------------------------------------------------------
// Data source overrides.
// -----------------------------------------------------------------------------

HdVectorDataSourceHandle
_BuildInstancingContextVectorDataSource(const _InstancingContext &instancingCtx)
{
    std::vector<HdDataSourceBaseHandle> dataSources;
    dataSources.reserve(instancingCtx.size());

    for (const auto &instanceInfo : instancingCtx) {
         dataSources.push_back(
            HdInstanceSchema::Builder()
                .SetInstancer(
                    HdRetainedTypedSampledDataSource<SdfPath>::New(
                        instanceInfo.instancerPath))
                .SetPrototypeIndex(
                    HdRetainedTypedSampledDataSource<int>::New(
                        instanceInfo.prototypeIndex))
                .SetInstanceIndex(
                    HdRetainedTypedSampledDataSource<int>::New(
                        instanceInfo.instanceIndex))
                .Build());
    }

    return HdRetainedSmallVectorDataSource::New(
        dataSources.size(), dataSources.data());
}

HdContainerDataSourceHandle
_BuildInstanceProxyDataSource(
    const _InstanceProxyContext &instanceProxyCtx)
{
    return HdRetainedContainerDataSource::New(
        HdInstanceProxySchema::GetSchemaToken(),
        HdInstanceProxySchema::Builder()
            .SetPathToPrimInPrototype(
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                    instanceProxyCtx.pathToPrimInPrototype))
            .SetInstancingContext(
                _BuildInstancingContextVectorDataSource(
                    instanceProxyCtx.instancingCtx))
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
        std::optional<_InstanceProxyContext> optCtx;
    
        if (_impl->IsInstanceProxy(primPath, &optCtx)) {
            TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
                "Getting prim for instance proxy path: <%s>\n",
                primPath.GetText());
            
            const auto &primInPrototype = optCtx->primInPrototype;

            return {
                primInPrototype.primType,
                HdOverlayContainerDataSource::OverlayedContainerDataSources(
                    primInPrototype.dataSource,
                    _BuildInstanceProxyDataSource(*optCtx))
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
    //    Here, we need to account for the possibility that the mapped prim
    //    corresponding to the instance proxy prim is an instance, in which
    //    case we need to obtain its prototype path.
    //    Return the remapped child prim paths of the prim at theresolved path.
    //
    // 3. Any other prim path. Simply pass through to the input scene index.
    //
    {
        SdfPath prototypeRootPath;
        if (_impl->IsOutermostInstance(primPath, &prototypeRootPath)) {
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
            
            return _impl->ComputeRemappedChildPrimPaths(
                prototypeRootPath, primPath);
        }
    }

    {
        std::optional<_InstanceProxyContext> optCtx;
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
                " that corresponds to the mapped path: <%s>\n",
                primPath.GetText(), ctx.pathToPrimInPrototype.GetText());
            
            // XXX This bit feels unfortunate. Is there a better way to
            // express this?
            SdfPath resolvedPrimPath = ctx.pathToPrimInPrototype;
            if (_impl->IsInstance(
                    ctx.pathToPrimInPrototype, &resolvedPrimPath)) {
                TF_DEBUG(HD_INSTANCE_PROXY_VIEW_SCENE_INDEX).Msg(
                    "Mapped prototype path <%s> corresponds to an instance prim. "
                    "Using prototype root path <%s> from instance info map for "
                    "computing child prim paths.\n",
                    ctx.pathToPrimInPrototype.GetText(),
                    resolvedPrimPath.GetText());
            }

            return _impl->ComputeRemappedChildPrimPaths(
                resolvedPrimPath, primPath);
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
