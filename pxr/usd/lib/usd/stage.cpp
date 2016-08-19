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
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/clipCache.h"
#include "pxr/usd/usd/conversions.h"
#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/interpolators.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/resolveInfo.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stageCache.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usd/usdFileFormat.h"

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

#include "pxr/base/tracelite/trace.h"
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
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/scoped.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/utils.h"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/utility/in_place_factory.hpp>

#include <tbb/spin_rw_mutex.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

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

//
// Usd lets you configure the fallback variants to use in plugInfo.json.
// This static data goes to discover that on first access.
//
TF_MAKE_STATIC_DATA(PcpVariantFallbackMap, _usdGlobalVariantFallbackMap)
{
    PcpVariantFallbackMap fallbacks;

    PlugPluginPtrVector plugs = PlugRegistry::GetInstance().GetAllPlugins();
    TF_FOR_ALL(plugIter, plugs) {
        PlugPluginPtr plug = *plugIter;
        JsObject metadata = plug->GetMetadata();
        JsValue dictVal;
        if (TfMapLookup(metadata, "UsdVariantFallbacks", &dictVal)) {
            if (not dictVal.Is<JsObject>()) {
                TF_CODING_ERROR(
                        "%s[UsdVariantFallbacks] was not a dictionary.",
                        plug->GetName().c_str());
                continue;
            }
            JsObject dict = dictVal.Get<JsObject>();
            TF_FOR_ALL(d, dict) {
                std::string vset = d->first;
                if (not d->second.IsArray()) {
                    TF_CODING_ERROR(
                            "%s[UsdVariantFallbacks] value for %s must "
                            "be an arrays.",
                            plug->GetName().c_str(), vset.c_str());
                    continue;
                }
                std::vector<std::string> vsels =
                    d->second.GetArrayOf<std::string>();
                if (not vsels.empty()) {
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
_GetLayerOffsetToRoot(const PcpNodeRef& pcpNode,
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

    // PERFORMANCE: GetLayerOffsetForLayer() is seems fairly cheap (because the
    // offsets are cached), however it requires iterating over every layer in
    // the stack calling SdfLayerOffset::IsIdentity.
    if (const SdfLayerOffset *layerToRootLayerOffset =
        pcpNode.GetLayerStack()->GetLayerOffsetForLayer(layer)) {
        localOffset = localOffset * (*layerToRootLayerOffset);
    }

    // NOTE: FPS is intentionally excluded here; in Usd FPS is treated as pure
    // metadata, and does not factor into the layer offset scale. Additionally,
    // it is a validation error to compose mixed frame rates. This was done as a
    // performance optimization.

    return localOffset;
}

// Make a copy of paths, but uniqued with a prefix-check, which
// removes all elements that are prefixed by other elements.
template <class T>
static void
_CopyAndRemoveDescendentPaths(const T& paths, SdfPathVector* outPaths)
{
    using std::unique_copy;
    using boost::bind;

    outPaths->reserve(paths.size());

    // Unique with an equivalence predicate that checks if the rhs has the lhs
    // as a prefix.  If so, it's considered equivalent and therefore elided by
    // unique_copy.  This leaves outPaths in a state where it contains
    // no path that is descendant to any other.  Said another way, for all paths
    // 'p' in pathVecToRecompose, there does not exist another (different) path
    // 'q' also in pathVecToRecompose such that p.HasPrefix(q).
    unique_copy(paths.begin(), paths.end(),
                back_inserter(*outPaths),
                bind(&SdfPath::HasPrefix, _2, _1));
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
    if (layer and not layer->IsAnonymous()) {
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
_ResolveAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& assetPath)
{
    if (assetPath.empty() or 
        SdfLayer::IsAnonymousLayerIdentifier(assetPath)) {
        return assetPath;
    }

    const std::string computedAssetPath = 
        SdfComputeAssetPathRelativeToLayer(anchor, assetPath);
    if (computedAssetPath.empty()) {
        return computedAssetPath;
    }

    return ArGetResolver().Resolve(computedAssetPath);
}

void
UsdStage::_MakeResolvedAssetPaths(UsdTimeCode time,
                                  const UsdAttribute& attr,
                                  SdfAssetPath *assetPaths,
                                  size_t numAssetPaths) const
{
    auto anchor = _GetLayerWithStrongestValue(time, attr);
    auto context = GetPathResolverContext();

    // Get the layer providing the strongest value and use that to anchor the
    // resolve.
    if (anchor) {
        ArResolverContextBinder binder(context);
        for (size_t i = 0; i != numAssetPaths; ++i) {
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
                                  VtValue* value) const
{
    if (value->IsHolding<SdfAssetPath>()) {
        SdfAssetPath assetPath;
        value->UncheckedSwap(assetPath);
        _MakeResolvedAssetPaths(time, attr, &assetPath, 1);
        value->UncheckedSwap(assetPath);
            
    }
    else if (value->IsHolding<VtArray<SdfAssetPath>>()) {
        VtArray<SdfAssetPath> assetPaths;
        value->UncheckedSwap(assetPaths);
        _MakeResolvedAssetPaths(time, attr, assetPaths.data(), 
                                assetPaths.size());
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
                   const ArResolverContext& pathResolverContext)
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
    , _isClosingStage(false)
{
    if (not TF_VERIFY(_rootLayer))
        return;

    TF_DEBUG(USD_STAGE_LIFETIMES).Msg(
        "UsdStage::UsdStage(rootLayer=@%s@, sessionLayer=@%s@)\n",
        _rootLayer->GetIdentifier().c_str(),
        _sessionLayer ? _sessionLayer->GetIdentifier().c_str() : "<null>");

    _mallocTagID = TfMallocTag::IsInitialized() ?
        strdup(::_StageTag(rootLayer->GetIdentifier()).c_str()) :
        ::_dormantMallocTagID;

    _cache->SetVariantFallbacks(GetGlobalVariantFallbacks());
}

UsdStage::~UsdStage()
{
    TF_DEBUG(USD_STAGE_LIFETIMES).Msg(
        "UsdStage::~UsdStage(rootLayer=@%s@, sessionLayer=@%s@)\n",
        _rootLayer ? _rootLayer->GetIdentifier().c_str() : "<null>",
        _sessionLayer ? _sessionLayer->GetIdentifier().c_str() : "<null>");
    Close();
    if (_mallocTagID != ::_dormantMallocTagID){
        free(const_cast<char*>(_mallocTagID));
    }
}

void
UsdStage::Close()
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
    explicit _NameChildrenPred(Usd_InstanceCache* instanceCache)
        : _instanceCache(instanceCache)
    { }

    bool operator()(const PcpPrimIndex &index) const {
        // Use a resolver to walk the index and find the strongest active
        // opinion.
        Usd_Resolver res(&index);
        for (; res.IsValid(); res.NextLayer()) {
            bool active = true;
            if (res.GetLayer()->HasField(
                    res.GetLocalPath(), SdfFieldKeys->Active, &active)) {
                if (not active) {
                    return false;
                }
                break;
            }
        }

        // UsdStage doesn't expose any prims beneath instances, so we don't
        // need to compute indexes for children of instances unless the
        // index will be used as a source for a master prim.
        if (index.IsInstanceable()) {
            const bool indexUsedAsMasterSource = 
                _instanceCache->RegisterInstancePrimIndex(index)
                or not _instanceCache->GetMasterUsingPrimIndexAtPath(
                    index.GetPath()).IsEmpty();
            return indexUsedAsMasterSource;
        }

        return true;
    }

private:
    Usd_InstanceCache* _instanceCache;
};

} // anon

/* static */
UsdStageRefPtr
UsdStage::_InstantiateStage(const SdfLayerRefPtr &rootLayer,
                            const SdfLayerRefPtr &sessionLayer,
                            const ArResolverContext &pathResolverContext,
                            InitialLoadSet load)
{
    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::_InstantiateStage: Creating new UsdStage\n");

    // We don't want to pay for the tag-string construction unless
    // we instrumentation is on, since some Stage ctors (InMemory) can be
    // very lightweight.
    boost::optional<TfAutoMallocTag2> tag;

    if (TfMallocTag::IsInitialized()){
        tag = boost::in_place("Usd", ::_StageTag(rootLayer->GetIdentifier()));
    }

    // Debug timing info
    boost::optional<TfStopwatch> stopwatch;
    const bool usdInstantiationTimeDebugCodeActive = 
        TfDebug::IsEnabled(USD_STAGE_INSTANTIATION_TIME);

    if (usdInstantiationTimeDebugCodeActive) {
        stopwatch = TfStopwatch(); 
        stopwatch->Start();
    }

    if (not rootLayer)
        return TfNullPtr;

    UsdStageRefPtr stage = TfCreateRefPtr(
        new UsdStage(rootLayer, sessionLayer, pathResolverContext));

    ArResolverScopedCache resolverCache;

    // Minimally populate the stage, do not request payloads.
    stage->_ComposePrimIndexesInParallel(
        SdfPathVector(1, SdfPath::AbsoluteRootPath()), "Instantiating stage");
    stage->_pseudoRoot = stage->_InstantiatePrim(SdfPath::AbsoluteRootPath());
    stage->_ComposeSubtreeInParallel(stage->_pseudoRoot);
    stage->_RegisterPerLayerNotices();

    // Include all payloads, if desired.
    if (load == LoadAll) {
        // we will ignore the aggregation of loads/unloads 
        // because we won't be using them to send a notification
        SdfPathSet include, exclude;

        include.insert(SdfPath::AbsoluteRootPath());
        stage->_LoadAndUnload(include, exclude, NULL, NULL);
    }

    // Publish this stage into all current writable caches.
    BOOST_FOREACH(UsdStageCache *cache,
                  UsdStageCacheContext::_GetWritableCaches()) {
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
    if (not rootLayer) {
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
UsdStage::CreateNew(const std::string& identifier)
{
    TfAutoMallocTag2 tag("Usd", ::_StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, _CreateAnonymousSessionLayer(layer));
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    const SdfLayerHandle& sessionLayer)
{
    TfAutoMallocTag2 tag("Usd", ::_StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, sessionLayer);
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    const ArResolverContext& pathResolverContext)
{
    TfAutoMallocTag2 tag("Usd", ::_StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, pathResolverContext);
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    const SdfLayerHandle& sessionLayer,
                    const ArResolverContext& pathResolverContext)
{
    TfAutoMallocTag2 tag("Usd", ::_StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, sessionLayer, pathResolverContext);
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory()
{
    // Use usda file format if an identifier was not provided.
    //
    // In regards to "tmp.usda" below, SdfLayer::CreateAnonymous always
    // prefixes the identifier with the layer's address in memory, so using the
    // same identifier multiple times still produces unique layers.
    return CreateInMemory("tmp.usda");
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(const std::string& identifier)
{
    return Open(SdfLayer::CreateAnonymous(identifier));
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(const std::string& identifier,
                         const ArResolverContext& pathResolverContext)
{
    // CreateAnonymous() will transform 'identifier', so don't bother
    // using it as a tag
    TfAutoMallocTag tag("Usd");
    
    return Open(SdfLayer::CreateAnonymous(identifier), pathResolverContext);
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(const std::string& identifier,
                         const SdfLayerHandle &sessionLayer)
{
    // CreateAnonymous() will transform 'identifier', so don't bother
    // using it as a tag
    TfAutoMallocTag tag("Usd");
    
    return Open(SdfLayer::CreateAnonymous(identifier), sessionLayer);
}

/* static */
UsdStageRefPtr
UsdStage::CreateInMemory(const std::string& identifier,
                         const SdfLayerHandle &sessionLayer,
                         const ArResolverContext& pathResolverContext)
{
    // CreateAnonymous() will transform 'identifier', so don't bother
    // using it as a tag
    TfAutoMallocTag tag("Usd");
    
    return Open(SdfLayer::CreateAnonymous(identifier),
                sessionLayer, pathResolverContext);
}

static
SdfLayerRefPtr
_OpenLayer(
    const std::string &filePath,
    const ArResolverContext &resolverContext = ArResolverContext())
{
    boost::optional<ArResolverContextBinder> binder;
    if (not resolverContext.IsEmpty())
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
    TfAutoMallocTag2 tag("Usd", ::_StageTag(filePath));

    SdfLayerRefPtr rootLayer = _OpenLayer(filePath);
    if (not rootLayer) {
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
    TfAutoMallocTag2 tag("Usd", ::_StageTag(filePath));

    SdfLayerRefPtr rootLayer = _OpenLayer(filePath, pathResolverContext);
    if (not rootLayer) {
        TF_RUNTIME_ERROR("Failed to open layer @%s@", filePath.c_str());
        return TfNullPtr;
    }
    return Open(rootLayer, pathResolverContext, load);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const SdfLayerHandle& rootLayer, InitialLoadSet load)
{
    if (not rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::Open(rootLayer=@%s@, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             TfStringify(load).c_str());

    // Try to find a matching stage in any caches.
    BOOST_FOREACH(const UsdStageCache *cache,
                  UsdStageCacheContext::_GetReadableCaches()) {
        if (UsdStageRefPtr stage = cache->FindOneMatching(rootLayer))
            return stage;
    }

    // No cached stages.  Make a new stage, and populate caches with it.
    return _InstantiateStage(
        rootLayer,
        _CreateAnonymousSessionLayer(rootLayer),
        _CreatePathResolverContext(rootLayer),
        load);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const SdfLayerHandle& rootLayer,
               const SdfLayerHandle& sessionLayer,
               InitialLoadSet load)
{
    if (not rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::Open(rootLayer=@%s@, sessionLayer=@%s@, load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             sessionLayer ? sessionLayer->GetIdentifier().c_str() : "<null>",
             TfStringify(load).c_str());

    // Try to find a matching stage in any caches.
    BOOST_FOREACH(const UsdStageCache *cache,
                  UsdStageCacheContext::_GetReadableCaches()) {
        if (UsdStageRefPtr stage =
            cache->FindOneMatching(rootLayer, sessionLayer)) {
            return stage;
        }
    }

    // No cached stages.  Make a new stage, and populate caches with it.
    return _InstantiateStage(
        rootLayer,
        sessionLayer,
        _CreatePathResolverContext(rootLayer),
        load);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const SdfLayerHandle& rootLayer,
               const ArResolverContext& pathResolverContext,
               InitialLoadSet load)
{
    if (not rootLayer) {
        TF_CODING_ERROR("Invalid root layer");
        return TfNullPtr;
    }

    TF_DEBUG(USD_STAGE_OPEN)
        .Msg("UsdStage::Open(rootLayer=@%s@, pathResolverContext=%s, "
                            "load=%s)\n",
             rootLayer->GetIdentifier().c_str(),
             pathResolverContext.GetDebugString().c_str(), 
             TfStringify(load).c_str());

    // Try to find a matching stage in any caches.
    BOOST_FOREACH(const UsdStageCache *cache,
                  UsdStageCacheContext::_GetReadableCaches()) {
        if (UsdStageRefPtr stage =
            cache->FindOneMatching(rootLayer, pathResolverContext)) {
            return stage;
        }
    }

    // No cached stages.  Make a new stage, and populate caches with it.
    return _InstantiateStage(
        rootLayer,
        _CreateAnonymousSessionLayer(rootLayer),
        pathResolverContext,
        load);
}

/* static */
UsdStageRefPtr
UsdStage::Open(const SdfLayerHandle& rootLayer,
               const SdfLayerHandle& sessionLayer,
               const ArResolverContext& pathResolverContext,
               InitialLoadSet load)
{
    if (not rootLayer) {
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

    // Try to find a matching stage in any caches.
    BOOST_FOREACH(const UsdStageCache *cache,
                  UsdStageCacheContext::_GetReadableCaches()) {
        if (UsdStageRefPtr stage = cache->FindOneMatching(
                rootLayer, sessionLayer, pathResolverContext)) {
            return stage;
        }
    }

    // No cached stages.  Make a new stage, and populate caches with it.
    return _InstantiateStage(
        rootLayer,
        sessionLayer,
        pathResolverContext,
        load);
}

SdfPropertySpecHandle
UsdStage::_GetPropertyDefinition(const UsdPrim &prim,
                                 const TfToken &propName) const
{
    if (not prim)
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

SdfPrimSpecHandle
UsdStage::_CreatePrimSpecForEditing(const SdfPath& path)
{
    const UsdEditTarget &editTarget = GetEditTarget();
    const SdfPath &targetPath = editTarget.MapToSpecPath(path);
    return targetPath.IsEmpty() ? SdfPrimSpecHandle() :
        SdfCreatePrimInLayer(editTarget.GetLayer(), targetPath);
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
    UsdPrim prim = prop.GetPrim();
    specToCopy = _GetPropertyDefinition<PropType>(prop);

    if (not specToCopy) {
        // There is no definition available, either because the prim has no
        // known schema, or its schema has no definition for this property.  In
        // this case, we look to see if there's a strongest property spec.  If
        // so, we copy its required metadata.
        for (Usd_Resolver r(&prim.GetPrimIndex()); r.IsValid(); r.NextLayer()) {
            if (SdfPropertySpecHandle propSpec = r.GetLayer()->
                GetPropertyAtPath(r.GetLocalPath().AppendProperty(propName))) {
                if (specToCopy = TfDynamic_cast<TypedSpecHandle>(propSpec))
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
        SdfPrimSpecHandle primSpec = _CreatePrimSpecForEditing(prim.GetPath());
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

    if (ARCH_UNLIKELY(obj.GetPrim().IsInMaster())) {
        TF_CODING_ERROR("Cannot set metadata at path <%s>; "
                        "authoring to a prim in an instancing master is not "
                        "allowed.",
                        obj.GetPath().GetText());
        return false;
    }

    SdfSpecHandle spec;

    if (obj.Is<UsdProperty>()) {
        spec = _CreatePropertySpecForEditing(obj.As<UsdProperty>());
    } else if (obj.Is<UsdPrim>()) {
        spec = _CreatePrimSpecForEditing(obj.GetPath());
    } else {
        TF_CODING_ERROR("Cannot set metadata at path <%s> in layer @%s@; "
                        "a prim or property is required",
                        GetEditTarget().MapToSpecPath(obj.GetPath()).GetText(),
                        GetEditTarget().GetLayer()->GetIdentifier().c_str());
        return false;
    }

    // XXX: why is this not caught by SdfLayer? Is this a BdData bug?
    if (not spec) {
        TF_CODING_ERROR("Cannot set metadata. Failed to create spec <%s> in "
                        "layer @%s@",
                        GetEditTarget().MapToSpecPath(obj.GetPath()).GetText(),
                        GetEditTarget().GetLayer()->GetIdentifier().c_str());
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

namespace {
bool 
_ValueContainsBlock(const VtValue* value) {
    return value and value->IsHolding<SdfValueBlock>();
}

bool
_ValueContainsBlock(const SdfAbstractDataValue* value) {
    static const TfType valueBlockType = TfType::Find<SdfValueBlock>();
    return value and value->valueType == valueBlockType.GetTypeid();
}

bool
_ValueContainsBlock(const SdfAbstractDataConstValue* value) {
    static const TfType valueBlockType = TfType::Find<SdfValueBlock>();
    return value and value->valueType == valueBlockType.GetTypeid();
}

bool 
_ClearValueIfBlocked(VtValue* value) {
    if (_ValueContainsBlock(value)) {
        *value = VtValue();
        return true;
    }

    return false;
}

bool _ClearValueIfBlocked(SdfAbstractDataValue* value) {
    return _ValueContainsBlock(value);
}
}

template <class T>
bool
UsdStage::_SetValueImpl(
    UsdTimeCode time, const UsdAttribute &attr, const T& newValue)
{
    if (ARCH_UNLIKELY(attr.GetPrim().IsInMaster())) {
        TF_CODING_ERROR("Cannot set attribute value at path <%s>; "
                        "authoring to a prim in an instancing master is not "
                        "allowed.",
                        attr.GetPath().GetText());
        return false;
    }

    // if we are setting a value block, we don't want type checking
    if (not _ValueContainsBlock(&newValue)) {
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
        if (not TfSafeTypeCompare(_GetTypeInfo(newValue), valType.GetTypeid())) {
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
        if (TfDebug::IsEnabled(USD_VALIDATE_VARIABILITY) and
            time != UsdTimeCode::Default() and 
            _GetVariability(attr) == SdfVariabilityUniform) {
            TF_DEBUG(USD_VALIDATE_VARIABILITY)
                .Msg("Warning: authoring time sample value on "
                     "uniform attribute <%s> at time %.3f\n", 
                     UsdDescribe(attr).c_str(), time.GetValue());
        }
    }

    SdfAttributeSpecHandle attrSpec = _CreateAttributeSpecForEditing(attr);

    if (not attrSpec) {
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

        const PcpPrimIndex* idx = &attr.GetPrim().GetPrimIndex();

        // Walk the Pcp node tree until we find the node the EditTarget is at.
        // Then we can get the correct layer offset and invert the time value.
        UsdEditTarget const &editTarget = GetEditTarget();
        SdfLayerOffset layerOffset;
        if (editTarget.IsLocalLayer()) {
            layerOffset = _GetLayerOffsetToRoot(idx->GetRootNode(),
                                                editTarget.GetLayer());
        } else {
            for (Usd_Resolver res(idx); res.IsValid(); res.NextNode()) {
                const PcpNodeRef &node = res.GetNode();
                if (editTarget.IsAtNode(node)) {
                    layerOffset = _GetLayerOffsetToRoot(
                        node, editTarget.GetLayer());
                    break;
                }
            }
        }

        double localTime = layerOffset.GetInverse() * time.GetValue();

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
    if (ARCH_UNLIKELY(attr.GetPrim().IsInMaster())) {
        TF_CODING_ERROR("Cannot clear attribute value at path <%s>; "
                        "authoring to a prim in an instancing master is not "
                        "allowed.",
                        attr.GetPath().GetText());
        return false;
    }

    if (time.IsDefault())
        return _ClearMetadata(attr, SdfFieldKeys->Default);

    const UsdEditTarget &editTarget = GetEditTarget();
    if (not editTarget.IsValid()) {
        TF_CODING_ERROR("EditTarget does not contain a valid layer.");
        return false;
    }

    const SdfLayerHandle &layer = editTarget.GetLayer();
    SdfPath localPath = editTarget.MapToSpecPath(attr.GetPrimPath());
    const TfToken &attrName = attr.GetName();
    if (not layer->HasSpec(SdfAbstractDataSpecId(&localPath, &attrName))) {
        return true;
    }

    SdfAttributeSpecHandle attrSpec = _CreateAttributeSpecForEditing(attr);

    if (not TF_VERIFY(attrSpec, 
                      "Failed to get attribute spec <%s> in layer @%s@",
                      editTarget.MapToSpecPath(attr.GetPath()).GetText(),
                      editTarget.GetLayer()->GetIdentifier().c_str())) {
        return false;
    }

    // NOTE: This logic also exist in _SetValueImpl:
    //
    // Walk the Pcp node tree until we find the node the EditTarget is at.
    // Then we can get the correct layer offset and invert the time value.
    SdfLayerOffset layerOffset;
    const PcpPrimIndex* idx = &attr.GetPrim().GetPrimIndex();
    if (editTarget.IsLocalLayer()) {
        layerOffset = _GetLayerOffsetToRoot(idx->GetRootNode(),
                                            editTarget.GetLayer());
    } else {
        for (Usd_Resolver res(idx); res.IsValid(); res.NextNode()) {
            const PcpNodeRef &node = res.GetNode();
            if (editTarget.IsAtNode(node)) {
                layerOffset = _GetLayerOffsetToRoot(
                    node, editTarget.GetLayer());
                break;
            }
        }
    }

    attrSpec->GetLayer()->EraseTimeSample(
        attrSpec->GetPath(), layerOffset.GetInverse() * time.GetValue());

    return true;
}

bool
UsdStage::_ClearMetadata(const UsdObject &obj, const TfToken& fieldName,
    const TfToken &keyPath)
{
    if (ARCH_UNLIKELY(obj.GetPrim().IsInMaster())) {
        TF_CODING_ERROR("Cannot clear metadata at path <%s>; "
                        "authoring to a prim in an instancing master is not "
                        "allowed.",
                        obj.GetPath().GetText());
        return false;
    }

    const UsdEditTarget &editTarget = GetEditTarget();
    if (not editTarget.IsValid()) {
        TF_CODING_ERROR("EditTarget does not contain a valid layer.");
        return false;
    }

    const SdfLayerHandle &layer = editTarget.GetLayer();
    SdfPath localPath = editTarget.MapToSpecPath(obj.GetPrimPath());
    static TfToken empty;
    const TfToken &propName = obj.Is<UsdProperty>() ? obj.GetName() : empty;
    if (not layer->HasSpec(SdfAbstractDataSpecId(&localPath, &propName))) {
        return true;
    }

    SdfSpecHandle spec;
    if (obj.Is<UsdProperty>())
        spec = _CreatePropertySpecForEditing(obj.As<UsdProperty>());
    else
        spec = _CreatePrimSpecForEditing(obj.GetPrimPath());

    if (not TF_VERIFY(spec, 
                      "No spec at <%s> in layer @%s@",
                      editTarget.MapToSpecPath(obj.GetPath()).GetText(),
                      GetEditTarget().GetLayer()->GetIdentifier().c_str())) {
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
        ignoredKeys.insert(UsdTokens->allTokens.begin(),
                           UsdTokens->allTokens.end());
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
    if (field and (field->IsReadOnly() or field->HoldsChildren()))
        return true;

    // The field is not private.
    return false;
}

UsdPrim
UsdStage::GetPseudoRoot() const
{
    return UsdPrim(_pseudoRoot);
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
    return UsdPrim(_GetPrimDataAtPath(path));
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

bool
UsdStage::_IsValidForLoadUnload(const SdfPath& path) const
{
    if (not path.IsAbsolutePath()) {
        TF_CODING_ERROR("Attempted to load/unload a relative path <%s>",
                        path.GetText());
        return false;
    }

    // XXX PERFORMANCE: could use HasPrimAtPath
    UsdPrim curPrim = GetPrimAtPath(path);

    if (not curPrim) {
        // Lets see if any ancestor exists, if so it's safe to attempt to load.
        SdfPath parentPath = path;
        while (parentPath != SdfPath::AbsoluteRootPath()) {
            if (curPrim = GetPrimAtPath(parentPath)) {
                break;
            }
            parentPath = parentPath.GetParentPath();
        }

        // We walked up to the absolute root without finding anything
        // report error.
        if (parentPath == SdfPath::AbsoluteRootPath()) {
            TF_RUNTIME_ERROR("Attempt to load/unload a path <%s> which is not "
                             "present in the stage",
                    path.GetString().c_str());
            return false;
        }
    }

    if (not curPrim.IsActive()) {
        TF_RUNTIME_ERROR("Attempt to load/unload an inactive path <%s>",
                path.GetString().c_str());
        return false;
    }

    if (curPrim.IsMaster()) {
        TF_RUNTIME_ERROR("Attempt to load/unload instance master <%s>",
                path.GetString().c_str());
        return false;
    }

    return true;
}

void
UsdStage::_DiscoverPayloads(const SdfPath& rootPath,
                            SdfPathSet* primIndexPaths,
                            bool unloadedOnly,
                            SdfPathSet* usdPrimPaths) const
{
    tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash> seenMasterPrimPaths;
    tbb::concurrent_vector<SdfPath> primIndexPathsVec;
    tbb::concurrent_vector<SdfPath> usdPrimPathsVec;

    UsdPrim root = GetPrimAtPath(rootPath);
    if (not root)
        return;

    _DiscoverPayloadsInternal(root,
                              primIndexPaths ? &primIndexPathsVec : nullptr,
                              unloadedOnly,
                              usdPrimPaths ? &usdPrimPathsVec : nullptr,
                              &seenMasterPrimPaths);

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
UsdStage::_DiscoverPayloadsInternal(
    UsdPrim const &prim,
    tbb::concurrent_vector<SdfPath> *primIndexPaths,
    bool unloadedOnly,
    tbb::concurrent_vector<SdfPath> *usdPrimPaths,
    tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash> *seenMasterPrimPaths
    ) const
{
    UsdTreeIterator childIt = UsdTreeIterator::AllPrims(prim);
    
    WorkParallelForEach(
        childIt, childIt.GetEnd(),
        [=](UsdPrim const &child) {
            // Inactive prims are never included in this query.
            // Masters are also never included, since they aren't
            // independently loadable.
            if (not child.IsActive() or child.IsMaster())
                return;

            if (child._GetSourcePrimIndex().HasPayload()) {
                const SdfPath& payloadIncludePath = 
                    child._GetSourcePrimIndex().GetPath();
                if (not unloadedOnly or
                    not _cache->IsPayloadIncluded(payloadIncludePath)) {
                    if (primIndexPaths)
                        primIndexPaths->push_back(payloadIncludePath);
                    if (usdPrimPaths)
                        usdPrimPaths->push_back(child.GetPath());
                }
            }

            if (child.IsInstance()) {
                const UsdPrim masterPrim = child.GetMaster();
                if (TF_VERIFY(masterPrim) and 
                    seenMasterPrimPaths->insert(masterPrim.GetPath()).second) {
                    // Recurse.
                    _DiscoverPayloadsInternal(
                        masterPrim, primIndexPaths, unloadedOnly, usdPrimPaths,
                        seenMasterPrimPaths);
                }
            }
        });
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
        if (not parent)
            continue;

        // Inactive prims are never included in this query.
        // Masters are also never included, since they aren't
        // independently loadable.
        if (not parent.IsActive() or parent.IsMaster())
            continue;

        if (parent._GetSourcePrimIndex().HasPayload()) {
            const SdfPath& payloadIncludePath = 
                parent._GetSourcePrimIndex().GetPath();
            if (not unloadedOnly or 
                    not _cache->IsPayloadIncluded(payloadIncludePath)) {
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
UsdStage::Load(const SdfPath& path)
{
    SdfPathSet exclude, include;
    include.insert(path);

    // Update the load set; this will trigger recomposition and include any
    // recursive payloads needed.
    LoadAndUnload(include, exclude);

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
                        const SdfPathSet &unloadSet)
{
    TfAutoMallocTag2 tag("Usd", _mallocTagID);

    SdfPathSet aggregateLoads, aggregateUnloads;
    _LoadAndUnload(loadSet, unloadSet, &aggregateLoads, &aggregateUnloads);

    // send notifications when loading or unloading
    if (aggregateLoads.empty() and aggregateUnloads.empty()) {
        return;
    }

    UsdStageWeakPtr self(this);
    SdfPathVector pathsToRecomposeVec, otherPaths;
    pathsToRecomposeVec.insert(pathsToRecomposeVec.begin(), 
                               aggregateLoads.begin(), aggregateLoads.end());
    pathsToRecomposeVec.insert(pathsToRecomposeVec.begin(),
                               aggregateUnloads.begin(), aggregateUnloads.end());
    UsdNotice::ObjectsChanged(self, &pathsToRecomposeVec, &otherPaths)
               .Send(self);
}

void
UsdStage::_LoadAndUnload(const SdfPathSet &loadSet,
                         const SdfPathSet &unloadSet,
                         SdfPathSet *aggregateLoads,
                         SdfPathSet *aggregateUnloads)
{
    // Include implicit (recursive or ancestral) related payloads in both sets.
    SdfPathSet finalLoadSet, finalUnloadSet;

    // It's important that we do not included payloads that were previously
    // loaded because we need to iterate and will enter an infinite loop if we
    // do not reduce the load set on each iteration. This manifests below in
    // the unloadedOnly=true argument.
    TF_FOR_ALL(pathIt, loadSet) {
        if (not _IsValidForLoadUnload(*pathIt))
            continue;

        _DiscoverPayloads(*pathIt, &finalLoadSet, true /*unloadedOnly*/);
        _DiscoverAncestorPayloads(*pathIt, &finalLoadSet,
                                  true /*unloadedOnly*/);
    }

    // Recursively populate the unload set.
    TF_FOR_ALL(pathIt, unloadSet) {
        if (not _IsValidForLoadUnload(*pathIt))
            continue;

        // PERFORMANCE: This should exclude any paths in the load set,
        // to avoid unloading and then reloading the same path.
        _DiscoverPayloads(*pathIt, &finalUnloadSet);
    }

    // If we aren't changing the load set, terminate recursion.
    if (finalLoadSet.empty() and finalUnloadSet.empty()) {
        TF_DEBUG(USD_PAYLOADS).Msg("PAYLOAD: terminate recursion\n");
        return;
    }

    // Debug output only.
    if (TfDebug::IsEnabled(USD_PAYLOADS)) {
        TF_DEBUG(USD_PAYLOADS).Msg("PAYLOAD: Load/Unload payload sets\n"
                                   "  Include set:\n");
        TF_FOR_ALL(pathIt, loadSet) {
            TF_DEBUG(USD_PAYLOADS).Msg("\t<%s>\n", pathIt->GetString().c_str());
        }
        TF_DEBUG(USD_PAYLOADS).Msg("  Final Include set:\n");
        TF_FOR_ALL(pathIt, finalLoadSet) {
            TF_DEBUG(USD_PAYLOADS).Msg("\t<%s>\n", pathIt->GetString().c_str());
        }

        TF_DEBUG(USD_PAYLOADS).Msg("  Exclude set:\n");
        TF_FOR_ALL(pathIt, unloadSet) {
            TF_DEBUG(USD_PAYLOADS).Msg("\t<%s>\n", pathIt->GetString().c_str());
        }
        TF_DEBUG(USD_PAYLOADS).Msg("  Final Exclude set:\n");
        TF_FOR_ALL(pathIt, finalUnloadSet) {
            TF_DEBUG(USD_PAYLOADS).Msg("\t<%s>\n", pathIt->GetString().c_str());
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
    _Recompose(changes, NULL);

    // Recurse.
    //
    // Note that recursion is not necessary for the unload set, which gets upon
    // the first recursion.
    
    // aggregate our results for notification
    if (aggregateLoads and aggregateUnloads) {
        aggregateLoads->insert(finalLoadSet.begin(), finalLoadSet.end());
        aggregateUnloads->insert(finalUnloadSet.begin(), finalUnloadSet.end());
    }

    _LoadAndUnload(loadSet, SdfPathSet(), aggregateLoads, aggregateUnloads);
}

SdfPathSet
UsdStage::GetLoadSet()
{
    SdfPathSet loadSet;
    BOOST_FOREACH(const SdfPath& primIndexPath, _cache->GetIncludedPayloads()) {
        const SdfPath primPath = _GetPrimPathUsingPrimIndexAtPath(primIndexPath);
        if (TF_VERIFY(not primPath.IsEmpty(), "Unable to get prim path using "
            "prim index at path <%s>.", primIndexPath.GetText())) {
            loadSet.insert(primPath);
        }
    }

    return loadSet;
}

SdfPathSet
UsdStage::FindLoadable(const SdfPath& rootPath)
{
    SdfPathSet loadable;
    _DiscoverPayloads(rootPath, NULL, /* unloadedOnly = */ false, &loadable);
    return loadable;
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
    BOOST_FOREACH(const SdfPath& path, masterPaths) {
        UsdPrim p = GetPrimAtPath(path);
        if (TF_VERIFY(p)) {
            masterPrims.push_back(p);
        }
    }
    return masterPrims;
}

Usd_PrimDataConstPtr 
UsdStage::_GetMasterForInstance(Usd_PrimDataConstPtr prim) const
{
    if (not prim->IsInstance()) {
        return NULL;
    }

    const SdfPath masterPath = 
        _instanceCache->GetMasterForPrimIndexAtPath(
            prim->GetPrimIndex().GetPath());
    return masterPath.IsEmpty() ? NULL : _GetPrimDataAtPath(masterPath);
}

bool 
UsdStage::_IsObjectElidedFromStage(const SdfPath& path) const
{
    // If the given path is a descendant of an instanceable
    // prim index, it would not be computed during composition unless
    // it is also serving as the source prim index for a master prim
    // on this stage.
    return (_instanceCache->IsPrimInMasterForPrimIndexAtPath(
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
            _instanceCache->GetPrimsInMastersUsingPrimIndexAtPath(
                primIndexPath);

        BOOST_FOREACH(const SdfPath& pathInMaster, mastersUsingPrimIndex) {
            // If this path is a root prim path, it must be the path of a
            // master prim. This function wants to ignore master prims,
            // since they appear to have no prim index to the outside
            // consumer.
            //
            // However, if this is not a root prim path, it must be the
            // path of an prim nested inside a master, which we do want
            // to return. There will only ever be one of these, so we
            // can get this prim and break immediately.
            if (not pathInMaster.IsRootPrimPath()) {
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
UsdStage::_ComposeChildren(Usd_PrimDataPtr prim, bool recurse)
{
    // If prim is deactivated, discard any existing children and return.
    if (not prim->IsActive()) {
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
            _instanceCache->GetMasterUsingPrimIndexAtPath(sourceIndexPath);

        if (not masterPath.IsEmpty()) {
            Usd_PrimDataPtr masterPrim = _GetPrimDataAtPath(masterPath);
            if (not masterPrim) {
                masterPrim = _InstantiatePrim(masterPath);

                // Master prims are parented beneath the pseudo-root,
                // but are *not* children of the pseudo-root. This ensures
                // that consumers never see master prims unless they are
                // explicitly asked for. So, we don't need to set the child
                // link here.
                masterPrim->_SetParentLink(_pseudoRoot);
            }
            _ComposeSubtree(masterPrim, _pseudoRoot, sourceIndexPath);
        }
        return;
    }

    // Compose child names for this prim.
    TfTokenVector nameOrder;
    if (not TF_VERIFY(prim->_ComposePrimChildNames(&nameOrder)))
        return;

    // Optimize for important special cases:
    //
    // 1) the prim has no children.
    if (nameOrder.empty()) {
        TF_DEBUG(USD_COMPOSITION).Msg("Children empty <%s>\n",
                                      prim->GetPath().GetText());
        _DestroyDescendents(prim);
        return;
    }
    // 2) the prim had no children previously.
    if (not prim->_firstChild) {
        TF_DEBUG(USD_COMPOSITION).Msg("Children all new <%s>\n",
                                      prim->GetPath().GetText());
        SdfPath parentPath = prim->GetPath();
        Usd_PrimDataPtr head = NULL, prev = NULL, cur = NULL;
        TF_FOR_ALL(i, nameOrder) {
            cur = _InstantiatePrim(parentPath.AppendChild(*i));
            if (recurse) {
                _ComposeChildSubtree(cur, prim);
            }
            if (not prev)
                head = cur;
            else
                prev->_SetSiblingLink(cur);
            prev = cur;
        }
        prim->_firstChild = head;
        cur->_SetParentLink(prim);
        return;
    }
    // 3) the prim's set of children and its order hasn't changed.
    {
        Usd_PrimDataSiblingIterator
            begin = prim->_ChildrenBegin(),
            end = prim->_ChildrenEnd(),
            cur = begin;
        TfTokenVector::const_iterator
            curName = nameOrder.begin(),
            nameEnd = nameOrder.end();
        for (; cur != end and curName != nameEnd; ++cur, ++curName) {
            if ((*cur)->GetName() != *curName)
                break;
        }
        if (cur == end and curName == nameEnd) {
            TF_DEBUG(USD_COMPOSITION).Msg("Children same in same order <%s>\n",
                                          prim->GetPath().GetText());
            if (recurse) {
                for (cur = begin; cur != end; ++cur) {
                    _ComposeChildSubtree(*cur, prim);
                }
            }
            return;
        }
    }

    TF_DEBUG(USD_COMPOSITION).Msg(
        "Require general children recomposition <%s>\n",
        prim->GetPath().GetText());

    // Otherwise we do the general form of preserving preexisting children and
    // ordering them according to nameOrder.

    // Make a vector of iterators into nameOrder.
    typedef vector<TfTokenVector::const_iterator> TokenVectorIterVec;
    TokenVectorIterVec nameOrderIters(nameOrder.size());
    for (size_t i = 0, sz = nameOrder.size(); i != sz; ++i)
        nameOrderIters[i] = nameOrder.begin() + i;

    // Sort the name order iterators *by name*.
    sort(nameOrderIters.begin(), nameOrderIters.end(), _DerefIterLess());

    // Make a vector of the existing prim children and sort them by name.
    vector<Usd_PrimDataPtr> oldChildren(
        prim->_ChildrenBegin(), prim->_ChildrenEnd());
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
    tempChildren.reserve(nameOrder.size());

    const SdfPath &parentPath = prim->GetPath();

    while (newNameItersIt != newNameItersEnd or oldChildIt != oldChildEnd) {
        // Walk through old children that no longer exist up to the current
        // potentially new name, removing them.
        while (oldChildIt != oldChildEnd and
               (newNameItersIt == newNameItersEnd or
                (*oldChildIt)->GetName() < **newNameItersIt)) {
            TF_DEBUG(USD_COMPOSITION).Msg("Removing <%s>\n",
                                          (*oldChildIt)->GetPath().GetText());
            _DestroyPrim(*oldChildIt++);
        }

        // Walk through any matching children and preserve them.
        for (; newNameItersIt != newNameItersEnd and
                 oldChildIt != oldChildEnd and
                 **newNameItersIt == (*oldChildIt)->GetName();
             ++newNameItersIt, ++oldChildIt) {
            TF_DEBUG(USD_COMPOSITION).Msg("Preserving <%s>\n",
                                          (*oldChildIt)->GetPath().GetText());
            tempChildren.push_back(make_pair(*oldChildIt, *newNameItersIt));
            if (recurse) {
                Usd_PrimDataPtr child = tempChildren.back().first;
                _ComposeChildSubtree(child, prim);
            }
        }

        // Walk newly-added names up to the next old name, adding them.
        for (; newNameItersIt != newNameItersEnd and
                 (oldChildIt == oldChildEnd or
                  **newNameItersIt < (*oldChildIt)->GetName());
             ++newNameItersIt) {
            SdfPath newChildPath = parentPath.AppendChild(**newNameItersIt);
            TF_DEBUG(USD_COMPOSITION).Msg("Creating new <%s>\n",
                                          newChildPath.GetText());
            tempChildren.push_back(
                make_pair(_InstantiatePrim(newChildPath), *newNameItersIt));
            if (recurse) {
                Usd_PrimDataPtr child = tempChildren.back().first;
                _ComposeChildSubtree(child, prim);
            }
        }
    }

    // Now all the new children are in lexicographical order by name, paired
    // with their name's iterator in the original name order.  Recover the
    // original order by sorting by the iterators natural order.
    sort(tempChildren.begin(), tempChildren.end(), _SecondLess());

    // Now copy the correctly ordered children into place.
    prim->_firstChild = NULL;
    TF_REVERSE_FOR_ALL(i, tempChildren)
        prim->_AddChild(i->first);
}

void 
UsdStage::_ComposeChildSubtree(Usd_PrimDataPtr prim, 
                               Usd_PrimDataConstPtr parent)
{
    if (parent->IsInMaster()) {
        // If this UsdPrim is a child of an instance master, its 
        // source prim index won't be at the same path as its stage path.
        // We need to construct the path from the parent's source index.
        const SdfPath sourcePrimIndexPath = 
            parent->GetSourcePrimIndex().GetPath().AppendChild(prim->GetName());
        _ComposeSubtree(prim, parent, sourcePrimIndexPath);
    }
    else {
        _ComposeSubtree(prim, parent);
    }
}

void
UsdStage::_ReportPcpErrors(const PcpErrorVector &errors,
                           const std::string &context) const
{
    _ReportErrors(errors, std::vector<std::string>(), context);
}

void
UsdStage::_ReportErrors(const PcpErrorVector &errors,
                        const std::vector<std::string> &otherErrors,
                        const std::string &context) const
{
    // Report any errors.
    if (not errors.empty() or not otherErrors.empty()) {
        std::string message = context + ":\n";
        BOOST_FOREACH(const PcpErrorBasePtr &err, errors) {
            message += "    " +
                TfStringReplace(err->ToString(), "\n", "\n    ") + '\n';
        }
        BOOST_FOREACH(const std::string &str, otherErrors) {
            message += "    " +
                TfStringReplace(str, "\n", "\n    ") + '\n';
        }
        TF_WARN(message);
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
            primIndexPaths ? (*primIndexPaths)[i] : p->GetPath());
    }

    _dispatcher = boost::none;
    _primMapMutex = boost::none;
}

void
UsdStage::_ComposeSubtree(
    Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
    const SdfPath& primIndexPath)
{
    if (_dispatcher) {
        _dispatcher->Run(
            &UsdStage::_ComposeSubtreeImpl, this, prim, parent, primIndexPath);
    } else {
        // TF_DEBUG(USD_COMPOSITION).Msg("Composing Subtree at <%s>\n",
        //                               prim->GetPath().GetText());
        // TRACE_FUNCTION();
        _ComposeSubtreeImpl(prim, parent, primIndexPath);
    }
}

void
UsdStage::_ComposeSubtreeImpl(
    Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
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
    if (not errors.empty()) {
        _ReportPcpErrors(
            errors, TfStringPrintf("Computing prim index <%s>",
                                   primIndexPath.GetText()));
    }

    parent = parent ? parent : prim->GetParent();

    // If this prim's parent is the pseudo-root and it has a different
    // path from its source prim index, it must represent a master prim.
    const bool isMasterPrim =
        (parent == _pseudoRoot 
         and prim->_primIndex->GetPath() != prim->GetPath());

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
            primHasAuthoredClips or parent->MayHaveOpinionsInClips());
    }

    // Compose the set of children on this prim.
    _ComposeChildren(prim, /*recurse=*/true);
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

    TF_AXIOM(not _dispatcher and not _primMapMutex);

    _primMapMutex = boost::in_place();
    _dispatcher = boost::in_place();

    BOOST_FOREACH(const SdfPath& path, paths) {
        Usd_PrimDataPtr prim = _GetPrimDataAtPath(path);
        _dispatcher->Run(&UsdStage::_DestroyPrim, this, prim);
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
    if (not _isClosingStage) {
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
        
    // Process changes.  This won't be invoked automatically if we didn't
    // reload any layers but only loaded layers that we failed to load
    // previously (because loading a previously unloaded layer doesn't
    // invoke change processing).
    _Recompose(changes, NULL);
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

static bool
_CheckAbsolutePrimPath(const SdfPath &path)
{
    // Path must be absolute.
    if (ARCH_UNLIKELY(not path.IsAbsolutePath())) {
        TF_CODING_ERROR("Path must be an absolute path: <%s>", path.GetText());
        return false;
    }

    // Path must be a prim path (or the absolute root path).
    if (ARCH_UNLIKELY(not path.IsAbsoluteRootOrPrimPath())) {
        TF_CODING_ERROR("Path must be a prim path: <%s>", path.GetText());
        return false;
    }

    // Path must not contain variant selections.
    if (ARCH_UNLIKELY(path.ContainsPrimVariantSelection())) {
        TF_CODING_ERROR("Path must not contain variant selections: <%s>",
                        path.GetText());
        return false;
    }

    return true;
}

UsdPrim
UsdStage::OverridePrim(const SdfPath &path)
{
    // Special-case requests for the root.  It always succeeds and never does
    // authoring since the root cannot have PrimSpecs.
    if (path == SdfPath::AbsoluteRootPath())
        return GetPseudoRoot();
    
    // Validate path input.
    if (not _CheckAbsolutePrimPath(path))
        return UsdPrim();

    // If there is already a UsdPrim at the given path, grab it.
    UsdPrim prim = GetPrimAtPath(path);

    // Do the authoring, if any to do.
    if (not prim) {
        {
            SdfChangeBlock block;
            TfErrorMark m;
            SdfPrimSpecHandle primSpec = _CreatePrimSpecForEditing(path);
            // If spec creation failed, return.  Issue an error if a more
            // specific error wasn't already issued.
            if (not primSpec) {
                if (m.IsClean())
                    TF_RUNTIME_ERROR("Failed to create PrimSpec for <%s>",
                                     path.GetText());
                return UsdPrim();
            }
        }

        // Attempt to fetch the prim we tried to create.
        prim = GetPrimAtPath(path);
    }

    return prim;
}

UsdPrim
UsdStage::DefinePrim(const SdfPath &path,
                     const TfToken &typeName)
{
    // Special-case requests for the root.  It always succeeds and never does
    // authoring since the root cannot have PrimSpecs.
    if (path == SdfPath::AbsoluteRootPath())
        return GetPseudoRoot();

    // Validate path input.
    if (not _CheckAbsolutePrimPath(path))
        return UsdPrim();

    // Define all ancestors.
    if (not DefinePrim(path.GetParentPath()))
        return UsdPrim();
    
    // Now author scene description for this prim.
    TfErrorMark m;
    UsdPrim prim = GetPrimAtPath(path);
    if (not prim or not prim.IsDefined() or
        (not typeName.IsEmpty() and prim.GetTypeName() != typeName)) {
        {
            SdfChangeBlock block;
            SdfPrimSpecHandle primSpec = _CreatePrimSpecForEditing(path);
            // If spec creation failed, return.  Issue an error if a more
            // specific error wasn't already issued.
            if (not primSpec) {
                if (m.IsClean())
                    TF_RUNTIME_ERROR(
                        "Failed to create primSpec for <%s>", path.GetText());
                return UsdPrim();
            }
            
            // Set specifier and typeName, if not empty.
            primSpec->SetSpecifier(SdfSpecifierDef);
            if (not typeName.IsEmpty())
                primSpec->SetTypeName(typeName);
        }
        // Fetch prim if newly created.
        prim = prim ? prim : GetPrimAtPath(path);
    }
    
    // Issue an error if we were unable to define this prim and an error isn't
    // already issued.
    if ((not prim or not prim.IsDefined()) and m.IsClean())
        TF_RUNTIME_ERROR("Failed to define UsdPrim <%s>", path.GetText());

    return prim;
}

UsdPrim
UsdStage::CreateClassPrim(const SdfPath &path)
{
    // Classes must be root prims.
    if (not path.IsRootPrimPath()) {
        TF_CODING_ERROR("Classes must be root prims.  <%s> is not a root prim "
                        "path", path.GetText());
        return UsdPrim();
    }

    // Classes must be created in local layers.
    if (not GetEditTarget().IsLocalLayer()) {
        TF_CODING_ERROR("Must create classes in local LayerStack");
        return UsdPrim();
    }

    // It's an error to try to transform a defined non-class into a class.
    UsdPrim prim = GetPrimAtPath(path);
    if (prim and prim.IsDefined() and
        prim.GetSpecifier() != SdfSpecifierClass) {
        TF_RUNTIME_ERROR("Non-class prim already exists at <%s>",
                         path.GetText());
        return UsdPrim();
    }

    // Stamp a class PrimSpec if need-be.
    if (not prim or not prim.IsAbstract()) {
        prim = DefinePrim(path);
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

bool
UsdStage::HasLocalLayer(const SdfLayerHandle &layer) const
{
    return _cache->GetLayerStack()->HasLayer(layer);
}

void
UsdStage::SetEditTarget(const UsdEditTarget &editTarget)
{
    if (not editTarget.IsValid()){
        TF_CODING_ERROR("Attempt to set an invalid UsdEditTarget as current");
        return;
    }
    // Do some extra error checking if the EditTarget specifies a local layer.
    if (editTarget.IsLocalLayer()) {
        if (not HasLocalLayer(editTarget.GetLayer())) {
            TF_CODING_ERROR("Layer @%s@ is not in the local LayerStack rooted "
                            "at @%s@",
                            editTarget.GetLayer()->GetIdentifier().c_str(),
                            GetRootLayer()->GetIdentifier().c_str());
            return;
        }
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
    if (not TF_VERIFY(_GetPcpCache())) {
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
UsdStage::GetUsedLayers() const
{
    if (not _cache)
        return SdfLayerHandleVector();
    
    const SdfLayerHandleSet &usedLayers = _cache->GetUsedLayers();
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

    SdfPathSet paths;
    _Recompose(changes, &paths);

    UsdStageWeakPtr self(this);
    const SdfPathVector recomposedPaths(paths.begin(), paths.end());
    const SdfPathVector otherPaths;
    UsdNotice::ObjectsChanged(self, &recomposedPaths, &otherPaths)
               .Send(self);
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

UsdTreeIterator
UsdStage::Traverse()
{
    return UsdTreeIterator::Stage(UsdStagePtr(this));
}

UsdTreeIterator
UsdStage::Traverse(const Usd_PrimFlagsPredicate &predicate)
{
    return UsdTreeIterator::Stage(UsdStagePtr(this), predicate);
}

UsdTreeIterator
UsdStage::TraverseAll()
{
    return UsdTreeIterator::Stage(UsdStagePtr(this),
                                  Usd_PrimFlagsPredicate::Tautology());
}

bool
UsdStage::_RemovePrim(const SdfPath& path)
{
    SdfPrimSpecHandle spec = _GetPrimSpec(path);
    if (not spec) {
        return false;
    }

    SdfPrimSpecHandle parent = spec->GetRealNameParent();
    if (not parent) {
        return false;
    }

    return parent->RemoveNameChild(spec);
}

bool
UsdStage::_RemoveProperty(const SdfPath &path)
{
    SdfPropertySpecHandle propHandle =
        GetEditTarget().GetPropertySpecForScenePath(path);

    if (not propHandle) {
        return false;
    }

    // dynamic cast needed because of protected copyctor
    // safe to assume a prim owner because we are in UsdPrim
    SdfPrimSpecHandle parent 
        = TfDynamic_cast<SdfPrimSpecHandle>(propHandle->GetOwner());

    if (not TF_VERIFY(parent, "Prop has no parent")) {
        return false;
    }

    parent->RemoveProperty(propHandle);
    return true;
}

// Add paths in the given cache that depend on the given path in the given layer
// to the output.
static void
_AddDependentPaths(const SdfLayerHandle &layer, const SdfPath &path,
                   const PcpCache &cache, SdfPathSet *output)
{
    // Use Pcp's LayerStack dependency facilities.  We cannot use Pcp's spec
    // dependency facilities, since we skip populating those in Usd mode.  We
    // may need to revisit this when we decide to tackle namespace editing.
    const PcpLayerStackPtrVector& layerStacks =
        cache.FindAllLayerStacksUsingLayer(layer);

    BOOST_FOREACH(const PcpLayerStackPtr &layerStack, layerStacks) {
        // If this path is in the cache's LayerStack, we always add it.
        if (layerStack == cache.GetLayerStack())
            output->insert(path.StripAllVariantSelections());

        // Ask the cache for further dependencies and add any to the output.
        SdfPathVector deps = cache.GetPathsUsingSite(
            layerStack, path, PcpDirect | PcpAncestral, /*recursive*/ true);
        output->insert(deps.begin(), deps.end());

        TF_DEBUG(USD_CHANGES).Msg(
            "Adding paths that use <%s> in layer @%s@%s: %s\n",
            path.GetText(), layer->GetIdentifier().c_str(),
            layerStack->GetIdentifier().rootLayer != layer ?
            TfStringPrintf(" (stack root: @%s@)",
                           layerStack->GetIdentifier().rootLayer->
                           GetIdentifier().c_str()).c_str() : "",
            TfStringify(deps).c_str());
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

    // both of these path sets will be stage-relative and thus are safe to
    // feed into all public stage API
    SdfPathSet pathsToRecompose, otherChangedPaths;

    // Add dependent paths for any PrimSpecs whose fields have changed that may
    // affect cached prim information.
    TF_FOR_ALL(itr, n.GetChangeListMap()) {

        // If this layer does not pertain to us, skip.
        if (_cache->FindAllLayerStacksUsingLayer(itr->first).empty())
            continue;

        TF_FOR_ALL(entryIter, itr->second.GetEntryList()) {

            const SdfPath &path = entryIter->first;
            const SdfChangeList::Entry &entry = entryIter->second;

            TF_DEBUG(USD_CHANGES).Msg(
                "<%s> in @%s@ changed.\n",
                path.GetText(), itr->first->GetIdentifier().c_str());

            bool willRecompose = false;
            if (path == SdfPath::AbsoluteRootPath() or
                path.IsPrimOrPrimVariantSelectionPath()) {

                if (entry.flags.didReorderChildren) {
                    willRecompose = true;
                } else {
                    TF_FOR_ALL(infoIter, entry.infoChanged) {
                        if ((infoIter->first == SdfFieldKeys->Active) or
                            (infoIter->first == SdfFieldKeys->Kind) or
                            (infoIter->first == SdfFieldKeys->TypeName) or
                            (infoIter->first == SdfFieldKeys->Specifier) or
                            
                            // XXX: Could be more specific when recomposing due
                            //      to clip changes. E.g., only update the clip
                            //      resolver and bits on each prim.
                            UsdIsClipRelatedField(infoIter->first)) {

                            TF_DEBUG(USD_CHANGES).Msg(
                                "Changed field: %s\n",
                                infoIter->first.GetText());

                            willRecompose = true;
                            break;
                        }
                    }
                }

                if (willRecompose) {
                    _AddDependentPaths(
                        itr->first, path, *_cache, &pathsToRecompose);
                }
            }

            // If we're not going to recompose this path, record the dependent
            // scene paths separately so we can notify clients about the
            // changes.
            if (not willRecompose) {
                _AddDependentPaths(
                    itr->first, path, *_cache, &otherChangedPaths);
            }
        }
    }

    PcpChanges changes;
    changes.DidChange(std::vector<PcpCache*>(1, _cache.get()),
                      n.GetChangeListMap());
    _Recompose(changes, &pathsToRecompose);

    // Make a copy of pathsToRecompose, but uniqued with a prefix-check, which
    // removes all elements that are prefixed by other elements.  Also
    // remove any paths that are beneath instances, since UsdStage doesn't
    // expose any objects at these paths.
    SdfPathVector pathsToRecomposeVec;
    _CopyAndRemoveDescendentPaths(pathsToRecompose, &pathsToRecomposeVec);

    using std::remove_if;
    using boost::bind;

    pathsToRecomposeVec.erase(
        remove_if(pathsToRecomposeVec.begin(), pathsToRecomposeVec.end(),
                  bind(&UsdStage::_IsObjectElidedFromStage, this, _1)),
        pathsToRecomposeVec.end());

    // Collect the paths in otherChangedPaths that aren't under paths that
    // were recomposed.  If the pseudo-root had been recomposed, we can
    // just clear out otherChangedPaths since everything was recomposed.
    if (not pathsToRecomposeVec.empty() and
        pathsToRecomposeVec.front() == SdfPath::AbsoluteRootPath()) {
        // If the pseudo-root is present, it should be the only path in the
        // vector.
        TF_VERIFY(pathsToRecomposeVec.size() == 1);
        otherChangedPaths.clear();
    }

    SdfPathVector otherChangedPathsVec(otherChangedPaths.begin(),
                                       otherChangedPaths.end());

    otherChangedPathsVec.erase(
        remove_if(otherChangedPathsVec.begin(), otherChangedPathsVec.end(),
                  bind(&UsdStage::_IsObjectElidedFromStage, this, _1)),
        otherChangedPathsVec.end());

    // Now we want to remove all elements of otherChangedPathsVec that are
    // prefixed by elements in pathsToRecompose.
    SdfPathVector::iterator
        other = otherChangedPathsVec.begin(),
        otherEnd = otherChangedPathsVec.end();
    SdfPathVector::const_iterator
        recomp = pathsToRecomposeVec.begin(),
        recompEnd = pathsToRecomposeVec.end();
    while (recomp != recompEnd and other != otherEnd) {
        if (*other < *recomp) {
            // If the current element in other is less than the current element
            // in recomp, it cannot be prefixed, so retain it.
            ++other;
        } else if (other->HasPrefix(*recomp)) {
            // Otherwise if this element in other is prefixed by the current
            // element in pathsToRecompose, shuffle it to the end to discard.
            if (other+1 != otherEnd)
                std::rotate(other, other + 1, otherEnd);
            --otherEnd;
        } else {
            // Otherwise advance to the next element in pathsToRecompose.
            ++recomp;
        }
    }
    // Erase removed elements.
    otherChangedPathsVec.erase(otherEnd, otherChangedPathsVec.end());

    UsdStageWeakPtr self(this);

    // Notify about changed objects.
    UsdNotice::ObjectsChanged(
        self, &pathsToRecomposeVec, &otherChangedPathsVec).Send(self);

    // Receivers can now refresh their caches... or just dirty them
    UsdNotice::StageContentsChanged(self).Send(self);
}

void UsdStage::_Recompose(const PcpChanges &changes,
                          SdfPathSet *initialPathsToRecompose)
{
    ArResolverScopedCache resolverCache;

    SdfPathSet newPathsToRecompose;
    SdfPathSet *pathsToRecompose = initialPathsToRecompose ?
        initialPathsToRecompose : &newPathsToRecompose;

    changes.Apply();

    const PcpChanges::CacheChanges &cacheChanges = changes.GetCacheChanges();

    if (not cacheChanges.empty()) {
        const PcpCacheChanges &ourChanges = cacheChanges.begin()->second;

        TF_FOR_ALL(itr, ourChanges.didChangeSignificantly) {
            // Translate the real path from Pcp into a stage-relative path
            pathsToRecompose->insert(*itr);
            TF_DEBUG(USD_CHANGES).Msg("Did Change Significantly: %s\n",
                                          itr->GetText());
        }

        TF_FOR_ALL(itr, ourChanges.didChangeSpecs) {
            // Translate the real path from Pcp into a stage-relative path
            pathsToRecompose->insert(*itr);
            TF_DEBUG(USD_CHANGES).Msg("Did Change Spec: %s\n",
                                          itr->GetText());
        }

        TF_FOR_ALL(itr, ourChanges.didChangePrims) {
            // Translate the real path from Pcp into a stage-relative path
            pathsToRecompose->insert(*itr);
            TF_DEBUG(USD_CHANGES).Msg("Did Change Prim: %s\n",
                                          itr->GetText());
        }

        if (pathsToRecompose->empty()) {
            TF_DEBUG(USD_CHANGES).Msg(
                "Nothing to recompose in cache changes\n");
        }
    } else {
        TF_DEBUG(USD_CHANGES).Msg("No cache changes\n");
    }

    // Prune descendant paths into a vector.
    SdfPathVector pathVecToRecompose;
    _CopyAndRemoveDescendentPaths(*pathsToRecompose, &pathVecToRecompose);

    // Invalidate the clip cache, but keep the clips alive for the duration
    // of recomposition in the (likely) case that clip data hasn't changed
    // and the underlying clip layer can be reused.
    Usd_ClipCache::Lifeboat clipLifeboat;
    TF_FOR_ALL(it, pathVecToRecompose) {
        _clipCache->InvalidateClipsForPrim(*it, &clipLifeboat);
    }

    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash> _MasterToPrimIndexMap;
    _MasterToPrimIndexMap masterToPrimIndexMap;

    if (not pathVecToRecompose.empty()) {
        // Ask Pcp to compute all the prim indexes in parallel, stopping at
        // stuff that's not active.
        SdfPathVector primPathsToRecompose;
        primPathsToRecompose.reserve(pathVecToRecompose.size());
        BOOST_FOREACH(const SdfPath &path, pathVecToRecompose) {
            if (not path.IsAbsoluteRootOrPrimPath() or
                path.ContainsPrimVariantSelection()) {
                continue;
            }

            // Instance prims don't expose any name children, so we don't
            // need to recompose any prim index beneath instance prim 
            // indexes *unless* they are being used as the source index
            // for a master.
            if (_instanceCache->IsPrimInMasterForPrimIndexAtPath(path)) {
                const bool primIndexUsedByMaster = 
                    _instanceCache->IsPrimInMasterUsingPrimIndexAtPath(path);
                if (not primIndexUsedByMaster) {
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

        Usd_InstanceChanges changes;
        _ComposePrimIndexesInParallel(
            primPathsToRecompose, "Recomposing stage", &changes);

        // Determine what instance master prims on this stage need to
        // be recomposed due to instance prim index changes.
        SdfPathVector masterPrimsToRecompose;
        BOOST_FOREACH(const SdfPath &path, primPathsToRecompose) {
            BOOST_FOREACH(
                const SdfPath& masterPath, 
                _instanceCache->GetPrimsInMastersUsingPrimIndexAtPath(path)) {
                masterPrimsToRecompose.push_back(masterPath);
                masterToPrimIndexMap[masterPath] = path;
            }
        }

        for (size_t i = 0; i != changes.newMasterPrims.size(); ++i) {
            masterPrimsToRecompose.push_back(changes.newMasterPrims[i]);
            masterToPrimIndexMap[changes.newMasterPrims[i]] =
                changes.newMasterPrimIndexes[i];
        }

        for (size_t i = 0; i != changes.changedMasterPrims.size(); ++i) {
            masterPrimsToRecompose.push_back(changes.changedMasterPrims[i]);
            masterToPrimIndexMap[changes.changedMasterPrims[i]] =
                changes.changedMasterPrimIndexes[i];
        }

        if (not masterPrimsToRecompose.empty()) {
            // Insert these master prims into the pathsToRecompose set to
            // ensure we send the appropriate notices.
            pathsToRecompose->insert(
                masterPrimsToRecompose.begin(), masterPrimsToRecompose.end());

            pathVecToRecompose.insert(
                pathVecToRecompose.end(), 
                masterPrimsToRecompose.begin(), masterPrimsToRecompose.end());
            SdfPath::RemoveDescendentPaths(&pathVecToRecompose);
        }

        pathsToRecompose->insert(
            changes.deadMasterPrims.begin(), changes.deadMasterPrims.end());
        _DestroyPrimsInParallel(changes.deadMasterPrims);
    }

    SdfPathVector::const_iterator
        i = pathVecToRecompose.begin(), end = pathVecToRecompose.end();
    std::vector<Usd_PrimDataPtr> subtreesToRecompose;
    _ComputeSubtreesToRecompose(i, end, &subtreesToRecompose);

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
        BOOST_FOREACH(const Usd_PrimDataPtr prim, subtreesToRecompose) {
            primIndexPathsForSubtrees.push_back(TfMapLookupByValue(
                masterToPrimIndexMap, prim->GetPath(), prim->GetPath()));
        }
        _ComposeSubtreesInParallel(
            subtreesToRecompose, &primIndexPathsForSubtrees);
    }

    if (not pathVecToRecompose.empty())
        _RegisterPerLayerNotices();
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
        [](const Usd_PrimDataPtr& p) { return not p->IsMaster(); });

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
            _instanceCache->GetMasterUsingPrimIndexAtPath(
                sourceIndexPath ? *sourceIndexPath : p->GetPath());
        if (not masterPath.IsEmpty()) {
            if (not mastersForSubtrees) {
                mastersForSubtrees.reset(new _PathSet);
            }
            mastersForSubtrees->insert(masterPath);
        }
    }

    if (not mastersForSubtrees) {
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
        if (not i->IsAbsoluteRootOrPrimPath() or
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
            _ComposeChildren(parentIt->second.get(), /*recurse=*/false);

            // Recompose the subtree for each affected sibling.
            do {
                PathToNodeMap::const_iterator primIt = _primMap.find(*i);
                if (primIt != _primMap.end())
                    subtreesToRecompose->push_back(primIt->second.get());
                ++i;
            } while (i != end and i->GetParentPath() == parentPath);
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

void 
UsdStage::_ComposePrimIndexesInParallel(
    const std::vector<SdfPath>& primIndexPaths,
    const std::string& context,
    Usd_InstanceChanges* instanceChanges)
{
    TF_DEBUG(USD_COMPOSITION).Msg(
        "Composing prim indexes: %s\n", TfStringify(primIndexPaths).c_str());

    // Ask Pcp to compute all the prim indexes in parallel, stopping at
    // prim indexes that won't be used by the stage.
    PcpErrorVector errs;
    _cache->ComputePrimIndexesInParallel(
        primIndexPaths,
        &errs, _NameChildrenPred(_instanceCache.get()),
        "Usd", _mallocTagID);
    if (not errs.empty()) {
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
    if (not changes.changedMasterPrims.empty()) {
        _ComposePrimIndexesInParallel(
            changes.changedMasterPrimIndexes, context, instanceChanges);
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

    while (usedLayersIter != usedLayersEnd or
           layerAndKeyIter != layerAndKeyEnd) {

        // There are three cases to consider: a newly added layer, a layer no
        // longer used, or a layer that we used before and continue to use.
        if (layerAndKeyIter == layerAndKeyEnd or
            (usedLayersIter != usedLayersEnd and
             *usedLayersIter < layerAndKeyIter->first)) {
            // This is a newly added layer.  Register for the notice and add it.
            newLayersAndNoticeKeys.push_back(
                make_pair(*usedLayersIter,
                          TfNotice::Register(
                              self, &UsdStage::_HandleLayersDidChange,
                              *usedLayersIter)));
            ++usedLayersIter;
        } else if (usedLayersIter == usedLayersEnd or
                   (layerAndKeyIter != layerAndKeyEnd and
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
UsdStage::_GetDefiningSpecType(const UsdPrim& prim,
                               const TfToken& propName) const
{
    if (not TF_VERIFY(prim) or not TF_VERIFY(not propName.IsEmpty()))
        return SdfSpecTypeUnknown;

    // Check for a spec type in the definition registry, in case this is a
    // builtin property.
    SdfSpecType specType =
        UsdSchemaRegistry::GetSpecType(prim.GetTypeName(), propName);

    if (specType != SdfSpecTypeUnknown)
        return specType;

    // Otherwise look for the strongest authored property spec.
    for (Usd_Resolver res(
             &prim.GetPrimIndex()); res.IsValid(); res.NextLayer()) {
        const SdfLayerRefPtr& layer = res.GetLayer();
        SdfAbstractDataSpecId specId(&res.GetLocalPath(), &propName);
        specType = layer->GetSpecType(specId);
        if (specType != SdfSpecTypeUnknown)
            return specType;
    }

    // Unknown.
    return SdfSpecTypeUnknown;
}

// ------------------------------------------------------------------------- //
// Flatten & Export Utilities
// ------------------------------------------------------------------------- //

namespace {
using _MasterToFlattenedPathMap 
    = std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>;

// We want to give generated masters in the flattened stage
// reserved(using '__' as a prefix), unclashing paths, however,
// we don't want to use the '__Master' paths which have special
// meaning to UsdStage. So we create a mapping between our generated
// 'Flattened_Master'-style paths and the '__Master' paths.
_MasterToFlattenedPathMap
_GenerateFlattenedMasterPath(const std::vector<UsdPrim>& masters)
{
    size_t primMasterId = 1;

    const auto generatePathName = [&primMasterId]() {
        return SdfPath(TfStringPrintf("/Flattened_Master_%lu", 
                                      primMasterId++));
    };

    _MasterToFlattenedPathMap masterToFlattened;

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

            masterToFlattened.emplace(make_pair(
                masterPrimPath, flattenedMasterPath));
        } else {
            flattenedMasterPath = masterPathLookup->second;
        }     
    }

    return masterToFlattened;
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

    if (not TF_VERIFY(rootLayer)) {
        return TfNullPtr;
    }

    if (not TF_VERIFY(flatLayer)) {
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

    for (auto childIt = UsdTreeIterator::AllPrims(GetPseudoRoot()); 
         childIt; ++childIt) {
        UsdPrim usdPrim = *childIt;
        _FlattenPrim(usdPrim, flatLayer, usdPrim.GetPath(), masterToFlattened);
    }

    if (addSourceFileComment) {
        std::string doc = flatLayer->GetDocumentation();

        if (not doc.empty()) {
            doc.append("\n\n");
        }

        doc.append(TfStringPrintf("Generated from Composed Stage "
                                  "of root layer %s\n",
                                  GetRootLayer()->GetRealPath().c_str()));

        flatLayer->SetDocumentation(doc);
    }

    return flatLayer;
}


void
UsdStage::_FlattenPrim(const UsdPrim &usdPrim,
                       const SdfLayerHandle &layer,
                       const SdfPath &path,
                       const _MasterToFlattenedPathMap &masterToFlattened) const
{
    SdfPrimSpecHandle newPrim;
    
    if (not usdPrim.IsActive()) {
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
    
    _CopyMetadata(usdPrim, newPrim);
    for (auto const &prop : usdPrim.GetAuthoredProperties()) {
        _CopyProperty(prop, newPrim);
    }
}

void
UsdStage::_CopyMasterPrim(const UsdPrim &masterPrim,
                          const SdfLayerHandle &destinationLayer,
                          const _MasterToFlattenedPathMap 
                            &masterToFlattened) const
{
    const auto& flattenedMasterPath 
        = masterToFlattened.at(masterPrim.GetPath());
   
    for (auto primIt = UsdTreeIterator::AllPrims(masterPrim); primIt; primIt++){
        UsdPrim child = *primIt;
        
        // We need to update the child path to use the Flatten name.
        const auto flattenedChildPath = child.GetPath().ReplacePrefix(
            masterPrim.GetPath(), flattenedMasterPath);

        _FlattenPrim(child, destinationLayer, flattenedChildPath,
                     masterToFlattened);
    }
}

void 
UsdStage::_CopyProperty(const UsdProperty &prop,
                        const SdfPrimSpecHandle &dest) const
{
    if (prop.Is<UsdAttribute>()) {
        UsdAttribute attr = prop.As<UsdAttribute>();
        
        if (not attr.GetTypeName()){
            TF_WARN("Attribute <%s> has unknown value type. " 
                    "It will be omitted from the flattened result.", 
                    attr.GetPath().GetText());
            return;
        }

        SdfAttributeSpecHandle sdfAttr =
            SdfAttributeSpec::New(
                dest, attr.GetName(), attr.GetTypeName());
        _CopyMetadata(attr, sdfAttr);

        // Copy the default & time samples, if present. We get the
        // correct timeSamples/default value resolution here because
        // GetBracketingTimeSamples sets hasSamples=false when the
        // default value is stronger.

        double lower = 0.0, upper = 0.0;
        bool hasSamples = false;
        VtValue defaultValue;
        if (attr.GetBracketingTimeSamples(
            0.0, &lower, &upper, &hasSamples) and hasSamples) {
            sdfAttr->SetInfo(SdfFieldKeys->TimeSamples,
                             VtValue(_GetTimeSampleMap(attr)));
        }
        if (attr.HasAuthoredMetadata(SdfFieldKeys->Default)) {
            if (not attr.Get(&defaultValue)) {
                sdfAttr->SetInfo(SdfFieldKeys->Default, 
                                 VtValue(SdfValueBlock())); 
            } else {
                sdfAttr->SetInfo(SdfFieldKeys->Default, defaultValue);
            }
        }
     }
     else if (prop.Is<UsdRelationship>()) {
         UsdRelationship rel = prop.As<UsdRelationship>();
         // NOTE: custom = true by default for relationship, but the
         // SdfSchema fallback is false, so we must set it explicitly
         // here. The situation is similar for variability.
         SdfRelationshipSpecHandle sdfRel =
             SdfRelationshipSpec::New(dest, rel.GetName(),
                                      /*custom*/ false,
                                      SdfVariabilityVarying);
         _CopyMetadata(rel, sdfRel);

         SdfPathVector targets;
         rel.GetTargets(&targets);

         SdfTargetsProxy sdfTargets = sdfRel->GetTargetPathList();
         sdfTargets.ClearEditsAndMakeExplicit();
         for (auto const& path : targets) {
             sdfTargets.Add(path);
         }
     }
}

void
UsdStage::_CopyMetadata(const UsdObject &source, 
                        const SdfSpecHandle& dest) const
{
    // GetAllMetadata returns all non-private metadata fields (it excludes
    // composition arcs and values), which is exactly what we want here.
    UsdMetadataValueMap metadata = source.GetAllAuthoredMetadata();

    // Copy each key/value into the Sdf spec.
    TfErrorMark m;
    vector<string> msgs;
    for (auto const& tokVal : metadata) {
        dest->SetInfo(tokVal.first, tokVal.second);
        if (not m.IsClean()) {
            msgs.clear();
            for (auto i = m.GetBegin(); i != m.GetEnd(); ++i) {
                msgs.push_back(i->GetCommentary());
            }
            m.Clear();
            TF_WARN("Failed copying metadata: %s", TfStringJoin(msgs).c_str());
        }
    }
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
static void
_Set(SdfAbstractDataValue *dv, T const &val) { dv->StoreValue(val); }
template <class T>
static void _Set(VtValue *value, T const &val) { *value = val; }


template <class Storage>
static void _ApplyLayerOffset(Storage storage,
                              const PcpNodeRef &node,
                              const SdfLayerRefPtr &layer)
{
    SdfLayerOffset offset = _GetLayerOffsetToRoot(node, layer).GetInverse();
    if (not offset.IsIdentity()) {
        const SdfTimeSampleMap &samples =
            _UncheckedGet<SdfTimeSampleMap>(storage);
        SdfTimeSampleMap transformed;
        TF_FOR_ALL(i, samples)
            transformed[offset * i->first] = i->second;
        _Set(storage, transformed);
    }
}

namespace {

template <class Storage>
struct StrongestValueComposer
{
    static const bool ProducesValue = true;

    explicit StrongestValueComposer(Storage s) : _value(s), _done(false) {}
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

        if (_done and _IsHolding<VtDictionary>(_value)) {
            // Continue composing if we got a dictionary.
            _done = false;
            if (isDict) {
                // Merge dictionaries: _value is weaker, tmpDict stronger.
                VtDictionaryOverRecursive(&tmpDict, 
                                          _UncheckedGet<VtDictionary>(_value));
                _Set(_value, tmpDict);
                return true;
            }
        } else if (_done and _IsHolding<SdfTimeSampleMap>(_value)) {
            _ApplyLayerOffset(_value, node, layer);
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

        if (_done and isDict and _IsHolding<VtDictionary>(_value)) {
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
        if (_done and _strongestLayer)
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
            if (not tok.IsEmpty() and tok != SdfTokens->AnyTypeToken)
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

        if (itr->IsInert() or not itr->HasSpecs()) {
            continue;
        }

        const SdfAbstractDataSpecId specId(&itr->GetPath(), &propName);
        TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
            bool result = false;
            if ((*layerIt)->HasField(specId, SdfFieldKeys->Custom, &result)
                and result) {
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
            if (itr->IsInert() or not itr->HasSpecs())
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
        if (itr->IsInert() or not itr->HasSpecs())
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
        if (itr->IsInert() or not itr->HasSpecs())
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
            if (not tok.IsEmpty() and tok != SdfTokens->AnyTypeToken) {
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

    SdfPath localPath;
    SdfLayerRefPtr layer;
    PcpNodeRef node;

    const PcpPrimIndex &primIndex = primData->GetPrimIndex();
    for (Usd_Resolver res(&primIndex); res.IsValid(); res.NextLayer()) {
        // Get specifier and its strength from this prim.
        _SpecifierStrength curStrength = _SpecifierStrengthDefining;
        if (res.GetLayer()->HasField(
                res.GetLocalPath(), SdfFieldKeys->Specifier, &curSpecifier)) {
            node = res.GetNode();
            layer = res.GetLayer();
            localPath = res.GetLocalPath();
            if (SdfIsDefiningSpecifier(curSpecifier)) {
                // Compute strength.
                if (curSpecifier == SdfSpecifierClass) {
                    // See if this excerpt is due to direct inherits.  Walk up
                    // the excerpt tree looking for a direct inherit.  If we
                    // find one set the strength and stop.
                    for (PcpNodeRef node = res.GetNode();
                         node; node = node.GetParentNode()) {

                        if (PcpIsInheritArc(node.GetArcType()) and
                            not node.IsDueToAncestor()) {
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
    if (TF_VERIFY(layer, "No PrimSpecs for '%s'",
                  primData->GetPath().GetText())) {
        // Let the composer see the deciding opinion.
        composer->ConsumeAuthored(
            node, layer, SdfAbstractDataSpecId(&localPath),
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
    if (not listOps.empty()) {
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

    if (not m.IsClean()) {
        // An error occurred during _GetSpecialMetadataImpl.
        return false;
    }

    return _GetGeneralMetadataImpl(
        obj, fieldName, keyPath, useFallbacks, composer) and m.IsClean();
}

template <class Composer>
bool
UsdStage::_GetGeneralMetadataImpl(const UsdObject &obj,
                                  const TfToken& fieldName,
                                  const TfToken& keyPath,
                                  bool useFallbacks,
                                  Composer *composer) const
{
    Usd_Resolver resolver(&obj.GetPrim().GetPrimIndex());
    if (not _ComposeGeneralMetadataImpl(
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
    return gotOpinion or composer->IsDone();
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

        BOOST_FOREACH(const TfToken &fieldName, layer->ListFields(specId)) {
            if (not _IsPrivateFieldKey(fieldName))
                result.push_back(fieldName);
        }
    }

    // Insert required fields for spec type.
    const SdfSchema::SpecDefinition* specDef = NULL;
    specDef = SdfSchema::GetInstance().GetSpecDefinition(specType);
    if (specDef) {
        BOOST_FOREACH(const TfToken &fieldName, specDef->GetRequiredFields()) {
            if (not _IsPrivateFieldKey(fieldName))
                result.push_back(fieldName);
        }
    }

    // If this is a builtin property, add any defined metadata fields.
    // XXX: this should handle prim definitions too.
    if (useFallbacks and propDef) {
        BOOST_FOREACH(const TfToken &fieldName, propDef->ListFields()) {
            if (not _IsPrivateFieldKey(fieldName))
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
                          UsdMetadataValueMap* resultMap) const
{
    TRACE_FUNCTION();

    UsdMetadataValueMap &result = *resultMap;

    TfTokenVector fieldNames = _ListMetadataFields(obj, useFallbacks);
    BOOST_FOREACH(const TfToken &fieldName, fieldNames) {
        VtValue val;
        StrongestValueComposer<VtValue *> composer(&val);
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
    return (layerStack == clip->sourceNode.GetLayerStack()
        and primPathInLayerStack.HasPrefix(clip->sourceNode.GetPath()));
}

static bool
_ClipsApplyToNode(
    const Usd_ClipCache::Clips& clips, 
    const PcpNodeRef& node)
{
    return (node.GetLayerStack() == clips.sourceNode.GetLayerStack()
            and node.GetPath().HasPrefix(clips.sourceNode.GetPath()));
}

static
const Usd_ClipCache::Clips*
_GetClipsThatApplyToNode(
    const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim,
    const PcpNodeRef& node,
    const SdfAbstractDataSpecId& specId)
{
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
                if (not localClips.manifestClip->HasField(
                        specId, SdfFieldKeys->Variability, &attrVariability)
                    or attrVariability != SdfVariabilityVarying) {
                    return nullptr;
                }
            }

            return &localClips;
        }
    }

    return nullptr;
}


bool
UsdStage::_GetValue(UsdTimeCode time, const UsdAttribute &attr, 
                    VtValue* result) const
{
    Usd_UntypedInterpolator interpolator(result);
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

namespace {

template <class T>
bool _GetTimeSampleValue(UsdTimeCode time, const UsdAttribute& attr,
                         const Usd_ResolveInfo &info,
                         const double *lowerHint, const double *upperHint,
                         Usd_InterpolatorBase *interpolator,
                         T *result)
{
    const SdfAbstractDataSpecId specId(
        &info.primPathInLayerStack, &attr.GetName());
    const SdfLayerRefPtr& layer = 
        info.layerStack->GetLayers()[info.layerIndex];
    const double localTime = info.offset * time.GetValue();

    double upper = 0.0;
    double lower = 0.0;

    if (lowerHint and upperHint) {
        lower = *lowerHint;
        upper = *upperHint;
    }
    else {
        if (not TF_VERIFY(layer->GetBracketingTimeSamplesForPath(
                    specId, localTime, &lower, &upper))) {
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

    if (GfIsClose(lower, upper, /* epsilon = */ 1e-6)) {
        bool queryResult = layer->QueryTimeSample(specId, lower, result);
        return queryResult and (not _ClearValueIfBlocked(result));
    }

    return interpolator->Interpolate(
        attr, layer, specId, localTime, lower, upper);
} 

template <class T>
bool _GetClipValue(UsdTimeCode time, const UsdAttribute& attr,
                   const Usd_ResolveInfo &info,
                   const Usd_ClipRefPtr &clip,
                   double lower, double upper,
                   Usd_InterpolatorBase *interpolator,
                   T *result)
{
    const SdfAbstractDataSpecId specId(
        &info.primPathInLayerStack, &attr.GetName());
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

    if (GfIsClose(lower, upper, /* epsilon = */ 1e-6)) {
        bool queryResult = clip->QueryTimeSample(specId, lower, result);
        return queryResult and (not _ClearValueIfBlocked(result));
    }

    return interpolator->Interpolate(
        attr, clip, specId, localTime, lower, upper);
}

}

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
        Usd_ResolveInfo resolveInfo;
        _ExtraResolveInfo<SdfAbstractDataValue> extraResolveInfo;
        
        _GetResolveInfo(attr, &resolveInfo, &time, &extraResolveInfo);
        
        if (resolveInfo.source == Usd_ResolveInfoSourceTimeSamples or
            resolveInfo.source == Usd_ResolveInfoSourceDefault) {
            resultLayer = 
                resolveInfo.layerStack->GetLayers()[resolveInfo.layerIndex];
        }
        else if (resolveInfo.source == Usd_ResolveInfoSourceValueClips) {
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
        bool metadataFetched = _GetMetadata(attr, SdfFieldKeys->Default,
                                            TfToken(), /*useFallbacks=*/true, 
                                            result);
        return metadataFetched and (not _ClearValueIfBlocked(result));
    }

    Usd_ResolveInfo resolveInfo;
    _ExtraResolveInfo<T> extraResolveInfo;
    extraResolveInfo.defaultOrFallbackValue = result;

    TfErrorMark m;
    _GetResolveInfo(attr, &resolveInfo, &time, &extraResolveInfo);

    if (resolveInfo.source == Usd_ResolveInfoSourceTimeSamples) {
        return _GetTimeSampleValue(
            time, attr, resolveInfo, 
            &extraResolveInfo.lowerSample, &extraResolveInfo.upperSample,
            interpolator, result);
    }
    else if (resolveInfo.source == Usd_ResolveInfoSourceValueClips) {
        return _GetClipValue(
            time, attr, resolveInfo, extraResolveInfo.clip,
            extraResolveInfo.lowerSample, extraResolveInfo.upperSample,
            interpolator, result);
    }
    else if (resolveInfo.source == Usd_ResolveInfoSourceDefault or
             resolveInfo.source == Usd_ResolveInfoSourceFallback) {
        // Nothing to do here -- the call to _GetResolveInfo will have
        // filled in the result with the default value.
        return m.IsClean();
    }

    return _GetValueFromResolveInfoImpl(
        resolveInfo, time, attr, interpolator, result);
}

namespace 
{
template <class ClipOrLayer>
bool
_HasTimeSamples(const ClipOrLayer& source, 
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

    return source->HasField(specId, SdfFieldKeys->TimeSamples);
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
    if (not value) {
        VtValue dummy;
        return _HasDefault(layer, specId, &dummy);
    }

    if (layer->HasField(specId, SdfFieldKeys->Default, value)) {
        if (_ClearValueIfBlocked(value)) {
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

// A 'Resolver' for filling Usd_ResolveInfo.
template <typename T>
struct UsdStage::_ResolveInfoResolver 
{
    explicit _ResolveInfoResolver(const UsdAttribute& attr,
                                 Usd_ResolveInfo* resolveInfo,
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
            _resolveInfo->source = Usd_ResolveInfoSourceFallback;
            return true;
        }

        // No values at all.
        _resolveInfo->source = Usd_ResolveInfoSourceNone;
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
        const SdfLayerOffset layerOffset = _GetLayerOffsetToRoot(node, 
            layerStack[layerStackPosition]);
        const SdfLayerRefPtr& layer = layerStack[layerStackPosition];
        boost::optional<double> localTime;
        if (time) {
            localTime = layerOffset * (*time);
        }

        if (_HasTimeSamples(layer, specId, localTime.get_ptr(), 
                            &_extraInfo->lowerSample, 
                            &_extraInfo->upperSample)) {
            _resolveInfo->source = Usd_ResolveInfoSourceTimeSamples;
        }
        else { 
            _DefaultValueResult defValue = 
                _HasDefault(layer, specId, _extraInfo->defaultOrFallbackValue);
            if (defValue == _DefaultValueFound) {
                _resolveInfo->source = Usd_ResolveInfoSourceDefault;
            }
            else if (defValue == _DefaultValueBlocked) {
                _resolveInfo->valueIsBlocked = true;
                return ProcessFallback();
            }
        }

        if (_resolveInfo->source != Usd_ResolveInfoSourceNone) {
            _resolveInfo->layerStack = nodeLayers;
            _resolveInfo->layerIndex = layerStackPosition;
            _resolveInfo->primPathInLayerStack = node.GetPath();
            _resolveInfo->offset = layerOffset;
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
            _resolveInfo->source = time ?
                Usd_ResolveInfoSourceValueClips :
                Usd_ResolveInfoSourceIsTimeDependent;
            _resolveInfo->layerStack = node.GetLayerStack();
            _resolveInfo->primPathInLayerStack = node.GetPath();
            return true;
        }

        return false;
    }

private:
    const UsdAttribute& _attr;
    Usd_ResolveInfo* _resolveInfo;
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
                          Usd_ResolveInfo *resolveInfo,
                          const UsdTimeCode *time, 
                          _ExtraResolveInfo<T> *extraInfo) const
{
    _ExtraResolveInfo<T> localExtraInfo;
    if (not extraInfo) {
        extraInfo = &localExtraInfo;
    }

    _ResolveInfoResolver<T> resolver(attr, resolveInfo, extraInfo);
    _GetResolvedValueImpl(attr, &resolver, time);
    
    if (TfDebug::IsEnabled(USD_VALIDATE_VARIABILITY) &&
        (resolveInfo->source == Usd_ResolveInfoSourceTimeSamples ||
         resolveInfo->source == Usd_ResolveInfoSourceValueClips ||
         resolveInfo->source == Usd_ResolveInfoSourceIsTimeDependent) &&
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
    const UsdPrim prim = prop.GetPrim();
    boost::optional<double> localTime;
    if (time and not time->IsDefault()) {
        localTime = time->GetValue();
    }

    // Retrieve all clips that may contribute time samples for this
    // attribute at the given time. Clips never contribute default
    // values.
    const std::vector<Usd_ClipCache::Clips>* clipsAffectingPrim = nullptr;
    if (prim._Prim()->MayHaveOpinionsInClips()
        and (not time or not time->IsDefault())) {
        clipsAffectingPrim = &(_clipCache->GetClipsForPrim(prim.GetPath()));
    }

    // Clips may contribute opinions at nodes where no specs for the attribute
    // exist in the node's LayerStack. So, if we have any clips, tell
    // Usd_Resolver that we want to iterate over 'empty' nodes as well.
    const bool skipEmptyNodes = (bool)(not clipsAffectingPrim);

    for (Usd_Resolver res(&prim.GetPrimIndex(), skipEmptyNodes); 
         res.IsValid(); res.NextNode()) {

        const PcpNodeRef& node = res.GetNode();
        const bool nodeHasSpecs = node.HasSpecs();
        if (not nodeHasSpecs and not clipsAffectingPrim) {
            continue;
        }

        const SdfAbstractDataSpecId specId(&node.GetPath(), &prop.GetName());
        const SdfLayerRefPtrVector& layerStack 
            = node.GetLayerStack()->GetLayers();
        boost::optional<const Usd_ClipCache::Clips*> clips;
        for (size_t i = 0, e = layerStack.size(); i < e; ++i) {
            if (nodeHasSpecs) { 
                if (resolver->ProcessLayer(i, specId, node, 
                                           localTime.get_ptr())) {
                    return;
                }
            }

            if (clipsAffectingPrim){ 
                if (not clips) {
                    clips = _GetClipsThatApplyToNode(*clipsAffectingPrim,
                                                     node, specId);
                    // If we don't have specs on this node and clips don't
                    // apply we can mode onto the next node.
                    if (not nodeHasSpecs and not clips.get()) { 
                        break; 
                    }
                }
                
                // gcc 4.8 incorrectly detects boost::optional as uninitialized. 
                // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
                ARCH_PRAGMA_PUSH_NOERROR_MAYBE_UNINITIALIZED;

                // We only care about clips that were introduced at this
                // position within the LayerStack.
                if (not clips.get() or clips.get()->sourceLayerIndex != i) {
                    continue;
                }

                // Look through clips to see if they have a time sample for
                // this attribute. If a time is given, examine just the clips
                // that are active at that time.
                for (const auto& clip : clips.get()->valueClips) {
                    if (resolver->ProcessClip(clip, specId, node,
                                              localTime.get_ptr())) {
                        return;
                    }
                }
                ARCH_PRAGMA_POP_NOERROR_MAYBE_UNINITIALIZED;
            }    
        }
    }

    resolver->ProcessFallback();
}

void
UsdStage::_GetResolveInfo(const UsdAttribute &attr, 
                          Usd_ResolveInfo *resolveInfo) const
{
    _GetResolveInfo<SdfAbstractDataValue>(attr, resolveInfo);
}

template <class T>
bool 
UsdStage::_GetValueFromResolveInfoImpl(const Usd_ResolveInfo &info,
                                       UsdTimeCode time, const UsdAttribute &attr,
                                       Usd_InterpolatorBase* interpolator,
                                       T* result) const
{
    if (time.IsDefault())
        return _GetMetadata(attr, SdfFieldKeys->Default,
                            TfToken(), /*useFallbacks=*/true, result);

    if (info.source == Usd_ResolveInfoSourceTimeSamples) {
        return _GetTimeSampleValue(
            time, attr, info, nullptr, nullptr, interpolator, result);
    }
    else if (info.source == Usd_ResolveInfoSourceDefault) {
        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());
        const SdfLayerHandle& layer = 
            info.layerStack->GetLayers()[info.layerIndex];

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
    else if (info.source == Usd_ResolveInfoSourceValueClips) {
        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());

        const UsdPrim prim = attr.GetPrim();
        const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        TF_FOR_ALL(clipsIt, clipsAffectingPrim) {
            const Usd_ClipRefPtrVector& clips = clipsIt->valueClips;
            for (size_t i = 0, numClips = clips.size(); i < numClips; ++i) {
                // Note that we do not apply layer offsets to the time.
                // Because clip metadata may be authored in different 
                // layers in the LayerStack, each with their own 
                // layer offsets, it is simpler to bake the effects of 
                // those offsets into Usd_Clip.
                const Usd_ClipRefPtr& clip = clips[i];
                const double localTime = time.GetValue();
                
                if (not _ClipAppliesToLayerStackSite(
                        clip, info.layerStack, info.primPathInLayerStack) 
                    or localTime < clip->startTime
                    or localTime >= clip->endTime) {
                    continue;
                }

                double upper = 0.0;
                double lower = 0.0;
                if (clip->GetBracketingTimeSamplesForPath(
                        specId, localTime, &lower, &upper)) {
                    return _GetClipValue(
                        time, attr, info, clip, lower, upper, interpolator, 
                        result);
                }
            }
        }
    }
    else if (info.source == Usd_ResolveInfoSourceIsTimeDependent) {
        // In this case, we obtained a resolve info for an attribute value whose
        // value source may vary over time.  So we must fall back on invoking
        // the normal Get() machinery now that we actually have a specific time.
        return _GetValueImpl(time, attr, interpolator, result);
    }
    else if (info.source == Usd_ResolveInfoSourceFallback) {
        return _GetFallbackMetadata(attr, SdfFieldKeys->Default, 
                                    TfToken(), result);
    }

    return false;
}

bool 
UsdStage::_GetValueFromResolveInfo(const Usd_ResolveInfo &info,
                                   UsdTimeCode time, const UsdAttribute &attr,
                                   VtValue* value) const
{
    Usd_UntypedInterpolator interpolator(value);
    return _GetValueFromResolveInfoImpl(
        info, time, attr, &interpolator, value);
}

template <class T>
bool 
UsdStage::_GetValueFromResolveInfo(const Usd_ResolveInfo &info,
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
UsdStage::_GetTimeSampleMap(const UsdAttribute &attr,
                            SdfTimeSampleMap *out) const
{
    std::vector<double> timeSamples;
    if (_GetTimeSamplesInInterval(attr, GfInterval::GetFullInterval(), 
                                  &timeSamples)) {
        // Interpolation should not be triggered below, since we are asking
        // for values on times where we know there are authored time samples.
        Usd_NullInterpolator nullInterpolator;

        TF_FOR_ALL(t, timeSamples) {
            VtValue value;
            if (_GetValueImpl(*t, attr, &nullInterpolator, &value)) {
                (*out)[*t] = value;
            } else {
                (*out)[*t] = VtValue(SdfValueBlock());
            }
        }
        return true;
    }
    return false;
}

bool
UsdStage::_GetTimeSamplesInInterval(const UsdAttribute& attr,
                                    const GfInterval& interval,
                                    std::vector<double>* times) const
{
    Usd_ResolveInfo info;
    _GetResolveInfo(attr, &info);
    return _GetTimeSamplesInIntervalFromResolveInfo(info, attr, interval, times);
}

SdfTimeSampleMap
UsdStage::_GetTimeSampleMap(const UsdAttribute &attr) const
{
    SdfTimeSampleMap result;
    _GetTimeSampleMap(attr, &result);
    return result;
}

bool 
UsdStage::_GetTimeSamplesInIntervalFromResolveInfo(
    const Usd_ResolveInfo &info,
    const UsdAttribute &attr,
    const GfInterval& interval,
    std::vector<double>* times) const
{
    if ((interval.IsMinFinite() and interval.IsMinOpen())
        or (interval.IsMaxFinite() and interval.IsMaxOpen())) {
        TF_CODING_ERROR("Finite endpoints in the specified interval (%s)"
                        "must be closed.", TfStringify(interval).c_str());
        return false;
    }

    const auto copySamplesInInterval = [](const std::set<double>& samples, 
                                          vector<double>* target, 
                                          const GfInterval& interval) 
    {
            const auto sampleRangeBegin = std::lower_bound(samples.begin(),
                samples.end(), interval.GetMin());
            const auto sampleRangeEnd = std::upper_bound(sampleRangeBegin,
                samples.end(), interval.GetMax());
            target->insert(target->end(), sampleRangeBegin, sampleRangeEnd);
    };

    if (info.source == Usd_ResolveInfoSourceTimeSamples) {
        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());
        const SdfLayerRefPtr& layer = 
            info.layerStack->GetLayers()[info.layerIndex];

        const std::set<double> samples = layer->ListTimeSamplesForPath(specId);
        if (not samples.empty()) {
            copySamplesInInterval(samples, times, interval);
            const SdfLayerOffset offset = info.offset.GetInverse();
            if (not offset.IsIdentity()) {
                for (auto &time : *times) {
                    time = offset * time;
                }
            }
        }

        return true;
    }
    else if (info.source == Usd_ResolveInfoSourceValueClips ||
             info.source == Usd_ResolveInfoSourceIsTimeDependent) {
        const UsdPrim prim = attr.GetPrim();

        // See comments in _GetValueImpl regarding clips.
        const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());

        std::vector<double> timesFromAllClips;

        // Loop through all the clips that apply to this node and
        // combine all the time samples that are provided.
        TF_FOR_ALL(clipsIt, clipsAffectingPrim) {
            TF_FOR_ALL(clipIt, clipsIt->valueClips) {
                const Usd_ClipRefPtr& clip = *clipIt;
                if (not _ClipAppliesToLayerStackSite(
                        clip, info.layerStack, info.primPathInLayerStack)) {
                    continue;
                }

                const auto clipInterval 
                    = GfInterval(clip->startTime, clip->endTime);
                
                // if we are constraining our range, and none of our range
                // intersects with the specified clip range, we can ignore
                // and move on to the next clip.
                if (not interval.Intersects(clipInterval)) {
                    continue;
                }
                
                // See comments in _GetValueImpl regarding layer
                // offsets and why they're not applied here.
                const auto samples = clip->ListTimeSamplesForPath(specId);
                if (not samples.empty()) {
                    copySamplesInInterval(samples, &timesFromAllClips, interval);
                }

                // Clips introduce time samples at their boundaries to
                // isolate them from surrounding clips, even if time samples
                // don't actually exist. 
                //
                // See _GetBracketingTimeSamplesFromResolveInfo for more
                // details.
                if (interval.Contains(clipInterval.GetMin())
                    and clipInterval.GetMin() 
                        != -std::numeric_limits<double>::max()) {
                    timesFromAllClips.push_back(clip->startTime);
                }

                if (interval.Contains(clipInterval.GetMax())
                    and clipInterval.GetMax() 
                        != std::numeric_limits<double>::max()){
                    timesFromAllClips.push_back(clip->endTime);
                }
            }

            if (not timesFromAllClips.empty()) {
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
    Usd_ResolveInfo info;
    _GetResolveInfo(attr, &info);
    return _GetNumTimeSamplesFromResolveInfo(info, attr);
   
}

size_t 
UsdStage::_GetNumTimeSamplesFromResolveInfo(const Usd_ResolveInfo &info,
                                            const UsdAttribute &attr) const
{
    if (info.source == Usd_ResolveInfoSourceTimeSamples) {
        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());
        const SdfLayerRefPtr& layer = 
            info.layerStack->GetLayers()[info.layerIndex];

        return layer->GetNumTimeSamplesForPath(specId);
    } 
    else if (info.source == Usd_ResolveInfoSourceValueClips ||
             info.source == Usd_ResolveInfoSourceIsTimeDependent) {
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
        Usd_ResolveInfo resolveInfo;
        _GetResolveInfo<SdfAbstractDataValue>(attr, &resolveInfo);
        return _GetBracketingTimeSamplesFromResolveInfo(
            resolveInfo, attr, desiredTime, requireAuthored, lower, upper, 
            hasSamples);
    }

    const UsdTimeCode time(desiredTime);

    Usd_ResolveInfo resolveInfo;
    _ExtraResolveInfo<SdfAbstractDataValue> extraInfo;

    _GetResolveInfo<SdfAbstractDataValue>(
        attr, &resolveInfo, &time, &extraInfo);

    if (resolveInfo.source == Usd_ResolveInfoSourceTimeSamples) {
        // In the time samples case, we bail out early to avoid another
        // call to SdfLayer::GetBracketingTimeSamples. _GetResolveInfo will 
        // already have filled in the lower and upper samples with the
        // results of that function at the desired time.
        *lower = extraInfo.lowerSample;
        *upper = extraInfo.upperSample;

        if (not resolveInfo.offset.IsIdentity()) {
            const SdfLayerOffset offset = resolveInfo.offset.GetInverse();
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
UsdStage::_GetBracketingTimeSamplesFromResolveInfo(const Usd_ResolveInfo &info,
                                                   const UsdAttribute &attr,
                                                   double desiredTime,
                                                   bool requireAuthored,
                                                   double* lower,
                                                   double* upper,
                                                   bool* hasSamples) const
{
    if (info.source == Usd_ResolveInfoSourceTimeSamples) {
        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());
        const SdfLayerRefPtr& layer = 
            info.layerStack->GetLayers()[info.layerIndex];
        const double layerTime = info.offset * desiredTime;
        
        if (layer->GetBracketingTimeSamplesForPath(
                specId, layerTime, lower, upper)) {

            if (not info.offset.IsIdentity()) {
                const SdfLayerOffset offset = info.offset.GetInverse();
                *lower = offset * (*lower);
                *upper = offset * (*upper);
            }

            *hasSamples = true;
            return true;
        }
    }
    else if (info.source == Usd_ResolveInfoSourceDefault) {
        *hasSamples = false;
        return true;
    }
    else if (info.source == Usd_ResolveInfoSourceValueClips ||
             info.source == Usd_ResolveInfoSourceIsTimeDependent) {
        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());

        const UsdPrim prim = attr.GetPrim();

        // See comments in _GetValueImpl regarding clips.
        const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        TF_FOR_ALL(clipsIt, clipsAffectingPrim) {
            TF_FOR_ALL(clipIt, clipsIt->valueClips) {
                const Usd_ClipRefPtr& clip = *clipIt;

                if (not _ClipAppliesToLayerStackSite(
                        clip, info.layerStack, info.primPathInLayerStack)
                    or desiredTime < clip->startTime
                    or desiredTime >= clip->endTime) {
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

                if (not foundLower and 
                    clip->startTime != -std::numeric_limits<double>::max()) {
                    *lower = clip->startTime;
                    foundLower = true;
                }

                if (not foundUpper and
                    clip->endTime != std::numeric_limits<double>::max()) {
                    *upper = clip->endTime;
                    foundUpper = true;
                }

                if (foundLower and not foundUpper) {
                    *upper = *lower;
                }
                else if (not foundLower and foundUpper) {
                    *lower = *upper;
                }
                
                // 'or' is correct here. Consider the case where we only
                // have a single clip and desiredTime is earlier than the
                // first time sample -- foundLower will be false, but we
                // want to return the bracketing samples from the sole
                // clip anyway.
                if (foundLower or foundUpper) {
                    *hasSamples = true;
                    return true;
                }
            }
        }
    }
    else if (info.source == Usd_ResolveInfoSourceFallback) {
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
    if (firstClipWithSamples->startTime == -std::numeric_limits<double>::max()
      and firstClipWithSamples->endTime == std::numeric_limits<double>::max()) {
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
    Usd_ResolveInfo info;
    _ExtraResolveInfo<SdfAbstractDataValue> extraInfo;
    _GetResolveInfo(attr, &info, NULL, &extraInfo);

    if (info.source == Usd_ResolveInfoSourceValueClips ||
        info.source == Usd_ResolveInfoSourceIsTimeDependent) {
        // See comment in _ValueMightBeTimeVaryingFromResolveInfo.
        // We can short-cut the work in that function because _GetResolveInfo
        // gives us the first clip that has time samples for this attribute.
        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());
        return _ValueFromClipsMightBeTimeVarying(extraInfo.clip, specId);
    }

    return _ValueMightBeTimeVaryingFromResolveInfo(info, attr);
}

bool 
UsdStage::_ValueMightBeTimeVaryingFromResolveInfo(const Usd_ResolveInfo &info,
                                                  const UsdAttribute &attr) const
{
    if (info.source == Usd_ResolveInfoSourceValueClips ||
        info.source == Usd_ResolveInfoSourceIsTimeDependent) {
        // In the case that the attribute value comes from a value clip, we
        // need to find the first clip that has samples for attr to see if the
        // clip values may be time varying. This is potentially much more 
        // efficient than the _GetNumTimeSamples check below, since that 
        // requires us to open every clip to get the time sample count.
        //
        // Note that we still wind up checking every clip if none of them
        // have samples for this attribute.
        const SdfAbstractDataSpecId specId(
            &info.primPathInLayerStack, &attr.GetName());

        const std::vector<Usd_ClipCache::Clips>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(attr.GetPrim().GetPath());
        TF_FOR_ALL(clipsIt, clipsAffectingPrim) {
            TF_FOR_ALL(clipIt, clipsIt->valueClips) {
                const Usd_ClipRefPtr& clip = *clipIt;
                if (_ClipAppliesToLayerStackSite(
                        clip, info.layerStack, info.primPathInLayerStack)
                    and _HasTimeSamples(clip, specId)) {
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
    if (sessionLayer and 
        _HasLayerFieldOrDictKey(sessionLayer, key, keyPath, value)){
        VtValue rootValue;
        if (value and 
            value->IsHolding<VtDictionary>() and
            _HasLayerFieldOrDictKey(stage.GetRootLayer(), key, keyPath, 
                                    &rootValue) and
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
    if (not value){
        TF_CODING_ERROR(
            "Null out-param 'value' for UsdStage::GetMetadata(\"%s\")",
            key.GetText());
        return false;

    }
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (not schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot)){
        return false;
    }
    
    if (not _HasStageMetadataOrDictKey(*this, key, TfToken(), value)){
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
    
    if (not schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
        return false;

    return (HasAuthoredMetadata(key) or 
            not schema.GetFallback(key).IsEmpty());
}

bool
UsdStage::HasAuthoredMetadata(const TfToken& key) const
{
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (not schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
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
    
    if (not schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot)) {
        TF_CODING_ERROR("Metadata '%s' is not registered as valid Layer "
                        "metadata, and cannot be set on UsdStage %s.",
                        key.GetText(),
                        rootLayer->GetIdentifier().c_str());
        return false;
    }

    const SdfLayerHandle &editTargetLayer = stage.GetEditTarget().GetLayer();
    if (editTargetLayer == rootLayer or editTargetLayer == sessionLayer) {
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

    if (not schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot)) {
        TF_CODING_ERROR("Metadata '%s' is not registered as valid Layer "
                        "metadata, and cannot be cleared on UsdStage %s.",
                        key.GetText(),
                        rootLayer->GetIdentifier().c_str());
        return false;
    }

    const SdfLayerHandle &editTargetLayer = stage.GetEditTarget().GetLayer();
    if (editTargetLayer == rootLayer or editTargetLayer == sessionLayer) {
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
    
    if (not value){
        TF_CODING_ERROR(
            "Null out-param 'value' for UsdStage::GetMetadataByDictKey"
            "(\"%s\", \"%s\")",
            key.GetText(), keyPath.GetText());
        return false;

    }
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (not schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
        return false;

    if (not _HasStageMetadataOrDictKey(*this, key, keyPath, value)){
        const VtValue &fallback =  SdfSchema::GetInstance().GetFallback(key);
        if (not fallback.IsEmpty()){
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
        if (elt and elt->IsHolding<VtDictionary>()){
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
    
    if (keyPath.IsEmpty() or 
        not schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
        return false;

    if (HasAuthoredMetadataDictKey(key, keyPath))
        return true;

    const VtValue &fallback =  schema.GetFallback(key);
    
    return ((not fallback.IsEmpty()) and
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

    return (sessionLayer and
            ((sessionLayer->HasStartTimeCode() and sessionLayer->HasEndTimeCode()) or
             (_HasStartFrame(sessionLayer) and _HasEndFrame(sessionLayer)))) or
           (rootLayer and
            ((rootLayer->HasStartTimeCode() and rootLayer->HasEndTimeCode()) or 
             (_HasStartFrame(rootLayer) and _HasEndFrame(rootLayer))));
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
        else if (anchor->IsAnonymous() and 
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
    if (not stage) {
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
        UsdTimeCode, const UsdAttribute&,                                   \
        SDF_VALUE_TRAITS_TYPE(elem)::Type*) const;                      \
    template bool UsdStage::_GetValue(                                  \
        UsdTimeCode, const UsdAttribute&,                                   \
        SDF_VALUE_TRAITS_TYPE(elem)::ShapedType*) const;                \
                                                                        \
    template bool UsdStage::_GetValueFromResolveInfo(                   \
        const Usd_ResolveInfo&, UsdTimeCode, const UsdAttribute&,           \
        SDF_VALUE_TRAITS_TYPE(elem)::Type*) const;                      \
    template bool UsdStage::_GetValueFromResolveInfo(                   \
        const Usd_ResolveInfo&, UsdTimeCode, const UsdAttribute&,           \
        SDF_VALUE_TRAITS_TYPE(elem)::ShapedType*) const;                      

BOOST_PP_SEQ_FOR_EACH(_INSTANTIATE_GET, ~, SDF_VALUE_TYPES)
#undef _INSTANTIATE_GET
