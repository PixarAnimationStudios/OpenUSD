//
// Copyright 2020 Pixar
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
#include "pxr/usd/usd/clipSet.h"

#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/clipSetDefinition.h"
#include "pxr/usd/usd/debugCodes.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

static bool
_ValidateClipFields(
    const VtArray<SdfAssetPath>& clipAssetPaths,
    const std::string& clipPrimPath,
    const VtVec2dArray& clipActive,
    const VtVec2dArray* clipTimes,
    std::string* errMsg)
{
    // Note that we do allow empty clipAssetPath and clipActive data; 
    // this provides users with a way to 'block' clips specified in a 
    // weaker layer.
    if (clipPrimPath.empty()) {
        *errMsg = TfStringPrintf(
            "No clip prim path specified in '%s'",
            UsdClipsAPIInfoKeys->primPath.GetText());
        return false;
    }

    const size_t numClips = clipAssetPaths.size();

    // Each entry in the clipAssetPaths array is the asset path to a clip.
    for (const auto& clipAssetPath : clipAssetPaths) {
        if (clipAssetPath.GetAssetPath().empty()) {
            *errMsg = TfStringPrintf(
                "Empty clip asset path in '%s'",
                UsdClipsAPIInfoKeys->assetPaths.GetText());
            return false;
        }
    }

    // The 'clipPrimPath' field identifies a prim from which clip data
    // will be read.
    if (!SdfPath::IsValidPathString(clipPrimPath, errMsg)) {
        return false;
    }
    
    const SdfPath path(clipPrimPath);
    if (!(path.IsAbsolutePath() && path.IsPrimPath())) {
        *errMsg = TfStringPrintf(
            "Path '%s' in '%s' must be an absolute path to a prim",
            clipPrimPath.c_str(),
            UsdClipsAPIInfoKeys->primPath.GetText());
        return false;
    }

    // Each Vec2d in the 'clipActive' array is a (start frame, clip index)
    // tuple. Ensure the clip index points to a valid clip.
    for (const auto& startFrameAndClipIndex : clipActive) {
        if (startFrameAndClipIndex[1] < 0 ||
            startFrameAndClipIndex[1] >= numClips) {

            *errMsg = TfStringPrintf(
                "Invalid clip index %d in '%s'", 
                (int)startFrameAndClipIndex[1],
                UsdClipsAPIInfoKeys->active.GetText());
            return false;
        }
    }

    // Ensure that 'clipActive' does not specify multiple clips to be
    // active at the same time.
    typedef std::map<double, int> _ActiveClipMap;
    _ActiveClipMap activeClipMap;
    for (const auto& startFrameAndClipIndex : clipActive) {
        std::pair<_ActiveClipMap::iterator, bool> status = 
            activeClipMap.insert(std::make_pair(
                    startFrameAndClipIndex[0], startFrameAndClipIndex[1]));
        
        if (!status.second) {
            *errMsg = TfStringPrintf(
                "Clip %d cannot be active at time %.3f in '%s' because "
                "clip %d was already specified as active at this time.",
                (int)startFrameAndClipIndex[1],
                startFrameAndClipIndex[0],
                UsdClipsAPIInfoKeys->active.GetText(),
                status.first->second);
            return false;
        }
    }

    // Ensure there are at most two (stage time, clip time) entries in
    // clip times that have the same stage time.
    if (clipTimes) {
        typedef std::unordered_map<double, int> _StageTimesMap;
        _StageTimesMap stageTimesMap;
        for (const auto& stageTimeAndClipTime : *clipTimes) {
            int& numSeen = stageTimesMap.emplace(
                stageTimeAndClipTime[0], 0).first->second;
            numSeen += 1;

            if (numSeen > 2) {
                *errMsg = TfStringPrintf(
                    "Cannot have more than two entries in '%s' with the same "
                    "stage time (%.3f).",
                    UsdClipsAPIInfoKeys->times.GetText(),
                    stageTimeAndClipTime[0]);
                return false;
            }
        }
    }

    return true;
}

// ------------------------------------------------------------

Usd_ClipSetRefPtr
Usd_ClipSet::New(
    const Usd_ClipSetDefinition& clipDef,
    std::string* status)
{
    // If we haven't found all of the required clip metadata we can just 
    // bail out. Note that clipTimes and clipManifestAssetPath are *not* 
    // required.
    if (!clipDef.clipAssetPaths 
        || !clipDef.clipPrimPath 
        || !clipDef.clipActive) {
        return nullptr;
    }

    // XXX: Possibly want a better way to inform consumers of the error
    //      message..
    if (!_ValidateClipFields(
            *clipDef.clipAssetPaths, *clipDef.clipPrimPath, 
            *clipDef.clipActive, clipDef.clipTimes.get_ptr(), 
            status)) {
        return nullptr;
    }

    // The clip manifest is currently optional but can greatly improve
    // performance if specified. For debugging performance problems,
    // issue a message indicating if one hasn't been specified.
    if (!clipDef.clipManifestAssetPath) {
        *status = "No clip manifest specified. "
            "Performance may be improved if a manifest is specified.";
    }

    return Usd_ClipSetRefPtr(new Usd_ClipSet(clipDef));
}

namespace
{
struct Usd_ClipEntry {
public:
    double startTime;
    SdfAssetPath clipAssetPath;
    SdfPath clipPrimPath;
};
} // end anonymous namespace

Usd_ClipSet::Usd_ClipSet(
    const Usd_ClipSetDefinition& clipDef)
{
    // NOTE: Assumes definition has already been validated

    // If a clip manifest has been specified, create a clip for it.
    if (clipDef.clipManifestAssetPath) {
        const Usd_ClipRefPtr clip(new Usd_Clip(
            /* clipSourceLayerStack = */ clipDef.sourceLayerStack,
            /* clipSourcePrimPath   = */ clipDef.sourcePrimPath,
            /* clipSourceLayerIndex = */ 
                clipDef.indexOfLayerWhereAssetPathsFound,
            /* clipAssetPath        = */ *clipDef.clipManifestAssetPath,
            /* clipPrimPath         = */ SdfPath(*clipDef.clipPrimPath),
            /* clipStartTime        = */ Usd_ClipTimesEarliest,
            /* clipEndTime          = */ Usd_ClipTimesLatest,
            /* clipTimes            = */ Usd_Clip::TimeMappings()));
        manifestClip = clip;
    }

    // Generate a mapping of startTime -> clip entry. This allows us to
    // quickly determine the (startTime, endTime) for a given clip.
    typedef std::map<double, Usd_ClipEntry> _TimeToClipMap;
    _TimeToClipMap startTimeToClip;

    for (const auto& startFrameAndClipIndex : *clipDef.clipActive) {
        const double startFrame = startFrameAndClipIndex[0];
        const int clipIndex = (int)(startFrameAndClipIndex[1]);
        const SdfAssetPath& assetPath = (*clipDef.clipAssetPaths)[clipIndex];

        Usd_ClipEntry entry;
        entry.startTime = startFrame;
        entry.clipAssetPath = assetPath;
        entry.clipPrimPath = SdfPath(*clipDef.clipPrimPath);

        // Validation should have caused us to bail out if there were any
        // conflicting clip activations set.
        TF_VERIFY(startTimeToClip.insert(
                std::make_pair(entry.startTime, entry)).second);
    }

    // Generate the clip time mapping that applies to all clips.
    Usd_Clip::TimeMappings timeMapping;
    if (clipDef.clipTimes) {
        for (const auto& clipTime : *clipDef.clipTimes) {
            const Usd_Clip::ExternalTime extTime = clipTime[0];
            const Usd_Clip::InternalTime intTime = clipTime[1];
            timeMapping.push_back(Usd_Clip::TimeMapping(extTime, intTime));
        }
    }

    // Build up the final vector of clips.
    const _TimeToClipMap::const_iterator itBegin = startTimeToClip.begin();
    const _TimeToClipMap::const_iterator itEnd = startTimeToClip.end();

    _TimeToClipMap::const_iterator it = startTimeToClip.begin();
    while (it != itEnd) {
        const Usd_ClipEntry& clipEntry = it->second;

        const Usd_Clip::ExternalTime clipStartTime = 
            (it == itBegin ? Usd_ClipTimesEarliest : it->first);
        ++it;
        const Usd_Clip::ExternalTime clipEndTime = 
            (it == itEnd ? Usd_ClipTimesLatest : it->first);

        const Usd_ClipRefPtr clip(new Usd_Clip(
            /* clipSourceLayerStack = */ clipDef.sourceLayerStack,
            /* clipSourcePrimPath   = */ clipDef.sourcePrimPath,
            /* clipSourceLayerIndex = */ 
                clipDef.indexOfLayerWhereAssetPathsFound,
            /* clipAssetPath = */ clipEntry.clipAssetPath,
            /* clipPrimPath = */ clipEntry.clipPrimPath,
            /* clipStartTime = */ clipStartTime,
            /* clipEndTime = */ clipEndTime,
            /* clipTimes = */ timeMapping));

        valueClips.push_back(clip);
    }

    sourceLayerStack = clipDef.sourceLayerStack;
    sourcePrimPath = clipDef.sourcePrimPath;
    sourceLayerIndex = clipDef.indexOfLayerWhereAssetPathsFound;    
}

PXR_NAMESPACE_CLOSE_SCOPE
