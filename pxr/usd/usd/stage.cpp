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
#include "pxr/usd/usd/clipSet.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/interpolators.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/primTypeInfoCache.h"
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
#include "pxr/base/tf/span.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/utils.h"
#include "pxr/base/work/withScopedParallelism.h"

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
template <class LayerPtr>
static SdfLayerOffset
_GetLayerToStageOffset(const PcpNodeRef& pcpNode,
                       const LayerPtr& layer)
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

char const *_dormantMallocTagID = "UsdStages in aggregate";

inline
std::string 
_StageTag(const std::string &id)
{
    return "UsdStage: @" + id + "@";
}

class UsdStage::_PendingChanges
{
public:
    // Set to true to force ObjectsChanged notice to indicate recomposition
    // of the pseudo-root regardless of what was actually recomposed.
    bool notifyPseudoRootResync = false;

    PcpChanges pcpChanges;

    using PathsToChangesMap = UsdNotice::ObjectsChanged::_PathsToChangesMap;
    PathsToChangesMap recomposeChanges, otherResyncChanges, otherInfoChanges;
};

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
UsdStage::_MakeResolvedAssetPathsValue(UsdTimeCode time,
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

void 
UsdStage::_MakeResolvedTimeCodes(UsdTimeCode time, const UsdAttribute &attr,
                                 SdfTimeCode *timeCodes,
                                 size_t numTimeCodes) const
{
    UsdResolveInfo info;
    _GetResolveInfo(attr, &info, &time);
    if (!info._layerToStageOffset.IsIdentity()) {
        for (size_t i = 0; i != numTimeCodes; ++i) {
            Usd_ApplyLayerOffsetToValue(&timeCodes[i], info._layerToStageOffset);
        }
    }
}

void 
UsdStage::_MakeResolvedAttributeValue(
    UsdTimeCode time, const UsdAttribute &attr, VtValue *value) const
{
    if (value->IsHolding<SdfTimeCode>()) {
        SdfTimeCode timeCode;
        value->UncheckedSwap(timeCode);
        _MakeResolvedTimeCodes(time, attr, &timeCode, 1);
        value->UncheckedSwap(timeCode);

    }
    else if (value->IsHolding<VtArray<SdfTimeCode>>()) {
        VtArray<SdfTimeCode> timeCodes;
        value->UncheckedSwap(timeCodes);
        _MakeResolvedTimeCodes(
            time, attr, timeCodes.data(), timeCodes.size());
        value->UncheckedSwap(timeCodes);
    } else {
        _MakeResolvedAssetPathsValue(time, attr, value);
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
    : _pseudoRoot(nullptr)
    , _rootLayer(rootLayer)
    , _sessionLayer(sessionLayer)
    , _editTarget(_rootLayer)
    , _editTargetIsLocalLayer(true)
    , _cache(new PcpCache(PcpLayerStackIdentifier(
                              _rootLayer, _sessionLayer, pathResolverContext),
                          UsdUsdFileFormatTokens->Target,
                          /*usdMode=*/true))
    , _clipCache(new Usd_ClipCache)
    , _instanceCache(new Usd_InstanceCache)
    , _usedLayersRevision(0)
    , _interpolationType(UsdInterpolationTypeLinear)
    , _lastChangeSerialNumber(0)
    , _pendingChanges(nullptr)
    , _initialLoadSet(load)
    , _populationMask(mask)
    , _isClosingStage(false)
    , _isWritingFallbackPrimTypes(false)
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

    WorkWithScopedParallelism([this]() {

            // Destroy prim structure.
            vector<SdfPath> primsToDestroy;
            {
                // Scope the dispatcher so that its dtor Wait()s for work to
                // complete before primsToDestroy is destroyed, since tasks we
                // schedule in the dispatcher access it.
                WorkDispatcher wd;

                // Stop listening for notices.
                wd.Run([this]() {
                        for (auto &p: _layersAndNoticeKeys)
                            TfNotice::Revoke(p.second);
                        TfNotice::Revoke(_resolverChangeKey);
                    });
                
                if (_pseudoRoot) {
                    // Instancing prototypes are not children of the pseudo-root
                    // so we need to explicitly destroy those subtrees.
                    primsToDestroy = _instanceCache->GetAllPrototypes();
                    wd.Run([this, &primsToDestroy]() {
                            primsToDestroy.push_back(
                                SdfPath::AbsoluteRootPath());
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
            }
        });

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
    explicit _NameChildrenPred(const UsdStagePopulationMask *mask,
                               const UsdStageLoadRules *loadRules,
                               Usd_InstanceCache* instanceCache)
        : _mask(mask)
        , _loadRules(loadRules)
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
        // used as a source for a prototype prim.
        if (index.IsInstanceable()) {
            return _instanceCache->RegisterInstancePrimIndex(
                index, _mask, *_loadRules);
        }

        // Compose only the child prims that are included in the population
        // mask, if any.  Masks are included in instancing keys, so this works
        // correctly with instancing.
        return !_mask ||
            _mask->GetIncludedChildNames(index.GetPath(), childNamesToCompose);
    }

private:
    const UsdStagePopulationMask *_mask;
    const UsdStageLoadRules *_loadRules;
    Usd_InstanceCache *_instanceCache;
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
    TfStopwatch stopwatch;
    const bool usdInstantiationTimeDebugCodeActive = 
        TfDebug::IsEnabled(USD_STAGE_INSTANTIATION_TIME);

    if (usdInstantiationTimeDebugCodeActive) {
        stopwatch.Start();
    }

    if (!rootLayer)
        return TfNullPtr;

    UsdStageRefPtr stage = TfCreateRefPtr(
        new UsdStage(rootLayer, sessionLayer, pathResolverContext, mask, load));

    ArResolverScopedCache resolverCache;

    // Set the stage's load rules.
    stage->_loadRules = (load == LoadAll) ?
        UsdStageLoadRules::LoadAll() : UsdStageLoadRules::LoadNone();

    Usd_InstanceChanges instanceChanges;
    const SdfPath& absoluteRootPath = SdfPath::AbsoluteRootPath();

    // Populate the stage, request payloads according to InitialLoadSet load.
    stage->_ComposePrimIndexesInParallel(
            {absoluteRootPath}, "instantiating stage", &instanceChanges);
    stage->_pseudoRoot = stage->_InstantiatePrim(absoluteRootPath);

    const size_t subtreeCount = instanceChanges.newPrototypePrims.size() + 1;
    std::vector<Usd_PrimDataPtr> subtreesToCompose;
    SdfPathVector primIndexPathsForSubtrees;
    subtreesToCompose.reserve(subtreeCount);
    primIndexPathsForSubtrees.reserve(subtreeCount);
    subtreesToCompose.push_back(stage->_pseudoRoot);
    primIndexPathsForSubtrees.push_back(absoluteRootPath);

    // We only need to add new prototypes since, during stage initialization
    // there should not be any changed prototypes
    for (size_t i = 0; i != instanceChanges.newPrototypePrims.size(); ++i) {
        const SdfPath& protoPath = instanceChanges.newPrototypePrims[i];
        const SdfPath& protoPrimIndexPath = 
            instanceChanges.newPrototypePrimIndexes[i];

        Usd_PrimDataPtr protoPrim = stage->_InstantiatePrototypePrim(protoPath);
        subtreesToCompose.push_back(protoPrim);
        primIndexPathsForSubtrees.push_back(protoPrimIndexPath);
    }

    stage->_ComposeSubtreesInParallel(
        subtreesToCompose, &primIndexPathsForSubtrees);

    stage->_RegisterPerLayerNotices();
    stage->_RegisterResolverChangeNotice();

    // Publish this stage into all current writable caches.
    for (const auto cache : UsdStageCacheContext::_GetWritableCaches()) {
        cache->Insert(stage);
    }

    // Debug timing info
    if (usdInstantiationTimeDebugCodeActive) {
        stopwatch.Stop();
        TF_DEBUG(USD_STAGE_INSTANTIATION_TIME)
            .Msg("UsdStage::_InstantiateStage: Time elapsed (s): %f\n",
                 stopwatch.GetSeconds());
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

static inline SdfAttributeSpecHandle
_GetSchemaPropSpec(SdfAttributeSpec *,
                   const UsdPrimDefinition &primDef, 
                   TfToken const &attrName)
{
    return primDef.GetSchemaAttributeSpec(attrName);
}

static inline SdfRelationshipSpecHandle
_GetSchemaPropSpec(SdfRelationshipSpec *,
                   const UsdPrimDefinition &primDef,  
                   TfToken const &attrName)
{
    return primDef.GetSchemaRelationshipSpec(attrName);
}

static inline SdfPropertySpecHandle
_GetSchemaPropSpec(SdfPropertySpec *,
                   const UsdPrimDefinition &primDef, 
                   TfToken const &attrName)
{
    return primDef.GetSchemaPropertySpec(attrName);
}

template <class PropType>
SdfHandle<PropType>
UsdStage::_GetSchemaPropertySpec(const UsdProperty &prop) const
{
    Usd_PrimDataHandle const &primData = prop._Prim();
    if (!primData)
        return TfNullPtr;

    // Consult the registry.
    return _GetSchemaPropSpec(static_cast<PropType *>(nullptr),
                              primData->GetPrimDefinition(), prop.GetName());
}

SdfPropertySpecHandle
UsdStage::_GetSchemaPropertySpec(const UsdProperty &prop) const
{
    return _GetSchemaPropertySpec<SdfPropertySpec>(prop);
}

SdfAttributeSpecHandle
UsdStage::_GetSchemaAttributeSpec(const UsdAttribute &attr) const
{
    return _GetSchemaPropertySpec<SdfAttributeSpec>(attr);
}

SdfRelationshipSpecHandle
UsdStage::_GetSchemaRelationshipSpec(const UsdRelationship &rel) const
{
    return _GetSchemaPropertySpec<SdfRelationshipSpec>(rel);
}

bool
UsdStage::_ValidateEditPrim(const UsdPrim &prim, const char* operation) const
{
    // This function would ideally issue an error if editing the given prim
    // at the stage's edit target would not have any visible effect on the
    // prim. For example, this could happen if the edit target maps the prim's
    // path to a site that is not part of the prim's composition structure.
    //
    // However, doing this requires that we query the prim's dependencies,
    // which is too expensive to do here. So we just allow edits to
    // non-local layers or that are mapped to a different path under the
    // assumption that the user has set up the stage's edit target to author
    // to the site they desire. In the most common case where the edit target
    // just targets a local layer with the identity path mapping, we can use
    // cached bits in the UsdPrim to check for instancing-related errors.
    if (_editTargetIsLocalLayer &&
        (_editTarget.GetMapFunction().IsIdentityPathMapping() ||
         _editTarget.MapToSpecPath(prim.GetPath()) == prim.GetPath())) {
        
        if (ARCH_UNLIKELY(prim.IsInPrototype())) {
            TF_CODING_ERROR(
                "Cannot %s at path <%s>; "
                "authoring to an instancing prototype is not allowed.",
                operation, prim.GetPath().GetText());
            return false;
        }

        if (ARCH_UNLIKELY(prim.IsInstanceProxy())) {
            TF_CODING_ERROR(
                "Cannot %s at path <%s>; "
                "authoring to an instance proxy is not allowed.",
                operation, prim.GetPath().GetText());
            return false;
        }
    }

    return true;
}

bool
UsdStage::_ValidateEditPrimAtPath(const SdfPath &primPath, 
                                  const char* operation) const
{
    // See comments in _ValidateEditPrim
    if (_editTargetIsLocalLayer &&
        (_editTarget.GetMapFunction().IsIdentityPathMapping() ||
         _editTarget.MapToSpecPath(primPath) == primPath)) {

        if (ARCH_UNLIKELY(Usd_InstanceCache::IsPathInPrototype(primPath))) {
            TF_CODING_ERROR(
                "Cannot %s at path <%s>; "
                "authoring to an instancing prototype is not allowed.",
                operation, primPath.GetText());
            return false;
        }

        if (ARCH_UNLIKELY(_IsObjectDescendantOfInstance(primPath))) {
            TF_CODING_ERROR(
                "Cannot %s at path <%s>; "
                "authoring to an instance proxy is not allowed.",
                operation, primPath.GetText());
            return false;
        }
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
                      const TfToken &propName,
                      const SdfAttributeSpecHandle &toCopy)
{
    return SdfAttributeSpec::New(
        primSpec, propName, toCopy->GetTypeName(),
        toCopy->GetVariability(), toCopy->IsCustom());
}

static SdfRelationshipSpecHandle
_StampNewPropertySpec(const SdfPrimSpecHandle &primSpec,
                      const TfToken &propName,
                      const SdfRelationshipSpecHandle &toCopy)
{
    return SdfRelationshipSpec::New(
        primSpec, propName, toCopy->IsCustom(),
        toCopy->GetVariability());
}

static SdfPropertySpecHandle
_StampNewPropertySpec(const SdfPrimSpecHandle &primSpec,
                      const TfToken &propName,
                      const SdfPropertySpecHandle &toCopy)
{
    // Type dispatch to correct property type.
    if (SdfAttributeSpecHandle attrSpec =
        TfDynamic_cast<SdfAttributeSpecHandle>(toCopy)) {
        return _StampNewPropertySpec(primSpec, propName, attrSpec);
    } else {
        return _StampNewPropertySpec(
            primSpec, propName, TfStatic_cast<SdfRelationshipSpecHandle>(toCopy));
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
    specToCopy = _GetSchemaPropertySpec<PropType>(prop);

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
            return _StampNewPropertySpec(primSpec, propName, specToCopy);
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
UsdStage::_SetMetadata(const UsdObject &object,
                       const TfToken &key,
                       const TfToken &keyPath,
                       const VtValue &value)
{
    // The VtValue may be holding a type that needs to be mapped across edit
    // targets.
    if (value.IsHolding<SdfTimeCode>()) {
        return _SetMetadata(object, key, keyPath, 
                            value.UncheckedGet<SdfTimeCode>());
    } else if (value.IsHolding<VtArray<SdfTimeCode>>()) {
        return _SetMetadata(object, key, keyPath, 
                            value.UncheckedGet<VtArray<SdfTimeCode>>());
    } else if (value.IsHolding<VtDictionary>()) {
        return _SetMetadata(object, key, keyPath, 
                            value.UncheckedGet<VtDictionary>());
    } else if (value.IsHolding<SdfTimeSampleMap>()) {
        return _SetMetadata(object, key, keyPath, 
                            value.UncheckedGet<SdfTimeSampleMap>());
    }

    return _SetMetadataImpl(object, key, keyPath, value);
}

// This function handles the inverse mapping of values to an edit target's layer
// for value types that get resolved by layer offsets. It's templated by a set 
// value implementation function in order to abstract out this value mapping for
// both attribute values and metadata. 
// Fn type is equivalent to:
//     bool setValueImpl(const SdfAbstractDataConstValue &)
template <typename T, typename Fn>
static bool
_SetMappedValueForEditTarget(const T &newValue,
                             const UsdEditTarget &editTarget,
                             const Fn &setValueImpl)
{
    const SdfLayerOffset &layerOffset = 
        editTarget.GetMapFunction().GetTimeOffset();
    if (!layerOffset.IsIdentity()) {
        // Copy the value, apply the offset to the edit layer, and set it using
        // the provided set function.
        T targetValue = newValue;
        Usd_ApplyLayerOffsetToValue(&targetValue, layerOffset.GetInverse());

        SdfAbstractDataConstTypedValue<T> in(&targetValue);
        return setValueImpl(in);
    }

    SdfAbstractDataConstTypedValue<T> in(&newValue);
    return setValueImpl(in);
}

template <class T>
bool UsdStage::_SetEditTargetMappedMetadata(
    const UsdObject &obj, const TfToken& fieldName,
    const TfToken &keyPath, const T &newValue)
{
    static_assert(_IsEditTargetMappable<T>::value, 
                  "_SetEditTargetMappedMetadata can only be instantiated for "
                  "types that are edit target mappable.");
    return _SetMappedValueForEditTarget(
        newValue, GetEditTarget(), 
        [this, &obj, &fieldName, &keyPath](const SdfAbstractDataConstValue &in)
        {
            return this->_SetMetadataImpl(obj, fieldName, keyPath, in);
        });
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
    if (!SdfSchema::GetInstance().IsRegistered(fieldName)) {
        TF_CODING_ERROR("Unregistered metadata field: %s", fieldName.GetText());
        return false;
    }

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

template <class T>
bool 
UsdStage::_SetEditTargetMappedValue(
    UsdTimeCode time, const UsdAttribute &attr, const T &newValue)
{
    static_assert(_IsEditTargetMappable<T>::value, 
                  "_SetEditTargetMappedValue can only be instantiated for "
                  "types that are edit target mappable.");
    return _SetMappedValueForEditTarget(newValue, GetEditTarget(),
        [this, &time, &attr](const SdfAbstractDataConstValue &in)
        {
            return this->_SetValueImpl(time, attr, in);
        });
}

// Default _SetValue implementation for most attribute value types that never
// need to be mapped for an edit target.
template <class T>
typename std::enable_if<!UsdStage::_IsEditTargetMappable<T>::value, bool>::type
UsdStage::_SetValue(UsdTimeCode time, const UsdAttribute &attr,
                    const T &newValue)
{
    SdfAbstractDataConstTypedValue<T> in(&newValue);
    return _SetValueImpl<SdfAbstractDataConstValue>(time, attr, in);
}

// Specializations for SdfTimeCode and its array type which may need to be
// value mapped for edit targets. 
// Note that VtDictionary and SdfTimeSampleMap are value types that are time
// mapped when setting metadata, but we don't include them for _SetValue as
// they're not valid attribute value types.
template <class T>
typename std::enable_if<UsdStage::_IsEditTargetMappable<T>::value, bool>::type
UsdStage::_SetValue(UsdTimeCode time, const UsdAttribute &attr,
                    const T &newValue)
{
    return _SetEditTargetMappedValue(time, attr, newValue);
}

bool
UsdStage::_SetValue(
    UsdTimeCode time, const UsdAttribute &attr, const VtValue &newValue)
{
    // May need to map the value if it's holding a time code type.
    if (newValue.IsHolding<SdfTimeCode>()) {
        return _SetValue(time, attr, 
                         newValue.UncheckedGet<SdfTimeCode>());
    } else if (newValue.IsHolding<VtArray<SdfTimeCode>>()) {
        return _SetValue(time, attr, 
                         newValue.UncheckedGet<VtArray<SdfTimeCode>>());
    }
    return _SetValueImpl(time, attr, newValue);
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
    if (!layer->HasSpec(editTarget.MapToSpecPath(attr.GetPath()))) {
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
        editTarget.GetMapFunction().GetTimeOffset().GetInverse();

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
    if (!layer->HasSpec(editTarget.MapToSpecPath(obj.GetPath()))) {
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

    // First look-up the field in the exclude/ignore table.
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
    // prim in the prototype but appears to be a prim at the given path.
    Usd_PrimDataConstPtr primData = _GetPrimDataAtPathOrInPrototype(path);
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

UsdProperty
UsdStage::GetPropertyAtPath(const SdfPath &path) const
{
    return GetObjectAtPath(path).As<UsdProperty>();
}

UsdAttribute
UsdStage::GetAttributeAtPath(const SdfPath &path) const
{
    return GetObjectAtPath(path).As<UsdAttribute>();
}

UsdRelationship
UsdStage::GetRelationshipAtPath(const SdfPath &path) const
{
    return GetObjectAtPath(path).As<UsdRelationship>();
}

Usd_PrimDataConstPtr
UsdStage::_GetPrimDataAtPath(const SdfPath &path) const
{
    tbb::spin_rw_mutex::scoped_lock lock;
    if (_primMapMutex)
        lock.acquire(*_primMapMutex, /*write=*/false);
    PathToNodeMap::const_iterator entry = _primMap.find(path);
    return entry != _primMap.end() ? entry->second.get() : nullptr;
}

Usd_PrimDataPtr
UsdStage::_GetPrimDataAtPath(const SdfPath &path)
{
    tbb::spin_rw_mutex::scoped_lock lock;
    if (_primMapMutex)
        lock.acquire(*_primMapMutex, /*write=*/false);
    PathToNodeMap::const_iterator entry = _primMap.find(path);
    return entry != _primMap.end() ? entry->second.get() : nullptr;
}

Usd_PrimDataConstPtr 
UsdStage::_GetPrimDataAtPathOrInPrototype(const SdfPath &path) const
{
    Usd_PrimDataConstPtr primData = _GetPrimDataAtPath(path);

    // If no prim data exists at the given path, check if this
    // path is pointing to a prim beneath an instance. If so, we
    // need to return the prim data for the corresponding prim
    // in the prototype.
    if (!primData) {
        const SdfPath primInPrototypePath = 
            _instanceCache->GetPathInPrototypeForInstancePath(path);
        if (!primInPrototypePath.IsEmpty()) {
            primData = _GetPrimDataAtPath(primInPrototypePath);
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
    if (_instanceCache->IsPathInPrototype(path)) {
        TF_CODING_ERROR("Attempted to load/unload a prototype path <%s>",
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

    if (curPrim.IsPrototype()) {
        TF_CODING_ERROR("Attempt to load instance prototype <%s>",
                        path.GetString().c_str());
        return false;
    }

    return true;
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
        // Inactive prims are never included in this query.  Prototypes are
        // also never included, since they aren't independently loadable.
        if (!prim.IsActive() || prim.IsPrototype())
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
        if (UsdPrim root = GetPrimAtPath(rootPath)) {
            UsdPrimRange children = UsdPrimRange(
                root, UsdTraverseInstanceProxies(UsdPrimAllPrimsPredicate));
            WorkParallelForEach(
                children.begin(), children.end(), addPrimPayload);
        }
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

    // Optimization: If either or both of the sets is empty then check the other
    // set to see if the load rules already produce the desired state.  If so
    // this is a noop and we can early-out.
    if (loadSet.empty() || unloadSet.empty()) {
        bool isNoOp = true;
        if (unloadSet.empty()) {
            // Check the loadSet to see if we're already in the desired state.
            for (SdfPath const &path: loadSet) {
                if ((policy == UsdLoadWithDescendants &&
                     !_loadRules.IsLoadedWithAllDescendants(path)) ||
                    (policy == UsdLoadWithoutDescendants &&
                     !_loadRules.IsLoadedWithNoDescendants(path))) {
                    isNoOp = false;
                    break;
                }
            }
        }
        else {
            // Check the unloadSet to see if we're already in the desired state.
            for (SdfPath const &path: unloadSet) {
                if (_loadRules.GetEffectiveRuleForPath(path) !=
                    UsdStageLoadRules::NoneRule) {
                    isNoOp = false;
                    break;
                }
            }
        }
        if (isNoOp) {
            // No changes in effective load state for given paths, early-out.
            return;
        }
    }

    SdfPathSet finalLoadSet, finalUnloadSet;

    for (auto const &path : loadSet) {
        if (!_IsValidForLoad(path)) {
            continue;
        }
        finalLoadSet.insert(path);
    }

    for (auto const &path: unloadSet) {
        if (!_IsValidForUnload(path)) {
            continue;
        }
        finalUnloadSet.insert(path);
    }

    _loadRules.LoadAndUnload(finalLoadSet, finalUnloadSet, policy);

    // Go through the finalLoadSet, and check ancestors -- if any are loaded,
    // include the most ancestral which was loaded last in the finalLoadSet.
    for (SdfPath const &p: finalLoadSet) {
        SdfPath curPath = p;
        while (true) {
            SdfPath parentPath = curPath.GetParentPath();
            if (parentPath.IsEmpty())
                break;
            UsdPrim prim = GetPrimAtPath(parentPath);
            if (prim && prim.IsLoaded() && p != curPath) {
                finalLoadSet.insert(curPath);
                break;
            }
            curPath = parentPath;
        }
    }

    // Go through the loadSet and unloadSet, and find the most ancestral
    // instance path for each (or the path itself if no such path exists) and
    // treat them as significant changes.
    SdfPathVector recomposePaths;
    for (SdfPath const &p: finalLoadSet) {
        SdfPath instancePath = _instanceCache->GetMostAncestralInstancePath(p);
        recomposePaths.push_back(instancePath.IsEmpty() ? p : instancePath);
    }    
    for (SdfPath const &p: finalUnloadSet) {
        SdfPath instancePath = _instanceCache->GetMostAncestralInstancePath(p);
        recomposePaths.push_back(instancePath.IsEmpty() ? p : instancePath);
    }

    // This leaves recomposePaths sorted.
    SdfPath::RemoveDescendentPaths(&recomposePaths);

    PcpChanges changes;
    for (SdfPath const &p: recomposePaths) {
        changes.DidChangeSignificantly(_cache.get(), p);
    }

    // Remove any included payloads that are descendant to recomposePaths.
    // We'll re-include everything we need during _Recompose via the inclusion
    // predicate.
    PcpCache::PayloadSet const &currentIncludes = _cache->GetIncludedPayloads();
    SdfPathSet currentIncludesAsSet(currentIncludes.begin(),
                                    currentIncludes.end());
    SdfPathSet payloadsToExclude;
    for (SdfPath const &p: recomposePaths) {
        auto range = SdfPathFindPrefixedRange(currentIncludesAsSet.begin(),
                                              currentIncludesAsSet.end(), p);
        payloadsToExclude.insert(range.first, range.second);
    }
    _cache->RequestPayloads(SdfPathSet(), payloadsToExclude, &changes);

    if (TfDebug::IsEnabled(USD_PAYLOADS)) {
        TF_DEBUG(USD_PAYLOADS).Msg(
            "UsdStage::LoadAndUnload()\n"
            "  finalLoadSet: %s\n"
            "  finalUnloadSet: %s\n"
            "  _loadRules: %s\n"
            "  payloadsToExclude: %s\n"
            "  recomposePaths: %s\n",
            TfStringify(finalLoadSet).c_str(),
            TfStringify(finalUnloadSet).c_str(),
            TfStringify(_loadRules).c_str(),
            TfStringify(payloadsToExclude).c_str(),
            TfStringify(recomposePaths).c_str());
    }

    // Recompose, given the resulting changes from Pcp.
    //
    // PERFORMANCE: Note that Pcp will always include the paths in
    // both sets as "significant changes" regardless of the actual changes
    // resulting from this request, this will trigger recomposition of UsdPrims
    // that potentially didn't change; it seems like we could do better.
    TF_DEBUG(USD_CHANGES).Msg("\nProcessing Load/Unload changes\n");
    _Recompose(changes);

    UsdStageWeakPtr self(this);

    UsdNotice::ObjectsChanged::_PathsToChangesMap resyncChanges, infoChanges;
    for (SdfPath const &p: recomposePaths) {
        resyncChanges[p];
    }

    UsdNotice::ObjectsChanged(self, &resyncChanges, &infoChanges).Send(self);

    UsdNotice::StageContentsChanged(self).Send(self);
}

SdfPathSet
UsdStage::GetLoadSet()
{
    SdfPathSet loadSet;
    for (const auto& primIndexPath : _cache->GetIncludedPayloads()) {
        // Get the path of the Usd prim using this prim index path.
        // This ensures we return the appropriate path if this prim index
        // is being used by a prim within a prototype.
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

    SdfPathSet loadable;
    _DiscoverPayloads(path, UsdLoadWithDescendants, nullptr,
                      /* unloadedOnly = */ false, &loadable);
    return loadable;
}

void
UsdStage::SetLoadRules(UsdStageLoadRules const &rules)
{
    // For now just set the rules and recompose everything.
    _loadRules = rules;
 
    PcpChanges changes;
    changes.DidChangeSignificantly(_cache.get(), SdfPath::AbsoluteRootPath());
    _Recompose(changes);

    // Notify.
    UsdStageWeakPtr self(this);
    UsdNotice::ObjectsChanged::_PathsToChangesMap resyncChanges, infoChanges;
    resyncChanges[SdfPath::AbsoluteRootPath()];
    UsdNotice::ObjectsChanged(self, &resyncChanges, &infoChanges).Send(self);
    UsdNotice::StageContentsChanged(self).Send(self);
}

void
UsdStage::SetPopulationMask(UsdStagePopulationMask const &mask)
{
    // For now just set the mask and recompose everything.
    _populationMask = mask;

    PcpChanges changes;
    changes.DidChangeSignificantly(_cache.get(), SdfPath::AbsoluteRootPath());
    _Recompose(changes);

    // Notify.
    UsdStageWeakPtr self(this);
    UsdNotice::ObjectsChanged::_PathsToChangesMap resyncChanges, infoChanges;
    resyncChanges[SdfPath::AbsoluteRootPath()];
    UsdNotice::ObjectsChanged(self, &resyncChanges, &infoChanges).Send(self);
    UsdNotice::StageContentsChanged(self).Send(self);
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
UsdStage::GetPrototypes() const
{
    // Sort the instance prototype paths to provide a stable ordering for
    // this function.
    SdfPathVector prototypePaths = _instanceCache->GetAllPrototypes();
    std::sort(prototypePaths.begin(), prototypePaths.end());

    vector<UsdPrim> prototypePrims;
    for (const auto& path : prototypePaths) {
        UsdPrim p = GetPrimAtPath(path);
        if (TF_VERIFY(p, "Failed to find prim at prototype path <%s>.\n",
                      path.GetText())) {
            prototypePrims.push_back(p);
        }                   
    }
    return prototypePrims;
}

vector<UsdPrim>
UsdStage::_GetInstancesForPrototype(const UsdPrim& prototypePrim) const
{
    if (!prototypePrim.IsPrototype()) {
        return {};
    }

    vector<UsdPrim> instances;
    SdfPathVector instancePaths = 
        _instanceCache->GetInstancePrimIndexesForPrototype(
            prototypePrim.GetPath());
    instances.reserve(instancePaths.size());
    for (const SdfPath& instancePath : instancePaths) {
        Usd_PrimDataConstPtr primData = 
            _GetPrimDataAtPathOrInPrototype(instancePath);
        instances.push_back(UsdPrim(primData, SdfPath::EmptyPath()));
    }
    return instances;
}

Usd_PrimDataConstPtr 
UsdStage::_GetPrototypeForInstance(Usd_PrimDataConstPtr prim) const
{
    if (!prim->IsInstance()) {
        return nullptr;
    }

    const SdfPath protoPath =
        _instanceCache->GetPrototypeForInstanceablePrimIndexPath(
            prim->GetPrimIndex().GetPath());
    return protoPath.IsEmpty() ? nullptr : _GetPrimDataAtPath(protoPath);
}

bool 
UsdStage::_IsObjectDescendantOfInstance(const SdfPath& path) const
{
    // If the given path is a descendant of an instanceable
    // prim index, it would not be computed during composition unless
    // it is also serving as the source prim index for a prototype prim
    // on this stage.
    //
    // Check if we have any instancing in this stage to avoid unnecessary
    // path operations for performance.
    return (_instanceCache->GetNumPrototypes() > 0 && 
        _instanceCache->IsPathDescendantToAnInstance(
            path.GetAbsoluteRootOrPrimPath()));
}

SdfPath
UsdStage::_GetPrimPathUsingPrimIndexAtPath(const SdfPath& primIndexPath) const
{
    SdfPath primPath;

    // In general, the path of a UsdPrim on a stage is the same as the
    // path of its prim index. However, this is not the case when
    // prims in prototypes are involved. In these cases, we need to use
    // the instance cache to map the prim index path to the prototype
    // prim on the stage.
    if (GetPrimAtPath(primIndexPath)) {
        primPath = primIndexPath;
    } 
    else if (_instanceCache->GetNumPrototypes() != 0) {
        const vector<SdfPath> prototypesUsingPrimIndex = 
            _instanceCache->GetPrimsInPrototypesUsingPrimIndexPath(
                primIndexPath);

        for (const auto& pathInPrototype : prototypesUsingPrimIndex) {
            // If this path is a root prim path, it must be the path of a
            // prototype prim. This function wants to ignore prototype prims,
            // since they appear to have no prim index to the outside
            // consumer.
            //
            // However, if this is not a root prim path, it must be the
            // path of an prim nested inside a prototype, which we do want
            // to return. There will only ever be one of these, so we
            // can get this prim and break immediately.
            if (!pathInPrototype.IsRootPrimPath()) {
                primPath = pathInPrototype;
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

Usd_PrimDataPtr
UsdStage::_InstantiatePrototypePrim(const SdfPath &primPath) 
{
    // Prototype prims are parented beneath the pseudo-root,
    // but are *not* children of the pseudo-root. This ensures
    // that consumers never see prototype prims unless they are
    // explicitly asked for. So, we don't need to set the child
    // link here.
    Usd_PrimDataPtr prototypePrim = _InstantiatePrim(primPath);
    prototypePrim->_SetParentLink(_pseudoRoot);
    return prototypePrim;
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
    // the instance's prototype's subtree if it's root uses this instance's
    // prim index as a source.
    if (prim->IsInstance()) {
        TF_DEBUG(USD_COMPOSITION).Msg("Instance prim <%s>\n",
                                      prim->GetPath().GetText());
        _DestroyDescendents(prim);
        return;
    }

    // Compose child names for this prim.
    TfTokenVector nameOrder;
    if (!TF_VERIFY(prim->_ComposePrimChildNames(&nameOrder)))
        return;

    // Filter nameOrder by the mask, if necessary.  If this subtree is
    // completely included, stop looking at the mask from here forward.
    if (mask) {
        // We always operate on the source prim index path here, not the prim
        // path since that would be something like /__Prototype_X/.. for prims
        // in prototypes.  Masks and load rules operate on the "uninstanced"
        // view of the world, and are included in instancing keys, so whichever
        // index we choose to be the source for a prototype must be included in
        // the stage-wide pop mask & load rules, and identically for all
        // instances that share a prototype.
        const SdfPath& sourceIndexPath = prim->GetSourcePrimIndex().GetPath();
        if (mask->IncludesSubtree(sourceIndexPath)) {
            mask = nullptr;
        } else {
            // Remove all names from nameOrder that aren't included in the mask.
            nameOrder.erase(
                remove_if(nameOrder.begin(), nameOrder.end(),
                          [&sourceIndexPath, mask](TfToken const &nameTok) {
                              return !mask->Includes(
                                  sourceIndexPath.AppendChild(nameTok));
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
    if (parent->IsInPrototype()) {
        // If this UsdPrim is a child of an instance prototype, its 
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

// Static prim type info cache
static Usd_PrimTypeInfoCache &
_GetPrimTypeInfoCache()
{
    static Usd_PrimTypeInfoCache cache;
    return cache;
}

// Iterate over a prim's specs until we get a non-empty, non-any-type typeName.
static TfToken
_ComposeTypeName(const PcpPrimIndex *primIndex)
{
    for (Usd_Resolver res(primIndex); res.IsValid(); res.NextLayer()) {
        TfToken tok;
        if (res.GetLayer()->HasField(
                res.GetLocalPath(), SdfFieldKeys->TypeName, &tok)) {
            if (!tok.IsEmpty() && tok != SdfTokens->AnyTypeToken)
                return tok;
        }
    }
    return TfToken();
}

static void
_ComposeAuthoredAppliedSchemas(
    const PcpPrimIndex *primIndex, TfTokenVector *schemas)
{
    // Collect all list op opinions for the API schemas field from strongest to
    // weakest. Then we apply them from weakest to strongest.
    std::vector<SdfTokenListOp> listOps;

    SdfTokenListOp listOp;
    for (Usd_Resolver res(primIndex); res.IsValid(); res.NextLayer()) {
        if (res.GetLayer()->HasField(
                res.GetLocalPath(), UsdTokens->apiSchemas, &listOp)) {
            // Add the populated list op to the end of the list.
            listOps.emplace_back();
            listOps.back().Swap(listOp);
            // An explicit list op overwrites anything weaker so we can just
            // stop here if it's explicit.
            if (listOps.back().IsExplicit()) {
                break;
            }
        }
    }

    // Apply the listops to our output in reverse order (weakest to strongest).
    std::for_each(listOps.crbegin(), listOps.crend(),
        [&schemas](const SdfTokenListOp& op) { op.ApplyOperations(schemas); });
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

    // Begin a subtree composition in parallel.
    WorkWithScopedParallelism([this, &prims, &primIndexPaths]() {
            _primMapMutex = boost::in_place();
            _dispatcher = boost::in_place();
            // We populate the clip cache concurrently during composition, so we
            // need to enable concurrent population here.
            Usd_ClipCache::ConcurrentPopulationContext
                clipConcurrentPopContext(*_clipCache);
            try {
                for (size_t i = 0; i != prims.size(); ++i) {
                    Usd_PrimDataPtr p = prims[i];
                    _dispatcher->Run([this, p, &primIndexPaths, i]() {
                        _ComposeSubtreeImpl(
                            p, p->GetParent(), &_populationMask,
                            primIndexPaths
                            ? (*primIndexPaths)[i] : p->GetPath());
                    });
                }
            }
            catch (...) {
                _dispatcher = boost::none;
                _primMapMutex = boost::none;
                throw;
            }
            
            _dispatcher = boost::none;
            _primMapMutex = boost::none;
        });
}

void
UsdStage::_ComposeSubtree(
    Usd_PrimDataPtr prim, Usd_PrimDataConstPtr parent,
    UsdStagePopulationMask const *mask,
    const SdfPath& primIndexPath)
{
    if (_dispatcher) {
        _dispatcher->Run([this, prim, parent, mask, primIndexPath]() {
            _ComposeSubtreeImpl(prim, parent, mask, primIndexPath);
        });
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

    // Find the prim's PcpPrimIndex. This should have already been
    // computed in a prior call to _ComposePrimIndexesInParallel.
    // Note that it's unsafe to call PcpCache::ComputePrimIndex here,
    // that method is not thread-safe unless the prim index happens
    // to have been computed already.
    prim->_primIndex = _GetPcpCache()->FindPrimIndex(primIndexPath);
    if (!TF_VERIFY(
            prim->_primIndex, 
            "Prim index at <%s> not found in PcpCache for UsdStage %s", 
            primIndexPath.GetText(), UsdDescribe(this).c_str())) {
        return;
    }

    parent = parent ? parent : prim->GetParent();

    // If this prim's parent is the pseudo-root and it has a different
    // path from its source prim index, it must represent a prototype prim.
    const bool isPrototypePrim =
        (parent == _pseudoRoot 
         && prim->_primIndex->GetPath() != prim->GetPath());

    if (parent && !isPrototypePrim) {
        // Compose the type info full type ID for the prim which includes
        // the type name, applied schemas, and a possible mapped fallback type 
        // if the stage specifies it.
        Usd_PrimTypeInfoCache::TypeId typeId(
            _ComposeTypeName(prim->_primIndex));
        _ComposeAuthoredAppliedSchemas(
            prim->_primIndex, &typeId.appliedAPISchemas);
        if (const TfToken *fallbackType = TfMapLookupPtr(
                _invalidPrimTypeToFallbackMap, typeId.primTypeName)) {
            typeId.mappedTypeName = *fallbackType;
        }

        // Ask the type info cache for the type info for our type.
        prim->_primTypeInfo = 
            _GetPrimTypeInfoCache().FindOrCreatePrimTypeInfo(std::move(typeId));
    } else {
        prim->_primTypeInfo = _GetPrimTypeInfoCache().GetEmptyPrimTypeInfo();
    }

    // Compose type info and flags for prim.
    prim->_ComposeAndCacheFlags(parent, isPrototypePrim);

    // Pre-compute clip information for this prim to avoid doing so
    // at value resolution time.
    if (prim->GetPath() != SdfPath::AbsoluteRootPath()) {
        bool primHasAuthoredClips = _clipCache->PopulateClipsForPrim(
            prim->GetPath(), prim->GetPrimIndex());
        prim->_SetMayHaveOpinionsInClips(
            primHasAuthoredClips || parent->MayHaveOpinionsInClips());
    } else {
        // When composing the pseudoroot we also determine any fallback type
        // mappings that the stage defines for type names that don't have a 
        // valid schema. The possible mappings are defined in the root layer
        // metadata and are needed to compose type info for all the other prims,
        // thus why we do this here.
        _invalidPrimTypeToFallbackMap.clear();
        VtDictionary fallbackPrimTypes;
        if (GetMetadata(UsdTokens->fallbackPrimTypes, &fallbackPrimTypes)) {
            _GetPrimTypeInfoCache().ComputeInvalidPrimTypeToFallbackMap(
                fallbackPrimTypes, &_invalidPrimTypeToFallbackMap);
        }
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
    prim->_firstChild = nullptr;
    while (childIt != childEnd) {
        if (_dispatcher) {
            // Make sure we advance to the next sibling before we destroy
            // the current child so we don't read from a deleted prim.
            Usd_PrimDataPtr child = *childIt++;
            _dispatcher->Run([this, child]() { _DestroyPrim(child); });
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

    WorkWithScopedParallelism([&]() {
            _primMapMutex = boost::in_place();
            _dispatcher = boost::in_place();
            for (const auto& path : paths) {
                Usd_PrimDataPtr prim = _GetPrimDataAtPath(path);
                // We *expect* every prim in paths to be valid as we iterate,
                // but at one time had issues with deactivated prototype prims,
                // so we preserve a guard for resiliency.  See
                // testUsdBug141491.py
                if (TF_VERIFY(prim)) {
                    _dispatcher->Run([this, prim]() {
                            _DestroyPrim(prim);
                        });
                }
            }
            _dispatcher = boost::none;
            _primMapMutex = boost::none;
        });
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

    // This UsdStage may receive layer change notices due to layers being
    // reloaded below. However, we won't receive that notice for any layers
    // that we failed to load previously but are now loadable. For example,
    // if a prim had a reference to a non-existent layer, but then that
    // layer was created, the only indication of that would be a prim resync
    // in the PcpChanges object returned by Reload.
    //
    // We want to combine the stage changes from processing the layer changes
    // with the stage changes indicated in the PcpChanges returned by Reload
    // so that this stage only goes through one round of change processing
    // and notification. So, we create a _PendingChanges object that will
    // be filled in by _HandleLayersDidChange and the call to Reload, then
    // process all of that information in _ProcessPendingChanges().
    _PendingChanges localPendingChanges;
    _pendingChanges = &localPendingChanges;

    ArResolverScopedCache resolverCache;

#if AR_VERSION > 1
    // Refresh the resolver to pick up changes that might have
    // affected asset resolution.
    ArGetResolver().RefreshContext(GetPathResolverContext());
#endif

    // Reload layers in a change block to batch together change notices.
    { 
        SdfChangeBlock block;
    
        // Reload layers that are reached via composition.
        PcpChanges& changes = _pendingChanges->pcpChanges;
        _cache->Reload(&changes);
        
        // Reload all clip layers that are opened.
        _clipCache->Reload();
    }

    // Process changes if they haven't already been processed in response
    // to layer change notices above. If they have already been processed,
    // _pendingChanges would have been reset to NULL.
    if (_pendingChanges == &localPendingChanges) {
        _ProcessPendingChanges();
    }
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

void 
UsdStage::WriteFallbackPrimTypes()
{
    // Mark that we're writing the fallback prim types from the schema registry
    // so that we can ignore changes to the fallbackPrimTypes metadata if we 
    // end up writing it below. Otherwise we could end up rebuilding the entire
    // stage unnecessarily when this particular data shouldn't change any of
    // the prims' composition.
    TfScopedVar<bool> resetIsWriting(_isWritingFallbackPrimTypes, true);

    // Any fallback types for schema prim types will be defined in the schemas
    // themselves. The schema registry provides the fallback prim type 
    // dictionary for us to write in the metadata
    const VtDictionary &schemaFallbackTypes = 
        UsdSchemaRegistry::GetInstance().GetFallbackPrimTypes();
    if (!schemaFallbackTypes.empty()) {
        // The stage may already have metadata for fallback prim types, written
        // from this version of Usd, a different version of Usd, or possibly
        // direct user authoring of the metadata. We don't overwrite any 
        // existing fallbacks; we only add entries for the types that don't have
        // fallbacks defined in the metadata yet.
        VtDictionary existingFallbackTypes;
        if (GetMetadata(UsdTokens->fallbackPrimTypes, &existingFallbackTypes)) {
            VtDictionaryOver(&existingFallbackTypes, schemaFallbackTypes);
            SetMetadata(UsdTokens->fallbackPrimTypes, existingFallbackTypes);
        } else {
            SetMetadata(UsdTokens->fallbackPrimTypes, schemaFallbackTypes);
        }
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
    bool isLocalLayer = true;
    bool* computedIsLocalLayer = nullptr; 

    if (editTarget.GetMapFunction().IsIdentity()) {
        isLocalLayer = HasLocalLayer(editTarget.GetLayer());
        computedIsLocalLayer = &isLocalLayer;

        if (!isLocalLayer) {
            TF_CODING_ERROR(
                "Layer @%s@ is not in the local LayerStack rooted at @%s@",
                editTarget.GetLayer()->GetIdentifier().c_str(),
                GetRootLayer()->GetIdentifier().c_str());
            return;
        }
    }

    // If different from current, set EditTarget and notify.
    if (editTarget != _editTarget) {
        _editTarget = editTarget;
        _editTargetIsLocalLayer = computedIsLocalLayer ? 
            *computedIsLocalLayer : HasLocalLayer(editTarget.GetLayer());
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
    std::vector<std::string> newMutedLayers, newUnMutedLayers;
    _cache->RequestLayerMuting(muteLayers, unmuteLayers, &changes, 
            &newMutedLayers, &newUnMutedLayers);

    UsdStageWeakPtr self(this);

    // Notify for layer muting/unmuting
    if (!newMutedLayers.empty() || !newUnMutedLayers.empty()) {
        UsdNotice::LayerMutingChanged(self, newMutedLayers, newUnMutedLayers)
            .Send(self);
    }

    if (changes.IsEmpty()) {
        return;
    }

    using _PathsToChangesMap = UsdNotice::ObjectsChanged::_PathsToChangesMap;
    _PathsToChangesMap resyncChanges, infoChanges;
    _Recompose(changes, &resyncChanges);

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
_AddAffectedStagePaths(const SdfLayerHandle &layer, const SdfPath &path,
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

    TF_DEBUG(USD_CHANGES).Msg(
        "\nHandleLayersDidChange received (%s)\n", UsdDescribe(this).c_str());

    // If a function up the call stack has set up _PendingChanges, merge in
    // all of the information from layer changes so it can be processed later.
    // Otherwise, fill in our own _PendingChanges and process it at the end
    // of this function.
    _PendingChanges localPendingChanges;
    if (!_pendingChanges) {
        _pendingChanges = &localPendingChanges;
    }

    // Keep track of paths to USD objects that need to be recomposed or
    // have otherwise changed.
    using _PathsToChangesMap = UsdNotice::ObjectsChanged::_PathsToChangesMap;
    _PathsToChangesMap& recomposeChanges = _pendingChanges->recomposeChanges;
    _PathsToChangesMap& otherResyncChanges = _pendingChanges->otherResyncChanges;
    _PathsToChangesMap& otherInfoChanges = _pendingChanges->otherInfoChanges;

    SdfPathVector changedActivePaths;

    // A fallback prim types change occurs when the fallbackPrimTypes metadata
    // changes on the root or session layer. 
    // Note that we never process these changes while writing the schema 
    // defined prim type fallbacks to the stage metadata via 
    // WriteFallbackPrimTypes. Since the function can only write fallbacks for 
    // recognized schema types and does not overwrite existing fallback entries,
    // it creates no effective changes to the composed prims. So, we have to 
    // ignore this layer metadata change to avoid unnecessarily recomposing 
    // the whole stage.
    auto _IsFallbackPrimTypesChange = 
        [this](const SdfLayerHandle &layer, const SdfPath &sdfPath,
               const TfToken &infoKey)
    {
        return infoKey == UsdTokens->fallbackPrimTypes &&
               !this->_isWritingFallbackPrimTypes &&
               sdfPath == SdfPath::AbsoluteRootPath() &&
               (layer == this->GetRootLayer() || 
                layer == this->GetSessionLayer());
    };

    // Add dependent paths for any PrimSpecs whose fields have changed that may
    // affect cached prim information.
    for(const auto& layerAndChangelist : n.GetChangeListVec()) {
        // If this layer does not pertain to us, skip.
        const SdfLayerHandle &layer = layerAndChangelist.first;
        if (_cache->FindAllLayerStacksUsingLayer(layer).empty()) {
            continue;
        }

        // Loop over the changes in this layer, and determine what parts of the
        // usd stage are affected by them.
        for (const auto& entryList : layerAndChangelist.second.GetEntryList()) {

            // This path is the path in the layer that was modified -- in
            // general it's not the same as a path to an object on a usd stage.
            // Instead, it's the path to the changed part of a layer, which may
            // affect zero or more objects on the usd stage, depending on
            // reference structures, active state, etc.  We have to map these
            // paths to those objects on the stage that are affected.
            const SdfPath &sdfPath = entryList.first;
            const SdfChangeList::Entry &entry = entryList.second;

            // Skip target paths entirely -- we do not create target objects in
            // USD.
            if (sdfPath.IsTargetPath())
                continue;

            TF_DEBUG(USD_CHANGES).Msg(
                "<%s> in @%s@ changed.\n",
                sdfPath.GetText(), 
                layer->GetIdentifier().c_str());

            bool willRecompose = false;
            if (sdfPath == SdfPath::AbsoluteRootPath() ||
                sdfPath.IsPrimOrPrimVariantSelectionPath()) {

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
                            infoKey == UsdTokens->apiSchemas ||
                            
                            // XXX: Could be more specific when recomposing due
                            //      to clip changes. E.g., only update the clip
                            //      resolver and bits on each prim.
                            UsdIsClipRelatedField(infoKey) ||
                            // Fallback prim type changes may potentially only 
                            // affect a small number or prims, but this type of
                            // change should be so rare that it's not really
                            // worth parsing the minimal set of prims to 
                            // recompose.
                            _IsFallbackPrimTypesChange(layer, sdfPath, infoKey)) {

                            TF_DEBUG(USD_CHANGES).Msg(
                                "Changed field: %s\n", infoKey.GetText());

                            willRecompose = true;
                            break;
                        }
                    }
                }

                if (willRecompose) {
                    _AddAffectedStagePaths(layer, sdfPath, 
                                           *_cache, &recomposeChanges, &entry);
                }
                if (didChangeActive) {
                    _AddAffectedStagePaths(layer, sdfPath, 
                                           *_cache, &changedActivePaths);
                }
            }
            else {
                willRecompose = sdfPath.IsPropertyPath() &&
                    (entry.flags.didAddPropertyWithOnlyRequiredFields ||
                     entry.flags.didAddProperty ||
                     entry.flags.didRemovePropertyWithOnlyRequiredFields ||
                     entry.flags.didRemoveProperty);

                if (willRecompose) {
                    _AddAffectedStagePaths(
                        layer, sdfPath, *_cache, &otherResyncChanges, &entry);
                }
            }

            // If we're not going to recompose this path, record the dependent
            // scene paths separately so we can notify clients about the
            // changes.
            if (!willRecompose) {
                _AddAffectedStagePaths(layer, sdfPath, 
                                  *_cache, &otherInfoChanges, &entry);
            }
        }
    }

    // Now we have collected the affected paths in UsdStage namespace in
    // recomposeChanges, otherResyncChanges, otherInfoChanges and
    // changedActivePaths.  Push changes through Pcp to determine further
    // invalidation based on composition metadata (reference, inherits, variant
    // selections, etc).

    PcpChanges& changes = _pendingChanges->pcpChanges;
    const PcpCache *cache = _cache.get();
    changes.DidChange(
        TfSpan<const PcpCache*>(&cache, 1), n.GetChangeListVec());

    // Pcp does not consider activation changes to be significant since
    // it doesn't look at activation during composition. However, UsdStage
    // needs to do so, since it elides children of deactivated prims.
    // This ensures that prim indexes for these prims are ejected from
    // the PcpCache.
    for (const SdfPath& p : changedActivePaths) {
        changes.DidChangeSignificantly(_cache.get(), p);
    }

    // Normally we'd call _ProcessPendingChanges only if _pendingChanges
    // pointed to localPendingChanges. If it didn't, it would mean that an
    // upstream caller initialized _pendingChanges and that caller would be
    // expected to call _ProcessPendingChanges itself.
    // 
    // However, the _PathsToChangesMap objects in _pendingChanges may hold
    // raw pointers to entries stored in the notice, so we must process these
    // changes immediately while the notice is still alive.
    _ProcessPendingChanges();
}

void
UsdStage::_ProcessPendingChanges()
{
    if (!TF_VERIFY(_pendingChanges)) {
        return;
    }

    TF_DEBUG(USD_CHANGES).Msg(
        "\nProcessPendingChanges (%s)\n", UsdDescribe(this).c_str());

    PcpChanges& changes = _pendingChanges->pcpChanges;

    using _PathsToChangesMap = UsdNotice::ObjectsChanged::_PathsToChangesMap;
    _PathsToChangesMap& recomposeChanges = _pendingChanges->recomposeChanges;
    _PathsToChangesMap& otherResyncChanges=_pendingChanges->otherResyncChanges;
    _PathsToChangesMap& otherInfoChanges = _pendingChanges->otherInfoChanges;

    _Recompose(changes, &recomposeChanges);

    if (_pendingChanges->notifyPseudoRootResync) {
        recomposeChanges.clear();
        recomposeChanges[SdfPath::AbsoluteRootPath()];

        otherResyncChanges.clear();
        otherInfoChanges.clear();
    }
    else {
        // Filter out all changes to objects beneath instances and remap
        // them to the corresponding object in the instance's prototype. Do this
        // after _Recompose so that the instancing cache is up-to-date.
        auto remapChangesToPrototypes = [this](_PathsToChangesMap* changes) {
            std::vector<_PathsToChangesMap::value_type> prototypeChanges;
            for (auto it = changes->begin(); it != changes->end(); ) {
                if (_IsObjectDescendantOfInstance(it->first)) {
                    const SdfPath primIndexPath = 
                        it->first.GetAbsoluteRootOrPrimPath();
                    for (const SdfPath& pathInPrototype :
                         _instanceCache->GetPrimsInPrototypesUsingPrimIndexPath(
                             primIndexPath)) {
                        prototypeChanges.emplace_back(
                            it->first.ReplacePrefix(
                                primIndexPath, pathInPrototype), 
                            it->second);
                    }
                    it = changes->erase(it);
                    continue;
                }
                ++it;
            }

            for (const auto& entry : prototypeChanges) {
                auto& value = (*changes)[entry.first];
                value.insert(
                    value.end(), entry.second.begin(), entry.second.end());
            }
        };

        remapChangesToPrototypes(&recomposeChanges);
        remapChangesToPrototypes(&otherResyncChanges);
        remapChangesToPrototypes(&otherInfoChanges);

        // Add in all other paths that are marked as resynced.
        if (recomposeChanges.empty()) {
            recomposeChanges.swap(otherResyncChanges);
        }
        else {
            _RemoveDescendentEntries(&recomposeChanges);
            _MergeAndRemoveDescendentEntries(
                &recomposeChanges, &otherResyncChanges);
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
    }

    // If the local layer stack has changed, recompute whether the edit target
    // layer is a local layer. We need to do this after the Pcp changes
    // have been applied so that the local layer stack has been updated.
    if (TfMapLookupPtr(
            _pendingChanges->pcpChanges.GetLayerStackChanges(), 
            _cache->GetLayerStack())) {
        _editTargetIsLocalLayer = HasLocalLayer(_editTarget.GetLayer());
    }

    // Reset _pendingChanges before sending notices so that any changes to
    // this stage that happen in response to the notices are handled
    // properly. The object that _pendingChanges referred to should remain
    // alive, so the references we took above are still valid.
    _pendingChanges = nullptr;

    if (!recomposeChanges.empty() || !otherInfoChanges.empty()) {
        UsdStageWeakPtr self(this);

        // Notify about changed objects.
        UsdNotice::ObjectsChanged(
            self, &recomposeChanges, &otherInfoChanges).Send(self);

        // Receivers can now refresh their caches... or just dirty them
        UsdNotice::StageContentsChanged(self).Send(self);
    }
}

void
UsdStage::_HandleResolverDidChange(
    const ArNotice::ResolverChanged& n)
{
#if AR_VERSION == 1
    return;
#endif

    // A ResolverChanged notice that affects our resolver context means that
    // any asset paths that have been resolved on this stage may now resolve
    // to a different resolved path. This includes asset paths that were
    // resolved during composition and asset path-valued attributes.
    //
    // Handling this notice correctly must be done downstream of Sdf, since
    // asset paths have to be re-resolved under the contexts they were
    // originally resolved with. Sdf does not have the information needed to do
    // this, since it only tracks the context a layer was originally opened
    // with and not any other contexts.
    //
    // For example: let's say we have stage A that opens a layer with asset path
    // L, then we create stage B with a different context that also references
    // L. If L happens to resolve to the same file under B's context, then A and
    // B will share that layer.  However, at the Sdf level that layer only knows
    // about A's context since that's what it was opened under. If we get a
    // ResolverChanged notice that affects stage B, we need to re-resolve L
    // under stage B's context to determine if anything needs to change.
    if (!n.AffectsContext(GetPathResolverContext())) {
        return;
    }

    TF_DEBUG(USD_CHANGES).Msg(
        "\nHandleResolverDidChange received (%s)\n", UsdDescribe(this).c_str());

    // Merge stage changes computed in this function with other pending changes
    // or start up our own pending changes batch so we can process them at the
    // end of the function.
    _PendingChanges localPendingChanges;
    if (!_pendingChanges) {
        _pendingChanges = &localPendingChanges;
    }

    // Inform Pcp of the change to the resolver to determine prims that
    // may need to be resynced. Pcp will re-resolve asset paths for all prim
    // indexes to see if any now refer to a different resolved path and
    // indicate that resyncs are necessary for those prims.
    PcpChanges& changes = _pendingChanges->pcpChanges;
    changes.DidChangeAssetResolver(_GetPcpCache());

    // Asset-path valued attributes on this stage may be invalidated.
    // We don't want to incur the expense of scanning the entire stage
    // to see if any such attributes exist so we conservatively notify
    // clients that the pseudo-root has resynced, even though we may
    // only be recomposing a subset of the stage.
    _pendingChanges->notifyPseudoRootResync = true;

    // Process pending changes if we are the originators of the batch.
    if (_pendingChanges == &localPendingChanges) {
        _ProcessPendingChanges();
    }
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

    // Note: Calling changes.Apply() will result in recomputation of  
    // pcpPrimIndexes for changed prims, these get updated on the respective  
    // prims during _ComposeSubtreeImpl call. Using these outdated primIndexes
    // can result in undefined behavior
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
            TF_DEBUG(USD_CHANGES).Msg("Did Change Prim: %s\n", path.GetText());
        }

    } else {
        TF_DEBUG(USD_CHANGES).Msg("No cache changes\n");
    }

    _RecomposePrims(pathsToRecompose);

    // Update layer change notice listeners if changes may affect
    // the set of used layers. This is potentially expensive which is why we
    // try to make sure the changes require it.
    _RegisterPerLayerNotices();
}

template <class T>
void 
UsdStage::_RecomposePrims(T *pathsToRecompose)
{
    if (pathsToRecompose->empty()) {
        TF_DEBUG(USD_CHANGES).Msg("Nothing to recompose in cache changes\n");
        return;
    }

    // Prune descendant paths.
    _RemoveDescendentEntries(pathsToRecompose);

    // Invalidate the clip cache, but keep the clips alive for the duration
    // of recomposition in the (likely) case that clip data hasn't changed
    // and the underlying clip layer can be reused.
    Usd_ClipCache::Lifeboat clipLifeboat(*_clipCache);
    for (const auto& entry : *pathsToRecompose) {
        _clipCache->InvalidateClipsForPrim(entry.first);
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
        // for a prototype.
        if (_instanceCache->IsPathDescendantToAnInstance(path)) {
            const bool primIndexUsedByPrototype = 
                _instanceCache->PrototypeUsesPrimIndexPath(path);
            if (!primIndexUsedByPrototype) {
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
        primPathsToRecompose, "recomposing stage", &instanceChanges);
    
    // Determine what instance prototype prims on this stage need to
    // be recomposed due to instance prim index changes.
    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash> _PrototypeToPrimIndexMap;
    _PrototypeToPrimIndexMap prototypeToPrimIndexMap;

    const bool pathsContainsAbsRoot = 
        pathsToRecompose->begin()->first == SdfPath::AbsoluteRootPath();

    //If AbsoluteRootPath is present then that should be the only entry!
    TF_VERIFY(!pathsContainsAbsRoot || pathsToRecompose->size() == 1);

    const size_t origNumPathsToRecompose = pathsToRecompose->size();
    for (const auto& entry : *pathsToRecompose) {
        const SdfPath& path = entry.first;
        // Add corresponding inPrototypePaths for any instance or proxy paths in
        // pathsToRecompose
        for (const SdfPath& inPrototypePath :
                 _instanceCache->GetPrimsInPrototypesUsingPrimIndexPath(path)) {
            prototypeToPrimIndexMap[inPrototypePath] = path;
            (*pathsToRecompose)[inPrototypePath];
        }
        // Add any unchanged prototypes whose instances are descendents of paths
        // in pathsToRecompose
        for (const std::pair<SdfPath, SdfPath>& prototypeSourceIndexPair:
                _instanceCache->GetPrototypesUsingPrimIndexPathOrDescendents(
                    path))
        {
            const SdfPath& prototypePath = prototypeSourceIndexPair.first;
            const SdfPath& sourceIndexPath = prototypeSourceIndexPair.second;
            prototypeToPrimIndexMap[prototypePath] = sourceIndexPath;
            (*pathsToRecompose)[prototypePath];
        }
    }

    // Add new prototypes paths to pathsToRecompose 
    for (size_t i = 0; i != instanceChanges.newPrototypePrims.size(); ++i) {
        prototypeToPrimIndexMap[instanceChanges.newPrototypePrims[i]] =
            instanceChanges.newPrototypePrimIndexes[i];
        (*pathsToRecompose)[instanceChanges.newPrototypePrims[i]];
    }

    // Add changed prototypes paths to pathsToRecompose 
    for (size_t i = 0; i != instanceChanges.changedPrototypePrims.size(); ++i) {
        prototypeToPrimIndexMap[instanceChanges.changedPrototypePrims[i]] =
            instanceChanges.changedPrototypePrimIndexes[i];
        (*pathsToRecompose)[instanceChanges.changedPrototypePrims[i]];
    }

    // If pseudoRoot is present in pathsToRecompose, then the only other prims
    // in pathsToRecompose can be prototype prims (added above), in which case
    // we do not want to remove these prototypes. If not we need to make sure
    // any descendents of prototypes are removed if corresponding prototype is
    // present
    if (!pathsContainsAbsRoot && 
            pathsToRecompose->size() != origNumPathsToRecompose) {
        _RemoveDescendentEntries(pathsToRecompose);
    }

    // XXX: If the call chain here ever starts composing prims in parallel,
    // we'll have to add a Usd_ClipCache::ConcurrentPopulationContext object
    // around this.
    std::vector<Usd_PrimDataPtr> subtreesToRecompose;
    _ComputeSubtreesToRecompose(
        make_transform_iterator(pathsToRecompose->begin(), TfGet<0>()),
        make_transform_iterator(pathsToRecompose->end(), TfGet<0>()),
        &subtreesToRecompose);

    // Recompose subtrees.
    if (prototypeToPrimIndexMap.empty()) {
        _ComposeSubtreesInParallel(subtreesToRecompose);
    }
    else {

        SdfPathVector primIndexPathsForSubtrees;
        primIndexPathsForSubtrees.reserve(subtreesToRecompose.size());
        for (const auto& prim : subtreesToRecompose) {
            primIndexPathsForSubtrees.push_back(TfMapLookupByValue(
                prototypeToPrimIndexMap, prim->GetPath(), prim->GetPath()));
        }
        _ComposeSubtreesInParallel(
            subtreesToRecompose, &primIndexPathsForSubtrees);
    }

    // Destroy dead prototype subtrees, making sure to record them in
    // paths to recompose for notifications.
    for (const SdfPath& p : instanceChanges.deadPrototypePrims) {
        (*pathsToRecompose)[p];
    }
    _DestroyPrimsInParallel(instanceChanges.deadPrototypePrims);
}


template <class Iter>
void 
UsdStage::_ComputeSubtreesToRecompose(
    Iter i, Iter end,
    std::vector<Usd_PrimDataPtr>* subtreesToRecompose)
{
    // XXX: If this function ever winds up composing prims in parallel, callers
    // will have to ensure that a Usd_ClipCache::ConcurrentPopulationContext
    // object is alive during the call.
    
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

        // Add prototypes to list of subtrees to recompose and instantiate any 
        // new prototype not present in the primMap from before
        if (_instanceCache->IsPrototypePath(*i)) {
            PathToNodeMap::const_iterator itr = _primMap.find(*i);
            Usd_PrimDataPtr prototypePrim;
            if (itr != _primMap.end()) {
                // should be a changed prototype if already in the primMap
                prototypePrim = itr->second.get();
            } else {
                // newPrototype should be absent from the primMap, instantiate
                // these now to be added to subtreesToRecompose
                prototypePrim = _InstantiatePrototypePrim(*i);
            }
            subtreesToRecompose->push_back(prototypePrim);
            ++i;
            continue;
        }

        // Collect all non-prototype prims (including descendants of prototypes)
        // to be added to subtreesToRecompute
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
            _ComposeChildren(
                parent, parent->IsInPrototype() ? nullptr : &_populationMask,
                /*recurse=*/false);

            // Recompose the subtree for each affected sibling.
            do {
                PathToNodeMap::const_iterator primIt = _primMap.find(*i);
                if (primIt != _primMap.end()) {
                    subtreesToRecompose->push_back(primIt->second.get());
                } else if (_instanceCache->IsPrototypePath(*i)) {
                    // If this path is a prototype path and is not present in
                    // the primMap, then this must be a new prototype added
                    // during this processing, instantiate and add it.
                    Usd_PrimDataPtr protoPrim = _InstantiatePrototypePrim(*i);
                    subtreesToRecompose->push_back(protoPrim);
                }
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

struct UsdStage::_IncludePayloadsPredicate
{
    explicit _IncludePayloadsPredicate(UsdStage const *stage)
        : _stage(stage) {}

    bool operator()(SdfPath const &primIndexPath) const {
        // Apply the stage's load rules to this primIndexPath.  This works
        // correctly with instancing, because load rules are included in
        // instancing keys.
        return _stage->_loadRules.IsLoaded(primIndexPath);
    }

    UsdStage const *_stage;
};

void 
UsdStage::_ComposePrimIndexesInParallel(
    const std::vector<SdfPath>& primIndexPaths,
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

    _cache->ComputePrimIndexesInParallel(
        primIndexPaths, &errs, 
        _NameChildrenPred(mask, &_loadRules, _instanceCache.get()),
        _IncludePayloadsPredicate(this),
        "Usd", _mallocTagID);

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

    // After processing changes, we may discover that some prototype prims
    // need to change their source prim index. This may be because their
    // previous source prim index was destroyed or was no longer an
    // instance. Compose the new source prim indexes.
    if (!changes.changedPrototypePrims.empty()) {
        _ComposePrimIndexesInParallel(
            changes.changedPrototypePrimIndexes, context, instanceChanges);
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

    // Check to see if the set of used layers hasn't changed, and skip all this
    // if so.
    size_t currentUsedLayersRevision = _cache->GetUsedLayersRevision();
    if (_usedLayersRevision &&
        _usedLayersRevision == currentUsedLayersRevision) {
        return;
    }

    SdfLayerHandleSet usedLayers = _cache->GetUsedLayers();
    _usedLayersRevision = currentUsedLayersRevision;

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

void
UsdStage::_RegisterResolverChangeNotice()
{
    _resolverChangeKey = TfNotice::Register(
        TfCreateWeakPtr(this), &UsdStage::_HandleResolverDidChange);
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
    const UsdPrimDefinition &primDef = primData->GetPrimDefinition();
    SdfSpecType specType = primDef.GetSpecType(propName);
    if (specType != SdfSpecTypeUnknown)
        return specType;

    // Otherwise look for the strongest authored property spec.
    Usd_Resolver res(&primData->GetPrimIndex(), /*skipEmptyNodes=*/true);
    SdfPath curPath;
    bool curPathValid = false;
    while (res.IsValid()) {
        const SdfLayerRefPtr& layer = res.GetLayer();
        if (layer->HasSpec(res.GetLocalPath())) {
            if (!curPathValid) {
                curPath = res.GetLocalPath().AppendProperty(propName);
                curPathValid = true;
            }
            specType = layer->GetSpecType(curPath);
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

    static void GetAllMetadataForFlatten(
        const UsdObject &obj, UsdMetadataValueMap* resultMap)
    {
        // Get the resolved metadata with any asset paths anchored.
        obj.GetStage()->_GetAllMetadata(
            obj, /* useFallbacks = */ false, resultMap, 
            /* anchorAssetPathsOnly = */ true);
    }

    static void ResolveValueForFlatten(
        UsdTimeCode time, const UsdAttribute& attr, 
        const SdfLayerOffset &timeOffset, VtValue* value)
    {
        // Asset path values are anchored for flatten operations
        attr.GetStage()->_MakeResolvedAssetPathsValue(
            time, attr, value, /* anchorAssetPathsOnly = */ true);
        // Time based values are adjusted by layer offset when flattened to a
        // layer affected by an offset.
        if (!timeOffset.IsIdentity()) {
            Usd_ApplyLayerOffsetToValue(value, timeOffset);
        }

    }

    static bool MakeTimeSampleMapForFlatten(
        const UsdAttribute &attr, const SdfLayerOffset& offset, 
        SdfTimeSampleMap *out)
    {
        UsdAttributeQuery attrQuery(attr);

        std::vector<double> timeSamples;
        if (attrQuery.GetTimeSamples(&timeSamples)) {
            for (const auto& timeSample : timeSamples) {
                VtValue value;
                if (attrQuery.Get(&value, timeSample)) {
                    Usd_FlattenAccess::ResolveValueForFlatten(
                        timeSample, attr, offset, &value);
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

};

namespace {

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
        auto it = SdfPathFindLongestPrefix(pathRemapping, p);
        if (it != pathRemapping.end()) {
            p = p.ReplacePrefix(it->first, it->second);
        }
    }
}

// Remove any paths to prototype prims or descendants from given target paths
// for srcProp. Issues a warning if any paths were removed.
void
_RemovePrototypeTargetPaths(const UsdProperty& srcProp, 
                         SdfPathVector* targetPaths)
{
    auto removeIt = std::remove_if(
        targetPaths->begin(), targetPaths->end(),
        Usd_InstanceCache::IsPathInPrototype);
    if (removeIt == targetPaths->end()) {
        return;
    }

    TF_WARN(
        "Some %s paths from <%s> could not be flattened because "
        "they targeted objects within an instancing prototype.",
        srcProp.Is<UsdAttribute>() ? 
            "attribute connection" : "relationship target",
        srcProp.GetPath().GetText());

    targetPaths->erase(removeIt, targetPaths->end());
}

// We want to give generated prototypes in the flattened stage
// reserved(using '__' as a prefix), unclashing paths, however,
// we don't want to use the '__Prototype' paths which have special
// meaning to UsdStage. So we create a mapping between our generated
// 'Flattened_Prototype'-style paths and the '__Prototype' paths.
_PathRemapping
_GenerateFlattenedPrototypePath(const std::vector<UsdPrim>& prototypes)
{
    size_t primPrototypeId = 1;

    const auto generatePathName = [&primPrototypeId]() {
        return SdfPath(TfStringPrintf("/Flattened_Prototype_%lu", 
                                      primPrototypeId++));
    };

    _PathRemapping prototypeToFlattened;

    for (auto const& prototypePrim : prototypes) {
        SdfPath flattenedPrototypePath;
        const auto prototypePrimPath = prototypePrim.GetPath();

        auto prototypePathLookup = prototypeToFlattened.find(prototypePrimPath);
        if (prototypePathLookup == prototypeToFlattened.end()) {
            // We want to ensure that we don't clash with user
            // prims in the unlikely even they named it Flatten_xxx
            flattenedPrototypePath = generatePathName();
            const auto stage = prototypePrim.GetStage();
            while (stage->GetPrimAtPath(flattenedPrototypePath)) {
                flattenedPrototypePath = generatePathName();
            }
            prototypeToFlattened.emplace(
                prototypePrimPath, flattenedPrototypePath);
        } else {
            flattenedPrototypePath = prototypePathLookup->second;
        }     
    }

    return prototypeToFlattened;
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
    Usd_FlattenAccess::GetAllMetadataForFlatten(source, &metadata);

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
            if (Usd_FlattenAccess::MakeTimeSampleMapForFlatten(
                    attr, timeOffset, &ts)) {
                sdfAttr->SetInfo(SdfFieldKeys->TimeSamples, VtValue::Take(ts));
            }
        }
        if (attr.HasAuthoredMetadata(SdfFieldKeys->Default)) {
            VtValue defaultValue;
            if (attr.Get(&defaultValue)) {
                Usd_FlattenAccess::ResolveValueForFlatten(
                    UsdTimeCode::Default(), attr, timeOffset, &defaultValue);
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
            _RemovePrototypeTargetPaths(prop, &sources);
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
             _RemovePrototypeTargetPaths(prop, &targets);
             sdfRel->GetTargetPathList().GetExplicitItems() = targets;
         }
     }
}

void
_CopyPrim(const UsdPrim &usdPrim, 
          const SdfLayerHandle &layer, const SdfPath &path,
          const _PathRemapping &prototypeToFlattened)
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
        const auto flattenedPrototypePath = 
            prototypeToFlattened.at(usdPrim.GetPrototype().GetPath());

        // Author an internal reference to our flattened prototype prim
        newPrim->GetReferenceList().Add(SdfReference(std::string(),
                                        flattenedPrototypePath));
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
            _CopyProperty(prop, newPrim, prop.GetName(), prototypeToFlattened,
                          SdfLayerOffset());
        }
    }
}

void
_CopyPrototypePrim(const UsdPrim &prototypePrim,
                   const SdfLayerHandle &destinationLayer,
                   const _PathRemapping &prototypeToFlattened)
{
    const auto& flattenedPrototypePath 
        = prototypeToFlattened.at(prototypePrim.GetPath());

    for (UsdPrim child: UsdPrimRange::AllPrims(prototypePrim)) {
        // We need to update the child path to use the Flatten name.
        const auto flattenedChildPath = child.GetPath().ReplacePrefix(
            prototypePrim.GetPath(), flattenedPrototypePath);

        _CopyPrim(child, destinationLayer, flattenedChildPath, 
                  prototypeToFlattened);
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
    const auto prototypeToFlattened =
        _GenerateFlattenedPrototypePath(GetPrototypes());

    // We author the prototype overs first to produce simpler 
    // assets which have them grouped at the top of the file.
    for (auto const& prototype : GetPrototypes()) {
        _CopyPrototypePrim(prototype, flatLayer, prototypeToFlattened);
    }

    for (UsdPrim prim: UsdPrimRange::AllPrims(GetPseudoRoot())) {
        _CopyPrim(prim, flatLayer, prim.GetPath(), prototypeToFlattened);
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

    UsdProperty dstProp;
    {
        SdfChangeBlock block;

        // Use the edit target from the destination prim's stage, since it may
        // be different from this stage
        SdfPrimSpecHandle primSpec = 
            dstParent.GetStage()->_CreatePrimSpecForEditing(dstParent);
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
        // Use the edit target from the destination prim's stage, since it may 
        // be different from this stage.
        const SdfLayerOffset stageToLayerOffset = 
            dstParent.GetStage()->GetEditTarget().GetMapFunction().
            GetTimeOffset().GetInverse();

        // Copy authored property values and metadata.
        _CopyProperty(srcProp, primSpec, dstName, remapping, stageToLayerOffset);
        SdfPropertySpecHandle dstPropSpec = 
            primSpec->GetProperties().get(dstName);
        if (!dstPropSpec) {
            return UsdProperty();
        }

        dstProp = dstParent.GetProperty(dstName);

        // Copy fallback property values and metadata if needed.
        _CopyFallbacks(
            _GetSchemaPropertySpec(srcProp),
            _GetSchemaPropertySpec(dstProp),
            dstPropSpec, dstPropStack);
    }

    return dstProp;
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
    value->UncheckedSwap(val);
}

namespace {

// Helper for lazily computing and caching the layer to stage offset for the 
// value resolution functions below. This allows to only resolve the layer 
// offset once we've determined that a value is holding a type that can be 
// resolved layer offsets while caching this computation for types that may
// use it multiple times (e.g. SdfTimeCodeMap and VtDictionary)
struct LayerOffsetAccess
{
public:
    LayerOffsetAccess(const PcpNodeRef &node, const SdfLayerHandle &layer) 
        : _node(node), _layer(layer), _hasLayerOffset(false) {}
    
    const SdfLayerOffset & Get() const {
        // Compute once and cache.
        if (!_hasLayerOffset){
            _hasLayerOffset = true;
            _layerOffset = _GetLayerToStageOffset(_node, _layer);
        }
        return _layerOffset;
    }
                                 
private:
    // Private helper meant to be transient so store references to inputs.
    const PcpNodeRef _node;
    const SdfLayerHandle _layer;

    mutable SdfLayerOffset _layerOffset;
    mutable bool _hasLayerOffset;
};
}; // end anonymous namespace

static void
_ResolveAssetPath(SdfAssetPath *v,
                  const ArResolverContext &context,
                  const SdfLayerRefPtr &layer,
                  bool anchorAssetPathsOnly)
{
    _MakeResolvedAssetPathsImpl(
        layer, context, v, 1,  anchorAssetPathsOnly);
}

static void
_ResolveAssetPath(VtArray<SdfAssetPath> *v,
                  const ArResolverContext &context,
                  const SdfLayerRefPtr &layer,
                  bool anchorAssetPathsOnly)
{
    _MakeResolvedAssetPathsImpl(
        layer, context, v->data(), v->size(),  anchorAssetPathsOnly);
}

template <class T, class Storage>
static void
_UncheckedResolveAssetPath(Storage storage,
                           const ArResolverContext &context,
                           const SdfLayerRefPtr &layer,
                           bool anchorAssetPathsOnly)
{
    T v;
    _UncheckedSwap(storage, v);
    _ResolveAssetPath(&v, context, layer, anchorAssetPathsOnly);
    _UncheckedSwap(storage, v);
}

template <class T, class Storage>
static bool 
_TryResolveAssetPath(Storage storage,
                     const ArResolverContext &context,
                     const SdfLayerRefPtr &layer,
                     bool anchorAssetPathsOnly)
{
    if (_IsHolding<T>(storage)) {
        _UncheckedResolveAssetPath<T>(
            storage, context, layer, anchorAssetPathsOnly);
        return true;
    }
    return false;
}

// Tries to resolve the asset path in storage if it's holding an asset path
// type. Returns true if the value is holding an asset path type.
template <class Storage>
static bool 
_TryResolveAssetPaths(Storage storage,
                      const ArResolverContext &context,
                      const SdfLayerRefPtr &layer,
                      bool anchorAssetPathsOnly)
{
    return 
        _TryResolveAssetPath<SdfAssetPath>(
            storage, context, layer, anchorAssetPathsOnly) ||
        _TryResolveAssetPath<VtArray<SdfAssetPath>>(
            storage, context, layer, anchorAssetPathsOnly);
}

template <class T, class Storage>
static void
_UncheckedApplyLayerOffsetToValue(Storage storage, 
                                  const SdfLayerOffset &offset)
{
    if (!offset.IsIdentity()) {
        T v;
        _UncheckedSwap(storage, v);
        Usd_ApplyLayerOffsetToValue(&v, offset);
        _UncheckedSwap(storage, v);
    }
}

// Tries to apply the layer offset to the value in storage if its holding the
// templated class type. Returns true if the value is holding the specified 
// type.
template <class T, class Storage>
static bool
_TryApplyLayerOffsetToValue(Storage storage, 
                            const LayerOffsetAccess &offsetAccess)
{
    if (_IsHolding<T>(storage)) {
        const SdfLayerOffset &offset = offsetAccess.Get();
        _UncheckedApplyLayerOffsetToValue<T>(storage, offset);
        return true;
    }
    return false;
}

// Tries to resolve the time code(s) in storage with the layer offset if it's 
// holding an time code type. Returns true if the value is holding a time code 
// type.
template <class Storage>
static bool 
_TryResolveTimeCodes(Storage storage, const LayerOffsetAccess &offsetAccess)
{
    return 
        _TryApplyLayerOffsetToValue<SdfTimeCode>(storage, offsetAccess) ||
        _TryApplyLayerOffsetToValue<VtArray<SdfTimeCode>>(storage, offsetAccess);
}

// If the given dictionary contains any resolvable values, fills in those values
// with their resolved paths.
static void
_ResolveValuesInDictionary(const SdfLayerRefPtr &anchor,
                           const ArResolverContext &context,
                           const LayerOffsetAccess *offsetAccess,
                           VtDictionary *dict,
                           bool anchorAssetPathsOnly)
{
    // If there is no layer offset, don't bother with resolving time codes and
    // just resolve asset paths.
    if (offsetAccess) {
        Usd_ResolveValuesInDictionary(dict, 
            [&anchor, &context, &offsetAccess, &anchorAssetPathsOnly]
                (VtValue *value) 
            {
                _TryResolveAssetPaths(
                    value, context, anchor, anchorAssetPathsOnly) ||
                _TryResolveTimeCodes(value, *offsetAccess);
            });
    } else {
        Usd_ResolveValuesInDictionary(dict, 
            [&anchor, &context, &anchorAssetPathsOnly](VtValue *value) 
            {
                _TryResolveAssetPaths(
                    value, context, anchor, anchorAssetPathsOnly);
            });
    }
}

// Tries to resolve all the resolvable values contained within a VtDictionary in
// storage. Returns true if the value is holding a VtDictionary.
template <class Storage>
static bool
_TryResolveValuesInDictionary(Storage storage,
                              const SdfLayerRefPtr &anchor,
                              const ArResolverContext &context,
                              const LayerOffsetAccess *offsetAccess,
                              bool anchorAssetPathsOnly)
{
    if (_IsHolding<VtDictionary>(storage)) {
        VtDictionary resolvedDict;
        _UncheckedSwap(storage, resolvedDict);
        _ResolveValuesInDictionary(
            anchor, context, offsetAccess, &resolvedDict, anchorAssetPathsOnly);
        _UncheckedSwap(storage, resolvedDict);
        return true;
    }
    return false;
}


namespace {

// Non-virtual value composer base class. Helps provide shared functionality 
// amongst the different derived value composer classed. The derived classes
// must all implement a ConsumeAuthored and ConsumeUsdFallback function.
template <class Storage>
struct ValueComposerBase
{
    static const bool ProducesValue = true;

    const std::type_info& GetHeldTypeid() const { return _GetTypeid(_value); }
    bool IsDone() const { return _done; }

    template <class ValueType>
    void ConsumeExplicitValue(ValueType type) 
    {
        Usd_SetValue(_value, type);
        _done = true;
    }

protected:
    // Protected constructor.
    explicit ValueComposerBase(Storage s, bool anchorAssetPathsOnly = false)
        : _value(s), _done(false), _anchorAssetPathsOnly(anchorAssetPathsOnly) 
        {}

    // Gets the value from the layer spec.
    bool _GetValue(const SdfLayerRefPtr &layer,
                   const SdfPath &specPath,
                   const TfToken &fieldName,
                   const TfToken &keyPath)
    {
        return keyPath.IsEmpty() ?
            layer->HasField(specPath, fieldName, _value) :
            layer->HasFieldDictKey(specPath, fieldName, keyPath, _value);
    }

    // Gets the fallback value for the property
    bool _GetFallbackValue(const UsdPrimDefinition &primDef,
                           const TfToken &propName,
                           const TfToken &fieldName,
                           const TfToken &keyPath)
    {
        // Try to read fallback value.
        return Usd_GetFallbackValue(
            primDef, propName, fieldName, keyPath, _value);
    }

    // Consumes an authored dictionary value and merges it into the current 
    // strongest dictionary value.
    bool _ConsumeAndMergeAuthoredDictionary(const PcpNodeRef &node,
                                            const SdfLayerRefPtr &layer,
                                            const SdfPath &specPath,
                                            const TfToken &fieldName,
                                            const TfToken &keyPath) 
    {
        // Copy to the side since we'll have to merge if the next opinion
        // is also a dictionary.
        VtDictionary tmpDict = _UncheckedGet<VtDictionary>(_value);

        // Try to read value from scene description.
        if (_GetValue(layer, specPath, fieldName, keyPath)) {
            const ArResolverContext &context = 
                node.GetLayerStack()->GetIdentifier().pathResolverContext;
            // Create a layer offset accessor so we don't compute the layer
            // offset unless one of the resolve functions actually needs it.
            LayerOffsetAccess layerOffsetAccess(node, layer);

            // Try resolving the values in the dictionary.
            if (_TryResolveValuesInDictionary(
                    _value, layer, context, &layerOffsetAccess, 
                    _anchorAssetPathsOnly)) {
                // Merge the resolved dictionary.
                VtDictionaryOverRecursive(
                    &tmpDict, _UncheckedGet<VtDictionary>(_value));
                _UncheckedSwap(_value, tmpDict);
            } 
            return true;
        }
        return false;
    }

    // Consumes the fallback dictionary value and merges it into the current
    // dictionary value.
    void _ConsumeAndMergeFallbackDictionary(
        const UsdPrimDefinition &primDef,
        const TfToken &propName,
        const TfToken &fieldName,
        const TfToken &keyPath) 
    {
        // Copy to the side since we'll have to merge if the next opinion is
        // also a dictionary.
        VtDictionary tmpDict = _UncheckedGet<VtDictionary>(_value);

        // Try to read fallback value.
        if(_GetFallbackValue(primDef, propName, fieldName, keyPath)) {
            // Always done after reading the fallback value.
            _done = true;
            if (_IsHolding<VtDictionary>(_value)) {
                // Merge dictionaries: _value is weaker, tmpDict stronger.
                VtDictionaryOverRecursive(&tmpDict, 
                                          _UncheckedGet<VtDictionary>(_value));
                _UncheckedSwap(_value, tmpDict);
            }
        }
    }

    Storage _value;
    bool _done;
    bool _anchorAssetPathsOnly;
};

// Value composer for a type erased VtValue. This will check the type
// of the stored value and do the appropriate value composition for the type.
struct UntypedValueComposer : public ValueComposerBase<VtValue *>
{
    using Base = ValueComposerBase<VtValue *>;

    explicit UntypedValueComposer(
        VtValue *s, bool anchorAssetPathsOnly = false)
        : Base(s, anchorAssetPathsOnly) {}

    bool ConsumeAuthored(const PcpNodeRef &node,
                         const SdfLayerRefPtr &layer,
                         const SdfPath &specPath,
                         const TfToken &fieldName,
                         const TfToken &keyPath) 
    {
        if (_IsHoldingDictionary()) {
            // Handle special value-type composition: dictionaries merge atop 
            // each other.
            return this->_ConsumeAndMergeAuthoredDictionary(
                node, layer, specPath, fieldName, keyPath);
        } else {
            // Try to read value from scene description and resolve it if needed
            // if the value is found.
            if (this->_GetValue(layer, specPath, fieldName, keyPath)) {
                // We're done if we got value and it's not a dictionary. For 
                // dictionaries we'll continue to merge in weaker dictionaries.
                if (!_IsHoldingDictionary()) {
                    this->_done = true;
                }
                _ResolveValue(node, layer);
                return true;
            }
            return false;
        }
    }

    void ConsumeUsdFallback(const UsdPrimDefinition &primDef,
                            const TfToken &propName,
                            const TfToken &fieldName,
                            const TfToken &keyPath) 
    {
        if (_IsHoldingDictionary()) {
            // Handle special value-type composition: fallback dictionaries 
            // are merged into the current dictionary value..
            this->_ConsumeAndMergeFallbackDictionary(
                primDef, propName, fieldName, keyPath);
        } else {
            // Try to read fallback value. Fallbacks are not resolved.
            this->_done = this->_GetFallbackValue(
                primDef, propName, fieldName, keyPath);
        }
    }

protected:
    bool _IsHoldingDictionary() const {
        return _IsHolding<VtDictionary>(this->_value);
    }

    void _ResolveValue(const PcpNodeRef &node, const SdfLayerRefPtr &layer)
    {
        const ArResolverContext &context = 
            node.GetLayerStack()->GetIdentifier().pathResolverContext;
        // Create a layer offset accessor so we don't compute the layer
        // offset unless one of the resolve functions actually needs it.
        LayerOffsetAccess layerOffsetAccess(node, layer);

        // Since we don't know the type, we have to try to resolve the 
        // consumed value for all the types that require additional 
        // value resolution.        

        // Try resolving the value as a dictionary first. Note that even though 
        // we have a special case in ConsumeAuthored for when the value is 
        // holding a dictionary, we still have to check for dictionary values
        // here to the cover the case when the storage container starts as an
        // empty VtValue.
        if (_TryResolveValuesInDictionary(
                this->_value, layer, context, &layerOffsetAccess, 
                this->_anchorAssetPathsOnly)) {
        } else {
            // Otherwise try resolving each of the the other resolvable 
            // types.
            _TryApplyLayerOffsetToValue<SdfTimeSampleMap>(
                this->_value, layerOffsetAccess) ||
            _TryResolveAssetPaths(
                this->_value, context, layer, this->_anchorAssetPathsOnly) ||
            _TryResolveTimeCodes(this->_value, layerOffsetAccess);
        }
    }
};

// Strongest value composer for a SdfAbstractDataValue holding a type we know 
// does not need type specific value resolution. The data value is type erased 
// since this composer only needs to get the strongest value. For value types 
// with, type specific value resolution, the TypeSpecficValueComposer must be 
// used instead to get a correctly resolved value.
struct StrongestValueComposer : public ValueComposerBase<SdfAbstractDataValue *>
{
    using Base = ValueComposerBase<SdfAbstractDataValue *>;

    explicit StrongestValueComposer(SdfAbstractDataValue *s)
        : Base(s, /* anchorAssetPathsOnly = */ false) {}


    bool ConsumeAuthored(const PcpNodeRef &node,
                         const SdfLayerRefPtr &layer,
                         const SdfPath &specPath,
                         const TfToken &fieldName,
                         const TfToken &keyPath) 
    {
        // Try to read value from scene description, we're done if the value 
        // is found.
        if (this->_GetValue(layer, specPath, fieldName, keyPath)) {
            this->_done = true;
            return true;
        }
        return false;
    }

    void ConsumeUsdFallback(const UsdPrimDefinition &primDef,
                            const TfToken &propName,
                            const TfToken &fieldName,
                            const TfToken &keyPath) 
    {
        this->_done = this->_GetFallbackValue(
            primDef, propName, fieldName, keyPath);
    }
};

// Value composer for a storage container whose type requires type specific
// value resolution. If this composer is used for a type that does not have 
// type specific value resolution then this is equivalent to using a 
// StrongestValueComposer. For types that have type specific value resolution,
// this is specialized to perform the appropriate resolution.
template <class T>
struct TypeSpecificValueComposer : 
    public ValueComposerBase<SdfAbstractDataValue *>
{
    using Base = ValueComposerBase<SdfAbstractDataValue *>;
    friend Base;

    explicit TypeSpecificValueComposer(SdfAbstractDataTypedValue<T> *s)
        : Base(s, /*anchorAssetPathsOnly = */ false) {}

    bool ConsumeAuthored(const PcpNodeRef &node,
                         const SdfLayerRefPtr &layer,
                         const SdfPath &specPath,
                         const TfToken &fieldName,
                         const TfToken &keyPath) 
    {
        // Try to read value from scene description and resolve it if needed
        // if the value is found.
        if (this->_GetValue(layer, specPath, fieldName, keyPath)) {
            // We're done if we got value and it's not a dictionary. For 
            // dictionaries we'll continue to merge in weaker dictionaries.
            this->_done = true;
            _ResolveValue(node, layer);
            return true;
        }
        return false;
    }

    void ConsumeUsdFallback(const UsdPrimDefinition &primDef,
                            const TfToken &propName,
                            const TfToken &fieldName,
                            const TfToken &keyPath) 
    {
        this->_done = this->_GetFallbackValue(
            primDef, propName, fieldName, keyPath);
    }

protected:
    // Implementation for the base class.
    void _ResolveValue(const PcpNodeRef &node,
                       const SdfLayerRefPtr &layer)
    {
        // The default for almost all types is to do no extra value resolution.
        // The few types that require resolution must either specialize this 
        // method or reimplement ConsumeAuthored and delete this method.
        static_assert(!UsdStage::_HasTypeSpecificResolution<T>::value,
                      "Value types that have type specific value resolution "
                      "must either specialize _ResolveValue or delete it and "
                      "reimplement ConsumeAuthored");
    }
};

// Specializations of _ResolveValue for type specific value resolution types. 
// Note that we can assume that _value always holds the template value type so 
// there is no value type checking. 
// We may, however, want to skip these resolves when _value.isValueBlock is true
template <>
void 
TypeSpecificValueComposer<SdfAssetPath>::_ResolveValue(
    const PcpNodeRef &node,
    const SdfLayerRefPtr &layer)
{
    const ArResolverContext &context = 
        node.GetLayerStack()->GetIdentifier().pathResolverContext;
    _UncheckedResolveAssetPath<SdfAssetPath>(
        _value, context, layer, /*anchorAssetPathsOnly = */ false);
}

template <>
void 
TypeSpecificValueComposer<VtArray<SdfAssetPath>>::_ResolveValue(
    const PcpNodeRef &node,
    const SdfLayerRefPtr &layer)
{
    const ArResolverContext &context = 
        node.GetLayerStack()->GetIdentifier().pathResolverContext;
    _UncheckedResolveAssetPath<VtArray<SdfAssetPath>>(
        _value, context, layer, /*anchorAssetPathsOnly = */ false);
}

template <>
void 
TypeSpecificValueComposer<SdfTimeCode>::_ResolveValue(
    const PcpNodeRef &node,
    const SdfLayerRefPtr &layer)
{
    SdfLayerOffset offset = _GetLayerToStageOffset(node, layer);
    _UncheckedApplyLayerOffsetToValue<SdfTimeCode>(_value, offset);
}

template <>
void 
TypeSpecificValueComposer<VtArray<SdfTimeCode>>::_ResolveValue(
    const PcpNodeRef &node,
    const SdfLayerRefPtr &layer)
{
    SdfLayerOffset offset = _GetLayerToStageOffset(node, layer);
    _UncheckedApplyLayerOffsetToValue<VtArray<SdfTimeCode>>(_value, offset);
}

template <>
void 
TypeSpecificValueComposer<SdfTimeSampleMap>::_ResolveValue(
    const PcpNodeRef &node,
    const SdfLayerRefPtr &layer)
{
    SdfLayerOffset offset = _GetLayerToStageOffset(node, layer);
    _UncheckedApplyLayerOffsetToValue<SdfTimeSampleMap>(_value, offset);
}

// The TypeSpecificValueComposer for VtDictionary has additional specialization
// for consuming values as it merges in weaker values unlike most types that
// only consume the strongest value.
template <>
bool 
TypeSpecificValueComposer<VtDictionary>::ConsumeAuthored(
    const PcpNodeRef &node,
    const SdfLayerRefPtr &layer,
    const SdfPath &specPath,
    const TfToken &fieldName,
    const TfToken &keyPath) 
{
    // Handle special value-type composition: dictionaries merge atop 
    // each other.
    return this->_ConsumeAndMergeAuthoredDictionary(
        node, layer, specPath, fieldName, keyPath);
}

template <>
void 
TypeSpecificValueComposer<VtDictionary>::ConsumeUsdFallback(
    const UsdPrimDefinition &primDef,
    const TfToken &propName,
    const TfToken &fieldName,
    const TfToken &keyPath) 
{
    // Handle special value-type composition: fallback dictionaries 
    // are merged into the current dictionary value..
    _ConsumeAndMergeFallbackDictionary(
        primDef, propName, fieldName, keyPath);
}

template <>
void 
TypeSpecificValueComposer<VtDictionary>::_ResolveValue(
    const PcpNodeRef &, const SdfLayerRefPtr &) = delete;


struct ExistenceComposer
{
    static const bool ProducesValue = false;

    ExistenceComposer() : _done(false), _strongestLayer(nullptr) {}
    explicit ExistenceComposer(SdfLayerRefPtr *strongestLayer) 
        : _done(false), _strongestLayer(strongestLayer) {}

    const std::type_info& GetHeldTypeid() const { return typeid(void); }
    bool IsDone() const { return _done; }
    bool ConsumeAuthored(const PcpNodeRef &node,
                         const SdfLayerRefPtr &layer,
                         const SdfPath &specPath,
                         const TfToken &fieldName,
                         const TfToken &keyPath,
                         const SdfLayerOffset & = SdfLayerOffset()) {
        _done = keyPath.IsEmpty() ?
            layer->HasField(specPath, fieldName,
                            static_cast<VtValue *>(nullptr)) :
            layer->HasFieldDictKey(specPath, fieldName, keyPath,
                                   static_cast<VtValue*>(nullptr));
        if (_done && _strongestLayer)
            *_strongestLayer = layer;
        return _done;
    }
    void ConsumeUsdFallback(const UsdPrimDefinition &primDef,
                            const TfToken &propName,
                            const TfToken &fieldName,
                            const TfToken &keyPath) {
        _done = Usd_GetFallbackValue(primDef, propName, fieldName, keyPath, 
                                     static_cast<VtValue *>(nullptr));
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
        TypeSpecificValueComposer<TfToken> composer(&abstrToken);
        _GetMetadataImpl(attr, SdfFieldKeys->TypeName, TfToken(), 
                         /*useFallbacks=*/true, &composer);

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
            GetEditTarget().GetMapFunction().GetTimeOffset().GetInverse();

        double localTime = stageToLayerOffset * time.GetValue();

        attrSpec->GetLayer()->SetTimeSample(
            attrSpec->GetPath(),
            localTime,
            newValue);
    }

    return true;
}

// --------------------------------------------------------------------- //
// Helpers for Metadata Resolution
// --------------------------------------------------------------------- //

template <class Composer>
static bool
_GetFallbackMetadataImpl(Usd_PrimDataConstPtr primData,
                         const TfToken& propName,
                         const TfToken &fieldName,
                         const TfToken &keyPath,
                         Composer *composer)
{
    // Look for a fallback value in the definition.
    // NOTE: This code is performance critical.
    composer->ConsumeUsdFallback(
        primData->GetPrimDefinition(), propName, fieldName, keyPath);
    return composer->IsDone();
}

template <class Composer>
static bool
_ComposeGeneralMetadataImpl(Usd_PrimDataConstPtr primData,
                            const TfToken& propName,
                            const TfToken& fieldName,
                            const TfToken& keyPath,
                            bool useFallbacks,
                            Usd_Resolver* res,
                            Composer *composer)
{
    // Main resolution loop.
    SdfPath specPath = res->GetLocalPath(propName);
    bool gotOpinion = false;

    for (bool isNewNode = false; res->IsValid(); isNewNode = res->NextLayer()) {
        if (isNewNode) {
            specPath = res->GetLocalPath(propName);
        }

        // Consume an authored opinion here, if one exists.
        gotOpinion |= composer->ConsumeAuthored(
            res->GetNode(), res->GetLayer(), specPath, fieldName, keyPath);
        
        if (composer->IsDone()) {
            return true;
        }
    }

    if (useFallbacks) {
        _GetFallbackMetadataImpl(
            primData, propName, fieldName, keyPath, composer);
    }

    return gotOpinion || composer->IsDone();
}

// Special composing for just the pseudoroot. The pseudoroot only composes
// metadata opinions on the absolute root path from the session and root layers.
// Note that the pseudoroot itself doesn't provide fallbacks.
// Returns true if an opinion was found.
template <class Composer>
static bool
_ComposePseudoRootMetadataImpl(Usd_PrimDataConstPtr primData,
                               const TfToken& fieldName,
                               const TfToken& keyPath,
                               const SdfLayerRefPtr &rootLayer,
                               const SdfLayerRefPtr &sessionLayer,
                               Composer *composer)
{
    const SdfPath &specPath = SdfPath::AbsoluteRootPath();
    bool gotOpinion = false;

    PcpNodeRef node = primData->GetPrimIndex().GetRootNode();

    // If we a have a session layer and it isn't muted, we try to consume its
    // opinion first. The session layer will be the first layer in the 
    // layer stack unless it is muted.
    if (sessionLayer && 
        node.GetLayerStack()->GetLayers().front() == sessionLayer) {
        // Consume an authored opinion here, if one exists.
        gotOpinion = composer->ConsumeAuthored(
            node, sessionLayer, specPath, fieldName, keyPath);
        if (composer->IsDone()) {
            return true;
        }
    }

    // Consume an authored opinion from the root layer (which cannot be muted).
    gotOpinion |= composer->ConsumeAuthored(
        node, rootLayer, specPath, fieldName, keyPath);

    // Return whether we got an opinion from either layer.
    return gotOpinion;
}

// --------------------------------------------------------------------- //
// Specialized Metadata Resolution
// --------------------------------------------------------------------- //

template <class Composer>
static bool
_GetPrimSpecifierImpl(Usd_PrimDataConstPtr primData,
                      bool useFallbacks, Composer *composer);

SdfSpecifier
UsdStage::_GetSpecifier(Usd_PrimDataConstPtr primData)
{
    SdfSpecifier result = SdfSpecifierOver;
    SdfAbstractDataTypedValue<SdfSpecifier> resultVal(&result);
    TypeSpecificValueComposer<SdfSpecifier> composer(&resultVal);
    _GetPrimSpecifierImpl(primData, /* useFallbacks = */ true, &composer);
    return result;
}

template <class Composer>
static bool 
_GetPrimKindImpl(Usd_PrimDataConstPtr primData,
                 bool useFallbacks, Composer *composer)
{
    Usd_Resolver resolver(&primData->GetPrimIndex());
    return _ComposeGeneralMetadataImpl(
        primData, TfToken(), SdfFieldKeys->Kind, TfToken(), useFallbacks, 
        &resolver, composer);
}

TfToken
UsdStage::_GetKind(Usd_PrimDataConstPtr primData)
{
    TfToken kind;
    SdfAbstractDataTypedValue<TfToken> resultValue(&kind);
    TypeSpecificValueComposer<TfToken> composer(&resultValue);

    // We don't allow fallbacks for kind.
    _GetPrimKindImpl(primData, /* useFallbacks = */ false, &composer);
    return kind;
}

template <class Composer>
static bool 
_GetPrimActiveImpl(Usd_PrimDataConstPtr primData,
                   bool useFallbacks, Composer *composer)
{
    Usd_Resolver resolver(&primData->GetPrimIndex());
    return _ComposeGeneralMetadataImpl(
        primData, TfToken(), SdfFieldKeys->Active, TfToken(), useFallbacks, 
        &resolver, composer);
}

bool
UsdStage::_IsActive(Usd_PrimDataConstPtr primData)
{
    bool active = true;
    SdfAbstractDataTypedValue<bool> resultValue(&active);
    TypeSpecificValueComposer<bool> composer(&resultValue);

    // We don't allow fallbacks for active.
    _GetPrimActiveImpl(primData, /* useFallbacks = */ false, &composer);
    return active;
}

bool
UsdStage::_IsCustom(const UsdProperty &prop) const
{
    // Custom is composed as true if there is no property definition and it is
    // true anywhere in the stack of opinions.

    if (_GetSchemaPropertySpec(prop))
        return false;

    const TfToken &propName = prop.GetName();

    TF_REVERSE_FOR_ALL(itr, prop.GetPrim().GetPrimIndex().GetNodeRange()) {

        if (itr->IsInert() || !itr->HasSpecs()) {
            continue;
        }

        SdfPath specPath = itr->GetPath().AppendProperty(propName);
        TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
            bool result = false;
            if ((*layerIt)->HasField(specPath, SdfFieldKeys->Custom, &result)
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
        if (SdfAttributeSpecHandle attrDef = _GetSchemaAttributeSpec(attr)) {
            return attrDef->GetVariability();
        }

        // Check authored scene description.
        const TfToken &attrName = attr.GetName();
        TF_REVERSE_FOR_ALL(itr, attr.GetPrim().GetPrimIndex().GetNodeRange()) {
            if (itr->IsInert() || !itr->HasSpecs())
                continue;

            SdfPath specPath = itr->GetPath().AppendProperty(attrName);
            TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
                SdfVariability result;
                if ((*layerIt)->HasField(
                        specPath, SdfFieldKeys->Variability, &result)) {
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

// Populates the time sample map with the resolved values for the given 
// attribute and returns true if time samples exist, false otherwise.
static bool 
_GetTimeSampleMap(const UsdAttribute &attr, SdfTimeSampleMap *out)
{
    UsdAttributeQuery attrQuery(attr);

    std::vector<double> timeSamples;
    if (attrQuery.GetTimeSamples(&timeSamples)) {
        for (const auto& timeSample : timeSamples) {
            VtValue value;
            if (attrQuery.Get(&value, timeSample)) {
                (*out)[timeSample].Swap(value);
            } else {
                (*out)[timeSample] = VtValue(SdfValueBlock());
            }
        }
        return true;
    }
    return false;
}

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

    UntypedValueComposer composer(result);
    return _GetMetadataImpl(obj, fieldName, keyPath, useFallbacks, &composer);
}

bool 
UsdStage::_GetStrongestResolvedMetadata(const UsdObject &obj,
                                        const TfToken& fieldName,
                                        const TfToken &keyPath,
                                        bool useFallbacks,
                                        SdfAbstractDataValue* result) const
{
    StrongestValueComposer composer(result);
    return _GetMetadataImpl(obj, fieldName, keyPath, useFallbacks, &composer);
}

template <class T>
bool 
UsdStage::_GetTypeSpecificResolvedMetadata(const UsdObject &obj,
                                           const TfToken& fieldName,
                                           const TfToken &keyPath,
                                           bool useFallbacks,
                                           T* result) const
{
    SdfAbstractDataTypedValue<T> out(result);
    TypeSpecificValueComposer<T> composer(&out);
    return _GetMetadataImpl(obj, fieldName, keyPath, useFallbacks, &composer);
}

// This specialization for SdfTimeSampleMap is still required because of the
// attribute time samples hack.
template <>
bool 
UsdStage::_GetTypeSpecificResolvedMetadata(const UsdObject &obj,
                                           const TfToken& fieldName,
                                           const TfToken &keyPath,
                                           bool useFallbacks,
                                           SdfTimeSampleMap* result) const
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
            return _GetTimeSampleMap(obj.As<UsdAttribute>(), result);
        }
    }

    SdfAbstractDataTypedValue<SdfTimeSampleMap> out(result);
    TypeSpecificValueComposer<SdfTimeSampleMap> composer(&out);
    return _GetMetadataImpl(obj, fieldName, keyPath, useFallbacks, &composer);
}

template <class Composer>
void
UsdStage::_GetAttrTypeImpl(const UsdAttribute &attr,
                           const TfToken &fieldName,
                           bool useFallbacks,
                           Composer *composer) const
{
    TRACE_FUNCTION();
    composer->ConsumeUsdFallback(
        attr._Prim()->GetPrimDefinition(),
        attr.GetName(), fieldName, TfToken());
    if (composer->IsDone()) {
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
    composer->ConsumeUsdFallback(
        attr._Prim()->GetPrimDefinition(),
        attr.GetName(), SdfFieldKeys->Variability, TfToken());
    if (composer->IsDone()) {
        return;
    }

    // Otherwise variability is determined by the *weakest* authored opinion.
    // Walk authored scene description in reverse order.
    const TfToken &attrName = attr.GetName();
    TF_REVERSE_FOR_ALL(itr, attr.GetPrim().GetPrimIndex().GetNodeRange()) {
        if (itr->IsInert() || !itr->HasSpecs())
            continue;
        SdfPath specPath = itr->GetPath().AppendProperty(attrName);
        TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
            composer->ConsumeAuthored(
                *itr, *layerIt, specPath, SdfFieldKeys->Variability, TfToken());
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
    if (_GetSchemaPropertySpec(prop)) {
        composer->ConsumeUsdFallback(
            prop._Prim()->GetPrimDefinition(), 
            prop.GetName(),
            SdfFieldKeys->Custom, TfToken());
        return;
    }

    const TfToken &propName = prop.GetName();

    TF_REVERSE_FOR_ALL(itr, prop.GetPrim().GetPrimIndex().GetNodeRange()) {
        if (itr->IsInert() || !itr->HasSpecs())
            continue;

        SdfPath specPath = itr->GetPath().AppendProperty(propName);
        TF_REVERSE_FOR_ALL(layerIt, itr->GetLayerStack()->GetLayers()) {
            composer->ConsumeAuthored(
                *itr, *layerIt, specPath, SdfFieldKeys->Custom, TfToken());
            if (composer->IsDone())
                return;
        }
    }
}

template <class Composer>
static void
_GetPrimTypeNameImpl(Usd_PrimDataConstPtr primData, 
                     bool useFallbacks, Composer *composer)
{
    TRACE_FUNCTION();
    for (Usd_Resolver res(&primData->GetPrimIndex());
         res.IsValid(); res.NextLayer()) {
        TfToken tok;
        if (res.GetLayer()->HasField(
                res.GetLocalPath(), SdfFieldKeys->TypeName, &tok)) {
            if (!tok.IsEmpty() && tok != SdfTokens->AnyTypeToken) {
                composer->ConsumeAuthored(
                    res.GetNode(), res.GetLayer(), res.GetLocalPath(),
                    SdfFieldKeys->TypeName, TfToken());
                if (composer->IsDone())
                    return;
            }
        }
    }
}

template <class Composer>
static bool
_GetPrimSpecifierImpl(Usd_PrimDataConstPtr primData,
                      bool useFallbacks, Composer *composer)
{
    // The pseudo-root and instance prototype prims are always defined -- see
    // Usd_PrimData for details. Since the fallback for specifier is 'over', we
    // have to handle these prims specially here.
    if (primData->GetPath().IsAbsoluteRootPath() || primData->IsPrototype()) {
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
    // Here /A references /B in other.file, and /B inherits class /C.
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
            specPos.GetLocalPath(),
            SdfFieldKeys->Specifier, TfToken());
    }
    return true;
}

template <class ListOpType, class Composer>
static bool 
_GetListOpMetadataImpl(Usd_PrimDataConstPtr primData,
                       const TfToken &propName,
                       const TfToken &fieldName,
                       bool useFallbacks,
                       Usd_Resolver *res,
                       Composer *composer)
{
    // Collect all list op opinions for this field.
    std::vector<ListOpType> listOps;

    SdfPath specPath = res->GetLocalPath(propName);

    for (bool isNewNode = false; res->IsValid(); isNewNode = res->NextLayer()) {
        if (isNewNode)
            specPath = res->GetLocalPath(propName);

        // Consume an authored opinion here, if one exists.
        ListOpType op;
        if (res->GetLayer()->HasField(specPath, fieldName, &op)) {
            listOps.emplace_back(op);
        }
    }

    if (useFallbacks) {
        ListOpType fallbackListOp;
        SdfAbstractDataTypedValue<ListOpType> out(&fallbackListOp);
        TypeSpecificValueComposer<ListOpType> composer(&out);
        if (_GetFallbackMetadataImpl(
                primData, propName, fieldName, TfToken(), &composer)) {
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
UsdStage::_GetSpecialPropMetadataImpl(const UsdObject &obj,
                                      const TfToken &fieldName,
                                      const TfToken &keyPath,
                                      bool useFallbacks,
                                      Composer *composer) const
{
    // Dispatch to special-case composition rules based on type and field.
    // Return true if the given field was handled, false otherwise.
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
    return false;
}

template <class Composer>
static bool
_GetSpecialPrimMetadataImpl(Usd_PrimDataConstPtr primData,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            bool useFallbacks,
                            Composer *composer)
{
    // Dispatch to special-case composition rules based on type and field.
    // Return true if the given field was handled, false otherwise.
    if (fieldName == SdfFieldKeys->TypeName) {
        _GetPrimTypeNameImpl(primData, useFallbacks, composer);
        return true;
    } else if (fieldName == SdfFieldKeys->Specifier) {
        _GetPrimSpecifierImpl(primData, useFallbacks, composer);
        return true;
    } else if (fieldName == SdfFieldKeys->Kind) {
        // XXX: We do not not respect fallback kind values during
        // Usd_PrimData composition (see _GetKind), but we do allow
        // fallback values here to maintain existing behavior. However,
        // we may want to force the useFallbacks flag to false here for
        // consistency.
        _GetPrimKindImpl(primData, useFallbacks, composer);
        return true;
    } else if (fieldName == SdfFieldKeys->Active) {
        // XXX: See comment in the handling of 'kind' re: fallback values.
        _GetPrimActiveImpl(primData, useFallbacks, composer);
        return true;
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
    if (obj.Is<UsdProperty>()) {
        if (_GetSpecialPropMetadataImpl(
                obj, fieldName, keyPath, useFallbacks, composer)) {
            return composer->IsDone() && m.IsClean();
        }
    } else if (obj.Is<UsdPrim>()) {
        // If the prim is the pseudo root, we have a special metadata 
        // composition to perform as the pseudoroot only composes metadata
        // opinions from the session layer and root layer.
        if (obj._Prim()->IsPseudoRoot()) {
            // Note that this function returns true if an opinion was found so
            // we don't need to check composer->IsDone(). IsDone will always
            // return false for dictionary metadata on the pseudo root since
            // we don't have fallbacks.
            return _ComposePseudoRootMetadataImpl(
                get_pointer(obj._Prim()), fieldName, keyPath,
                _rootLayer, _sessionLayer, composer) && m.IsClean();
        } else if (_GetSpecialPrimMetadataImpl(
            get_pointer(obj._Prim()), fieldName, keyPath, useFallbacks, 
            composer)) {
            return composer->IsDone() && m.IsClean();
        }
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
    const Usd_PrimDataConstPtr primData = get_pointer(obj._Prim());

    static TfToken empty;
    const TfToken &propName = obj.Is<UsdProperty>() ? obj.GetName() : empty;

    Usd_Resolver resolver(&primData->GetPrimIndex());
    if (!_ComposeGeneralMetadataImpl(
            primData, propName, fieldName, keyPath, useFallbacks, &resolver, 
            composer)) {
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
                primData, propName, fieldName, useFallbacks, &resolver, 
                composer);
        }
        else if (valueTypeId == typeid(SdfInt64ListOp)) {
            return _GetListOpMetadataImpl<SdfInt64ListOp>(
                primData, propName, fieldName, useFallbacks, &resolver, 
                composer);
        }
        else if (valueTypeId == typeid(SdfUIntListOp)) {
            return _GetListOpMetadataImpl<SdfUIntListOp>(
                primData, propName, fieldName, useFallbacks, &resolver, 
                composer);
        }
        else if (valueTypeId == typeid(SdfUInt64ListOp)) {
            return _GetListOpMetadataImpl<SdfUInt64ListOp>(
                primData, propName, fieldName, useFallbacks, &resolver,
                composer);
        }
        else if (valueTypeId == typeid(SdfStringListOp)) {
            return _GetListOpMetadataImpl<SdfStringListOp>(
                primData, propName, fieldName, useFallbacks, &resolver, 
                composer);
        }
        else if (valueTypeId == typeid(SdfTokenListOp)) {
            return _GetListOpMetadataImpl<SdfTokenListOp>(
                primData, propName, fieldName, useFallbacks, &resolver, 
                composer);
        }
    }
    
    return true;
}

bool
UsdStage::_HasMetadata(const UsdObject &obj, const TfToken& fieldName,
                       const TfToken &keyPath, bool useFallbacks) const
{
    ExistenceComposer composer;
    _GetMetadataImpl(obj, fieldName, keyPath, useFallbacks, &composer);
    return composer.IsDone();
}

static
SdfSpecType
_ListMetadataFieldsImpl(Usd_PrimDataConstPtr primData,
                        const TfToken &propName,
                        bool useFallbacks,
                        TfTokenVector *result)
{
    TRACE_FUNCTION();

    Usd_Resolver res(&primData->GetPrimIndex());
    SdfPath specPath = res.GetLocalPath(propName);
    PcpNodeRef lastNode = res.GetNode();
    SdfSpecType specType = SdfSpecTypeUnknown;

    const UsdPrimDefinition &primDef = primData->GetPrimDefinition();

    // If this is a builtin property, determine specType from the definition.
    if (!propName.IsEmpty()) {
        specType = primDef.GetSpecType(propName);
    }

    // Insert authored fields, discovering spec type along the way.
    for (; res.IsValid(); res.NextLayer()) {
        if (res.GetNode() != lastNode) {
            lastNode = res.GetNode();
            specPath = res.GetLocalPath(propName);
        }
        const SdfLayerRefPtr& layer = res.GetLayer();
        if (specType == SdfSpecTypeUnknown)
            specType = layer->GetSpecType(specPath);

        for (const auto& fieldName : layer->ListFields(specPath)) {
            if (!_IsPrivateFieldKey(fieldName))
                result->push_back(fieldName);
        }
    }

    // If including fallbacks, add any defined metadata fields from the prim
    // definition for the property (or the prim if the prop name is empty). 
    if (useFallbacks) {
        const TfTokenVector fallbackFields = propName.IsEmpty() ?
            primDef.ListMetadataFields() : 
            primDef.ListPropertyMetadataFields(propName);
        result->insert(result->end(), 
                       fallbackFields.begin(), fallbackFields.end());
    }

    return specType;
}

static
SdfSpecType
_ListPseudoRootMetadataFieldsImpl(Usd_PrimDataConstPtr primData, 
                                  const SdfLayerRefPtr &rootLayer,
                                  const SdfLayerRefPtr &sessionLayer,
                                  TfTokenVector *result)
{
    TRACE_FUNCTION();

    const SdfPath &specPath = SdfPath::AbsoluteRootPath();
    PcpNodeRef node = primData->GetPrimIndex().GetRootNode();

    // If we a have a session layer and it isn't muted, get its authored layer
    // metadata fields. The session layer will be the first layer in the 
    // layer stack unless it is muted.
    if (sessionLayer && 
        node.GetLayerStack()->GetLayers().front() == sessionLayer) {
        for (const auto& fieldName : sessionLayer->ListFields(specPath)) {
            if (!_IsPrivateFieldKey(fieldName)) {
                result->push_back(fieldName);
            }
        }
    }

    // Get all authored layer metadata fields from the root layer (which can't
    // be muted).
    for (const auto& fieldName : rootLayer->ListFields(specPath)) {
        if (!_IsPrivateFieldKey(fieldName)) {
            result->push_back(fieldName);
        }
    }

    return SdfSpecTypePseudoRoot;
}

TfTokenVector
UsdStage::_ListMetadataFields(const UsdObject &obj, bool useFallbacks) const
{
    TRACE_FUNCTION();

    TfTokenVector result;

    SdfSpecType specType = SdfSpecTypeUnknown;
    Usd_PrimDataConstPtr primData = get_pointer(obj._Prim());
    if (obj.Is<UsdProperty>()) {
        // List metadata fields for property
        specType = _ListMetadataFieldsImpl(
            primData, obj.GetName(), useFallbacks, &result);
    } else if (obj._Prim()->IsPseudoRoot()) {
        // Custom implementation for listing metadata for the pseudo root.
        specType = _ListPseudoRootMetadataFieldsImpl(
            primData, _rootLayer, _sessionLayer, &result);
    } else {
        // List metadata fields for non pseudo root prims.
        specType = _ListMetadataFieldsImpl(
            primData, TfToken(), useFallbacks, &result);
    }

    // Insert required fields for spec type.
    const SdfSchema::SpecDefinition* specDef = nullptr;
    specDef = SdfSchema::GetInstance().GetSpecDefinition(specType);
    if (specDef) {
        for (const auto& fieldName : specDef->GetRequiredFields()) {
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
        UntypedValueComposer composer(&val, anchorAssetPathsOnly);
        _GetMetadataImpl(obj, fieldName, TfToken(), useFallbacks, &composer);
        result[fieldName] = val;
    }
}

// --------------------------------------------------------------------- //
// Default & TimeSample Resolution
// --------------------------------------------------------------------- //

static bool
_ClipsApplyToLayerStackSite(
    const Usd_ClipSetRefPtr& clips,
    const PcpLayerStackPtr& layerStack, const SdfPath& primPathInLayerStack)
{
    return (layerStack == clips->sourceLayerStack
            && primPathInLayerStack.HasPrefix(clips->sourcePrimPath));
}

static bool
_ClipsApplyToNode(
    const Usd_ClipSetRefPtr& clips, 
    const PcpNodeRef& node)
{
    return (node.GetLayerStack() == clips->sourceLayerStack
            && node.GetPath().HasPrefix(clips->sourcePrimPath));
}

static bool
_ClipsContainValueForAttribute(
    const Usd_ClipSetRefPtr& clips,
    const SdfPath& attrSpecPath)
{
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
    if (clips->manifestClip) {
        SdfVariability attrVariability = SdfVariabilityUniform;
        if (clips->manifestClip->HasField(
                attrSpecPath, SdfFieldKeys->Variability, &attrVariability)
            && attrVariability == SdfVariabilityVarying) {
            return true;
        }
    }
    return false;
}

static
const std::vector<Usd_ClipSetRefPtr>
_GetClipsThatApplyToNode(
    const std::vector<Usd_ClipSetRefPtr>& clipsAffectingPrim,
    const PcpNodeRef& node,
    const SdfPath& specPath)
{
    std::vector<Usd_ClipSetRefPtr> relevantClips;

    for (const auto& localClips : clipsAffectingPrim) {
        if (_ClipsApplyToNode(localClips, node)
            && _ClipsContainValueForAttribute(localClips, specPath)) {
            relevantClips.push_back(localClips);
        }
    }

    return relevantClips;
}

static bool
_HasTimeSamples(const SdfLayerRefPtr& source, 
                const SdfPath& specPath, 
                const double* time = nullptr, 
                double* lower = nullptr, double* upper = nullptr)
{
    if (time) {
        // If caller wants bracketing time samples as well, we can just use
        // GetBracketingTimeSamplesForPath. If no samples exist, this should
        // return false.
        return source->GetBracketingTimeSamplesForPath(
            specPath, *time, lower, upper);
    }

    return source->GetNumTimeSamplesForPath(specPath) > 0;
}

static bool
_HasTimeSamples(const Usd_ClipSetRefPtr& sourceClips, 
                const SdfPath& specPath, 
                const double* time = nullptr, 
                double* lower = nullptr, double* upper = nullptr)
{
    // Bail out immediately if this clip set does not contain values
    // for this attribute.
    if (!_ClipsContainValueForAttribute(sourceClips, specPath)) {
        return false;
    }

    if (time) {
        return sourceClips->GetBracketingTimeSamplesForPath(
            specPath, *time, lower, upper);
    }

    // Since this clip set has declared it contains values for this
    // attribute, we always return true.
    return true;
}

// Helper for getting the fully resolved value from an attribute generically
// for all value types for use by _GetValue and _GetValueForResolveInfo. 
template <class T>
struct Usd_AttrGetValueHelper {

public:
    // Get the value at time for the attribute. The getValueImpl function is
    // templated for sharing of this functionality between _GetValue and 
    // _GetValueForResolveInfo.
    template <class Fn>
    static bool GetValue(const UsdStage &stage, UsdTimeCode time, 
                         const UsdAttribute &attr, T* result, 
                         const Fn &getValueImpl)
    {
        // Special case if time is default: we can grab the value from the
        // metadata. This value will be fully resolved already.
        if (time.IsDefault()) {
            SdfAbstractDataTypedValue<T> out(result);
            TypeSpecificValueComposer<T> composer(&out);
            bool valueFound = stage._GetMetadataImpl(
                attr, SdfFieldKeys->Default, TfToken(), 
                /*useFallbacks=*/true, &composer);

            return valueFound && 
                (!Usd_ClearValueIfBlocked<SdfAbstractDataValue>(&out));
        }

        return _GetResolvedValue(stage, time, attr, result, getValueImpl);
    }

private:
    // Metafunction for selecting the appropriate interpolation object if the
    // given value type supports linear interpolation.
    struct _SelectInterpolator 
        : public boost::mpl::if_c<
              UsdLinearInterpolationTraits<T>::isSupported,
              Usd_LinearInterpolator<T>,
              Usd_HeldInterpolator<T> > { };

    // Gets the attribute value from the implementation with appropriate 
    // interpolation. In the case of value types that have type specific value
    // resolution (like SdfAssetPath and SdfTimeCode), the value returned from
    // from this is NOT fully resolved yet.
    template <class Fn>
    static bool _GetValueFromImpl(const UsdStage &stage,
                                  UsdTimeCode time, const UsdAttribute &attr,
                                  T* result, const Fn &getValueImpl)
    {
        SdfAbstractDataTypedValue<T> out(result);

        if (stage._interpolationType == UsdInterpolationTypeLinear) {
            typedef typename _SelectInterpolator::type _Interpolator;
            _Interpolator interpolator(result);
            return getValueImpl(stage, time, attr, &interpolator, &out);
        };

        Usd_HeldInterpolator<T> interpolator(result);
        return getValueImpl(stage, time, attr, &interpolator, &out);
    }

    // Gets the fully resolved value for the attribute.
    template <class Fn>
    static bool _GetResolvedValue(const UsdStage &stage,
                                  UsdTimeCode time, const UsdAttribute &attr,
                                  T* result, const Fn &getValueImpl)
    {
        if (_GetValueFromImpl(stage, time, attr, result, getValueImpl)) {
            // Do the the type specific value resolution on the result. For 
            // most types _ResolveValue does nothing. 
            _ResolveValue(stage, time, attr, result);
            return true;
        }
        return false;
    }

    // Performs type specific value resolution.
    static void _ResolveValue(
        const UsdStage &stage, UsdTimeCode time, const UsdAttribute &attr,
        T* result)
    {
        // Do nothing for types without type specific value resolution.
        static_assert(!UsdStage::_HasTypeSpecificResolution<T>::value, 
                      "Value types with type specific value resolution must "
                      "specialize Usd_AttrGetValueHelper::_ResolveValue");
    }
};

// Specializations implementing _ResolveValue for types with type specific
// value resolution.
template <>
void Usd_AttrGetValueHelper<SdfAssetPath>::_ResolveValue(
    const UsdStage &stage, UsdTimeCode time, const UsdAttribute &attr,
    SdfAssetPath* result)
{
    stage._MakeResolvedAssetPaths(time, attr, result, 1);
}

template <>
void Usd_AttrGetValueHelper<VtArray<SdfAssetPath>>::_ResolveValue(
    const UsdStage &stage, UsdTimeCode time, const UsdAttribute &attr,
    VtArray<SdfAssetPath>* result)
{
    stage._MakeResolvedAssetPaths(time, attr, result->data(), result->size());
}

template <>
void Usd_AttrGetValueHelper<SdfTimeCode>::_ResolveValue(
    const UsdStage &stage, UsdTimeCode time, const UsdAttribute &attr,
    SdfTimeCode* result)
{
    stage._MakeResolvedTimeCodes(time, attr, result, 1);
}

template <>
void Usd_AttrGetValueHelper<VtArray<SdfTimeCode>>::_ResolveValue(
    const UsdStage &stage, UsdTimeCode time, const UsdAttribute &attr,
    VtArray<SdfTimeCode>* result)
{
    stage._MakeResolvedTimeCodes(time, attr, result->data(), result->size());
}

// Attribute value getter for type erased VtValue.
struct Usd_AttrGetUntypedValueHelper {
    template <class Fn>
    static bool GetValue(const UsdStage &stage, UsdTimeCode time, 
                         const UsdAttribute &attr, VtValue* result, 
                         const Fn &getValueImpl)
    {
        // Special case if time is default: we can grab the value from the
        // metadata. This value will be fully resolved already because 
        // _GetMetadata returns fully resolved values.
        if (time.IsDefault()) {
            bool valueFound = stage._GetMetadata(
                attr, SdfFieldKeys->Default, TfToken(), 
                /*useFallbacks=*/true, result);
            return valueFound && (!Usd_ClearValueIfBlocked(result));
        }

        Usd_UntypedInterpolator interpolator(attr, result);
        if (getValueImpl(stage, time, attr, &interpolator, result)) {
            if (result) {
                // Always run the resolve functions for value types that need 
                // it.
                stage._MakeResolvedAttributeValue(time, attr, result);
            }
            return true;
        }
        return false;
    }
};

bool
UsdStage::_GetValue(UsdTimeCode time, const UsdAttribute &attr,
                    VtValue* result) const
{
    auto getValueImpl = [](const UsdStage &stage,
                           UsdTimeCode time, const UsdAttribute &attr,
                           Usd_InterpolatorBase* interpolator,
                           VtValue* value) 
    {
        return stage._GetValueImpl(time, attr, interpolator, value);
    };

    return Usd_AttrGetUntypedValueHelper::GetValue(
        *this, time, attr, result, getValueImpl);
}

template <class T>
bool
UsdStage::_GetValue(UsdTimeCode time, const UsdAttribute &attr,
                    T* result) const
{
    auto getValueImpl = [](const UsdStage &stage,
                           UsdTimeCode time, const UsdAttribute &attr,
                           Usd_InterpolatorBase* interpolator,
                           SdfAbstractDataValue* value) 
    {
        return stage._GetValueImpl(time, attr, interpolator, value);
    };

    return Usd_AttrGetValueHelper<T>::GetValue(
        *this, time, attr, result, getValueImpl);
}

class UsdStage_ResolveInfoAccess
{
public:
    template <class T>
    static bool _GetTimeSampleValue(
        UsdTimeCode time, const UsdAttribute& attr,
        const UsdResolveInfo &info,
        const double *lowerHint, const double *upperHint,
        Usd_InterpolatorBase *interpolator,
        T *result)
    {
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());
        const SdfLayerHandle& layer = info._layer;
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
                               specPath, localTime, &lower, &upper),
                           "No bracketing time samples for "
                           "%s on <%s> for time %g between %g and %g",
                           layer->GetIdentifier().c_str(),
                           specPath.GetText(),
                           localTime, lower, upper)) {
                return false;
            }
        }

        TF_DEBUG(USD_VALUE_RESOLUTION).Msg(
            "RESOLVE: reading field %s:%s from @%s@, "
            "with requested time = %.3f (local time = %.3f) "
            "reading from sample %.3f \n",
            specPath.GetText(),
            SdfFieldKeys->TimeSamples.GetText(),
            layer->GetIdentifier().c_str(),
            time.GetValue(),
            localTime,
            lower);

        return Usd_GetOrInterpolateValue(
            layer, specPath, localTime, lower, upper, interpolator, result);
    } 

    template <class T>
    static bool _GetClipValue(
        UsdTimeCode time, const UsdAttribute& attr,
        const UsdResolveInfo &info,
        const Usd_ClipSetRefPtr &clipSet, 
        const double *lowerHint, const double *upperHint,
        Usd_InterpolatorBase *interpolator,
        T *result)
    {
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());

        // Note that we do not apply layer offsets to the time.
        // Because clip metadata may be authored in different 
        // layers in the LayerStack, each with their own 
        // layer offsets, it is simpler to bake the effects of 
        // those offsets into Usd_Clip.
        const double localTime = time.GetValue();
        double upper = 0.0;
        double lower = 0.0;

        if (lowerHint && upperHint) {
            lower = *lowerHint;
            upper = *upperHint;
        }
        else {
            _HasTimeSamples(clipSet, specPath, &localTime, &lower, &upper);
        }

        TF_DEBUG(USD_VALUE_RESOLUTION).Msg(
            "RESOLVE: reading field %s:%s from clip set %s, "
            "with requested time = %.3f "
            "reading from sample %.3f \n",
            specPath.GetText(),
            SdfFieldKeys->TimeSamples.GetText(),
            clipSet->name.c_str(),
            localTime,
            lower);

        return Usd_GetOrInterpolateValue(
            clipSet, specPath, localTime, lower, upper, interpolator, result);
    }
};

// Helper structure populated by _GetResolveInfo and _ResolveInfoResolver
// with extra information accumulated in the process. This allows clients to
// avoid redoing work.
template <class T>
struct UsdStage::_ExtraResolveInfo
{
    // If the resolve info source is UsdResolveInfoSourceTimeSamples or
    // UsdResolveInfoSourceValueClips and an explicit time is given to
    // _GetResolveInfo, this will be the lower and upper bracketing time
    // samples for that time.
    double lowerSample = 0;
    double upperSample = 0;

    // If the resolve info source is UsdResolveInfoSourceDefault or
    // UsdResolveInfoSourceFallback and this is non-null, the default
    // or fallback value will be copied to the object this pointer refers to.
    T* defaultOrFallbackValue = nullptr;

    // If the resolve info source is UsdResolveInfoSourceValueClips this will 
    // be the Usd_ClipSet containing values for the attribute.
    Usd_ClipSetRefPtr clipSet;
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
            resultLayer = resolveInfo._layer;
        }
        else if (resolveInfo._source == UsdResolveInfoSourceValueClips) {
            const Usd_ClipSetRefPtr& clipSet = extraResolveInfo.clipSet;
            const Usd_ClipRefPtr& activeClip = 
                clipSet->GetActiveClip(time.GetValue());
            const SdfPath specPath =
                resolveInfo._primPathInLayerStack.AppendProperty(attr.GetName());

            // If the active clip has authored time samples, the value will
            // come from it (or at least be interpolated from it) so use that
            // clip's layer. Otherwise the value will come from the manifest.
            resultLayer = activeClip->HasAuthoredTimeSamples(specPath) ? 
                activeClip->GetLayer() : clipSet->manifestClip->GetLayer();
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
            time, attr, resolveInfo, 
            extraResolveInfo.clipSet,
            &extraResolveInfo.lowerSample, &extraResolveInfo.upperSample,
            interpolator, result);
    }
    else if (resolveInfo._source == UsdResolveInfoSourceDefault ||
             resolveInfo._source == UsdResolveInfoSourceFallback) {
        // Nothing to do here -- the call to _GetResolveInfo will have
        // filled in the result with the default value.
        return m.IsClean();
    }

    return false;
}

// Our property stack resolver never indicates for resolution to stop
// as we need to gather all relevant property specs in the LayerStack
struct UsdStage::_PropertyStackResolver {
    SdfPropertySpecHandleVector propertyStack;

    bool ProcessFallback() { return false; }

    bool
    ProcessLayer(const size_t layerStackPosition,
                 const SdfPath& specPath,
                 const PcpNodeRef& node,
                 const double *time) 
    {
        const auto layer
            = node.GetLayerStack()->GetLayers()[layerStackPosition];
        const auto propertySpec = layer->GetPropertyAtPath(specPath);
        if (propertySpec) {
            propertyStack.push_back(propertySpec); 
        }

        return false;
    }

    bool
    ProcessClips(const Usd_ClipSetRefPtr& clipSet,
                 const SdfPath& specPath,
                 const PcpNodeRef& node,
                 const double* time) 
    {
        // Look through clips to see if they have a time sample for
        // this attribute. If a time is given, examine just the clips
        // that are active at that time.
        double lowerSample = 0.0, upperSample = 0.0;

        if (_HasTimeSamples(
                clipSet, specPath, time, &lowerSample, &upperSample)) {

            const Usd_ClipRefPtr& activeClip = clipSet->GetActiveClip(*time);

            // If the active clip has authored time samples, the value will
            // come from it (or at least be interpolated from it) so use the
            // property spec from that clip. Otherwise the value will come
            // from the manifest.
            const Usd_ClipRefPtr& sourceClip = 
                activeClip->HasAuthoredTimeSamples(specPath) ?
                activeClip : clipSet->manifestClip;

            if (!TF_VERIFY(sourceClip)) {
                return false;
            }

            if (const auto propertySpec = 
                    sourceClip->GetPropertyAtPath(specPath)) {
                propertyStack.push_back(propertySpec);
            }
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
        if (const bool hasFallback = 
                _attr._Prim()->GetPrimDefinition().GetAttributeFallbackValue(
                    _attr.GetName(), _extraInfo->defaultOrFallbackValue)) {
            _resolveInfo->_source = UsdResolveInfoSourceFallback;
            return true;
        }

        // No values at all.
        _resolveInfo->_source = UsdResolveInfoSourceNone;
        return true;
    }

    bool
    ProcessLayer(const size_t layerStackPosition,
                 const SdfPath& specPath,
                 const PcpNodeRef& node,
                 const double *time) 
    {
        const PcpLayerStackRefPtr& nodeLayers = node.GetLayerStack();
        const SdfLayerRefPtrVector& layerStack = nodeLayers->GetLayers();
        const SdfLayerOffset layerToStageOffset =
            _GetLayerToStageOffset(node, layerStack[layerStackPosition]);
        const SdfLayerRefPtr& layer = layerStack[layerStackPosition];
        boost::optional<double> localTime;
        if (time) {
            localTime = layerToStageOffset.GetInverse() * (*time);
        }

        if (_HasTimeSamples(layer, specPath, localTime.get_ptr(), 
                            &_extraInfo->lowerSample, 
                            &_extraInfo->upperSample)) {
            _resolveInfo->_source = UsdResolveInfoSourceTimeSamples;
        }
        else { 
            Usd_DefaultValueResult defValue = Usd_HasDefault(
                layer, specPath, _extraInfo->defaultOrFallbackValue);
            if (defValue == Usd_DefaultValueResult::Found) {
                _resolveInfo->_source = UsdResolveInfoSourceDefault;
            }
            else if (defValue == Usd_DefaultValueResult::Blocked) {
                _resolveInfo->_valueIsBlocked = true;
                return ProcessFallback();
            }
        }

        if (_resolveInfo->_source != UsdResolveInfoSourceNone) {
            _resolveInfo->_layerStack = nodeLayers;
            _resolveInfo->_layer = layer;
            _resolveInfo->_primPathInLayerStack = node.GetPath();
            _resolveInfo->_layerToStageOffset = layerToStageOffset;
            _resolveInfo->_node = node;
            return true;
        }

        return false;
    }

    bool
    ProcessClips(const Usd_ClipSetRefPtr& clipSet,
                 const SdfPath& specPath,
                 const PcpNodeRef& node,
                 const double* time)
    {
        if (!_HasTimeSamples(
                clipSet, specPath, time,
                &_extraInfo->lowerSample, &_extraInfo->upperSample)) {
            return false;
        }

        _extraInfo->clipSet = clipSet;

        _resolveInfo->_source = UsdResolveInfoSourceValueClips;
        _resolveInfo->_layerStack = node.GetLayerStack();
        _resolveInfo->_primPathInLayerStack = node.GetPath();
        _resolveInfo->_node = node;
        
        return true;
    }

private:
    const UsdAttribute& _attr;
    UsdResolveInfo* _resolveInfo;
    UsdStage::_ExtraResolveInfo<T>* _extraInfo;
};

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
         resolveInfo->_source == UsdResolveInfoSourceValueClips) &&
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
//       ProcessClips()
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
    const std::vector<Usd_ClipSetRefPtr>* clipsAffectingPrim = nullptr;
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

        const SdfPath specPath = node.GetPath().AppendProperty(prop.GetName());
        const SdfLayerRefPtrVector& layerStack 
            = node.GetLayerStack()->GetLayers();
        boost::optional<std::vector<Usd_ClipSetRefPtr>> clips;
        for (size_t i = 0, e = layerStack.size(); i < e; ++i) {
            if (nodeHasSpecs) { 
                if (resolver->ProcessLayer(i, specPath, node, 
                                           localTime.get_ptr())) {
                    return;
                }
            }

            if (clipsAffectingPrim){ 
                if (!clips) {
                    clips = _GetClipsThatApplyToNode(*clipsAffectingPrim,
                                                     node, specPath);
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

                for (const Usd_ClipSetRefPtr& clipSet : *clips) {
                    // We only care about clips that were introduced at this
                    // position within the LayerStack.
                    if (clipSet->sourceLayerIndex != i) {
                        continue;
                    }

                    // Look through clips to see if they have a time sample for
                    // this attribute. If a time is given, examine just the clips
                    // that are active at that time.
                    if (resolver->ProcessClips(
                            clipSet, specPath, node, localTime.get_ptr())) {
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
    if (info._source == UsdResolveInfoSourceTimeSamples) {
        return UsdStage_ResolveInfoAccess::_GetTimeSampleValue(
            time, attr, info, nullptr, nullptr, interpolator, result);
    }
    else if (info._source == UsdResolveInfoSourceDefault) {
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());
        const SdfLayerHandle& layer = info._layer;

        TF_DEBUG(USD_VALUE_RESOLUTION).Msg(
            "RESOLVE: reading field %s:%s from @%s@, with t = %.3f"
            " as default\n",
            specPath.GetText(),
            SdfFieldKeys->TimeSamples.GetText(),
            layer->GetIdentifier().c_str(),
            time.GetValue());

        return TF_VERIFY(
            layer->HasField(specPath, SdfFieldKeys->Default, result));
    }
    else if (info._source == UsdResolveInfoSourceValueClips) {
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());

        const UsdPrim prim = attr.GetPrim();
        const std::vector<Usd_ClipSetRefPtr>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        for (const auto& clipSet : clipsAffectingPrim) {
            if (!_ClipsApplyToLayerStackSite(
                    clipSet, info._layerStack, info._primPathInLayerStack)
                || !_ClipsContainValueForAttribute(clipSet, specPath)) {
                continue;
            }

            return UsdStage_ResolveInfoAccess::_GetClipValue(
                time, attr, info, clipSet, nullptr, nullptr,
                interpolator, result);
        }
    }
    else if (info._source == UsdResolveInfoSourceFallback) {
        // Get the fallback value.
        return attr._Prim()->GetPrimDefinition().GetAttributeFallbackValue(
                attr.GetName(), result);
    }

    return false;
}

bool
UsdStage::_GetValueFromResolveInfo(const UsdResolveInfo &info,
                                   UsdTimeCode time, const UsdAttribute &attr,
                                   VtValue* result) const
{
    auto getValueImpl = [&info](const UsdStage &stage,
                                UsdTimeCode time, const UsdAttribute &attr,
                                Usd_InterpolatorBase* interpolator,
                                VtValue* value) 
    {
        return stage._GetValueFromResolveInfoImpl(
            info, time, attr, interpolator, value);
    };

    return Usd_AttrGetUntypedValueHelper::GetValue(
        *this, time, attr, result, getValueImpl);
}

template <class T>
bool 
UsdStage::_GetValueFromResolveInfo(const UsdResolveInfo &info,
                                   UsdTimeCode time, const UsdAttribute &attr,
                                   T* result) const
{
    auto getValueImpl = [&info](const UsdStage &stage,
                                UsdTimeCode time, const UsdAttribute &attr, 
                                Usd_InterpolatorBase* interpolator,
                                SdfAbstractDataValue* value) 
    {
        return stage._GetValueFromResolveInfoImpl(
            info, time, attr, interpolator, value);
    };

    return Usd_AttrGetValueHelper<T>::GetValue(
        *this, time, attr, result, getValueImpl);
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
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());
        const SdfLayerHandle& layer = info._layer;

        const std::set<double> samples =
            layer->ListTimeSamplesForPath(specPath);
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
    else if (info._source == UsdResolveInfoSourceValueClips) {
        const UsdPrim prim = attr.GetPrim();

        // See comments in _GetValueImpl regarding clips.
        const std::vector<Usd_ClipSetRefPtr>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());

        // Loop through all the clips that apply to this node and
        // combine all the time samples that are provided.
        for (const auto& clipSet : clipsAffectingPrim) {
            if (!_ClipsApplyToLayerStackSite(
                    clipSet, info._layerStack, info._primPathInLayerStack)
                || !_ClipsContainValueForAttribute(clipSet, specPath)) {
                continue;
            }

            // See comments in _GetValueImpl regarding layer
            // offsets and why they're not applied here.
            const std::set<double> samples =
                clipSet->ListTimeSamplesForPath(specPath);
            copySamplesInInterval(samples, times, interval);;
            return true;
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
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());
        const SdfLayerHandle& layer = info._layer;

        return layer->GetNumTimeSamplesForPath(specPath);
    } 
    else if (info._source == UsdResolveInfoSourceValueClips) {
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
    else if (resolveInfo._source == UsdResolveInfoSourceValueClips) {
        *lower = extraInfo.lowerSample;
        *upper = extraInfo.upperSample;
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
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());
        const SdfLayerHandle& layer = info._layer;
        const double layerTime =
            info._layerToStageOffset.GetInverse() * desiredTime;
        
        if (layer->GetBracketingTimeSamplesForPath(
                specPath, layerTime, lower, upper)) {

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
    else if (info._source == UsdResolveInfoSourceValueClips) {
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());

        const UsdPrim prim = attr.GetPrim();

        // See comments in _GetValueImpl regarding clips.
        const std::vector<Usd_ClipSetRefPtr>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(prim.GetPath());

        for (const auto& clipSet : clipsAffectingPrim) {
            if (!_ClipsApplyToLayerStackSite(
                    clipSet, info._layerStack, info._primPathInLayerStack)
                || !_ClipsContainValueForAttribute(clipSet, specPath)) {
                continue;
            }

            if (clipSet->GetBracketingTimeSamplesForPath(
                    specPath, desiredTime, lower, upper)) {
                *hasSamples = true;
                return true;
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
        if (SdfAttributeSpecHandle attrDef = _GetSchemaAttributeSpec(attr)) {
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
_ValueFromClipsMightBeTimeVarying(const Usd_ClipSetRefPtr &clipSet,
                                  const SdfPath &attrSpecPath)
{
    // If there is only one clip active over all time and it has more than one
    // time sample for the attribute, it might be time varying. Otherwise the
    // attribute's value must be constant over all time.
    if (clipSet->valueClips.size() == 1) {
        const size_t numTimeSamples = 
            clipSet->valueClips.front()->GetNumTimeSamplesForPath(attrSpecPath);
        return numTimeSamples > 1;
    }

    // Since there are multiple clips active across all time, we can't say
    // for certain whether there are multiple time samples without 
    // potentially opening every clip. So, we have to report that the value
    // might be time varying.
    return true;
}

bool 
UsdStage::_ValueMightBeTimeVarying(const UsdAttribute &attr) const
{
    UsdResolveInfo info;
    _ExtraResolveInfo<SdfAbstractDataValue> extraInfo;
    _GetResolveInfo(attr, &info, nullptr, &extraInfo);

    if (info._source == UsdResolveInfoSourceValueClips) {
        // See comment in _ValueMightBeTimeVaryingFromResolveInfo.
        const SdfPath specPath = 
            info._primPathInLayerStack.AppendProperty(attr.GetName());
        return _ValueFromClipsMightBeTimeVarying(extraInfo.clipSet, specPath);
    }

    return _ValueMightBeTimeVaryingFromResolveInfo(info, attr);
}

bool 
UsdStage::_ValueMightBeTimeVaryingFromResolveInfo(const UsdResolveInfo &info,
                                                  const UsdAttribute &attr) const
{
    if (info._source == UsdResolveInfoSourceValueClips) {
        // Do a specialized check for value clips instead of falling through
        // to calling _GetNumTimeSamplesFromResolveInfo, which requires opening
        // every clip to get the total time sample count.
        const SdfPath specPath =
            info._primPathInLayerStack.AppendProperty(attr.GetName());

        const std::vector<Usd_ClipSetRefPtr>& clipsAffectingPrim =
            _clipCache->GetClipsForPrim(attr.GetPrim().GetPath());
        for (const auto& clipSet : clipsAffectingPrim) {
            if (!_ClipsApplyToLayerStackSite(
                    clipSet, info._layerStack, info._primPathInLayerStack)) {
                continue;
            }

            if (_HasTimeSamples(clipSet, specPath)) {
                return _ValueFromClipsMightBeTimeVarying(clipSet, specPath);
            }
        }
        
        return false;
    }

    return _GetNumTimeSamplesFromResolveInfo(info, attr) > 1;
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
    
    if (!GetPseudoRoot().GetMetadata(key, value)) {
        *value = SdfSchema::GetInstance().GetFallback(key);
    } else if (value->IsHolding<VtDictionary>()){
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

    return (GetPseudoRoot().HasAuthoredMetadata(key) ||
            !schema.GetFallback(key).IsEmpty());
}

bool
UsdStage::HasAuthoredMetadata(const TfToken& key) const
{
    const SdfSchema &schema = SdfSchema::GetInstance();
    
    if (!schema.IsValidFieldForSpec(key, SdfSpecTypePseudoRoot))
        return false;

    return GetPseudoRoot().HasAuthoredMetadata(key);
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

    if (!GetPseudoRoot().GetMetadataByDictKey(key, keyPath, value)) {
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

    if (GetPseudoRoot().HasAuthoredMetadataDictKey(key, keyPath)) {
        return true;
    }

    const VtValue &fallback =  schema.GetFallback(key);
    
    return ((!fallback.IsEmpty()) &&
            (fallback.Get<VtDictionary>().GetValueAtPath(keyPath) != nullptr));
}

bool
UsdStage::HasAuthoredMetadataDictKey(
    const TfToken& key, const TfToken &keyPath) const
{
    if (keyPath.IsEmpty())
        return false;

    return GetPseudoRoot().HasAuthoredMetadataDictKey(key, keyPath);
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
    // PcpLayerStack computes timeCodesPerSecond for its map function layer 
    // offsets. The root layer stack will always have the stage's fully
    // computed timeCodesPerSecond value accounting for the unique interaction
    // between the root and session layer.
    const PcpLayerStackPtr localLayerStack = _GetPcpCache()->GetLayerStack();
    return localLayerStack->GetTimeCodesPerSecond();
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
    if (SdfLayer::IsAnonymousLayerIdentifier(identifier)) {
        if (SdfLayerHandle lyr = SdfLayer::Find(identifier)){
            TF_DEBUG(USD_PATH_RESOLUTION).Msg(
                "Resolved identifier %s because it was anonymous\n",
                identifier.c_str());
            return identifier;
        }
        else {
            TF_DEBUG(USD_PATH_RESOLUTION).Msg(
                "Resolved identifier %s to \"\" because it was anonymous but "
                "no layer is open with that identifier\n",
                identifier.c_str());
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

        // Notify, as interpolated attributes values have likely changed.
        UsdStageWeakPtr self(this);
        UsdNotice::ObjectsChanged::_PathsToChangesMap resyncChanges, infoChanges;
        resyncChanges[SdfPath::AbsoluteRootPath()];
        UsdNotice::ObjectsChanged(self, &resyncChanges, &infoChanges).Send(self);
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

// Explicitly instantiate templated getters and setters for all Sdf value
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
        SDF_VALUE_CPP_ARRAY_TYPE(elem)*) const;                         \
                                                                        \
    template bool UsdStage::_SetValue(                                  \
        UsdTimeCode, const UsdAttribute&,                               \
        const SDF_VALUE_CPP_TYPE(elem)&);                               \
    template bool UsdStage::_SetValue(                                  \
        UsdTimeCode, const UsdAttribute&,                               \
        const SDF_VALUE_CPP_ARRAY_TYPE(elem)&);

BOOST_PP_SEQ_FOR_EACH(_INSTANTIATE_GET, ~, SDF_VALUE_TYPES)
#undef _INSTANTIATE_GET

// In addition to the Sdf value types, _SetValue can also be called with an 
// SdfValueBlock.
template bool UsdStage::_SetValue(
    UsdTimeCode, const UsdAttribute&, const SdfValueBlock &);

// Explicitly instantiate the templated _SetEditTargetMappedMetadata and
// _GetTypeSpecificResolvedMetadata functions for the types that support each. 
// The types instantiated here must match the types whose value is true for 
// _HasTypeSpecificResolution<T> and _IsEditTargetMappable<T>.
#define INSTANTIATE_SET_MAPPED_METADATA(elem)                               \
    template USD_API bool UsdStage::_SetEditTargetMappedMetadata(           \
        const UsdObject &, const TfToken&, const TfToken &, const elem &);     

#define INSTANTIATE_GET_TYPE_RESOLVED_METADATA(elem)                                 \
    template USD_API bool UsdStage::_GetTypeSpecificResolvedMetadata(                       \
        const UsdObject &, const TfToken&, const TfToken &, bool, elem *) const;

#define INSTANTIATE_GET_TYPE_RESOLVED_AND_SET_MAPPED_METADATA(elem)  \
    INSTANTIATE_GET_TYPE_RESOLVED_METADATA(elem);                    \
    INSTANTIATE_SET_MAPPED_METADATA(elem);      

INSTANTIATE_GET_TYPE_RESOLVED_METADATA(SdfAssetPath);
INSTANTIATE_GET_TYPE_RESOLVED_METADATA(VtArray<SdfAssetPath>);
INSTANTIATE_GET_TYPE_RESOLVED_AND_SET_MAPPED_METADATA(SdfTimeCode);
INSTANTIATE_GET_TYPE_RESOLVED_AND_SET_MAPPED_METADATA(VtArray<SdfTimeCode>);
// Do not explicitly instantiate _GetTypeSpecificResolvedMetadata for
// SdfTimeSampleMap because we provide a specialization instead.
INSTANTIATE_SET_MAPPED_METADATA(SdfTimeSampleMap);
INSTANTIATE_GET_TYPE_RESOLVED_AND_SET_MAPPED_METADATA(VtDictionary);

#undef INSTANTIATE_GET_TYPE_RESOLVED_AND_SET_MAPPED_METADATA
#undef INSTANTIATE_GET_TYPE_RESOLVED_METADATA
#undef INSTANTIATE_SET_MAPPED_METADATA

// Make sure both versions of _SetMetadataImpl are instantiated as they are 
// directly called from UsdObject.
template USD_API bool UsdStage::_SetMetadataImpl(
    const UsdObject &, const TfToken &, const TfToken &, 
    const VtValue &);
template USD_API bool UsdStage::_SetMetadataImpl(
    const UsdObject &, const TfToken &, const TfToken &, 
    const SdfAbstractDataConstValue &);

PXR_NAMESPACE_CLOSE_SCOPE

