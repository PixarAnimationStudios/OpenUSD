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

PXR_NAMESPACE_OPEN_SCOPE


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
_ResolveAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& assetPath)
{
    if (assetPath.empty() ||
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
    Close();
    if (_mallocTagID != _dormantMallocTagID){
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
                if (!active) {
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
                || !_instanceCache->GetMasterUsingPrimIndexAtPath(
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
        "Instantiating stage");
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
UsdStage::CreateNew(const std::string& identifier)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, _CreateAnonymousSessionLayer(layer));
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    const SdfLayerHandle& sessionLayer)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(identifier));

    if (SdfLayerRefPtr layer = _CreateNewLayer(identifier))
        return Open(layer, sessionLayer);
    return TfNullPtr;
}

/* static */
UsdStageRefPtr
UsdStage::CreateNew(const std::string& identifier,
                    const ArResolverContext& pathResolverContext)
{
    TfAutoMallocTag2 tag("Usd", _StageTag(identifier));

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
    TfAutoMallocTag2 tag("Usd", _StageTag(identifier));

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

    if (!specToCopy) {
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

namespace {
bool 
_ValueContainsBlock(const VtValue* value) {
    return value && value->IsHolding<SdfValueBlock>();
}

bool
_ValueContainsBlock(const SdfAbstractDataValue* value) 
{
    return value && value->isValueBlock;
}

bool
_ValueContainsBlock(const SdfAbstractDataConstValue* value)
{
    const std::type_info& valueBlockTypeId(typeid(SdfValueBlock));
    return value && value->valueType == valueBlockTypeId;
}

bool 
_ClearValueIfBlocked(VtValue* value) {
    if (_ValueContainsBlock(value)) {
        *value = VtValue();
        return true;
    }

    return false;
}

bool 
_ClearValueIfBlocked(SdfAbstractDataValue* value) {
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
    if (!_ValueContainsBlock(&newValue)) {
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
        const SdfLayerOffset layerOffset = 
            GetEditTarget().GetMapFunction().GetTimeOffset();

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

    const SdfLayerOffset layerOffset = 
        editTarget.GetMapFunction().GetTimeOffset();

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
        spec = _CreatePrimSpecForEditing(obj.GetPrimPath());

    if (!TF_VERIFY(spec, 
                      "No spec at <%s> in layer @%s@",
                      editTarget.MapToSpecPath(obj.GetPath()).GetText(),
                      GetEditTarget().GetLayer()->GetIdentifier().c_str())) {
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
    if (field && (field->IsReadOnly() || field->HoldsChildren()))
        return true;

    // The field is not private.
    return false;
}

UsdPrim
UsdStage::GetPseudoRoot() const
{
    return UsdPrim(_pseudoRoot, SdfPath::AbsoluteRootPath());
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
    // If this path points to a prim beneath an instance, return
    // an instance proxy that uses the prim data from the corresponding
    // prim in the master but appears to be a prim at the given path.
    Usd_PrimDataConstPtr primData = _GetPrimDataAtPathOrInMaster(path);
    return UsdPrim(primData, primData ? path : SdfPath());
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
            _instanceCache->GetPrimInMasterForPath(path);
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
            if (curPrim = GetPrimAtPath(parentPath)) {
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
    UsdPrimRange childIt = UsdPrimRange::AllPrims(prim);
    WorkParallelForEach(
        childIt, childIt.GetEnd(),
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
                            SdfPathSet* primIndexPaths,
                            bool unloadedOnly,
                            SdfPathSet* usdPrimPaths) const
{
    tbb::concurrent_vector<SdfPath> primIndexPathsVec;
    tbb::concurrent_vector<SdfPath> usdPrimPathsVec;

    _WalkPrimsWithMasters(
        rootPath,
        [this, unloadedOnly, primIndexPaths, usdPrimPaths,
         &primIndexPathsVec, &usdPrimPathsVec]
        (UsdPrim const &prim) {
            // Inactive prims are never included in this query.  Masters are
            // also never included, since they aren't independently loadable.
            if (!prim.IsActive() || prim.IsMaster())
                return;
            
            if (prim._GetSourcePrimIndex().HasPayload()) {
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
        });

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

        if (parent._GetSourcePrimIndex().HasPayload()) {
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
    if (aggregateLoads.empty() && aggregateUnloads.empty()) {
        return;
    }

    UsdStageWeakPtr self(this);
    SdfPathVector pathsToRecomposeVec, otherPaths;
    pathsToRecomposeVec.insert(pathsToRecomposeVec.begin(), 
                               aggregateLoads.begin(), aggregateLoads.end());
    pathsToRecomposeVec.insert(pathsToRecomposeVec.begin(),
                               aggregateUnloads.begin(), aggregateUnloads.end());
    SdfPath::RemoveDescendentPaths(&pathsToRecomposeVec);
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

    // It's important that we do not include payloads that were previously
    // loaded because we need to iterate and will enter an infinite loop if we
    // do not reduce the load set on each iteration. This manifests below in
    // the unloadedOnly=true argument.
    for (auto const &path : loadSet) {
        if (!_IsValidForLoad(path)) {
            continue;
        }
        _DiscoverPayloads(path, &finalLoadSet, /*unloadedOnly=*/true);
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
            [this, &unloadIndexPaths] (UsdPrim const &prim) {
                if (prim.IsInMaster() && prim.HasPayload()) {
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
    _Recompose(changes, NULL);

    // Recurse.
    //
    // Note that recursion is not necessary for the unload set, which gets upon
    // the first recursion.
    
    // aggregate our results for notification
    if (aggregateLoads && aggregateUnloads) {
        aggregateLoads->insert(finalLoadSet.begin(), finalLoadSet.end());
        aggregateUnloads->insert(finalUnloadSet.begin(), finalUnloadSet.end());
    }

    _LoadAndUnload(loadSet, SdfPathSet(), aggregateLoads, aggregateUnloads);
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
    if (!Usd_InstanceCache::IsPathMasterOrInMaster(path)) {
        const SdfPath pathInMaster = 
            _instanceCache->GetPrimInMasterForPath(path);
        if (!pathInMaster.IsEmpty()) {
            path = pathInMaster;
        }
    }

    SdfPathSet loadable;
    _DiscoverPayloads(path, NULL, /* unloadedOnly = */ false, &loadable);
    return loadable;
}

void
UsdStage::SetPopulationMask(UsdStagePopulationMask const &mask)
{
    // For now just set the mask and recompose everything at the Usd level.
    _populationMask = mask;
    SdfPathSet absRoot = { SdfPath::AbsoluteRootPath() };
    _Recompose(PcpChanges(), &absRoot);
}

void
UsdStage::ExpandPopulationMask(
    std::function<bool (UsdRelationship const &)> const &pred)
{
    if (GetPopulationMask().IncludesSubtree(SdfPath::AbsoluteRootPath()))
        return;

    // Walk everything, calling UsdPrim::FindAllRelationshipTargetPaths() and
    // include them in the mask.  If the mask changes, call SetPopulationMask()
    // and redo.  Continue until the mask ceases expansion.  
    while (true) {
        SdfPathVector tgtPaths =
            GetPseudoRoot().FindAllRelationshipTargetPaths(pred, false);
        
        tgtPaths.erase(remove_if(tgtPaths.begin(), tgtPaths.end(),
                                 [this](SdfPath const &path) {
                                     return _populationMask.Includes(path);
                                 }),
                       tgtPaths.end());
        
        if (tgtPaths.empty())
            break;

        auto popMask = GetPopulationMask();
        for (auto const &path: tgtPaths) {
            popMask.Add(path);
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
            _instanceCache->GetMasterUsingPrimIndexAtPath(sourceIndexPath);

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
    if (!prim->_firstChild) {
        TF_DEBUG(USD_COMPOSITION).Msg("Children all new <%s>\n",
                                      prim->GetPath().GetText());
        SdfPath parentPath = prim->GetPath();
        Usd_PrimDataPtr head = NULL, prev = NULL, cur = NULL;
        for (const auto& child : nameOrder) {
            cur = _InstantiatePrim(parentPath.AppendChild(child));
            if (recurse) {
                _ComposeChildSubtree(cur, prim, mask);
            }
            if (!prev) {
                head = cur;
            }
            else {
                prev->_SetSiblingLink(cur);
            }
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
        for (; cur != end && curName != nameEnd; ++cur, ++curName) {
            if ((*cur)->GetName() != *curName)
                break;
        }
        if (cur == end && curName == nameEnd) {
            TF_DEBUG(USD_COMPOSITION).Msg("Children same in same order <%s>\n",
                                          prim->GetPath().GetText());
            if (recurse) {
                for (cur = begin; cur != end; ++cur) {
                    _ComposeChildSubtree(*cur, prim, mask);
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
                 (oldChildIt == oldChildEnd      ||
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

    // Now all the new children are in lexicographical order by name, paired
    // with their name's iterator in the original name order.  Recover the
    // original order by sorting by the iterators natural order.
    sort(tempChildren.begin(), tempChildren.end(), _SecondLess());

    // Now copy the correctly ordered children into place.
    prim->_firstChild = nullptr;
    TF_REVERSE_FOR_ALL(i, tempChildren)
        prim->_AddChild(i->first);
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

void
UsdStage::_ReportErrors(const PcpErrorVector &errors,
                        const std::vector<std::string> &otherErrors,
                        const std::string &context) const
{
    // Report any errors.
    if (!errors.empty() || !otherErrors.empty()) {
        std::string message = context + ":\n";
        for (const auto& err : errors) {
            message += "    " + TfStringReplace(err->ToString(), "\n", "\n    ") 
                       + '\n';
        }
        for (const auto& err : otherErrors) {
            message += "    " + TfStringReplace(err, "\n", "\n    ") + '\n';
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
            errors, TfStringPrintf("Computing prim index <%s>",
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
    if (ARCH_UNLIKELY(!path.IsAbsolutePath())) {
        TF_CODING_ERROR("Path must be an absolute path: <%s>", path.GetText());
        return false;
    }

    // Path must be a prim path (or the absolute root path).
    if (ARCH_UNLIKELY(!path.IsAbsoluteRootOrPrimPath())) {
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
    if (!_CheckAbsolutePrimPath(path))
        return UsdPrim();

    // If there is already a UsdPrim at the given path, grab it.
    UsdPrim prim = GetPrimAtPath(path);

    // Do the authoring, if any to do.
    if (!prim) {
        {
            SdfChangeBlock block;
            TfErrorMark m;
            SdfPrimSpecHandle primSpec = _CreatePrimSpecForEditing(path);
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
    if (!_CheckAbsolutePrimPath(path))
        return UsdPrim();

    // Define all ancestors.
    if (!DefinePrim(path.GetParentPath()))
        return UsdPrim();
    
    // Now author scene description for this prim.
    TfErrorMark m;
    UsdPrim prim = GetPrimAtPath(path);
    if (!prim || !prim.IsDefined() ||
        (!typeName.IsEmpty() && prim.GetTypeName() != typeName)) {
        {
            SdfChangeBlock block;
            SdfPrimSpecHandle primSpec = _CreatePrimSpecForEditing(path);
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

    // It's an error to try to transform a defined non-class into a class.
    UsdPrim prim = GetPrimAtPath(path);
    if (prim && prim.IsDefined() &&
        prim.GetSpecifier() != SdfSpecifierClass) {
        TF_RUNTIME_ERROR("Non-class prim already exists at <%s>",
                         path.GetText());
        return UsdPrim();
    }

    // Stamp a class PrimSpec if need-be.
    if (!prim || !prim.IsAbstract()) {
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
    return UsdPrimRange::Stage(UsdStagePtr(this),
                                  Usd_PrimFlagsPredicate::Tautology());
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

// Add paths in the given cache that depend on the given path in the given layer
// to the output.
static void
_AddDependentPaths(const SdfLayerHandle &layer, const SdfPath &path,
                   const PcpCache &cache, SdfPathSet *output)
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
        output->insert(path.StripAllVariantSelections());
    }

    for (const PcpDependency& dep:
         cache.FindSiteDependencies(layer, path, depTypes,
                                    /* recurseOnSite */ true,
                                    /* recurseOnIndex */ false,
                                    filterForExistingCachesOnly)) {
        output->insert(dep.indexPath);
    }

    TF_DEBUG(USD_CHANGES).Msg(
        "Adding paths that use <%s> in layer @%s@: %s\n",
        path.GetText(),
        layer->GetIdentifier().c_str(),
        TfStringify(*output).c_str());
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
    SdfPathSet pathsToRecompose, otherResyncPaths, otherChangedPaths;

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

            TF_DEBUG(USD_CHANGES).Msg(
                "<%s> in @%s@ changed.\n",
                path.GetText(), 
                layerAndChangelist.first->GetIdentifier().c_str());

            bool willRecompose = false;
            if (path == SdfPath::AbsoluteRootPath() ||
                path.IsPrimOrPrimVariantSelectionPath()) {

                if (entry.flags.didReorderChildren) {
                    willRecompose = true;
                } else {
                    for (const auto& info : entry.infoChanged) {
                        const auto infoKey = info.first;
                        if ((infoKey == SdfFieldKeys->Active)    ||
                            (infoKey == SdfFieldKeys->Kind)      ||
                            (infoKey == SdfFieldKeys->TypeName)  ||
                            (infoKey == SdfFieldKeys->Specifier) ||
                            
                            // XXX: Could be more specific when recomposing due
                            //      to clip changes. E.g., only update the clip
                            //      resolver and bits on each prim.
                            UsdIsClipRelatedField(infoKey)) {

                            TF_DEBUG(USD_CHANGES).Msg(
                                "Changed field: %s\n",
                                infoKey.GetText());

                            willRecompose = true;
                            break;
                        }
                    }
                }

                if (willRecompose) {
                    _AddDependentPaths(layerAndChangelist.first, path, 
                                       *_cache, &pathsToRecompose);
            }
            }
            else {
                if (path.IsPropertyPath()) {
                    willRecompose = 
                        entry.flags.didAddPropertyWithOnlyRequiredFields    ||
                        entry.flags.didAddProperty                          ||
                        entry.flags.didRemovePropertyWithOnlyRequiredFields ||
                        entry.flags.didRemoveProperty;
                }
                else if (path.IsTargetPath()) {
                    // XXX: This will cause us to include target paths like
                    // /Foo.rel[/Bar] in the resynced path list in the
                    // ObjectsChanged notice we emit. This is a bug; no such
                    // object exists in the USD scenegraph. Keeping this here
                    // for now to maintain current behavior.
                    willRecompose =
                        entry.flags.didAddTarget ||
                        entry.flags.didRemoveTarget;
                }

                if (willRecompose) {
                    _AddDependentPaths(layerAndChangelist.first, path, 
                                       *_cache, &otherResyncPaths);
                }
            }

            // If we're not going to recompose this path, record the dependent
            // scene paths separately so we can notify clients about the
            // changes.
            if (!willRecompose) {
                _AddDependentPaths(layerAndChangelist.first, path, 
                                   *_cache, &otherChangedPaths);
            }
        }
    }

    PcpChanges changes;
    changes.DidChange(std::vector<PcpCache*>(1, _cache.get()),
                      n.GetChangeListMap());
    _Recompose(changes, &pathsToRecompose);

    // Add in all other paths that are marked as resynced here so
    // that any descendents of resynced prims are removed below.
    pathsToRecompose.insert(otherResyncPaths.begin(), otherResyncPaths.end());

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
    if (!pathsToRecomposeVec.empty() && 
        pathsToRecomposeVec.front() == SdfPath::AbsoluteRootPath()) {
        // If the pseudo-root is present, it should be the only path in the
        // vector.
        TF_VERIFY(pathsToRecomposeVec.size() == 1);
        otherChangedPaths.clear();
    }

    SdfPathVector otherChangedPathsVec;
    otherChangedPathsVec.reserve(otherChangedPaths.size());
    remove_copy_if(otherChangedPaths.begin(), otherChangedPaths.end(),
                   back_inserter(otherChangedPathsVec),
                   bind(&UsdStage::_IsObjectElidedFromStage, this, _1));

    // Now we want to remove all elements of otherChangedPathsVec that are
    // prefixed by elements in pathsToRecompose.
    SdfPathVector::iterator
        other = otherChangedPathsVec.begin(),
        otherEnd = otherChangedPathsVec.end();
    SdfPathVector::const_iterator
        recomp = pathsToRecomposeVec.begin(),
        recompEnd = pathsToRecomposeVec.end();
    while (recomp != recompEnd && other != otherEnd) {
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
    SdfPathSet newPathsToRecompose;
    SdfPathSet *pathsToRecompose = initialPathsToRecompose ?
        initialPathsToRecompose : &newPathsToRecompose;

    changes.Apply();

    const PcpChanges::CacheChanges &cacheChanges = changes.GetCacheChanges();

    if (!cacheChanges.empty()) {
        const PcpCacheChanges &ourChanges = cacheChanges.begin()->second;

        for (const auto& path : ourChanges.didChangeSignificantly) {
            // Translate the real path from Pcp into a stage-relative path
            pathsToRecompose->insert(path);
            TF_DEBUG(USD_CHANGES).Msg("Did Change Significantly: %s\n",
                                      path.GetText());
        }

        for (const auto& path : ourChanges.didChangeSpecs) {
            // Translate the real path from Pcp into a stage-relative path
            pathsToRecompose->insert(path);
            TF_DEBUG(USD_CHANGES).Msg("Did Change Spec: %s\n",
                                      path.GetText());
        }

        for (const auto& path : ourChanges.didChangePrims) {
            // Translate the real path from Pcp into a stage-relative path
            pathsToRecompose->insert(path);
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

    // Prune descendant paths into a vector.
    SdfPathVector pathVecToRecompose;
    _CopyAndRemoveDescendentPaths(*pathsToRecompose, &pathVecToRecompose);

    // Invalidate the clip cache, but keep the clips alive for the duration
    // of recomposition in the (likely) case that clip data hasn't changed
    // and the underlying clip layer can be reused.
    Usd_ClipCache::Lifeboat clipLifeboat;
    for (const auto& path : pathVecToRecompose) {
        _clipCache->InvalidateClipsForPrim(path, &clipLifeboat);
    }

    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash> _MasterToPrimIndexMap;
    _MasterToPrimIndexMap masterToPrimIndexMap;

    // Ask Pcp to compute all the prim indexes in parallel, stopping at
    // stuff that's not active.
    SdfPathVector primPathsToRecompose;
    primPathsToRecompose.reserve(pathVecToRecompose.size());
    for (const SdfPath& path : pathVecToRecompose) {
        if (!path.IsAbsoluteRootOrPrimPath() ||
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
        "Recomposing stage", &instanceChanges);

    // Determine what instance master prims on this stage need to
    // be recomposed due to instance prim index changes.
    SdfPathVector masterPrimsToRecompose;
    for (const SdfPath& path : primPathsToRecompose) {
        for (const SdfPath& masterPath :
                 _instanceCache->GetPrimsInMastersUsingPrimIndexAtPath(path)) {
            masterPrimsToRecompose.push_back(masterPath);
            masterToPrimIndexMap[masterPath] = path;
        }
    }

    for (size_t i = 0; i != instanceChanges.newMasterPrims.size(); ++i) {
        masterPrimsToRecompose.push_back(instanceChanges.newMasterPrims[i]);
        masterToPrimIndexMap[instanceChanges.newMasterPrims[i]] =
            instanceChanges.newMasterPrimIndexes[i];
    }

    for (size_t i = 0; i != instanceChanges.changedMasterPrims.size(); ++i) {
        masterPrimsToRecompose.push_back(instanceChanges.changedMasterPrims[i]);
        masterToPrimIndexMap[instanceChanges.changedMasterPrims[i]] =
            instanceChanges.changedMasterPrimIndexes[i];
    }

    if (!masterPrimsToRecompose.empty()) {
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
        instanceChanges.deadMasterPrims.begin(), 
        instanceChanges.deadMasterPrims.end());
    _DestroyPrimsInParallel(instanceChanges.deadMasterPrims);

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
        for (const auto& prim : subtreesToRecompose) {
            primIndexPathsForSubtrees.push_back(TfMapLookupByValue(
                masterToPrimIndexMap, prim->GetPath(), prim->GetPath()));
        }
        _ComposeSubtreesInParallel(
            subtreesToRecompose, &primIndexPathsForSubtrees);
    }

    if (!pathVecToRecompose.empty())
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
            _instanceCache->GetMasterUsingPrimIndexAtPath(
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
        bool isNewPayload = !prim || !prim.HasPayload();

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
        for (; !prim.HasPayload() && prim != root; prim = prim.GetParent()) {
            // continue
        }

        // If we hit the root, then consult the initial population state.
        // Otherwise load the payload if the ancestor is loaded.
        return prim != root ? prim.IsLoaded() :
            _stage->_initialLoadSet == LoadAll;
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

    std::vector<SdfPath> const *pathsToCompose = &primIndexPaths;

    // If we have a population mask, take the intersection of the requested
    // paths with the stage's population mask, and only compute those.
    static auto allMask = UsdStagePopulationMask::All();
    vector<SdfPath> maskedPaths;
    if (GetPopulationMask() != allMask) {
        maskedPaths = UsdStagePopulationMask(primIndexPaths).
            GetIntersection(GetPopulationMask()).GetPaths();
        pathsToCompose = &maskedPaths;
    }

    // Ask Pcp to compute all the prim indexes in parallel, stopping at
    // prim indexes that won't be used by the stage.
    PcpErrorVector errs;

    if (includeRule == _IncludeAllDiscoveredPayloads) {
        _cache->ComputePrimIndexesInParallel(
            *pathsToCompose, &errs, _NameChildrenPred(_instanceCache.get()),
            [](const SdfPath &) { return true; },
            "Usd", _mallocTagID);
    }
    else if (includeRule == _IncludeNoDiscoveredPayloads) {
        _cache->ComputePrimIndexesInParallel(
            *pathsToCompose, &errs, _NameChildrenPred(_instanceCache.get()),
            [](const SdfPath &) { return false; },
            "Usd", _mallocTagID);
    }
    else if (includeRule == _IncludeNewPayloadsIfAncestorWasIncluded) {
        _cache->ComputePrimIndexesInParallel(
            *pathsToCompose, &errs, _NameChildrenPred(_instanceCache.get()),
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
UsdStage::_GetDefiningSpecType(const UsdPrim& prim,
                               const TfToken& propName) const
{
    if (!TF_VERIFY(prim) || !TF_VERIFY(!propName.IsEmpty()))
        return SdfSpecTypeUnknown;

    // Check for a spec type in the definition registry, in case this is a
    // builtin property.
    SdfSpecType specType =
        UsdSchemaRegistry::GetSpecType(prim.GetTypeName(), propName);

    if (specType != SdfSpecTypeUnknown)
        return specType;

    // Otherwise look for the strongest authored property spec.
    Usd_Resolver res(&prim.GetPrimIndex(), /*skipEmptyNodes=*/true);
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

namespace {
using _MasterToFlattenedPathMap 
    = std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>;

SdfPath
_GenerateTranslatedTargetPath(const SdfPath& inputPath,
                              const _MasterToFlattenedPathMap& masterToFlattened)
{
    if (inputPath == SdfPath::AbsoluteRootPath()) {
        return inputPath;
    }

    // Master prims will always be the root        
    auto prefix = inputPath;
    for ( ; prefix.GetParentPath() != SdfPath::AbsoluteRootPath(); 
            prefix = prefix.GetParentPath()) { 

        // Nothing to do here, just climbing to the parent path
    }

    auto replacement = masterToFlattened.find(prefix);
    if (replacement == end(masterToFlattened)) {
        return inputPath;
    }

    return inputPath.ReplacePrefix(prefix, replacement->second);
}

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
            masterToFlattened.emplace(masterPrimPath, flattenedMasterPath);
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

    for (auto childIt = UsdPrimRange::AllPrims(GetPseudoRoot()); 
         childIt; ++childIt) {
        UsdPrim usdPrim = *childIt;
        _FlattenPrim(usdPrim, flatLayer, usdPrim.GetPath(), masterToFlattened);
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


void
UsdStage::_FlattenPrim(const UsdPrim &usdPrim,
                       const SdfLayerHandle &layer,
                       const SdfPath &path,
                       const _MasterToFlattenedPathMap &masterToFlattened) const
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
    
    _CopyMetadata(usdPrim, newPrim);

    // In the case of flattening clips, we may have builtin attributes which aren't
    // declared in the static scene topology, but may have a value in some
    // clips that we want to relay into the flattened result.
    // XXX: This should be removed if we fix GetProperties()
    // and GetAuthoredProperties to consider clips.
    auto hasValue = [](const UsdProperty& prop){
        return prop.Is<UsdAttribute>()
               && prop.As<UsdAttribute>().HasAuthoredValueOpinion();
    };
    
    for (auto const &prop : usdPrim.GetProperties()) {
        if (prop.IsAuthored() || hasValue(prop)) {
            _CopyProperty(prop, newPrim, masterToFlattened);
        }
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
   
    for (auto primIt = UsdPrimRange::AllPrims(masterPrim); primIt; primIt++){
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
                        const SdfPrimSpecHandle &dest,
                        const _MasterToFlattenedPathMap
                            &masterToFlattened) const
{
    if (prop.Is<UsdAttribute>()) {
        UsdAttribute attr = prop.As<UsdAttribute>();
        
        if (!attr.GetTypeName()){
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
            0.0, &lower, &upper, &hasSamples) && hasSamples) {
            sdfAttr->SetInfo(SdfFieldKeys->TimeSamples,
                             VtValue(_GetTimeSampleMap(attr)));
        }
        if (attr.HasAuthoredMetadata(SdfFieldKeys->Default)) {
            if (!attr.Get(&defaultValue)) {
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
             sdfTargets.Add(_GenerateTranslatedTargetPath(path, 
                                                          masterToFlattened));
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

        if (_done && _IsHolding<VtDictionary>(_value)) {
            // Continue composing if we got a dictionary.
            _done = false;
            if (isDict) {
                // Merge dictionaries: _value is weaker, tmpDict stronger.
                VtDictionaryOverRecursive(
                    &tmpDict, _UncheckedGet<VtDictionary>(_value));
                _Set(_value, tmpDict);
            }
            return true;
        } else if (_done && _IsHolding<SdfTimeSampleMap>(_value)) {
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
    Usd_Resolver resolver(&obj.GetPrim().GetPrimIndex());
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
                          UsdMetadataValueMap* resultMap) const
{
    TRACE_FUNCTION();

    UsdMetadataValueMap &result = *resultMap;

    TfTokenVector fieldNames = _ListMetadataFields(obj, useFallbacks);
    for (const auto& fieldName : fieldNames) {
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
        && primPathInLayerStack.HasPrefix(clip->sourceNode.GetPath()));
}

static bool
_ClipsApplyToNode(
    const Usd_ClipCache::Clips& clips, 
    const PcpNodeRef& node)
{
    return (node.GetLayerStack() == clips.sourceNode.GetLayerStack()
            && node.GetPath().HasPrefix(clips.sourceNode.GetPath()));
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
                if (!localClips.manifestClip->HasField(
                        specId, SdfFieldKeys->Variability, &attrVariability)
                    || attrVariability != SdfVariabilityVarying) {
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
        const double localTime = info._offset * time.GetValue();

        double upper = 0.0;
        double lower = 0.0;

        if (lowerHint && upperHint) {
            lower = *lowerHint;
            upper = *upperHint;
        }
        else {
            if (!TF_VERIFY(layer->GetBracketingTimeSamplesForPath(
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
            return queryResult && (!_ClearValueIfBlocked(result));
        }

        return interpolator->Interpolate(
            attr, layer, specId, localTime, lower, upper);
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

        if (GfIsClose(lower, upper, /* epsilon = */ 1e-6)) {
            bool queryResult = clip->QueryTimeSample(specId, lower, result);
            return queryResult && (!_ClearValueIfBlocked(result));
        }

        return interpolator->Interpolate(
            attr, clip, specId, localTime, lower, upper);
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
        return valueFound && (!_ClearValueIfBlocked(result));
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
    if (!value) {
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
            _resolveInfo->_offset = layerOffset;
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
    const UsdPrim prim = prop.GetPrim();
    boost::optional<double> localTime;
    if (time && !time->IsDefault()) {
        localTime = time->GetValue();
    }

    // Retrieve all clips that may contribute time samples for this
    // attribute at the given time. Clips never contribute default
    // values.
    const std::vector<Usd_ClipCache::Clips>* clipsAffectingPrim = nullptr;
    if (prim._Prim()->MayHaveOpinionsInClips()
        && (!time || !time->IsDefault())) {
        clipsAffectingPrim = &(_clipCache->GetClipsForPrim(prim.GetPath()));
    }

    // Clips may contribute opinions at nodes where no specs for the attribute
    // exist in the node's LayerStack. So, if we have any clips, tell
    // Usd_Resolver that we want to iterate over 'empty' nodes as well.
    const bool skipEmptyNodes = (bool)(!clipsAffectingPrim);

    for (Usd_Resolver res(&prim.GetPrimIndex(), skipEmptyNodes); 
         res.IsValid(); res.NextNode()) {

        const PcpNodeRef& node = res.GetNode();
        const bool nodeHasSpecs = node.HasSpecs();
        if (!nodeHasSpecs && !clipsAffectingPrim) {
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
                if (!clips) {
                    clips = _GetClipsThatApplyToNode(*clipsAffectingPrim,
                                                     node, specId);
                    // If we don't have specs on this node and clips don't
                    // apply we can mode onto the next node.
                    if (!nodeHasSpecs && !clips.get()) { 
                        break; 
                    }
                }
                
                // gcc 4.8 incorrectly detects boost::optional as uninitialized. 
                // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=47679
                ARCH_PRAGMA_PUSH
                ARCH_PRAGMA_MAYBE_UNINITIALIZED

                // We only care about clips that were introduced at this
                // position within the LayerStack.
                if (!clips.get() || clips.get()->sourceLayerIndex != i) {
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
        return valueFound && (!_ClearValueIfBlocked(result));
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
    Usd_UntypedInterpolator interpolator(value);
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
UsdStage::_GetTimeSampleMap(const UsdAttribute &attr,
                            SdfTimeSampleMap *out) const
{
    std::vector<double> timeSamples;
    if (_GetTimeSamplesInInterval(attr, GfInterval::GetFullInterval(), 
                                  &timeSamples)) {
        // Interpolation should not be triggered below, since we are asking
        // for values on times where we know there are authored time samples.
        Usd_NullInterpolator nullInterpolator;

        for (const auto& timeSample : timeSamples) {
            VtValue value;
            if (_GetValueImpl(timeSample, attr, &nullInterpolator, &value)) {
                (*out)[timeSample] = value;
            } else {
                (*out)[timeSample] = VtValue(SdfValueBlock());
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
    UsdResolveInfo info;
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
    const UsdResolveInfo &info,
    const UsdAttribute &attr,
    const GfInterval& interval,
    std::vector<double>* times) const
{
    if ((interval.IsMinFinite() && interval.IsMinOpen())
        || (interval.IsMaxFinite() && interval.IsMaxOpen())) {
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

    if (info._source == UsdResolveInfoSourceTimeSamples) {
        const SdfAbstractDataSpecId specId(
            &info._primPathInLayerStack, &attr.GetName());
        const SdfLayerRefPtr& layer = 
            info._layerStack->GetLayers()[info._layerIndex];

        const std::set<double> samples = layer->ListTimeSamplesForPath(specId);
        if (!samples.empty()) {
            copySamplesInInterval(samples, times, interval);
            const SdfLayerOffset offset = info._offset.GetInverse();
            if (!offset.IsIdentity()) {
                for (auto &time : *times) {
                    time = offset * time;
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

        if (!resolveInfo._offset.IsIdentity()) {
            const SdfLayerOffset offset = resolveInfo._offset.GetInverse();
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
        const double layerTime = info._offset * desiredTime;
        
        if (layer->GetBracketingTimeSamplesForPath(
                specId, layerTime, lower, upper)) {

            if (!info._offset.IsIdentity()) {
                const SdfLayerOffset offset = info._offset.GetInverse();
                *lower = offset * (*lower);
                *upper = offset * (*upper);
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
                
                // 'or' is correct here. Consider the case where we only
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
        SDF_VALUE_TRAITS_TYPE(elem)::Type*) const;                      \
    template bool UsdStage::_GetValue(                                  \
        UsdTimeCode, const UsdAttribute&,                               \
        SDF_VALUE_TRAITS_TYPE(elem)::ShapedType*) const;                \
                                                                        \
    template bool UsdStage::_GetValueFromResolveInfo(                   \
        const UsdResolveInfo&, UsdTimeCode, const UsdAttribute&,        \
        SDF_VALUE_TRAITS_TYPE(elem)::Type*) const;                      \
    template bool UsdStage::_GetValueFromResolveInfo(                   \
        const UsdResolveInfo&, UsdTimeCode, const UsdAttribute&,        \
        SDF_VALUE_TRAITS_TYPE(elem)::ShapedType*) const;                      

BOOST_PP_SEQ_FOR_EACH(_INSTANTIATE_GET, ~, SDF_VALUE_TYPES)
#undef _INSTANTIATE_GET

PXR_NAMESPACE_CLOSE_SCOPE

