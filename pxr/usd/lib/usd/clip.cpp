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
#include "pxr/usd/usd/clip.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/usdaFileFormat.h"

#include "pxr/base/tf/stringUtils.h"

#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>

#include <ostream>
#include <string>
#include <vector>

// XXX: Work around GCC 4.8's inability to see that we're using
//      boost::optional safely.  See GCC bug 47679.
#include "pxr/base/arch/defines.h"
#if ARCH_COMPILER_GCC_MAJOR == 4 && ARCH_COMPILER_GCC_MINOR == 8
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

bool
UsdIsClipRelatedField(const TfToken& fieldName)
{
    return std::find(UsdTokens->allTokens.begin(), 
                     UsdTokens->allTokens.end(), 
                     fieldName) != UsdTokens->allTokens.end();
}

struct Usd_SortByExternalTime
{
    bool 
    operator()(const Usd_Clip::TimeMapping& x, 
               const Usd_Clip::ExternalTime y) const
    { 
        return x.externalTime < y; 
    }

    bool 
    operator()(const Usd_Clip::TimeMapping& x, 
               const Usd_Clip::TimeMapping& y) const
    { 
        return x.externalTime < y.externalTime; 
    }
};

std::ostream&
operator<<(std::ostream& out, const Usd_ClipRefPtr& clip)
{
    out << TfStringPrintf(
        "%s<%s> (start: %s end: %s)",
        TfStringify(clip->assetPath).c_str(),
        clip->primPath.GetString().c_str(),
        (clip->startTime == Usd_ClipTimesEarliest ?
            "-inf" : TfStringPrintf("%.3f", clip->startTime).c_str()),
        (clip->endTime == Usd_ClipTimesLatest ? 
            "inf" : TfStringPrintf("%.3f", clip->endTime).c_str()));
    return out;
}

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
_ApplyLayerOffsetToExternalTimes(
    const SdfLayerOffset& layerOffset,
    VtVec2dArray* array)
{
    if (layerOffset.IsIdentity()) {
        return;
    }

    const SdfLayerOffset inverse = layerOffset.GetInverse();
    for (auto& time : *array) {
        time[0] = inverse * time[0]; 
    }
}

static void
_ClipDebugMsg(const PcpNodeRef& node,
              const SdfLayerRefPtr& layer,
              const TfToken& metadataName,
              const bool derived=false) 
{
    TF_DEBUG(USD_CLIPS).Msg(
        "%s for prim <%s> %s in LayerStack %s "
        "at spec @%s@<%s>\n",
        metadataName.GetText(),
        node.GetRootNode().GetPath().GetString().c_str(),
        (derived? "derived" : "found"),
        TfStringify(node.GetLayerStack()).c_str(),
        layer->GetIdentifier().c_str(), 
        node.GetPath().GetString().c_str());
}

template <typename V>
static void
_ClipDerivationMsg(const TfToken& metadataName,
                   const PcpNodeRef& node,
                   const V& v)
{
    TF_DEBUG(USD_CLIPS).Msg(
        "%s for prim <%s> derived: %s",
        metadataName.GetText(),
        node.GetRootNode().GetPath().GetText(),
        TfStringify(v).c_str());
}

namespace {
    struct _ClipTimeString {
        std::string integerPortion;
        std::string decimalPortion;
    };
}

_ClipTimeString
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

void
_DeriveClipInfo(const std::string& templateAssetPath,
                const double stride,
                const double startTimeCode,
                const double endTimeCode,
                boost::optional<VtVec2dArray>* clipTimes,
                boost::optional<VtVec2dArray>* clipActive,
                boost::optional<VtArray<SdfAssetPath>>* clipAssetPaths,
                const PcpNodeRef& node,
                const SdfLayerHandle& layer,
                const PcpLayerStackPtr& layerStack)
{
    auto path = TfGetPathName(templateAssetPath);
    auto basename = TfGetBaseName(templateAssetPath);
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
        TF_WARN("Invalid template string specified %s, must be "
                "of the form path/basename.###.usd or "
                "path/basename.###.###.usd. Note that the number "
                "of hash marks is variable in each group.", 
                templateAssetPath.c_str());
        return;
    }

    if (startTimeCode > endTimeCode) {
        TF_WARN("Invalid range specified in template clip metadata. "
                "clipTemplateEndTime (%f) cannot be greater than "
                "clipTemplateStartTime (%f).", 
                endTimeCode, 
                startTimeCode);
        return;
    }

    *clipTimes = VtVec2dArray();
    *clipActive = VtVec2dArray();
    *clipAssetPaths = VtArray<SdfAssetPath>();

    const ArResolverContextBinder binder(
        layerStack->GetIdentifier().pathResolverContext);
    ArResolverScopedCache resolverScopedCache;
    auto& resolver = ArGetResolver();

    // XXX: We shift the value here into the integer range 
    // to ensure consistency when incrementing by a stride
    // that is fractional. This does have the possibility of 
    // chopping of large values with fractional components.
    const size_t promotion = 10000;
    size_t clipActiveIndex = 0; 
    
    for (double t = startTimeCode * promotion;
         t <= endTimeCode*promotion; 
         t += stride*promotion) 
    {
        double clipTime = t/(double)promotion; 
        auto timeString = _DeriveClipTimeString(clipTime, numIntegerHashes,
                                                numDecimalHashes);

        tokenizedBasename[integerHashSectionIndex] = timeString.integerPortion;

        if (!timeString.decimalPortion.empty()) {
            tokenizedBasename[decimalHashSectionIndex] = timeString.decimalPortion;
        }

        auto filePath = SdfComputeAssetPathRelativeToLayer(layer, 
            path + TfStringJoin(tokenizedBasename, "."));

        if (!resolver.Resolve(filePath).empty()) {
            (*clipAssetPaths)->push_back(SdfAssetPath(filePath));
            (*clipTimes)->push_back(GfVec2d(clipTime, clipTime));
            (*clipActive)->push_back(GfVec2d(clipTime, clipActiveIndex));
            clipActiveIndex++;
        }
    }

    _ClipDerivationMsg(UsdTokens->clipAssetPaths, node, **clipAssetPaths);
    _ClipDerivationMsg(UsdTokens->clipTimes, node, **clipTimes);
    _ClipDerivationMsg(UsdTokens->clipActive, node, **clipActive);
}

void
Usd_ResolveClipInfo(
    const PcpNodeRef& node,
    Usd_ResolvedClipInfo* clipInfo)
{
    const SdfPath& primPath = node.GetPath();
    const PcpLayerStackPtr& layerStack = node.GetLayerStack();
    const SdfLayerRefPtrVector& layers = layerStack->GetLayers();

    bool nontemplateMetadataSeen = false;
    bool templateMetadataSeen    = false;

    boost::optional<double> templateStartTime;
    boost::optional<double> templateEndTime;
    boost::optional<double> templateStride;
    boost::optional<std::string> templateAssetPath;

    for (size_t i = 0, j = layers.size(); i != j; ++i) {
        const SdfLayerRefPtr& layer = layers[i];
        VtArray<SdfAssetPath> clipAssetPaths;
        if (layer->HasField(primPath, UsdTokens->clipAssetPaths, 
                            &clipAssetPaths)){
            nontemplateMetadataSeen = true;
            _ClipDebugMsg(node, layer, UsdTokens->clipAssetPaths);
            clipInfo->indexOfLayerWhereAssetPathsFound = i;
            clipInfo->clipAssetPaths = boost::in_place();
            clipInfo->clipAssetPaths->swap(clipAssetPaths);
            break;
        }

        std::string clipTemplateAssetPath;
        if (layer->HasField(primPath, UsdTokens->clipTemplateAssetPath,
                            &clipTemplateAssetPath)) {
            templateMetadataSeen = true;
            clipInfo->indexOfLayerWhereAssetPathsFound = i;
            templateAssetPath = clipTemplateAssetPath;
            break;
        }

        if (templateMetadataSeen && nontemplateMetadataSeen) {
            TF_WARN("Both template and non-template clip metadata are "
                    "authored for prim <%s> in layerStack %s "
                    "at spec @%s@<%s>",
                    primPath.GetText(),
                    TfStringify(layerStack).c_str(),
                    layer->GetIdentifier().c_str(),
                    node.GetPath().GetString().c_str());
        }
    }

    // we need not complete resolution if there are no clip
    // asset paths available, as they are a necessary component for clips.
    if (!templateMetadataSeen && !nontemplateMetadataSeen) {
        return;
    }


    // Compose the various pieces of clip metadata; iterate the LayerStack
    // from strong-to-weak and save the strongest opinion.
    for (size_t i = 0, j = layers.size(); i != j; ++i) {
        const SdfLayerRefPtr& layer = layers[i];

        if (!clipInfo->clipManifestAssetPath) {
            SdfAssetPath clipManifestAssetPath;
            if (layer->HasField(primPath, UsdTokens->clipManifestAssetPath, 
                                &clipManifestAssetPath)) {
                _ClipDebugMsg(node, layer, UsdTokens->clipManifestAssetPath);
                clipInfo->clipManifestAssetPath = clipManifestAssetPath;
            }
        }

        if (!clipInfo->clipPrimPath) {
            std::string clipPrimPath;
            if (layer->HasField(primPath, UsdTokens->clipPrimPath, 
                                &clipPrimPath)) {
                _ClipDebugMsg(node, layer, UsdTokens->clipPrimPath);
                clipInfo->clipPrimPath = boost::in_place();
                clipInfo->clipPrimPath->swap(clipPrimPath);
            }
        }

        if (nontemplateMetadataSeen) {
            if (!clipInfo->clipActive) {
                VtVec2dArray clipActive;
                if (layer->HasField(primPath, UsdTokens->clipActive, 
                                    &clipActive)) {
                    _ClipDebugMsg(node, layer, UsdTokens->clipActive);
                    _ApplyLayerOffsetToExternalTimes(
                        _GetLayerOffsetToRoot(node, layer), &clipActive);
                    clipInfo->clipActive = boost::in_place();
                    clipInfo->clipActive->swap(clipActive);
                }
            }

            if (!clipInfo->clipTimes) {
                VtVec2dArray clipTimes;
                if (layer->HasField(primPath, UsdTokens->clipTimes, 
                                    &clipTimes)) {
                    _ClipDebugMsg(node, layer, UsdTokens->clipTimes);
                    _ApplyLayerOffsetToExternalTimes(
                        _GetLayerOffsetToRoot(node, layer), &clipTimes);
                    clipInfo->clipTimes = boost::in_place();
                    clipInfo->clipTimes->swap(clipTimes);
                }
            }
        } else {
            if (!templateStride) {
                double clipTemplateStride;
                if (layer->HasField(primPath, UsdTokens->clipTemplateStride,
                                    &clipTemplateStride)) {
                    _ClipDebugMsg(node, layer, UsdTokens->clipTemplateStride); 
                    auto layerOffset = _GetLayerOffsetToRoot(node, layer);
                    layerOffset.SetOffset(0);
                    templateStride = layerOffset * clipTemplateStride;
                }
            }

            if (!templateStartTime) {
                double clipTemplateStartTime;
                if (layer->HasField(primPath, UsdTokens->clipTemplateStartTime,
                                    &clipTemplateStartTime)) {
                    _ClipDebugMsg(node, layer, UsdTokens->clipTemplateStartTime); 
                    auto layerOffset = _GetLayerOffsetToRoot(node, layer);
                    templateStartTime = layerOffset * clipTemplateStartTime;
                }
            }

            if (!templateEndTime) {
                double clipTemplateEndTime;
                if (layer->HasField(primPath, UsdTokens->clipTemplateEndTime,
                                    &clipTemplateEndTime)) {
                    _ClipDebugMsg(node, layer, UsdTokens->clipTemplateEndTime); 
                    auto layerOffset = _GetLayerOffsetToRoot(node, layer);
                    templateEndTime  = layerOffset * clipTemplateEndTime;
                }
            }

            if (templateStride && templateStartTime && templateEndTime) {
                _DeriveClipInfo(*templateAssetPath, *templateStride,
                                *templateStartTime, *templateEndTime,
                                &clipInfo->clipTimes, &clipInfo->clipActive,
                                &clipInfo->clipAssetPaths, node, 
                                layers[clipInfo->indexOfLayerWhereAssetPathsFound],
                                layerStack);
                break;
            }
        }
    }
}

// ------------------------------------------------------------

Usd_Clip::Usd_Clip()
    : startTime(0)
    , endTime(0)
    , _hasLayer(false)
{ 
}

Usd_Clip::Usd_Clip(
    const PcpNodeRef& clipSourceNode,
    size_t clipSourceLayerIndex,
    const SdfAssetPath& clipAssetPath,
    const SdfPath& clipPrimPath,
    ExternalTime clipStartTime,
    ExternalTime clipEndTime,
    const TimeMappings& timeMapping)
    : sourceNode(clipSourceNode)
    , sourceLayerIndex(clipSourceLayerIndex)
    , assetPath(clipAssetPath)
    , primPath(clipPrimPath)
    , startTime(clipStartTime)
    , endTime(clipEndTime)
    , times(timeMapping)
{ 
    // Sort the time mappings and add sentinel values to the beginning and
    // end for convenience in other functions.
    if (not times.empty()) {
        std::sort(times.begin(), times.end(), Usd_SortByExternalTime());
        times.insert(times.begin(), times.front());
        times.insert(times.end(), times.back());
    }

    // For performance reasons, we want to defer the loading of the layer
    // for this clip until absolutely needed. However, if the layer happens
    // to already be opened, we can take advantage of that here. 
    //
    // This is important for change processing. Clip layers will be kept
    // alive during change processing, so any clips that are reconstructed
    // will have the opportunity to reuse the already-opened layer.
    const PcpLayerStackPtr& layerStack = sourceNode.GetLayerStack();
    if (TF_VERIFY(sourceLayerIndex < layerStack->GetLayers().size())) {
        const ArResolverContextBinder binder(
            layerStack->GetIdentifier().pathResolverContext);
        _layer = SdfLayer::FindRelativeToLayer(
            layerStack->GetLayers()[sourceLayerIndex],
            assetPath.GetAssetPath());
    }

    _hasLayer = (bool)_layer;
}

// Helper function to determine the linear segment in the given
// time mapping that applies to the given time.
static bool
_GetBracketingTimeSegment(
    const Usd_Clip::TimeMappings& times,
    Usd_Clip::ExternalTime time,
    size_t* m1, size_t* m2)
{
    if (times.empty()) {
        return false;
    }
    
    // This relies on the Usd_Clip c'tor inserting sentinel values at the
    // beginning and end of the TimeMappings object. Consumers rely on this
    // function never returning m1 == m2.
    if (time <= times.front().externalTime) {
        *m1 = 0;
        *m2 = 1;
    }
    else if (time >= times.back().externalTime) {
        *m1 = times.size() - 2;
        *m2 = times.size() - 1;
    }
    else {
        typedef Usd_Clip::TimeMappings::const_iterator _Iterator;
        _Iterator lowerBound = std::lower_bound(times.begin(), times.end(),
            time, Usd_SortByExternalTime());
        *m2 = std::distance(times.begin(), lowerBound);
        *m1 = *m2 - 1;
    }

    TF_VERIFY(*m1 < *m2);
    TF_VERIFY(0 <= *m1 and *m1 < times.size());
    TF_VERIFY(0 <= *m2 and *m2 < times.size());
    
    return true;
}

static bool
_GetBracketingTimeSegment(
    const Usd_Clip::TimeMappings& times,
    Usd_Clip::ExternalTime time,
    Usd_Clip::TimeMapping* m1, Usd_Clip::TimeMapping* m2)
{
    size_t index1, index2;
    if (not _GetBracketingTimeSegment(times, time, &index1, &index2)) {
        return false;
    }

    *m1 = times[index1];
    *m2 = times[index2];
    return true;
}

bool 
Usd_Clip::GetBracketingTimeSamplesForPath(
    const SdfAbstractDataSpecId& id, ExternalTime time, 
    ExternalTime* tLower, ExternalTime* tUpper) const
{
    const SdfLayerRefPtr& clip = _GetLayerForClip();
    const _TranslatedSpecId idInClip = _TranslateIdToClip(id);
    const InternalTime timeInClip = _TranslateTimeToInternal(time);

    InternalTime lowerInClip, upperInClip;
    if (not clip->GetBracketingTimeSamplesForPath(
            idInClip.id, timeInClip, &lowerInClip, &upperInClip)) { 
        return false;
    }

    // Need to translate the time samples in the internal time domain
    // to the external time domain. The external -> internal mapping
    // is many-to-one; a given internal time could translate to multiple
    // external times. We need to look for the translation that is closest
    // to the time we were given.
    //
    // An example case:
    //
    // int. time
    //  -
    //  |
    //  |                     m3    m1, m2, m3 are mappings in the times vector
    //  |                    ,*     s1, s2 are time samples in the clip
    // s2..................,'      
    //  |                ,'.
    // i0..............,'  .
    //  |            ,'.   .
    //  |          ,*  .   .
    // s1........,' m2 .   .
    //  |      ,'      .   .
    //  |    ,' .      .   .
    //  |   *   .      .   .
    //  | m1    .      .   .
    //  |-------.------.---.------| ext. time
    //          e1     e0  e2
    // 
    // Suppose we are asked for bracketing samples at external time t0.
    // We map this into the internal time domain, which gives us i0. The
    // bracketing samples for i0 in the internal domain are (s1, s2). 
    // 
    // Now we need to map these back to the external domain. The bracketing
    // time segment for e0 is (m2, m3). s1 is not in the range of this segment,
    // so we walk backwards to the previous segment (m1, m2). s1 *is* in the
    // range of this segment, so we use these mappings to map s1 to e1. For
    // s2, since s2 is in the range of (m2, m3), we use those mappings to map
    // s2 to e2. So, our final answer is (e1, e2).
    size_t m1, m2;
    if (not _GetBracketingTimeSegment(times, time, &m1, &m2)) {
        *tLower = lowerInClip;
        *tUpper = upperInClip;
        return true;
    }

    boost::optional<ExternalTime> translatedLower, translatedUpper;
    auto _CanTranslate = [&time, &upperInClip, &lowerInClip, this, 
                          &translatedLower, &translatedUpper](
                                            const TimeMapping& map1, 
                                            const TimeMapping& map2, 
                                            const bool translatingLower) {
        const double timeInClip = translatingLower ? lowerInClip : upperInClip;
        auto& translated = translatingLower ? translatedLower : translatedUpper;

        const double lower = std::min(map1.internalTime, map2.internalTime);
        const double upper = std::max(map1.internalTime, map2.internalTime);

        if (lower <= timeInClip and timeInClip <= upper) {
            if (map1.internalTime != map2.internalTime) {
                translated.reset(
                    this->_TranslateTimeToExternal(timeInClip, map1, map2));
            } else {
                const bool lowerUpperMatch = (lowerInClip == upperInClip);
                if (lowerUpperMatch && time == map1.externalTime) {
                    translated.reset(map1.externalTime);
                } else if (lowerUpperMatch && time == map2.externalTime) {
                    translated.reset(map2.externalTime);
                } else {
                    if (translatingLower) {
                        translated.reset(map1.externalTime);
                    } else {
                        translated.reset(map2.externalTime);
                    }
                }
            }
        }
        return static_cast<bool>(translated);
    };

    for (int i1 = m1, i2 = m2; i1 >= 0 and i2 >= 0; --i1, --i2) {
         if (_CanTranslate(times[i1], times[i2], /*lower=*/true)) { break; }
    }
        
    for (size_t i1 = m1, i2 = m2, sz = times.size(); i1 < sz and i2 < sz; ++i1, ++i2) {
         if (_CanTranslate(times[i1], times[i2], /*lower=*/false)) { break; }
    }

    if (translatedLower and not translatedUpper) {
        translatedUpper = translatedLower;
    }
    else if (not translatedLower and translatedUpper) {
        translatedLower = translatedUpper;
    }
    else if (not translatedLower and not translatedUpper) {
        // If we haven't been able to translate either internal time, it's
        // because they are outside the range of the clip time mappings. We
        // clamp them to the nearest external time to match the behavior of
        // SdfLayer::GetBracketingTimeSamples.
        //
        // The issue here is that the clip may not have a sample at these
        // times. Usd_Clip::QueryTimeSample does a secondary step of finding
        // the corresponding time sample if it determines this is the case.
        //
        // The 'timingOutsideClip' test case in testUsdModelClips exercises
        // this behavior.
        if (lowerInClip < times.front().internalTime) {
            translatedLower.reset(times.front().externalTime);
        }
        else if (lowerInClip > times.back().internalTime) {
            translatedLower.reset(times.back().externalTime);
        }

        if (upperInClip < times.front().internalTime) {
            translatedUpper.reset(times.front().externalTime);
        }
        else if (upperInClip > times.back().internalTime) {
            translatedUpper.reset(times.back().externalTime);
        }
    }
            
    *tLower = *translatedLower;
    *tUpper = *translatedUpper;
    return true;
}

size_t
Usd_Clip::GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    return _GetLayerForClip()->GetNumTimeSamplesForPath(
        _TranslateIdToClip(id).id);
}

std::set<Usd_Clip::ExternalTime>
Usd_Clip::ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const
{
    const std::set<InternalTime> timeSamplesInClip = 
        _GetLayerForClip()->ListTimeSamplesForPath(_TranslateIdToClip(id).id);
    if (times.empty()) {
        return timeSamplesInClip;
    }

    std::set<ExternalTime> timeSamples;

    // We need to convert the internal time samples to the external
    // domain using the clip's time mapping. This is tricky because the
    // mapping is many-to-one: multiple external times may map to the
    // same internal time, e.g. mapping { 0:5, 5:10, 10:5 }. 
    //
    // To deal with this, every internal time sample has to be checked 
    // against the entire mapping function.
    for (InternalTime t: timeSamplesInClip) {
        for (size_t i = 0; i < times.size() - 1; ++i) {
            const TimeMapping& m1 = times[i];
            const TimeMapping& m2 = times[i+1];

            if (m1.internalTime <= t and t <= m2.internalTime) {
                if (m1.internalTime == m2.internalTime) {
                    timeSamples.insert(m1.externalTime);
                    timeSamples.insert(m2.externalTime);
                }
                else {
                    timeSamples.insert(_TranslateTimeToExternal(t, m1, m2));
                }
            }
        }
    }

    // If none of the time samples have been mapped, it's because they're
    // all outside the range of the clip time mappings. In that case, we
    // apply the same clamping behavior as GetBracketingTimeSamples to
    // maintain consistency.
    if (timeSamples.empty()) {
        for (InternalTime t: timeSamplesInClip) {
            if (t < times.front().internalTime) {
                timeSamples.insert(times.front().externalTime);
            }
            else if (t > times.back().internalTime) {
                timeSamples.insert(times.back().externalTime);
            }
        }
    }

    return timeSamples;
}

bool 
Usd_Clip::HasField(const SdfAbstractDataSpecId& id, const TfToken& field) const
{
    return _GetLayerForClip()->HasField(_TranslateIdToClip(id).id, field);
}

Usd_Clip::_TranslatedSpecId 
Usd_Clip::_TranslateIdToClip(const SdfAbstractDataSpecId& id) const
{
    return _TranslatedSpecId(
        id.GetPropertyOwningSpecPath()
            .ReplacePrefix(sourceNode.GetPath(), primPath),
        id.GetPropertyName());
}

Usd_Clip::InternalTime
Usd_Clip::_TranslateTimeToInternal(ExternalTime extTime) const
{
    if (times.empty()) {
        return extTime;
    }

    TimeMapping m1, m2;
    _GetBracketingTimeSegment(times, extTime, &m1, &m2);

    // Early out in some special cases to avoid unnecessary
    // math operations that could introduce precision issues.
    if (m1.externalTime == m2.externalTime) {
        return m1.internalTime;
    }
    else if (extTime == m1.externalTime) {
        return m1.internalTime;
    }
    else if (extTime == m2.externalTime) {
        return m2.internalTime;
    }

    return (m2.internalTime - m1.internalTime) /
           (m2.externalTime - m1.externalTime)
        * (extTime - m1.externalTime)
        + m1.internalTime;
}

Usd_Clip::ExternalTime
Usd_Clip::_TranslateTimeToExternal(
    InternalTime intTime, TimeMapping m1, TimeMapping m2) const
{
    // Early out in some special cases to avoid unnecessary
    // math operations that could introduce precision issues.
    if (m1.internalTime == m2.internalTime) {
        return m1.externalTime;
    }
    else if (intTime == m1.internalTime) {
        return m1.externalTime;
    }
    else if (intTime == m2.internalTime) {
        return m2.externalTime;
    }

    return (m2.externalTime - m1.externalTime) / 
           (m2.internalTime - m1.internalTime)
        * (intTime - m1.internalTime)
        + m1.externalTime;
}

SdfPropertySpecHandle
Usd_Clip::GetPropertyAtPath(const SdfAbstractDataSpecId &id) const
{
    const auto path = _TranslateIdToClip(id).id.GetFullSpecPath();
    return _GetLayerForClip()->GetPropertyAtPath(path);
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (dummy_clip)
    ((dummy_clipFormat, "dummy_clip.%s"))
    );

SdfLayerRefPtr
Usd_Clip::_GetLayerForClip() const
{
    if (_hasLayer) {
        return _layer; 
    }

    SdfLayerRefPtr layer;

    const PcpLayerStackPtr& layerStack = sourceNode.GetLayerStack();
    if (TF_VERIFY(sourceLayerIndex < layerStack->GetLayers().size())) {
        std::string resolvedPath = assetPath.GetAssetPath();
        const ArResolverContextBinder binder(
            layerStack->GetIdentifier().pathResolverContext);
        layer = SdfFindOrOpenRelativeToLayer(
            layerStack->GetLayers()[sourceLayerIndex],
            &resolvedPath);
    }

    if (not layer) {
        // If we failed to open the specified layer, report an error
        // and use a dummy anonymous layer instead, to avoid having
        // to check layer validity everywhere and to avoid reissuing
        // this error.
        // XXX: Better way to report this error?
        TF_WARN("Unable to open clip layer @%s@", 
                assetPath.GetAssetPath().c_str());
        layer = SdfLayer::CreateAnonymous(TfStringPrintf(
                     _tokens->dummy_clipFormat.GetText(), 
                     UsdUsdaFileFormatTokens->Id.GetText()));
    }

    std::lock_guard<std::mutex> lock(_layerMutex);
    if (not _layer) { 
        _layer = layer;
        _hasLayer = true;
    }

    return _layer;
}

SdfLayerHandle
Usd_Clip::GetLayerIfOpen() const
{
    if (_hasLayer){
        return TfStringStartsWith(_layer->GetIdentifier(), 
                                  _tokens->dummy_clip.GetString()) ?
            SdfLayerHandle() : SdfLayerHandle(_layer);
    }

    return SdfLayerHandle();
}
