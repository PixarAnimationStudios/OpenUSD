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
#include "pxr/usd/pcp/cache.h"

#include "pxr/usd/pcp/arc.h"
#include "pxr/usd/pcp/changes.h"
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
#include "pxr/base/work/dispatcher.h"
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

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include <algorithm>
#include <utility>
#include <vector>

using std::make_pair;
using std::pair;
using std::vector;

using boost::dynamic_pointer_cast;

TF_DEFINE_ENV_SETTING(
    PCP_CULLING, true,
    "Controls whether culling is enabled in Pcp caches.");

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(PcpCache::UsingSiteOnly);
    TF_ADD_ENUM_NAME(PcpCache::UsingSiteAndDescendants);
    TF_ADD_ENUM_NAME(PcpCache::UsingSiteAndDescendantPrims);
}

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
        if (not _changes) {
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
    _dependencies(new Pcp_Dependencies())
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

    wd.Run([this]() { _dependencies.reset(); });
    wd.Run([this]() { _rootLayer.Reset(); });
    wd.Run([this]() { _sessionLayer.Reset(); });
    wd.Run([this]() { _payloadDecorator.Reset(); });
    wd.Run([this]() { TfReset(_includedPayloads); });
    wd.Run([this]() { TfReset(_variantFallbackMap); });
    wd.Run([this]() { _primIndexCache.ClearInParallel(); });
    wd.Run([this]() { TfReset(_propertyIndexCache); });

    // Wait, since _layerStackCache cannot be destroyed until _dependencies is
    // cleaned up.
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

    if (finalLayersToMute.empty() and finalLayersToUnmute.empty()) {
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
    if (not finalLayersToUnmute.empty()) {
        for (const auto& primIndexEntry : _primIndexCache) {
            const PcpPrimIndex& primIndex = primIndexEntry.second;
            if (not primIndex.GetRootNode()) {
                continue;
            }

            for (const auto& error : primIndex.GetLocalErrors()) {
                PcpErrorMutedAssetPathPtr typedError = 
                    dynamic_pointer_cast<PcpErrorMutedAssetPath>(error);
                if (not typedError) {
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
    if (not _layerStack and id == GetLayerStackIdentifier()) {
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

    if (not relPath.IsPropertyPath()) {
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

    if (not attrPath.IsPropertyPath()) {
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
    SdfLayerHandleSet rval = _dependencies->GetUsedLayers();

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
    SdfLayerHandleSet rval = _dependencies->GetUsedRootLayers();

    // Dependencies don't include the local layer stack, so manually add the
    // local root layer here.
    rval.insert(_rootLayer);
    return rval;
}

SdfPathVector 
PcpCache::_GetPathsUsingPcpSite(
    const PcpLayerStackPtr& layerStack,
    const SdfPath& path,
    unsigned int dependencyType,
    PcpNodeRefVector* sourceNodes,
    bool recursive,
    bool spooky) const
{
    TRACE_FUNCTION();

    if (layerStack->_registry != _layerStackCache) {
        TF_CODING_ERROR("PcpLayerStack does not belong to this PcpCache");
        return SdfPathVector();
    }

    const SdfPathVector results = 
        _dependencies->Get(layerStack, path, dependencyType, recursive, spooky);

    // If asked, we need to do some extra processing to figure out the nodes
    // that introduced the dependencies on the given Pcp site. In the simple
    // case, we would just need to grab the prim index for each result and
    // find the node that uses that site. However, this doesn't always work.
    // We may not have actually computed the prim index for each result path,
    // or we the node that actually uses the exact Pcp site may have been
    // culled out. Both cases happen when dealing with ancestral dependencies.
    //
    // To produce the desired nodes, this code walks up namespace until it
    // finds the node that introduced the dependency. This means that the
    // nodes that are returned are not always from the prim index for each path 
    // returned. However, these nodes are still valid and usable for path
    // translation.
    if (sourceNodes) {
        sourceNodes->clear();
        sourceNodes->reserve(results.size());
        
        BOOST_FOREACH(const SdfPath& pcpSitePath, results) {
            SdfPath primUsingSite = pcpSitePath;
            PcpLayerStackSite usedSite(layerStack, path);
            PcpNodeRef nodeUsingSite;

            while (primUsingSite.IsAbsoluteRootOrPrimPath()) {
                if (const PcpPrimIndex* primIndex = 
                        _GetPrimIndex(primUsingSite)) {

                    TF_FOR_ALL(it, primIndex->GetNodeRange()) {
                        const PcpLayerStackSite& nodeSite = it->GetSite();
                        if (nodeSite.layerStack == usedSite.layerStack and 
                            nodeSite.path.HasPrefix(usedSite.path)) {
                            nodeUsingSite = *it;
                            goto found;
                        }
                    }
                }

                if (not usedSite.path.IsPrimVariantSelectionPath()) {
                    primUsingSite = primUsingSite.GetParentPath();
                }
                usedSite.path = usedSite.path.GetParentPath();
            }

        found:
            TF_VERIFY(
                nodeUsingSite, 
                "Unable to find node that introduced dependency on Pcp site %s"
                "for prim <%s>", 
                TfStringify(PcpLayerStackSite(layerStack, path)).c_str(),
                pcpSitePath.GetText());

            sourceNodes->push_back(nodeUsingSite);
        }
    }

    TF_VERIFY(not sourceNodes or results.size() == sourceNodes->size(),
              "%zu sourceNodes != %zu results -- they must correspond",
              sourceNodes->size(), results.size());
    return results;
}

bool 
PcpCache::_UsesLayer(const SdfLayerHandle& layer) const
{
    const PcpLayerStackPtrVector& layerStacks = 
        FindAllLayerStacksUsingLayer(layer);

    BOOST_FOREACH(const PcpLayerStackPtr& layerStack, layerStacks) {
        // PcpCache doesn't record dependencies on its root layer stack,
        // so we have to check that explicitly.
        if (layerStack == _layerStack or
            _dependencies->UsesLayerStack(layerStack)) {
            return true;
        }
    }
    
    return false;
}

static bool
_IsAbsoluteRootOrPrimOrVariantPath(const SdfPath& path)
{
    return (path == SdfPath::AbsoluteRootPath() or 
            path.IsPrimOrPrimVariantSelectionPath());
}

static SdfPath
_GetAbsoluteRootOrPrimOrVariantPath(const SdfPath& path)
{
    return path == SdfPath::AbsoluteRootPath() ? 
        path : path.GetPrimOrPrimVariantSelectionPath();
}

SdfPathVector
PcpCache::_GetPathsUsingPrimFromDependencies(
    const SdfLayerHandle& layer,
    const SdfPath& path,
    PcpNodeRefVector* sourceNodes,
    UsingSite usingSite,
    bool spookyDependencies) const
{
    // PcpCache only stores dependencies for prims and variants
    if (not TF_VERIFY(
            _IsAbsoluteRootOrPrimOrVariantPath(path),
            "Expected prim path, got: <%s>", path.GetText())) {
        return SdfPathVector();
    }

    // Map UsingSite to flags.
    static const bool recursive[] = { false, true,  true };
    static const bool primsOnly[] = { false, false, true };

    // Get the dependencies.
    SdfPathVector result =
        _dependencies->Get(layer, path,
                           recursive[usingSite],
                           primsOnly[usingSite] or spookyDependencies,
                           spookyDependencies, 
                           sourceNodes);

    // Special case for the pseudo-root.  
    //
    // XXX: It seems like this special case shouldn't be necessary -- shouldn't
    //      we be capturing spec dependencies for the pseudo-root? Removing
    //      this causes at least one test (testCsdLayering6) to fail, though.
    if (path == SdfPath::AbsoluteRootPath()) {
        if (_layerStack and _layerStack->HasLayer(layer)) {
            switch (usingSite) {
            case UsingSiteOnly:
                result.push_back(path);
                if (sourceNodes) {
                    const PcpPrimIndex* rootIndex = _GetPrimIndex(path);
                    if (TF_VERIFY(rootIndex)) {
                        sourceNodes->push_back(rootIndex->GetRootNode());
                    }
                }
                break;

            // This function is only expected to return prims, so we don't
            // need to look for descendant properties -- that will be up
            // to the consumer.
            case UsingSiteAndDescendants:
            case UsingSiteAndDescendantPrims:
                TF_FOR_ALL(it, _primIndexCache) {
                    const SdfPath& primPath = it->first;
                    const PcpPrimIndex& primIndex = it->second;
                    if (primIndex.GetRootNode()) {
                        result.push_back(primPath);
                        if (sourceNodes) {
                            sourceNodes->push_back(primIndex.GetRootNode());
                        }
                    }
                }
                break;
            }
        }
    }

    TF_VERIFY(not sourceNodes or result.size() == sourceNodes->size(),
              "%zu sourceNodes != %zu results -- they must correspond",
              sourceNodes->size(), result.size());

    return result;
}

struct PcpCache::_PathDebugInfo
{
    std::vector<std::string> descriptions;
    std::vector<SdfPathVector> paths;
    std::vector<PcpNodeRefVector> sourceNodes;
};

#define PCP_PATH_DEBUG(resultPaths, resultSourceNodes, ...)             \
    if (debugInfo) {                                                    \
        debugInfo->descriptions.push_back(TfStringPrintf(__VA_ARGS__)); \
        debugInfo->paths.push_back(resultPaths);                        \
        debugInfo->sourceNodes.push_back(resultSourceNodes);            \
    }

SdfPathVector
PcpCache::_GetPathsUsingPrim(
    const SdfLayerHandle& layer,
    const SdfPath& path,
    PcpNodeRefVector* sourceNodes,
    UsingSite usingSite,
    const SdfPath& fallbackAncestor,
    bool spookyDependencies,
    _PathDebugInfo* debugInfo) const
{
    if (not TF_VERIFY(
            _IsAbsoluteRootOrPrimOrVariantPath(path),
            "Expected prim path, got: <%s>", path.GetText())) {
        return SdfPathVector();
    }

    // Dummy variables to make some code below a bit shorter.
    PcpNodeRefVector dummy;
    const PcpNodeRefVector& resultNodes = (sourceNodes ? *sourceNodes : dummy);

    SdfPathVector result = 
        _GetPathsUsingPrimFromDependencies(layer, path, sourceNodes,
                                           usingSite, spookyDependencies);

    // Found dependencies for the search path.
    if (not result.empty()) {
        PCP_PATH_DEBUG(result, resultNodes, "found by spec");
    }
    else if (spookyDependencies) {
        // Spooky dependencies can only be queried on prim paths.
        TF_VERIFY(path != SdfPath::AbsoluteRootPath());
        TF_VERIFY(fallbackAncestor.IsEmpty());
        
        // Spooky dependencies cannot be queried recursively
        // (because they are due to namespace relocations).
        static const bool recursive = true;

        const PcpLayerStackPtrVector& layerStacks =
            FindAllLayerStacksUsingLayer(layer);
        BOOST_FOREACH(const PcpLayerStackPtr& layerStack, layerStacks) {
            PcpNodeRefVector spookyNodes;
            const SdfPathVector spookyPaths = 
                _GetPathsUsingPcpSite(layerStack, path,
                                      PcpDirect | PcpAncestral, 
                                      sourceNodes ? &spookyNodes : NULL,
                                      not recursive, spookyDependencies);
            if (not spookyPaths.empty()) {
                result.insert(
                    result.end(), spookyPaths.begin(), spookyPaths.end());

                if (sourceNodes) {
                    sourceNodes->insert(
                        sourceNodes->end(), 
                        spookyNodes.begin(), spookyNodes.end());
                }

                PCP_PATH_DEBUG(
                    spookyPaths, spookyNodes,
                    "found by spooky arc to site %s",
                    TfStringify(PcpLayerStackSite(layerStack, path)).c_str());
            }
        }
    }
    // No dependencies.  If no fallback and no spooky dependenceies to
    // check then we're done.
    else if (fallbackAncestor.IsEmpty()) {
        // Do nothing
    }

    // Try dependencies on the name parent prim for prim paths.
    else if (path.IsPrimOrPrimVariantSelectionPath()) {
        // Find dependencies on the parent.  Don't check recursively because
        // that would search siblings of path, which we don't want, and
        // because since there is no opinion at path there can't be any
        // opinions below path so there can't be any dependencies on anything
        // below path.  Since we recursively blow at the dependencies we find,
        // we're sure to update any prim index that depends on anything below
        // path.
        result = _GetPathsUsingPrimFromDependencies(
            layer, fallbackAncestor, sourceNodes);

        if (not result.empty()) {
            BOOST_FOREACH(SdfPath& resultPath, result) {
                // Strip out any variant selections from the result path,
                // as each should be a Pcp site path and not a storage path.
                // XXX: This probably isn't handling
                //      relocations in some important way.
                resultPath = path
                    .ReplacePrefix(fallbackAncestor, resultPath)
                    .StripAllVariantSelections();
            }

            PCP_PATH_DEBUG(result, resultNodes, "found by ancestor spec");
        }

        // The above isn't quite enough.  If we inherit from a site with no
        // opinions then add a spec there, we won't yet have dependencies on
        // that spec (so the initial GetPathsUsingSite() call failed) and
        // our parent will not have a relevant dependency.  We must use the
        // Pcp site to site dependencies to find that dangling inherit.
        const PcpLayerStackPtrVector& layerStacks =
            FindAllLayerStacksUsingLayer(layer);
        BOOST_FOREACH(const PcpLayerStackPtr& layerStack, layerStacks) {
            PcpNodeRefVector nodes;
            const SdfPathVector paths = 
                _GetPathsUsingPcpSite(layerStack, path,
                                      PcpDirect | PcpAncestral, 
                                      sourceNodes ? &nodes : NULL);

            if (not paths.empty()) {
                result.insert(result.end(), paths.begin(), paths.end());

                if (sourceNodes) {
                    sourceNodes->insert(
                        sourceNodes->end(), nodes.begin(), nodes.end());
                }

                PCP_PATH_DEBUG(
                    paths, nodes,
                    "found by arc to site %s",
                    TfStringify(PcpLayerStackSite(layerStack, path)).c_str());
            }
        }
    }

    TF_VERIFY(not sourceNodes or result.size() == sourceNodes->size(),
              "%zu sourceNodes != %zu results -- they must correspond",
              sourceNodes->size(), result.size());

    return result;
}

SdfPathVector
PcpCache::_GetPathsUsingSite(
    const SdfLayerHandle& layer,
    const SdfPath& path,
    UsingSite usingSite,
    SdfLayerOffsetVector* layerOffsets,
    PcpNodeRefVector* sourceNodes,
    const SdfPath& fallbackAncestor,
    bool spookyDependencies,
    std::string* debugSummary) const
{
    if (not TF_VERIFY(path.IsAbsolutePath())) {
        return SdfPathVector();
    }

    SdfPathVector result;

    // Instantiate a struct to hold on to debug info; this will only be used
    // if we need to fill in debugSummary.
    _PathDebugInfo localDebugInfo;
    _PathDebugInfo* debugInfo = debugSummary ? &localDebugInfo : NULL;

    // Flag to determine whether path translation is needed after dependencies
    // are retrieved. If translation is required, sourceNodes must be filled
    // with the nodes from which each dependency originates.
    PcpNodeRefVector localSourceNodes;
    bool needsPathTranslation = (layerOffsets != NULL);
    bool needsSourceNodes = (needsPathTranslation or sourceNodes);

    // If layerOffsets are requested, we will need to go through the path
    // translation mechanism to compute them after we find dependencies.
    if (layerOffsets) {
        layerOffsets->clear();
    }

    // We can bail out immediately if there are no prim indexes that use
    // the given layer. 
    if (not _UsesLayer(layer)) {
        return result;
    }

    // Pcp_Dependencies explicitly stores prim dependencies, so if we're
    // given a prim path, we can just query the object directly.
    if (_IsAbsoluteRootOrPrimOrVariantPath(path)) {
        result = _GetPathsUsingPrim(
            layer, path, 
            needsSourceNodes ? &localSourceNodes : NULL, 
            usingSite, fallbackAncestor, 
            spookyDependencies, debugInfo);
    }

    // For everything else, we need to synthesize the answer from
    // dependencies on the owning prim. We can skip this if we've been
    // asked to return prims only, because at this point we know we've
    // been asked for dependencies on something that isn't a prim.
    else if (usingSite != UsingSiteAndDescendantPrims) {
        const SdfPath owningPrimPath = 
            _GetAbsoluteRootOrPrimOrVariantPath(path);
        const SdfPath fallbackAncestorPrimPath = 
            _GetAbsoluteRootOrPrimOrVariantPath(fallbackAncestor);

        // Optimization: Prim properties are easy -- if we can find
        // the composed prims that depend on the property's owning prim,
        // the dependent paths are just the prim paths plus the property
        // name. This lets us skip some path translations. If we don't
        // find any dependencies, we fall back to using the fallback ancestor 
        // below.
        if (path.IsPrimPropertyPath()) {
            result = _GetPathsUsingPrim(
                layer, owningPrimPath, 
                needsSourceNodes ? &localSourceNodes : NULL);

            if (not result.empty()) {
                // Prim-path-less references/payloads to layers that don't have
                // 'defaultPrim' bill dependencies to the pseudoroot, since they
                // need to be invalidated when that metadata changes, but we
                // can't (and don't need to) translate property paths to those
                // places, so we remove them here.  We must also remove the
                // corresponding elements from localSourceNodes (if
                // needsSourceNodes is true).
                SdfPath const &absRoot = SdfPath::AbsoluteRootPath();
                SdfPathVector::iterator i = result.begin();
                while (i != result.end()) {
                    if (*i == absRoot) {
                        i = result.erase(i);
                        if (needsSourceNodes) {
                            localSourceNodes.erase(localSourceNodes.begin() +
                                                   (i - result.begin()));
                        }
                    } else {
                        ++i;
                    }
                }

                TF_FOR_ALL(it, result) {
                    *it = it->AppendProperty(path.GetNameToken());
                }
                PCP_PATH_DEBUG(
                    result, localSourceNodes, "found by owning prim spec");
            }
        }

        // Determine the composed prims that depend on the owning prim and,
        // more importantly, the nodes in those prim indexes where those
        // dependencies originated. These nodes will be used to translate
        // \p path to the root namespace, which will give us the dependency
        // paths we want.
        if (result.empty()) {
            needsPathTranslation = true;
            needsSourceNodes = true;

            // Spooky dependencies can only be queried for prim paths, so
            // we won't look at spooky dependencies here.
            static const bool spooky = true;
            // We only need dependencies on the owning prim and not any of
            // its descendant prims. We're synthesizing dependencies on a 
            // property beneath this prim and this prim only.
            static const UsingSite owningPrimSiteOnly = UsingSiteOnly;

            result = _GetPathsUsingPrim(
                layer, owningPrimPath, 
                &localSourceNodes, owningPrimSiteOnly, fallbackAncestorPrimPath,
                not spooky, debugInfo);
        }
    }

    // Perform any necessary path translations.
    if (not result.empty() and needsPathTranslation) {
        TF_VERIFY(needsSourceNodes);
        result = _Translate(localSourceNodes, layer, path, layerOffsets);

        // Make sure we also translate the paths we've stored in the debug
        // info. If the debugSummary wasn't requested, the info should be
        // empty and this will do nothing.
        if (debugInfo) {
            TF_VERIFY(debugInfo->paths.size() == debugInfo->sourceNodes.size());
            for (size_t i = 0; i < debugInfo->paths.size(); ++i) {
                debugInfo->paths[i] = _Translate(
                    debugInfo->sourceNodes[i], layer, path, NULL);
            }
        }
    }

    // If asked for descendant dependencies, we need to check for descendant
    // properties beneath the dependencies we've already computed. We don't
    // need to check for descendant prims, as that would have been handled
    // for us earlier in _GetPathsUsingPrim.
    SdfPathVector descendantDeps;
    SdfLayerOffsetVector descendantOffsets;
    PcpNodeRefVector descendantNodes;
    if (not result.empty() and usingSite == UsingSiteAndDescendants) {
        // The descendant dependencies we'll find below won't need path 
        // translation, as they're already in the cache's namespace, unless
        // we need layer offsets.
        const bool descendantsNeedPathTranslation = (layerOffsets != NULL);
        const bool descendantsNeedSourceNodes = 
            (descendantsNeedPathTranslation or needsSourceNodes);

        BOOST_FOREACH(const SdfPath& depPath, result) {
            BOOST_FOREACH(const _PropertyIndexCache::value_type& v,
                          _propertyIndexCache.FindSubtreeRange(depPath)) {

                const SdfPath& propertyPath = v.first;
                const PcpPropertyIndex& propertyIndex = v.second;

                // Check the property index to make sure it actually has a
                // dependency on a descendant of \p path in \p layer. Note
                // that specs in the property index may be invalid at this
                // point if this function is being called during change
                // processing.
                TF_FOR_ALL(propIt, propertyIndex.GetPropertyRange()) {
                    const SdfPropertySpecHandle& spec = *propIt;
                    if (spec and 
                        spec->GetLayer() == layer and 
                        spec->GetPath().HasPrefix(path)) {
                        descendantDeps.push_back(propertyPath);

                        if (descendantsNeedSourceNodes) {
                            descendantNodes.push_back(propIt.base().GetNode());
                        }

                        break;
                    }

                } // end for each property in property stack
            } // end for each property index under depPath
        } // end for each path in result

        if (not descendantDeps.empty()) {
            PCP_PATH_DEBUG(
                descendantDeps, descendantNodes, "found by descendant search");
        }

        if (descendantsNeedPathTranslation) {
            TF_VERIFY(descendantsNeedSourceNodes);
            _Translate(descendantNodes, layer, path, &descendantOffsets);
        }
    }

    // All of the previous code may have synthesized dependent paths that don't
    // refer to any object in the cache. If \p fallbackAncestor is supplied,
    // this is OK -- the consumer wants to know about all possible dependents.
    // Otherwise, we need to filter out the paths that aren't in the cache.
    const bool needsCacheCheck = (fallbackAncestor.IsEmpty());
    if (not result.empty() and needsCacheCheck) {

        struct _CacheChecker {
            static void _RemoveResultsNotInCache(
                const PcpCache* cache,
                SdfPathVector* paths, 
                PcpNodeRefVector* sourceNodes = 0,
                SdfLayerOffsetVector* layerOffsets = 0)
            {
                SdfPathVector existingResults;
                PcpNodeRefVector existingSourceNodes;
                SdfLayerOffsetVector existingOffsets;

                for (size_t i = 0; i < paths->size(); ++i) {
                    SdfPath depPath = (*paths)[i];
                    while (not (depPath.IsAbsoluteRootOrPrimPath() or 
                            depPath.IsPropertyPath())) {
                        depPath = depPath.GetParentPath();
                    }

                    const bool dependentObjectExistsInCache = 
                        (depPath.IsAbsoluteRootOrPrimPath() and 
                         cache->_GetPrimIndex(depPath))
                        or 
                        (depPath.IsPropertyPath() and 
                         cache->_GetPropertyIndex(depPath));

                    if (dependentObjectExistsInCache) {
                        existingResults.push_back((*paths)[i]);

                        if (layerOffsets and not layerOffsets->empty()) {
                            existingOffsets.push_back((*layerOffsets)[i]);
                        }

                        if (sourceNodes and not sourceNodes->empty()) {
                            existingSourceNodes.push_back((*sourceNodes)[i]);
                        }
                    }
                }

                paths->swap(existingResults);
                if (sourceNodes) {
                    sourceNodes->swap(existingSourceNodes);
                }
                if (layerOffsets) { 
                    layerOffsets->swap(existingOffsets);
                }
            }
        };


        _CacheChecker::_RemoveResultsNotInCache(
            this, &result, &localSourceNodes, layerOffsets);

        if (debugInfo) {
            for (size_t i = 0; i < debugInfo->paths.size(); ++i) {
                _CacheChecker::_RemoveResultsNotInCache(
                    this, &debugInfo->paths[i], &debugInfo->sourceNodes[i]);
            }
        }
    }

    // Descendant property dependencies (if any) were generated by inspecting
    // cache contents, so they don't need to be run through the cache existence
    // check above. Just append them to the results.
    result.insert(result.end(), descendantDeps.begin(), descendantDeps.end());
    localSourceNodes.insert(
        localSourceNodes.end(), 
        descendantNodes.begin(), descendantNodes.end());

    if (layerOffsets) {
        layerOffsets->insert(
            layerOffsets->end(), 
            descendantOffsets.begin(), descendantOffsets.end());
    }

    // If consumer wants the source nodes for the dependencies, swap them
    // into place.
    if (sourceNodes) {
        sourceNodes->swap(localSourceNodes);
    }

    // Generate the debug summary if requested.
    if (debugInfo) {
        std::vector<std::string> entries(debugInfo->descriptions.size());

        std::string usingSiteDescription;
        switch (usingSite) {
        case UsingSiteOnly: break;
        case UsingSiteAndDescendants: 
            usingSiteDescription = " and descendants"; break;
        case UsingSiteAndDescendantPrims:
            usingSiteDescription = " and descendant prims"; break;
        }

        for (size_t i = 0, numEntries = entries.size(); i < numEntries; ++i) {
            std::vector<std::string> pathEntries(debugInfo->paths[i].size());
            for (size_t j = 0; j < pathEntries.size(); ++j) {
                pathEntries[j] = 
                    TfStringPrintf("    <%s>", debugInfo->paths[i][j].GetText());
            }

            std::sort(pathEntries.begin(), pathEntries.end());

            entries[i] = TfStringPrintf(
                "Paths using site%s @%s@<%s> (%s):\n%s",
                usingSiteDescription.c_str(),
                layer->GetIdentifier().c_str(), path.GetText(),
                debugInfo->descriptions[i].c_str(),
                TfStringJoin(pathEntries, "\n").c_str());
        }

        std::sort(entries.begin(), entries.end());
        *debugSummary = TfStringJoin(entries, "\n");
    }

    TF_VERIFY(not layerOffsets or layerOffsets->size() == result.size(),
              "%zu layerOffsets != %zu results",
              layerOffsets->size(), result.size());
    TF_VERIFY(not sourceNodes or result.size() == sourceNodes->size(),
              "%zu sourceNodes != %zu results -- they must correspond",
              sourceNodes->size(), result.size());

    return result;
}

SdfPathVector
PcpCache::_Translate(
    const PcpNodeRefVector& depSourceNodes,
    const SdfLayerHandle& layer,
    const SdfPath& path,
    SdfLayerOffsetVector* layerOffsets) const
{

    SdfPathVector result;
    result.reserve(depSourceNodes.size());

    if (layerOffsets) {
        layerOffsets->clear();
        layerOffsets->reserve(depSourceNodes.size());
    }

    BOOST_FOREACH(const PcpNodeRef& sourceNode, depSourceNodes) {
        const SdfPath translatedPath = 
            PcpTranslatePathFromNodeToRoot(sourceNode, path);
        if (not translatedPath.IsEmpty()) {
            result.push_back(translatedPath);

            if (layerOffsets) {
                SdfLayerOffset layerOffset;
                    
                // Apply sublayer offsets.
                if (const SdfLayerOffset* sublayerOffset =
                        sourceNode.GetLayerStack()->
                            GetLayerOffsetForLayer(layer)) {
                    layerOffset = *sublayerOffset * layerOffset;
                }

                // Apply arc offsets.
                layerOffset = 
                    sourceNode.GetMapToRoot().GetTimeOffset() * layerOffset;
                layerOffsets->push_back(layerOffset);
            }
        }
    }

    return result;
}

SdfPathVector
PcpCache::GetPathsUsingSite(
    const SdfLayerHandle& layer,
    const SdfPath& path,
    UsingSite usingSite,
    SdfLayerOffsetVector* layerOffsets,
    const SdfPath& fallbackAncestor) const
{
    return _GetPathsUsingSite(
        layer, path, usingSite, layerOffsets, /* sourceNodes = */ NULL, 
        fallbackAncestor);
}

SdfPathVector
PcpCache::GetPathsUsingSite(
    const PcpLayerStackPtr& layerStack,
    const SdfPath& path,
    unsigned int dependencyType,
    bool recursive) const
{
    return _GetPathsUsingPcpSite(
        layerStack, path, dependencyType, /* sourceNodes = */ NULL, recursive);
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
        PcpNodeRange range = primIndex->GetNodeRange();
        BOOST_FOREACH(const PcpNodeRef& node, range) {
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

static bool
_IsInvalidEdit(const SdfPath& oldPath, const SdfPath& newPath)
{
    // Can't reparent an object to be a descendant of itself.
    // See testPcpRegressionBugs_bug109700 for more details
    // on how this can happen.
    return newPath.HasPrefix(oldPath);
}

static PcpCache::NamespaceEdits::LayerStackSites&
_GetLayerStackSitesForEdit(
    PcpCache::NamespaceEdits* result,
    const SdfPath& oldPath,
    const SdfPath& newPath)
{
    // Catch any invalid fixups that have been requested here and
    // store them away in invalidLayerStackSites so that consumers
    // can be informed about them.
    return _IsInvalidEdit(oldPath, newPath) ?
        result->invalidLayerStackSites : result->layerStackSites;
}

static bool
_RelocatesMapContainsPrimOrDescendant(
    const SdfRelocatesMapProxy& reloMap,
    const SdfPath& primPath)
{
    TF_FOR_ALL(it, reloMap) {
        if (it->first.HasPrefix(primPath) or
            it->second.HasPrefix(primPath)) {
            return true;
        }
    }

    return false;
}

static void
_AddRelocateEditsForLayerStack(
    PcpCache::NamespaceEdits* result,
    const PcpLayerStackPtr& layerStack,
    size_t cacheIndex,
    const SdfPath& oldRelocatePath,
    const SdfPath& newRelocatePath)
{
    if (not result) {
        return;
    }

    // Record a relocates edit for each layer stack site if any prim spec
    // at that site has a relocates statement that contains oldRelocatePath.
    // 
    // XXX: If this is a performance issue, PcpLayerStack could keep track 
    //      of a finer-grained table to avoid scanning through every prim 
    //      with relocates here.
    const SdfPathVector& relocatePrimPaths = 
        layerStack->GetPathsToPrimsWithRelocates();
    TF_FOR_ALL(pathIt, relocatePrimPaths) {
        TF_FOR_ALL(layerIt, layerStack->GetLayers()) {
            // XXX: The prim should always be valid, but might not be due to 
            //      bug 101300
            const SdfPrimSpecHandle prim = (*layerIt)->GetPrimAtPath(*pathIt);
            if (not prim or not prim->HasRelocates()) {
                continue;
            }

            if (_RelocatesMapContainsPrimOrDescendant(
                    prim->GetRelocates(), oldRelocatePath)) {

                PcpCache::NamespaceEdits::LayerStackSites& layerStackSites = 
                    _GetLayerStackSitesForEdit(
                        result, oldRelocatePath, newRelocatePath);

                layerStackSites.resize(layerStackSites.size() + 1);
                PcpCache::NamespaceEdits::LayerStackSite& site =
                    layerStackSites.back();
                site.cacheIndex = cacheIndex;
                site.type       = PcpCache::NamespaceEditRelocate;
                site.sitePath   = *pathIt;
                site.oldPath    = oldRelocatePath;
                site.newPath    = newRelocatePath;
                site.layerStack = layerStack;

                // Since we record edits at the granularity of layer stacks,
                // we can bail out once we've determined that at least one
                // prim in this layer stack needs relocates edits.
                break;
            }
        }
    }
}

static SdfPath
_TranslatePathAndTargetPaths(
    const PcpNodeRef& node,
    const SdfPath& pathIn)
{
    SdfPath path = node.GetMapToParent().MapSourceToTarget(pathIn);

    if (path == pathIn) {
        // We don't want to map paths that aren't explicitly allowed.  The
        // </> -> </> mapping should not be attempted.
        const SdfPath absRoot = SdfPath::AbsoluteRootPath();
        if (node.GetMapToParent().MapSourceToTarget(absRoot) == absRoot) {
            return SdfPath();
        }
    }

    SdfPathVector targetPaths;
    path.GetAllTargetPathsRecursively(&targetPaths);
    BOOST_FOREACH(const SdfPath& targetPath, targetPaths) {
        // Do allow translation via </> -> </> in target paths.
        const SdfPath translatedTargetPath =
            node.GetMapToParent().MapSourceToTarget(targetPath);
        if (translatedTargetPath.IsEmpty()) {
            return SdfPath();
        }
        path = path.ReplacePrefix(targetPath, translatedTargetPath);
    }

    return path;
}

// Translate *oldNodePath and *newNodePath to node's parent's namespace.
// Request any necessary edits to relocations used for the translation.
static void
_TranslatePathsAndEditRelocates(
    PcpCache::NamespaceEdits* result,
    const PcpNodeRef& node,
    size_t cacheIndex,
    SdfPath* oldNodePath,
    SdfPath* newNodePath)
{
    // Map the path in the node namespace to the parent's namespace.  Note
    // that these are not the namespace parent paths, they're the paths in
    // the *node's parent's* namespace.
    SdfPath oldParentPath = _TranslatePathAndTargetPaths(node, *oldNodePath);
    SdfPath newParentPath = _TranslatePathAndTargetPaths(node, *newNodePath);

    // Check if there are relocations that need fixing.
    //
    // At this point, if oldParentPath and newParentPath refer to a 
    // relocated prim or a descendant of a relocated prim, they will have 
    // already had the relevant relocations from the parent's layerStack 
    // applied.
    // Check if one of these relocations affected oldParentPath.
    //
    const PcpLayerStackPtr& layerStack = 
        node.GetParentNode().GetLayerStack();

    // Since oldParentPath and newParentPath are post-relocation,
    // we'll use the targetToSource table to check for applicable
    // relocations.
    const SdfRelocatesMap& relocates = layerStack->GetRelocatesTargetToSource();

    // Find the relocation.
    SdfRelocatesMap::const_iterator i = relocates.lower_bound(oldParentPath);
    if (i != relocates.end() and oldParentPath.HasPrefix(i->first)) {
        const SdfPath & reloTargetPath = i->first;
        const SdfPath & reloSourcePath = i->second;

        // Un-relocate oldParentPath and newParentPath.
        // We will use these paths to decide how to fix the
        // relocation that applied to them.
        const SdfPath unrelocatedOldParentPath =
            oldParentPath.ReplacePrefix(reloTargetPath, reloSourcePath);
        const SdfPath unrelocatedNewParentPath =
            newParentPath.ReplacePrefix(reloTargetPath, reloSourcePath);

        bool reloTargetNeedsEdit = true;

        // Old path is relocated in this layer stack.  We might need to
        // change the relocation as part of the namespace edit.  If so
        // we'll relocate the new path differently from the old path.
        if (oldParentPath == reloTargetPath) {
            // Relocating the new parent path depends on various stuff.
            if (newParentPath.IsEmpty()) {
                // Removing the object or can't map across the arc.
                // Nothing to translate, but need to add a relocation edit
                // to indicate that relocations that involve prims at and
                // below oldParentPath need to be fixed.
                _AddRelocateEditsForLayerStack(
                    result, layerStack, cacheIndex,
                    reloTargetPath, newParentPath);
            }
            else if (oldNodePath->GetParentPath() !=
                     newNodePath->GetParentPath()) {
                // Reparenting within the arc's root.  We'll fix the
                // relocation source but not the target.
                _AddRelocateEditsForLayerStack(
                    result, layerStack, cacheIndex, 
                    unrelocatedOldParentPath, unrelocatedNewParentPath);

                reloTargetNeedsEdit = false;
            }
            else {
                // Renaming.  We must fix the relocation source path,
                // and potentially also the relocation target path
                // (if the relocation keeps the prim name).
                _AddRelocateEditsForLayerStack(
                    result, layerStack, cacheIndex,
                    unrelocatedOldParentPath, unrelocatedNewParentPath);

                // If the relocation keeps the prim name then
                // we'll fix the relocation by changing the final prim
                // name in both the source and target.  So the new parent
                // path is the old parent path with the name changed.
                if (reloSourcePath.GetNameToken()
                    == reloTargetPath.GetNameToken()) {
                    // Relocate the new path.
                    newParentPath = reloTargetPath
                        .ReplaceName(newNodePath->GetNameToken());

                    _AddRelocateEditsForLayerStack(
                        result, layerStack, cacheIndex,
                        reloTargetPath, newParentPath);
                }
                else {
                    // The relocation changes the prim name.  We've no
                    // reason to try to adjust the target's name but we
                    // should change the source name.
                    reloTargetNeedsEdit = false;
                }
            }

            if (not reloTargetNeedsEdit) {
                // Since the relocation target isn't changing, this node
                // 'absorbs' the namespace edit -- no layer stacks that 
                // reference this layer stack need to be updated.  So, 
                // we can stop traversing the graph looking for things
                // that need to be fixed.  Indicate that to consumers by 
                // setting newParentPath = oldParentPath.
                newParentPath = oldParentPath;
            }
        }
        else {
            // We don't need to fix the relocation.
        }
    }
    else {
        // In this case, oldParentPath and newParentPath do not refer to a
        // relocated prim.  However, there may be descendants of oldParentPath
        // that have been relocated, requiring relocates to be fixed up.
        _AddRelocateEditsForLayerStack(
            result, layerStack, cacheIndex,
            oldParentPath, newParentPath);
    }

    *oldNodePath = oldParentPath;
    *newNodePath = newParentPath;
}

static bool
_AddLayerStackSite(
    PcpCache::NamespaceEdits* result,
    const PcpNodeRef& node,
    size_t cacheIndex,
    SdfPath* oldNodePath,
    SdfPath* newNodePath)
{
    bool final = false;

    // Save the old paths.
    SdfPath oldPath = *oldNodePath, newPath = *newNodePath;

    // Translate the paths to the parent.
    _TranslatePathsAndEditRelocates(result, node, cacheIndex,
                                    oldNodePath, newNodePath);

    // The site is the parent's path.
    SdfPath sitePath = *oldNodePath;

    // Compute the type of edit.
    PcpCache::NamespaceEditType type;
    if (node.GetArcType() == PcpArcTypeRelocate) {
        // Ignore.
        *oldNodePath = oldPath;
        *newNodePath = newPath;
        return final;
    }
    else if (*oldNodePath == *newNodePath) {
        // The edit is absorbed by this layer stack, so there's
        // no need to propagate the edit any further.
        final = true;
        return final;
    }
    else if (oldNodePath->IsPrimPath() and not node.IsDueToAncestor()) {
        final = true;
        switch (node.GetArcType()) {
        case PcpArcTypeLocalInherit:
        case PcpArcTypeGlobalInherit:
            type = PcpCache::NamespaceEditInherit;
            break;

        case PcpArcTypeReference:
            type = PcpCache::NamespaceEditReference;
            break;

        case PcpArcTypePayload:
            type = PcpCache::NamespaceEditPayload;
            break;

        case PcpArcTypeVariant:
            // Do nothing.  The variant prim has no name (and therefore
            // nothing referring to the name) so there's nothing to do.
            return final;

        default:
            TF_VERIFY(false, "Unexpected arc type %d", node.GetArcType());
            return final;
        }
    }
    else {
        // NamespaceEditPath the parent.
        type    = PcpCache::NamespaceEditPath;
        oldPath = *oldNodePath;
        newPath = *newNodePath;
    }

    // Add a new layer stack site element at the end.
    PcpCache::NamespaceEdits::LayerStackSites& layerStackSites = 
        _GetLayerStackSitesForEdit(result, oldPath, newPath);

    layerStackSites.resize(layerStackSites.size() + 1);
    PcpCache::NamespaceEdits::LayerStackSite& site = layerStackSites.back();

    // Fill in the site.
    site.cacheIndex = cacheIndex;
    site.type       = type;
    site.sitePath   = sitePath;
    site.oldPath    = oldPath;
    site.newPath    = newPath;
    site.layerStack = node.GetParentNode().GetLayerStack();

    return final;
}

PcpCache::NamespaceEdits
PcpCache::ComputeNamespaceEdits(
    const std::vector<PcpCache*>& caches,
    const SdfPath& curPath,
    const SdfPath& newPath,
    const SdfLayerHandle& relocatesLayer) const
{
    TRACE_FUNCTION();

    static const bool recursive = true;
    static const bool spooky = true;
    static const bool primsOnly = true;

    NamespaceEdits result;
    SdfPathVector depPaths;

    // We find dependencies using prim paths.  Compute the closest prim
    // path to curPath.
    SdfPath primPath = curPath.GetPrimPath();

    // Verify that a prim index at primPath exists.
    if (not _GetPrimIndex(primPath)) {
        TF_CODING_ERROR("No prim index computed for @%s@<%s>\n",
                        _rootLayer->GetIdentifier().c_str(), curPath.GetText());
        return result;
    }

    // Handle trivial case.
    if (curPath == newPath) {
        return result;
    }

    // Find cache sites one cache at a time.  We can't simply check if a
    // site uses (_layerStack, primPath) -- we must check if it uses any
    // site at primPath with an intersecting layer stack.  Even that's not
    // quite right -- we only care if the layer stacks intersect where a
    // spec already exists (see bug 59216).  And, unfortunately, that's
    // not right either -- if (_layerStack, primPath) has no specs at all
    // (because opinions come across an ancestor arc) then we're doing a
    // relocation and only sites using primPath in a layer stack that
    // includes relocatesLayer are affected.  We special case the last
    // case.  The earlier cases we handle by looking for any site using
    // any spec at the namespace edited site.

    // Find all specs at (_layerStack, primPath).
    SdfSiteVector primSites;
    PcpComposeSitePrimSites(
        PcpLayerStackSite(_layerStack,primPath), &primSites);

    // Find the nodes corresponding to primPath in any relevant layer
    // stack over all caches.
    typedef std::pair<size_t, PcpNodeRef> CacheNodePair;
    std::set<CacheNodePair> nodes, descendantNodes;

    struct _CacheNodeHelper {
        static void InsertCacheNodePair(size_t cacheIdx, PcpNodeRef node,
                                        std::set<CacheNodePair>* nodes)
        {
            // If a dependency on primPath was introduced via a variant
            // node (e.g., a prim authored locally in a variant), we store
            // the node that introduced the variant as this truly represents
            // the namespace edited site.
            while (node and node.GetArcType() == PcpArcTypeVariant) {
                node = node.GetParentNode();
            }

            if (TF_VERIFY(node)) {
                nodes->insert(std::make_pair(cacheIdx, node));
            }
        }
    };

    if (primSites.empty()) {
        // This is the relocation case and we don't have any convenient
        // dependency to report sites using (relocatesLayer, primPath).
        // So we'll find every site using (someLayerStack, primPath)
        // where someLayerStack is any layer stack that includes
        // relocatesLayer.
        for (size_t cacheIndex = 0, n = caches.size();
                                        cacheIndex != n; ++cacheIndex) {
            PcpCache* cache = caches[cacheIndex];
            const PcpLayerStackPtrVector& layerStacks =
                cache->FindAllLayerStacksUsingLayer(relocatesLayer);

            // Get all sites in cache that depend on primPath in any of
            // the layer stacks.
            BOOST_FOREACH(const PcpLayerStackPtr& layerStack, layerStacks) {
                PcpNodeRefVector depNodes;
                depPaths = cache->_GetPathsUsingPcpSite(
                    layerStack, primPath, PcpDirect | PcpAncestral, &depNodes);

                // If this is the cache's own layer stack then include the
                // original path itself.
                if (cache->_layerStack == layerStack) {
                    depPaths.push_back(primPath);
                    depNodes.push_back(cache->_GetNodeProvidingSpec(
                        primPath, relocatesLayer, primPath));
                    TF_VERIFY(depNodes.back());
                }

                // Store the node for each dependent site.
                BOOST_FOREACH(const PcpNodeRef& node, depNodes) {
                    _CacheNodeHelper::InsertCacheNodePair(
                        cacheIndex, node, &nodes);
                }
            }
        }
    }
    else {
        // We find dependent sites by looking for used prim specs.
        for (size_t cacheIndex = 0, n = caches.size();
                                        cacheIndex != n; ++cacheIndex) {
            PcpCache* cache = caches[cacheIndex];

            // Get all sites in cache that depend on any spec in primSites.
            BOOST_FOREACH(const SdfSite& primSite, primSites) {
                TF_VERIFY(primSite.path == primPath);
                PcpNodeRefVector depNodes;
                depPaths =
                    cache->_dependencies->Get(primSite.layer, primSite.path,
                                              not recursive, primsOnly,
                                              not spooky, &depNodes);

                // Store the node for each dependent site.
                BOOST_FOREACH(const PcpNodeRef& node, depNodes) {
                    _CacheNodeHelper::InsertCacheNodePair(
                        cacheIndex, node, &nodes);
                }
            }

            // Special case for direct inherits.  An inherit can target a
            // descendant of curPath and we must fix up those inherits.
            // References and payloads can't target a non-root prim so we
            // don't have to worry about those.
            //
            // XXX: We only do this in this cache.  Inherits in this
            //      cache can definitely see the namespace edit so
            //      they must be fixed, but can't inherits outside
            //      this cache also see the namespace edit?
            if (cache == this and curPath.IsPrimPath()) {
                // Get all of the direct dependents on anything at and below
                // the namespace edited site.
                depPaths = GetPathsUsingSite(_layerStack, primPath,
                                             PcpDirect, recursive);
                SdfPathSet allSites(depPaths.begin(), depPaths.end());

                // Get all of the direct dependents on the namespace edited
                // site.
                depPaths = GetPathsUsingSite(_layerStack, primPath,
                                             PcpDirect, not recursive);
                SdfPathSet directSites(depPaths.begin(), depPaths.end());

                // Compute the direct dependents on descendant of the
                // namespace edited site.
                SdfPathSet descendantSites;
                std::set_difference(allSites.begin(),    allSites.end(),
                                    directSites.begin(), directSites.end(),
                                    std::inserter(descendantSites,
                                                  descendantSites.end()));

                // Check each direct dependent site for inherits pointing
                // at this cache's layer stack. Make sure to skip ancestral
                // nodes, since the code that handles direct inherits below
                // needs to have the nodes where the inherits are introduced.
                BOOST_FOREACH(const SdfPath& descendantSite, descendantSites) {
                    BOOST_FOREACH(const PcpNodeRef& node,
                                  _GetPrimIndex(descendantSite)->GetNodeRange(
                                      PcpRangeTypeLocalInherit)) {
                        if (node.GetLayerStack() == _layerStack and
                            not node.IsDueToAncestor()) {
                            // Found an inherit using a descendant.
                            descendantNodes.insert(
                                std::make_pair(cacheIndex, node));
                        }
                    }
                }
            }
        }
    }

    // We now have every node representing the namespace edited site in
    // every graph in every cache in caches that uses that site.  We now
    // need to convert them into something the client can use.  There are
    // two kinds of sites from the client's point of view:
    //
    //   1) Composed sites.  Composed sites are stored in
    //      result.cacheSites.  They represent composed namespace
    //      that is being namespace edited, i.e. the object is
    //      being renamed, reparented, renamed and reparented, or
    //      removed. They're identified by a cache (index) and path.
    //      
    //   2) Uncomposed sites.  Uncomposed sites are stored in
    //      result.layerStackSites.  Each site is a layer stack and a
    //      path (the site path) -- all Sd specs at the path in any
    //      layer in the layer stack must be fixed up the same way.
    //
    // We don't include composed sites here just because they have a
    // reference (or payload or inherit) to a site that's being namespace
    // edited.  The reference itself absorbs the namespace edit, so the
    // object with the reference doesn't need a namespace edit.  For
    // example, if </A> references </B> and we rename </B> to </C> we need
    // to fix the reference on </A> but we don't need to change the name
    // of </A>. So </B> is added to cacheSites but </A> is not.
    //
    // To find composed sites we simply include one for every node that
    // doesn't have any direct (i.e. non-ancestral) reference, inherit, or
    // payload on the traversal of the graph from node to the root.  The
    // old and new paths are found by translating the edited site's old
    // and new paths from the node to the root.
    //
    // We expect the caller (i.e. Csd) to consume cacheSites to fix up
    // connections and targets that point to anything in cacheSites.  It
    // must also fix up relocations involving old paths in cacheSites.
    //
    // Uncomposed sites are found by walking the graph from node to root
    // for each node.  Every node is an uncomposed site.  If we find a
    // a direct (i.e. non-ancestral) reference, inherit, or payload then
    // we store the referencing site and stop the traversal because the
    // reference absorbs the namespace edit.
    //
    // The trick is to do path translation correctly while traversing the
    // graph.  The easy but wrong way is to translate the path from node
    // to root once, then translate the root paths to each node in the
    // traversal.  That doesn't work for the new path if we need to fix
    // any relocation, reference, inherit, or payload along the traversal
    // because the mapping functions will not have the new mapping.  Using
    // the example above </A> references </B> and we rename </B> to </C>.
    // We can't map path </C> across the existing reference because it
    // maps </A> -> </B>.  If we try we'll just get the empty path.  So
    // we translate the path using mapToParent across each arc and we
    // account for relocations as necessary.
    //
    // If we're removing then we must also remove every object using any
    // descendant of the namespace edited prim.  (This only applies to
    // prims since non-prims can't have arcs.)  This cleans up the layers
    // in a way that users expect.  Note, however, that we remove objects
    // even if they're provided via other arcs.  That could be unexpected
    // but it doesn't normally happen.  We don't find these descendants
    // during traversal;  instead we find them separately by getting sites
    // using any prim spec that's a descendant of the namespace edited
    // object.
    //
    // Similar to removing, if we reparent a descendant of a referenced
    // (or inherited or payloaded) prim outside of the arc then it's as
    // if the object was removed as far as the referencing site is
    // concerned.
    //
    // Uncomposed sites have an edit type associated with them, along
    // with an old path and a new path.  The type may be a namespace edit
    // (in which case the site path and the old path are the same);  an
    // edit to the references, inherits, or payload where old path must
    // be replaced with new path;  or a relocation where we must replace
    // old path with new path in every relocation table on every ancestor.
    // These edits must be applied directly to the Sd objects.  If we know
    // there are no opinions to fix at a given uncomposed site we don't
    // have to record the site (but may anyway).

    // XXX: We need to report errors, too.  There are various edits
    //      that can't be represented and we should detect them and
    //      call them out.  Examples are:
    //        1) Reparent a referenced/payloaded prim (e.g. /B ->
    //           /X/B).  We can't target a non-root prim.
    //        2) Rename a prim into namespace already used upstream.
    //           E.g. </A> references </B>, </A/Y> exists and so
    //           does </B/X> -- rename </B/X> to </B/Y>.  Doing so
    //           would cause </A/Y> to pull in </B/Y>, probably
    //           unexpectedly.
    //        3) Rename a prim into namespace that's salted earth
    //           upstream.  E.g. </A> references </B>, </A> relocates
    //           </A/Y> to </A/Z> and </B/X> exists -- rename </B/X>
    //           to </B/Y>.  Doing so would cause </B/Y> to map to
    //           the salted earth </A/Y>.
    // XXX: Should we be doing (layer,path) sites?  If we have
    //      intersecting layer stacks we might do the same spec
    //      twice.  Csd is aware of this so it's okay for now.

    // Walk the graph for each node.
    typedef NamespaceEdits::LayerStackSites LayerStackSites;
    typedef NamespaceEdits::LayerStackSite LayerStackSite;
    typedef NamespaceEdits::CacheSite CacheSite;
    std::set<PcpLayerStackSite> sites;
    BOOST_FOREACH(const CacheNodePair& cacheAndNode, nodes) {
        size_t cacheIndex   = cacheAndNode.first;
        PcpNodeRef node     = cacheAndNode.second;
        SdfPath oldNodePath  = curPath;
        SdfPath newNodePath  = newPath;

        // Handle the node itself.  Note that the node, although representing
        // the namespace edited site, can appear in different layer stacks.
        // This happens when we have two scenes sharing a layer.  If we edit
        // in the shared layer then we must do the corresponding edit in each
        // of the scene's layer stacks.
        if (sites.insert(node.GetSite()).second) {
            LayerStackSites& layerStackSites = 
                _GetLayerStackSitesForEdit(&result, oldNodePath, newNodePath);
            layerStackSites.resize(layerStackSites.size()+1);
            LayerStackSite& site = layerStackSites.back();
            site.cacheIndex      = cacheIndex;
            site.type            = NamespaceEditPath;
            site.sitePath        = oldNodePath;
            site.oldPath         = oldNodePath;
            site.newPath         = newNodePath;
            site.layerStack      = node.GetLayerStack();

            _AddRelocateEditsForLayerStack(
                &result, site.layerStack, cacheIndex,
                oldNodePath, newNodePath);
        }

        // Handle each arc from node to the root.
        while (node.GetParentNode()) {
            if (sites.insert(node.GetParentNode().GetSite()).second) {
                // Add site and translate paths to parent node.
                if (_AddLayerStackSite(&result, node, cacheIndex,
                                       &oldNodePath, &newNodePath)) {
                    // Reached a direct arc so we don't have to continue.
                    break;
                }
            }
            else {
                // Translate paths to parent node.
                _TranslatePathsAndEditRelocates(NULL, node, cacheIndex,
                                                &oldNodePath, &newNodePath);
            }

            // Next node.
            node = node.GetParentNode();
        }

        // If we made it all the way to the root then we have a cacheSite.
        if (not node.GetParentNode()) {
            if (not _IsInvalidEdit(oldNodePath, newNodePath)) {
                result.cacheSites.resize(result.cacheSites.size() + 1);
                CacheSite& cacheSite = result.cacheSites.back();
                cacheSite.cacheIndex = cacheIndex;
                cacheSite.oldPath    = oldNodePath;
                cacheSite.newPath    = newNodePath;
            }
        }
    }

    // Helper functions.
    struct _Helper {
        // Returns true if sites contains site or any ancestor of site.
        static bool HasSite(const std::map<PcpLayerStackSite, size_t>& sites,
                            const PcpLayerStackSite& site)
        {
            std::map<PcpLayerStackSite, size_t>::const_iterator i =
                sites.lower_bound(site);
            if (i != sites.end() and i->first == site) {
                // Site exists in sites.
                return true;
            }
            if (i == sites.begin()) {
                // Site comes before any other site so it's not in sites.
                return false;
            }
            --i;
            if (site.layerStack != i->first.layerStack) {
                // Closest site is in a different layer stack.
                return false;
            }
            if (site.path.HasPrefix(i->first.path)) {
                // Ancestor exists.
                return true;
            }
            // Neither site nor any ancestor of it is in sites.
            return false;
        }
    };

    // If we're removing a prim then also collect every uncomposed site
    // that uses a descendant of the namespace edited site.
    if (newPath.IsEmpty() and curPath.IsPrimPath()) {
        std::map<PcpLayerStackSite, size_t> descendantSites;

        // Make a set of sites we already know have direct arcs to
        // descendants.  We don't want to remove those but we may
        // want to remove their descendants.
        std::set<PcpLayerStackSite> doNotRemoveSites;
        BOOST_FOREACH(const CacheNodePair& cacheAndNode, descendantNodes) {
            const PcpNodeRef& node = cacheAndNode.second;
            doNotRemoveSites.insert(node.GetParentNode().GetSite());
        }

        for (size_t cacheIndex = 0, n = caches.size();
                                        cacheIndex != n; ++cacheIndex) {
            PcpCache* cache = caches[cacheIndex];

            // Get the sites in cache that use any proper descendant of the
            // namespace edited site and what each site depends on.
            std::map<SdfPath, PcpNodeRef> descendantPathsAndNodes;
            PcpNodeRefVector sourceNodes;
            BOOST_FOREACH(const SdfLayerRefPtr& layer, _layerStack->GetLayers()) {
                SdfPathVector paths = 
                    cache->_GetPathsUsingSite(layer, curPath,
                                              UsingSiteAndDescendants,
                                              /* layerOffsets = */ NULL, 
                                              &sourceNodes);
                for (size_t i = 0, n = paths.size(); i != n; ++i) {
                    if (not paths[i].IsPrimPath() or 
                        sourceNodes[i].GetPath() != curPath) {
                        descendantPathsAndNodes[paths[i]] = sourceNodes[i];
                    }
                }
            }

            // Add every uncomposed site used by each (cache,path) pair
            // if we haven't already added its parent.  We don't need to
            // add a site if we've added its parent because removing the
            // parent will remove its descendants.  The result is that we
            // add every uncomposed site that doesn't have a direct arc to
            // another uncomposed site.
            //
            // Note that we only check nodes from the namespace edited
            // site and its descendants to the root.  Other nodes are due
            // to other arcs and not affected by the namespace edit.
            TF_FOR_ALL(i, descendantPathsAndNodes) {
                PcpNodeRef node              = i->second;
                const SdfPath& descendantPath = i->first;
                SdfPath descendantPrimPath    = descendantPath.GetPrimPath();

                for (; node; node = node.GetParentNode()) {
                    SdfPath path =
                        descendantPath.ReplacePrefix(descendantPrimPath,
                                                     node.GetPath());
                    PcpLayerStackSite site(node.GetLayerStack(), path);
                    if (not _Helper::HasSite(descendantSites, site)) {
                        // We haven't seen this site or an ancestor yet.
                        if (doNotRemoveSites.count(site) == 0) {
                            // We're not blocking the addition of this site.
                            descendantSites[site] = cacheIndex;
                        }
                    }
                }
            }
        }

        // We now have all the descendant sites to remove.  Add them to
        // result.layerStackSites.
        TF_FOR_ALL(j, descendantSites) {
            result.layerStackSites.resize(result.layerStackSites.size()+1);
            LayerStackSite& site = result.layerStackSites.back();
            site.cacheIndex      = j->second;
            site.type            = NamespaceEditPath;
            site.sitePath        = j->first.path;
            site.oldPath         = j->first.path;
            site.newPath         = newPath;         // This is the empty path.
            site.layerStack      = j->first.layerStack;
        }
    }

    // Fix up all direct inherits to a descendant site.
    if (not descendantNodes.empty()) {
        BOOST_FOREACH(const CacheNodePair& cacheAndNode, descendantNodes) {
            size_t cacheIndex   = cacheAndNode.first;
            const PcpNodeRef& node = cacheAndNode.second;
            SdfPath oldNodePath  = node.GetPath();
            SdfPath newNodePath  = oldNodePath.ReplacePrefix(curPath, newPath);
            _AddLayerStackSite(&result, node, cacheIndex,
                               &oldNodePath, &newNodePath);
        }
    }

    return result;
}

PcpNodeRef
PcpCache::_GetNodeProvidingSpec(
    const SdfPath& path,
    const SdfLayerHandle& siteLayer,
    const SdfPath& sitePath) const
{
    const PcpPrimIndex* primIndex = _GetPrimIndex(path);
    return (primIndex 
            ? primIndex->GetNodeProvidingSpec(siteLayer, sitePath)
            : PcpNodeRef());
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
        if (primIndex.GetRootNode()) {
            PcpErrorVector errors = primIndex.GetLocalErrors();
            BOOST_FOREACH(const PcpErrorBasePtr & e, errors) {
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
        _dependencies->RemoveAll(lifeboat);
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
        TF_FOR_ALL(i, changes.didChangeSpecs) {
            const SdfPath& path = *i;
            if (path.IsAbsoluteRootOrPrimPath()) {
                // We've possibly changed the prim spec stack.  Since
                // we track dependencies using those specs we must
                // update those specs.  Note that we may have blown
                // the prim index so check that it still exists.
                if (PcpPrimIndex* primIndex = _GetPrimIndex(path)) {
                    // Rebuild the prim stack and get the sites the prim
                    // index depends on.
                    SdfSiteVector depSites;
                    PcpNodeRefVector depNodes;
                    Pcp_UpdatePrimStack(primIndex, &depSites, &depNodes);

                    // If there are no specs left then we can discard the
                    // prim index.
                    if (depSites.empty()) {
                        _RemovePrimAndPropertyCaches(path, lifeboat);
                    }
                    else {
                        // Replace dependencies.
                        static const bool specsAtPathOnly = true;
                        _dependencies->Remove(path, lifeboat, specsAtPathOnly);
                        _dependencies->Add(path, depSites, depNodes);
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
        }

        // Fix the keys for any prim or property under any of the renamed
        // paths.
        // XXX: It'd be nice if this was a usd by just adjusting
        //      paths here and there.
        // First blow all caches under the new names.
        TF_FOR_ALL(i, changes.didChangePath) {
            if (not i->second.IsEmpty()) {
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
                                                       not fixTargetPaths));
                _includedPayloads.erase(j++);
            }
            else {
                ++j;
            }
        }
    }
    _includedPayloads.insert(newIncludes.begin(), newIncludes.end());

    // We can now flush any pending changes in our dependencies.
    _dependencies->Flush();
}

void
PcpCache::Reload(PcpChanges* changes)
{
    TRACE_FUNCTION();

    if (not _layerStack) {
        return;
    }

    ArResolverContextBinder binder(_pathResolverContext);

    // Reload every invalid sublayer and asset we know about,
    // in any layer stack or prim index.
    std::vector<PcpLayerStackPtr> allLayerStacks =
        _layerStackCache->GetAllLayerStacks();
    TF_FOR_ALL(layerStack, allLayerStacks) {
        const PcpErrorVector errors = (*layerStack)->GetLocalErrors();
        BOOST_FOREACH(const PcpErrorBasePtr& e, errors) {
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
        if (primIndex.GetRootNode()) {
            const PcpErrorVector errors = primIndex.GetLocalErrors();
            BOOST_FOREACH(const PcpErrorBasePtr& e, errors) {
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

    static const bool recursive = true;

    ArResolverContextBinder binder(_pathResolverContext);

    // Traverse every PrimIndex at or under primPath to find
    // InvalidAssetPath errors, and collect the unique layer stacks used.
    std::set<PcpLayerStackPtr> layerStacksAtOrUnderPrim;
    BOOST_FOREACH(const _PrimIndexCache::value_type& entry,
                  _primIndexCache.FindSubtreeRange(primPath)) {
        const PcpPrimIndex& primIndex = entry.second;
        if (primIndex.GetRootNode()) {
            PcpErrorVector errors = primIndex.GetLocalErrors();
            BOOST_FOREACH(const PcpErrorBasePtr& e, errors) {
                if (PcpErrorInvalidAssetPathPtr typedErr =
                    dynamic_pointer_cast<PcpErrorInvalidAssetPath>(e))
                {
                    changes->DidMaybeFixAsset(this, typedErr->site,
                                              typedErr->layer,
                                              typedErr->resolvedAssetPath);
                }
            }
            TF_FOR_ALL(it, primIndex.GetNodeRange()) {
                layerStacksAtOrUnderPrim.insert( it->GetSite().layerStack );
            }
        }
    }

    // Check each used layer stack (gathered above) for invalid sublayers.
    TF_FOR_ALL(layerStack, layerStacksAtOrUnderPrim) {
        // Scan errors for a sublayer error.
        PcpErrorVector errs = (*layerStack)->GetLocalErrors();
        TF_FOR_ALL(e, errs) {
            if (PcpErrorInvalidSublayerPathPtr typedErr =
                dynamic_pointer_cast<PcpErrorInvalidSublayerPath>(*e)){
                changes->DidMaybeFixSublayer(this, typedErr->layer,
                                             typedErr->sublayerPath);
            }
        }
    }

    // Reload every layer used by prims at or under primPath, except for
    // local layers.
    SdfLayerHandleSet layersToReload =
        _dependencies->GetLayersUsedByPrim(primPath, recursive);
    
    if (_layerStack) {
        TF_FOR_ALL(it, _layerStack->GetLayers()) {
            layersToReload.erase(*it);
        }
    }

    SdfLayer::ReloadLayers(layersToReload);
}

void
PcpCache::_RemovePrimCache(const SdfPath& primPath, PcpLifeboat* lifeboat)
{
    _PrimIndexCache::iterator it = _primIndexCache.find(primPath);
    if (it != _primIndexCache.end()) {
        static const bool specsAtPathOnly = true;
        _dependencies->Remove(it->first, lifeboat, specsAtPathOnly);

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
        _dependencies->Remove(i->first, lifeboat);
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
        if (primIndex.GetRootNode()) {
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
        if (primIndex.GetRootNode()) {
            return &primIndex;
        }
    }
    return NULL;
}

template <class Predicate>
struct Pcp_ParallelIndexer
{
    typedef Pcp_ParallelIndexer This;

    Pcp_ParallelIndexer(PcpCache *cache,
                        Predicate pred,
                        const PcpLayerStackPtr &layerStack,
                        PcpPrimIndexInputs baseInputs,
                        PcpErrorVector *allErrors,
                        const ArResolverScopedCache* parentCache,
                        const char *mallocTag1,
                        const char *mallocTag2)
        : _cache(cache)
        , _allErrors(allErrors)
        , _predicate(pred)
        , _layerStack(layerStack)
        , _baseInputs(baseInputs)
        , _resolver(ArGetResolver())
        , _consumer(_dispatcher, &This::_ConsumeIndexes, this, /*flush=*/false)
        , _parentCache(parentCache)
        , _mallocTag1(mallocTag1)
        , _mallocTag2(mallocTag2)
    {}

    ~Pcp_ParallelIndexer() {
        // Tear down async.
        WorkSwapDestroyAsync(_toCompute);
        WorkMoveDestroyAsync(_finishedOutputs);
        WorkSwapDestroyAsync(_consumerScratch);

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
        TF_AXIOM(parentIndex or path == SdfPath::AbsoluteRootPath());
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
            index = _cache->FindPrimIndex(path);
        }

        PcpPrimIndexOutputs *outputs = NULL;
        if (not index) {
            // We didn't find an index in the cache, so we must compute one.

            // Make space in the results for the output.
            outputs = &(*_results.grow_by(1));
            
            // Establish inputs.
            PcpPrimIndexInputs inputs = _baseInputs;
            inputs.parentIndex = parentIndex;

            TF_VERIFY(parentIndex or path == SdfPath::AbsoluteRootPath());
        
            // Run indexing.
            PcpComputePrimIndex(
                path, _layerStack, inputs, outputs, &_resolver);

            // Now we have an index in hand.
            index = &outputs->primIndex;
        }

        // Invoke the client's predicate to see if we should do children.
        bool didChildren = false;
        if (_predicate(*index)) {
            // Compute the children paths and add new tasks for them.
            TfTokenVector names;
            PcpTokenSet prohibitedNames;
            index->ComputePrimChildNames(&names, &prohibitedNames);
            BOOST_FOREACH(const TfToken &name, names) {
                // We only check the cache for the children if we got a cache
                // hit for this index.  Pcp tends to invalidate entire subtrees
                // at once.
                didChildren = true;
                _dispatcher.Run(
                    &This::_ComputeIndex, this, index,
                    path.AppendChild(name), /*checkCache=*/not outputs);
            }
        }

        if (outputs) {
            // We're done with this index, arrange for it to be added to the
            // cache and dependencies, then wake the consumer if we didn't have
            // any children to process.  If we did have children to process
            // we'll let them wake the consumer later.
            _finishedOutputs.push(outputs);
            if (not didChildren)
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
            
            // Add dependencies.
            _cache->_AddIndexDependencies(
                outputs->primIndex.GetPath(), outputs);
            
            // Store index off to the side so we can publish several at once,
            // ideally.  We have to make a copy to move into the _cache itself,
            // since sibling caches in other tasks will still require that their
            // parent be valid.
            _consumerScratch.push_back(outputs->primIndex);
        }
        
        // Ok, publish the set of indexes.
        if (not _consumerScratch.empty()) {
            // If we're told to flush, or if we're over a threshold number of
            // pending results, then take the write lock and publish.  Otherwise
            // only attempt to take the write lock, and if we fail to do so then
            // we do nothing, since we're guaranteed to run again.  This helps
            // minimize contention and maximize throughput.

            // This size threshold is arbitrary but helps ensure that even with
            // writer starvation we'll never grow our working space too huge.
            static const size_t PendingSizeThreshold = 20000;

            tbb::spin_rw_mutex::scoped_lock lock;
            if (flush or _consumerScratch.size() >= PendingSizeThreshold) {
                lock.acquire(_primIndexCacheMutex, /*write=*/true);
            } else {
                if (not lock.try_acquire(_primIndexCacheMutex, /*write=*/true))
                    return;
            }
            for (auto &index: _consumerScratch) {
                // Save the prim index in the cache.
                const SdfPath &path = index.GetPath();
                _cache->_primIndexCache[path].Swap(index);
            }
            lock.release();
            _consumerScratch.clear();
        }
    }

    PcpCache *_cache;
    PcpErrorVector *_allErrors;
    Predicate _predicate;
    vector<pair<const PcpPrimIndex *, SdfPath> > _toCompute;
    const PcpLayerStackPtr &_layerStack;
    PcpPrimIndexInputs _baseInputs;
    tbb::concurrent_vector<PcpPrimIndexOutputs> _results;
    tbb::spin_rw_mutex _primIndexCacheMutex;
    tbb::concurrent_queue<PcpPrimIndexOutputs *> _finishedOutputs;
    vector<PcpPrimIndex> _consumerScratch;
    ArResolver& _resolver;
    WorkDispatcher _dispatcher;
    WorkSingularTask _consumer;
    const ArResolverScopedCache* _parentCache;
    char const * const _mallocTag1;
    char const * const _mallocTag2;
};

void
PcpCache::_ComputePrimIndexesInParallel(
    const SdfPathVector &roots,
    PcpErrorVector *allErrors,
    _UntypedPredicate pred,
    const char *mallocTag1,
    const char *mallocTag2)
{
    if (not IsUsd()) {
        TF_CODING_ERROR("Computing prim indexes in parallel only supported "
                        "for USD caches.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    ArResolverScopedCache parentCache;
    TfAutoMallocTag2  tag(mallocTag1, mallocTag2);
    
    typedef Pcp_ParallelIndexer<_UntypedPredicate> Indexer;

    if (not _layerStack)
        ComputeLayerStack(GetLayerStackIdentifier(), allErrors);

    // General strategy: Compute indexes recursively starting from roots, in
    // parallel.  When we've computed an index, ask the predicate if we should
    // continue to compute its children indexes.  If so, we add all the children
    // as new tasks for threads to pick up.
    //
    // Once all the indexes are computed, add them to the cache and add their
    // dependencies to the dependencies structures.

    Indexer indexer(this, pred, _layerStack,
                    GetPrimIndexInputs().USD(_usd), allErrors, &parentCache,
                    mallocTag1, mallocTag2);

    BOOST_FOREACH(const SdfPath &rootPath, roots) {
        // Obtain the parent index, if this is not the absolute root.  Note that
        // the call to ComputePrimIndex below is not concurrency safe.
        const PcpPrimIndex *parentIndex =
            rootPath == SdfPath::AbsoluteRootPath() ? NULL :
            &ComputePrimIndex(rootPath.GetParentPath(), allErrors);
        indexer.ComputeIndex(parentIndex, rootPath);
    }

    // Do the indexing and wait for it to complete.
    indexer.RunAndWait();
}

const PcpPrimIndex &
PcpCache::ComputePrimIndex(const SdfPath & path, PcpErrorVector *allErrors)
{
    // NOTE:TRACE_FUNCTION() is too much overhead here.

    // Check for a cache hit. Default constructed PcpPrimIndex objects
    // may live in the SdfPathTable for paths that haven't yet been computed,
    // so we have to explicitly check for that.
    _PrimIndexCache::const_iterator i = _primIndexCache.find(path);
    if (i != _primIndexCache.end() and i->second.GetRootNode()) {
        return i->second;
    }

    TRACE_FUNCTION();

    if (not _layerStack) {
        ComputeLayerStack(GetLayerStackIdentifier(), allErrors);
    }

    // Run the prim indexing algorithm.
    PcpPrimIndexOutputs outputs;
    PcpComputePrimIndex(path, _layerStack,
                        GetPrimIndexInputs().USD(_usd), &outputs);
    allErrors->insert(
        allErrors->end(),
        outputs.allErrors.begin(),
        outputs.allErrors.end());

    // Save the prim index.
    PcpPrimIndex &cacheEntry = _primIndexCache[path];
    cacheEntry.Swap(outputs.primIndex);

    // Add dependencies.
    _AddIndexDependencies(path, &outputs);

    return cacheEntry;
}

void
PcpCache::_AddIndexDependencies(
    const SdfPath &path, PcpPrimIndexOutputs *outputs)
{
    // Don't add dependencies on our own site.
    outputs->dependencies.sites.erase(
        PcpPrimIndexDependencies::Site(_layerStack, path));

    // Add dependencies.
    Pcp_Dependencies *deps = _dependencies.get();
    deps->Add(path, outputs->dependencies);
    if (not _usd) {
        deps->Add(path, outputs->dependencySites, outputs->dependencyNodes);
        deps->AddSpookySitesUsedByPrim(path, outputs->spookyDependencies);
        deps->AddSpookySitesUsedByPrim(
            path, outputs->spookyDependencySites, 
            outputs->spookyDependencyNodes);
    }
    else {
        // We should not be picking up any spooky dependencies here, since
        // inherits are not supported in the usd codepath.
        TF_VERIFY(outputs->spookyDependencies.sites.empty());
        TF_VERIFY(outputs->spookyDependencySites.empty());
        TF_VERIFY(outputs->spookyDependencyNodes.empty());
    }
}

PcpPropertyIndex*
PcpCache::_GetPropertyIndex(const SdfPath& path)
{
    _PropertyIndexCache::iterator i = _propertyIndexCache.find(path);
    if (i != _propertyIndexCache.end() and not i->second.IsEmpty()) {
        return &i->second;
    }

    return NULL;
}

const PcpPropertyIndex*
PcpCache::_GetPropertyIndex(const SdfPath& path) const
{
    _PropertyIndexCache::const_iterator i = _propertyIndexCache.find(path);
    if (i != _propertyIndexCache.end() and not i->second.IsEmpty()) {
        return &i->second;
    }
    return NULL;
}

const PcpPropertyIndex &
PcpCache::ComputePropertyIndex(const SdfPath & path, PcpErrorVector *allErrors)
{
    TRACE_FUNCTION();

    static PcpPropertyIndex nullIndex;
    if (not path.IsPropertyPath()) {
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

void
PcpCache::PrintDependencies() const
{
    _dependencies->DumpDependencies(std::cout);
}

void
PcpCache::CheckDependencies() const
{
    _dependencies->CheckInvariants();
}
