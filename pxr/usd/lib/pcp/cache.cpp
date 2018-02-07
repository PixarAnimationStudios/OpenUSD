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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/cache.h"
#include "pxr/usd/pcp/arc.h"
#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/debugCodes.h"
#include "pxr/usd/pcp/dependencies.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/layerStackRegistry.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/pathTranslation.h"
#include "pxr/usd/pcp/payloadDecorator.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/propertyIndex.h"
#include "pxr/usd/pcp/statistics.h"
#include "pxr/usd/pcp/targetIndex.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/singularTask.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/registryManager.h"

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_vector.h>
#include <tbb/spin_rw_mutex.h>

#include <algorithm>
#include <utility>
#include <vector>

using std::make_pair;
using std::pair;
using std::vector;

using boost::dynamic_pointer_cast;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    PCP_CULLING, true,
    "Controls whether culling is enabled in Pcp caches.");

// Helper for applying changes immediately if the client hasn't asked that
// they only be collected instead.
class Pcp_CacheChangesHelper {
public:
    // Construct.  If \p changes is \c NULL then collect changes into an
    // internal object and apply them to \p cache when this object is
    // destroyed.
    Pcp_CacheChangesHelper(PcpChanges* changes) :
        _changes(changes)
    {
        // Do nothing
    }

    ~Pcp_CacheChangesHelper()
    {
        // Apply changes now immediately if _changes is NULL.
        if (!_changes) {
            _immediateChanges.Apply();
        }
    }

    // Act like a pointer to the c'tor PcpChanges or, if that's NULL, the
    // internal changes.
    PcpChanges* operator->()
    {
        return _changes ? _changes : &_immediateChanges;
    }

private:
    PcpChanges* _changes;
    PcpChanges _immediateChanges;
};

PcpCache::PcpCache(
    const PcpLayerStackIdentifier & layerStackIdentifier,
    const std::string& targetSchema,
    bool usd,
    const PcpPayloadDecoratorRefPtr& payloadDecorator) :
    _rootLayer(layerStackIdentifier.rootLayer),
    _sessionLayer(layerStackIdentifier.sessionLayer),
    _pathResolverContext(layerStackIdentifier.pathResolverContext),
    _usd(usd),
    _targetSchema(targetSchema),
    _payloadDecorator(payloadDecorator),
    _layerStackCache(Pcp_LayerStackRegistry::New(_targetSchema, _usd)),
    _primDependencies(new Pcp_Dependencies())
{
    // Do nothing
}

PcpCache::~PcpCache()
{
    // We have to release the GIL here, since we don't know whether or not we've
    // been invoked by some python-wrapped thing here which might not have
    // released the GIL itself.  Dropping the layer RefPtrs could cause the
    // layers to expire, which might try to invoke the python/c++ shared
    // lifetime management support, which will need to acquire the GIL.  If that
    // happens in a separate worker thread while this thread holds the GIL,
    // we'll deadlock.  Dropping the GIL here prevents this.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Clear the layer stack before destroying the registry, so
    // that it can safely unregister itself.
    TfReset(_layerStack);

    // Tear down some of our datastructures in parallel, since it can take quite
    // a bit of time.
    WorkArenaDispatcher wd;

    wd.Run([this]() { _rootLayer.Reset(); });
    wd.Run([this]() { _sessionLayer.Reset(); });
    wd.Run([this]() { _payloadDecorator.Reset(); });
    wd.Run([this]() { TfReset(_includedPayloads); });
    wd.Run([this]() { TfReset(_variantFallbackMap); });
    wd.Run([this]() { _primIndexCache.ClearInParallel(); });
    wd.Run([this]() { TfReset(_propertyIndexCache); });

    // Wait, since _primDependencies cannot be destroyed concurrently
    // with the prim indexes, since they both hold references to
    // layer stacks and the layer stack registry is not currently
    // prepared to handle concurrent expiry of layer stacks.
    wd.Wait();

    wd.Run([this]() { _primDependencies.reset(); });

    // Wait, since _layerStackCache cannot be destroyed until
    // _primDependencies is cleaned up.
    wd.Wait();

    wd.Run([this]() { _layerStackCache.Reset(); });

    wd.Wait();
}

////////////////////////////////////////////////////////////////////////
// Cache parameters.

PcpLayerStackIdentifier
PcpCache::GetLayerStackIdentifier() const
{
    return PcpLayerStackIdentifier(_rootLayer, _sessionLayer,
                                   _pathResolverContext);
}

PcpLayerStackPtr
PcpCache::GetLayerStack() const
{
    return _layerStack;
}

PcpLayerStackPtr
PcpCache::FindLayerStack(const PcpLayerStackIdentifier &id) const
{
    return _layerStackCache->Find(id);
}

const PcpLayerStackPtrVector&
PcpCache::FindAllLayerStacksUsingLayer(const SdfLayerHandle& layer) const
{
    return _layerStackCache->FindAllUsingLayer(layer);
}

bool
PcpCache::IsUsd() const
{
    return _usd;
}

const std::string& 
PcpCache::GetTargetSchema() const
{
    return _targetSchema;
}

PcpPayloadDecorator* 
PcpCache::GetPayloadDecorator() const
{
    return boost::get_pointer(_payloadDecorator);
}

PcpVariantFallbackMap
PcpCache::GetVariantFallbacks() const
{
    return _variantFallbackMap;
}

void
PcpCache::SetVariantFallbacks( const PcpVariantFallbackMap &map,
                                   PcpChanges* changes )
{
    if (_variantFallbackMap != map) {
        _variantFallbackMap = map;

        Pcp_CacheChangesHelper cacheChanges(changes);

        // We could scan to find prim indices that actually use the
        // affected variant sets, but for simplicity of implementing what
        // is a really uncommon operation, we just invalidate everything.
        cacheChanges->DidChangeSignificantly(this, SdfPath::AbsoluteRootPath());
    }
}

bool
PcpCache::IsPayloadIncluded(const SdfPath &path) const
{
    return _includedPayloads.find(path) != _includedPayloads.end();
}

SdfPathSet
PcpCache::GetIncludedPayloads() const
{
    return SdfPathSet(_includedPayloads.begin(), _includedPayloads.end());
}

void
PcpCache::RequestPayloads( const SdfPathSet & pathsToInclude,
                           const SdfPathSet & pathsToExclude,
                           PcpChanges* changes )
{
    Pcp_CacheChangesHelper cacheChanges(changes);

    TF_FOR_ALL(path, pathsToInclude) {
        if (path->IsPrimPath()) {
            if (_includedPayloads.insert(*path).second) {
                cacheChanges->DidChangeSignificantly(this, *path);
            }
        }
        else {
            TF_CODING_ERROR("Path <%s> must be a prim path", path->GetText());
        }
    }
    TF_FOR_ALL(path, pathsToExclude) {
        if (path->IsPrimPath()) {
            if (pathsToInclude.find(*path) == pathsToInclude.end()) {
                if (_includedPayloads.erase(*path)) {
                    cacheChanges->DidChangeSignificantly(this, *path);
                }
            }
        }
        else {
            TF_CODING_ERROR("Path <%s> must be a prim path", path->GetText());
        }
    }
}

void 
PcpCache::RequestLayerMuting(const std::vector<std::string>& layersToMute,
                             const std::vector<std::string>& layersToUnmute,
                             PcpChanges* changes)
{
    ArResolverContextBinder binder(_pathResolverContext);

    std::vector<std::string> finalLayersToMute;
    for (const auto& layerToMute : layersToMute) {
        if (layerToMute.empty()) {
            continue;
        }

        if (SdfLayer::Find(layerToMute) == _rootLayer) {
            TF_CODING_ERROR("Cannot mute cache's root layer @%s@", 
                            layerToMute.c_str());
            continue;
        }

        finalLayersToMute.push_back(layerToMute);
    }

    std::vector<std::string> finalLayersToUnmute;
    for (const auto& layerToUnmute : layersToUnmute) {
        if (layerToUnmute.empty()) {
            continue;
        }

        if (std::find(layersToMute.begin(), layersToMute.end(), 
                      layerToUnmute) == layersToMute.end()) {
            finalLayersToUnmute.push_back(layerToUnmute);
        }
    }

    if (finalLayersToMute.empty() && finalLayersToUnmute.empty()) {
        return;
    }

    _layerStackCache->MuteAndUnmuteLayers(
        _rootLayer, &finalLayersToMute, &finalLayersToUnmute);

    Pcp_CacheChangesHelper cacheChanges(changes);

    // Register changes for all computed layer stacks that are
    // affected by the newly muted/unmuted layers.
    for (const auto& layerToMute : finalLayersToMute) {
        cacheChanges->DidMuteLayer(this, layerToMute);
    }

    for (const auto& layerToUnmute : finalLayersToUnmute) {
        cacheChanges->DidUnmuteLayer(this, layerToUnmute);
    }

    // The above won't handle cases where we've unmuted the root layer
    // of a reference or payload layer stack, since prim indexing will skip
    // computing those layer stacks altogether. So, find all prim indexes
    // that have the associated composition error and treat this as if
    // we're reloading the unmuted layer.
    if (!finalLayersToUnmute.empty()) {
        for (const auto& primIndexEntry : _primIndexCache) {
            const PcpPrimIndex& primIndex = primIndexEntry.second;
            if (!primIndex.IsValid()) {
                continue;
            }

            for (const auto& error : primIndex.GetLocalErrors()) {
                PcpErrorMutedAssetPathPtr typedError = 
                    dynamic_pointer_cast<PcpErrorMutedAssetPath>(error);
                if (!typedError) {
                    continue;
                }

                const bool assetWasUnmuted = std::find(
                    finalLayersToUnmute.begin(), finalLayersToUnmute.end(), 
                    typedError->resolvedAssetPath) != finalLayersToUnmute.end();
                if (assetWasUnmuted) {
                    cacheChanges->DidMaybeFixAsset(
                        this, typedError->site, typedError->layer, 
                        typedError->resolvedAssetPath);
                }
            }
        }
    }
}

const std::vector<std::string>&
PcpCache:: GetMutedLayers() const
{
    return _layerStackCache->GetMutedLayers();
}

bool 
PcpCache::IsLayerMuted(const std::string& layerId) const
{
    return IsLayerMuted(_rootLayer, layerId);
}

bool 
PcpCache::IsLayerMuted(const SdfLayerHandle& anchorLayer,
                       const std::string& layerId,
                       std::string* canonicalMutedLayerId) const
{
    return _layerStackCache->IsLayerMuted(
        anchorLayer, layerId, canonicalMutedLayerId);
}

PcpPrimIndexInputs 
PcpCache::GetPrimIndexInputs()
{
    return PcpPrimIndexInputs()
        .Cache(this)
        .PayloadDecorator(GetPayloadDecorator())
        .VariantFallbacks(&_variantFallbackMap)
        .IncludedPayloads(&_includedPayloads)
        .Cull(TfGetEnvSetting(PCP_CULLING))
        .TargetSchema(_targetSchema);
}

PcpLayerStackRefPtr
PcpCache::ComputeLayerStack(const PcpLayerStackIdentifier &id,
                            PcpErrorVector *allErrors)
{
    PcpLayerStackRefPtr result =
        _layerStackCache->FindOrCreate(id, allErrors);

    // Retain the cache's root layer stack.
    if (!_layerStack && id == GetLayerStackIdentifier()) {
        _layerStack = result;
    }

    return result;
}

const PcpPrimIndex *
PcpCache::FindPrimIndex(const SdfPath & path) const
{
    return _GetPrimIndex(path);
}

void
PcpCache::ComputeRelationshipTargetPaths(const SdfPath & relPath, 
                                         SdfPathVector *paths,
                                         bool localOnly,
                                         const SdfSpecHandle &stopProperty,
                                         bool includeStopProperty,
                                         PcpErrorVector *allErrors)
{
    TRACE_FUNCTION();

    if (!relPath.IsPropertyPath()) {
        TF_CODING_ERROR(
            "Path <%s> must be a relationship path", relPath.GetText());
        return;
    }

    PcpTargetIndex targetIndex;
    PcpBuildFilteredTargetIndex( PcpSite(GetLayerStackIdentifier(), relPath),
                                 ComputePropertyIndex(relPath, allErrors),
                                 SdfSpecTypeRelationship,
                                 localOnly, stopProperty, includeStopProperty,
                                 this, &targetIndex, allErrors );
    paths->swap(targetIndex.paths);
}

void
PcpCache::ComputeAttributeConnectionPaths(const SdfPath & attrPath, 
                                          SdfPathVector *paths,
                                          bool localOnly,
                                          const SdfSpecHandle &stopProperty,
                                          bool includeStopProperty,
                                          PcpErrorVector *allErrors)
{
    TRACE_FUNCTION();

    if (!attrPath.IsPropertyPath()) {
        TF_CODING_ERROR(
            "Path <%s> must be an attribute path", attrPath.GetText());
        return;
    }

    PcpTargetIndex targetIndex;
    PcpBuildFilteredTargetIndex( PcpSite(GetLayerStackIdentifier(), attrPath),
                                 ComputePropertyIndex(attrPath, allErrors),
                                 SdfSpecTypeAttribute,
                                 localOnly, stopProperty, includeStopProperty,
                                 this, &targetIndex, allErrors );
    paths->swap(targetIndex.paths);
}

const PcpPropertyIndex *
PcpCache::FindPropertyIndex(const SdfPath & path) const
{
    return _GetPropertyIndex(path);
}

SdfLayerHandleSet
PcpCache::GetUsedLayers() const
{
    SdfLayerHandleSet rval = _primDependencies->GetUsedLayers();

    // Dependencies don't include the local layer stack, so manually add those
    // layers here.
    if (_layerStack) {
        const SdfLayerRefPtrVector& localLayers = _layerStack->GetLayers();
        rval.insert(localLayers.begin(), localLayers.end());
    }
    return rval;
}

SdfLayerHandleSet
PcpCache::GetUsedRootLayers() const
{
    SdfLayerHandleSet rval = _primDependencies->GetUsedRootLayers();

    // Dependencies don't include the local layer stack, so manually add the
    // local root layer here.
    rval.insert(_rootLayer);
    return rval;
}

PcpDependencyVector
PcpCache::FindSiteDependencies(
    const SdfLayerHandle& layer,
    const SdfPath& sitePath,
    PcpDependencyFlags depMask,
    bool recurseOnSite,
    bool recurseOnIndex,
    bool filterForExistingCachesOnly
    ) const
{
    PcpDependencyVector result;
    for (const auto& layerStack: FindAllLayerStacksUsingLayer(layer)) {
        PcpDependencyVector deps = FindSiteDependencies(
            layerStack, sitePath, depMask, recurseOnSite, recurseOnIndex,
            filterForExistingCachesOnly);
        for (PcpDependency dep: deps) {
            SdfLayerOffset offset = dep.mapFunc.GetTimeOffset();
            // Fold in any sublayer offset.
            if (const SdfLayerOffset *sublayer_offset =
                layerStack->GetLayerOffsetForLayer(layer)) {
                offset = offset * *sublayer_offset;
            }
            dep.mapFunc = PcpMapFunction::Create(
                dep.mapFunc.GetSourceToTargetMap(), offset);
            result.push_back(dep);
        }
    }
    return result;
}

PcpDependencyVector
PcpCache::FindSiteDependencies(
    const PcpLayerStackPtr& siteLayerStack,
    const SdfPath& sitePath,
    PcpDependencyFlags depMask,
    bool recurseOnSite,
    bool recurseOnIndex,
    bool filterForExistingCachesOnly
    ) const
{
    TRACE_FUNCTION();

    PcpDependencyVector deps;

    //
    // Validate arguments.
    //
    if (!(depMask & (PcpDependencyTypeVirtual|PcpDependencyTypeNonVirtual))) {
        TF_CODING_ERROR("depMask must include at least one of "
                        "{PcpDependencyTypeVirtual, "
                        "PcpDependencyTypeNonVirtual}");
        return deps;
    }
    if (!(depMask & (PcpDependencyTypeRoot | PcpDependencyTypeDirect |
                     PcpDependencyTypeAncestral))) {
        TF_CODING_ERROR("depMask must include at least one of "
                        "{PcpDependencyTypeRoot, "
                        "PcpDependencyTypePurelyDirect, "
                        "PcpDependencyTypePartlyDirect, "
                        "PcpDependencyTypeAncestral}");
        return deps;
    }
    if ((depMask & PcpDependencyTypeRoot) &&
        !(depMask & PcpDependencyTypeNonVirtual)) {
        // Root deps are only ever non-virtual.
        TF_CODING_ERROR("depMask of PcpDependencyTypeRoot requires "
                        "PcpDependencyTypeNonVirtual");
        return deps;
    }
    if (siteLayerStack->_registry != _layerStackCache) {
        TF_CODING_ERROR("PcpLayerStack does not belong to this PcpCache");
        return deps;
    }

    // Filter function for dependencies to return.
    auto cacheFilterFn = [this, filterForExistingCachesOnly]
        (const SdfPath &indexPath) {
            if (!filterForExistingCachesOnly) {
                return true;
            } else if (indexPath.IsAbsoluteRootOrPrimPath()) {
                return bool(FindPrimIndex(indexPath));
            } else if (indexPath.IsPropertyPath()) {
                return bool(FindPropertyIndex(indexPath));
            } else {
                return false;
            }
        };

    // Dependency arcs expressed in scene description connect prim
    // paths, prim variant paths, and absolute paths only. Those arcs
    // imply dependency structure for children, such as properties.
    // To service dependency queries about those children, we must
    // examine structure at the enclosing prim/root level where deps
    // are expresed. Find the containing path.
    const SdfPath sitePrimPath =
        (sitePath == SdfPath::AbsoluteRootPath()) ? sitePath :
        sitePath.GetPrimOrPrimVariantSelectionPath();

    // Handle the root dependency.
    // Sites containing variant selections are never root dependencies.
    if (depMask & PcpDependencyTypeRoot &&
        siteLayerStack == _layerStack &&
        !sitePath.ContainsPrimVariantSelection() &&
        cacheFilterFn(sitePath)) {
        deps.push_back(PcpDependency{
            sitePath, sitePath, PcpMapFunction::Identity()});
    }

    // Handle dependencies stored in _primDependencies.
    auto visitSiteFn = [&](const SdfPath &depPrimIndexPath,
                           const SdfPath &depPrimSitePath)
    {
        // Because arc dependencies are analyzed in terms of prims,
        // if we are querying deps for a property, and recurseOnSite
        // is true, we must guard against recursing into paths
        // that are siblings of the property and filter them out.
        if (depPrimSitePath != sitePrimPath &&
            depPrimSitePath.HasPrefix(sitePrimPath) &&
            !depPrimSitePath.HasPrefix(sitePath)) {
            return;
        }

        // If we have recursed above to an ancestor, include its direct
        // dependencies, since they are considered ancestral by descendants.
        const PcpDependencyFlags localMask =
            (depPrimSitePath != sitePrimPath &&
             sitePrimPath.HasPrefix(depPrimSitePath))
            ? (depMask | PcpDependencyTypeDirect) : depMask;

        // If we have recursed below sitePath, use that site;
        // otherwise use the site the caller requested.
        const SdfPath localSitePath =
            (depPrimSitePath != sitePrimPath &&
             depPrimSitePath.HasPrefix(sitePrimPath))
            ? depPrimSitePath : sitePath;

        auto visitNodeFn = [&](const SdfPath &depPrimIndexPath,
                               const PcpNodeRef &node)
        {
            // Skip computing the node's dependency type if we aren't looking
            // for a specific type -- that computation can be expensive.
            if (localMask != PcpDependencyTypeAnyIncludingVirtual) {
                PcpDependencyFlags flags = PcpClassifyNodeDependency(node);
                if ((flags & localMask) != flags) { 
                    return;
                }
            }

            // Now that we have found a dependency on depPrimSitePath,
            // use path translation to get the corresponding depIndexPath.
            SdfPath depIndexPath;
            bool valid = false;
            if (node.GetArcType() == PcpArcTypeRelocate) {
                // Relocates require special handling.  Because
                // a relocate node's map function is always
                // identity, we must do our own prefix replacement
                // to step out of the relocate, then continue
                // with regular path translation.
                const PcpNodeRef parent = node.GetParentNode(); 
                depIndexPath = PcpTranslatePathFromNodeToRoot(
                    parent,
                    localSitePath.ReplacePrefix( node.GetPath(),
                                                 parent.GetPath() ),
                    &valid );
            } else {
                depIndexPath = PcpTranslatePathFromNodeToRoot(
                    node, localSitePath, &valid);
            }
            if (valid && TF_VERIFY(!depIndexPath.IsEmpty()) &&
                cacheFilterFn(depIndexPath)) {
                deps.push_back(PcpDependency{
                    depIndexPath, localSitePath,
                    node.GetMapToRoot().Evaluate() });
            }
        };
        Pcp_ForEachDependentNode(depPrimSitePath, siteLayerStack,
                                 depPrimIndexPath, *this, visitNodeFn);
    };
    _primDependencies->ForEachDependencyOnSite(
        siteLayerStack, sitePrimPath,
        /* includeAncestral = */ depMask & PcpDependencyTypeAncestral,
        recurseOnSite, visitSiteFn);

    // If recursing down namespace, we may have cache entries for
    // descendants that did not introduce new dependency arcs, and
    // therefore were not encountered above, but which nonetheless
    // represent dependent paths.  Add them if requested.
    if (recurseOnIndex) {
        TRACE_SCOPE("PcpCache::FindSiteDependencies - recurseOnIndex");
        SdfPathSet seenDeps;
        PcpDependencyVector expandedDeps;

        for(const PcpDependency &dep: deps) {
            const SdfPath & indexPath = dep.indexPath;

            auto it = seenDeps.upper_bound(indexPath);
            if (it != seenDeps.begin()) {
                --it;
                if (indexPath.HasPrefix(*it)) {
                    // Short circuit further expansion; expect we
                    // have already recursed below this path.
                    continue;
                }
            }

            seenDeps.insert(indexPath);
            expandedDeps.push_back(dep);
            // Recurse on child index entries.
            if (indexPath.IsAbsoluteRootOrPrimPath()) {
                auto primRange =
                    _primIndexCache.FindSubtreeRange(indexPath);
                if (primRange.first != primRange.second) {
                    // Skip initial entry, since we've already added it 
                    // to expandedDeps above.
                    ++primRange.first;
                }

                for (auto entryIter = primRange.first;
                     entryIter != primRange.second; ++entryIter) {
                    const SdfPath& subPath = entryIter->first;
                    const PcpPrimIndex& subPrimIndex = entryIter->second;
                    if (subPrimIndex.IsValid()) {
                        expandedDeps.push_back(PcpDependency{
                            subPath,
                            subPath.ReplacePrefix(indexPath, dep.sitePath),
                            dep.mapFunc});
                    }
                }
            }
            // Recurse on child property entries.
            const auto propRange =
                _propertyIndexCache.FindSubtreeRange(indexPath);
            for (auto entryIter = propRange.first;
                 entryIter != propRange.second; ++entryIter) {
                const SdfPath& subPath = entryIter->first;
                const PcpPropertyIndex& subPropIndex = entryIter->second;
                if (!subPropIndex.IsEmpty()) {
                    expandedDeps.push_back(PcpDependency{
                        subPath,
                        subPath.ReplacePrefix(indexPath, dep.sitePath),
                        dep.mapFunc});
                }
            }
        }
        std::swap(deps, expandedDeps);
    }

    return deps;
}

bool
PcpCache::CanHaveOpinionForSite(
    const SdfPath& localPcpSitePath,
    const SdfLayerHandle& layer,
    SdfPath* allowedPathInLayer) const
{
    // Get the prim index.
    if (const PcpPrimIndex* primIndex = _GetPrimIndex(localPcpSitePath)) {
        // We only want to check any layer stack for layer once.
        std::set<PcpLayerStackPtr> visited;

        // Iterate over all nodes.
        for (const PcpNodeRef &node: primIndex->GetNodeRange()) {
            // Ignore nodes that don't provide specs.
            if (node.CanContributeSpecs()) {
                // Check each layer stack that contributes specs only once.
                if (visited.insert(node.GetLayerStack()).second) {
                    // Check for layer.
                    TF_FOR_ALL(i, node.GetLayerStack()->GetLayers()) {
                        if (*i == layer) {
                            if (allowedPathInLayer) {
                                *allowedPathInLayer = node.GetPath();
                            }
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

std::vector<std::string>
PcpCache::GetInvalidSublayerIdentifiers() const
{
    TRACE_FUNCTION();

    std::set<std::string> result;

    std::vector<PcpLayerStackPtr> allLayerStacks =
        _layerStackCache->GetAllLayerStacks();

    TF_FOR_ALL(layerStack, allLayerStacks) {
        // Scan errors for a sublayer error.
        PcpErrorVector errs = (*layerStack)->GetLocalErrors();
        TF_FOR_ALL(e, errs) {
            if (PcpErrorInvalidSublayerPathPtr typedErr =
                dynamic_pointer_cast<PcpErrorInvalidSublayerPath>(*e)){
                result.insert(typedErr->sublayerPath);
            }
        }
    }

    return std::vector<std::string>( result.begin(), result.end() );
}

bool 
PcpCache::IsInvalidSublayerIdentifier(const std::string& identifier) const
{
    TRACE_FUNCTION();

    std::vector<std::string> layers = GetInvalidSublayerIdentifiers();
    std::vector<std::string>::const_iterator i =
        std::find(layers.begin(), layers.end(), identifier);
    return i != layers.end();
}

std::map<SdfPath, std::vector<std::string>, SdfPath::FastLessThan>
PcpCache::GetInvalidAssetPaths() const
{
    TRACE_FUNCTION();

    std::map<SdfPath, std::vector<std::string>, SdfPath::FastLessThan> result;

    TF_FOR_ALL(it, _primIndexCache) {
        const SdfPath& primPath = it->first;
        const PcpPrimIndex& primIndex = it->second;
        if (primIndex.IsValid()) {
            PcpErrorVector errors = primIndex.GetLocalErrors();
            for (const auto& e : errors) {
                if (PcpErrorInvalidAssetPathPtr typedErr =
                    dynamic_pointer_cast<PcpErrorInvalidAssetPath>(e)){
                    result[primPath].push_back(typedErr->resolvedAssetPath);
                }
            }
        }
    }

    return result;
}

bool
PcpCache::IsInvalidAssetPath(const std::string& resolvedAssetPath) const
{
    TRACE_FUNCTION();

    std::map<SdfPath, std::vector<std::string>, SdfPath::FastLessThan>
        pathMap = GetInvalidAssetPaths();
    TF_FOR_ALL(i, pathMap) {
        TF_FOR_ALL(j, i->second) {
            if (*j == resolvedAssetPath) {
                return true;
            }
        }
    }
    return false;
}

void
PcpCache::Apply(const PcpCacheChanges& changes, PcpLifeboat* lifeboat)
{
    TRACE_FUNCTION();

    // Check for special case of blowing everything.
    if (changes.didChangeSignificantly.count(SdfPath::AbsoluteRootPath())) {
        // Clear everything for scene graph objects.
        _primIndexCache.clear();
        _propertyIndexCache.clear();
        _primDependencies->RemoveAll(lifeboat);
    }
    else {
        // Blow prim and property indexes due to prim graph changes.
        TF_FOR_ALL(i, changes.didChangeSignificantly) {
            const SdfPath& path = *i;
            if (path.IsPrimPath()) {
                _RemovePrimAndPropertyCaches(path, lifeboat);
            }
            else {
                _RemovePropertyCaches(path, lifeboat);
            }
        }

        // Blow prim and property indexes due to prim graph changes.
        TF_FOR_ALL(i, changes.didChangePrims) {
            _RemovePrimCache(*i, lifeboat);
            _RemovePropertyCaches(*i, lifeboat);
        }

        // Blow property stacks and update spec dependencies on prims.
        auto updateSpecStacks = [this, &lifeboat](const SdfPath& path) {
            if (path.IsAbsoluteRootOrPrimPath()) {
                // We've possibly changed the prim spec stack.  Note that
                // we may have blown the prim index so check that it exists.
                if (PcpPrimIndex* primIndex = _GetPrimIndex(path)) {
                    Pcp_RescanForSpecs(primIndex, IsUsd(),
                                       /* updateHasSpecs */ true);

                    // If there are no specs left then we can discard the
                    // prim index.
                    bool anyNodeHasSpecs = false;
                    for (const PcpNodeRef &node: primIndex->GetNodeRange()) {
                        if (node.HasSpecs()) {
                            anyNodeHasSpecs = true;
                            break;
                        }
                    }
                    if (!anyNodeHasSpecs) {
                        _RemovePrimAndPropertyCaches(path, lifeboat);
                    }
                }
            }
            else if (path.IsPropertyPath()) {
                _RemovePropertyCache(path, lifeboat);
            }
            else if (path.IsTargetPath()) {
                // We have potentially aded or removed a relationship target
                // spec.  This invalidates the property stack for any
                // relational attributes for this target.
                _RemovePropertyCaches(path, lifeboat);
            }
        };

        TF_FOR_ALL(i, changes.didChangeSpecs) {
            updateSpecStacks(*i);
        }

        TF_FOR_ALL(i, changes._didChangeSpecsInternal) {
            updateSpecStacks(*i);
        }

        // Fix the keys for any prim or property under any of the renamed
        // paths.
        // XXX: It'd be nice if this was a usd by just adjusting
        //      paths here and there.
        // First blow all caches under the new names.
        TF_FOR_ALL(i, changes.didChangePath) {
            if (!i->second.IsEmpty()) {
                _RemovePrimAndPropertyCaches(i->second, lifeboat);
            }
        }
        // XXX: Blow the caches at the old names.  We'd rather just
        //      adjust paths here and there in the prim graphs and the
        //      SdfPathTable keys, but the latter isn't possible yet
        //      and the former is inconvenient.
        TF_FOR_ALL(i, changes.didChangePath) {
            _RemovePrimAndPropertyCaches(i->first, lifeboat);
        }
    }

    // Fix up payload paths.  First remove everything we renamed then add
    // the new names.  This avoids any problems where we rename both from
    // and to a path, e.g. B -> C, A -> B.
    // XXX: This is a loop over both the changes and all included
    //      payloads because we have no way to find a prefix in a
    //      hash set of payload paths.  We could store SdfPathSet
    //      but at an increased cost when testing if any given
    //      path is in the set.  We'd have to benchmark to see if
    //      this is more costly or that would be.
    static const bool fixTargetPaths = true;
    std::vector<SdfPath> newIncludes;
    TF_FOR_ALL(i, changes.didChangePath) {
        for (_PayloadSet::iterator j = _includedPayloads.begin();
                j != _includedPayloads.end(); ) {
            // If the payload path has the old path as a prefix then remove
            // the payload path and add the payload path with the old path
            // prefix replaced by the new path.  We don't fix target paths
            // because there can't be any on a payload path.
            if (j->HasPrefix(i->first)) {
                newIncludes.push_back(j->ReplacePrefix(i->first, i->second,
                                                       !fixTargetPaths));
                _includedPayloads.erase(j++);
            }
            else {
                ++j;
            }
        }
    }
    _includedPayloads.insert(newIncludes.begin(), newIncludes.end());
}

void
PcpCache::Reload(PcpChanges* changes)
{
    TRACE_FUNCTION();

    if (!_layerStack) {
        return;
    }

    ArResolverContextBinder binder(_pathResolverContext);

    // Reload every invalid sublayer and asset we know about,
    // in any layer stack or prim index.
    std::vector<PcpLayerStackPtr> allLayerStacks =
        _layerStackCache->GetAllLayerStacks();
    TF_FOR_ALL(layerStack, allLayerStacks) {
        const PcpErrorVector errors = (*layerStack)->GetLocalErrors();
        for (const auto& e : errors) {
            if (PcpErrorInvalidSublayerPathPtr typedErr =
                dynamic_pointer_cast<PcpErrorInvalidSublayerPath>(e)) {
                changes->DidMaybeFixSublayer(this,
                                             typedErr->layer,
                                             typedErr->sublayerPath);
            }
        }
    }
    TF_FOR_ALL(it, _primIndexCache) {
        const PcpPrimIndex& primIndex = it->second;
        if (primIndex.IsValid()) {
            const PcpErrorVector errors = primIndex.GetLocalErrors();
            for (const auto& e : errors) {
                if (PcpErrorInvalidAssetPathPtr typedErr =
                    dynamic_pointer_cast<PcpErrorInvalidAssetPath>(e)) {
                    changes->DidMaybeFixAsset(this,
                                              typedErr->site,
                                              typedErr->layer,
                                              typedErr->resolvedAssetPath);
                }
            }
        }
    }

    // Reload every layer we've reached except the session layers (which we
    // never want to reload from disk).
    SdfLayerHandleSet layersToReload = GetUsedLayers();

    for (const SdfLayerHandle &layer : _layerStack->GetSessionLayers()) {
        layersToReload.erase(layer);
    }

    SdfLayer::ReloadLayers(layersToReload);
}

void
PcpCache::ReloadReferences(PcpChanges* changes, const SdfPath& primPath)
{
    TRACE_FUNCTION();

    ArResolverContextBinder binder(_pathResolverContext);

    // Traverse every PrimIndex at or under primPath to find
    // InvalidAssetPath errors, and collect the unique layer stacks used.
    std::set<PcpLayerStackPtr> layerStacksAtOrUnderPrim;
    const auto range = _primIndexCache.FindSubtreeRange(primPath);
    for (auto entryIter = range.first; entryIter != range.second; ++entryIter) {
        const auto& entry = *entryIter;
        const PcpPrimIndex& primIndex = entry.second;
        if (primIndex.IsValid()) {
            PcpErrorVector errors = primIndex.GetLocalErrors();
            for (const auto& e : errors) {
                if (PcpErrorInvalidAssetPathPtr typedErr =
                    dynamic_pointer_cast<PcpErrorInvalidAssetPath>(e))
                {
                    changes->DidMaybeFixAsset(this, typedErr->site,
                                              typedErr->layer,
                                              typedErr->resolvedAssetPath);
                }
            }
            for (const PcpNodeRef &node: primIndex.GetNodeRange()) {
                layerStacksAtOrUnderPrim.insert( node.GetSite().layerStack );
            }
        }
    }

    // Check each used layer stack (gathered above) for invalid sublayers.
    for (const PcpLayerStackPtr& layerStack: layerStacksAtOrUnderPrim) {
        // Scan errors for a sublayer error.
        PcpErrorVector errs = layerStack->GetLocalErrors();
        for (const PcpErrorBasePtr &err: errs) {
            if (PcpErrorInvalidSublayerPathPtr typedErr =
                dynamic_pointer_cast<PcpErrorInvalidSublayerPath>(err)){
                changes->DidMaybeFixSublayer(this, typedErr->layer,
                                             typedErr->sublayerPath);
            }
        }
    }

    // Reload every layer used by prims at or under primPath, except for
    // local layers.
    SdfLayerHandleSet layersToReload;
    for (const PcpLayerStackPtr& layerStack: layerStacksAtOrUnderPrim) {
        for (const SdfLayerHandle& layer: layerStack->GetLayers()) {
            if (!_layerStack->HasLayer(layer)) {
                layersToReload.insert(layer);
            }
        }
    }

    SdfLayer::ReloadLayers(layersToReload);
}

void
PcpCache::_RemovePrimCache(const SdfPath& primPath, PcpLifeboat* lifeboat)
{
    _PrimIndexCache::iterator it = _primIndexCache.find(primPath);
    if (it != _primIndexCache.end()) {
        _primDependencies->Remove(it->second, lifeboat);
        PcpPrimIndex empty;
        it->second.Swap(empty);
    }
}

void
PcpCache::_RemovePrimAndPropertyCaches(const SdfPath& root,
                                       PcpLifeboat* lifeboat)
{
    std::pair<_PrimIndexCache::iterator, _PrimIndexCache::iterator> range =
        _primIndexCache.FindSubtreeRange(root);
    for (_PrimIndexCache::iterator i = range.first; i != range.second; ++i) {
        _primDependencies->Remove(i->second, lifeboat);
    }
    if (range.first != range.second) {
        _primIndexCache.erase(range.first);
    }

    // Remove all properties under any removed prim.
    _RemovePropertyCaches(root, lifeboat);
}

void 
PcpCache::_RemovePropertyCache(const SdfPath& root, PcpLifeboat* lifeboat)
{
    _PropertyIndexCache::iterator it = _propertyIndexCache.find(root);
    if (it != _propertyIndexCache.end()) {
        PcpPropertyIndex empty;
        it->second.Swap(empty);
    }
}

void
PcpCache::_RemovePropertyCaches(const SdfPath& root, PcpLifeboat* lifeboat)
{
    std::pair<_PropertyIndexCache::iterator,
              _PropertyIndexCache::iterator> range =
        _propertyIndexCache.FindSubtreeRange(root);

    if (range.first != range.second) {
        _propertyIndexCache.erase(range.first);
    }
}

////////////////////////////////////////////////////////////////////////
// Private helper methods.

PcpPrimIndex*
PcpCache::_GetPrimIndex(const SdfPath& path)
{
    _PrimIndexCache::iterator i = _primIndexCache.find(path);
    if (i != _primIndexCache.end()) {
        PcpPrimIndex &primIndex = i->second;
        if (primIndex.IsValid()) {
            return &primIndex;
        }
    }
    return NULL;
}

const PcpPrimIndex*
PcpCache::_GetPrimIndex(const SdfPath& path) const
{
    _PrimIndexCache::const_iterator i = _primIndexCache.find(path);
    if (i != _primIndexCache.end()) {
        const PcpPrimIndex &primIndex = i->second;
        if (primIndex.IsValid()) {
            return &primIndex;
        }
    }
    return NULL;
}

template <class ChildrenPredicate>
struct Pcp_ParallelIndexer
{
    typedef Pcp_ParallelIndexer This;

    Pcp_ParallelIndexer(PcpCache *cache,
                        ChildrenPredicate childrenPred,
                        const PcpLayerStackPtr &layerStack,
                        PcpPrimIndexInputs baseInputs,
                        PcpErrorVector *allErrors,
                        const ArResolverScopedCache* parentCache,
                        const char *mallocTag1,
                        const char *mallocTag2)
        : _cache(cache)
        , _allErrors(allErrors)
        , _childrenPredicate(childrenPred)
        , _layerStack(layerStack)
        , _baseInputs(baseInputs)
        , _resolver(ArGetResolver())
        , _consumer(_dispatcher, &This::_ConsumeIndexes, this, /*flush=*/false)
        , _parentCache(parentCache)
        , _mallocTag1(mallocTag1)
        , _mallocTag2(mallocTag2)
    {
        // Set the payload predicate in the base inputs, as well as the
        // includedPayloadsMutex.
        _baseInputs
            .IncludedPayloadsMutex(&_includedPayloadsMutex)
            ;
    }

    ~Pcp_ParallelIndexer() {
        // Tear down async.
        WorkSwapDestroyAsync(_toCompute);
        WorkMoveDestroyAsync(_finishedOutputs);
        WorkSwapDestroyAsync(_consumerScratch);
        WorkSwapDestroyAsync(_consumerScratchPayloads);

        // We need to tear down the _results synchronously because doing so may
        // drop layers, and that's something that clients rely on, but we can
        // tear down the elements in parallel.
        WorkParallelForEach(_results.begin(), _results.end(),
                            [](PcpPrimIndexOutputs &out) {
                                PcpPrimIndexOutputs().swap(out);
                            });
    }

    // Run the added work and wait for it to complete.
    void RunAndWait() {
        TF_FOR_ALL(i, _toCompute) {
            _dispatcher.Run(&This::_ComputeIndex, this, i->first, i->second,
                            /*checkCache=*/true);
        }
        _dispatcher.Wait();

        // Flush any left-over results.
        _ConsumeIndexes(/*flush=*/true);
    }

    // Add an index to compute.
    void ComputeIndex(const PcpPrimIndex *parentIndex, const SdfPath &path) {
        TF_AXIOM(parentIndex || path == SdfPath::AbsoluteRootPath());
        _toCompute.push_back(make_pair(parentIndex, path));
    }

  private:

    // This function is run in parallel by the _dispatcher.  It computes prim
    // indexes and publishes them to _finishedOutputs, which are then consumed
    // by _ConsumeIndex().
    void _ComputeIndex(const PcpPrimIndex *parentIndex,
                       SdfPath path, bool checkCache) {
        TfAutoMallocTag2  tag(_mallocTag1, _mallocTag2);
        ArResolverScopedCache taskCache(_parentCache);

        // Check to see if we already have an index for this guy.  If we do,
        // don't bother computing it.
        const PcpPrimIndex *index = NULL;
        if (checkCache) {
            tbb::spin_rw_mutex::scoped_lock
                lock(_primIndexCacheMutex, /*write=*/false);
            PcpCache::_PrimIndexCache::const_iterator
                i = _cache->_primIndexCache.find(path);
            if (i == _cache->_primIndexCache.end()) {
                // There is no cache entry for this path or any children.
                checkCache = false;
            } else if (i->second.IsValid()) {
                // There is a valid cache entry.
                index = &i->second;
            } else {
                // There is a cache entry but it is invalid.  There still
                // may be valid cache entries for children, so we must
                // continue to checkCache.  An example is when adding a
                // new empty spec to a layer stack already used by a
                // prim, causing a culled node to no longer be culled,
                // and the children to be unaffected.
            }
        }

        PcpPrimIndexOutputs *outputs = NULL;
        if (!index) {
            // We didn't find an index in the cache, so we must compute one.

            // Make space in the results for the output.
            outputs = &(*_results.grow_by(1));
            
            // Establish inputs.
            PcpPrimIndexInputs inputs = _baseInputs;
            inputs.parentIndex = parentIndex;

            TF_VERIFY(parentIndex || path == SdfPath::AbsoluteRootPath());
        
            // Run indexing.
            PcpComputePrimIndex(
                path, _layerStack, inputs, outputs, &_resolver);

            // Now we have an index in hand.
            index = &outputs->primIndex;
        }

        // Invoke the client's predicate to see if we should do children.
        bool didChildren = false;
        TfTokenVector namesToCompose;
        if (_childrenPredicate(*index, &namesToCompose)) {
            // Compute the children paths and add new tasks for them.
            TfTokenVector names;
            PcpTokenSet prohibitedNames;
            index->ComputePrimChildNames(&names, &prohibitedNames);
            for (const auto& name : names) {
                if (!namesToCompose.empty() &&
                    std::find(namesToCompose.begin(), namesToCompose.end(), 
                              name) == namesToCompose.end()) {
                    continue;
                }

                didChildren = true;
                _dispatcher.Run(
                    &This::_ComputeIndex, this, index,
                    path.AppendChild(name), checkCache);
            }
        }

        if (outputs) {
            // We're done with this index, arrange for it to be added to the
            // cache and dependencies, then wake the consumer if we didn't have
            // any children to process.  If we did have children to process
            // we'll let them wake the consumer later.
            _finishedOutputs.push(outputs);
            if (!didChildren)
                _consumer.Wake();
        }
    }

    // This is the task that consumes completed indexes.  It's run as a task in
    // the dispatcher as a WorkSingularTask to ensure that at most one is ever
    // running at once.  This lets us avoid locking while publishing the results
    // to cache-wide datastructures.
    void _ConsumeIndexes(bool flush) {
        TfAutoMallocTag2 tag(_mallocTag1, _mallocTag2);

        // While running, consume results from _finishedOutputs.
        PcpPrimIndexOutputs *outputs;
        while (_finishedOutputs.try_pop(outputs)) {
            // Append errors.
            _allErrors->insert(_allErrors->end(),
                               outputs->allErrors.begin(),
                               outputs->allErrors.end());

            SdfPath const &primIndexPath = outputs->primIndex.GetPath();

            // Store index off to the side so we can publish several at once,
            // ideally.  We have to make a copy to move into the _cache itself,
            // since sibling caches in other tasks will still require that their
            // parent be valid.
            _consumerScratch.push_back(outputs->primIndex);

            // Store included payload path to the side to publish several at
            // once, as well.
            if (outputs->includedDiscoveredPayload)
                _consumerScratchPayloads.push_back(primIndexPath);
        }

        // This size threshold is arbitrary but helps ensure that even with
        // writer starvation we'll avoid growing our working spaces too large.
        static const size_t PendingSizeThreshold = 20000;

        if (!_consumerScratchPayloads.empty()) {
            // Publish to _includedPayloads if possible.  If we're told to
            // flush, or if we're over a threshold number of pending results,
            // then take the write lock and publish.  Otherwise only attempt to
            // take the write lock, and if we fail to do so then we do nothing,
            // since we're guaranteed to run again.  This helps minimize
            // contention and maximize throughput.
            tbb::spin_rw_mutex::scoped_lock lock;

            bool locked = flush ||
                _consumerScratch.size() >= PendingSizeThreshold;
            if (locked) {
                lock.acquire(_includedPayloadsMutex, /*write=*/true);
            } else {
                locked = lock.try_acquire(
                    _includedPayloadsMutex, /*write=*/true);
            }
            if (locked) {
                for (auto const &path: _consumerScratchPayloads) {
                    _cache->_includedPayloads.insert(path);
                }
                lock.release();
                _consumerScratchPayloads.clear();
            }
        }
            
        // Ok, publish the set of indexes.
        if (!_consumerScratch.empty()) {
            // If we're told to flush, or if we're over a threshold number of
            // pending results, then take the write lock and publish.  Otherwise
            // only attempt to take the write lock, and if we fail to do so then
            // we do nothing, since we're guaranteed to run again.  This helps
            // minimize contention and maximize throughput.
            tbb::spin_rw_mutex::scoped_lock lock;

            bool locked = flush ||
                _consumerScratch.size() >= PendingSizeThreshold;
            if (locked) {
                lock.acquire(_primIndexCacheMutex, /*write=*/true);
            } else {
                locked = lock.try_acquire(_primIndexCacheMutex, /*write=*/true);
            }
            if (locked) {
                for (auto &index: _consumerScratch) {
                    // Save the prim index in the cache.
                    const SdfPath &path = index.GetPath();
                    PcpPrimIndex &entry = _cache->_primIndexCache[path];
                    if (TF_VERIFY(!entry.IsValid(),
                                  "PrimIndex for %s already exists in cache",
                                  entry.GetPath().GetText())) {
                        entry.Swap(index);
                        _cache->_primDependencies->Add(entry);
                    }
                }
                lock.release();
                _consumerScratch.clear();
            }
        }
    }

    PcpCache *_cache;
    PcpErrorVector *_allErrors;
    ChildrenPredicate _childrenPredicate;
    vector<pair<const PcpPrimIndex *, SdfPath> > _toCompute;
    PcpLayerStackPtr _layerStack;
    PcpPrimIndexInputs _baseInputs;
    tbb::concurrent_vector<PcpPrimIndexOutputs> _results;
    tbb::spin_rw_mutex _primIndexCacheMutex;
    tbb::spin_rw_mutex _includedPayloadsMutex;
    tbb::concurrent_queue<PcpPrimIndexOutputs *> _finishedOutputs;
    vector<PcpPrimIndex> _consumerScratch;
    vector<SdfPath> _consumerScratchPayloads;
    ArResolver& _resolver;
    WorkArenaDispatcher _dispatcher;
    WorkSingularTask _consumer;
    const ArResolverScopedCache* _parentCache;
    char const * const _mallocTag1;
    char const * const _mallocTag2;
};

void
PcpCache::_ComputePrimIndexesInParallel(
    const SdfPathVector &roots,
    PcpErrorVector *allErrors,
    _UntypedIndexingChildrenPredicate childrenPred,
    _UntypedIndexingPayloadPredicate payloadPred,
    const char *mallocTag1,
    const char *mallocTag2)
{
    if (!IsUsd()) {
        TF_CODING_ERROR("Computing prim indexes in parallel only supported "
                        "for USD caches.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    ArResolverScopedCache parentCache;
    TfAutoMallocTag2 tag(mallocTag1, mallocTag2);
    
    using Indexer = Pcp_ParallelIndexer<_UntypedIndexingChildrenPredicate>;

    if (!_layerStack)
        ComputeLayerStack(GetLayerStackIdentifier(), allErrors);

    // General strategy: Compute indexes recursively starting from roots, in
    // parallel.  When we've computed an index, ask the children predicate if we
    // should continue to compute its children indexes.  If so, we add all the
    // children as new tasks for threads to pick up.
    //
    // Once all the indexes are computed, add them to the cache and add their
    // dependencies to the dependencies structures.

    PcpPrimIndexInputs inputs = GetPrimIndexInputs()
        .USD(_usd)
        .IncludePayloadPredicate(payloadPred)
        ;
    
    Indexer indexer(this, childrenPred, _layerStack, inputs,
                    allErrors, &parentCache, mallocTag1, mallocTag2);

    for (const auto& rootPath : roots) {
        // Obtain the parent index, if this is not the absolute root.  Note that
        // the call to ComputePrimIndex below is not concurrency safe.
        const PcpPrimIndex *parentIndex =
            rootPath == SdfPath::AbsoluteRootPath() ? nullptr :
            &_ComputePrimIndexWithCompatibleInputs(
                rootPath.GetParentPath(), inputs, allErrors);
        indexer.ComputeIndex(parentIndex, rootPath);
    }

    // Do the indexing and wait for it to complete.
    indexer.RunAndWait();
}

const PcpPrimIndex &
PcpCache::ComputePrimIndex(const SdfPath & path, PcpErrorVector *allErrors) {
    return _ComputePrimIndexWithCompatibleInputs(
        path, GetPrimIndexInputs().USD(_usd), allErrors);
}

const PcpPrimIndex &
PcpCache::_ComputePrimIndexWithCompatibleInputs(
    const SdfPath & path, const PcpPrimIndexInputs &inputs,
    PcpErrorVector *allErrors)
{
    // NOTE:TRACE_FUNCTION() is too much overhead here.

    // Check for a cache hit. Default constructed PcpPrimIndex objects
    // may live in the SdfPathTable for paths that haven't yet been computed,
    // so we have to explicitly check for that.
    _PrimIndexCache::const_iterator i = _primIndexCache.find(path);
    if (i != _primIndexCache.end() && i->second.IsValid()) {
        return i->second;
    }

    TRACE_FUNCTION();

    if (!_layerStack) {
        ComputeLayerStack(GetLayerStackIdentifier(), allErrors);
    }

    // Run the prim indexing algorithm.
    PcpPrimIndexOutputs outputs;
    PcpComputePrimIndex(path, _layerStack, inputs, &outputs);
    allErrors->insert(
        allErrors->end(),
        outputs.allErrors.begin(),
        outputs.allErrors.end());

    // Add dependencies.
    _primDependencies->Add(outputs.primIndex);

    // Update _includedPayloads if we included a discovered payload.
    if (outputs.includedDiscoveredPayload) {
        _includedPayloads.insert(path);
    }

    // Save the prim index.
    PcpPrimIndex &cacheEntry = _primIndexCache[path];
    cacheEntry.Swap(outputs.primIndex);

    return cacheEntry;
}

PcpPropertyIndex*
PcpCache::_GetPropertyIndex(const SdfPath& path)
{
    _PropertyIndexCache::iterator i = _propertyIndexCache.find(path);
    if (i != _propertyIndexCache.end() && !i->second.IsEmpty()) {
        return &i->second;
    }

    return NULL;
}

const PcpPropertyIndex*
PcpCache::_GetPropertyIndex(const SdfPath& path) const
{
    _PropertyIndexCache::const_iterator i = _propertyIndexCache.find(path);
    if (i != _propertyIndexCache.end() && !i->second.IsEmpty()) {
        return &i->second;
    }
    return NULL;
}

const PcpPropertyIndex &
PcpCache::ComputePropertyIndex(const SdfPath & path, PcpErrorVector *allErrors)
{
    TRACE_FUNCTION();

    static PcpPropertyIndex nullIndex;
    if (!path.IsPropertyPath()) {
        TF_CODING_ERROR("Path <%s> must be a property path", path.GetText());
        return nullIndex;
    }
    if (_usd) {
        // Disable computation and cache of property indexes in USD mode.
        // Although PcpBuildPropertyIndex does support this computation in
        // USD mode, we do not want to pay the cost of caching these.
        //
        // XXX: Maybe we shouldn't explicitly disallow this, but let consumers
        //      decide if they want this; if they don't, they should just
        //      avoid calling ComputePropertyIndex?
        TF_CODING_ERROR("PcpCache will not compute a cached property index in "
                        "USD mode; use PcpBuildPropertyIndex() instead.  Path "
                        "was <%s>", path.GetText());
        return nullIndex;
    }

    // Check for a cache hit. Default constructed PcpPrimIndex objects
    // may live in the SdfPathTable for paths that haven't yet been computed,
    // so we have to explicitly check for that.
    PcpPropertyIndex &cacheEntry = _propertyIndexCache[path];
    if (cacheEntry.IsEmpty()) {
        PcpBuildPropertyIndex(path, this, &cacheEntry, allErrors);
    }
    return cacheEntry;
}

////////////////////////////////////////////////////////////////////////
// Diagnostics

void 
PcpCache::PrintStatistics() const
{
    Pcp_PrintCacheStatistics(this);
}

PXR_NAMESPACE_CLOSE_SCOPE
