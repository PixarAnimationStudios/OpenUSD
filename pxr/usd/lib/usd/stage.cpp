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
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/clipCache.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/interpolators.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/resolveInfo.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stageCache.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usd/valueUtils.h"

#include "pxr/usd/pcp/changes.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/layerStackIdentifier.h"
#include "pxr/usd/pcp/site.h"

// used for creating prims
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h" 

#include "pxr/base/trace/trace.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/ar/resolverScopedCache.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/multiInterval.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/pragmas.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/utils.h"

#include <boost/optional.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/utility/in_place_factory.hpp>

#include <tbb/spin_rw_mutex.h>
#include <tbb/spin_mutex.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using boost::make_transform_iterator;
using std::pair;
using std::make_pair;
using std::map;
using std::string;
using std::vector;

// Definition below under "value resolution".
// Composes a prim's typeName, with special consideration for __AnyType__.
static TfToken _ComposeTypeName(const PcpPrimIndex &primIndex);

// ------------------------------------------------------------------------- //
// UsdStage Helpers
// ------------------------------------------------------------------------- //

using _ColorConfigurationFallbacks = pair<SdfAssetPath, TfToken>;

// Fetch the color configuration fallback values from the plugins.
TF_MAKE_STATIC_DATA(_ColorConfigurationFallbacks, _colorConfigurationFallbacks)
{
    PlugPluginPtrVector plugs = PlugRegistry::GetInstance().GetAllPlugins();
    for (const auto& plug : plugs) {
        JsObject metadata = plug->GetMetadata();
        JsValue dictVal;
        if (TfMapLookup(metadata, "UsdColorConfigFallbacks", &dictVal)) {
            if (!dictVal.Is<JsObject>()) {
                TF_CODING_ERROR(
                        "%s[UsdColorConfigFallbacks] was not a dictionary.",
                        plug->GetName().c_str());
                continue;
            }

            JsObject dict = dictVal.Get<JsObject>();
            for (const auto& d : dict) {
                const std::string &key = d.first; 
                if (key == SdfFieldKeys->ColorConfiguration) {
                    if (!d.second.IsString()) {
                        TF_CODING_ERROR("'colorConfiguration' value in "  
                            "%s[UsdColorConfigFallbacks] must be a string.",
                            plug->GetName().c_str());
                        continue;
                    }
                    std::string colorConfig = d.second.GetString();
                    if (!colorConfig.empty()) {
                        _colorConfigurationFallbacks->first = 
                            SdfAssetPath(colorConfig);
                    }
                } else if (key == SdfFieldKeys->ColorManagementSystem) {
                    if (!d.second.IsString()) {
                        TF_CODING_ERROR("'colorManagementSystem' value in "  
                            "%s[UsdColorConfigFallbacks] must be a string.",
                            plug->GetName().c_str());
                        continue;
                    }
                    std::string cms = d.second.GetString();
                    if (!cms.empty()) {
                        _colorConfigurationFallbacks->second = TfToken(cms);
                    }
                } else {
                    TF_CODING_ERROR("Unknown key '%s' found in "
                        "%s[UsdColorConfigFallbacks].", key.c_str(),
                        plug->GetName().c_str());
                }
            }
            // Once we file a plugInfo file with UsdColorConfigFallbacks and 
            // there were no errors in retrieving the fallbacks, skip the 
            // remaining plugins. There should only be one plugin site-wide
            // that defines this.
            continue;
        }
    }
}

//
// Usd lets you configure the fallback variants to use in plugInfo.json.
// This static data goes to discover that on first access.
//
TF_MAKE_STATIC_DATA(PcpVariantFallbackMap, _usdGlobalVariantFallbackMap)
{
    PcpVariantFallbackMap fallbacks;

    PlugPluginPtrVector plugs = PlugRegistry::GetInstance().GetAllPlugins();
    for (const auto& plug : plugs) {
        JsObject metadata = plug->GetMetadata();
        JsValue dictVal;
        if (TfMapLookup(metadata, "UsdVariantFallbacks", &dictVal)) {
            if (!dictVal.Is<JsObject>()) {
                TF_CODING_ERROR(
                        "%s[UsdVariantFallbacks] was not a dictionary.",
                        plug->GetName().c_str());
                continue;
            }
            JsObject dict = dictVal.Get<JsObject>();
            for (const auto& d : dict) {
                std::string vset = d.first;
                if (!d.second.IsArray()) {
                    TF_CODING_ERROR(
                            "%s[UsdVariantFallbacks] value for %s must "
                            "be an arrays.",
                            plug->GetName().c_str(), vset.c_str());
                    continue;
                }
                std::vector<std::string> vsels =
                    d.second.GetArrayOf<std::string>();
                if (!vsels.empty()) {
                    fallbacks[vset] = vsels;
                }
            }
        }
    }

    *_usdGlobalVariantFallbackMap = fallbacks;
}
static tbb::spin_rw_mutex _usdGlobalVariantFallbackMapMutex;

PcpVariantFallbackMap
UsdStage::GetGlobalVariantFallbacks()
{
    tbb::spin_rw_mutex::scoped_lock
        lock(_usdGlobalVariantFallbackMapMutex, /*write=*/false);
    return *_usdGlobalVariantFallbackMap;
}

void
UsdStage::SetGlobalVariantFallbacks(const PcpVariantFallbackMap &fallbacks)
{
    tbb::spin_rw_mutex::scoped_lock
        lock(_usdGlobalVariantFallbackMapMutex, /*write=*/true);
    *_usdGlobalVariantFallbackMap = fallbacks;
}

// Returns the SdfLayerOffset that maps times in \a layer in the local layer
// stack of \a node up to the root of the pcp node tree.  Use
// SdfLayerOffset::GetInverse() to go the other direction.
static SdfLayerOffset
_GetLayerToStageOffset(const PcpNodeRef& pcpNode,
                       const SdfLayerHandle& layer)
{
    // PERFORMANCE: This is cached in the PcpNode and should be cheap.
    // Get the node-local path and layer offset.
    const SdfLayerOffset &nodeToRootNodeOffset =
        pcpNode.GetMapToRoot().GetTimeOffset();

    //
    // Each sublayer may have a layer offset, so we must adjust the
    // time accordingly here.
    //
    // This is done by first translating the current layer's time to
    // the root layer's time (for this LayerStack) followed by a
    // translation from the local PcpNode to the root PcpNode.
    //
    SdfLayerOffset localOffset = nodeToRootNodeOffset;

    if (const SdfLayerOffset *layerToRootLayerOffset =
        pcpNode.GetLayerStack()->GetLayerOffsetForLayer(layer)) {
        localOffset = localOffset * (*layerToRootLayerOffset);
    }

    // NOTE: FPS is intentionally excluded here; in Usd FPS is treated as pure
    // metadata, and does not factor into the layer offset scale. Additionally,
    // it is a validation error to compose mixed frame rates. This was done as a
    // performance optimization.

    return UsdPrepLayerOffset(localOffset);
}

char const *_dormantMallocTagID = "UsdStages in aggregate";

inline
std::string 
_StageTag(const std::string &id)
{
    return "UsdStage: @" + id + "@";
}

// ------------------------------------------------------------------------- //
// UsdStage implementation
// ------------------------------------------------------------------------- //

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdStage::LoadAll, "Load all loadable prims");
    TF_ADD_ENUM_NAME(UsdStage::LoadNone, "Load no loadable prims");
}

static ArResolverContext
_CreatePathResolverContext(
    const SdfLayerHandle& layer)
{
    if (layer && !layer->IsAnonymous()) {
        // Ask for a default context for the layer based on the repository
        // path, or if that's empty (i.e. the asset system is not
        // initialized), use the file path.
        // XXX: This should ultimately not be based on repository path.
        return ArGetResolver().CreateDefaultContextForAsset(
            layer->GetRepositoryPath().empty() ?
                layer->GetRealPath() : layer->GetRepositoryPath());
    }

    return ArGetResolver().CreateDefaultContext();
}

static std::string
_AnchorAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& assetPath)
{
    if (assetPath.empty() ||
        SdfLayer::IsAnonymousLayerIdentifier(assetPath)) {
        return assetPath;
    }

    return SdfComputeAssetPathRelativeToLayer(anchor, assetPath);
}

static std::string
_ResolveAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& assetPath)
{
    const std::string computedAssetPath = 
        _AnchorAssetPathRelativeToLayer(anchor, assetPath);
    if (computedAssetPath.empty()) {
        return computedAssetPath;
    }

    return ArGetResolver().Resolve(computedAssetPath);
}

// If anchorAssetPathsOnly is true, this function will only
// update the authored assetPaths by anchoring them to the
// anchor layer; it will not fill in the resolved path field.
static void
_MakeResolvedAssetPathsImpl(const SdfLayerRefPtr &anchor,
                            const ArResolverContext &context,
                            SdfAssetPath *assetPaths,
                            size_t numAssetPaths,
                            bool anchorAssetPathsOnly)
{
    ArResolverContextBinder binder(context);
    for (size_t i = 0; i != numAssetPaths; ++i) {
        if (anchorAssetPathsOnly) {
            assetPaths[i] = SdfAssetPath(
                _AnchorAssetPathRelativeToLayer(
                    anchor, assetPaths[i].GetAssetPath()));
        }
        else {
            assetPaths[i] = SdfAssetPath(
                assetPaths[i].GetAssetPath(),
                _ResolveAssetPathRelativeToLayer(
                    anchor, assetPaths[i].GetAssetPath()));
        }
    }
}

void
UsdStage::_MakeResolvedAssetPaths(UsdTimeCode time,
                                  const UsdAttribute& attr,
                                  SdfAssetPath *assetPaths,
                                  size_t numAssetPaths,
                                  bool anchorAssetPathsOnly) const
{
    // Get the layer providing the strongest value and use that to anchor the
    // resolve.
    auto anchor = _GetLayerWithStrongestValue(time, attr);
    if (anchor) {
        _MakeResolvedAssetPathsImpl(
            anchor, GetPathResolverContext(), assetPaths, numAssetPaths,
            anchorAssetPathsOnly);
    }
}

void
UsdStage::_MakeResolvedAssetPaths(UsdTimeCode time,
                                  const UsdAttribute& attr,
                                  VtValue* value,
                                  bool anchorAssetPathsOnly) const
{
    if (value->IsHolding<SdfAssetPath>()) {
        SdfAssetPath assetPath;
        value->UncheckedSwap(assetPath);
        _MakeResolvedAssetPaths(
            time, attr, &assetPath, 1, anchorAssetPathsOnly);
        value->UncheckedSwap(assetPath);
            
    }
    else if (value->IsHolding<VtArray<SdfAssetPath>>()) {
        VtArray<SdfAssetPath> assetPaths;
        value->UncheckedSwap(assetPaths);
        _MakeResolvedAssetPaths(
            time, attr, assetPaths.data(), assetPaths.size(), 
            anchorAssetPathsOnly);
        value->UncheckedSwap(assetPaths);
    }
}

static SdfLayerRefPtr
_CreateAnonymousSessionLayer(const SdfLayerHandle &rootLayer)
{
    return SdfLayer::CreateAnonymous(
        TfStringGetBeforeSuffix(
            SdfLayer::GetDisplayNameFromIdentifier(rootLayer->GetIdentifier())) +
        "-session.usda");
}

UsdStage::UsdStage(const SdfLayerRefPtr& rootLayer,
                   const SdfLayerRefPtr& sessionLayer,
                   const ArResolverContext& pathResolverContext,
                   const UsdStagePopulationMask& mask,
                   InitialLoadSet load)
    : _pseudoRoot(0)
    , _rootLayer(rootLayer)
    , _sessionLayer(sessionLayer)
    , _editTarget(_rootLayer)
    , _cache(new PcpCache(PcpLayerStackIdentifier(
                              _rootLayer, _sessionLayer, pathResolverContext),
                          UsdUsdFileFormatTokens->Target,
                          /*usdMode=*/true))
    , _clipCache(new Usd_ClipCache)
    , _instanceCache(new Usd_InstanceCache)
    , _interpolationType(UsdInterpolationTypeLinear)
    , _lastChangeSerialNumber(0)
    , _initialLoadSet(load)
    , _populationMask(mask)
    , _isClosingStage(false)
{
    if (!TF_VERIFY(_rootLayer))
        return;

    TF_DEBUG(USD_STAGE_LIFETIMES).Msg(
        "UsdStage::UsdStage(rootLayer=@%s@, sessionLayer=@%s@)\n",
        _rootLayer->GetIdentifier().c_str(),
        _sessionLayer ? _sessionLayer->GetIdentifier().c_str() : "<null>");

ARCH_PRAGMA_PUSH
ARCH_PRAGMA_DEPRECATED_POSIX_NAME
    _mallocTagID = TfMallocTag::IsInitialized() ?
        strdup(_StageTag(rootLayer->GetIdentifier()).c_str()) :
        _dormantMallocTagID;
ARCH_PRAGMA_POP

    _cache->SetVariantFallbacks(GetGlobalVariantFallbacks());
}

UsdStage::~UsdStage()
{
    TF_DEBUG(USD_STAGE_LIFETIMES).Msg(
        "UsdStage::~UsdStage(rootLayer=@%s@, sessionLayer=@%s@)\n",
        _rootLayer ? _rootLayer->GetIdentifier().c_str() : "<null>",
        _sessionLayer ? _sessionLayer->GetIdentifier().c_str() : "<null>");
    _Close();
    if (_mallocTagID != _dormantMallocTagID){
        free(const_cast<char*>(_mallocTagID));
    }
}

void
UsdStage::_Close()
{
    TfScopedVar<bool> resetIsClosing(_isClosingStage, true);

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    WorkArenaDispatcher wd;

    // Stop listening for notices.
    wd.Run([this]() {
            for (auto &p: _layersAndNoticeKeys)
                TfNotice::Revoke(p.second);
        });

    // Destroy prim structure.
    vector<SdfPath> primsToDestroy;
    if (_pseudoRoot) {
        // Instancing masters are not children of the pseudo-root so
        // we need to explicitly destroy those subtrees.
        primsToDestroy = _instanceCache->GetAllMasters();
        wd.Run([this, &primsToDestroy]() {
                primsToDestroy.push_back(SdfPath::AbsoluteRootPath());
                _DestroyPrimsInParallel(primsToDestroy);
                _pseudoRoot = nullptr;
                WorkMoveDestroyAsync(primsToDestroy);
            });
    }
    
    // Clear members.
    wd.Run([this]() { _cache.reset(); });
    wd.Run([this]() { _clipCache.reset(); });
    wd.Run([this]() { _instanceCache.reset(); });
    wd.Run([this]() { _sessionLayer.Reset(); });
    wd.Run([this]() { _rootLayer.Reset(); });
    _editTarget = UsdEditTarget();

    wd.Wait();

    WorkSwapDestroyAsync(_primMap);
    // XXX: Do not do this async, since python might shut down concurrently with
    // this vector's destruction, and if any of the layers within have been
    // reflected to python, the identity management stuff can blow up (since it
    // accesses python).
    //WorkSwapDestroyAsync(_layersAndNoticeKeys);
}

namespace {

// A predicate we pass to PcpCache::ComputePrimIndexesInParallel() to avoid
// computing indexes for children of inactive prims or instance prims.
// We don't populate such prims in Usd.
struct _NameChildrenPred
{
    explicit _NameChildrenPred(const UsdStagePopulationMask* mask,
                               Usd_InstanceCache* instanceCache)
        : _mask(mask)
        , _instanceCache(instanceCache)
    { }

    bool operator()(const PcpPrimIndex &index, 
                    TfTokenVector* childNamesToCompose) const 
    {
        // Use a resolver to walk the index and find the strongest active
        // opinion.
        Usd_Resolver res(&index);
        for (; res.IsValid(); res.NextLayer()) {
            bool active = true;
            if (res.GetLayer()->HasField(
                    res.GetLocalPath(), SdfFieldKeys->Active, &active)) {
                if (!active) {
                    return false;
                }
                break;
            }
        }

        // UsdStage doesn't expose any prims beneath instances, so we don't need
        // to compute indexes for children of instances unless the index will be
        // used as a source for a master prim.
        if (index.IsInstanceable()) {
            const bool indexUsedAsMasterSource = 
                _instanceCache->RegisterInstancePrimIndex(index);
            if (_mask && indexUsedAsMasterSource) {
                // Add this to the _masterSrcIndexes mask.  We use this to know
                // which master src indexes need to be populated fully, due to
                // instancing.
                tbb::spin_mutex::scoped_lock lock(_masterSrcIndexesMutex);
                _masterSrcIndexes.Add(index.GetPath());
            }
            return indexUsedAsMasterSource;
        }

        // Compose only the child prims that are included in the population
        // mask, if any, unless we're composing an index that a master uses, in
        // which case we do the whole thing.
        if (_mask) {
            SdfPath const &indexPath = index.GetPath();
            bool masterUses = false;
            {
                // Check to see if this path is included by one of the master
                // src indexes we registered for use by a master.  If so, we do
                // the entire subtree.  Maybe someday in the future we'll do
                // something fancier for masks beneath instances.
                masterUses = _instanceCache->MasterUsesPrimIndexPath(indexPath);
                if (!masterUses) {
                    tbb::spin_mutex::scoped_lock lock(_masterSrcIndexesMutex);
                    masterUses = _masterSrcIndexes.IncludesSubtree(indexPath);
                }
            }
            return masterUses ||
                _mask->GetIncludedChildNames(indexPath, childNamesToCompose);
        }

        return true;
    }

private:
    const UsdStagePopulationMask* _mask;
    Usd_InstanceCache* _instanceCache;
    mutable UsdStagePopulationMask _masterSrcIndexes;
    mutable tbb::spin_mutex _masterSrcIndexesMutex;
};

} // anon

/* static */
UsdStageRefPtr
UsdStage::_InstantiateStage(const SdfLayerRefPtr &rootLayer,
                            const SdfLayerRefPtr &sessionLayer,
                            const ArResolverContext &pathResolverContext,
                            const UsdStagePopulationMask &mask,
                            InitialLoadSet load)
{
    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::_InstantiateStage: Creating new UsdStage\n");

    // We don't want to pay for the tag-string construction unless
    // we instrumentation is on, since some Stage ctors (InMemory) can be
    // very lightweight.
    boost::optional<TfAutoMallocTag2> tag;

    if (TfMallocTag::IsInitialized()){
        tag = boost::in_place("Usd", _StageTag(rootLayer->GetIdentifier()));
    }

    // Debug timing info
    boost::optional<TfStopwatch> stopwatch;
    const bool usdInstantiationTimeDebugCodeActive = 
        TfDebug::IsEnabled(USD_STAGE_INSTANTIATION_TIME);

    if (usdInstantiationTimeDebugCodeActive) {
        stopwatch = TfStopwatch();
        stopwatch->Start();
    }

    if (!rootLayer)
        return TfNullPtr;

    UsdStageRefPtr stage = TfCreateRefPtr(
        new UsdStage(rootLayer, sessionLayer, pathResolverContext, mask, load));

    ArResolverScopedCache resolverCache;

    // Populate the stage, request payloads according to InitialLoadSet load.
    stage->_ComposePrimIndexesInParallel(
        SdfPathVector(1, SdfPath::AbsoluteRootPath()),
        load == LoadAll ?
        _IncludeAllDiscoveredPayloads : _IncludeNoDiscoveredPayloads,
        "instantiating stage");
    stage->_pseudoRoot = stage->_InstantiatePrim(SdfPath::AbsoluteRootPath());
    stage->_ComposeSubtreeInParallel(stage->_pseudoRoot);
    stage->_RegisterPerLayerNotices();

    // Publish this stage into all current writable caches.
    for (const auto cache : UsdStageCacheContext::_GetWritableCaches()) {
        cache->Insert(stage);
    }

    // Debug timing info
    if (usdInstantiationTimeDebugCodeActive) {
        stopwatch->Stop();
        TF_DEBUG(USD_STAGE_INSTANTIATION_TIME)
            .Msg("UsdStage::_InstantiateStage: Time elapsed (s): %f\n",
                 stopwatch->GetSeconds());
    }
    
    return stage;
}

// Attempt to create a new layer with \p identifier.  Issue an error in case of
// failure.
static SdfLayerRefPtr
_CreateNewLayer(const std::string &identifier)
{
    TfErrorMark mark;
    SdfLayerRefPtr rootLayer = SdfLayer::CreateNew(identifier);
    if (!rootLayer) {
        // If Sdf did not report an error message, we must.
        if (mark.IsClean()) {
            TF_RUNTIME_ERROR("Failed to CreateNew layer with identifier '%s'",
                             identifier.c_str());
        }
    }
    return rootLayer;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    InitialLoadSet load)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, _CreateAnonymousSessionLayer(layer), load);
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    const SdfLayerHandle& sessionLayer,
                    InitialLoadSet load)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, sessionLayer, load);
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    const ArResolverContext& pathResolverContext,
                    InitialLoadSet load)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, pathResolverContext, load);
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    const SdfLayerHandle& sessionLayer,
                    const ArResolverContext& pathResolverContext,
                    InitialLoadSet load)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, sessionLayer, pathResolverContext, load);
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(InitialLoadSet load)
{
    // Use usda file format if an identifier was not provided.
    //
    // In regards to "tmp.usda" below, SdfLayer::CreateAnonymous always
    // prefixes the identifier with the layer's address in memory, so using the
    // same identifier multiple times still produces unique layers.
    return CreateInMemory("tmp.usda", load);
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(const std::string& identifier,
                         InitialLoadSet load)
{
    return Open(SdfLayer::CreateAnonymous(identifier), load);
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(const std::string& identifier,
                         const ArResolverContext& pathResolverContext,
                         InitialLoadSet load)
{
    // CreateAnonymous() will transform 'identifier', so don't bother
    // using it as a tag
    TfAutoMallocTag tag("Usd");
    
    return Open(SdfLayer::CreateAnonymous(identifier), 
                pathResolverContext, load);
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(const std::string& identifier,
                         const SdfLayerHandle &sessionLayer,
                         InitialLoadSet load)
{
    // CreateAnonymous() will transform 'identifier', so don't bother
    // using it as a tag
    TfAutoMallocTag tag("Usd");
    
    return Open(SdfLayer::CreateAnonymous(identifier), 
                sessionLayer, load);
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(const std::string& identifier,
                         const SdfLayerHandle &sessionLayer,
                         const ArResolverContext& pathResolverContext,
                         InitialLoadSet load)
{
    // CreateAnonymous() will transform 'identifier', so don't bother
    // using it as a tag
    TfAutoMallocTag tag("Usd");
    
    return Open(SdfLayer::CreateAnonymous(identifier),
                sessionLayer, pathResolverContext, load);
}

static
SdfLayerRefPtr
_OpenLayer(
    const std::string &filePath,
    const ArResolverContext &resolverContext = ArResolverContext())
{
    boost::optional<ArResolverContextBinder> binder;
    if (!resolverContext.IsEmpty())
        binder = boost::in_place(resolverContext);

    SdfLayer::FileFormatArguments args;
    args[SdfFileFormatTokens->TargetArg] =
        UsdUsdFileFormatTokens->Target.GetString();

    return SdfLayer::FindOrOpen(filePath, args);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const std::string& filePath, InitialLoadSet load)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(filePath));

    SdfLayerRefPtr rootLayer = _OpenLayer(filePath);
    if (!rootLayer) {
        TF_RUNTIME_ERROR("Failed to open layer @%s@", filePath.c_str());
        return TfNullPtr;
    }
    return Open(rootLayer, load);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const std::string& filePath,
               const ArResolverContext& pathResolverContext,
               InitialLoadSet load)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(filePath));

    SdfLayerRefPtr rootLayer = _OpenLayer(filePath, pathResolverContext);
    if (!rootLayer) {
        TF_RUNTIME_ERROR("Failed to open layer @%s@", filePath.c_str());
        return TfNullPtr;
    }
    return Open(rootLayer, pathResolverContext, load);
}

/* static */
UsdStageRefPtr
UsdStage::OpenMasked(const std::string& filePath,
                     const UsdStagePopulationMask &mask,
                     InitialLoadSet load)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(filePath));

    SdfLayerRefPtr rootLayer = _OpenLayer(filePath);
    if (!rootLayer) {
        TF_RUNTIME_ERROR("Failed to open layer @%s@", filePath.c_str());
        return TfNullPtr;
    }
    return OpenMasked(rootLayer, mask, load);
}

/* static */
UsdStageRefPtr
UsdStage::OpenMasked(const std::string& filePath,
                     const ArResolverContext& pathResolverContext,
                     const UsdStagePopulationMask &mask,
                     InitialLoadSet load)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(filePath));

    SdfLayerRefPtr rootLayer = _OpenLayer(filePath, pathResolverContext);
    if (!rootLayer) {
        TF_RUNTIME_ERROR("Failed to open layer @%s@", filePath.c_str());
        return TfNullPtr;
    }
    return OpenMasked(rootLayer, pathResolverContext, mask, load);
}

class Usd_StageOpenRequest : public UsdStageCacheRequest
{
public:
    Usd_StageOpenRequest(UsdStage::InitialLoadSet load,
                         SdfLayerHandle const &rootLayer)
        : _rootLayer(rootLayer)
        , _initialLoadSet(load) {}
    Usd_StageOpenRequest(UsdStage::InitialLoadSet load,
                         SdfLayerHandle const &rootLayer,
                         SdfLayerHandle const &sessionLayer)
        : _rootLayer(rootLayer)
        , _sessionLayer(sessionLayer)
        , _initialLoadSet(load) {}
    Usd_StageOpenRequest(UsdStage::InitialLoadSet load,
                         SdfLayerHandle const &rootLayer,
                         ArResolverContext const &pathResolverContext)
        : _rootLayer(rootLayer)
        , _pathResolverContext(pathResolverContext)
        , _initialLoadSet(load) {}
    Usd_StageOpenRequest(UsdStage::InitialLoadSet load,
                         SdfLayerHandle const &rootLayer,
                         SdfLayerHandle const &sessionLayer,
                         ArResolverContext const &pathResolverContext)
        : _rootLayer(rootLayer)
        , _sessionLayer(sessionLayer)
        , _pathResolverContext(pathResolverContext)
        , _initialLoadSet(load) {}

    virtual ~Usd_StageOpenRequest() {}
    virtual bool IsSatisfiedBy(UsdStageRefPtr const &stage) const {
        // Works if other stage's root layer matches and we either don't care
        // about the session layer or it matches, and we either don't care about
        // the path resolverContext or it matches.
        return _rootLayer == stage->GetRootLayer() &&
            (!_sessionLayer || (*_sessionLayer == stage->GetSessionLayer())) &&
            (!_pathResolverContext || (*_pathResolverContext ==
                                       stage->GetPathResolverContext()));
    }
    virtual bool IsSatisfiedBy(UsdStageCacheRequest const &other) const {
        auto req = dynamic_cast<Usd_StageOpenRequest const *>(&other);
        if (!req)
            return false;

        // Works if other's root layer matches and we either don't care about
        // the session layer or it matches, and we either don't care about the
        // path resolverContext or it matches.
        return _rootLayer == req->_rootLayer &&
            (!_sessionLayer || (_sessionLayer == req->_sessionLayer)) &&
            (!_pathResolverContext || (_pathResolverContext ==
                                       req->_pathResolverContext));
    }
    virtual UsdStageRefPtr Manufacture() {
        return UsdStage::_InstantiateStage(
            SdfLayerRefPtr(_rootLayer),
            _sessionLayer ? SdfLayerRefPtr(*_sessionLayer) :
            _CreateAnonymousSessionLayer(_rootLayer),
            _pathResolverContext ? *_pathResolverContext :
            _CreatePathResolverContext(_rootLayer),
            UsdStagePopulationMask::All(),
            _initialLoadSet);
    }

private:
    SdfLayerHandle _rootLayer;
    boost::optional<SdfLayerHandle> _sessionLayer;
    boost::optional<ArResolverContext> _pathResolverContext;
    UsdStage::InitialLoadSet _initialLoadSet;
};

/* static */
template <class... Args>
UsdStageRefPtr
UsdStage::_OpenImpl(InitialLoadSet load, Args const &... args)
{
    // Try to find a matching stage in read-only caches.
    for (const UsdStageCache *cache:
             UsdStageCacheContext::_GetReadableCaches()) {
        if (UsdStageRefPtr stage = cache->FindOneMatching(args...))
            return stage;
    }

    // If none found, request the stage in all the writable caches.  If we
    // manufacture a stage, we'll publish it to all the writable caches, so
    // subsequent requests will get the same stage out.
    UsdStageRefPtr stage;
    auto writableCaches = UsdStageCacheContext::_GetWritableCaches();
    if (writableCaches.empty()) {
        stage = Usd_StageOpenRequest(load, args...).Manufacture();
    }
    else {
        for (UsdStageCache *cache: writableCaches) {
            auto r = cache->RequestStage(Usd_StageOpenRequest(load, args...));
            if (!stage)
                stage = r.first;
            if (r.second) {
                // We manufactured the stage -- we published it to all the other
                // caches too, so nothing left to do.
                break;
            }
        }
    }
    TF_VERIFY(stage);
    return stage;
}

/* static */
UsdStageRefPtr
UsdStage::Open(const SdfLayerHandle& rootLayer, InitialLoadSet load)
{
    if (!rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::Open(rootLayer=@%s@, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             TfStringify(load).c_str());

    return _OpenImpl(load, rootLayer);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const SdfLayerHandle& rootLayer,
               const SdfLayerHandle& sessionLayer,
               InitialLoadSet load)
{
    if (!rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::Open(rootLayer=@%s@, sessionLayer=@%s@, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             sessionLayer ? sessionLayer->GetIdentifier().c_str() : "<null>",
             TfStringify(load).c_str());

    return _OpenImpl(load, rootLayer, sessionLayer);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const SdfLayerHandle& rootLayer,
               const ArResolverContext& pathResolverContext,
               InitialLoadSet load)
{
    if (!rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::Open(rootLayer=@%s@, pathResolverContext=%s, "
                            "load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             pathResolverContext.GetDebugString().c_str(), 
             TfStringify(load).c_str());

    return _OpenImpl(load, rootLayer, pathResolverContext);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const SdfLayerHandle& rootLayer,
               const SdfLayerHandle& sessionLayer,
               const ArResolverContext& pathResolverContext,
               InitialLoadSet load)
{
    if (!rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::Open(rootLayer=@%s@, sessionLayer=@%s@, "
                             "pathResolverContext=%s, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             sessionLayer ? sessionLayer->GetIdentifier().c_str() : "<null>",
             pathResolverContext.GetDebugString().c_str(),
             TfStringify(load).c_str());

    return _OpenImpl(load, rootLayer, sessionLayer, pathResolverContext);
}

////////////////////////////////////////////////////////////////////////
// masked opens.

/* static */
UsdStageRefPtr
UsdStage::OpenMasked(const SdfLayerHandle& rootLayer,
                     const UsdStagePopulationMask &mask,
                     InitialLoadSet load)
{
    if (!rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::OpenMasked(rootLayer=@%s@, mask=%s, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             TfStringify(mask).c_str(),
             TfStringify(load).c_str());

    return _InstantiateStage(SdfLayerRefPtr(rootLayer),
                             _CreateAnonymousSessionLayer(rootLayer),
                             _CreatePathResolverContext(rootLayer),
                             mask,
                             load);
}

/* static */
UsdStageRefPtr
UsdStage::OpenMasked(const SdfLayerHandle& rootLayer,
                     const SdfLayerHandle& sessionLayer,
                     const UsdStagePopulationMask &mask,
                     InitialLoadSet load)
{
    if (!rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::OpenMasked(rootLayer=@%s@, sessionLayer=@%s@, "
             "mask=%s, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             sessionLayer ? sessionLayer->GetIdentifier().c_str() : "<null>",
             TfStringify(mask).c_str(),
             TfStringify(load).c_str());

    return _InstantiateStage(SdfLayerRefPtr(rootLayer),
                             SdfLayerRefPtr(sessionLayer),
                             _CreatePathResolverContext(rootLayer),
                             mask,
                             load);
}

/* static */
UsdStageRefPtr
UsdStage::OpenMasked(const SdfLayerHandle& rootLayer,
                     const ArResolverContext& pathResolverContext,
                     const UsdStagePopulationMask &mask,
                     InitialLoadSet load)
{
    if (!rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::OpenMasked(rootLayer=@%s@, pathResolverContext=%s, "
             "mask=%s, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             pathResolverContext.GetDebugString().c_str(),
             TfStringify(mask).c_str(),
             TfStringify(load).c_str());

    return _InstantiateStage(SdfLayerRefPtr(rootLayer),
                             _CreateAnonymousSessionLayer(rootLayer),
                             pathResolverContext,
                             mask,
                             load);
}

/* static */
UsdStageRefPtr
UsdStage::OpenMasked(const SdfLayerHandle& rootLayer,
                     const SdfLayerHandle& sessionLayer,
                     const ArResolverContext& pathResolverContext,
                     const UsdStagePopulationMask &mask,
                     InitialLoadSet load)
{
    if (!rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::OpenMasked(rootLayer=@%s@, sessionLayer=@%s@, "
             "pathResolverContext=%s, mask=%s, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             sessionLayer ? sessionLayer->GetIdentifier().c_str() : "<null>",
             pathResolverContext.GetDebugString().c_str(),
             TfStringify(mask).c_str(),
             TfStringify(load).c_str());

    return _InstantiateStage(SdfLayerRefPtr(rootLayer),
                             SdfLayerRefPtr(sessionLayer),
                             pathResolverContext,
                             mask,
                             load);
}

SdfPropertySpecHandle
UsdStage::_GetPropertyDefinition(const UsdPrim &prim,
                                 const TfToken &propName) const
{
    if (!prim)
        return TfNullPtr;

    const TfToken &typeName = prim.GetTypeName();
    if (typeName.IsEmpty())
        return TfNullPtr;

    // Consult the registry.
    return UsdSchemaRegistry::GetPropertyDefinition(typeName, propName);
}

SdfPropertySpecHandle
UsdStage::_GetPropertyDefinition(const UsdProperty &prop) const
{
    return _GetPropertyDefinition(prop.GetPrim(), prop.GetName());
}

template <class PropType>
SdfHandle<PropType>
UsdStage::_GetPropertyDefinition(const UsdProperty &prop) const
{
    return TfDynamic_cast<SdfHandle<PropType> >(_GetPropertyDefinition(prop));
}

SdfAttributeSpecHandle
UsdStage::_GetAttributeDefinition(const UsdAttribute &attr) const
{
    return _GetPropertyDefinition<SdfAttributeSpec>(attr);
}

SdfRelationshipSpecHandle
UsdStage::_GetRelationshipDefinition(const UsdRelationship &rel) const
{
    return _GetPropertyDefinition<SdfRelationshipSpec>(rel);
}

bool
UsdStage::_ValidateEditPrim(const UsdPrim &prim, const char* operation) const
{
    if (ARCH_UNLIKELY(prim.IsInMaster())) {
        TF_CODING_ERROR("Cannot %s at path <%s>; "
                        "authoring to an instancing master is not allowed.",
                        operation, prim.GetPath().GetText());
        return false;
    }

    if (ARCH_UNLIKELY(prim.IsInstanceProxy())) {
        TF_CODING_ERROR("Cannot %s at path <%s>; "
                        "authoring to an instance proxy is not allowed.",
                        operation, prim.GetPath().GetText());
        return false;
    }

    return true;
}

bool
UsdStage::_ValidateEditPrimAtPath(const SdfPath &primPath, 
                                  const char* operation) const
{
    if (ARCH_UNLIKELY(Usd_InstanceCache::IsPathInMaster(primPath))) {
        TF_CODING_ERROR("Cannot %s at path <%s>; "
                        "authoring to an instancing master is not allowed.",
                        operation, primPath.GetText());
        return false;
    }

    if (ARCH_UNLIKELY(_IsObjectDescendantOfInstance(primPath))) {
        TF_CODING_ERROR("Cannot %s at path <%s>; "
                        "authoring to an instance proxy is not allowed.",
                        operation, primPath.GetText());
        return false;
    }

    return true;
}

namespace {

SdfPrimSpecHandle
_CreatePrimSpecAtEditTarget(const UsdEditTarget &editTarget, 
                            const SdfPath &path)
{
    const SdfPath &targetPath = editTarget.MapToSpecPath(path);
    return targetPath.IsEmpty() ? SdfPrimSpecHandle() :
        SdfCreatePrimInLayer(editTarget.GetLayer(), targetPath);
}

}

SdfPrimSpecHandle
UsdStage::_CreatePrimSpecForEditing(const UsdPrim& prim)
{
    if (ARCH_UNLIKELY(!_ValidateEditPrim(prim, "create prim spec"))) {
        return TfNullPtr;
    }

    return _CreatePrimSpecAtEditTarget(GetEditTarget(), prim.GetPath());
}

static SdfAttributeSpecHandle
_StampNewPropertySpec(const SdfPrimSpecHandle &primSpec,
                      const SdfAttributeSpecHandle &toCopy)
{
    return SdfAttributeSpec::New(
        primSpec, toCopy->GetNameToken(), toCopy->GetTypeName(),
        toCopy->GetVariability(), toCopy->IsCustom());
}

static SdfRelationshipSpecHandle
_StampNewPropertySpec(const SdfPrimSpecHandle &primSpec,
                      const SdfRelationshipSpecHandle &toCopy)
{
    return SdfRelationshipSpec::New(
        primSpec, toCopy->GetNameToken(), toCopy->IsCustom(),
        toCopy->GetVariability());
}

static SdfPropertySpecHandle
_StampNewPropertySpec(const SdfPrimSpecHandle &primSpec,
                      const SdfPropertySpecHandle &toCopy)
{
    // Type dispatch to correct property type.
    if (SdfAttributeSpecHandle attrSpec =
        TfDynamic_cast<SdfAttributeSpecHandle>(toCopy)) {
        return _StampNewPropertySpec(primSpec, attrSpec);
    } else {
        return _StampNewPropertySpec(
            primSpec, TfStatic_cast<SdfRelationshipSpecHandle>(toCopy));
    }
}

template <class PropType>
SdfHandle<PropType>
UsdStage::_CreatePropertySpecForEditing(const UsdProperty &prop)
{
    UsdPrim prim = prop.GetPrim();
    if (ARCH_UNLIKELY(!_ValidateEditPrim(prim, "create property spec"))) {
        return TfNullPtr;
    }

    typedef SdfHandle<PropType> TypedSpecHandle;

    const UsdEditTarget &editTarget = GetEditTarget();

    const SdfPath &propPath = prop.GetPath();
    const TfToken &propName = prop.GetName();

    // Check to see if there already exists a property with this path at the
    // current EditTarget.
    if (SdfPropertySpecHandle propSpec =
        editTarget.GetPropertySpecForScenePath(propPath)) {
        // If it's of the correct type, we're done.  Otherwise this is an error:
        // attribute/relationship type mismatch.
        if (TypedSpecHandle spec = TfDynamic_cast<TypedSpecHandle>(propSpec))
            return spec;

        TF_RUNTIME_ERROR("Spec type mismatch.  Failed to create %s for <%s> at "
                         "<%s> in @%s@.  %s already at that location.",
                         ArchGetDemangled<PropType>().c_str(),
                         propPath.GetText(),
                         editTarget.MapToSpecPath(propPath).GetText(),
                         editTarget.GetLayer()->GetIdentifier().c_str(),
                         TfStringify(propSpec->GetSpecType()).c_str());
        return TfNullPtr;
    }

    // There is no property spec at the current EditTarget.  Look for a typed
    // spec whose metadata we can copy.  First check to see if there is a
    // builtin we can use.  Failing that, try to take the strongest authored
    // spec.
    TypedSpecHandle specToCopy;

    // Get definition, if any.
    specToCopy = _GetPropertyDefinition<PropType>(prop);

    if (!specToCopy) {
        // There is no definition available, either because the prim has no
        // known schema, or its schema has no definition for this property.  In
        // this case, we look to see if there's a strongest property spec.  If
        // so, we copy its required metadata.
        for (Usd_Resolver r(&prim.GetPrimIndex()); r.IsValid(); r.NextLayer()) {
            if (SdfPropertySpecHandle propSpec = r.GetLayer()->
                GetPropertyAtPath(r.GetLocalPath().AppendProperty(propName))) {
                if ((specToCopy = TfDynamic_cast<TypedSpecHandle>(propSpec)))
                    break;
                // Type mismatch.
                TF_RUNTIME_ERROR("Spec type mismatch.  Failed to create %s for "
                                 "<%s> at <%s> in @%s@.  Strongest existing "
                                 "spec, %s at <%s> in @%s@",
                                 ArchGetDemangled<PropType>().c_str(),
                                 propPath.GetText(),
                                 editTarget.MapToSpecPath(propPath).GetText(),
                                 editTarget.GetLayer()->GetIdentifier().c_str(),
                                 TfStringify(propSpec->GetSpecType()).c_str(),
                                 propSpec->GetPath().GetText(),
                                 propSpec->GetLayer()->GetIdentifier().c_str());
                return TfNullPtr;
            }
        }
    }

    // If we have a spec to copy from, then we author an opinion at the edit
    // target.
    if (specToCopy) {
        SdfChangeBlock block;
        SdfPrimSpecHandle primSpec = _CreatePrimSpecForEditing(prim);
        if (TF_VERIFY(primSpec))
            return _StampNewPropertySpec(primSpec, specToCopy);
    }

    // Otherwise, we fail to create a spec.
    return TfNullPtr;
}

SdfAttributeSpecHandle
UsdStage::_CreateAttributeSpecForEditing(const UsdAttribute &attr)
{
    return _CreatePropertySpecForEditing<SdfAttributeSpec>(attr);
}

SdfRelationshipSpecHandle
UsdStage::_CreateRelationshipSpecForEditing(const UsdRelationship &rel)
{
    return _CreatePropertySpecForEditing<SdfRelationshipSpec>(rel);
}

SdfPropertySpecHandle
UsdStage::_CreatePropertySpecForEditing(const UsdProperty &prop)
{
    return _CreatePropertySpecForEditing<SdfPropertySpec>(prop);
}

bool
UsdStage::_SetMetadata(
    const UsdObject &obj, const TfToken& fieldName,
    const TfToken &keyPath, const SdfAbstractDataConstValue &newValue)
{
    return _SetMetadataImpl(obj, fieldName, keyPath, newValue);
}

bool
UsdStage::_SetMetadata(
    const UsdObject &obj, const TfToken& fieldName,
    const TfToken &keyPath, const VtValue &newValue)
{
    return _SetMetadataImpl(obj, fieldName, keyPath, newValue);
}

static const std::type_info &
_GetTypeInfo(const SdfAbstractDataConstValue &value)
{
    return value.valueType;
}

static const std::type_info &
_GetTypeInfo(const VtValue &value)
{
    return value.IsEmpty() ? typeid(void) : value.GetTypeid();
}

template <class T>
bool
UsdStage::_SetMetadataImpl(const UsdObject &obj,
                           const TfToken &fieldName,
                           const TfToken &keyPath,
                           const T &newValue)
{
    TfAutoMallocTag2 tag("Usd", _mallocTagID);

    SdfSpecHandle spec;

    if (obj.Is<UsdProperty>()) {
        spec = _CreatePropertySpecForEditing(obj.As<UsdProperty>());
    } else if (obj.Is<UsdPrim>()) {
        spec = _CreatePrimSpecForEditing(obj.As<UsdPrim>());
    } else {
        TF_CODING_ERROR("Cannot set metadata at path <%s> in layer @%s@; "
                        "a prim or property is required",
                        GetEditTarget().MapToSpecPath(obj.GetPath()).GetText(),
                        GetEditTarget().GetLayer()->GetIdentifier().c_str());
        return false;
    }

    if (!spec) {
        TF_CODING_ERROR("Cannot set metadata. Failed to create spec <%s> in "
                        "layer @%s@",
                        GetEditTarget().MapToSpecPath(obj.GetPath()).GetText(),
                        GetEditTarget().GetLayer()->GetIdentifier().c_str());
        return false;
    }

    const auto& schema = spec->GetSchema();
    const auto specType = spec->GetSpecType();
    if (!schema.IsValidFieldForSpec(fieldName, specType)) {
        TF_CODING_ERROR("Cannot set metadata. '%s' is not registered "
                        "as valid metadata for spec type %s.",
                        fieldName.GetText(),
                        TfStringify(specType).c_str());
        return false;
    }

    if (keyPath.IsEmpty()) {
        spec->GetLayer()->SetField(spec->GetPath(), fieldName, newValue);
    } else {
        spec->GetLayer()->SetFieldDictValueByKey(
            spec->GetPath(), fieldName, keyPath, newValue);
    }
    return true;
}

bool
UsdStage::_SetValue(UsdTimeCode time, const UsdAttribute &attr,
                    const SdfAbstractDataConstValue &newValue)
{
    return _SetValueImpl(time, attr, newValue);
}

bool
UsdStage::_SetValue(
    UsdTimeCode time, const UsdAttribute &attr, const VtValue &newValue)
{
    return _SetValueImpl(time, attr, newValue);
}

template <class T>
bool
UsdStage::_SetValueImpl(
    UsdTimeCode time, const UsdAttribute &attr, const T& newValue)
{
    // if we are setting a value block, we don't want type checking
    if (!Usd_ValueContainsBlock(&newValue)) {
        // Do a type check.  Obtain typeName.
        TfToken typeName;
        SdfAbstractDataTypedValue<TfToken> abstrToken(&typeName);
        _GetMetadata(attr, SdfFieldKeys->TypeName,
                     TfToken(), /*useFallbacks=*/true, &abstrToken);
        if (typeName.IsEmpty()) {
                TF_RUNTIME_ERROR("Empty typeName for <%s>", 
                                 attr.GetPath().GetText());
            return false;
        }
        // Ensure this typeName is known to our schema.
        TfType valType = SdfSchema::GetInstance().FindType(typeName).GetType();
        if (valType.IsUnknown()) {
            TF_RUNTIME_ERROR("Unknown typename for <%s>: '%s'",
                             typeName.GetText(), attr.GetPath().GetText());
            return false;
        }
        // Check that the passed value is the expected type.
        if (!TfSafeTypeCompare(_GetTypeInfo(newValue), valType.GetTypeid())) {
            TF_CODING_ERROR("Type mismatch for <%s>: expected '%s', got '%s'",
                            attr.GetPath().GetText(),
                            ArchGetDemangled(valType.GetTypeid()).c_str(),
                            ArchGetDemangled(_GetTypeInfo(newValue)).c_str());
            return false;
        }

        // Check variability, but only if the appropriate debug flag is
        // enabled. Variability is a statement of intent but doesn't control
        // behavior, so we only want to perform this validation when it is
        // requested.
        if (TfDebug::IsEnabled(USD_VALIDATE_VARIABILITY) && 
            time != UsdTimeCode::Default() && 
            _GetVariability(attr) == SdfVariabilityUniform) {
            TF_DEBUG(USD_VALIDATE_VARIABILITY)
                .Msg("Warning: authoring time sample value on "
                     "uniform attribute <%s> at time %.3f\n", 
                     UsdDescribe(attr).c_str(), time.GetValue());
        }
    }

    SdfAttributeSpecHandle attrSpec = _CreateAttributeSpecForEditing(attr);

    if (!attrSpec) {
        TF_RUNTIME_ERROR(
            "Cannot set attribute value.  Failed to create "
            "attribute spec <%s> in layer @%s@",
            GetEditTarget().MapToSpecPath(attr.GetPath()).GetText(),
            GetEditTarget().GetLayer()->GetIdentifier().c_str());
        return false;
    }

    if (time.IsDefault()) {
        attrSpec->GetLayer()->SetField(attrSpec->GetPath(),
                                       SdfFieldKeys->Default,
                                       newValue);
    } else {
        // XXX: should this loft the underlying values up when
        // authoring over a weaker layer?

        // XXX: this won't be correct if we are trying to edit
        // across two different reference arcs -- which may have
        // different time offsets.  perhaps we need the map function
        // to track a time offset for each path?
        const SdfLayerOffset stageToLayerOffset = 
            UsdPrepLayerOffset(GetEditTarget().GetMapFunction().GetTimeOffset())
            .GetInverse();

        double localTime = stageToLayerOffset * time.GetValue();

        attrSpec->GetLayer()->SetTimeSample(
            attrSpec->GetPath(),
            localTime,
            newValue);
    }

    return true;
}

bool
UsdStage::_ClearValue(UsdTimeCode time, const UsdAttribute &attr)
{
    if (ARCH_UNLIKELY(!_ValidateEditPrim(attr.GetPrim(), "clear attribute value"))) {
        return false;
    }

    if (time.IsDefault())
        return _ClearMetadata(attr, SdfFieldKeys->Default);

    const UsdEditTarget &editTarget = GetEditTarget();
    if (!editTarget.IsValid()) {
        TF_CODING_ERROR("EditTarget does not contain a valid layer.");
        return false;
    }

    const SdfLayerHandle &layer = editTarget.GetLayer();
    SdfPath localPath = editTarget.MapToSpecPath(attr.GetPrimPath());
    const TfToken &attrName = attr.GetName();
    if (!layer->HasSpec(SdfAbstractDataSpecId(&localPath, &attrName))) {
        return true;
    }

    SdfAttributeSpecHandle attrSpec = _CreateAttributeSpecForEditing(attr);

    if (!TF_VERIFY(attrSpec, 
                      "Failed to get attribute spec <%s> in layer @%s@",
                      editTarget.MapToSpecPath(attr.GetPath()).GetText(),
                      editTarget.GetLayer()->GetIdentifier().c_str())) {
        return false;
    }

    const SdfLayerOffset stageToLayerOffset = 
        UsdPrepLayerOffset(editTarget.GetMapFunction().GetTimeOffset())
        .GetInverse();

    const double layerTime = stageToLayerOffset * time.GetValue();

    attrSpec->GetLayer()->EraseTimeSample(attrSpec->GetPath(), layerTime);

    return true;
}

bool
UsdStage::_ClearMetadata(const UsdObject &obj, const TfToken& fieldName,
    const TfToken &keyPath)
{
    if (ARCH_UNLIKELY(!_ValidateEditPrim(obj.GetPrim(), "clear metadata"))) {
        return false;
    }

    const UsdEditTarget &editTarget = GetEditTarget();
    if (!editTarget.IsValid()) {
        TF_CODING_ERROR("EditTarget does not contain a valid layer.");
        return false;
    }

    const SdfLayerHandle &layer = editTarget.GetLayer();
    SdfPath localPath = editTarget.MapToSpecPath(obj.GetPrimPath());
    static TfToken empty;
    const TfToken &propName = obj.Is<UsdProperty>() ? obj.GetName() : empty;
    if (!layer->HasSpec(SdfAbstractDataSpecId(&localPath, &propName))) {
        return true;
    }

    SdfSpecHandle spec;
    if (obj.Is<UsdProperty>())
        spec = _CreatePropertySpecForEditing(obj.As<UsdProperty>());
    else
        spec = _CreatePrimSpecForEditing(obj.As<UsdPrim>());

    if (!TF_VERIFY(spec, 
                      "No spec at <%s> in layer @%s@",
                      editTarget.MapToSpecPath(obj.GetPath()).GetText(),
                      editTarget.GetLayer()->GetIdentifier().c_str())) {
        return false;
    }

    const auto& schema = spec->GetSchema();
    const auto specType = spec->GetSpecType();
    if (!schema.IsValidFieldForSpec(fieldName, specType)) {
        TF_CODING_ERROR("Cannot clear metadata. '%s' is not registered "
                        "as valid metadata for spec type %s.",
                        fieldName.GetText(),
                        TfStringify(specType).c_str());
        return false;
    }

    if (keyPath.IsEmpty()) {
        spec->GetLayer()->EraseField(spec->GetPath(), fieldName);
    } else {
        spec->GetLayer()->EraseFieldDictValueByKey(
            spec->GetPath(), fieldName, keyPath);
    }
    return true;
}

static
bool
_IsPrivateFieldKey(const TfToken& fieldKey)
{
    static TfHashSet<TfToken, TfToken::HashFunctor> ignoredKeys;

    // XXX -- Use this instead of an initializer list in case TfHashSet
    //        doesn't support initializer lists.  Should ensure that
    //        TfHashSet does support them.
    static std::once_flag once;
    std::call_once(once, [](){
        // Composition keys.
        ignoredKeys.insert(SdfFieldKeys->InheritPaths);
        ignoredKeys.insert(SdfFieldKeys->Payload);
        ignoredKeys.insert(SdfFieldKeys->References);
        ignoredKeys.insert(SdfFieldKeys->Specializes);
        ignoredKeys.insert(SdfFieldKeys->SubLayers);
        ignoredKeys.insert(SdfFieldKeys->SubLayerOffsets);
        ignoredKeys.insert(SdfFieldKeys->VariantSelection);
        ignoredKeys.insert(SdfFieldKeys->VariantSetNames);
        // Clip keys.
        {
            auto clipFields = UsdGetClipRelatedFields();
            ignoredKeys.insert(clipFields.begin(), clipFields.end());
        }
        // Value keys.
        ignoredKeys.insert(SdfFieldKeys->Default);
        ignoredKeys.insert(SdfFieldKeys->TimeSamples);
    });

    // First look-up the field in the black-list table.
    if (ignoredKeys.find(fieldKey) != ignoredKeys.end())
        return true;

    // Implicitly excluded fields (child containers & readonly metadata).
    SdfSchema const & schema = SdfSchema::GetInstance();
    SdfSchema::FieldDefinition const* field =
                                schema.GetFieldDefinition(fieldKey);
    if (field && (field->IsReadOnly() || field->HoldsChildren()))
        return true;

    // The field is not private.
    return false;
}

UsdPrim
UsdStage::GetPseudoRoot() const
{
    return UsdPrim(_pseudoRoot, SdfPath());
}

UsdPrim
UsdStage::GetDefaultPrim() const
{
    TfToken name = GetRootLayer()->GetDefaultPrim();
    return SdfPath::IsValidIdentifier(name)
        ? GetPrimAtPath(SdfPath::AbsoluteRootPath().AppendChild(name))
        : UsdPrim();
}

void
UsdStage::SetDefaultPrim(const UsdPrim &prim)
{
    GetRootLayer()->SetDefaultPrim(prim.GetName());
}

void
UsdStage::ClearDefaultPrim()
{
    GetRootLayer()->ClearDefaultPrim();
}

bool
UsdStage::HasDefaultPrim() const
{
    return GetRootLayer()->HasDefaultPrim();
}

UsdPrim
UsdStage::GetPrimAtPath(const SdfPath &path) const
{
    // Silently return an invalid UsdPrim if the given path is not an
    // absolute path to maintain existing behavior.
    if (!path.IsAbsolutePath()) {
        return UsdPrim();
    }

    // If this path points to a prim beneath an instance, return
    // an instance proxy that uses the prim data from the corresponding
    // prim in the master but appears to be a prim at the given path.
    Usd_PrimDataConstPtr primData = _GetPrimDataAtPathOrInMaster(path);
    const SdfPath& proxyPrimPath = 
        primData && primData->GetPath() != path ? path : SdfPath::EmptyPath();
    return UsdPrim(primData, proxyPrimPath);
}

UsdObject
UsdStage::GetObjectAtPath(const SdfPath &path) const
{
    // Maintain consistent behavior with GetPrimAtPath
    if (!path.IsAbsolutePath()) {
        return UsdObject();
    }

    const bool isPrimPath = path.IsPrimPath();
    const bool isPropPath = !isPrimPath && path.IsPropertyPath();
    if (!isPrimPath && !isPropPath) {
        return UsdObject();
    }

    // A valid prim must be found to return either a prim or prop
    if (isPrimPath) {
        return GetPrimAtPath(path);
    } else if (isPropPath) {
        if (auto prim = GetPrimAtPath(path.GetPrimPath())) {
            return prim.GetProperty(path.GetNameToken());
        }
    }

    return UsdObject();
}

Usd_PrimDataConstPtr
UsdStage::_GetPrimDataAtPath(const SdfPath &path) const
{
    tbb::spin_rw_mutex::scoped_lock lock;
    if (_primMapMutex)
        lock.acquire(*_primMapMutex, /*write=*/false);
    PathToNodeMap::const_iterator entry = _primMap.find(path);
    return entry != _primMap.end() ? entry->second.get() : NULL;
}

Usd_PrimDataPtr
UsdStage::_GetPrimDataAtPath(const SdfPath &path)
{
    tbb::spin_rw_mutex::scoped_lock lock;
    if (_primMapMutex)
        lock.acquire(*_primMapMutex, /*write=*/false);
    PathToNodeMap::const_iterator entry = _primMap.find(path);
    return entry != _primMap.end() ? entry->second.get() : NULL;
}

Usd_PrimDataConstPtr 
UsdStage::_GetPrimDataAtPathOrInMaster(const SdfPath &path) const
{
    Usd_PrimDataConstPtr primData = _GetPrimDataAtPath(path);

    // If no prim data exists at the given path, check if this
    // path is pointing to a prim beneath an instance. If so, we
    // need to return the prim data for the corresponding prim
    // in the master.
    if (!primData) {
        const SdfPath primInMasterPath = 
            _instanceCache->GetPathInMasterForInstancePath(path);
        if (!primInMasterPath.IsEmpty()) {
            primData = _GetPrimDataAtPath(primInMasterPath);
        }
    }

    return primData;
}

bool
UsdStage::_IsValidForUnload(const SdfPath& path) const
{
    if (!path.IsAbsolutePath()) {
        TF_CODING_ERROR("Attempted to load/unload a relative path <%s>",
                        path.GetText());
        return false;
    }

    return true;
}

bool
UsdStage::_IsValidForLoad(const SdfPath& path) const
{
    if (!_IsValidForUnload(path)) {
        return false;
    }

    // XXX PERFORMANCE: could use HasPrimAtPath
    UsdPrim curPrim = GetPrimAtPath(path);

    if (!curPrim) {
        // Lets see if any ancestor exists, if so it's safe to attempt to load.
        SdfPath parentPath = path;
        while (parentPath != SdfPath::AbsoluteRootPath()) {
            if ((curPrim = GetPrimAtPath(parentPath))) {
                break;
            }
            parentPath = parentPath.GetParentPath();
        }

        // We walked up to the absolute root without finding anything
        // report error.
        if (parentPath == SdfPath::AbsoluteRootPath()) {
            TF_RUNTIME_ERROR("Attempt to load a path <%s> which is not "
                             "present in the stage",
                    path.GetString().c_str());
            return false;
        }
    }

    if (!curPrim.IsActive()) {
        TF_CODING_ERROR("Attempt to load an inactive path <%s>",
                        path.GetString().c_str());
        return false;
    }

    if (curPrim.IsMaster()) {
        TF_CODING_ERROR("Attempt to load instance master <%s>",
                        path.GetString().c_str());
        return false;
    }

    return true;
}

template <class Callback>
void
UsdStage::_WalkPrimsWithMasters(
    const SdfPath& rootPath, Callback const &cb) const
{
    tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash> seenMasterPrimPaths;
    if (UsdPrim root = GetPrimAtPath(rootPath))
        _WalkPrimsWithMastersImpl(root, cb, &seenMasterPrimPaths);
}

template <class Callback>
void
UsdStage::_WalkPrimsWithMastersImpl(
    UsdPrim const &prim,
    Callback const &cb,
    tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash> *seenMasterPrimPaths
    ) const
{
    UsdPrimRange children = UsdPrimRange::AllPrims(prim);
    WorkParallelForEach(
        children.begin(), children.end(),
        [=](UsdPrim const &child) {
            cb(child);
            if (child.IsInstance()) {
                const UsdPrim masterPrim = child.GetMaster();
                if (TF_VERIFY(masterPrim) &&
                    seenMasterPrimPaths->insert(masterPrim.GetPath()).second) {
                    // Recurse.
                    _WalkPrimsWithMastersImpl(
                        masterPrim, cb, seenMasterPrimPaths);
                }
            }
        });
}

void
UsdStage::_DiscoverPayloads(const SdfPath& rootPath,
                            UsdLoadPolicy policy,
                            SdfPathSet* primIndexPaths,
                            bool unloadedOnly,
                            SdfPathSet* usdPrimPaths) const
{
    tbb::concurrent_vector<SdfPath> primIndexPathsVec;
    tbb::concurrent_vector<SdfPath> usdPrimPathsVec;

    auto addPrimPayload =
        [this, unloadedOnly, primIndexPaths, usdPrimPaths,
         &primIndexPathsVec, &usdPrimPathsVec]
        (UsdPrim const &prim) {
        // Inactive prims are never included in this query.  Masters are
        // also never included, since they aren't independently loadable.
        if (!prim.IsActive() || prim.IsMaster())
            return;
        
        if (prim._GetSourcePrimIndex().HasAnyPayloads()) {
            SdfPath const &payloadIncludePath =
                prim._GetSourcePrimIndex().GetPath();
            if (!unloadedOnly ||
                !_cache->IsPayloadIncluded(payloadIncludePath)) {
                if (primIndexPaths)
                    primIndexPathsVec.push_back(payloadIncludePath);
                if (usdPrimPaths)
                    usdPrimPathsVec.push_back(prim.GetPath());
            }
        }
    };
    
    if (policy == UsdLoadWithDescendants) {
        _WalkPrimsWithMasters(rootPath, addPrimPayload);
    } else {
        addPrimPayload(GetPrimAtPath(rootPath));
    }
            
    // Copy stuff out.
    if (primIndexPaths) {
        primIndexPaths->insert(
            primIndexPathsVec.begin(), primIndexPathsVec.end());
    }
    if (usdPrimPaths) {
        usdPrimPaths->insert(usdPrimPathsVec.begin(), usdPrimPathsVec.end());
    }
}

void
UsdStage::_DiscoverAncestorPayloads(const SdfPath& rootPath,
                                    SdfPathSet* result,
                                    bool unloadedOnly) const
{
    if (rootPath == SdfPath::AbsoluteRootPath())
        return;

    for (SdfPath parentPath = rootPath.GetParentPath(); 
         parentPath != SdfPath::AbsoluteRootPath(); 
         parentPath = parentPath.GetParentPath()) {

        UsdPrim parent = GetPrimAtPath(parentPath);
        if (!parent)
            continue;

        // Inactive prims are never included in this query.
        // Masters are also never included, since they aren't
        // independently loadable.
        if (!parent.IsActive() || parent.IsMaster())
            continue;

        if (parent._GetSourcePrimIndex().HasAnyPayloads()) {
            const SdfPath& payloadIncludePath = 
                parent._GetSourcePrimIndex().GetPath();
            if (!unloadedOnly ||
                !_cache->IsPayloadIncluded(payloadIncludePath)) {
                TF_DEBUG(USD_PAYLOADS).Msg(
                    "PAYLOAD DISCOVERY: discovered ancestor payload at <%s>\n",
                    payloadIncludePath.GetText());
                result->insert(payloadIncludePath);
            } else {
                TF_DEBUG(USD_PAYLOADS).Msg(
                        "PAYLOAD DISCOVERY: ignored ancestor payload at <%s> "
                        "because it was already loaded\n",
                        payloadIncludePath.GetText());
            }
        }
    }
}

UsdPrim
UsdStage::Load(const SdfPath& path, UsdLoadPolicy policy)
{
    SdfPathSet exclude, include;
    include.insert(path);

    // Update the load set; this will trigger recomposition and include any
    // recursive payloads needed.
    LoadAndUnload(include, exclude, policy);

    return GetPrimAtPath(path);
}

void
UsdStage::Unload(const SdfPath& path)
{
    SdfPathSet include, exclude;
    exclude.insert(path);

    // Update the load set; this will trigger recomposition and include any
    // recursive payloads needed.
    LoadAndUnload(include, exclude);
}

void
UsdStage::LoadAndUnload(const SdfPathSet &loadSet,
                        const SdfPathSet &unloadSet,
                        UsdLoadPolicy policy)
{
    TfAutoMallocTag2 tag("Usd", _mallocTagID);

    SdfPathSet aggregateLoads, aggregateUnloads;
    _LoadAndUnload(loadSet, unloadSet,
                   &aggregateLoads, &aggregateUnloads, policy);

    // send notifications when loading or unloading
    if (aggregateLoads.empty() && aggregateUnloads.empty()) {
        return;
    }

    UsdStageWeakPtr self(this);
    SdfPathVector pathsToRecomposeVec;

    pathsToRecomposeVec.insert(pathsToRecomposeVec.begin(), 
                               aggregateLoads.begin(), aggregateLoads.end());
    pathsToRecomposeVec.insert(pathsToRecomposeVec.begin(),
                               aggregateUnloads.begin(), aggregateUnloads.end());
    SdfPath::RemoveDescendentPaths(&pathsToRecomposeVec);

    UsdNotice::ObjectsChanged::_PathsToChangesMap resyncChanges, infoChanges;
    for (const SdfPath& p : pathsToRecomposeVec) {
        resyncChanges[p];
    }

    UsdNotice::ObjectsChanged(self, &resyncChanges, &infoChanges).Send(self);

    UsdNotice::StageContentsChanged(self).Send(self);
}

void
UsdStage::_LoadAndUnload(const SdfPathSet &loadSet,
                         const SdfPathSet &unloadSet,
                         SdfPathSet *aggregateLoads,
                         SdfPathSet *aggregateUnloads,
                         UsdLoadPolicy policy)
{
    // Include implicit (recursive or ancestral) related payloads in both sets.
    SdfPathSet finalLoadSet, finalUnloadSet;

    // It's important that we do not include payloads that were previously
    // loaded because we need to iterate and will enter an infinite loop if we
    // do not reduce the load set on each iteration. This manifests below in
    // the unloadedOnly=true argument.
    for (auto const &path : loadSet) {
        if (!_IsValidForLoad(path)) {
            continue;
        }
        _DiscoverPayloads(path, policy, &finalLoadSet, /*unloadedOnly=*/true);
        _DiscoverAncestorPayloads(path, &finalLoadSet, /*unloadedOnly=*/true);
    }

    // Recursively populate the unload set.
    SdfPathVector unloadPruneSet;
    for (auto const &path: unloadSet) {
        if (!_IsValidForUnload(path)) {
            continue;
        }

        // Find all the prim index paths including recursively in masters.  Then
        // the payload exclude set is everything in pcp's payload set prefixed
        // by these paths.
        tbb::concurrent_vector<SdfPath> unloadIndexPaths;
        _WalkPrimsWithMasters(
            path,
            [&unloadIndexPaths] (UsdPrim const &prim) {
                if (prim.IsInMaster() && prim.HasAuthoredPayloads()) {
                    unloadIndexPaths.push_back(
                        prim._GetSourcePrimIndex().GetPath());
                }
            });
        UsdPrim prim = GetPrimAtPath(path);
        if (prim && !prim.IsInMaster())
            unloadPruneSet.push_back(prim._GetSourcePrimIndex().GetPath());
        unloadPruneSet.insert(unloadPruneSet.end(),
                              unloadIndexPaths.begin(), unloadIndexPaths.end());
    }
    TF_DEBUG(USD_PAYLOADS).Msg("PAYLOAD: unloadPruneSet: %s\n",
                               TfStringify(unloadPruneSet).c_str());
    SdfPath::RemoveDescendentPaths(&unloadPruneSet);

    // Now get the current load set and find everything that's prefixed by
    // something in unloadPruneSet.  That's the finalUnloadSet.
    SdfPathSet curLoadSet = _cache->GetIncludedPayloads(); //GetLoadSet();
    SdfPathVector curLoadVec(curLoadSet.begin(), curLoadSet.end());
    curLoadVec.erase(
        std::remove_if(
            curLoadVec.begin(), curLoadVec.end(),
            [&unloadPruneSet](SdfPath const &path) {
                return SdfPathFindLongestPrefix(
                    unloadPruneSet.begin(), unloadPruneSet.end(), path) ==
                    unloadPruneSet.end();
            }),
        curLoadVec.end());
    finalUnloadSet.insert(curLoadVec.begin(), curLoadVec.end());

    // If we aren't changing the load set, terminate recursion.
    if (finalLoadSet.empty() && finalUnloadSet.empty()) {
        TF_DEBUG(USD_PAYLOADS).Msg("PAYLOAD: terminate recursion\n");
        return;
    }

    // Debug output only.
    if (TfDebug::IsEnabled(USD_PAYLOADS)) {
        TF_DEBUG(USD_PAYLOADS).Msg("PAYLOAD: Load/Unload payload sets\n"
                                   "  Include set:\n");
        for (const auto& path : loadSet) {
            TF_DEBUG(USD_PAYLOADS).Msg("\t<%s>\n", path.GetString().c_str());
        }
        TF_DEBUG(USD_PAYLOADS).Msg("  Final Include set:\n");
        for (const auto& path : finalLoadSet) {
            TF_DEBUG(USD_PAYLOADS).Msg("\t<%s>\n", path.GetString().c_str());
        }

        TF_DEBUG(USD_PAYLOADS).Msg("  Exclude set:\n");
        for (const auto& path : unloadSet) {
            TF_DEBUG(USD_PAYLOADS).Msg("\t<%s>\n", path.GetString().c_str());
        }
        TF_DEBUG(USD_PAYLOADS).Msg("  Final Exclude set:\n");
        for (const auto& path : finalUnloadSet) {
            TF_DEBUG(USD_PAYLOADS).Msg("\t<%s>\n", path.GetString().c_str());
        }
    }

    ArResolverScopedCache resolverCache;

    // Send include/exclude sets to the PcpCache.
    PcpChanges changes;
    _cache->RequestPayloads(finalLoadSet, finalUnloadSet, &changes);

    // Recompose, given the resulting changes from Pcp.
    //
    // PERFORMANCE: Note that Pcp will always include the paths in
    // both sets as "significant changes" regardless of the actual changes
    // resulting from this request, this will trigger recomposition of UsdPrims
    // that potentially didn't change; it seems like we could do better.
    TF_DEBUG(USD_CHANGES).Msg("\nProcessing Load/Unload changes\n");
    _Recompose(changes);

    // Recurse.
    //
    // Note that recursion is not necessary for the unload set, which gets upon
    // the first recursion.
    
    // aggregate our results for notification
    if (aggregateLoads && aggregateUnloads) {
        aggregateLoads->insert(finalLoadSet.begin(), finalLoadSet.end());
        aggregateUnloads->insert(finalUnloadSet.begin(), finalUnloadSet.end());
    }

    _LoadAndUnload(loadSet, SdfPathSet(),
                   aggregateLoads, aggregateUnloads, policy);
}

SdfPathSet
UsdStage::GetLoadSet()
{
    SdfPathSet loadSet;
    for (const auto& primIndexPath : _cache->GetIncludedPayloads()) {
        // Get the path of the Usd prim using this prim index path.
        // This ensures we return the appropriate path if this prim index
        // is being used by a prim within a master.
        //
        // If there is no Usd prim using this prim index, we return the
        // prim index path anyway. This could happen if the ancestor of
        // a previously-loaded prim is deactivated, for instance. 
        // Including this path in the returned set reflects what's loaded
        // in the underlying PcpCache and ensures users can still unload
        // the payloads for those prims by calling 
        // LoadAndUnload([], GetLoadSet()).
        const SdfPath primPath = 
            _GetPrimPathUsingPrimIndexAtPath(primIndexPath);
        if (primPath.IsEmpty()) {
            loadSet.insert(primIndexPath);
        }
        else {
            loadSet.insert(primPath);
        }
    }

    return loadSet;
}

SdfPathSet
UsdStage::FindLoadable(const SdfPath& rootPath)
{
    SdfPath path = rootPath;

    // If the given path points to a prim beneath an instance,
    // convert it to the path of the prim in the corresponding master.
    // This ensures _DiscoverPayloads will always return paths to 
    // prims in masters for loadable prims in instances.
    if (!Usd_InstanceCache::IsPathInMaster(path)) {
        const SdfPath pathInMaster = 
            _instanceCache->GetPathInMasterForInstancePath(path);
        if (!pathInMaster.IsEmpty()) {
            path = pathInMaster;
        }
    }

    SdfPathSet loadable;
    _DiscoverPayloads(path, UsdLoadWithDescendants, nullptr,
                      /* unloadedOnly = */ false, &loadable);
    return loadable;
}

void
UsdStage::SetPopulationMask(UsdStagePopulationMask const &mask)
{
    // For now just set the mask and recompose everything.
    _populationMask = mask;

    PcpChanges changes;
    changes.DidChangeSignificantly(_cache.get(), SdfPath::AbsoluteRootPath());
    _Recompose(changes);
}

void
UsdStage::ExpandPopulationMask(
    std::function<bool (UsdRelationship const &)> const &relPred,
    std::function<bool (UsdAttribute const &)> const &attrPred)
{
    if (GetPopulationMask().IncludesSubtree(SdfPath::AbsoluteRootPath()))
        return;

    // Walk everything, calling UsdPrim::FindAllRelationshipTargetPaths() and
    // include them in the mask.  If the mask changes, call SetPopulationMask()
    // and redo.  Continue until the mask ceases expansion.  
    while (true) {
        auto root = GetPseudoRoot();
        SdfPathVector
            tgtPaths = root.FindAllRelationshipTargetPaths(relPred, false),
            connPaths = root.FindAllAttributeConnectionPaths(attrPred, false);
        
        tgtPaths.erase(remove_if(tgtPaths.begin(), tgtPaths.end(),
                                 [this](SdfPath const &path) {
                                     return _populationMask.Includes(path);
                                 }),
                       tgtPaths.end());
        connPaths.erase(remove_if(connPaths.begin(), connPaths.end(),
                                 [this](SdfPath const &path) {
                                     return _populationMask.Includes(path);
                                 }),
                       connPaths.end());
        
        if (tgtPaths.empty() && connPaths.empty())
            break;

        auto popMask = GetPopulationMask();
        for (auto const &path: tgtPaths) {
            popMask.Add(path.GetPrimPath());
        }
        for (auto const &path: connPaths) {
            popMask.Add(path.GetPrimPath());
        }
        SetPopulationMask(popMask);
    }
}

// ------------------------------------------------------------------------- //
// Instancing
// ------------------------------------------------------------------------- //

vector<UsdPrim>
UsdStage::GetMasters() const
{
    // Sort the instance master paths to provide a stable ordering for
    // this function.
    SdfPathVector masterPaths = _instanceCache->GetAllMasters();
    std::sort(masterPaths.begin(), masterPaths.end());

    vector<UsdPrim> masterPrims;
    for (const auto& path : masterPaths) {
        UsdPrim p = GetPrimAtPath(path);
        if (TF_VERIFY(p, "Failed to find prim at master path <%s>.\n",
                      path.GetText())) {
            masterPrims.push_back(p);
        }                   
    }
    return masterPrims;
}

Usd_PrimDataConstPtr 
UsdStage::_GetMasterForInstance(Usd_PrimDataConstPtr prim) const
{
    if (!prim->IsInstance()) {
        return NULL;
    }

    const SdfPath masterPath =
        _instanceCache->GetMasterForInstanceablePrimIndexPath(
            prim->GetPrimIndex().GetPath());
    return masterPath.IsEmpty() ? NULL : _GetPrimDataAtPath(masterPath);
}

bool 
UsdStage::_IsObjectDescendantOfInstance(const SdfPath& path) const
{
    // If the given path is a descendant of an instanceable
    // prim index, it would not be computed during composition unless
    // it is also serving as the source prim index for a master prim
    // on this stage.
    return (_instanceCache->IsPathDescendantToAnInstance(
            path.GetAbsoluteRootOrPrimPath()));
}

SdfPath
UsdStage::_GetPrimPathUsingPrimIndexAtPath(const SdfPath& primIndexPath) const
{
    SdfPath primPath;

    // In general, the path of a UsdPrim on a stage is the same as the
    // path of its prim index. However, this is not the case when
    // prims in masters are involved. In these cases, we need to use
    // the instance cache to map the prim index path to the master
    // prim on the stage.
    if (GetPrimAtPath(primIndexPath)) {
        primPath = primIndexPath;
    } 
    else if (_instanceCache->GetNumMasters() != 0) {
        const vector<SdfPath> mastersUsingPrimIndex = 
            _instanceCache->GetPrimsInMastersUsingPrimIndexPath(
                primIndexPath);

        for (const auto& pathInMaster : mastersUsingPrimIndex) {
            // If this path is a root prim path, it must be the path of a
            // master prim. This function wants to ignore master prims,
            // since they appear to have no prim index to the outside
            // consumer.
            //
            // However, if this is not a root prim path, it must be the
            // path of an prim nested inside a master, which we do want
            // to return. There will only ever be one of these, so we
            // can get this prim and break immediately.
            if (!pathInMaster.IsRootPrimPath()) {
                primPath = pathInMaster;
                break;
            }
        }
    }

    return primPath;
}

Usd_PrimDataPtr
UsdStage::_InstantiatePrim(const SdfPath &primPath)
{
    TfAutoMallocTag tag("Usd_PrimData");

    // Instantiate new prim data instance.
    Usd_PrimDataPtr p = new Usd_PrimData(this, primPath);
    pair<PathToNodeMap::iterator, bool> result;
    std::pair<SdfPath, Usd_PrimDataPtr> payload(primPath, p);
    {
        tbb::spin_rw_mutex::scoped_lock lock;
        if (_primMapMutex)
            lock.acquire(*_primMapMutex);
        result = _primMap.insert(payload);
    }

    // Insert entry into the map -- should always succeed.
    TF_VERIFY(result.second, "Newly instantiated prim <%s> already present in "
              "_primMap", primPath.GetText());
    return p;
}

namespace {
// Less-than comparison for iterators that compares what they point to.
struct _DerefIterLess {
    template <class Iter>
    bool operator()(const Iter &lhs, const Iter &rhs) const {
        return *lhs < *rhs;
    }
};
// Less-than comparison by prim name.
struct _PrimNameLess {
    template <class PrimPtr>
    bool operator()(const PrimPtr &lhs, const PrimPtr &rhs) const {
        return lhs->GetName() < rhs->GetName();
    }
};
// Less-than comparison of second element in a pair.
struct _SecondLess {
    template <class Pair>
    bool operator()(const Pair &lhs, const Pair &rhs) const {
        return lhs.second < rhs.second;
    }
};
}

// This method has some subtle behavior to support minimal repopulation and
// ideal allocation order.  See documentation for this method in the .h file for
// important details regarding this method's behavior.
void
UsdStage::_ComposeChildren(Usd_PrimDataPtr prim,
                           UsdStagePopulationMask const *mask, bool recurse)
{
    // If prim is deactivated, discard any existing children and return.
    if (!prim->IsActive()) {
        TF_DEBUG(USD_COMPOSITION).Msg("Inactive prim <%s>\n",
                                      prim->GetPath().GetText());
        _DestroyDescendents(prim);
        return;
    }

    // Instance prims do not directly expose any of their name children.
    // Discard any pre-existing children and add a task for composing
    // the instance's master's subtree if it's root uses this instance's
    // prim index as a source.
    if (prim->IsInstance()) {
        TF_DEBUG(USD_COMPOSITION).Msg("Instance prim <%s>\n",
                                      prim->GetPath().GetText());
        _DestroyDescendents(prim);

        const SdfPath& sourceIndexPath = 
            prim->GetSourcePrimIndex().GetPath();
        const SdfPath masterPath = 
            _instanceCache->GetMasterUsingPrimIndexPath(sourceIndexPath);

        if (!masterPath.IsEmpty()) {
            Usd_PrimDataPtr masterPrim = _GetPrimDataAtPath(masterPath);
            if (!masterPrim) {
                masterPrim = _InstantiatePrim(masterPath);

                // Master prims are parented beneath the pseudo-root,
                // but are *not* children of the pseudo-root. This ensures
                // that consumers never see master prims unless they are
                // explicitly asked for. So, we don't need to set the child
                // link here.
                masterPrim->_SetParentLink(_pseudoRoot);
            }
            // XXX: For now, always do full masters without masking.
            _ComposeSubtree(masterPrim, _pseudoRoot, /*mask=*/nullptr,
                            sourceIndexPath);
        }
        return;
    }

    // Compose child names for this prim.
    TfTokenVector nameOrder;
    if (!TF_VERIFY(prim->_ComposePrimChildNames(&nameOrder)))
        return;

    // Filter nameOrder by the mask, if necessary.  If this subtree is
    // completely included, stop looking at the mask from here forward.
    if (mask) {
        if (mask->IncludesSubtree(prim->GetPath())) {
            mask = nullptr;
        } else {
            // Remove all names from nameOrder that aren't included in the mask.
            SdfPath const &primPath = prim->GetPath();
            nameOrder.erase(
                remove_if(nameOrder.begin(), nameOrder.end(),
                          [&primPath, mask](TfToken const &nameTok) {
                              return !mask->Includes(
                                  primPath.AppendChild(nameTok));
                          }), nameOrder.end());
        }
    }

    // If the prim has no children, simply destroy any existing child prims.
    if (nameOrder.empty()) {
        TF_DEBUG(USD_COMPOSITION).Msg("Children empty <%s>\n",
                                      prim->GetPath().GetText());
        _DestroyDescendents(prim);
        return;
    }

    // Find the first mismatch between the prim's current child prims and
    // the new list of child prims specified in nameOrder.
    Usd_PrimDataSiblingIterator
        begin = prim->_ChildrenBegin(),
        end = prim->_ChildrenEnd(),
        cur = begin;
    TfTokenVector::const_iterator
        curName = nameOrder.begin(),
        nameEnd = nameOrder.end();
    for (; cur != end && curName != nameEnd; ++cur, ++curName) {
        if ((*cur)->GetName() != *curName)
            break;
    }

    // The prims in [begin, cur) match the children specified in 
    // [nameOrder.begin(), curName); recompose these child subtrees if needed.
    if (recurse) {
        for (Usd_PrimDataSiblingIterator it = begin; it != cur; ++it) {
            _ComposeChildSubtree(*it, prim, mask);
        }
    }

    // The prims in [cur, end) do not match the children specified in 
    // [curName, nameEnd), so we need to process these trailing elements.

    // No trailing elements means children are unchanged.
    if (cur == end && curName == nameEnd) {
        TF_DEBUG(USD_COMPOSITION).Msg("Children same in same order <%s>\n",
                                      prim->GetPath().GetText());
        return;
    }
        
    // Trailing names only mean that children have been added to the end
    // of the prim's existing children. Note this includes the case where
    // the prim had no children previously.
    if (cur == end && curName != nameEnd) {
        const SdfPath& parentPath = prim->GetPath();
        Usd_PrimDataPtr head = nullptr, prev = nullptr, tail = nullptr;
        for (; curName != nameEnd; ++curName) {
            tail = _InstantiatePrim(parentPath.AppendChild(*curName));
            if (recurse) {
                _ComposeChildSubtree(tail, prim, mask);
            }
            if (!prev) {
                head = tail;
            }
            else {
                prev->_SetSiblingLink(tail);
            }
            prev = tail;
        }

        if (cur == begin) {
            TF_DEBUG(USD_COMPOSITION).Msg("Children all new <%s>\n",
                                          prim->GetPath().GetText());
            TF_VERIFY(!prim->_firstChild);
            prim->_firstChild = head;
            tail->_SetParentLink(prim);
        }
        else {
            TF_DEBUG(USD_COMPOSITION).Msg("Children appended <%s>\n",
                                          prim->GetPath().GetText());
            Usd_PrimDataSiblingIterator lastChild = begin, next = begin;
            for (++next; next != cur; lastChild = next, ++next) { }
            
            (*lastChild)->_SetSiblingLink(head);
            tail->_SetParentLink(prim);
        }
        return;
    }

    // Trailing children only mean that children have been removed from
    // the end of the prim's existing children.
    if (cur != end && curName == nameEnd) {
        TF_DEBUG(USD_COMPOSITION).Msg("Children removed from end <%s>\n",
                                      prim->GetPath().GetText());
        for (Usd_PrimDataSiblingIterator it = cur; it != end; ) {
            // Make sure we advance to the next sibling before we destroy
            // the current child so we don't read from a deleted prim.
            _DestroyPrim(*it++);
        }

        if (cur == begin) {
            prim->_firstChild = nullptr;
        }
        else {
            Usd_PrimDataSiblingIterator lastChild = begin, next = begin;
            for (++next; next != cur; lastChild = next, ++next) { }
            (*lastChild)->_SetParentLink(prim);
        }
        return;
    }

    // Otherwise, both trailing children and names mean there was some 
    // other change to the prim's list of children. Do the general form 
    // of preserving preexisting children and ordering them according 
    // to nameOrder.
    TF_DEBUG(USD_COMPOSITION).Msg(
        "Require general children recomposition <%s>\n",
        prim->GetPath().GetText());

    // Make a vector of iterators into nameOrder from [curName, nameEnd).
    typedef vector<TfTokenVector::const_iterator> TokenVectorIterVec;
    TokenVectorIterVec nameOrderIters(std::distance(curName, nameEnd));
    for (size_t i = 0, sz = nameOrderIters.size(); i != sz; ++i) {
        nameOrderIters[i] = curName + i;
    }

    // Sort the name order iterators *by name*.
    sort(nameOrderIters.begin(), nameOrderIters.end(), _DerefIterLess());

    // Make a vector of the existing prim children and sort them by name.
    vector<Usd_PrimDataPtr> oldChildren(cur, end);
    sort(oldChildren.begin(), oldChildren.end(), _PrimNameLess());

    vector<Usd_PrimDataPtr>::const_iterator
        oldChildIt = oldChildren.begin(),
        oldChildEnd = oldChildren.end();

    TokenVectorIterVec::const_iterator
        newNameItersIt = nameOrderIters.begin(),
        newNameItersEnd = nameOrderIters.end();

    // We build a vector of pairs of prims and the original name order
    // iterators.  This lets us re-sort by original order once we're finished.
    vector<pair<Usd_PrimDataPtr, TfTokenVector::const_iterator> >
        tempChildren;
    tempChildren.reserve(nameOrderIters.size());

    const SdfPath &parentPath = prim->GetPath();

    while (newNameItersIt != newNameItersEnd || oldChildIt != oldChildEnd) {
        // Walk through old children that no longer exist up to the current
        // potentially new name, removing them.
        while (oldChildIt != oldChildEnd &&
               (newNameItersIt == newNameItersEnd ||
                (*oldChildIt)->GetName() < **newNameItersIt)) {
            TF_DEBUG(USD_COMPOSITION).Msg("Removing <%s>\n",
                                          (*oldChildIt)->GetPath().GetText());
            _DestroyPrim(*oldChildIt++);
        }

        // Walk through any matching children and preserve them.
        for (; newNameItersIt != newNameItersEnd &&
                 oldChildIt != oldChildEnd &&
                 **newNameItersIt == (*oldChildIt)->GetName();
             ++newNameItersIt, ++oldChildIt) {
            TF_DEBUG(USD_COMPOSITION).Msg("Preserving <%s>\n",
                                          (*oldChildIt)->GetPath().GetText());
            tempChildren.push_back(make_pair(*oldChildIt, *newNameItersIt));
            if (recurse) {
                Usd_PrimDataPtr child = tempChildren.back().first;
                _ComposeChildSubtree(child, prim, mask);
            }
        }

        // Walk newly-added names up to the next old name, adding them.
        for (; newNameItersIt != newNameItersEnd &&
                 (oldChildIt == oldChildEnd ||
                  **newNameItersIt < (*oldChildIt)->GetName());
             ++newNameItersIt) {
            SdfPath newChildPath = parentPath.AppendChild(**newNameItersIt);
            TF_DEBUG(USD_COMPOSITION).Msg("Creating new <%s>\n",
                                          newChildPath.GetText());
            tempChildren.push_back(
                make_pair(_InstantiatePrim(newChildPath), *newNameItersIt));
            if (recurse) {
                Usd_PrimDataPtr child = tempChildren.back().first;
                _ComposeChildSubtree(child, prim, mask);
            }
        }
    }

    // tempChildren should never be empty at this point. If it were, it means
    // that the above loop would have only deleted existing children, but that
    // case is covered by optimization 4 above.
    if (!TF_VERIFY(!tempChildren.empty())) {
        return;
    }

    // Now all the new children are in lexicographical order by name, paired
    // with their name's iterator in the original name order.  Recover the
    // original order by sorting by the iterators natural order.
    sort(tempChildren.begin(), tempChildren.end(), _SecondLess());

    // Now all the new children are correctly ordered.  Set the 
    // sibling and parent links to add them to the prim's children.
    for (size_t i = 0, e = tempChildren.size() - 1; i < e; ++i) {
        tempChildren[i].first->_SetSiblingLink(tempChildren[i+1].first);
    }
    tempChildren.back().first->_SetParentLink(prim);

    if (cur == begin) {
        prim->_firstChild = tempChildren.front().first;
    }
    else {
        Usd_PrimDataSiblingIterator lastChild = begin, next = begin;
        for (++next; next != cur; lastChild = next, ++next) { }
        (*lastChild)->_SetSiblingLink(tempChildren.front().first);
    }
}

void 
UsdStage::_ComposeChildSubtree(Usd_PrimDataPtr prim, 
                               Usd_PrimDataConstPtr parent,
                               UsdStagePopulationMask const *mask)
{
    if (parent->IsInMaster()) {
        // If this UsdPrim is a child of an instance master, its 
        // source prim index won't be at the same path as its stage path.
        // We need to construct the path from the parent's source index.
        const SdfPath sourcePrimIndexPath = 
            parent->GetSourcePrimIndex().GetPath().AppendChild(prim->GetName());
        _ComposeSubtree(prim, parent, mask, sourcePrimIndexPath);
    }
    else {
        _ComposeSubtree(prim, parent, mask);
    }
}

void
UsdStage::_ReportPcpErrors(const PcpErrorVector &errors,
                           const std::string &context) const
{
    _ReportErrors(errors, std::vector<std::string>(), context);
}

// Report any errors.  It's important for error filtering that each
// error be a single line. It's equally important that we provide
// some clue to associating the errors to the originating stage
// (it is caller's responsibility to ensure that any further required
// context (e.g. prim path) be present in 'context' already).  We choose
// a balance between total specificity (which would require identifying
// both the session layer and ArResolverContext and be very long) 
// and brevity.  We can modulate this behavior with TfDebug if needed.
// Finally, we use a mutex to ensure there is no interleaving of errors
// from multiple threads.
void
UsdStage::_ReportErrors(const PcpErrorVector &errors,
                        const std::vector<std::string> &otherErrors,
                        const std::string &context) const
{
    static std::mutex   errMutex;
   
    if (!errors.empty() || !otherErrors.empty()) {
        std::string  fullContext = TfStringPrintf("(%s on stage @%s@ <%p>)", 
                                      context.c_str(), 
                                      GetRootLayer()->GetIdentifier().c_str(),
                                      this);
        std::vector<std::string>  allErrors;
        allErrors.reserve(errors.size() + otherErrors.size());

        for (const auto& err : errors) {
            allErrors.push_back(TfStringPrintf("%s %s", 
                                               err->ToString().c_str(), 
                                               fullContext.c_str()));
        }
        for (const auto& err : otherErrors) {
            allErrors.push_back(TfStringPrintf("%s %s", 
                                               err.c_str(), 
                                               fullContext.c_str()));
        }

        {
            std::lock_guard<std::mutex>  lock(errMutex);

            for (const auto &err : allErrors){
                TF_WARN(err);
            }
        }
    }
}

void
UsdStage::_ComposeSubtreeInParallel(Usd_PrimDataPtr prim)
{
    _ComposeSubtreesInParallel(vector<Usd_PrimDataPtr>(1, prim));
}

void
UsdStage::_ComposeSubtreesInParallel(
    const vector<Usd_PrimDataPtr> &prims,
    const vector<SdfPath> *primIndexPaths)
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // TF_DEBUG(USD_COMPOSITION).Msg("Composing Subtrees at: %s\n",
    //     TfStringify(
    //         [&prims]() {
    //             vector<SdfPath> paths;
    //             for (auto p : prims) { paths.push_back(p->GetPath()); }
    //             return paths;
    //         }()).c_str());

    TRACE_FUNCTION();

    // Begin a subtree composition in parallel.  Calling _ComposeChildren and
    // passing recurse=true will spawn a task for each subtree.

    _primMapMutex = boost::in_place();
    _dispatcher = boost::in_place();
    
    for (size_t i = 0; i != prims.size(); ++i) {
        Usd_PrimDataPtr p = prims[i];
        _dispatcher->Run(
            &UsdStage::_ComposeSubtreeImpl, this, p, p->GetParent(),
            p->IsInMaster() ? nullptr : &_populationMask,
            primIndexPaths ? (*primIndexPaths)[i] : p->GetPath());
    }

    _dispatcher = boost::none;
    _primMapMutex = boost::none;
}

void
UsdStage::_ComposeSubtree(
    Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
    UsdStagePopulationMask const *mask,
    const SdfPath& primIndexPath)
{
    if (_dispatcher) {
        _dispatcher->Run(
            &UsdStage::_ComposeSubtreeImpl, this,
            prim, parent, mask, primIndexPath);
    } else {
        // TF_DEBUG(USD_COMPOSITION).Msg("Composing Subtree at <%s>\n",
        //                               prim->GetPath().GetText());
        // TRACE_FUNCTION();
        _ComposeSubtreeImpl(prim, parent, mask, primIndexPath);
    }
}

void
UsdStage::_ComposeSubtreeImpl(
    Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
    UsdStagePopulationMask const *mask,
    const SdfPath& inPrimIndexPath)
{
    TfAutoMallocTag2 tag("Usd", _mallocTagID);

    const SdfPath primIndexPath = 
        (inPrimIndexPath.IsEmpty() ? prim->GetPath() : inPrimIndexPath);

    // Compute the prim's PcpPrimIndex.
    PcpErrorVector errors;
    prim->_primIndex =
        &_GetPcpCache()->ComputePrimIndex(primIndexPath, &errors);

    // Report any errors.
    if (!errors.empty()) {
        _ReportPcpErrors(
            errors, TfStringPrintf("computing prim index <%s>",
                                   primIndexPath.GetText()));
    }

    parent = parent ? parent : prim->GetParent();

    // If this prim's parent is the pseudo-root and it has a different
    // path from its source prim index, it must represent a master prim.
    const bool isMasterPrim =
        (parent == _pseudoRoot 
         && prim->_primIndex->GetPath() != prim->GetPath());

    // Compose the typename for this prim unless it's a master prim, since
    // master prims don't expose any data except name children.
    // Note this needs to come before _ComposeAndCacheFlags, since that
    // function may need typename to be populated.
    if (isMasterPrim) {
        prim->_typeName = TfToken();
    }
    else {
        prim->_typeName = _ComposeTypeName(prim->GetPrimIndex());
    }

    // Compose flags for prim.
    prim->_ComposeAndCacheFlags(parent, isMasterPrim);

    // Pre-compute clip information for this prim to avoid doing so
    // at value resolution time.
    if (prim->GetPath() != SdfPath::AbsoluteRootPath()) {
        bool primHasAuthoredClips = _clipCache->PopulateClipsForPrim(
            prim->GetPath(), prim->GetPrimIndex());
        prim->_SetMayHaveOpinionsInClips(
            primHasAuthoredClips || parent->MayHaveOpinionsInClips());
    }

    // Compose the set of children on this prim.
    _ComposeChildren(prim, mask, /*recurse=*/true);
}

void
UsdStage::_DestroyDescendents(Usd_PrimDataPtr prim)
{
    // Recurse to children first.
    Usd_PrimDataSiblingIterator
        childIt = prim->_ChildrenBegin(), childEnd = prim->_ChildrenEnd();
    prim->_firstChild = NULL;
    while (childIt != childEnd) {
        if (_dispatcher) {
            _dispatcher->Run(&UsdStage::_DestroyPrim, this, *childIt++);
        } else {
            _DestroyPrim(*childIt++);
        }
    }
}

void 
UsdStage::_DestroyPrimsInParallel(const vector<SdfPath>& paths)
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    TRACE_FUNCTION();

    TF_AXIOM(!_dispatcher && !_primMapMutex);

    _primMapMutex = boost::in_place();
    _dispatcher = boost::in_place();

    for (const auto& path : paths) {
        Usd_PrimDataPtr prim = _GetPrimDataAtPath(path);
        // XXX: This should be converted to a TF_VERIFY once
        // bug 141575 is fixed.
        if (prim) {
            _dispatcher->Run(&UsdStage::_DestroyPrim, this, prim);
        }
    }

    _dispatcher = boost::none;
    _primMapMutex = boost::none;
}

void
UsdStage::_DestroyPrim(Usd_PrimDataPtr prim)
{
    TF_DEBUG(USD_COMPOSITION).Msg(
        "Destroying <%s>\n", prim->GetPath().GetText());

    // Destroy descendents first.
    _DestroyDescendents(prim);

    // Set the prim's dead bit.
    prim->_MarkDead();

    // Remove from the map -- this prim should always be present.

    // XXX: We intentionally copy the prim's path to the local variable primPath
    // here.  If we don't, the erase() call ends up reading freed memory.  This
    // is because libstdc++'s hash_map's backing implementation implements
    // erase() as: find the first element with a matching key, erase it, then
    // walk subsequent bucket elements with matching keys and erase them.  This
    // might seem odd for hash_map since only one element can have the given
    // key, but it works this way since the backing implementation is shared
    // between hash_map and hash_multimap.
    //
    // This is a problem since prim->GetPath() returns a const reference to a
    // member variable, so once the first element (and only element) is erased
    // the reference is invalidated, but erase() may look at the path reference
    // again to do the key comparison for subsequent elements.  Copying the path
    // out to a local variable ensures it stays alive for the duration of
    // erase().
    //
    // NOTE: The above was true in gcc 4.4 but not in gcc 4.8, nor is it
    //       true in boost::unordered_map or std::unordered_map.
    if (!_isClosingStage) {
        SdfPath primPath = prim->GetPath(); 
        tbb::spin_rw_mutex::scoped_lock lock;
        const bool hasMutex = static_cast<bool>(_primMapMutex);
        if (hasMutex)
            lock.acquire(*_primMapMutex);
        bool erased = _primMap.erase(primPath);
        if (hasMutex)
            lock.release();
        TF_VERIFY(erased, 
                  "Destroyed prim <%s> not present in stage's data structures",
                  prim->GetPath().GetString().c_str());
    }
}

void
UsdStage::Reload()
{
    TfAutoMallocTag2 tag("Usd", _mallocTagID);

    ArResolverScopedCache resolverCache;

    PcpChanges changes;
    _cache->Reload(&changes);

    // XXX: Usd should ideally be doing the reloads for both clip layers
    // as well as any that need to be reloaded as noticed by Pcp.
    // See bug/140498 for more info.
    SdfLayer::ReloadLayers(_clipCache->GetUsedLayers()); 

    // Process changes.  This won't be invoked automatically if we didn't
    // reload any layers but only loaded layers that we failed to load
    // previously (because loading a previously unloaded layer doesn't
    // invoke change processing).
    _Recompose(changes);
}

/*static*/
bool
UsdStage::IsSupportedFile(const std::string& filePath) 
{
    if (filePath.empty()) {
        TF_CODING_ERROR("Empty file path given");
        return false;
    }

    // grab the file's extension, and assert it to be valid
    std::string fileExtension = SdfFileFormat::GetFileExtension(filePath);
    if (fileExtension.empty()) {
        return false;
    }

    // if the extension is valid we'll get a non null FileFormatPtr
    return SdfFileFormat::FindByExtension(fileExtension, 
                                          UsdUsdFileFormatTokens->Target);
}

namespace {

void _SaveLayers(const SdfLayerHandleVector& layers)
{
    for (const SdfLayerHandle& layer : layers) {
        if (!layer->IsDirty()) {
            continue;
        }

        if (layer->IsAnonymous()) {
            TF_WARN("Not saving @%s@ because it is an anonymous layer",
                    layer->GetIdentifier().c_str());
            continue;
        }

        // Sdf will emit errors if there are any problems with
        // saving the layer.
        layer->Save();
    }
}

}

void
UsdStage::Save()
{
    SdfLayerHandleVector layers = GetUsedLayers();

    const PcpLayerStackPtr localLayerStack = _GetPcpCache()->GetLayerStack();
    if (TF_VERIFY(localLayerStack)) {
        const SdfLayerHandleVector sessionLayers = 
            localLayerStack->GetSessionLayers();
        const auto isSessionLayer = 
            [&sessionLayers](const SdfLayerHandle& l) {
                return std::find(
                    sessionLayers.begin(), sessionLayers.end(), l) 
                    != sessionLayers.end();
            };

        layers.erase(std::remove_if(layers.begin(), layers.end(), 
                                    isSessionLayer),
                     layers.end());
    }

    _SaveLayers(layers);
}

void
UsdStage::SaveSessionLayers()
{
    const PcpLayerStackPtr localLayerStack = _GetPcpCache()->GetLayerStack();
    if (TF_VERIFY(localLayerStack)) {
        _SaveLayers(localLayerStack->GetSessionLayers());
    }
}

std::pair<bool, UsdPrim>
UsdStage::_IsValidPathForCreatingPrim(const SdfPath &path) const
{
    std::pair<bool, UsdPrim> status = { false, UsdPrim() };

    // Path must be absolute.
    if (ARCH_UNLIKELY(!path.IsAbsolutePath())) {
        TF_CODING_ERROR("Path must be an absolute path: <%s>", path.GetText());
        return status;
    }

    // Path must be a prim path (or the absolute root path).
    if (ARCH_UNLIKELY(!path.IsAbsoluteRootOrPrimPath())) {
        TF_CODING_ERROR("Path must be a prim path: <%s>", path.GetText());
        return status;
    }

    // Path must not contain variant selections.
    if (ARCH_UNLIKELY(path.ContainsPrimVariantSelection())) {
        TF_CODING_ERROR("Path must not contain variant selections: <%s>",
                        path.GetText());
        return status;
    }

    const UsdPrim prim = GetPrimAtPath(path);
    if (ARCH_UNLIKELY(prim ? !_ValidateEditPrim(prim, "create prim")
                           : !_ValidateEditPrimAtPath(path, "create prim"))) {
        return status;
    }

    status = { true, prim };
    return status;
}

UsdPrim
UsdStage::OverridePrim(const SdfPath &path)
{
    // Special-case requests for the root.  It always succeeds and never does
    // authoring since the root cannot have PrimSpecs.
    if (path == SdfPath::AbsoluteRootPath())
        return GetPseudoRoot();
    
    // Validate path input.
    std::pair<bool, UsdPrim> status = _IsValidPathForCreatingPrim(path);
    if (!status.first) {
        return UsdPrim();
    }

    // Do the authoring, if any to do.
    if (!status.second) {
        {
            SdfChangeBlock block;
            TfErrorMark m;
            SdfPrimSpecHandle primSpec = 
                _CreatePrimSpecAtEditTarget(GetEditTarget(), path);
            // If spec creation failed, return.  Issue an error if a more
            // specific error wasn't already issued.
            if (!primSpec) {
                if (m.IsClean())
                    TF_RUNTIME_ERROR("Failed to create PrimSpec for <%s>",
                                     path.GetText());
                return UsdPrim();
            }
        }

        // Attempt to fetch the prim we tried to create.
        status.second = GetPrimAtPath(path);
    }

    return status.second;
}

UsdPrim
UsdStage::DefinePrim(const SdfPath &path,
                     const TfToken &typeName)
{
    // Validate path input.
    if (!_IsValidPathForCreatingPrim(path).first)
        return UsdPrim();

    return _DefinePrim(path, typeName);
}

UsdPrim 
UsdStage::_DefinePrim(const SdfPath &path, const TfToken &typeName)
{
    // Special-case requests for the root.  It always succeeds and never does
    // authoring since the root cannot have PrimSpecs.
    if (path == SdfPath::AbsoluteRootPath())
        return GetPseudoRoot();

    // Define all ancestors.
    if (!_DefinePrim(path.GetParentPath(), TfToken()))
        return UsdPrim();
    
    // Now author scene description for this prim.
    TfErrorMark m;
    UsdPrim prim = GetPrimAtPath(path);
    if (!prim || !prim.IsDefined() ||
        (!typeName.IsEmpty() && prim.GetTypeName() != typeName)) {
        {
            SdfChangeBlock block;
            SdfPrimSpecHandle primSpec = 
                _CreatePrimSpecAtEditTarget(GetEditTarget(), path);
            // If spec creation failed, return.  Issue an error if a more
            // specific error wasn't already issued.
            if (!primSpec) {
                if (m.IsClean())
                    TF_RUNTIME_ERROR(
                        "Failed to create primSpec for <%s>", path.GetText());
                return UsdPrim();
            }
            
            // Set specifier and typeName, if not empty.
            primSpec->SetSpecifier(SdfSpecifierDef);
            if (!typeName.IsEmpty())
                primSpec->SetTypeName(typeName);
        }
        // Fetch prim if newly created.
        prim = prim ? prim : GetPrimAtPath(path);
    }
    
    // Issue an error if we were unable to define this prim and an error isn't
    // already issued.
    if ((!prim || !prim.IsDefined()) && m.IsClean())
        TF_RUNTIME_ERROR("Failed to define UsdPrim <%s>", path.GetText());

    return prim;
}

UsdPrim
UsdStage::CreateClassPrim(const SdfPath &path)
{
    // Classes must be root prims.
    if (!path.IsRootPrimPath()) {
        TF_CODING_ERROR("Classes must be root prims.  <%s> is not a root prim "
                        "path", path.GetText());
        return UsdPrim();
    }

    // Classes must be created in local layers.
    if (_editTarget.GetMapFunction().IsIdentity() &&
        !HasLocalLayer(_editTarget.GetLayer())) {
        TF_CODING_ERROR("Must create classes in local LayerStack");
        return UsdPrim();
    }

    // Validate path input.
    const std::pair<bool, UsdPrim> status = _IsValidPathForCreatingPrim(path);
    if (!status.first) {
        return UsdPrim();
    }

    UsdPrim prim = status.second;

    // It's an error to try to transform a defined non-class into a class.
    if (prim && prim.IsDefined() &&
        prim.GetSpecifier() != SdfSpecifierClass) {
        TF_RUNTIME_ERROR("Non-class prim already exists at <%s>",
                         path.GetText());
        return UsdPrim();
    }

    // Stamp a class PrimSpec if need-be.
    if (!prim || !prim.IsAbstract()) {
        prim = _DefinePrim(path, TfToken());
        if (prim)
            prim.SetMetadata(SdfFieldKeys->Specifier, SdfSpecifierClass);
    }
    return prim;
}

bool
UsdStage::RemovePrim(const SdfPath& path)
{
    return _RemovePrim(path);
}

const UsdEditTarget &
UsdStage::GetEditTarget() const
{
    return _editTarget;
}

UsdEditTarget
UsdStage::GetEditTargetForLocalLayer(size_t i)
{
    const SdfLayerRefPtrVector & layers = _cache->GetLayerStack()->GetLayers();
    if (i >= layers.size()) {
        TF_CODING_ERROR("Layer index %zu is out of range: only %zu entries in "
                        "layer stack", i, layers.size());
        return UsdEditTarget();
    }
    const SdfLayerOffset *layerOffset =
        _cache->GetLayerStack()->GetLayerOffsetForLayer(i);
    return UsdEditTarget(layers[i],
                         layerOffset ? *layerOffset : SdfLayerOffset() );
}

UsdEditTarget 
UsdStage::GetEditTargetForLocalLayer(const SdfLayerHandle &layer)
{
    const SdfLayerOffset *layerOffset =
        _cache->GetLayerStack()->GetLayerOffsetForLayer(layer);
    return UsdEditTarget(layer, layerOffset ? *layerOffset : SdfLayerOffset() );
}

bool
UsdStage::HasLocalLayer(const SdfLayerHandle &layer) const
{
    return _cache->GetLayerStack()->HasLayer(layer);
}

void
UsdStage::SetEditTarget(const UsdEditTarget &editTarget)
{
    if (!editTarget.IsValid()){
        TF_CODING_ERROR("Attempt to set an invalid UsdEditTarget as current");
        return;
    }
    // Do some extra error checking if the EditTarget specifies a local layer.
    if (editTarget.GetMapFunction().IsIdentity() &&
        !HasLocalLayer(editTarget.GetLayer())) {
        TF_CODING_ERROR("Layer @%s@ is not in the local LayerStack rooted "
                        "at @%s@",
                        editTarget.GetLayer()->GetIdentifier().c_str(),
                        GetRootLayer()->GetIdentifier().c_str());
        return;
    }

    // If different from current, set EditTarget and notify.
    if (editTarget != _editTarget) {
        _editTarget = editTarget;
        UsdStageWeakPtr self(this);
        UsdNotice::StageEditTargetChanged(self).Send(self);
    }
}

SdfLayerHandle
UsdStage::GetRootLayer() const
{
    return _rootLayer;
}

ArResolverContext
UsdStage::GetPathResolverContext() const
{
    if (!TF_VERIFY(_GetPcpCache())) {
        static ArResolverContext empty;
        return empty;
    }
    return _GetPcpCache()->GetLayerStackIdentifier().pathResolverContext;
}

SdfLayerHandleVector
UsdStage::GetLayerStack(bool includeSessionLayers) const
{
    SdfLayerHandleVector result;

    // Pcp's API lets us get either the whole stack or just the session layer
    // stack.  We get the whole stack and either copy the whole thing to Handles
    // or only the portion starting at the root layer to the end.

    if (PcpLayerStackPtr layerStack = _cache->GetLayerStack()) {
        const SdfLayerRefPtrVector &layers = layerStack->GetLayers();

        // Copy everything if sublayers requested, otherwise copy from the root
        // layer to the end.
        SdfLayerRefPtrVector::const_iterator copyBegin =
            includeSessionLayers ? layers.begin() :
            find(layers.begin(), layers.end(), GetRootLayer());

        TF_VERIFY(copyBegin != layers.end(),
                  "Root layer @%s@ not in LayerStack",
                  GetRootLayer()->GetIdentifier().c_str());

        result.assign(copyBegin, layers.end());
    }

    return result;
}

SdfLayerHandleVector
UsdStage::GetUsedLayers(bool includeClipLayers) const
{
    if (!_cache)
        return SdfLayerHandleVector();
    
    SdfLayerHandleSet usedLayers = _cache->GetUsedLayers();

    if (includeClipLayers && _clipCache){
        SdfLayerHandleSet clipLayers = _clipCache->GetUsedLayers();
        if (!clipLayers.empty()){
            usedLayers.insert(clipLayers.begin(), clipLayers.end());
        }
    }

    return SdfLayerHandleVector(usedLayers.begin(), usedLayers.end());
}


SdfLayerHandle
UsdStage::GetSessionLayer() const
{
    return _sessionLayer;
}

void
UsdStage::MuteLayer(const std::string &layerIdentifier)
{
    MuteAndUnmuteLayers({layerIdentifier}, {});
}

void
UsdStage::UnmuteLayer(const std::string &layerIdentifier)
{
    MuteAndUnmuteLayers({}, {layerIdentifier});
}

void 
UsdStage::MuteAndUnmuteLayers(const std::vector<std::string> &muteLayers,
                              const std::vector<std::string> &unmuteLayers)
{
    TfAutoMallocTag2 tag("Usd", _mallocTagID);

    PcpChanges changes;
    _cache->RequestLayerMuting(muteLayers, unmuteLayers, &changes);
    if (changes.IsEmpty()) {
        return;
    }

    using _PathsToChangesMap = UsdNotice::ObjectsChanged::_PathsToChangesMap;
    _PathsToChangesMap resyncChanges, infoChanges;
    _Recompose(changes, &resyncChanges);

    UsdStageWeakPtr self(this);

    UsdNotice::ObjectsChanged(self, &resyncChanges, &infoChanges)
        .Send(self);
    UsdNotice::StageContentsChanged(self).Send(self);
}

const std::vector<std::string>&
UsdStage::GetMutedLayers() const
{
    return _cache->GetMutedLayers();
}

bool
UsdStage::IsLayerMuted(const std::string& layerIdentifier) const
{
    return _cache->IsLayerMuted(layerIdentifier);
}

UsdPrimRange
UsdStage::Traverse()
{
    return UsdPrimRange::Stage(UsdStagePtr(this));
}

UsdPrimRange
UsdStage::Traverse(const Usd_PrimFlagsPredicate &predicate)
{
    return UsdPrimRange::Stage(UsdStagePtr(this), predicate);
}

UsdPrimRange
UsdStage::TraverseAll()
{
    return UsdPrimRange::Stage(UsdStagePtr(this), UsdPrimAllPrimsPredicate);
}

bool
UsdStage::_RemovePrim(const SdfPath& path)
{
    SdfPrimSpecHandle spec = _GetPrimSpec(path);
    if (!spec) {
        return false;
    }

    SdfPrimSpecHandle parent = spec->GetRealNameParent();
    if (!parent) {
        return false;
    }

    return parent->RemoveNameChild(spec);
}

bool
UsdStage::_RemoveProperty(const SdfPath &path)
{
    SdfPropertySpecHandle propHandle =
        GetEditTarget().GetPropertySpecForScenePath(path);

    if (!propHandle) {
        return false;
    }

    // dynamic cast needed because of protected copyctor
    // safe to assume a prim owner because we are in UsdPrim
    SdfPrimSpecHandle parent 
        = TfDynamic_cast<SdfPrimSpecHandle>(propHandle->GetOwner());

    if (!TF_VERIFY(parent, "Prop has no parent")) {
        return false;
    }

    parent->RemoveProperty(propHandle);
    return true;
}

template <class... Values>
static void
_AddToChangedPaths(SdfPathVector *paths, const SdfPath& p, 
                   const Values&... data)
{
    paths->push_back(p);
}

template <class ChangedPaths, class... Values>
static void
_AddToChangedPaths(ChangedPaths *paths, const SdfPath& p, const Values&... data)
{
    (*paths)[p].emplace_back(data...);
}

static std::string
_Stringify(const SdfPathVector& paths)
{
    return TfStringify(paths);
}

template <class ChangedPaths>
static std::string
_Stringify(const ChangedPaths& paths)
{
    return _Stringify(SdfPathVector(
        make_transform_iterator(paths.begin(), TfGet<0>()),
        make_transform_iterator(paths.end(), TfGet<0>())));
}

// Add paths in the given cache that depend on the given path in the given 
// layer to changedPaths. If ChangedPaths is a map of paths to list of 
// objects, will construct an object using the given extraData
// and append to the back of the list for each dependent path. If 
// ChangedPaths is a vector, each dependent path will be appended to
// the vector and extraData is ignored.
template <class ChangedPaths, class... ExtraData>
static void
_AddDependentPaths(const SdfLayerHandle &layer, const SdfPath &path,
                   const PcpCache &cache,
                   ChangedPaths *changedPaths,
                   const ExtraData&... extraData)
{
    // We include virtual dependencies so that we can process
    // changes like adding missing defaultPrim metadata.
    const PcpDependencyFlags depTypes =
        PcpDependencyTypeDirect
        | PcpDependencyTypeAncestral
        | PcpDependencyTypeNonVirtual
        | PcpDependencyTypeVirtual;

    // Do not filter dependencies against the indexes cached in PcpCache,
    // because Usd does not cache PcpPropertyIndex entries.
    const bool filterForExistingCachesOnly = false;

    // If this site is in the cache's layerStack, we always add it here.
    // We do this instead of including PcpDependencyTypeRoot in depTypes
    // because we do not want to include root deps on those sites, just
    // the other kinds of inbound deps.
    if (cache.GetLayerStack()->HasLayer(layer)) {
        const SdfPath depPath = path.StripAllVariantSelections();
        _AddToChangedPaths(changedPaths, depPath, extraData...);
    }

    for (const PcpDependency& dep:
         cache.FindSiteDependencies(layer, path, depTypes,
                                    /* recurseOnSite */ true,
                                    /* recurseOnIndex */ false,
                                    filterForExistingCachesOnly)) {
        _AddToChangedPaths(changedPaths, dep.indexPath, extraData...);
    }

    TF_DEBUG(USD_CHANGES).Msg(
        "Adding paths that use <%s> in layer @%s@: %s\n",
        path.GetText(),
        layer->GetIdentifier().c_str(),
        _Stringify(*changedPaths).c_str());
}

// Removes all elements from changedPaths whose paths are prefixed 
// by other elements.
template <class ChangedPaths>
static void
_RemoveDescendentEntries(ChangedPaths *changedPaths)
{
    for (auto it = changedPaths->begin(); it != changedPaths->end(); ) {
        auto prefixedIt = it;
        ++prefixedIt;

        auto prefixedEndIt = prefixedIt;
        for (; prefixedEndIt != changedPaths->end()
               && prefixedEndIt->first.HasPrefix(it->first); ++prefixedEndIt)
            { }

        changedPaths->erase(prefixedIt, prefixedEndIt);
        ++it;
    }
}

// Removes all elements from weaker whose paths are prefixed by other
// elements in stronger. If elements with the same path exist in both
// weaker and stronger, merges those elements into stronger and removes
// the element from weaker. Assumes that stronger has no elements
// whose paths are prefixed by other elements in stronger.
template <class ChangedPaths>
static void
_MergeAndRemoveDescendentEntries(ChangedPaths *stronger, ChangedPaths *weaker)
{
    // We may be removing entries from weaker, and depending on the
    // concrete type of ChangedPaths that may invalidate iterators. So don't
    // cache the end iterator here.
    auto weakIt = weaker->begin();

    auto strongIt = stronger->begin();
    const auto strongEndIt = stronger->end();

    while (strongIt != strongEndIt && weakIt != weaker->end()) {
        if (weakIt->first < strongIt->first) {
            // If the current element in weaker is less than the current element
            // in stronger, it cannot be prefixed, so retain it.
            ++weakIt;
        } else if (weakIt->first == strongIt->first) {
            // If the same path exists in both weaker and stronger, merge the
            // weaker entry into stronger, then remove it from weaker.
            strongIt->second.insert(strongIt->second.end(),
                weakIt->second.begin(), weakIt->second.end());
            weakIt = weaker->erase(weakIt);
        } else if (weakIt->first.HasPrefix(strongIt->first)) {
            // Otherwise if this element in weaker is prefixed by the current
            // element in stronger, discard it. 
            //
            // Note that if stronger was allowed to have elements that were
            // prefixed by other elements in stronger, this would not be 
            // correct, since stronger could have an exact match for this
            // path, which we'd need to merge.
            weakIt = weaker->erase(weakIt);
        } else {
            // Otherwise advance to the next element in stronger.
            ++strongIt;
        }
    }
}

void
UsdStage::_HandleLayersDidChange(
    const SdfNotice::LayersDidChangeSentPerLayer &n)
{
    TfAutoMallocTag2 tag("Usd", _mallocTagID);

    // Ignore if this is not the round of changes we're looking for.
    size_t serial = n.GetSerialNumber();
    if (serial == _lastChangeSerialNumber)
        return;

    if (ARCH_UNLIKELY(serial < _lastChangeSerialNumber)) {
        // If we receive a change from an earlier round of change processing
        // than one we've already seen, there must be a violation of the Usd
        // threading model -- concurrent edits to layers that apply to a single
        // stage are disallowed.
        TF_CODING_ERROR("Detected usd threading violation.  Concurrent changes "
                        "to layer(s) composed in stage %p rooted at @%s@.  "
                        "(serial=%zu, lastSerial=%zu).",
                        this, GetRootLayer()->GetIdentifier().c_str(),
                        serial, _lastChangeSerialNumber);
        return;
    }

    _lastChangeSerialNumber = serial;

    TF_DEBUG(USD_CHANGES).Msg("\nHandleLayersDidChange received\n");

    // Keep track of paths to USD objects that need to be recomposed or
    // have otherwise changed.
    using _PathsToChangesMap = UsdNotice::ObjectsChanged::_PathsToChangesMap;
    _PathsToChangesMap recomposeChanges, otherResyncChanges, otherInfoChanges;

    SdfPathVector changedActivePaths;

    // Add dependent paths for any PrimSpecs whose fields have changed that may
    // affect cached prim information.
    for(const auto& layerAndChangelist : n.GetChangeListMap()) {
        // If this layer does not pertain to us, skip.
        if (_cache->FindAllLayerStacksUsingLayer(
                layerAndChangelist.first).empty()) {
            continue;
        }

        for (const auto& entryList : layerAndChangelist.second.GetEntryList()) {

            const SdfPath &path = entryList.first;
            const SdfChangeList::Entry &entry = entryList.second;

            // Skip target paths entirely -- we do not create target objects in
            // USD.
            if (path.IsTargetPath())
                continue;

            TF_DEBUG(USD_CHANGES).Msg(
                "<%s> in @%s@ changed.\n",
                path.GetText(), 
                layerAndChangelist.first->GetIdentifier().c_str());

            bool willRecompose = false;
            if (path == SdfPath::AbsoluteRootPath() ||
                path.IsPrimOrPrimVariantSelectionPath()) {

                bool didChangeActive = false;
                for (const auto& info : entry.infoChanged) {
                    if (info.first == SdfFieldKeys->Active) {
                        TF_DEBUG(USD_CHANGES).Msg(
                            "Changed field: %s\n", info.first.GetText());
                        didChangeActive = true;
                        break;
                    }
                }

                if (didChangeActive || entry.flags.didReorderChildren) {
                    willRecompose = true;
                } else {
                    for (const auto& info : entry.infoChanged) {
                        const auto& infoKey = info.first;
                        if (infoKey == SdfFieldKeys->Kind ||
                            infoKey == SdfFieldKeys->TypeName ||
                            infoKey == SdfFieldKeys->Specifier ||
                            
                            // XXX: Could be more specific when recomposing due
                            //      to clip changes. E.g., only update the clip
                            //      resolver and bits on each prim.
                            UsdIsClipRelatedField(infoKey)) {

                            TF_DEBUG(USD_CHANGES).Msg(
                                "Changed field: %s\n", infoKey.GetText());

                            willRecompose = true;
                            break;
                        }
                    }
                }

                if (willRecompose) {
                    _AddDependentPaths(layerAndChangelist.first, path, 
                                        *_cache, &recomposeChanges, &entry);
                }
                if (didChangeActive) {
                    _AddDependentPaths(layerAndChangelist.first, path, 
                                       *_cache, &changedActivePaths);
                }
            }
            else {
                willRecompose = path.IsPropertyPath() &&
                    (entry.flags.didAddPropertyWithOnlyRequiredFields ||
                     entry.flags.didAddProperty ||
                     entry.flags.didRemovePropertyWithOnlyRequiredFields ||
                     entry.flags.didRemoveProperty);

                if (willRecompose) {
                    _AddDependentPaths(layerAndChangelist.first, path, 
                                       *_cache, &otherResyncChanges, &entry);
                }
            }

            // If we're not going to recompose this path, record the dependent
            // scene paths separately so we can notify clients about the
            // changes.
            if (!willRecompose) {
                _AddDependentPaths(layerAndChangelist.first, path, 
                                  *_cache, &otherInfoChanges, &entry);
            }
        }
    }

    PcpChanges changes;
    changes.DidChange(std::vector<PcpCache*>(1, _cache.get()),
                      n.GetChangeListMap());

    // Pcp does not consider activation changes to be significant since
    // it doesn't look at activation during composition. However, UsdStage
    // needs to do so, since it elides children of deactivated prims.
    // This ensures that prim indexes for these prims are ejected from
    // the PcpCache.
    for (const SdfPath& p : changedActivePaths) {
        changes.DidChangeSignificantly(_cache.get(), p);
    }

    _Recompose(changes, &recomposeChanges);

    // Filter out all changes to objects beneath instances and remap
    // them to the corresponding object in the instance's master. Do this
    // after _Recompose so that the instancing cache is up-to-date.
    auto remapChangesToMasters = [this](_PathsToChangesMap* changes) {
        std::vector<_PathsToChangesMap::value_type> masterChanges;
        for (auto it = changes->begin(); it != changes->end(); ) {
            if (_IsObjectDescendantOfInstance(it->first)) {
                const SdfPath primIndexPath = 
                    it->first.GetAbsoluteRootOrPrimPath();
                for (const SdfPath& pathInMaster :
                     _instanceCache->GetPrimsInMastersUsingPrimIndexPath(
                         primIndexPath)) {
                    masterChanges.emplace_back(
                        it->first.ReplacePrefix(primIndexPath, pathInMaster), 
                        it->second);
                }
                it = changes->erase(it);
                continue;
            }
            ++it;
        }

        for (const auto& entry : masterChanges) {
            auto& value = (*changes)[entry.first];
            value.insert(value.end(), entry.second.begin(), entry.second.end());
        }
    };

    remapChangesToMasters(&recomposeChanges);
    remapChangesToMasters(&otherResyncChanges);
    remapChangesToMasters(&otherInfoChanges);

    // Add in all other paths that are marked as resynced.
    if (recomposeChanges.empty()) {
        recomposeChanges.swap(otherResyncChanges);
    }
    else {
        _RemoveDescendentEntries(&recomposeChanges);
        _MergeAndRemoveDescendentEntries(&recomposeChanges, &otherResyncChanges);
        for (auto& entry : otherResyncChanges) {
            recomposeChanges[entry.first] = std::move(entry.second);
        }
    }

    // Collect the paths in otherChangedPaths that aren't under paths that
    // were recomposed.  If the pseudo-root had been recomposed, we can
    // just clear out otherChangedPaths since everything was recomposed.
    if (!recomposeChanges.empty() &&
        recomposeChanges.begin()->first == SdfPath::AbsoluteRootPath()) {
        // If the pseudo-root is present, it should be the only path in the
        // changes.
        TF_VERIFY(recomposeChanges.size() == 1);
        otherInfoChanges.clear();
    }

    // Now we want to remove all elements of otherInfoChanges that are
    // prefixed by elements in recomposeChanges or beneath instances.
    _MergeAndRemoveDescendentEntries(&recomposeChanges, &otherInfoChanges);

    UsdStageWeakPtr self(this);

    // Notify about changed objects.
    UsdNotice::ObjectsChanged(
        self, &recomposeChanges, &otherInfoChanges).Send(self);

    // Receivers can now refresh their caches... or just dirty them
    UsdNotice::StageContentsChanged(self).Send(self);
}

void 
UsdStage::_Recompose(const PcpChanges &changes)
{
    using _PathsToChangesMap = UsdNotice::ObjectsChanged::_PathsToChangesMap;
    _Recompose(changes, (_PathsToChangesMap*)nullptr);
}

template <class T>
void 
UsdStage::_Recompose(const PcpChanges &changes,
                     T *initialPathsToRecompose)
{
    T newPathsToRecompose;
    T *pathsToRecompose = initialPathsToRecompose ?
        initialPathsToRecompose : &newPathsToRecompose;

    _RecomposePrims(changes, pathsToRecompose);

    // Update layer change notice listeners if changes may affect
    // the set of used layers.
    bool changedUsedLayers = !pathsToRecompose->empty();
    if (!changedUsedLayers) {
        const PcpChanges::LayerStackChanges& layerStackChanges = 
            changes.GetLayerStackChanges();
        for (const auto& entry : layerStackChanges) {
            if (entry.second.didChangeLayers ||
                entry.second.didChangeSignificantly) {
                changedUsedLayers = true;
                break;
            }
        }
    }

    if (changedUsedLayers) {
        _RegisterPerLayerNotices();
    }
}

template <class T>
void 
UsdStage::_RecomposePrims(const PcpChanges &changes,
                          T *pathsToRecompose)
{
    changes.Apply();

    // Process layer stack changes.
    //
    // Pcp recomputes layer stacks immediately upon the call to 
    // PcpChanges::Apply, which causes composition errors that occur
    // during this process to not be reported in _ComposePrimIndexesInParallel.
    // Walk through all modified layer stacks and report their errors here.
    const PcpChanges::LayerStackChanges &layerStackChanges = 
        changes.GetLayerStackChanges();

    for (const auto& layerStackChange : layerStackChanges) {
        const PcpLayerStackPtr& layerStack = layerStackChange.first;
        const PcpErrorVector& errors = layerStack->GetLocalErrors();
        if (!errors.empty()) {
            _ReportPcpErrors(errors, "Recomposing stage");
        }
    }

    // Process composed prim changes.
    const PcpChanges::CacheChanges &cacheChanges = changes.GetCacheChanges();
    if (!cacheChanges.empty()) {
        const PcpCacheChanges &ourChanges = cacheChanges.begin()->second;

        for (const auto& path : ourChanges.didChangeSignificantly) {
            (*pathsToRecompose)[path];
            TF_DEBUG(USD_CHANGES).Msg("Did Change Significantly: %s\n",
                                      path.GetText());
        }

        for (const auto& path : ourChanges.didChangePrims) {
            (*pathsToRecompose)[path];
            TF_DEBUG(USD_CHANGES).Msg("Did Change Prim: %s\n",
                                      path.GetText());
        }

    } else {
        TF_DEBUG(USD_CHANGES).Msg("No cache changes\n");
    }

    if (pathsToRecompose->empty()) {
        TF_DEBUG(USD_CHANGES).Msg("Nothing to recompose in cache changes\n");
        return;
    }

    // Prune descendant paths.
    _RemoveDescendentEntries(pathsToRecompose);

    // Invalidate the clip cache, but keep the clips alive for the duration
    // of recomposition in the (likely) case that clip data hasn't changed
    // and the underlying clip layer can be reused.
    Usd_ClipCache::Lifeboat clipLifeboat;
    for (const auto& entry : *pathsToRecompose) {
        _clipCache->InvalidateClipsForPrim(entry.first, &clipLifeboat);
    }

    // Ask Pcp to compute all the prim indexes in parallel, stopping at
    // stuff that's not active.
    SdfPathVector primPathsToRecompose;
    primPathsToRecompose.reserve(pathsToRecompose->size());
    for (const auto& entry : *pathsToRecompose) {
        const SdfPath& path = entry.first;
        if (!path.IsAbsoluteRootOrPrimPath() ||
            path.ContainsPrimVariantSelection()) {
            continue;
        }

        // Instance prims don't expose any name children, so we don't
        // need to recompose any prim index beneath instance prim 
        // indexes *unless* they are being used as the source index
        // for a master.
        if (_instanceCache->IsPathDescendantToAnInstance(path)) {
            const bool primIndexUsedByMaster = 
                _instanceCache->MasterUsesPrimIndexPath(path);
            if (!primIndexUsedByMaster) {
                TF_DEBUG(USD_CHANGES).Msg(
                    "Ignoring elided prim <%s>\n", path.GetText());
                continue;
            }
        }

        // Unregister all instances beneath the given path. This
        // allows us to determine which instance prim indexes are
        // no longer present and make the appropriate instance
        // changes during prim index composition below.
        _instanceCache->UnregisterInstancePrimIndexesUnder(path);

        primPathsToRecompose.push_back(path);
    }

    ArResolverScopedCache resolverCache;
    Usd_InstanceChanges instanceChanges;
    _ComposePrimIndexesInParallel(
        primPathsToRecompose, _IncludeNewPayloadsIfAncestorWasIncluded,
        "recomposing stage", &instanceChanges);
    
    // Determine what instance master prims on this stage need to
    // be recomposed due to instance prim index changes.
    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash> _MasterToPrimIndexMap;
    _MasterToPrimIndexMap masterToPrimIndexMap;

    const size_t origNumPathsToRecompose = pathsToRecompose->size();
    for (const auto& entry : *pathsToRecompose) {
        const SdfPath& path = entry.first;
        for (const SdfPath& masterPath :
                 _instanceCache->GetPrimsInMastersUsingPrimIndexPath(path)) {
            masterToPrimIndexMap[masterPath] = path;
            (*pathsToRecompose)[masterPath];
        }
    }

    for (size_t i = 0; i != instanceChanges.newMasterPrims.size(); ++i) {
        masterToPrimIndexMap[instanceChanges.newMasterPrims[i]] =
            instanceChanges.newMasterPrimIndexes[i];
        (*pathsToRecompose)[instanceChanges.newMasterPrims[i]];
    }

    for (size_t i = 0; i != instanceChanges.changedMasterPrims.size(); ++i) {
        masterToPrimIndexMap[instanceChanges.changedMasterPrims[i]] =
            instanceChanges.changedMasterPrimIndexes[i];
        (*pathsToRecompose)[instanceChanges.changedMasterPrims[i]];
    }

    if (pathsToRecompose->size() != origNumPathsToRecompose) {
        _RemoveDescendentEntries(pathsToRecompose);
    }

    std::vector<Usd_PrimDataPtr> subtreesToRecompose;
    _ComputeSubtreesToRecompose(
        make_transform_iterator(pathsToRecompose->begin(), TfGet<0>()),
        make_transform_iterator(pathsToRecompose->end(), TfGet<0>()),
        &subtreesToRecompose);

    // Recompose subtrees.
    if (masterToPrimIndexMap.empty()) {
        _ComposeSubtreesInParallel(subtreesToRecompose);
    }
    else {
        // Make sure we remove any subtrees for master prims that would
        // be composed when an instance subtree is composed. Otherwise,
        // the same master subtree could be composed concurrently, which
        // is unsafe.
        _RemoveMasterSubtreesSubsumedByInstances(
            &subtreesToRecompose, masterToPrimIndexMap);

        SdfPathVector primIndexPathsForSubtrees;
        primIndexPathsForSubtrees.reserve(subtreesToRecompose.size());
        for (const auto& prim : subtreesToRecompose) {
            primIndexPathsForSubtrees.push_back(TfMapLookupByValue(
                masterToPrimIndexMap, prim->GetPath(), prim->GetPath()));
        }
        _ComposeSubtreesInParallel(
            subtreesToRecompose, &primIndexPathsForSubtrees);
    }

    // Destroy dead master subtrees, making sure to record them in
    // paths to recompose for notifications.
    for (const SdfPath& p : instanceChanges.deadMasterPrims) {
        (*pathsToRecompose)[p];
    }
    _DestroyPrimsInParallel(instanceChanges.deadMasterPrims);

    // If the instancing changes produced old/new associated indexes, we need to
    // square up payload inclusion, and recurse.
    if (!instanceChanges.associatedIndexOld.empty()) {

        // Walk the old and new, and if the old has payloads included strictly
        // descendent to the old path, find the equivalent relative path on the
        // new and include that payload.
        SdfPathSet curLoadSet = _cache->GetIncludedPayloads();
        SdfPathSet newPayloads;

        for (size_t i = 0;
             i != instanceChanges.associatedIndexOld.size(); ++i) {
            SdfPath const &oldPath = instanceChanges.associatedIndexOld[i];
            SdfPath const &newPath = instanceChanges.associatedIndexNew[i];
            for (auto iter = curLoadSet.lower_bound(oldPath);
                 iter != curLoadSet.end() && iter->HasPrefix(oldPath); ++iter) {
                if (*iter == oldPath)
                    continue;
                SdfPath payloadPath = iter->ReplacePrefix(oldPath, newPath);
                newPayloads.insert(payloadPath);
                TF_DEBUG(USD_INSTANCING).Msg(
                    "Including equivalent payload <%s> -> <%s> for instancing "
                    "changes.\n",
                    iter->GetText(), payloadPath.GetText());
            }
        }
        if (!newPayloads.empty()) {
            // Request payloads and recurse.
            PcpChanges pcpChanges;
            _cache->RequestPayloads(newPayloads, SdfPathSet(), &pcpChanges);
            T toRecompose;
            _RecomposePrims(pcpChanges, &toRecompose);
            pathsToRecompose->insert(toRecompose.begin(), toRecompose.end());
        }
    }
}

template <class PrimIndexPathMap>
void
UsdStage::_RemoveMasterSubtreesSubsumedByInstances(
    std::vector<Usd_PrimDataPtr>* subtreesToRecompose,
    const PrimIndexPathMap& primPathToSourceIndexPathMap) const
{
    TRACE_FUNCTION();

    // Partition so [masterIt, subtreesToRecompose->end()) contains all 
    // subtrees for master prims.
    auto masterIt = std::partition(
        subtreesToRecompose->begin(), subtreesToRecompose->end(),
        [](const Usd_PrimDataPtr& p) { return !p->IsMaster(); });

    if (masterIt == subtreesToRecompose->end()) {
        return;
    }

    // Collect the paths for all master subtrees that will be composed when
    // the instance subtrees in subtreesToRecompose are composed. 
    // See the instancing handling in _ComposeChildren.
    using _PathSet = TfHashSet<SdfPath, SdfPath::Hash>;
    std::unique_ptr<_PathSet> mastersForSubtrees;
    for (const Usd_PrimDataPtr& p : 
         boost::make_iterator_range(subtreesToRecompose->begin(), masterIt)) {

        const SdfPath* sourceIndexPath = 
            TfMapLookupPtr(primPathToSourceIndexPathMap, p->GetPath());
        const SdfPath& masterPath = 
            _instanceCache->GetMasterUsingPrimIndexPath(
                sourceIndexPath ? *sourceIndexPath : p->GetPath());
        if (!masterPath.IsEmpty()) {
            if (!mastersForSubtrees) {
                mastersForSubtrees.reset(new _PathSet);
            }
            mastersForSubtrees->insert(masterPath);
        }
    }

    if (!mastersForSubtrees) {
        return;
    }

    // Remove all master prim subtrees that will get composed when an 
    // instance subtree in subtreesToRecompose is composed.
    auto masterIsSubsumedByInstanceSubtree = 
        [&mastersForSubtrees](const Usd_PrimDataPtr& master) {
        return mastersForSubtrees->find(master->GetPath()) != 
               mastersForSubtrees->end();
    };

    subtreesToRecompose->erase(
        std::remove_if(
            masterIt, subtreesToRecompose->end(),
            masterIsSubsumedByInstanceSubtree),
        subtreesToRecompose->end());
}

template <class Iter>
void 
UsdStage::_ComputeSubtreesToRecompose(
    Iter i, Iter end,
    std::vector<Usd_PrimDataPtr>* subtreesToRecompose)
{
    subtreesToRecompose->reserve(
        subtreesToRecompose->size() + std::distance(i, end));

    while (i != end) {
        TF_DEBUG(USD_CHANGES).Msg("Recomposing: %s\n", i->GetText());
        // TODO: refactor into shared method
        // We only care about recomposing prim-like things
        // so avoid recomposing anything else.
        if (!i->IsAbsoluteRootOrPrimPath() ||
            i->ContainsPrimVariantSelection()) {
            TF_DEBUG(USD_CHANGES).Msg("Skipping non-prim: %s\n",
                                      i->GetText());
            ++i;
            continue;
        }

        SdfPath const &parentPath = i->GetParentPath();
        PathToNodeMap::const_iterator parentIt = _primMap.find(parentPath);
        if (parentIt != _primMap.end()) {

            // Since our input range contains no descendant paths, siblings
            // must appear consecutively.  We want to process all siblings that
            // have changed together in order to only recompose the parent's
            // list of children once.  We scan forward while the paths share a
            // parent to find the range of siblings.

            // Recompose parent's list of children.
            auto parent = parentIt->second.get();
            _ComposeChildren(parent,
                             parent->IsInMaster() ? nullptr : &_populationMask,
                             /*recurse=*/false);

            // Recompose the subtree for each affected sibling.
            do {
                PathToNodeMap::const_iterator primIt = _primMap.find(*i);
                if (primIt != _primMap.end())
                    subtreesToRecompose->push_back(primIt->second.get());
                ++i;
            } while (i != end && i->GetParentPath() == parentPath);
        } else if (parentPath.IsEmpty()) {
            // This is the pseudo root, so we need to blow and rebuild
            // everything.
            subtreesToRecompose->push_back(_pseudoRoot);
            ++i;
        } else {
            ++i;
        }
    }
}

struct UsdStage::_IncludeNewlyDiscoveredPayloadsPredicate
{
    explicit _IncludeNewlyDiscoveredPayloadsPredicate(UsdStage const *stage)
        : _stage(stage) {}

    bool operator()(SdfPath const &path) const {
        // We want to include newly discovered payloads on existing prims or on
        // new prims if their nearest loadable ancestor was loaded, or if there
        // is no nearest loadable ancestor and the stage was initially populated
        // with LoadAll.

        // First, check to see if this payload is new to us.  This is safe to do
        // concurrently without a lock since these are only ever reads.

        // The path we're given is a prim index path.  Due to instancing, the
        // path to the corresponding prim on the stage may differ (it may be a
        // generated master path).
        SdfPath stagePath = _stage->_GetPrimPathUsingPrimIndexAtPath(path);
        if (stagePath.IsEmpty())
            stagePath = path;

        UsdPrim prim = _stage->GetPrimAtPath(stagePath);
        bool isNewPayload = !prim || !prim.HasAuthoredPayloads();

        if (!isNewPayload)
            return false;

        // XXX: This does not quite work correctly with instancing.  What we
        // need to do is once we hit a master, continue searching ancestors of
        // all instances that use it.  If we find *any* nearest ancestor that's
        // loadable, we should return true.

        // This is a new payload -- find the nearest ancestor with a payload.
        // First walk up by path until we find an existing prim.
        if (prim) {
            prim = prim.GetParent();
        }
        else {
            for (SdfPath curPath = stagePath.GetParentPath(); !prim;
                 curPath = curPath.GetParentPath()) {
                prim = _stage->GetPrimAtPath(curPath);
            }
        }

        UsdPrim root = _stage->GetPseudoRoot();
        for (; !prim.HasAuthoredPayloads() && prim != root; 
             prim = prim.GetParent()) {
            // continue
        }

        // If we hit the root, then consult the initial population state.
        if (prim == root) {
            return _stage->_initialLoadSet == LoadAll;
        }

        // Otherwise load the payload if the ancestor is loaded, or if it
        // was formerly active=false.  In that case we only populate indexes
        // descendant to it because it has become active=true, so we should
        // include the payload in that case too.
        return prim.IsLoaded() || !prim.IsActive();
    }

    UsdStage const *_stage;
};

void 
UsdStage::_ComposePrimIndexesInParallel(
    const std::vector<SdfPath>& primIndexPaths,
    _IncludePayloadsRule includeRule,
    const std::string& context,
    Usd_InstanceChanges* instanceChanges)
{
    if (TfDebug::IsEnabled(USD_COMPOSITION)) {
        // Ensure not too much spew if primIndexPaths is big.
        constexpr size_t maxPaths = 16;
        std::vector<SdfPath> dbgPaths(
            primIndexPaths.begin(),
            primIndexPaths.begin() + std::min(maxPaths, primIndexPaths.size()));
        string msg = TfStringPrintf(
            "Composing prim indexes: %s%s\n",
            TfStringify(dbgPaths).c_str(), primIndexPaths.size() > maxPaths ?
            TfStringPrintf(
                " (and %zu more)", primIndexPaths.size() - maxPaths).c_str() :
            "");
        TF_DEBUG(USD_COMPOSITION).Msg("%s", msg.c_str());
    }

    // We only want to compute prim indexes included by the stage's 
    // population mask. As an optimization, if all prims are included the 
    // name children predicate doesn't need to consider the mask at all.
    static auto allMask = UsdStagePopulationMask::All();
    const UsdStagePopulationMask* mask = 
        _populationMask == allMask ? nullptr : &_populationMask;

    // Ask Pcp to compute all the prim indexes in parallel, stopping at
    // prim indexes that won't be used by the stage.
    PcpErrorVector errs;

    if (includeRule == _IncludeAllDiscoveredPayloads) {
        _cache->ComputePrimIndexesInParallel(
            primIndexPaths, &errs, 
            _NameChildrenPred(mask, _instanceCache.get()),
            [](const SdfPath &) { return true; },
            "Usd", _mallocTagID);
    }
    else if (includeRule == _IncludeNoDiscoveredPayloads) {
        _cache->ComputePrimIndexesInParallel(
            primIndexPaths, &errs, 
            _NameChildrenPred(mask, _instanceCache.get()),
            [](const SdfPath &) { return false; },
            "Usd", _mallocTagID);
    }
    else if (includeRule == _IncludeNewPayloadsIfAncestorWasIncluded) {
        _cache->ComputePrimIndexesInParallel(
            primIndexPaths, &errs, 
            _NameChildrenPred(mask, _instanceCache.get()),
            _IncludeNewlyDiscoveredPayloadsPredicate(this),
            "Usd", _mallocTagID);
    }

    if (!errs.empty()) {
        _ReportPcpErrors(errs, context);
    }

    // Process instancing changes due to new or changed instanceable
    // prim indexes discovered during composition.
    Usd_InstanceChanges changes;
    _instanceCache->ProcessChanges(&changes);

    if (instanceChanges) {
        instanceChanges->AppendChanges(changes);
    }

    // After processing changes, we may discover that some master prims
    // need to change their source prim index. This may be because their
    // previous source prim index was destroyed or was no longer an
    // instance. Compose the new source prim indexes.
    if (!changes.changedMasterPrims.empty()) {
        _ComposePrimIndexesInParallel(
            changes.changedMasterPrimIndexes, includeRule,
            context, instanceChanges);
    }
}

void
UsdStage::_RegisterPerLayerNotices()
{
    // The goal is to update _layersAndNoticeKeys so it reflects the current
    // cache's set of used layers (from GetUsedLayers()).  We want to avoid
    // thrashing the TfNotice registrations since we expect that usually only a
    // relatively small subset of used layers will change, if any.
    //
    // We walk both the current _layersAndNoticeKeys and the cache's
    // GetUsedLayers, and incrementally update, TfNotice::Revoke()ing any layers
    // we no longer use, TfNotice::Register()ing for new layers we didn't use
    // previously, and leaving alone those layers that remain.  The linear walk
    // works because the PcpCache::GetUsedLayers() returns a std::set, so we
    // always retain things in a stable order.

    SdfLayerHandleSet usedLayers = _cache->GetUsedLayers();

    SdfLayerHandleSet::const_iterator
        usedLayersIter = usedLayers.begin(),
        usedLayersEnd = usedLayers.end();

    _LayerAndNoticeKeyVec::iterator
        layerAndKeyIter = _layersAndNoticeKeys.begin(),
        layerAndKeyEnd = _layersAndNoticeKeys.end();

    // We'll build a new vector and swap it into place at the end.  We can
    // preallocate space upfront since we know the resulting size will be
    // exactly the size of usedLayers.
    _LayerAndNoticeKeyVec newLayersAndNoticeKeys;
    newLayersAndNoticeKeys.reserve(usedLayers.size());
    
    UsdStagePtr self(this);

    while (usedLayersIter != usedLayersEnd ||
           layerAndKeyIter != layerAndKeyEnd) {

        // There are three cases to consider: a newly added layer, a layer no
        // longer used, or a layer that we used before and continue to use.
        if (layerAndKeyIter == layerAndKeyEnd ||
            (usedLayersIter != usedLayersEnd  &&
             *usedLayersIter < layerAndKeyIter->first)) {
            // This is a newly added layer.  Register for the notice and add it.
            newLayersAndNoticeKeys.push_back(
                make_pair(*usedLayersIter,
                          TfNotice::Register(
                              self, &UsdStage::_HandleLayersDidChange,
                              *usedLayersIter)));
            ++usedLayersIter;
        } else if (usedLayersIter == usedLayersEnd    ||
                   (layerAndKeyIter != layerAndKeyEnd &&
                    layerAndKeyIter->first < *usedLayersIter)) {
            // This is a layer we no longer use, unregister and skip over.
            TfNotice::Revoke(layerAndKeyIter->second);
            ++layerAndKeyIter;
        } else {
            // This is a layer we had before and still have, just copy it over.
            newLayersAndNoticeKeys.push_back(*layerAndKeyIter);
            ++layerAndKeyIter, ++usedLayersIter;
        }
    }

    // Swap new set into place.
    _layersAndNoticeKeys.swap(newLayersAndNoticeKeys);
}

SdfPrimSpecHandle
UsdStage::_GetPrimSpec(const SdfPath& path)
{
    return GetEditTarget().GetPrimSpecForScenePath(path);
}

SdfSpecType
UsdStage::_GetDefiningSpecType(Usd_PrimDataConstPtr primData,
                               const TfToken& propName) const
{
    if (!TF_VERIFY(primData) || !TF_VERIFY(!propName.IsEmpty()))
        return SdfSpecTypeUnknown;

    // Check for a spec type in the definition registry, in case this is a
    // builtin property.
    SdfSpecType specType =
        UsdSchemaRegistry::GetSpecType(primData->GetTypeName(), propName);

    if (specType != SdfSpecTypeUnknown)
        return specType;

    // Otherwise look for the strongest authored property spec.
    Usd_Resolver res(&primData->GetPrimIndex(), /*skipEmptyNodes=*/true);
    SdfPath curPath;
    bool curPathValid = false;
    while (res.IsValid()) {
        const SdfLayerRefPtr& layer = res.GetLayer();
        if (layer->HasSpec(SdfAbstractDataSpecId(&res.GetLocalPath()))) {
            if (!curPathValid) {
                curPath = res.GetLocalPath().AppendProperty(propName);
                curPathValid = true;
            }
            specType = layer->GetSpecType(SdfAbstractDataSpecId(&curPath));
            if (specType != SdfSpecTypeUnknown)
                return specType;
        }
        if (res.NextLayer())
            curPathValid = false;
    }

    // Unknown.
    return SdfSpecTypeUnknown;
}

// ------------------------------------------------------------------------- //
// Flatten & Export Utilities
// ------------------------------------------------------------------------- //

class Usd_FlattenAccess
{
public:
    static void GetAllMetadata(
        const UsdObject &obj, bool useFallbacks,
        UsdMetadataValueMap* resultMap, bool anchorAssetPathsOnly)
    {
        obj.GetStage()->_GetAllMetadata(
            obj, useFallbacks, resultMap, anchorAssetPathsOnly);
    }

    static void MakeResolvedAssetPaths(
        UsdTimeCode time, const UsdAttribute& attr,
        VtValue* value, bool anchorAssetPathsOnly)
    {
        attr.GetStage()->_MakeResolvedAssetPaths(
            time, attr, value, anchorAssetPathsOnly);
    }
};

namespace {

// Populates the time sample map with the resolved values for the given 
// attribute and returns true if time samples exist, false otherwise.
bool 
_GetTimeSampleMap(const UsdAttribute &attr, SdfTimeSampleMap *out,
                  const SdfLayerOffset& offset = SdfLayerOffset(),
                  bool anchorAssetPathsOnly = false)
{
    UsdAttributeQuery attrQuery(attr);

    std::vector<double> timeSamples;
    if (attrQuery.GetTimeSamples(&timeSamples)) {
        for (const auto& timeSample : timeSamples) {
            VtValue value;
            if (attrQuery.Get(&value, timeSample)) {
                Usd_FlattenAccess::MakeResolvedAssetPaths(
                    timeSample, attr, &value, anchorAssetPathsOnly);
                (*out)[offset * timeSample].Swap(value);
            }
            else {
                (*out)[offset * timeSample] = VtValue(SdfValueBlock());
            }
        }
        return true;
    }
    return false;
}

// Map from path to replacement for remapping target paths during flattening.
using _PathRemapping = std::map<SdfPath, SdfPath>;

// Apply path remappings to a list of target paths.
void
_RemapTargetPaths(SdfPathVector* targetPaths, 
                  const _PathRemapping& pathRemapping)
{
    if (pathRemapping.empty()) {
        return;
    }

    for (SdfPath& p : *targetPaths) {
        // XXX: This is not optimal; SdfPathFindLongestPrefix uses
        // std::lower_bound, which is linear instead of std::map::lower_bound,
        // which is logarithmic.
        auto it = SdfPathFindLongestPrefix(
            make_transform_iterator(pathRemapping.begin(), TfGet<0>()),
            make_transform_iterator(pathRemapping.end(), TfGet<0>()),
            p);
        if (it.base() != pathRemapping.end()) {
            p = p.ReplacePrefix(it.base()->first, it.base()->second);
        }
    }
}

// Remove any paths to master prims or descendants from given target paths
// for srcProp. Issues a warning if any paths were removed.
void
_RemoveMasterTargetPaths(const UsdProperty& srcProp, 
                         SdfPathVector* targetPaths)
{
    auto removeIt = std::remove_if(
        targetPaths->begin(), targetPaths->end(),
        Usd_InstanceCache::IsPathInMaster);
    if (removeIt == targetPaths->end()) {
        return;
    }

    TF_WARN(
        "Some %s paths from <%s> could not be flattened because "
        "they targeted objects within an instancing master.",
        srcProp.Is<UsdAttribute>() ? 
            "attribute connection" : "relationship target",
        srcProp.GetPath().GetText());

    targetPaths->erase(removeIt, targetPaths->end());
}

// We want to give generated masters in the flattened stage
// reserved(using '__' as a prefix), unclashing paths, however,
// we don't want to use the '__Master' paths which have special
// meaning to UsdStage. So we create a mapping between our generated
// 'Flattened_Master'-style paths and the '__Master' paths.
_PathRemapping
_GenerateFlattenedMasterPath(const std::vector<UsdPrim>& masters)
{
    size_t primMasterId = 1;

    const auto generatePathName = [&primMasterId]() {
        return SdfPath(TfStringPrintf("/Flattened_Master_%lu", 
                                      primMasterId++));
    };

    _PathRemapping masterToFlattened;

    for (auto const& masterPrim : masters) {
        SdfPath flattenedMasterPath;
        const auto masterPrimPath = masterPrim.GetPath();

        auto masterPathLookup = masterToFlattened.find(masterPrimPath);
        if (masterPathLookup == masterToFlattened.end()) {
            // We want to ensure that we don't clash with user
            // prims in the unlikely even they named it Flatten_xxx
            flattenedMasterPath = generatePathName();
            const auto stage = masterPrim.GetStage();
            while (stage->GetPrimAtPath(flattenedMasterPath)) {
                flattenedMasterPath = generatePathName();
            }
            masterToFlattened.emplace(masterPrimPath, flattenedMasterPath);
        } else {
            flattenedMasterPath = masterPathLookup->second;
        }     
    }

    return masterToFlattened;
}

void
_CopyMetadata(const SdfSpecHandle& dest, const UsdMetadataValueMap& metadata)
{
    // Copy each key/value into the Sdf spec.
    TfErrorMark m;
    vector<string> msgs;
    for (auto const& tokVal : metadata) {
        dest->SetInfo(tokVal.first, tokVal.second);
        if (!m.IsClean()) {
            msgs.clear();
            for (auto i = m.GetBegin(); i != m.GetEnd(); ++i) {
                msgs.push_back(i->GetCommentary());
            }
            m.Clear();
            TF_WARN("Failed copying metadata: %s", TfStringJoin(msgs).c_str());
        }
    }
}

void
_CopyAuthoredMetadata(const UsdObject &source, const SdfSpecHandle& dest)
{
    // GetAllMetadata returns all non-private metadata fields (it excludes
    // composition arcs and values), which is exactly what we want here.
    UsdMetadataValueMap metadata;
    Usd_FlattenAccess::GetAllMetadata(
        source, /* useFallbacks = */ false, &metadata,
        /* anchorAssetPathsOnly = */ true);

    _CopyMetadata(dest, metadata);
}

void
_CopyProperty(const UsdProperty &prop,
              const SdfPrimSpecHandle &dest, const TfToken &destName,
              const _PathRemapping &pathRemapping,
              const SdfLayerOffset &timeOffset)
{
    if (prop.Is<UsdAttribute>()) {
        UsdAttribute attr = prop.As<UsdAttribute>();
        
        if (!attr.GetTypeName()){
            TF_WARN("Attribute <%s> has unknown value type. " 
                    "It will be omitted from the flattened result.", 
                    attr.GetPath().GetText());
            return;
        }

        SdfAttributeSpecHandle sdfAttr = dest->GetAttributes()[destName];
        if (!sdfAttr) {
            sdfAttr = SdfAttributeSpec::New(dest, destName, attr.GetTypeName());
        }

        _CopyAuthoredMetadata(attr, sdfAttr);

        // Copy the default & time samples, if present. We get the
        // correct timeSamples/default value resolution here because
        // GetBracketingTimeSamples sets hasSamples=false when the
        // default value is stronger.

        double lower = 0.0, upper = 0.0;
        bool hasSamples = false;
        if (attr.GetBracketingTimeSamples(
            0.0, &lower, &upper, &hasSamples) && hasSamples) {
            SdfTimeSampleMap ts;
            if (_GetTimeSampleMap(attr, &ts, timeOffset, 
                                  /* anchorAssetPathsOnly = */ true)) {
                sdfAttr->SetInfo(SdfFieldKeys->TimeSamples, VtValue::Take(ts));
            }
        }
        if (attr.HasAuthoredMetadata(SdfFieldKeys->Default)) {
            VtValue defaultValue;
            if (attr.Get(&defaultValue)) {
                Usd_FlattenAccess::MakeResolvedAssetPaths(
                    UsdTimeCode::Default(), attr, &defaultValue, 
                    /* anchorAssetPathsOnly = */ true);
            }
            else {
                defaultValue = SdfValueBlock();
            }
            sdfAttr->SetInfo(SdfFieldKeys->Default, defaultValue);
        }
        SdfPathVector sources;
        attr.GetConnections(&sources);
        if (!sources.empty()) {
            _RemapTargetPaths(&sources, pathRemapping);
            _RemoveMasterTargetPaths(prop, &sources);
            sdfAttr->GetConnectionPathList().GetExplicitItems() = sources;
        }
     }
     else if (prop.Is<UsdRelationship>()) {
         UsdRelationship rel = prop.As<UsdRelationship>();
         // NOTE: custom = true by default for relationship, but the
         // SdfSchema fallback is false, so we must set it explicitly
         // here. The situation is similar for variability.
         SdfRelationshipSpecHandle sdfRel = dest->GetRelationships()[destName];
         if (!sdfRel){
             sdfRel = SdfRelationshipSpec::New(
                 dest, destName, /*custom*/ false, SdfVariabilityVarying);
         }

         _CopyAuthoredMetadata(rel, sdfRel);

         SdfPathVector targets;
         rel.GetTargets(&targets);
         if (!targets.empty()) {
             _RemapTargetPaths(&targets, pathRemapping);
             _RemoveMasterTargetPaths(prop, &targets);
             sdfRel->GetTargetPathList().GetExplicitItems() = targets;
         }
     }
}

void
_CopyPrim(const UsdPrim &usdPrim, 
          const SdfLayerHandle &layer, const SdfPath &path,
          const _PathRemapping &masterToFlattened)
{
    SdfPrimSpecHandle newPrim;
    
    if (!usdPrim.IsActive()) {
        return;
    }
    
    if (usdPrim.GetPath() == SdfPath::AbsoluteRootPath()) {
        newPrim = layer->GetPseudoRoot();
    } else {
        // Note that the true value for spec will be populated in _CopyMetadata
        newPrim = SdfPrimSpec::New(layer->GetPrimAtPath(path.GetParentPath()), 
                                   path.GetName(), SdfSpecifierOver, 
                                   usdPrim.GetTypeName());
    }

    if (usdPrim.IsInstance()) {
        const auto flattenedMasterPath = 
            masterToFlattened.at(usdPrim.GetMaster().GetPath());

        // Author an internal reference to our flattened master prim
        newPrim->GetReferenceList().Add(SdfReference(std::string(),
                                        flattenedMasterPath));
    }
    
    _CopyAuthoredMetadata(usdPrim, newPrim);

    // In the case of flattening clips, we may have builtin attributes which 
    // aren't declared in the static scene topology, but may have a value 
    // in some clips that we want to relay into the flattened result.
    // XXX: This should be removed if we fix GetProperties()
    // and GetAuthoredProperties to consider clips.
    auto hasValue = [](const UsdProperty& prop){
        return prop.Is<UsdAttribute>()
               && prop.As<UsdAttribute>().HasAuthoredValue();
    };
    
    for (auto const &prop : usdPrim.GetProperties()) {
        if (prop.IsAuthored() || hasValue(prop)) {
            _CopyProperty(prop, newPrim, prop.GetName(), masterToFlattened,
                          SdfLayerOffset());
        }
    }
}

void
_CopyMasterPrim(const UsdPrim &masterPrim,
                const SdfLayerHandle &destinationLayer,
                const _PathRemapping &masterToFlattened)
{
    const auto& flattenedMasterPath 
        = masterToFlattened.at(masterPrim.GetPath());

    for (UsdPrim child: UsdPrimRange::AllPrims(masterPrim)) {
        // We need to update the child path to use the Flatten name.
        const auto flattenedChildPath = child.GetPath().ReplacePrefix(
            masterPrim.GetPath(), flattenedMasterPath);

        _CopyPrim(child, destinationLayer, flattenedChildPath, 
                  masterToFlattened);
    }
}

bool
_IsPrivateFallbackFieldKey(const TfToken& fieldKey)
{
    // Consider documentation and comment fallbacks as private; these are
    // primarily for schema authors and are not expected to be authored 
    // in flattened results.
    if (fieldKey == SdfFieldKeys->Documentation ||
        fieldKey == SdfFieldKeys->Comment) {
        return true;
    }

    // Consider default value fallback as non-private, since we do write out
    // default values during flattening.
    if (fieldKey == SdfFieldKeys->Default) {
        return false;
    }

    return _IsPrivateFieldKey(fieldKey);
}

bool
_HasAuthoredValue(const TfToken& fieldKey, 
                  const SdfPropertySpecHandleVector& propStack)
{
    return std::any_of(
        propStack.begin(), propStack.end(),
        [&fieldKey](const SdfPropertySpecHandle& spec) {
            return spec->HasInfo(fieldKey);
        });
}

void
_CopyFallbacks(const SdfPropertySpecHandle &srcPropDef,
               const SdfPropertySpecHandle &dstPropDef,
               const SdfPropertySpecHandle &dstPropSpec,
               const SdfPropertySpecHandleVector &dstPropStack)
{
    if (!srcPropDef) {
        return;
    }

    std::vector<TfToken> fallbackFields = srcPropDef->ListFields();
    fallbackFields.erase(
        std::remove_if(fallbackFields.begin(), fallbackFields.end(),
                       _IsPrivateFallbackFieldKey),
        fallbackFields.end());

    UsdMetadataValueMap fallbacks;
    for (const auto& fieldName : fallbackFields) {
        // If the property spec already has a value for this field,
        // don't overwrite it with the fallback.
        if (dstPropSpec->HasField(fieldName)) {
            continue;
        }

        // If we're flattening over a builtin property and the
        // fallback for that property matches the source fallback
        // and there isn't an authored value that's overriding that
        // fallback, we don't need to write the fallback.
        VtValue fallbackVal = srcPropDef->GetField(fieldName);
        if (dstPropDef && dstPropDef->GetField(fieldName) == fallbackVal &&
            !_HasAuthoredValue(fieldName, dstPropStack)) {
                continue;
        }

        fallbacks[fieldName].Swap(fallbackVal);
    }

    _CopyMetadata(dstPropSpec, fallbacks);
}

} // end anonymous namespace

bool
UsdStage::ExportToString(std::string *result, bool addSourceFileComment) const
{
    SdfLayerRefPtr flatLayer = Flatten(addSourceFileComment);
    return flatLayer->ExportToString(result);
}

bool
UsdStage::Export(const std::string & newFileName, bool addSourceFileComment,
                 const SdfLayer::FileFormatArguments &args) const
{
    SdfLayerRefPtr flatLayer = Flatten(addSourceFileComment);
    return flatLayer->Export(newFileName, /* comment = */ std::string(), args);
}

SdfLayerRefPtr
UsdStage::Flatten(bool addSourceFileComment) const
{
    TRACE_FUNCTION();

    SdfLayerHandle rootLayer = GetRootLayer();
    SdfLayerRefPtr flatLayer = SdfLayer::CreateAnonymous(".usda");

    if (!TF_VERIFY(rootLayer)) {
        return TfNullPtr;
    }

    if (!TF_VERIFY(flatLayer)) {
        return TfNullPtr;
    }

    // Preemptively populate our mapping. This allows us to populate
    // nested instances in the destination layer much more simply.
    const auto masterToFlattened = _GenerateFlattenedMasterPath(GetMasters());

    // We author the master overs first to produce simpler 
    // assets which have them grouped at the top of the file.
    for (auto const& master : GetMasters()) {
        _CopyMasterPrim(master, flatLayer, masterToFlattened);
    }

    for (UsdPrim prim: UsdPrimRange::AllPrims(GetPseudoRoot())) {
        _CopyPrim(prim, flatLayer, prim.GetPath(), masterToFlattened);
    }

    if (addSourceFileComment) {
        std::string doc = flatLayer->GetDocumentation();

        if (!doc.empty()) {
            doc.append("\n\n");
        }

        doc.append(TfStringPrintf("Generated from Composed Stage "
                                  "of root layer %s\n",
                                  GetRootLayer()->GetRealPath().c_str()));

        flatLayer->SetDocumentation(doc);
    }

    return flatLayer;
}

UsdProperty 
UsdStage::_FlattenProperty(const UsdProperty &srcProp,
                           const UsdPrim &dstParent, const TfToken &dstName)
{
    if (!srcProp) {
        TF_CODING_ERROR("Cannot flatten invalid property <%s>", 
                        UsdDescribe(srcProp).c_str());
        return UsdProperty();
    }

    if (!dstParent) {
        TF_CODING_ERROR("Cannot flatten property <%s> to invalid %s",
                        UsdDescribe(srcProp).c_str(),
                        UsdDescribe(dstParent).c_str());
        return UsdProperty();
    }

    // Keep track of the pre-existing property stack for the destination
    // property if any -- we use this later to determine if we need to
    // stamp out the fallback values from the source property.
    SdfPropertySpecHandleVector dstPropStack;
    if (UsdProperty dstProp = dstParent.GetProperty(dstName)) {
        if ((srcProp.Is<UsdAttribute>() && !dstProp.Is<UsdAttribute>()) ||
            (srcProp.Is<UsdRelationship>() && !dstProp.Is<UsdRelationship>())) {
            TF_CODING_ERROR("Cannot flatten %s to %s because they are "
                            "different property types", 
                            UsdDescribe(srcProp).c_str(), 
                            UsdDescribe(dstProp).c_str());
            return UsdProperty();
        }

        dstPropStack = dstProp.GetPropertyStack();
    }

    {
        SdfChangeBlock block;

        SdfPrimSpecHandle primSpec = _CreatePrimSpecForEditing(dstParent);
        if (!primSpec) {
            // _CreatePrimSpecForEditing will have already issued any
            // coding errors, so just bail out.
            return UsdProperty();
        }

        if (SdfPropertySpecHandle dstPropSpec = 
                primSpec->GetProperties()[dstName]) {
            // Ignore the pre-existing property spec when determining
            // whether to stamp out fallback values.
            dstPropStack.erase(
                std::remove(dstPropStack.begin(), dstPropStack.end(), 
                            dstPropSpec), 
                dstPropStack.end());

            // Clear out the existing property spec unless we're flattening
            // over the source property. In that case, we don't want to
            // remove the property spec because its authored opinions should
            // be considered when flattening. This won't leave behind any
            // unwanted opinions since we'll be overwriting all of the
            // destination property spec's fields anyway in this case.
            const bool flatteningToSelf = 
                srcProp.GetPrim() == dstParent && srcProp.GetName() == dstName;
            if (!flatteningToSelf) {
                primSpec->RemoveProperty(dstPropSpec);
            }
        }

        // Set up a path remapping so that attribute connections or 
        // relationships targeting an object beneath the old parent prim
        // now target objects beneath the new parent prim.
        _PathRemapping remapping;
        if (srcProp.GetPrim() != dstParent) {
            remapping[srcProp.GetPrimPath()] = dstParent.GetPath();
        }

        // Apply offsets that affect the edit target to flattened time 
        // samples to ensure they resolve to the expected value.
        const SdfLayerOffset stageToLayerOffset = 
            UsdPrepLayerOffset(GetEditTarget().GetMapFunction().GetTimeOffset())
            .GetInverse();

        // Copy authored property values and metadata.
        _CopyProperty(srcProp, primSpec, dstName, remapping, stageToLayerOffset);

        SdfPropertySpecHandle dstPropSpec = 
            primSpec->GetProperties().get(dstName);
        if (!dstPropSpec) {
            return UsdProperty();
        }

        // Copy fallback property values and metadata if needed.
        _CopyFallbacks(
            _GetPropertyDefinition(srcProp.GetPrim(), srcProp.GetName()),
            _GetPropertyDefinition(dstParent, dstName),
            dstPropSpec, dstPropStack);
    }

    return dstParent.GetProperty(dstName);
}

const PcpPrimIndex*
UsdStage::_GetPcpPrimIndex(const SdfPath& primPath) const
{
    return _cache->FindPrimIndex(primPath);
}

// ========================================================================== //
//                               VALUE RESOLUTION                             //
// ========================================================================== //

//
// Helper template function for determining type names from arbitrary pointer
// types, which may include SdfAbstractDataValue and VtValue.
//
static const std::type_info &
_GetTypeid(const SdfAbstractDataValue *val) { return val->valueType; }

static const std::type_info &
_GetTypeid(const VtValue *val) { return val->GetTypeid(); }

template <class T, class Holder>
static bool _IsHolding(const Holder &holder) {
    return TfSafeTypeCompare(typeid(T), _GetTypeid(holder));
}

template <class T>
static const T &_UncheckedGet(const SdfAbstractDataValue *val) {
    return *static_cast<T const *>(val->value);
}
template <class T>
static const T &_UncheckedGet(const VtValue *val) {
    return val->UncheckedGet<T>();
}

template <class T>
void _UncheckedSwap(SdfAbstractDataValue *dv, T& val) {
    using namespace std;
    swap(*static_cast<T*>(dv->value), val);
}
template <class T>
void _UncheckedSwap(VtValue *value, T& val) {
    value->Swap(val);
}

template <class T>
static void
_Set(SdfAbstractDataValue *dv, T const &val) { dv->StoreValue(val); }
template <class T>
static void _Set(VtValue *value, T const &val) { *value = val; }

template <class Storage>
static void _ApplyLayerOffset(Storage storage,
                              const PcpNodeRef &node,
                              const SdfLayerRefPtr &layer)
{
    SdfLayerOffset offset = _GetLayerToStageOffset(node, layer);
    if (!offset.IsIdentity()) {
        const SdfTimeSampleMap &samples =
            _UncheckedGet<SdfTimeSampleMap>(storage);
        SdfTimeSampleMap transformed;
        for (const auto& sample : samples) {
            transformed[offset * sample.first] = sample.second;
        }
        _Set(storage, transformed);
    }
}

template <class Storage>
static void _MakeResolvedAssetPaths(Storage storage,
                                    const PcpNodeRef &node,
                                    const SdfLayerRefPtr &layer,
                                    bool anchorAssetPathsOnly)
{
    if (_IsHolding<SdfAssetPath>(storage)) {
        SdfAssetPath assetPath;
        _UncheckedSwap(storage, assetPath);
        _MakeResolvedAssetPathsImpl(
            layer, node.GetLayerStack()->GetIdentifier().pathResolverContext,
            &assetPath, 1,  anchorAssetPathsOnly);
        _UncheckedSwap(storage, assetPath);
    } else if (_IsHolding<VtArray<SdfAssetPath>>(storage)) {
        VtArray<SdfAssetPath> assetPaths;
        _UncheckedSwap(storage, assetPaths);
        _MakeResolvedAssetPathsImpl(
            layer, node.GetLayerStack()->GetIdentifier().pathResolverContext,
            assetPaths.data(), assetPaths.size(), anchorAssetPathsOnly);
        _UncheckedSwap(storage, assetPaths);
    }
}

// If the given dictionary contains any SdfAssetPath or
// VtArray<SdfAssetPath> as values, fills in those values
// with their resolved paths.
static void
_ResolveAssetPathsInDictionary(const SdfLayerRefPtr &anchor,
                               const PcpNodeRef &node,
                               VtDictionary *dict,
                               bool anchorAssetPathsOnly)
{
    for (auto& entry : *dict) {
        VtValue& v = entry.second;
        if (v.IsHolding<VtDictionary>()) {
            VtDictionary resolvedDict;
            v.UncheckedSwap(resolvedDict);
            _ResolveAssetPathsInDictionary(
                anchor, node, &resolvedDict, anchorAssetPathsOnly);
            v.UncheckedSwap(resolvedDict);
        }
        else {
            _MakeResolvedAssetPaths(&v, node, anchor, anchorAssetPathsOnly);
        }
    }
}

namespace {

template <class Storage>
struct StrongestValueComposer
{
    static const bool ProducesValue = true;

    explicit StrongestValueComposer(Storage s, 
                                    bool anchorAssetPathsOnly = false)
        : _value(s), _done(false), _anchorAssetPathsOnly(anchorAssetPathsOnly) 
        {}

    const std::type_info& GetHeldTypeid() const { return _GetTypeid(_value); }
    bool IsDone() const { return _done; }
    bool ConsumeAuthored(const PcpNodeRef &node,
                         const SdfLayerRefPtr &layer,
                         const SdfAbstractDataSpecId &specId,
                         const TfToken &fieldName,
                         const TfToken &keyPath) {

        // Handle special value-type composition: dictionaries merge atop each
        // other, and time sample maps must be transformed by layer offsets.
        bool isDict = false;
        VtDictionary tmpDict;
        if (_IsHolding<VtDictionary>(_value)) {
            isDict = true;
            // Copy to the side since we'll have to merge if the next opinion is
            // also a dictionary.
            tmpDict = _UncheckedGet<VtDictionary>(_value);
        }

        // Try to read value from scene description.
        _done = keyPath.IsEmpty() ?
            layer->HasField(specId, fieldName, _value) :
            layer->HasFieldDictKey(specId, fieldName, keyPath, _value);

        if (_done) {
            if (_IsHolding<VtDictionary>(_value)) {
                VtDictionary resolvedDict;
                _UncheckedSwap(_value, resolvedDict);
                _ResolveAssetPathsInDictionary(
                    layer, node, &resolvedDict, _anchorAssetPathsOnly);
                _UncheckedSwap(_value, resolvedDict);                

                // Continue composing if we got a dictionary.
                _done = false;
                if (isDict) {
                    // Merge dictionaries: _value is weaker, tmpDict stronger.
                    VtDictionaryOverRecursive(
                        &tmpDict, _UncheckedGet<VtDictionary>(_value));
                    _UncheckedSwap(_value, tmpDict);
                }
                return true;
            } else if (_IsHolding<SdfTimeSampleMap>(_value)) {
                _ApplyLayerOffset(_value, node, layer);
            } else {
                _MakeResolvedAssetPaths(
                    _value, node, layer, _anchorAssetPathsOnly);
            }
        }
        return _done;
    }
    void ConsumeUsdFallback(const TfToken &primTypeName,
                            const TfToken &propName,
                            const TfToken &fieldName,
                            const TfToken &keyPath) {

        bool isDict = false;
        VtDictionary tmpDict;
        if (_IsHolding<VtDictionary>(_value)) {
            isDict = true;
            // Copy to the side since we'll have to merge if the next opinion is
            // also a dictionary.
            tmpDict = _UncheckedGet<VtDictionary>(_value);
        }

        // Try to read fallback value.
        _done = keyPath.IsEmpty() ?
            UsdSchemaRegistry::HasField(
                primTypeName, propName, fieldName, _value) :
            UsdSchemaRegistry::HasFieldDictKey(
                primTypeName, propName, fieldName, keyPath, _value);

        if (_done && isDict && _IsHolding<VtDictionary>(_value)) {
            // Merge dictionaries: _value is weaker, tmpDict stronger.
            VtDictionaryOverRecursive(&tmpDict, 
                                      _UncheckedGet<VtDictionary>(_value));
            _Set(_value, tmpDict);
        }
    }
    template <class ValueType>
    void ConsumeExplicitValue(ValueType type) {
        _Set(_value, type);
        _done = true;
    }

protected:
    Storage _value;
    bool _done;
    bool _anchorAssetPathsOnly;
};


struct ExistenceComposer
{
    static const bool ProducesValue = false;

    ExistenceComposer() : _done(false), _strongestLayer(NULL) {}
    explicit ExistenceComposer(SdfLayerRefPtr *strongestLayer) 
        : _done(false), _strongestLayer(strongestLayer) {}

    const std::type_info& GetHeldTypeid() const { return typeid(void); }
    bool IsDone() const { return _done; }
    bool ConsumeAuthored(const PcpNodeRef &node,
                         const SdfLayerRefPtr &layer,
                         const SdfAbstractDataSpecId &specId,
                         const TfToken &fieldName,
                         const TfToken &keyPath,
                         const SdfLayerOffset & = SdfLayerOffset()) {
        _done = keyPath.IsEmpty() ?
            layer->HasField(specId, fieldName,
                            static_cast<VtValue *>(NULL)) :
            layer->HasFieldDictKey(specId, fieldName, keyPath,
                                   static_cast<VtValue*>(NULL));
        if (_done && _strongestLayer)
            *_strongestLayer = layer;
        return _done;
    }
    void ConsumeUsdFallback(const TfToken &primTypeName,
                            const TfToken &propName,
                            const TfToken &fieldName,
                            const TfToken &keyPath) {
        _done = keyPath.IsEmpty() ?
            UsdSchemaRegistry::HasField(
                primTypeName, propName, fieldName,
                static_cast<VtValue *>(NULL)) :
            UsdSchemaRegistry::HasFieldDictKey(
                primTypeName, propName, fieldName, keyPath,
                static_cast<VtValue*>(NULL));
        if (_strongestLayer)
            *_strongestLayer = TfNullPtr;
    }
    template <class ValueType>
    void ConsumeExplicitValue(ValueType type) {
        _done = true;
    }

protected:
    bool _done;
    SdfLayerRefPtr *_strongestLayer;
};

}

// --------------------------------------------------------------------- //
// Specialized Value Resolution
// --------------------------------------------------------------------- //

// Iterate over a prim's specs until we get a non-empty, non-any-type typeName.
static TfToken
_ComposeTypeName(const PcpPrimIndex &primIndex)
{
    for (Usd_Resolver res(&primIndex); res.IsValid(); res.NextLayer()) {
        TfToken tok;
        if (res.GetLayer()->HasField(
                res.GetLocalPath(), SdfFieldKeys->TypeName, &tok)) {
            if (!tok.IsEmpty() && tok != SdfTokens->AnyTypeToken)
                return tok;
        }
    }
    return TfToken();
}

SdfSpecifier
UsdStage::_GetSpecifier(Usd_PrimDataConstPtr primData) const
{
    SdfSpecifier result = SdfSpecifierOver;
    SdfAbstractDataTypedValue<SdfSpecifier> resultVal(&result);
    StrongestValueComposer<SdfAbstractDataValue *> composer(&resultVal);
    _GetPrimSpecifierImpl(primData, /*useFallbacks=*/true, &composer);
    return result;
}

SdfSpecifier
UsdStage::_GetSpecifier(const UsdPrim &prim) const
{
    return _GetSpecifier(get_pointer(prim._Prim()));
}

bool
UsdStage::_IsCustom(const UsdProperty &prop) const
{
    // Custom is composed as true if there is no property definition and it is
    // true anywhere in the stack of opinions.

    if (_GetPropertyDefinition(prop))
        return false;

    const TfToken &propName = prop.GetName();

    TF_REVERSE_FOR_ALL(itr, prop.GetPrim().GetPrimIndex().GetNodeRange()) {

        if (itr->IsInert() || !itr->HasSpecs()) {
            continue;
        }

        const SdfAbstractDataSpecId specId(&itr->GetPath(), &propName);
        TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
            bool result = false;
            if ((*layerIt)->HasField(specId, SdfFieldKeys->Custom, &result)
                && result) {
                return true;
            }
        }
    }

    return SdfSchema::GetInstance().GetFieldDefinition(
        SdfFieldKeys->Custom)->GetFallbackValue().Get<bool>();
}

SdfVariability
UsdStage
::_GetVariability(const UsdProperty &prop) const
{
    // The composed variability is the taken from the weakest opinion in the
    // stack, unless this is a built-in attribute, in which case the definition
    // wins.

    if (prop.Is<UsdAttribute>()) {
        UsdAttribute attr = prop.As<UsdAttribute>();
        // Check definition.
        if (SdfAttributeSpecHandle attrDef = _GetAttributeDefinition(attr)) {
            return attrDef->GetVariability();
        }

        // Check authored scene description.
        const TfToken &attrName = attr.GetName();
        TF_REVERSE_FOR_ALL(itr, attr.GetPrim().GetPrimIndex().GetNodeRange()) {
            if (itr->IsInert() || !itr->HasSpecs())
                continue;

            const SdfAbstractDataSpecId specId(&itr->GetPath(), &attrName);
            TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
                SdfVariability result;
                if ((*layerIt)->HasField(
                        specId, SdfFieldKeys->Variability, &result)) {
                    return result;
                }
            }
        }
    }

    // Fall back to schema.
    return SdfSchema::GetInstance().GetFieldDefinition(
        SdfFieldKeys->Variability)->GetFallbackValue().Get<SdfVariability>();
}

// --------------------------------------------------------------------- //
// Metadata Resolution
// --------------------------------------------------------------------- //

bool
UsdStage::_GetMetadata(const UsdObject &obj, const TfToken &fieldName,
                       const TfToken &keyPath, bool useFallbacks,
                       VtValue* result) const
{
    TRACE_FUNCTION();

    // XXX: HORRIBLE HACK.  Special-case timeSamples for now, since its
    // resulting value is a complicated function influenced by "model clips",
    // not a single value from scene description or fallbacks.  We special-case
    // it upfront here, since the Composer mechanism cannot deal with it.  We'd
    // like to consider remove "attribute value" fields from the set of stuff
    // that Usd considers to be "metadata", in which case we can remove this.
    if (obj.Is<UsdAttribute>()) {
        if (fieldName == SdfFieldKeys->TimeSamples) {
            SdfTimeSampleMap timeSamples;
            if (_GetTimeSampleMap(obj.As<UsdAttribute>(), &timeSamples)) {
                *result = timeSamples;
                return true;
            }
            return false;
        }
    }

    StrongestValueComposer<VtValue *> composer(result);
    return _GetMetadataImpl(obj, fieldName, keyPath, useFallbacks, &composer);
}

bool
UsdStage::_GetMetadata(const UsdObject &obj,
                       const TfToken& fieldName,
                       const TfToken &keyPath, bool useFallbacks,
                       SdfAbstractDataValue* result) const
{
    TRACE_FUNCTION();

    // XXX: HORRIBLE HACK.  Special-case timeSamples for now, since its
    // resulting value is a complicated function influenced by "model clips",
    // not a single value from scene description or fallbacks.  We special-case
    // it upfront here, since the Composer mechanism cannot deal with it.  We'd
    // like to consider remove "attribute value" fields from the set of stuff
    // that Usd considers to be "metadata", in which case we can remove this.
    if (obj.Is<UsdAttribute>()) {
        if (fieldName == SdfFieldKeys->TimeSamples) {
            SdfTimeSampleMap timeSamples;
            if (_GetTimeSampleMap(obj.As<UsdAttribute>(), &timeSamples)) {
                _Set(result, timeSamples);
                return true;
            }
            return false;
        }
    }

    StrongestValueComposer<SdfAbstractDataValue *> composer(result);
    return _GetMetadataImpl(obj, fieldName, keyPath, useFallbacks, &composer);
}


template <class Composer>
bool
UsdStage::_GetFallbackMetadataImpl(const UsdObject &obj,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   Composer *composer) const
{
    // Look for a fallback value in the definition.  XXX: This currently only
    // handles property definitions -- needs to be extended to prim definitions
    // as well.
    if (obj.Is<UsdProperty>()) {
        // NOTE: This code is performance critical.
        const TfToken &typeName = obj._Prim()->GetTypeName();
        composer->ConsumeUsdFallback(
            typeName, obj.GetName(), fieldName, keyPath);
        return composer->IsDone();
    }
    return false;
}

template <class T>
bool
UsdStage::_GetFallbackMetadata(const UsdObject &obj, const TfToken& fieldName,
                               const TfToken &keyPath, T* result) const
{
    StrongestValueComposer<T *> composer(result);
    return _GetFallbackMetadataImpl(obj, fieldName, keyPath, &composer);
}

template <class Composer>
void
UsdStage::_GetAttrTypeImpl(const UsdAttribute &attr,
                           const TfToken &fieldName,
                           bool useFallbacks,
                           Composer *composer) const
{
    TRACE_FUNCTION();
    if (_GetAttributeDefinition(attr)) {
        // Builtin attribute typename comes from definition.
        composer->ConsumeUsdFallback(
            attr.GetPrim().GetTypeName(),
            attr.GetName(), fieldName, TfToken());
        return;
    }
    // Fall back to general metadata composition.
    _GetGeneralMetadataImpl(attr, fieldName, TfToken(), useFallbacks, composer);
}

template <class Composer>
void
UsdStage::_GetAttrVariabilityImpl(const UsdAttribute &attr, bool useFallbacks,
                                  Composer *composer) const
{
    TRACE_FUNCTION();
    if (_GetAttributeDefinition(attr)) {
        // Builtin attribute typename comes from definition.
        composer->ConsumeUsdFallback(
            attr.GetPrim().GetTypeName(),
            attr.GetName(), SdfFieldKeys->Variability, TfToken());
        return;
    }
    // Otherwise variability is determined by the *weakest* authored opinion.
    // Walk authored scene description in reverse order.
    const TfToken &attrName = attr.GetName();
    TF_REVERSE_FOR_ALL(itr, attr.GetPrim().GetPrimIndex().GetNodeRange()) {
        if (itr->IsInert() || !itr->HasSpecs())
            continue;
        const SdfAbstractDataSpecId specId(&itr->GetPath(), &attrName);
        TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
            composer->ConsumeAuthored(
                *itr, *layerIt, specId, SdfFieldKeys->Variability, TfToken());
            if (composer->IsDone())
                return;
        }
    }
}

template <class Composer>
void
UsdStage::_GetPropCustomImpl(const UsdProperty &prop, bool useFallbacks,
                             Composer *composer) const
{
    TRACE_FUNCTION();
    // Custom is composed as true if there is no property definition and it is
    // true anywhere in the stack of opinions.
    if (_GetPropertyDefinition(prop)) {
        composer->ConsumeUsdFallback(
            prop.GetPrim().GetTypeName(), prop.GetName(),
            SdfFieldKeys->Custom, TfToken());
        return;
    }

    const TfToken &propName = prop.GetName();

    TF_REVERSE_FOR_ALL(itr, prop.GetPrim().GetPrimIndex().GetNodeRange()) {
        if (itr->IsInert() || !itr->HasSpecs())
            continue;

        const SdfAbstractDataSpecId specId(&itr->GetPath(), &propName);
        TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
            composer->ConsumeAuthored(
                *itr, *layerIt, specId, SdfFieldKeys->Custom, TfToken());
            if (composer->IsDone())
                return;
        }
    }
}

template <class Composer>
void
UsdStage::_GetPrimTypeNameImpl(const UsdPrim &prim, bool useFallbacks,
                               Composer *composer) const
{
    TRACE_FUNCTION();
    for (Usd_Resolver res(&prim.GetPrimIndex());
         res.IsValid(); res.NextLayer()) {
        TfToken tok;
        SdfAbstractDataSpecId specId(&res.GetLocalPath());
        if (res.GetLayer()->HasField(specId, SdfFieldKeys->TypeName, &tok)) {
            if (!tok.IsEmpty() && tok != SdfTokens->AnyTypeToken) {
                composer->ConsumeAuthored(
                    res.GetNode(), res.GetLayer(), specId,
                    SdfFieldKeys->TypeName, TfToken());
                if (composer->IsDone())
                    return;
            }
        }
    }
}

template <class Composer>
bool
UsdStage::_GetPrimSpecifierImpl(Usd_PrimDataConstPtr primData,
                                bool useFallbacks, Composer *composer) const
{
    // Handle the pseudo root as a special case.
    if (primData == _pseudoRoot)
        return false;

    // Instance master prims are always defined -- see Usd_PrimData for
    // details. Since the fallback for specifier is 'over', we have to
    // handle these prims specially here.
    if (primData->IsMaster()) {
        composer->ConsumeExplicitValue(SdfSpecifierDef);
        return true;
    }

    TRACE_FUNCTION();
    // Compose specifier.  The result is not given by simple strength order.  A
    // defining specifier is always stronger than a non-defining specifier.
    // Also, perhaps surprisingly, a class specifier due to a direct inherit is
    // weaker than any other defining specifier.  This handles cases like the
    // following:
    //
    // -- root.file -----------------------------------------------------------
    //   class "C" {}
    //   over "A" (references = @other.file@</B>) {}
    //
    // -- other.file ----------------------------------------------------------
    //   class "C" {}
    //   def "B" (inherits = </C>) {}
    //
    // Here /A references /B in other.file, and /B inherits global class /C.
    // The strength order of specifiers for /A from strong-to-weak is:
    //
    // 1. 'over'  (from /A)
    // 2. 'class' (from /C in root)
    // 3. 'def'   (from /B)
    // 4. 'class' (from /C in other)
    //
    // If we were to pick the strongest defining specifier, /A would be a class.
    // But that's wrong: /A should be a 'def'.  Inheriting a class should not
    // make the instance a class.  Classness should not be inherited.  Treating
    // 'class' specifiers due to direct inherits as weaker than all other
    // defining specifiers avoids this problem.

    // These are ordered so stronger strengths are numerically larger.
    enum _SpecifierStrength {
        _SpecifierStrengthNonDefining,
        _SpecifierStrengthDirectlyInheritedClass,
        _SpecifierStrengthDefining
    };

    boost::optional<SdfSpecifier> specifier;
    _SpecifierStrength strength = _SpecifierStrengthNonDefining;

    // Iterate over all prims, strongest to weakest.
    SdfSpecifier curSpecifier = SdfSpecifierOver;

    Usd_Resolver::Position specPos;

    const PcpPrimIndex &primIndex = primData->GetPrimIndex();
    for (Usd_Resolver res(&primIndex); res.IsValid(); res.NextLayer()) {
        // Get specifier and its strength from this prim.
        _SpecifierStrength curStrength = _SpecifierStrengthDefining;
        if (res.GetLayer()->HasField(
                res.GetLocalPath(), SdfFieldKeys->Specifier, &curSpecifier)) {
            specPos = res.GetPosition();

            if (SdfIsDefiningSpecifier(curSpecifier)) {
                // Compute strength.
                if (curSpecifier == SdfSpecifierClass) {
                    // See if this excerpt is due to direct inherits.  Walk up
                    // the excerpt tree looking for a direct inherit.  If we
                    // find one set the strength and stop.
                    for (PcpNodeRef node = res.GetNode();
                         node; node = node.GetParentNode()) {

                        if (PcpIsInheritArc(node.GetArcType()) &&
                            !node.IsDueToAncestor()) {
                            curStrength =
                                _SpecifierStrengthDirectlyInheritedClass;
                            break;
                        }
                    }

                }
            }
            else {
                // Strength is _SpecifierStrengthNonDefining and can't be
                // stronger than the current strength so there's no need to do
                // the check below.
                continue;
            }
        }
        else {
            // Variant PrimSpecs don't have a specifier field, continue looking
            // for a specifier.
            continue;
        }

        // Use the specifier if it's stronger.
        if (curStrength > strength) {
            specifier = curSpecifier;
            strength = curStrength;

            // We can stop as soon as we find a specifier with the strongest
            // strength.
            if (strength == _SpecifierStrengthDefining)
                break;
        }
    }

    // Verify we found *something*.  We should never have PrimData without at
    // least one PrimSpec, and 'specifier' is required, so it must be present.
    if (TF_VERIFY(specPos.GetLayer(), "No PrimSpecs for '%s'",
                  primData->GetPath().GetText())) {
        // Let the composer see the deciding opinion.
        composer->ConsumeAuthored(
            specPos.GetNode(), specPos.GetLayer(), 
            SdfAbstractDataSpecId(&specPos.GetLocalPath()),
            SdfFieldKeys->Specifier, TfToken());
    }
    return true;
}

template <class ListOpType, class Composer>
bool 
UsdStage::_GetListOpMetadataImpl(const UsdObject &obj,
                                 const TfToken &fieldName,
                                 bool useFallbacks,
                                 Usd_Resolver *res,
                                 Composer *composer) const
{
    // Collect all list op opinions for this field.
    std::vector<ListOpType> listOps;

    static TfToken empty;
    const TfToken &propName = obj.Is<UsdProperty>() ? obj.GetName() : empty;
    SdfAbstractDataSpecId specId(&res->GetLocalPath(), &propName);

    for (bool isNewNode = false; res->IsValid(); isNewNode = res->NextLayer()) {
        if (isNewNode)
            specId = SdfAbstractDataSpecId(&res->GetLocalPath(), &propName);

        // Consume an authored opinion here, if one exists.
        ListOpType op;
        if (res->GetLayer()->HasField(specId, fieldName, &op)) {
            listOps.emplace_back(op);
        }
    }

    if (useFallbacks) {
        ListOpType fallbackListOp;
        SdfAbstractDataTypedValue<ListOpType> out(&fallbackListOp);        
        if (_GetFallbackMetadata(obj, fieldName, empty, 
                                 static_cast<SdfAbstractDataValue*>(&out))) {
            listOps.emplace_back(fallbackListOp);
        }
    }

    // Bake the result of applying the list ops into a single explicit
    // list op.
    if (!listOps.empty()) {
        typename ListOpType::ItemVector items;
        std::for_each(
            listOps.crbegin(), listOps.crend(),
            [&items](const ListOpType& op) { op.ApplyOperations(&items); });
         
        ListOpType bakedListOp;
        bakedListOp.SetExplicitItems(std::move(items));
        composer->ConsumeExplicitValue(bakedListOp);
        return true;
    }

    return false;
}

template <class Composer>
bool
UsdStage::_GetSpecialMetadataImpl(const UsdObject &obj,
                                  const TfToken &fieldName,
                                  const TfToken &keyPath,
                                  bool useFallbacks,
                                  Composer *composer) const
{
    // Dispatch to special-case composition rules based on type and field.
    if (obj.Is<UsdProperty>()) {
        if (obj.Is<UsdAttribute>()) {
            if (fieldName == SdfFieldKeys->TypeName) {
                _GetAttrTypeImpl(
                    obj.As<UsdAttribute>(), fieldName, useFallbacks, composer);
                return true;
            } else if (fieldName == SdfFieldKeys->Variability) {
                _GetAttrVariabilityImpl(
                    obj.As<UsdAttribute>(), useFallbacks, composer);
                return true;
            }
        }
        if (fieldName == SdfFieldKeys->Custom) {
            _GetPropCustomImpl(obj.As<UsdProperty>(), useFallbacks, composer);
            return true;
        }
    } else if (obj.Is<UsdPrim>()) {
        if (fieldName == SdfFieldKeys->TypeName) {
            _GetPrimTypeNameImpl(obj.As<UsdPrim>(), useFallbacks, composer);
            return true;
        } else if (fieldName == SdfFieldKeys->Specifier) {
            _GetPrimSpecifierImpl(
                get_pointer(obj._Prim()), useFallbacks, composer);
            return true;
        }
    }

    return false;
}

template <class Composer>
bool
UsdStage::_GetMetadataImpl(
    const UsdObject &obj,
    const TfToken& fieldName,
    const TfToken& keyPath,
    bool useFallbacks,
    Composer *composer) const
{
    // XXX: references, inherit paths, variant selection currently unhandled.
    TfErrorMark m;

    // Handle special cases.
    if (_GetSpecialMetadataImpl(
            obj, fieldName, keyPath, useFallbacks, composer)) {

        return true;
    }

    if (!m.IsClean()) {
        // An error occurred during _GetSpecialMetadataImpl.
        return false;
    }

    return _GetGeneralMetadataImpl(
        obj, fieldName, keyPath, useFallbacks, composer) && m.IsClean();
}

template <class Composer>
bool
UsdStage::_GetGeneralMetadataImpl(const UsdObject &obj,
                                  const TfToken& fieldName,
                                  const TfToken& keyPath,
                                  bool useFallbacks,
                                  Composer *composer) const
{
    Usd_Resolver resolver(&obj._Prim()->GetPrimIndex());
    if (!_ComposeGeneralMetadataImpl(
            obj, fieldName, keyPath, useFallbacks, &resolver, composer)) {
        return false;
    }

    if (Composer::ProducesValue) {
        // If the metadata value produced by the composer is a type that
        // requires specific composition behavior, dispatch to the appropriate 
        // helper. Pass along the same resolver so that the helper can start 
        // from where _ComposeGeneralMetadataImpl found the first metadata 
        // value.
        const std::type_info& valueTypeId(composer->GetHeldTypeid());
        if (valueTypeId == typeid(SdfIntListOp)) {
            return _GetListOpMetadataImpl<SdfIntListOp>(
                obj, fieldName, useFallbacks, &resolver, composer);
        }
        else if (valueTypeId == typeid(SdfInt64ListOp)) {
            return _GetListOpMetadataImpl<SdfInt64ListOp>(
                obj, fieldName, useFallbacks, &resolver, composer);
        }
        else if (valueTypeId == typeid(SdfUIntListOp)) {
            return _GetListOpMetadataImpl<SdfUIntListOp>(
                obj, fieldName, useFallbacks, &resolver, composer);
        }
        else if (valueTypeId == typeid(SdfUInt64ListOp)) {
            return _GetListOpMetadataImpl<SdfUInt64ListOp>(
                obj, fieldName, useFallbacks, &resolver, composer);
        }
        else if (valueTypeId == typeid(SdfStringListOp)) {
            return _GetListOpMetadataImpl<SdfStringListOp>(
                obj, fieldName, useFallbacks, &resolver, composer);
        }
        else if (valueTypeId == typeid(SdfTokenListOp)) {
            return _GetListOpMetadataImpl<SdfTokenListOp>(
                obj, fieldName, useFallbacks, &resolver, composer);
        }
    }
    
    return true;
}

template <class Composer>
bool
UsdStage::_ComposeGeneralMetadataImpl(const UsdObject &obj,
                                      const TfToken& fieldName,
                                      const TfToken& keyPath,
                                      bool useFallbacks,
                                      Usd_Resolver* res,
                                      Composer *composer) const
{
    // Main resolution loop.
    static TfToken empty;
    const TfToken &propName = obj.Is<UsdProperty>() ? obj.GetName() : empty;
    SdfAbstractDataSpecId specId(&res->GetLocalPath(), &propName);
    bool gotOpinion = false;

    for (bool isNewNode = false; res->IsValid(); isNewNode = res->NextLayer()) {
        if (isNewNode) 
            specId = SdfAbstractDataSpecId(&res->GetLocalPath(), &propName);

        // Consume an authored opinion here, if one exists.
        gotOpinion |= composer->ConsumeAuthored(
            res->GetNode(), res->GetLayer(), specId, fieldName, keyPath);
        
        if (composer->IsDone()) 
            return true;
    }

    if (useFallbacks)
        _GetFallbackMetadataImpl(obj, fieldName, keyPath, composer);

    return gotOpinion || composer->IsDone();
}

bool
UsdStage::_HasMetadata(const UsdObject &obj, const TfToken& fieldName,
                       const TfToken &keyPath, bool useFallbacks) const
{
    ExistenceComposer composer;
    _GetMetadataImpl(obj, fieldName, keyPath, useFallbacks, &composer);
    return composer.IsDone();
}

TfTokenVector
UsdStage::_ListMetadataFields(const UsdObject &obj, bool useFallbacks) const
{
    TRACE_FUNCTION();

    TfTokenVector result;

    static TfToken empty;
    const TfToken &propName = obj.Is<UsdProperty>() ? obj.GetName() : empty;

    Usd_Resolver res(&obj.GetPrim().GetPrimIndex());
    SdfAbstractDataSpecId specId(&res.GetLocalPath(), &propName);
    PcpNodeRef lastNode = res.GetNode();
    SdfSpecType specType = SdfSpecTypeUnknown;

    SdfPropertySpecHandle propDef;

    // If this is a builtin property, determine specType from the definition.
    if (obj.Is<UsdProperty>()) {
        propDef = _GetPropertyDefinition(obj.As<UsdProperty>());
        if (propDef)
            specType = propDef->GetSpecType();
    }

    // Insert authored fields, discovering spec type along the way.
    for (; res.IsValid(); res.NextLayer()) {
        if (res.GetNode() != lastNode) {
            lastNode = res.GetNode();
            specId = SdfAbstractDataSpecId(&res.GetLocalPath(), &propName);
        }
        const SdfLayerRefPtr& layer = res.GetLayer();
        if (specType == SdfSpecTypeUnknown)
            specType = layer->GetSpecType(specId);

        for (const auto& fieldName : layer->ListFields(specId)) {
            if (!_IsPrivateFieldKey(fieldName))
                result.push_back(fieldName);
        }
    }

    // Insert required fields for spec type.
    const SdfSchema::SpecDefinition* specDef = NULL;
    specDef = SdfSchema::GetInstance().GetSpecDefinition(specType);
    if (specDef) {
        for (const auto& fieldName : specDef->GetRequiredFields()) {
            if (!_IsPrivateFieldKey(fieldName))
                result.push_back(fieldName);
        }
    }

    // If this is a builtin property, add any defined metadata fields.
    // XXX: this should handle prim definitions too.
    if (useFallbacks && propDef) {
        for (const auto& fieldName : propDef->ListFields()) {
            if (!_IsPrivateFieldKey(fieldName))
                result.push_back(fieldName);
        }
    }

    // Sort & remove duplicate fields.
    std::sort(result.begin(), result.end(), TfDictionaryLessThan());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

void
UsdStage::_GetAllMetadata(const UsdObject &obj,
                          bool useFallbacks,
                          UsdMetadataValueMap* resultMap,
                          bool anchorAssetPathsOnly) const
{
    TRACE_FUNCTION();

    UsdMetadataValueMap &result = *resultMap;

    TfTokenVector fieldNames = _ListMetadataFields(obj, useFallbacks);
    for (const auto& fieldName : fieldNames) {
        VtValue val;
        StrongestValueComposer<VtValue *> composer(&val, anchorAssetPathsOnly);
        _GetMetadataImpl(obj, fieldName, TfToken(), useFallbacks, &composer);
        result[fieldName] = val;
    }
}

// --------------------------------------------------------------------- //
// Default & TimeSample Resolution
// --------------------------------------------------------------------- //

static bool
_ClipAppliesToLayerStackSite(
    const Usd_ClipRefPtr& clip,
    const PcpLayerStackPtr& layerStack, const SdfPath& primPathInLayerStack)
{
    return (layerStack == clip->sourceLayerStack
        && primPathInLayerStack.HasPrefix(clip->sourcePrimPath));
}

static bool
_ClipsApplyToNode(
    const Usd_ClipCache::Clips& clips, 
    const PcpNodeRef& node)
{
    return (node.GetLayerStack() == clips.sourceLayerStack
            && node.GetPath().HasPrefix(clips.sourcePrimPath));
}

static
const std::vector<const Usd_ClipCache::Clips*>
_GetClipsThatApplyToNode(
    const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim,
    const PcpNodeRef& node,
    const SdfAbstractDataSpecId& specId)
{
    std::vector<const Usd_ClipCache::Clips*> relevantClips;

    for (const auto& localClips : clipsAffectingPrim) {
        if (_ClipsApplyToNode(localClips, node)) {
            // Only look for samples in clips for attributes that are
            // marked as varying in the clip manifest (if one is present).
            // This gives users a way to indicate that an attribute will
            // never have samples in a clip, which can help performance.
            // 
            // We normally do not consider variability during value 
            // resolution to avoid the cost of composing variability on 
            // each value fetch. We can use it here because we're only 
            // fetching it from a single layer, which should be cheap. 
            // This is also convenient for users, since it allows them 
            // to reuse assets that may have both uniform and varying 
            // attributes as manifests.
            if (localClips.manifestClip) {
                SdfVariability attrVariability = SdfVariabilityUniform;
                if (!localClips.manifestClip->HasField(
                        specId, SdfFieldKeys->Variability, &attrVariability)
                    || attrVariability != SdfVariabilityVarying) {
                    continue;
                }
            }

            relevantClips.push_back(&localClips);
        }
    }

    return relevantClips;
}


bool
UsdStage::_GetValue(UsdTimeCode time, const UsdAttribute &attr, 
                    VtValue* result) const
{
    Usd_UntypedInterpolator interpolator(attr, result);
    return _GetValueImpl(time, attr, &interpolator, result);
}

namespace {

// Metafunction for selecting the appropriate interpolation object if the
// given value type supports linear interpolation.
template <typename T>
struct _SelectInterpolator 
    : public boost::mpl::if_c<
          UsdLinearInterpolationTraits<T>::isSupported,
          Usd_LinearInterpolator<T>,
          Usd_HeldInterpolator<T> > { };

}

template <class T>
bool
UsdStage::_GetValue(UsdTimeCode time, const UsdAttribute &attr,
                    T* result) const
{
    SdfAbstractDataTypedValue<T> out(result);

    if (_interpolationType == UsdInterpolationTypeLinear) {
        typedef typename _SelectInterpolator<T>::type _Interpolator;
        _Interpolator interpolator(result);
        return _GetValueImpl<SdfAbstractDataValue>(
            time, attr, &interpolator, &out);
    }

    Usd_HeldInterpolator<T> interpolator(result);
    return _GetValueImpl<SdfAbstractDataValue>(
        time, attr, &interpolator, &out);
}

class UsdStage_ResolveInfoAccess
{
public:
    template <class T>
    static bool _GetTimeSampleValue(UsdTimeCode time, const UsdAttribute& attr,
                             const UsdResolveInfo &info,
                             const double *lowerHint, const double *upperHint,
                             Usd_InterpolatorBase *interpolator,
                             T *result)
    {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());
        const SdfLayerRefPtr& layer = 
            info._layerStack->GetLayers()[info._layerIndex];
        const double localTime =
            info._layerToStageOffset.GetInverse() * time.GetValue();

        double upper = 0.0;
        double lower = 0.0;

        if (lowerHint && upperHint) {
            lower = *lowerHint;
            upper = *upperHint;
        }
        else {
            if (!TF_VERIFY(layer->GetBracketingTimeSamplesForPath(
                        specId, localTime, &lower, &upper),
                        "No bracketing time samples for "
                               "%s on <%s> for time %g between %g and %g",
                               layer->GetIdentifier().c_str(),
                               specId.GetFullSpecPath().GetText(),
                               localTime, lower, upper)) {
                return false;
            }
        }

        TF_DEBUG(USD_VALUE_RESOLUTION).Msg(
            "RESOLVE: reading field %s:%s from @%s@, "
            "with requested time = %.3f (local time = %.3f) "
            "reading from sample %.3f \n",
            specId.GetString().c_str(),
            SdfFieldKeys->TimeSamples.GetText(),
            layer->GetIdentifier().c_str(),
            time.GetValue(),
            localTime,
            lower);

        return Usd_GetOrInterpolateValue(
            layer, specId, localTime, lower, upper, interpolator, result);
    } 

    template <class T>
    static bool _GetClipValue(UsdTimeCode time, const UsdAttribute& attr,
                              const UsdResolveInfo &info,
                              const Usd_ClipRefPtr &clip,
                              double lower, double upper,
                              Usd_InterpolatorBase *interpolator,
                              T *result)
    {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());
        const double localTime = time.GetValue();

        TF_DEBUG(USD_VALUE_RESOLUTION).Msg(
            "RESOLVE: reading field %s:%s from clip %s, "
            "with requested time = %.3f "
            "reading from sample %.3f \n",
            specId.GetString().c_str(),
            SdfFieldKeys->TimeSamples.GetText(),
            TfStringify(clip->assetPath).c_str(),
            localTime,
            lower);

        return Usd_GetOrInterpolateValue(
            clip, specId, localTime, lower, upper, interpolator, result);
    }
};

template <class T>
struct UsdStage::_ExtraResolveInfo
{
    _ExtraResolveInfo()
        : lowerSample(0), upperSample(0), defaultOrFallbackValue(NULL)
    { }

    double lowerSample;
    double upperSample;
 
    T* defaultOrFallbackValue;

    Usd_ClipRefPtr clip;
};

SdfLayerRefPtr
UsdStage::_GetLayerWithStrongestValue(
    UsdTimeCode time, const UsdAttribute &attr) const
{
    SdfLayerRefPtr resultLayer;
    if (time.IsDefault()) {
        ExistenceComposer getLayerComposer(&resultLayer);
        _GetMetadataImpl(attr, SdfFieldKeys->Default,
                         TfToken(), /*useFallbacks=*/false, &getLayerComposer);
    } else {
        UsdResolveInfo resolveInfo;
        _ExtraResolveInfo<SdfAbstractDataValue> extraResolveInfo;
        
        _GetResolveInfo(attr, &resolveInfo, &time, &extraResolveInfo);
        
        if (resolveInfo._source == UsdResolveInfoSourceTimeSamples ||
            resolveInfo._source == UsdResolveInfoSourceDefault) {
            resultLayer = 
                resolveInfo._layerStack->GetLayers()[resolveInfo._layerIndex];
        }
        else if (resolveInfo._source == UsdResolveInfoSourceValueClips) {
            resultLayer = extraResolveInfo.clip->_GetLayerForClip();
        }
    }
    return resultLayer;
}

template <class T>
bool
UsdStage::_GetValueImpl(UsdTimeCode time, const UsdAttribute &attr, 
                        Usd_InterpolatorBase* interpolator,
                        T *result) const
{
    if (time.IsDefault()) {
        bool valueFound = _GetMetadata(attr, SdfFieldKeys->Default,
                                       TfToken(), /*useFallbacks=*/true, result);
        return valueFound && (!Usd_ClearValueIfBlocked(result));
    }

    UsdResolveInfo resolveInfo;
    _ExtraResolveInfo<T> extraResolveInfo;
    extraResolveInfo.defaultOrFallbackValue = result;

    TfErrorMark m;
    _GetResolveInfo(attr, &resolveInfo, &time, &extraResolveInfo);

    if (resolveInfo._source == UsdResolveInfoSourceTimeSamples) {
        return UsdStage_ResolveInfoAccess::_GetTimeSampleValue(
            time, attr, resolveInfo, 
            &extraResolveInfo.lowerSample, &extraResolveInfo.upperSample,
            interpolator, result);
    }
    else if (resolveInfo._source == UsdResolveInfoSourceValueClips) {
        return UsdStage_ResolveInfoAccess::_GetClipValue(
            time, attr, resolveInfo, extraResolveInfo.clip,
            extraResolveInfo.lowerSample, extraResolveInfo.upperSample,
            interpolator, result);
    }
    else if (resolveInfo._source == UsdResolveInfoSourceDefault ||
             resolveInfo._source == UsdResolveInfoSourceFallback) {
        // Nothing to do here -- the call to _GetResolveInfo will have
        // filled in the result with the default value.
        return m.IsClean();
    }

    return _GetValueFromResolveInfoImpl(
        resolveInfo, time, attr, interpolator, result);
}

namespace 
{
bool
_HasTimeSamples(const SdfLayerRefPtr& source, 
                const SdfAbstractDataSpecId& specId, 
                const double* time = nullptr, 
                double* lower = nullptr, double* upper = nullptr)
{
    if (time) {
        // If caller wants bracketing time samples as well, we can just use
        // GetBracketingTimeSamplesForPath. If no samples exist, this should
        // return false.
        return source->GetBracketingTimeSamplesForPath(
            specId, *time, lower, upper);
    }

    return source->GetNumTimeSamplesForPath(specId) > 0;
}

bool
_HasTimeSamples(const Usd_ClipRefPtr& source, 
                const SdfAbstractDataSpecId& specId, 
                const double* time = nullptr, 
                double* lower = nullptr, double* upper = nullptr)
{
    if (time) {
        return source->GetBracketingTimeSamplesForPath(
                    specId, *time, lower, upper) && 
               source->_GetNumTimeSamplesForPathInLayerForClip(specId) != 0;
    }

    // Use this method to directly access authored time samples,
    // disregarding 'fake' samples used by clips.
    return source->_GetNumTimeSamplesForPathInLayerForClip(specId) > 0;
}

enum _DefaultValueResult {
    _DefaultValueNone = 0,
    _DefaultValueFound,
    _DefaultValueBlocked,
};

template <class T>    
_DefaultValueResult 
_HasDefault(const SdfLayerRefPtr& layer, const SdfAbstractDataSpecId& specId, 
            T* value)
{
    // We need to actually examine the default value in all cases to see
    // if a block was authored. So, if no value to fill in was specified,
    // we need to create a dummy one.
    if (!value) {
        VtValue dummy;
        return _HasDefault(layer, specId, &dummy);
    }

    if (layer->HasField(specId, SdfFieldKeys->Default, value)) {
        if (Usd_ClearValueIfBlocked(value)) {
            return _DefaultValueBlocked;
        }
        return _DefaultValueFound;
    }
    return _DefaultValueNone;
}
} // end anonymous namespace

// Our property stack resolver never indicates for resolution to stop
// as we need to gather all relevant property specs in the LayerStack
struct UsdStage::_PropertyStackResolver {
    SdfPropertySpecHandleVector propertyStack;

    bool ProcessFallback() { return false; }

    bool
    ProcessLayer(const size_t layerStackPosition,
                 const SdfAbstractDataSpecId& specId,
                 const PcpNodeRef& node,
                 const double *time) 
    {
        const auto layer 
            = node.GetLayerStack()->GetLayers()[layerStackPosition];
        const auto propertySpec 
            = layer->GetPropertyAtPath(specId.GetFullSpecPath());
        if (propertySpec) {
            propertyStack.push_back(propertySpec); 
        }

        return false;
    }

    bool
    ProcessClip(const Usd_ClipRefPtr& clip,
                const SdfAbstractDataSpecId& specId,
                const PcpNodeRef& node,
                const double* time) 
    {
        // If given a time, do a range check on the clip first.
        if (time && (*time < clip->startTime || *time >= clip->endTime)) 
            return false;

        double lowerSample = 0.0, upperSample = 0.0;
        if (_HasTimeSamples(clip, specId, time, &lowerSample, &upperSample)) {
            if (const auto propertySpec = clip->GetPropertyAtPath(specId)) 
                propertyStack.push_back(propertySpec);    
        }
     
        return false;
    }
};

SdfPropertySpecHandleVector
UsdStage::_GetPropertyStack(const UsdProperty &prop,
                            UsdTimeCode time) const
{
    _PropertyStackResolver resolver;
    _GetResolvedValueImpl(prop, &resolver, &time);
    return resolver.propertyStack; 
}

// A 'Resolver' for filling UsdResolveInfo.
template <typename T>
struct UsdStage::_ResolveInfoResolver 
{
    explicit _ResolveInfoResolver(const UsdAttribute& attr,
                                 UsdResolveInfo* resolveInfo,
                                 UsdStage::_ExtraResolveInfo<T>* extraInfo)
    :   _attr(attr), 
        _resolveInfo(resolveInfo),
        _extraInfo(extraInfo)
    {
    }

    bool
    ProcessFallback()
    {
        if (const bool hasFallback = UsdSchemaRegistry::HasField(
                _attr.GetPrim().GetTypeName(), _attr.GetName(), 
                SdfFieldKeys->Default, _extraInfo->defaultOrFallbackValue)) {
            _resolveInfo->_source = UsdResolveInfoSourceFallback;
            return true;
        }

        // No values at all.
        _resolveInfo->_source = UsdResolveInfoSourceNone;
        return true;
    }

    bool
    ProcessLayer(const size_t layerStackPosition,
                 const SdfAbstractDataSpecId& specId,
                 const PcpNodeRef& node,
                 const double *time) 
    {
        const PcpLayerStackPtr& nodeLayers = node.GetLayerStack();
        const SdfLayerRefPtrVector& layerStack = nodeLayers->GetLayers();
        const SdfLayerOffset layerToStageOffset =
            _GetLayerToStageOffset(node, layerStack[layerStackPosition]);
        const SdfLayerRefPtr& layer = layerStack[layerStackPosition];
        boost::optional<double> localTime;
        if (time) {
            localTime = layerToStageOffset.GetInverse() * (*time);
        }

        if (_HasTimeSamples(layer, specId, localTime.get_ptr(), 
                            &_extraInfo->lowerSample, 
                            &_extraInfo->upperSample)) {
            _resolveInfo->_source = UsdResolveInfoSourceTimeSamples;
        }
        else { 
            _DefaultValueResult defValue = 
                _HasDefault(layer, specId, _extraInfo->defaultOrFallbackValue);
            if (defValue == _DefaultValueFound) {
                _resolveInfo->_source = UsdResolveInfoSourceDefault;
            }
            else if (defValue == _DefaultValueBlocked) {
                _resolveInfo->_valueIsBlocked = true;
                return ProcessFallback();
            }
        }

        if (_resolveInfo->_source != UsdResolveInfoSourceNone) {
            _resolveInfo->_layerStack = nodeLayers;
            _resolveInfo->_layerIndex = layerStackPosition;
            _resolveInfo->_primPathInLayerStack = node.GetPath();
            _resolveInfo->_layerToStageOffset = layerToStageOffset;
            _resolveInfo->_node = node;
            return true;
        }

        return false;
    }

    bool
    ProcessClip(const Usd_ClipRefPtr& clip,
                const SdfAbstractDataSpecId& specId,
                const PcpNodeRef& node,
                const double* time)
    {
        // If given a time, do a range check on the clip first.
        if (time && (*time < clip->startTime || *time >= clip->endTime))
            return false;

        if (_HasTimeSamples(clip, specId, time,
                            &_extraInfo->lowerSample, 
                            &_extraInfo->upperSample)){
    
            _extraInfo->clip = clip;
            // If we're querying at a particular time, we know the value comes
            // from this clip at this time.  If we're not given a time, then we
            // cannot be sure, and we must say that the value source may be time
            // dependent.
            _resolveInfo->_source = time ?
                UsdResolveInfoSourceValueClips :
                UsdResolveInfoSourceIsTimeDependent;
            _resolveInfo->_layerStack = node.GetLayerStack();
            _resolveInfo->_primPathInLayerStack = node.GetPath();
            _resolveInfo->_node = node;
            return true;
        }

        return false;
    }

private:
    const UsdAttribute& _attr;
    UsdResolveInfo* _resolveInfo;
    UsdStage::_ExtraResolveInfo<T>* _extraInfo;
};

// NOTE:
// When dealing with value clips, this function may return different 
// results for the same attribute depending on whether the optional 
// UsdTimeCode is passed in.  This may be a little surprising because the
// resolve info is the same across all time for all other sources of
// values (e.g., time samples, defaults).  
template <class T>
void
UsdStage::_GetResolveInfo(const UsdAttribute &attr, 
                          UsdResolveInfo *resolveInfo,
                          const UsdTimeCode *time, 
                          _ExtraResolveInfo<T> *extraInfo) const
{
    _ExtraResolveInfo<T> localExtraInfo;
    if (!extraInfo) {
        extraInfo = &localExtraInfo;
    }

    _ResolveInfoResolver<T> resolver(attr, resolveInfo, extraInfo);
    _GetResolvedValueImpl(attr, &resolver, time);
    
    if (TfDebug::IsEnabled(USD_VALIDATE_VARIABILITY) &&
        (resolveInfo->_source == UsdResolveInfoSourceTimeSamples ||
         resolveInfo->_source == UsdResolveInfoSourceValueClips ||
         resolveInfo->_source == UsdResolveInfoSourceIsTimeDependent) &&
        _GetVariability(attr) == SdfVariabilityUniform) {

        TF_DEBUG(USD_VALIDATE_VARIABILITY)
            .Msg("Warning: detected time sample value on "
                 "uniform attribute <%s>\n", 
                 UsdDescribe(attr).c_str());
    }
}

// This function takes a Resolver object, which is used to process opinions
// in strength order. Resolvers must implement three functions: 
//       
//       ProcessLayer()
//       ProcessClip()
//       ProcessFallback()
//
// Each of these functions is required to return true, to indicate that 
// iteration of opinions should stop, and false otherwise.
template <class Resolver>
void
UsdStage::_GetResolvedValueImpl(const UsdProperty &prop,
                                Resolver *resolver,
                                const UsdTimeCode *time) const
{
    auto primHandle = prop._Prim();
    boost::optional<double> localTime;
    if (time && !time->IsDefault()) {
        localTime = time->GetValue();
    }

    // Retrieve all clips that may contribute time samples for this
    // attribute at the given time. Clips never contribute default
    // values.
    const std::vector<Usd_ClipCache::Clips>* clipsAffectingPrim = nullptr;
    if (primHandle->MayHaveOpinionsInClips()
        && (!time || !time->IsDefault())) {
        clipsAffectingPrim =
            &(_clipCache->GetClipsForPrim(primHandle->GetPath()));
    }

    // Clips may contribute opinions at nodes where no specs for the attribute
    // exist in the node's LayerStack. So, if we have any clips, tell
    // Usd_Resolver that we want to iterate over 'empty' nodes as well.
    const bool skipEmptyNodes = (bool)(!clipsAffectingPrim);

    for (Usd_Resolver res(&primHandle->GetPrimIndex(), skipEmptyNodes); 
         res.IsValid(); res.NextNode()) {

        const PcpNodeRef& node = res.GetNode();
        const bool nodeHasSpecs = node.HasSpecs();
        if (!nodeHasSpecs && !clipsAffectingPrim) {
            continue;
        }

        const SdfAbstractDataSpecId specId(&node.GetPath(), &prop.GetName());
        const SdfLayerRefPtrVector& layerStack 
            = node.GetLayerStack()->GetLayers();
        boost::optional<std::vector<const Usd_ClipCache::Clips*>> clips;
        for (size_t i = 0, e = layerStack.size(); i < e; ++i) {
            if (nodeHasSpecs) { 
                if (resolver->ProcessLayer(i, specId, node, 
                                           localTime.get_ptr())) {
                    return;
                }
            }

            if (clipsAffectingPrim){ 
                if (!clips) {
                    clips = _GetClipsThatApplyToNode(*clipsAffectingPrim,
                                                     node, specId);
                    // If we don't have specs on this node and clips don't
                    // apply we can mode onto the next node.
                    if (!nodeHasSpecs && clips->empty()) { 
                        break; 
                    }
                }
                
                // gcc 4.8 incorrectly detects boost::optional as uninitialized. 
                // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
                ARCH_PRAGMA_PUSH
                ARCH_PRAGMA_MAYBE_UNINITIALIZED

                for (const Usd_ClipCache::Clips* clipSet : *clips) {
                    // We only care about clips that were introduced at this
                    // position within the LayerStack.
                    if (clipSet->sourceLayerIndex != i) {
                        continue;
                    }

                    // Look through clips to see if they have a time sample for
                    // this attribute. If a time is given, examine just the clips
                    // that are active at that time.
                    for (const Usd_ClipRefPtr& clip : clipSet->valueClips) {
                        if (resolver->ProcessClip(clip, specId, node,
                                                  localTime.get_ptr())) {
                            return;
                        }
                    }
                }

                ARCH_PRAGMA_POP
            }    
        }
    }

    resolver->ProcessFallback();
}

void
UsdStage::_GetResolveInfo(const UsdAttribute &attr, 
                          UsdResolveInfo *resolveInfo,
                          const UsdTimeCode *time) const
{
    _GetResolveInfo<SdfAbstractDataValue>(attr, resolveInfo, time);
}

template <class T>
bool 
UsdStage::_GetValueFromResolveInfoImpl(const UsdResolveInfo &info,
                                       UsdTimeCode time, const UsdAttribute &attr,
                                       Usd_InterpolatorBase* interpolator,
                                       T* result) const
{
    if (time.IsDefault()) {
        bool valueFound = _GetMetadata(attr, SdfFieldKeys->Default,
                                       TfToken(), /*useFallbacks=*/true, result);
        return valueFound && (!Usd_ClearValueIfBlocked(result));
    }

    if (info._source == UsdResolveInfoSourceTimeSamples) {
        return UsdStage_ResolveInfoAccess::_GetTimeSampleValue(
            time, attr, info, nullptr, nullptr, interpolator, result);
    }
    else if (info._source == UsdResolveInfoSourceDefault) {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());
        const SdfLayerHandle& layer = 
            info._layerStack->GetLayers()[info._layerIndex];

        TF_DEBUG(USD_VALUE_RESOLUTION).Msg(
            "RESOLVE: reading field %s:%s from @%s@, with t = %.3f"
            " as default\n",
            specId.GetString().c_str(),
            SdfFieldKeys->TimeSamples.GetText(),
            layer->GetIdentifier().c_str(),
            time.GetValue());

        return TF_VERIFY(
            layer->HasField(specId, SdfFieldKeys->Default, result));
    }
    else if (info._source == UsdResolveInfoSourceValueClips) {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());

        const UsdPrim prim = attr.GetPrim();
        const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        for (const auto& clipAffectingPrim : clipsAffectingPrim) {
            const Usd_ClipRefPtrVector& clips = clipAffectingPrim.valueClips;
            for (size_t i = 0, numClips = clips.size(); i < numClips; ++i) {
                // Note that we do not apply layer offsets to the time.
                // Because clip metadata may be authored in different 
                // layers in the LayerStack, each with their own 
                // layer offsets, it is simpler to bake the effects of 
                // those offsets into Usd_Clip.
                const Usd_ClipRefPtr& clip = clips[i];
                const double localTime = time.GetValue();
                
                if (!_ClipAppliesToLayerStackSite(
                        clip, info._layerStack, info._primPathInLayerStack) 
                    || localTime < clip->startTime
                    || localTime >= clip->endTime) {
                    continue;
                }

                double upper = 0.0;
                double lower = 0.0;
                if (clip->GetBracketingTimeSamplesForPath(
                        specId, localTime, &lower, &upper)) {
                    return UsdStage_ResolveInfoAccess::_GetClipValue(
                        time, attr, info, clip, lower, upper, interpolator, 
                        result);
                }
            }
        }
    }
    else if (info._source == UsdResolveInfoSourceIsTimeDependent) {
        // In this case, we obtained a resolve info for an attribute value whose
        // value source may vary over time.  So we must fall back on invoking
        // the normal Get() machinery now that we actually have a specific time.
        return _GetValueImpl(time, attr, interpolator, result);
    }
    else if (info._source == UsdResolveInfoSourceFallback) {
        return _GetFallbackMetadata(attr, SdfFieldKeys->Default, 
                                    TfToken(), result);
    }

    return false;
}

bool 
UsdStage::_GetValueFromResolveInfo(const UsdResolveInfo &info,
                                   UsdTimeCode time, const UsdAttribute &attr,
                                   VtValue* value) const
{
    Usd_UntypedInterpolator interpolator(attr, value);
    return _GetValueFromResolveInfoImpl(
        info, time, attr, &interpolator, value);
}

template <class T>
bool 
UsdStage::_GetValueFromResolveInfo(const UsdResolveInfo &info,
                                   UsdTimeCode time, const UsdAttribute &attr,
                                   T* value) const
{
    SdfAbstractDataTypedValue<T> out(value);

    if (_interpolationType == UsdInterpolationTypeLinear) {
        typedef typename _SelectInterpolator<T>::type _Interpolator;
        _Interpolator interpolator(value);
        return _GetValueFromResolveInfoImpl<SdfAbstractDataValue>(
            info, time, attr, &interpolator, &out);
    }

    Usd_HeldInterpolator<T> interpolator(value);
    return _GetValueFromResolveInfoImpl<SdfAbstractDataValue>(
        info, time, attr, &interpolator, &out);
}

// --------------------------------------------------------------------- //
// Specialized Time Sample I/O
// --------------------------------------------------------------------- //

bool
UsdStage::_GetTimeSamplesInInterval(const UsdAttribute& attr,
                                    const GfInterval& interval,
                                    std::vector<double>* times) const
{
    UsdResolveInfo info;
    _GetResolveInfo(attr, &info);
    return _GetTimeSamplesInIntervalFromResolveInfo(info, attr, interval, times);
}

bool 
UsdStage::_GetTimeSamplesInIntervalFromResolveInfo(
    const UsdResolveInfo &info,
    const UsdAttribute &attr,
    const GfInterval& interval,
    std::vector<double>* times) const
{
    // An empty requested interval would result in in empty times
    // vector so avoid computing any of the contained samples
    if (interval.IsEmpty()) {
        return true;
    }

    // This is the lowest-level site for guaranteeing that all GetTimeSample
    // queries clear out the return vector
    times->clear();
    const auto copySamplesInInterval = [](const std::set<double>& samples, 
                                          vector<double>* target, 
                                          const GfInterval& interval) 
    {
        std::set<double>::iterator samplesBegin, samplesEnd; 

        if (interval.IsMinOpen()) {
            samplesBegin = std::upper_bound(samples.begin(), 
                                            samples.end(), 
                                            interval.GetMin()); 
        } else {
            samplesBegin = std::lower_bound(samples.begin(), 
                                            samples.end(), 
                                            interval.GetMin());
        }

        if (interval.IsMaxOpen()) {
            samplesEnd = std::lower_bound(samplesBegin,
                                          samples.end(), 
                                          interval.GetMax());
        } else {
            samplesEnd = std::upper_bound(samplesBegin,
                                          samples.end(),
                                          interval.GetMax());
        }

        target->insert(target->end(), samplesBegin, samplesEnd);
    };

    if (info._source == UsdResolveInfoSourceTimeSamples) {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());
        const SdfLayerRefPtr& layer = 
            info._layerStack->GetLayers()[info._layerIndex];

        const std::set<double> samples = layer->ListTimeSamplesForPath(specId);
        if (!samples.empty()) {
            if (info._layerToStageOffset.IsIdentity()) {
                // The layer offset is identity, so we can use the interval
                // directly, and do not need to remap the sample times.
                copySamplesInInterval(samples, times, interval);
            } else {
                // Map the interval (expressed in stage time) to layer time.
                const SdfLayerOffset stageToLayer =
                    info._layerToStageOffset.GetInverse();
                const GfInterval layerInterval =
                    interval * stageToLayer.GetScale()
                    + stageToLayer.GetOffset();
                copySamplesInInterval(samples, times, layerInterval);
                // Map the layer sample times to stage times.
                for (auto &time : *times) {
                    time = info._layerToStageOffset * time;
                }
            }
        }

        return true;
    }
    else if (info._source == UsdResolveInfoSourceValueClips ||
             info._source == UsdResolveInfoSourceIsTimeDependent) {
        const UsdPrim prim = attr.GetPrim();

        // See comments in _GetValueImpl regarding clips.
        const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());

        std::vector<double> timesFromAllClips;

        // Loop through all the clips that apply to this node and
        // combine all the time samples that are provided.
        for (const auto& clipAffectingPrim : clipsAffectingPrim) {
            for (const auto& clip : clipAffectingPrim.valueClips) {
                if (!_ClipAppliesToLayerStackSite(
                        clip, info._layerStack, info._primPathInLayerStack)) {
                    continue;
                }

                const auto clipInterval 
                    = GfInterval(clip->startTime, clip->endTime);
                
                // if we are constraining our range, and none of our range
                // intersects with the specified clip range, we can ignore
                // and move on to the next clip.
                if (!interval.Intersects(clipInterval)) {
                    continue;
                }
                
                // See comments in _GetValueImpl regarding layer
                // offsets and why they're not applied here.
                const auto samples = clip->ListTimeSamplesForPath(specId);
                if (!samples.empty()) {
                    copySamplesInInterval(samples, &timesFromAllClips, interval);
                }

                // Clips introduce time samples at their boundaries to
                // isolate them from surrounding clips, even if time samples
                // don't actually exist. 
                //
                // See _GetBracketingTimeSamplesFromResolveInfo for more
                // details.
                if (interval.Contains(clipInterval.GetMin())
                    && clipInterval.GetMin() != Usd_ClipTimesEarliest) {
                    timesFromAllClips.push_back(clip->startTime);
                }

                if (interval.Contains(clipInterval.GetMax())
                    && clipInterval.GetMax() != Usd_ClipTimesLatest){
                    timesFromAllClips.push_back(clip->endTime);
                }
            }

            if (!timesFromAllClips.empty()) {
                std::sort(
                    timesFromAllClips.begin(), timesFromAllClips.end());
                timesFromAllClips.erase(
                    std::unique(
                        timesFromAllClips.begin(), timesFromAllClips.end()),
                    timesFromAllClips.end());
                times->swap(timesFromAllClips);
                return true;
            }
        }
    }

    return true;
}

size_t
UsdStage::_GetNumTimeSamples(const UsdAttribute &attr) const
{
    UsdResolveInfo info;
    _GetResolveInfo(attr, &info);
    return _GetNumTimeSamplesFromResolveInfo(info, attr);
   
}

size_t 
UsdStage::_GetNumTimeSamplesFromResolveInfo(const UsdResolveInfo &info,
                                            const UsdAttribute &attr) const
{
    if (info._source == UsdResolveInfoSourceTimeSamples) {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());
        const SdfLayerRefPtr& layer = 
            info._layerStack->GetLayers()[info._layerIndex];

        return layer->GetNumTimeSamplesForPath(specId);
    } 
    else if (info._source == UsdResolveInfoSourceValueClips ||
             info._source == UsdResolveInfoSourceIsTimeDependent) {
        // XXX: optimization
        // 
        // We don't have an efficient way of getting the number of time
        // samples from all the clips involved. To avoid code duplication, 
        // simply get all the time samples and return the size here. 
        // 
        // This is good motivation for why we really need the ability to 
        // ask the question of whether there is more than one sample directly.
        // 
        std::vector<double> timesFromAllClips;
        _GetTimeSamplesInIntervalFromResolveInfo(info, attr, 
            GfInterval::GetFullInterval(), &timesFromAllClips);
        return timesFromAllClips.size();
    }

    return 0;
}

bool
UsdStage::_GetBracketingTimeSamples(const UsdAttribute &attr,
                                    double desiredTime,
                                    bool requireAuthored, 
                                    double* lower,
                                    double* upper,
                                    bool* hasSamples) const
{
    // If value clips might apply to this attribute, the bracketing time
    // samples will depend on whether any of those clips contain samples
    // or not. For instance, if none of the clips contain samples, the
    // correct answer is *hasSamples == false.
    //
    // This means we have to scan all clips, not just the one at the 
    // specified time. We do this by calling _GetResolveInfo without a 
    // time -- see comment above that function for details. Unfortunately,
    // this skips the optimization below, meaning we may ask layers for
    // bracketing time samples more than once.
    if (attr._Prim()->MayHaveOpinionsInClips()) {
        UsdResolveInfo resolveInfo;
        _GetResolveInfo<SdfAbstractDataValue>(attr, &resolveInfo);
        return _GetBracketingTimeSamplesFromResolveInfo(
            resolveInfo, attr, desiredTime, requireAuthored, lower, upper, 
            hasSamples);
    }

    const UsdTimeCode time(desiredTime);

    UsdResolveInfo resolveInfo;
    _ExtraResolveInfo<SdfAbstractDataValue> extraInfo;

    _GetResolveInfo<SdfAbstractDataValue>(
        attr, &resolveInfo, &time, &extraInfo);

    if (resolveInfo._source == UsdResolveInfoSourceTimeSamples) {
        // In the time samples case, we bail out early to avoid another
        // call to SdfLayer::GetBracketingTimeSamples. _GetResolveInfo will 
        // already have filled in the lower and upper samples with the
        // results of that function at the desired time.
        *lower = extraInfo.lowerSample;
        *upper = extraInfo.upperSample;

        const SdfLayerOffset offset = resolveInfo._layerToStageOffset;
        if (!offset.IsIdentity()) {
            *lower = offset * (*lower);
            *upper = offset * (*upper);
        }

        *hasSamples = true;
        return true;
    }
    
    return _GetBracketingTimeSamplesFromResolveInfo(
        resolveInfo, attr, desiredTime, requireAuthored, lower, upper, 
        hasSamples);
}

bool 
UsdStage::_GetBracketingTimeSamplesFromResolveInfo(const UsdResolveInfo &info,
                                                   const UsdAttribute &attr,
                                                   double desiredTime,
                                                   bool requireAuthored,
                                                   double* lower,
                                                   double* upper,
                                                   bool* hasSamples) const
{
    if (info._source == UsdResolveInfoSourceTimeSamples) {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());
        const SdfLayerRefPtr& layer = 
            info._layerStack->GetLayers()[info._layerIndex];
        const double layerTime =
            info._layerToStageOffset.GetInverse() * desiredTime;
        
        if (layer->GetBracketingTimeSamplesForPath(
                specId, layerTime, lower, upper)) {

            if (!info._layerToStageOffset.IsIdentity()) {
                *lower = info._layerToStageOffset * (*lower);
                *upper = info._layerToStageOffset * (*upper);
            }

            *hasSamples = true;
            return true;
        }
    }
    else if (info._source == UsdResolveInfoSourceDefault) {
        *hasSamples = false;
        return true;
    }
    else if (info._source == UsdResolveInfoSourceValueClips ||
             info._source == UsdResolveInfoSourceIsTimeDependent) {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());

        const UsdPrim prim = attr.GetPrim();

        // See comments in _GetValueImpl regarding clips.
        const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        for (const auto& clipAffectingPrim : clipsAffectingPrim) {
            for (const auto& clip : clipAffectingPrim.valueClips) {
                if (!_ClipAppliesToLayerStackSite(
                        clip, info._layerStack, info._primPathInLayerStack)
                    || desiredTime < clip->startTime
                    || desiredTime >= clip->endTime) {
                    continue;
                }
                
                // Clips introduce time samples at their boundaries even 
                // if time samples don't actually exist. This isolates each
                // clip from its neighbors and means that value resolution
                // never has to look at more than one clip to answer a
                // time sample query.
                //
                // We have to accommodate these 'fake' time samples here.
                bool foundLower = false, foundUpper = false;

                if (desiredTime == clip->startTime) {
                    *lower = *upper = clip->startTime;
                    foundLower = foundUpper = true;
                }
                else if (desiredTime == clip->endTime) {
                    *lower = *upper = clip->endTime;
                    foundLower = foundUpper = true;
                }
                else if (clip->GetBracketingTimeSamplesForPath(
                         specId, desiredTime, lower, upper)) {
                    foundLower = foundUpper = true;
                    if (*lower == *upper) {
                        if (desiredTime < *lower) {
                            foundLower = false;
                        }
                        else if (desiredTime > *upper) {
                            foundUpper = false;
                        }
                    }
                }

                if (!foundLower && 
                    clip->startTime != Usd_ClipTimesEarliest) {
                    *lower = clip->startTime;
                    foundLower = true;
                }

                if (!foundUpper && 
                    clip->endTime != Usd_ClipTimesLatest) {
                    *upper = clip->endTime;
                    foundUpper = true;
                }

                if (foundLower && !foundUpper) {
                    *upper = *lower;
                }
                else if (!foundLower && foundUpper) {
                    *lower = *upper;
                }
                
                // '||' is correct here. Consider the case where we only
                // have a single clip and desiredTime is earlier than the
                // first time sample -- foundLower will be false, but we
                // want to return the bracketing samples from the sole
                // clip anyway.
                if (foundLower || foundUpper) {
                    *hasSamples = true;
                    return true;
                }
            }
        }
    }
    else if (info._source == UsdResolveInfoSourceFallback) {
        // At this point, no authored value was found, so if the client only 
        // wants authored values, we can exit.
        *hasSamples = false;
        if (requireAuthored)
            return false;

        // Check for a registered fallback.
        if (SdfAttributeSpecHandle attrDef = _GetAttributeDefinition(attr)) {
            if (attrDef->HasDefaultValue()) {
                *hasSamples = false;
                return true;
            }
        }
    }

    // No authored value, no fallback.
    return false;
}

static bool
_ValueFromClipsMightBeTimeVarying(const Usd_ClipRefPtr &firstClipWithSamples,
                                  const SdfAbstractDataSpecId &attrSpecId)
{
    // If the first clip is active over all time (i.e., it is the only 
    // clip that affects this attribute) and it has more than one time
    // sample, then it might be time varying. If it only has one sample,
    // its value must be constant over all time.
    if (firstClipWithSamples->startTime == Usd_ClipTimesEarliest
        && firstClipWithSamples->endTime == Usd_ClipTimesLatest) {
        return firstClipWithSamples->GetNumTimeSamplesForPath(attrSpecId) > 1;
    }

    // Since this clip isn't active over all time, we must have more clips.
    // Because Usd doesn't hold values across clip boundaries, we can't
    // say for certain that the value will be constant across all time.
    // So, we have to report that the value might be time varying.
    return true;
}

bool 
UsdStage::_ValueMightBeTimeVarying(const UsdAttribute &attr) const
{
    UsdResolveInfo info;
    _ExtraResolveInfo<SdfAbstractDataValue> extraInfo;
    _GetResolveInfo(attr, &info, NULL, &extraInfo);

    if (info._source == UsdResolveInfoSourceValueClips ||
        info._source == UsdResolveInfoSourceIsTimeDependent) {
        // See comment in _ValueMightBeTimeVaryingFromResolveInfo.
        // We can short-cut the work in that function because _GetResolveInfo
        // gives us the first clip that has time samples for this attribute.
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());
        return _ValueFromClipsMightBeTimeVarying(extraInfo.clip, specId);
    }

    return _ValueMightBeTimeVaryingFromResolveInfo(info, attr);
}

bool 
UsdStage::_ValueMightBeTimeVaryingFromResolveInfo(const UsdResolveInfo &info,
                                                  const UsdAttribute &attr) const
{
    if (info._source == UsdResolveInfoSourceValueClips ||
        info._source == UsdResolveInfoSourceIsTimeDependent) {
        // In the case that the attribute value comes from a value clip, we
        // need to find the first clip that has samples for attr to see if the
        // clip values may be time varying. This is potentially much more 
        // efficient than the _GetNumTimeSamples check below, since that 
        // requires us to open every clip to get the time sample count.
        //
        // Note that we still wind up checking every clip if none of them
        // have samples for this attribute.
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());

        const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(attr.GetPrim().GetPath());
        for (const auto& clipAffectingPrim : clipsAffectingPrim) {
            for (const auto& clip : clipAffectingPrim.valueClips) {
                if (_ClipAppliesToLayerStackSite(
                        clip, info._layerStack, info._primPathInLayerStack)
                    && _HasTimeSamples(clip, specId)) {
                    return _ValueFromClipsMightBeTimeVarying(clip, specId);
                }
            }
        }
        
        return false;
    }

    return _GetNumTimeSamplesFromResolveInfo(info, attr) > 1;
}

static
bool 
_HasLayerFieldOrDictKey(const SdfLayerHandle &layer, const TfToken &key, 
                        const TfToken &keyPath, VtValue *val)
{
    return keyPath.IsEmpty() ?
        layer->HasField(SdfPath::AbsoluteRootPath(), key, val) :
        layer->HasFieldDictKey(SdfPath::AbsoluteRootPath(), key, keyPath, val);
}

static
bool
_HasStageMetadataOrDictKey(const UsdStage &stage, 
                           const TfToken &key, const TfToken &keyPath,
                           VtValue *value)
{
    SdfLayerHandle sessionLayer = stage.GetSessionLayer();
    if (sessionLayer && 
        _HasLayerFieldOrDictKey(sessionLayer, key, keyPath, value)){
        VtValue rootValue;
        if (value && 
            value->IsHolding<VtDictionary>() &&
            _HasLayerFieldOrDictKey(stage.GetRootLayer(), key, keyPath, 
                                    &rootValue) && 
            rootValue.IsHolding<VtDictionary>() ){
            const VtDictionary &rootDict = rootValue.UncheckedGet<VtDictionary>();
            VtDictionary dict;
            value->UncheckedSwap<VtDictionary>(dict);
            VtDictionaryOverRecursive(&dict, rootDict);
            value->UncheckedSwap<VtDictionary>(dict);
        }

        return true;
    }
     
    return _HasLayerFieldOrDictKey(stage.GetRootLayer(), key, keyPath, value);
}

bool
UsdStage::GetMetadata(const TfToken &key, VtValue *value) const
{
    if (!value){
        TF_CODING_ERROR(
            "Null out-param 'value' for UsdStage::GetMetadata(\"%s\")",
            key.GetText());
        return false;

    }
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (!schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot)){
        return false;
    }
    
    if (!_HasStageMetadataOrDictKey(*this, key, TfToken(), value)){
        *value = SdfSchema::GetInstance().GetFallback(key);
    } 
    else if (value->IsHolding<VtDictionary>()){
        const VtDictionary &fallback = SdfSchema::GetInstance().GetFallback(key).Get<VtDictionary>();
        
        VtDictionary dict;
        value->UncheckedSwap<VtDictionary>(dict);
        VtDictionaryOverRecursive(&dict, fallback);
        value->UncheckedSwap<VtDictionary>(dict);
    }
    return true;
}

bool
UsdStage::HasMetadata(const TfToken &key) const
{
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (!schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
        return false;

    return (HasAuthoredMetadata(key) ||
            !schema.GetFallback(key).IsEmpty());
}

bool
UsdStage::HasAuthoredMetadata(const TfToken& key) const
{
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (!schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
        return false;

    return _HasStageMetadataOrDictKey(*this, key, TfToken(), NULL);
}

static
void
_SetLayerFieldOrDictKey(const SdfLayerHandle &layer, const TfToken &key, 
                        const TfToken &keyPath, const VtValue &val)
{
    if (keyPath.IsEmpty()) {
        layer->SetField(SdfPath::AbsoluteRootPath(), key, val);
    } else {
        layer->SetFieldDictValueByKey(SdfPath::AbsoluteRootPath(), 
                                      key, keyPath, val);
    }
}

static
void
_ClearLayerFieldOrDictKey(const SdfLayerHandle &layer, const TfToken &key, 
                          const TfToken &keyPath)
{
    if (keyPath.IsEmpty()) {
        layer->EraseField(SdfPath::AbsoluteRootPath(), key);
    } else {
        layer->EraseFieldDictValueByKey(SdfPath::AbsoluteRootPath(), 
                                        key, keyPath);
    }
}

static
bool
_SetStageMetadataOrDictKey(const UsdStage &stage, const TfToken &key,
                           const TfToken &keyPath, const VtValue &val)
{
    SdfLayerHandle rootLayer = stage.GetRootLayer();
    SdfLayerHandle sessionLayer = stage.GetSessionLayer();
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (!schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot)) {
        TF_CODING_ERROR("Metadata '%s' is not registered as valid Layer "
                        "metadata, and cannot be set on UsdStage %s.",
                        key.GetText(),
                        rootLayer->GetIdentifier().c_str());
        return false;
    }

    const SdfLayerHandle &editTargetLayer = stage.GetEditTarget().GetLayer();
    if (editTargetLayer == rootLayer || editTargetLayer == sessionLayer) {
        _SetLayerFieldOrDictKey(editTargetLayer, key, keyPath, val);
    } else {
        TF_CODING_ERROR("Cannot set layer metadata '%s' in current edit "
                        "target \"%s\", as it is not the root layer or "
                        "session layer of stage \"%s\".",
                        key.GetText(),
                        editTargetLayer->GetIdentifier().c_str(),
                        rootLayer->GetIdentifier().c_str());
        return false;
    }

    return true;
}

bool
UsdStage::SetMetadata(const TfToken &key, const VtValue &value) const
{
    return _SetStageMetadataOrDictKey(*this, key, TfToken(), value);
}


static
bool
_ClearStageMetadataOrDictKey(const UsdStage &stage, const TfToken &key,
                        const TfToken &keyPath)
{
    SdfLayerHandle rootLayer = stage.GetRootLayer();
    SdfLayerHandle sessionLayer = stage.GetSessionLayer();
    const SdfSchema &schema = SdfSchema::GetInstance();

    if (!schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot)) {
        TF_CODING_ERROR("Metadata '%s' is not registered as valid Layer "
                        "metadata, and cannot be cleared on UsdStage %s.",
                        key.GetText(),
                        rootLayer->GetIdentifier().c_str());
        return false;
    }

    const SdfLayerHandle &editTargetLayer = stage.GetEditTarget().GetLayer();
    if (editTargetLayer == rootLayer || editTargetLayer == sessionLayer) {
        _ClearLayerFieldOrDictKey(editTargetLayer, key, keyPath);
    } else {
        TF_CODING_ERROR("Cannot clear layer metadata '%s' in current edit "
                        "target \"%s\", as it is not the root layer or "
                        "session layer of stage \"%s\".",
                        key.GetText(),
                        editTargetLayer->GetIdentifier().c_str(),
                        rootLayer->GetIdentifier().c_str());
        return false;
    }

    return true;
}

bool
UsdStage::ClearMetadata(const TfToken &key) const
{
    return _ClearStageMetadataOrDictKey(*this, key, TfToken());
}

bool 
UsdStage::GetMetadataByDictKey(const TfToken& key, const TfToken &keyPath,
                               VtValue *value) const
{
    if (keyPath.IsEmpty())
        return false;
    
    if (!value){
        TF_CODING_ERROR(
            "Null out-param 'value' for UsdStage::GetMetadataByDictKey"
            "(\"%s\", \"%s\")",
            key.GetText(), keyPath.GetText());
        return false;

    }
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (!schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
        return false;

    if (!_HasStageMetadataOrDictKey(*this, key, keyPath, value)){
        const VtValue &fallback =  SdfSchema::GetInstance().GetFallback(key);
        if (!fallback.IsEmpty()){
            const VtValue *elt = fallback.Get<VtDictionary>().
                GetValueAtPath(keyPath);
            if (elt){
                *value = *elt;
                return true;
            }
        }
        return false;
    }
    else if (value->IsHolding<VtDictionary>()){
        const VtDictionary &fallback = SdfSchema::GetInstance().GetFallback(key).Get<VtDictionary>();
        const VtValue *elt = fallback.GetValueAtPath(keyPath);
        if (elt && elt->IsHolding<VtDictionary>()){
            VtDictionary dict;
            value->UncheckedSwap<VtDictionary>(dict);
            VtDictionaryOverRecursive(&dict, elt->UncheckedGet<VtDictionary>());
            value->UncheckedSwap<VtDictionary>(dict);
        }
   }

    return true;
}

bool 
UsdStage::HasMetadataDictKey(const TfToken& key, const TfToken &keyPath) const
{
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (keyPath.IsEmpty() ||
        !schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
        return false;

    if (HasAuthoredMetadataDictKey(key, keyPath))
        return true;

    const VtValue &fallback =  schema.GetFallback(key);
    
    return ((!fallback.IsEmpty()) &&
            (fallback.Get<VtDictionary>().GetValueAtPath(keyPath) != NULL));
}

bool
UsdStage::HasAuthoredMetadataDictKey(
    const TfToken& key, const TfToken &keyPath) const
{
    if (keyPath.IsEmpty())
        return false;

    return _HasStageMetadataOrDictKey(*this, key, keyPath, NULL);
}

bool
UsdStage::SetMetadataByDictKey(
    const TfToken& key, const TfToken &keyPath, const VtValue& value) const
{
    if (keyPath.IsEmpty())
        return false;
    
    return _SetStageMetadataOrDictKey(*this, key, keyPath, value);
}

bool
UsdStage::ClearMetadataByDictKey(
        const TfToken& key, const TfToken& keyPath) const
{
    if (keyPath.IsEmpty())
        return false;
    
    return _ClearStageMetadataOrDictKey(*this, key, keyPath);
}

///////////////////////////////////////////////////////////////////////////////
// XXX(Frame->Time): backwards compatibility
// Temporary helper functions to support backwards compatibility.
///////////////////////////////////////////////////////////////////////////////

static
bool
_HasStartFrame(const SdfLayerConstHandle &layer)
{
    return layer->GetPseudoRoot()->HasInfo(SdfFieldKeys->StartFrame);
}

static
bool
_HasEndFrame(const SdfLayerConstHandle &layer)
{
    return layer->GetPseudoRoot()->HasInfo(SdfFieldKeys->EndFrame);
}

static
double
_GetStartFrame(const SdfLayerConstHandle &layer)
{
    VtValue startFrame = layer->GetPseudoRoot()->GetInfo(SdfFieldKeys->StartFrame);
    if (startFrame.IsHolding<double>())
        return startFrame.UncheckedGet<double>();
    return 0.0;
}

static
double
_GetEndFrame(const SdfLayerConstHandle &layer)
{
    VtValue endFrame = layer->GetPseudoRoot()->GetInfo(SdfFieldKeys->EndFrame);
    if (endFrame.IsHolding<double>())
        return endFrame.UncheckedGet<double>();
    return 0.0;
}

//////////////////////////////////////////////////////////////////////////////

// XXX bug/123508 - Once we can remove backwards compatibility with
// startFrame/endFrame, these methods can become as simple as those for
// TimeCodesPerSecond and FramesPerSecond
double
UsdStage::GetStartTimeCode() const
{
    // Look for 'startTimeCode' first. If it is not available, then look for 
    // the deprecated field 'startFrame'.
    const SdfLayerConstHandle sessionLayer = GetSessionLayer();
    if (sessionLayer) {
        if (sessionLayer->HasStartTimeCode())
            return sessionLayer->GetStartTimeCode();
        else if (_HasStartFrame(sessionLayer))
            return _GetStartFrame(sessionLayer);
    }

    if (GetRootLayer()->HasStartTimeCode())
        return GetRootLayer()->GetStartTimeCode();
    return _GetStartFrame(GetRootLayer());
}

void
UsdStage::SetStartTimeCode(double startTime)
{
    SetMetadata(SdfFieldKeys->StartTimeCode, startTime);
}

double
UsdStage::GetEndTimeCode() const
{
    // Look for 'endTimeCode' first. If it is not available, then look for 
    // the deprecated field 'startFrame'.
    const SdfLayerConstHandle sessionLayer = GetSessionLayer();
    if (sessionLayer) {
        if (sessionLayer->HasEndTimeCode())
            return sessionLayer->GetEndTimeCode();
        else if (_HasEndFrame(sessionLayer))
            return _GetEndFrame(sessionLayer);
    }

    if (GetRootLayer()->HasEndTimeCode())
        return GetRootLayer()->GetEndTimeCode();
    return _GetEndFrame(GetRootLayer());
}

void
UsdStage::SetEndTimeCode(double endTime)
{
    SetMetadata(SdfFieldKeys->EndTimeCode, endTime);
}

bool
UsdStage::HasAuthoredTimeCodeRange() const
{
    SdfLayerHandle rootLayer = GetRootLayer();
    SdfLayerHandle sessionLayer = GetSessionLayer();

    return (sessionLayer && 
            ((sessionLayer->HasStartTimeCode() && sessionLayer->HasEndTimeCode()) ||
             (_HasStartFrame(sessionLayer) && _HasEndFrame(sessionLayer)))) || 
           (rootLayer && 
            ((rootLayer->HasStartTimeCode() && rootLayer->HasEndTimeCode()) ||
             (_HasStartFrame(rootLayer) && _HasEndFrame(rootLayer))));
}

double 
UsdStage::GetTimeCodesPerSecond() const
{
    // We expect the SdfSchema to provide a fallback, so simply:
    double result = 0;
    GetMetadata(SdfFieldKeys->TimeCodesPerSecond, &result);
    return result;
}

void 
UsdStage::SetTimeCodesPerSecond(double timeCodesPerSecond) const
{
    SetMetadata(SdfFieldKeys->TimeCodesPerSecond, timeCodesPerSecond);
}

double 
UsdStage::GetFramesPerSecond() const
{
    // We expect the SdfSchema to provide a fallback, so simply:
    double result = 0;
    GetMetadata(SdfFieldKeys->FramesPerSecond, &result);
    return result;
}

void 
UsdStage::SetFramesPerSecond(double framesPerSecond) const
{
    SetMetadata(SdfFieldKeys->FramesPerSecond, framesPerSecond);
}

void 
UsdStage::SetColorConfiguration(const SdfAssetPath &colorConfig) const
{
    SetMetadata(SdfFieldKeys->ColorConfiguration, colorConfig);
}

SdfAssetPath 
UsdStage::GetColorConfiguration() const
{
    SdfAssetPath colorConfig;
    GetMetadata(SdfFieldKeys->ColorConfiguration, &colorConfig);

    return colorConfig.GetAssetPath().empty() ? 
        _colorConfigurationFallbacks->first : colorConfig;
}

void 
UsdStage::SetColorManagementSystem(const TfToken &cms) const
{
    SetMetadata(SdfFieldKeys->ColorManagementSystem, cms);
}

TfToken
UsdStage::GetColorManagementSystem() const
{
    TfToken cms;
    GetMetadata(SdfFieldKeys->ColorManagementSystem, &cms);

    return cms.IsEmpty() ? _colorConfigurationFallbacks->second : cms;
}

/* static */
void 
UsdStage::GetColorConfigFallbacks(
    SdfAssetPath *colorConfiguration,
    TfToken *colorManagementSystem)
{
    if (colorConfiguration) {
        *colorConfiguration = _colorConfigurationFallbacks->first;
    }
    if (colorManagementSystem) {
        *colorManagementSystem = _colorConfigurationFallbacks->second;
    }
}

/* static */
void
UsdStage::SetColorConfigFallbacks(
    const SdfAssetPath &colorConfiguration, 
    const TfToken &colorManagementSystem)
{
    if (!colorConfiguration.GetAssetPath().empty())
        _colorConfigurationFallbacks->first = colorConfiguration;
    if (!colorManagementSystem.IsEmpty())
        _colorConfigurationFallbacks->second = colorManagementSystem;
}

std::string
UsdStage::ResolveIdentifierToEditTarget(std::string const &identifier) const
{
    const SdfLayerHandle &anchor = _editTarget.GetLayer();
    
    // This check finds anonymous layers, which we consider to always resolve
    if (SdfLayerHandle lyr = SdfLayer::Find(identifier)){
        if (lyr->IsAnonymous()){
            TF_DEBUG(USD_PATH_RESOLUTION).Msg("Resolved identifier %s because "
                                              "it was anonymous\n",
                                              identifier.c_str());
            return identifier;
        }
        else if (anchor->IsAnonymous() && 
                 ArGetResolver().IsRelativePath(identifier)){
            TF_DEBUG(USD_PATH_RESOLUTION).Msg("Cannot resolve identifier %s "
                                              "because anchoring layer %s is"
                                              "anonymous\n",
                                              identifier.c_str(),
                                              anchor->GetIdentifier().c_str());
            return std::string();
        }
    }
    
    ArResolverContextBinder binder(GetPathResolverContext());

    // Handles non-relative paths also
    const std::string resolved = 
        _ResolveAssetPathRelativeToLayer(anchor, identifier);
    TF_DEBUG(USD_PATH_RESOLUTION).Msg("Resolved identifier \"%s\" against layer "
                                      "@%s@ to: \"%s\"\n",
                                      identifier.c_str(), 
                                      anchor->GetIdentifier().c_str(), 
                                      resolved.c_str());
    return resolved;
}

void 
UsdStage::SetInterpolationType(UsdInterpolationType interpolationType)
{
    if (_interpolationType != interpolationType) {
        _interpolationType = interpolationType;

        // Emit StageContentsChanged, as interpolated attributes values
        // have likely changed.
        UsdStageWeakPtr self(this);
        UsdNotice::StageContentsChanged(self).Send(self);
    }
}

UsdInterpolationType 
UsdStage::GetInterpolationType() const
{
    return _interpolationType;
}

std::string UsdDescribe(const UsdStage *stage) {
    if (!stage) {
        return "null stage";
    } else {
        return TfStringPrintf(
            "stage with rootLayer @%s@%s",
            stage->GetRootLayer()->GetIdentifier().c_str(),
            (stage->GetSessionLayer() ? TfStringPrintf(
                ", sessionLayer @%s@", stage->GetSessionLayer()->
                GetIdentifier().c_str()).c_str() : ""));
    }
}

std::string UsdDescribe(const UsdStage &stage) {
    return UsdDescribe(&stage);
}

std::string UsdDescribe(const UsdStagePtr &stage) {
    return UsdDescribe(get_pointer(stage));
}

std::string UsdDescribe(const UsdStageRefPtr &stage) {
    return UsdDescribe(get_pointer(stage));
}

// Explicitly instantiate templated getters for all Sdf value
// types.
#define _INSTANTIATE_GET(r, unused, elem)                               \
    template bool UsdStage::_GetValue(                                  \
        UsdTimeCode, const UsdAttribute&,                               \
        SDF_VALUE_CPP_TYPE(elem)*) const;                               \
    template bool UsdStage::_GetValue(                                  \
        UsdTimeCode, const UsdAttribute&,                               \
        SDF_VALUE_CPP_ARRAY_TYPE(elem)*) const;                         \
                                                                        \
    template bool UsdStage::_GetValueFromResolveInfo(                   \
        const UsdResolveInfo&, UsdTimeCode, const UsdAttribute&,        \
        SDF_VALUE_CPP_TYPE(elem)*) const;                               \
    template bool UsdStage::_GetValueFromResolveInfo(                   \
        const UsdResolveInfo&, UsdTimeCode, const UsdAttribute&,        \
        SDF_VALUE_CPP_ARRAY_TYPE(elem)*) const;                      

BOOST_PP_SEQ_FOR_EACH(_INSTANTIATE_GET, ~, SDF_VALUE_TYPES)
#undef _INSTANTIATE_GET

PXR_NAMESPACE_CLOSE_SCOPE

