//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/clipSetDefinition.h"

#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/resolver.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

// offset is an optional metadata in template clips, this value is
// used to signify that it was not specified.
constexpr double _DefaultClipOffsetValue = std::numeric_limits<double>::max();

// ------------------------------------------------------------

// XXX: Duplicate of function in usd/stage.cpp. Refactor?
static SdfLayerOffset
_GetLayerOffsetToRoot(
    const PcpNodeRef& pcpNode, 
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

static void
_ApplyOffsetToTimes(
    const SdfLayerOffset& offset,
    const SdfLayerOffset& timesOffset,
    VtVec2dArray* array)
{
    if (!array) {
        return;
    }

    // When an attribute backed by a clip is queried, the stage
    // transforms the query time with the clipSet's layer offset.
    // In very rare scenarios, the clip times may be authored with
    // a different layer offset from the clipSet's layer offset.
    // To ensure the desired offset transformation to clip times is
    // accurately applied, we bake in a layer offset of
    // `timesOffset * offset.inverse`. Note that:
    //
    // timesOffset * clipTimes =
    //     offset * (timesOffset * offset.inverse) * clipTimes
    if (offset == timesOffset) {
        return;
    }

    for (auto& time : *array) {
        time[0] = timesOffset * offset.GetInverse() * time[0]; 
    }
}

template <typename V>
static void
_ClipDerivationMsg(const TfToken& metadataName,
                   const V& v,
                   const SdfPath& usdPrimPath)
{
    TF_DEBUG(USD_CLIPS).Msg(
        "%s for prim <%s> derived: %s\n",
        metadataName.GetText(),
        usdPrimPath.GetText(),
        TfStringify(v).c_str());
}

namespace {
    struct _ClipTimeString {
        std::string integerPortion;
        std::string decimalPortion;
    };
}

static _ClipTimeString
_DeriveClipTimeString(const double currentClipTime,
                      const size_t numIntegerHashes,
                      const size_t numDecimalHashes) 
{
    std::string integerPortion = "";
    std::string decimalPortion = "";
       
    auto integerSpec = "%0" + TfStringify(numIntegerHashes) + "d";
    integerPortion = TfStringPrintf(integerSpec.c_str(), int(currentClipTime));

    // If we are dealing with a subframe integer
    // specification, such as foo.###.###.usd
    if (numDecimalHashes != 0) {
        auto decimalSpec = "%.0" + TfStringify(numDecimalHashes) + "f";
        std::string stringRep = TfStringPrintf(decimalSpec.c_str(), 
                                               currentClipTime);
        auto splitAt = stringRep.find('.');

        // We trim anything larger that the specified number of values
        decimalPortion = stringRep.substr(splitAt+1);
    }

    return { integerPortion, decimalPortion };
}

static void
_DeriveClipInfo(const std::string& templateAssetPath,
                const double stride,
                const double activeOffset,
                const double startTimeCode,
                const double endTimeCode,
                std::optional<VtVec2dArray>* clipTimes,
                std::optional<VtVec2dArray>* clipActive,
                std::optional<VtArray<SdfAssetPath>>* clipAssetPaths,
                const SdfPath& usdPrimPath,
                const PcpLayerStackPtr& sourceLayerStack,
                const size_t indexOfSourceLayer)
{
    if (stride <= 0) {
        TF_WARN("Invalid %s %f for prim <%s>. %s must be greater than 0.", 
                UsdClipsAPIInfoKeys->templateStride.GetText(), stride,
                usdPrimPath.GetText(),
                UsdClipsAPIInfoKeys->templateStride.GetText());
        return;
    }

    bool activeOffsetProvided = activeOffset != _DefaultClipOffsetValue;
    if (activeOffsetProvided && (std::abs(activeOffset) > stride)) {
        TF_WARN("Invalid %s %f for prim <%s>. "
                "Absolute value of %s must not exceed %s %f.",
                UsdClipsAPIInfoKeys->templateActiveOffset.GetText(), 
                activeOffset, usdPrimPath.GetText(),
                UsdClipsAPIInfoKeys->templateActiveOffset.GetText(), 
                UsdClipsAPIInfoKeys->templateStride.GetText(), stride);
        return;
    }

    std::string templateLayerPath;
    SdfLayer::FileFormatArguments args;
    SdfLayer::SplitIdentifier(templateAssetPath, &templateLayerPath, &args);

    auto path = TfGetPathName(templateLayerPath);
    auto basename = TfGetBaseName(templateLayerPath);
    auto tokenizedBasename = TfStringTokenize(basename, ".");

    size_t integerHashSectionIndex = std::numeric_limits<size_t>::max();
    size_t decimalHashSectionIndex = std::numeric_limits<size_t>::max();

    size_t numIntegerHashes = 0;
    size_t numDecimalHashes = 0;

    size_t matchingGroups = 0;
    size_t tokenIndex = 0;

    // obtain our 'groups', meaning the hash sequences denoting
    // how much padding the user is requesting in their template string
    for (const auto& token : tokenizedBasename) {
        if (std::all_of(token.begin(), token.end(), 
                        [](const char& c) { return c == '#'; })) {
            if (integerHashSectionIndex == std::numeric_limits<size_t>::max()) {
                numIntegerHashes = token.size();
                integerHashSectionIndex = tokenIndex;
            } else {
                numDecimalHashes = token.size();
                decimalHashSectionIndex = tokenIndex;
            }
            matchingGroups++;
        }
        tokenIndex++;
    }

    if ((matchingGroups != 1 && matchingGroups != 2)
        || (matchingGroups == 2 
            && (integerHashSectionIndex != decimalHashSectionIndex - 1))) {
        TF_WARN("Invalid %s '%s' for prim <%s>. It must be "
                "of the form path/basename.###.usd or "
                "path/basename.###.###.usd. Note that the number "
                "of hash marks is variable in each group.",
                UsdClipsAPIInfoKeys->templateAssetPath.GetText(),
                templateAssetPath.c_str(),
                usdPrimPath.GetText());
        return;
    }

    if (startTimeCode > endTimeCode) {
        TF_WARN("Invalid time range specified for prim <%s>. "
                "%s (%f) cannot be greater than %s (%f).",
                usdPrimPath.GetText(),
                UsdClipsAPIInfoKeys->templateEndTime.GetText(), 
                endTimeCode,
                UsdClipsAPIInfoKeys->templateStartTime.GetText(), 
                startTimeCode);
        return;
    }

    *clipTimes = VtVec2dArray();
    *clipActive = VtVec2dArray();
    *clipAssetPaths = VtArray<SdfAssetPath>();

    const SdfLayerRefPtr& sourceLayer =
        sourceLayerStack->GetLayers()[indexOfSourceLayer];
    const ArResolverContextBinder binder(
        sourceLayerStack->GetIdentifier().pathResolverContext);
    ArResolverScopedCache resolverScopedCache;
    auto& resolver = ArGetResolver();

    // XXX: We shift the value here into the integer range
    // to ensure consistency when incrementing by a stride
    // that is fractional. This does have the possibility of
    // chopping of large values with fractional components.
    constexpr size_t promotion = 10000;
    size_t clipActiveIndex = 0;

    // If we have an activeOffset, we author a knot on the front so users can query
    // at time t where t is the first sample - the active offset
    if (activeOffsetProvided) {
        const double promotedStart = startTimeCode*promotion;
        const double promotedOffset = std::abs(activeOffset)*promotion;
        const double clipTime = (promotedStart - promotedOffset)
                                /(double)promotion;
        (*clipTimes)->push_back(GfVec2d(clipTime, clipTime));
    }

    for (double t = startTimeCode * promotion;
                t <= endTimeCode*promotion; t += stride*promotion) {
    
        const double clipTime = t/(double)promotion;
        auto timeString = _DeriveClipTimeString(clipTime, numIntegerHashes,
                                                numDecimalHashes);
        tokenizedBasename[integerHashSectionIndex] = timeString.integerPortion;

        if (!timeString.decimalPortion.empty()) {
            tokenizedBasename[decimalHashSectionIndex] = timeString.decimalPortion;
        }

        auto filePath = SdfComputeAssetPathRelativeToLayer(sourceLayer,
            path + TfStringJoin(tokenizedBasename, "."));

        if (!resolver.Resolve(filePath).empty()) {
            (*clipAssetPaths)->push_back(SdfAssetPath(
                SdfLayer::CreateIdentifier(filePath, args)));

            (*clipTimes)->push_back(GfVec2d(clipTime, clipTime));
            if (activeOffsetProvided) {
                const double offsetTime = (t + (activeOffset*(double)promotion))
                                          /(double)promotion;
                (*clipActive)->push_back(GfVec2d(offsetTime, clipActiveIndex));
            } else {
                (*clipActive)->push_back(GfVec2d(clipTime, clipActiveIndex));
            }
            clipActiveIndex++;
        }
    }

    // If we have an offset, we author a knot on the end so users can query
    // at time t where t is the last sample + the active offset
    if (activeOffsetProvided) {
        const double promotedEnd = endTimeCode*promotion;
        const double promotedOffset = std::abs(activeOffset)*promotion;
        const double clipTime = (promotedEnd + promotedOffset)/(double)promotion;
        (*clipTimes)->push_back(GfVec2d(clipTime, clipTime));
    }

    _ClipDerivationMsg(
        UsdClipsAPIInfoKeys->assetPaths, **clipAssetPaths, usdPrimPath);
    _ClipDerivationMsg(
        UsdClipsAPIInfoKeys->times, **clipTimes, usdPrimPath);
    _ClipDerivationMsg(
        UsdClipsAPIInfoKeys->active, **clipActive, usdPrimPath);
}

namespace 
{
struct _ClipSet {
    explicit _ClipSet(const std::string& name_) : name(name_) { }

    // This info is anchored to the layer contributing clip asset paths
    // unless otherwise specified.
    struct _AnchorInfo {
        PcpLayerStackPtr layerStack;
        SdfPath primPath;
        size_t layerIndex;
        size_t layerStackOrder;

        // The layer index from which the base offset for this clipset is
        // computed.
        size_t offsetLayerIndex;
        // The layer index from which the offset to clipTimes is computed.
        // This may or may not exist, and may or may not differ from
        // offsetLayerIndex.
        size_t timesOffsetLayerIndex;

        // `offset` stores the offset to the layer expressing the strongest
        // clip assetPaths if the clip is templated, or the strongest clip
        // active if the clip is not.
        SdfLayerOffset offset;
        // `timesOffset` stores the offset to the layer expressing the
        // strongest clip times if it is expressed at all.
        SdfLayerOffset timesOffset;
    };
    _AnchorInfo anchorInfo;
    VtDictionary clipInfo;
    std::string name;
};
}

template <class T>
static bool
_SetInfo(const VtDictionary& dict, const TfToken& key, std::optional<T>* out)
{
    const VtValue* v = TfMapLookupPtr(dict, key.GetString());
    if (v && v->IsHolding<T>()) {
        *out = v->UncheckedGet<T>();
        return true;
    }
    return false;
}
    
template <class T>
static const T*
_GetInfo(const VtDictionary& dict, const TfToken& key)
{
    const VtValue* v = TfMapLookupPtr(dict, key.GetString());
    return v && v->IsHolding<T>() ? &v->UncheckedGet<T>() : nullptr;
}

static void
_RecordAnchorInfo(
    const PcpNodeRef& node, size_t layerIdx,
    const VtDictionary& clipInfo, _ClipSet* clipSet)
{
    // A clip set is anchored to the strongest site containing opinions
    // about asset paths.
    if (_GetInfo<VtArray<SdfAssetPath>>(
            clipInfo, UsdClipsAPIInfoKeys->assetPaths) ||
        _GetInfo<std::string>(clipInfo, 
            UsdClipsAPIInfoKeys->templateAssetPath)) {

        const SdfPath& path = node.GetPath();
        const PcpLayerStackRefPtr& layerStack = node.GetLayerStack();
        clipSet->anchorInfo = _ClipSet::_AnchorInfo {
            layerStack, path, layerIdx, 0, // This will get filled in later
            layerIdx, layerIdx,
            SdfLayerOffset(), SdfLayerOffset() // Offsets are computed later
        };
    }

    // The layer offset of non-templated clips is recorded relative to the
    // strongest authored clip active metadata. Record the layer index at
    // which active is found. Note that the offsetLayerIndex for
    // templated clips is the layer index at which templateAssetPath was
    // found above.
    if (_GetInfo<VtVec2dArray>(clipInfo, UsdClipsAPIInfoKeys->active) &&
        !_GetInfo<std::string>(clipSet->clipInfo,
            UsdClipsAPIInfoKeys->templateAssetPath)) {

        clipSet->anchorInfo.offsetLayerIndex = layerIdx;
    }

    // The layer offset to the strongest authored clip times metadata
    // is in very uncommon circumstances different from the offset to the
    // clip active/assetPaths metadata for non-templated and templated clips
    // respectively. Record the layer index at which times was found.
    if (_GetInfo<VtVec2dArray>(clipInfo, UsdClipsAPIInfoKeys->times)) {
        clipSet->anchorInfo.timesOffsetLayerIndex = layerIdx;
    }
}

static void
_ResolveClipSetsInNode(
    const PcpNodeRef& node,
    std::map<std::string, _ClipSet>* result)
{
    if (!node.HasValueClips()) {
        return;
    }

    const SdfPath& primPath = node.GetPath();
    const SdfLayerRefPtrVector& layers = node.GetLayerStack()->GetLayers();

    // Iterate from weak-to-strong to build up the composed clip info
    // dictionaries for each clip set, as well as the list of clip sets 
    // that should be added from this layer stack.
    std::map<std::string, _ClipSet> clipSetsInNode;
    std::vector<std::string> addedClipSets;
    for (size_t i = layers.size(); i-- != 0;) {
        const SdfLayerRefPtr& layer = layers[i];

        VtDictionary clips;
        if (layer->HasField(primPath, UsdTokens->clips, &clips)) {
            std::vector<std::string> clipSetsInLayer;
            clipSetsInLayer.reserve(clips.size());

            for (auto& entry : clips) {
                const std::string& clipSetName = entry.first;
                VtValue& clipInfoValue = entry.second;

                if (clipSetName.empty()) {
                    TF_WARN(
                        "Invalid unnamed clip set for prim <%s> "
                        "in 'clips' dictionary on spec @%s@<%s>", 
                        node.GetRootNode().GetPath().GetText(),
                        layer->GetIdentifier().c_str(), primPath.GetText());
                    continue;
                }

                if (!clipInfoValue.IsHolding<VtDictionary>()) {
                    TF_WARN(
                        "Expected dictionary for entry '%s' for prim "
                        "<%s> in 'clips' dictionary on spec @%s@<%s>", 
                        clipSetName.c_str(), 
                        node.GetRootNode().GetPath().GetText(),
                        layer->GetIdentifier().c_str(), primPath.GetText());
                    continue;
                }

                _ClipSet& clipSet = clipSetsInNode.emplace(
                    clipSetName, clipSetName).first->second;

                VtDictionary clipInfoForLayer;
                clipInfoValue.Swap(clipInfoForLayer);

                _RecordAnchorInfo(node, i, clipInfoForLayer, &clipSet);

                VtDictionaryOverRecursive(&clipInfoForLayer, clipSet.clipInfo);
                clipSet.clipInfo.swap(clipInfoForLayer);

                clipSetsInLayer.push_back(clipSetName);
            }

            // Treat clip sets specified in the clips dictionary as though
            // they were added in the clipSets list op so that users don't
            // have to explicitly author this. 
            //
            // Sort the clip sets lexicographically to ensure a stable
            // default sort order.
            std::sort(clipSetsInLayer.begin(), clipSetsInLayer.end());

            SdfStringListOp addListOp;
            addListOp.SetAddedItems(clipSetsInLayer);
            addListOp.ApplyOperations(&addedClipSets);
        }

        SdfStringListOp clipSetsListOp;
        if (layer->HasField(primPath, UsdTokens->clipSets, &clipSetsListOp)) {
            clipSetsListOp.ApplyOperations(&addedClipSets);
        }
    }

    for (auto it = clipSetsInNode.begin(); it != clipSetsInNode.end(); ) {
        // Filter out composed clip sets that aren't in the addedClipSets list.
        // This could be because they were deleted via the clipSets list op.
        auto addedIt = std::find(
            addedClipSets.begin(), addedClipSets.end(), it->first);
        if (addedIt == addedClipSets.end()) {
            it = clipSetsInNode.erase(it);
            continue;
        }

        _ClipSet::_AnchorInfo& info = it->second.anchorInfo;

        // If no anchor info is found, this clip set will be removed
        // later on.
        if (!info.layerStack) {
            ++it;
            continue;
        }
        info.layerStackOrder =
            std::distance(addedClipSets.begin(), addedIt);

        // Compute the base layer offset only once, after the entire
        // stack has been traversed.
        info.offset = _GetLayerOffsetToRoot(
            node, info.layerStack->GetLayers()[info.offsetLayerIndex]);

        // In the rare case that the clip times metadata has a different
        // layer offset than the clipset's offset, transform clip times.
        VtValue* v = TfMapLookupPtr(it->second.clipInfo,
                                    UsdClipsAPIInfoKeys->times);
        if (v && v->IsHolding<VtVec2dArray>() &&
                info.offsetLayerIndex
                != info.timesOffsetLayerIndex)
        {
            VtVec2dArray clipTimes;
            v->Swap(clipTimes);
            info.timesOffset = _GetLayerOffsetToRoot(
                node,
                info.layerStack->GetLayers()[info.timesOffsetLayerIndex]);
            _ApplyOffsetToTimes(info.offset, info.timesOffset, &clipTimes);
            v->Swap(clipTimes);
        }
        ++it;
    }

    result->swap(clipSetsInNode);
}

// ------------------------------------------------------------

void
Usd_ComputeClipSetDefinitionsForPrimIndex(
    const PcpPrimIndex& primIndex,
    std::vector<Usd_ClipSetDefinition>* clipSetDefinitions,
    std::vector<std::string>* clipSetNames)
{
    std::map<std::string, _ClipSet> composedClipSets;

    // Iterate over all nodes from strong to weak to compose all clip sets
    for (Usd_Resolver res(&primIndex); res.IsValid(); res.NextNode()) {
        std::map<std::string, _ClipSet> clipSetsInNode;
        _ResolveClipSetsInNode(res.GetNode(), &clipSetsInNode);

        for (const auto& entry : clipSetsInNode) {
            const std::string& clipSetName = entry.first;
            const _ClipSet& nodeClipSet = entry.second;

            _ClipSet& composedClipSet = composedClipSets.emplace(
                clipSetName, clipSetName).first->second;
            if (!composedClipSet.anchorInfo.layerStack) {
                composedClipSet.anchorInfo = nodeClipSet.anchorInfo;
            }
            VtDictionaryOverRecursive(
                &composedClipSet.clipInfo, nodeClipSet.clipInfo);
        }
    }

    // Remove all clip sets that have no anchor info; without anchor info,
    // value resolution won't know at which point to introduce these clip sets.
    for (auto it = composedClipSets.begin(); it != composedClipSets.end(); ) {
        if (!it->second.anchorInfo.layerStack) {
            it = composedClipSets.erase(it);
        }
        else {
            ++it;
        }
    }

    if (composedClipSets.empty()) {
        return;
    }

    // Collapse the composed clip sets into a sorted list to ensure 
    // ordering as specified by the clipSets list-op is taken into 
    // account.
    std::vector<_ClipSet> sortedClipSets;
    sortedClipSets.reserve(composedClipSets.size());
    for (auto& entry : composedClipSets) {
        sortedClipSets.emplace_back(std::move(entry.second));
    }
    std::sort(sortedClipSets.begin(), sortedClipSets.end(),
        [](const _ClipSet& x, const _ClipSet& y) {
            return std::tie(x.anchorInfo.layerStack, x.anchorInfo.primPath,
                            x.anchorInfo.layerStackOrder) <
                std::tie(y.anchorInfo.layerStack, y.anchorInfo.primPath,
                         y.anchorInfo.layerStackOrder);
        });

    // Unpack the information in the composed clip sets into individual
    // Usd_ResolvedClipInfo objects.
    clipSetDefinitions->reserve(sortedClipSets.size());
    if (clipSetNames) {
        clipSetNames->reserve(sortedClipSets.size());
    }

    for (const _ClipSet& clipSet : sortedClipSets) {
        clipSetDefinitions->push_back(Usd_ClipSetDefinition());
        Usd_ClipSetDefinition& out = clipSetDefinitions->back();

        if (clipSetNames) {
            clipSetNames->push_back(clipSet.name);
        }

        out.sourceLayerStack = clipSet.anchorInfo.layerStack;
        out.sourcePrimPath = clipSet.anchorInfo.primPath;
        out.indexOfLayerWhereAssetPathsFound = clipSet.anchorInfo.layerIndex;
        out.toStageOffset = clipSet.anchorInfo.offset;

        const VtDictionary& clipInfo = clipSet.clipInfo;
        _SetInfo(clipInfo, UsdClipsAPIInfoKeys->primPath, &out.clipPrimPath);
        _SetInfo(clipInfo, UsdClipsAPIInfoKeys->manifestAssetPath, 
                 &out.clipManifestAssetPath);
        _SetInfo(clipInfo, UsdClipsAPIInfoKeys->interpolateMissingClipValues,
                 &out.interpolateMissingClipValues);

        if (_SetInfo(clipInfo, UsdClipsAPIInfoKeys->assetPaths, 
                     &out.clipAssetPaths)) {
            _SetInfo(clipInfo, UsdClipsAPIInfoKeys->active, &out.clipActive);
            _SetInfo(clipInfo, UsdClipsAPIInfoKeys->times, &out.clipTimes);
        }
        else if (const std::string* templateAssetPath = _GetInfo<std::string>(
                     clipInfo, UsdClipsAPIInfoKeys->templateAssetPath)) {

            const double* templateActiveOffset = _GetInfo<double>(
                clipInfo, UsdClipsAPIInfoKeys->templateActiveOffset);
            const double* templateStride = _GetInfo<double>(
                clipInfo, UsdClipsAPIInfoKeys->templateStride);
            const double* templateStartTime = _GetInfo<double>(
                clipInfo, UsdClipsAPIInfoKeys->templateStartTime);
            const double* templateEndTime = _GetInfo<double>(
                clipInfo, UsdClipsAPIInfoKeys->templateEndTime);

            if (templateStride && templateStartTime && templateEndTime) {
                _DeriveClipInfo(
                    *templateAssetPath, *templateStride,
                    (templateActiveOffset ? *templateActiveOffset 
                                          : _DefaultClipOffsetValue),
                    *templateStartTime, *templateEndTime,
                    &out.clipTimes, &out.clipActive, &out.clipAssetPaths,
                    primIndex.GetPath(),
                    out.sourceLayerStack,
                    out.indexOfLayerWhereAssetPathsFound);

                auto sourceLayer = out.sourceLayerStack->GetLayers()[
                    out.indexOfLayerWhereAssetPathsFound];

                // Apply layer offsets to clipTimes afterwards
                // so that they don't affect the derived asset paths. Consumers
                // expect offsets to affect what clip is being used at a given
                // time, not the set of clips that are available.
                //
                // As a basis, we use the layer offset for the layer where the
                // template asset path pattern was found. Although the
                // start/end/stride values may be authored on different layers
                // with different offsets, this is an uncommon situation --
                // consumers usually author all clip metadata in the same layer
                // -- and it's not clear what the desired result in that case
                // would be anyway.
                if (out.clipTimes.has_value() &&
                        clipSet.anchorInfo.offsetLayerIndex
                        != clipSet.anchorInfo.timesOffsetLayerIndex)
                {

                    _ApplyOffsetToTimes(clipSet.anchorInfo.offset,
                                        clipSet.anchorInfo.timesOffset,
                                        &*out.clipTimes);
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
