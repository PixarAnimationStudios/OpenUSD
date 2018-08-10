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
#include "pxr/usd/usd/clipCache.h"

#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


Usd_ClipCache::Usd_ClipCache()
{
}

Usd_ClipCache::~Usd_ClipCache()
{
}

struct Usd_ClipEntry {
public:
    double startTime;
    SdfAssetPath clipAssetPath;
    SdfPath clipPrimPath;
};

static bool
_ValidateClipFields(
    const VtArray<SdfAssetPath>& clipAssetPaths,
    const std::string& clipPrimPath,
    const VtVec2dArray& clipActive,
    std::string* errMsg)
{
    // Note that we do allow empty clipAssetPath and clipActive data; 
    // this provides users with a way to 'block' clips specified in a 
    // weaker layer.
    if (clipPrimPath.empty()) {
        *errMsg = "No clip prim path specified";
        return false;
    }

    const size_t numClips = clipAssetPaths.size();

    // Each entry in the 'clipAssetPaths' array is the asset path to a clip.
    for (const auto& clipAssetPath : clipAssetPaths) {
        if (clipAssetPath.GetAssetPath().empty()) {
            *errMsg = TfStringPrintf("Empty clip asset path in metadata '%s'",
                                     UsdTokens->clipAssetPaths.GetText());
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
            "Path '%s' in metadata '%s' must be an absolute path to a prim",
            clipPrimPath.c_str(),
            UsdTokens->clipPrimPath.GetText());
        return false;
    }

    // Each Vec2d in the 'clipActive' array is a (start frame, clip index)
    // tuple. Ensure the clip index points to a valid clip.
    for (const auto& startFrameAndClipIndex : clipActive) {
        if (startFrameAndClipIndex[1] < 0 ||
            startFrameAndClipIndex[1] >= numClips) {

            *errMsg = TfStringPrintf(
                "Invalid clip index %d in metadata '%s'", 
                (int)startFrameAndClipIndex[1],
                UsdTokens->clipActive.GetText());
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
                "Clip %d cannot be active at time %.3f in metadata '%s' "
                "because clip %d was already specified as active at this time.",
                (int)startFrameAndClipIndex[1],
                startFrameAndClipIndex[0],
                UsdTokens->clipActive.GetText(),
                status.first->second);
            return false;
        }
    }

    return true;
}

static void
_AddClipsFromClipInfo(
    const SdfPath& usdPrimPath, 
    const Usd_ResolvedClipInfo& clipInfo, Usd_ClipCache::Clips* clips)
{
    // If we haven't found all of the required clip metadata we can just 
    // bail out. Note that clipTimes and clipManifestAssetPath are *not* 
    // required.
    if (!clipInfo.clipAssetPaths 
        || !clipInfo.clipPrimPath 
        || !clipInfo.clipActive) {
        return;
    }

    // The clip manifest is currently optional but can greatly improve
    // performance if specified. For debugging performance problems,
    // issue a message indicating if one hasn't been specified.
    if (!clipInfo.clipManifestAssetPath) {
        TF_DEBUG(USD_CLIPS).Msg(
            "No clip manifest specified for prim <%s>. " 
            "Performance may be improved if a manifest is specified.\n",
            usdPrimPath.GetText());
    }

    // XXX: Possibly want a better way to inform consumers of the error
    //      message..
    std::string error;
    if (!_ValidateClipFields(
            *clipInfo.clipAssetPaths, *clipInfo.clipPrimPath, 
            *clipInfo.clipActive, &error)) {

        TF_WARN(
            "Invalid clips specified for prim <%s>: %s",
            clipInfo.sourcePrimPath.GetString().c_str(),
            error.c_str());
        return;
    }

    // If a clip manifest has been specified, create a clip for it.
    if (clipInfo.clipManifestAssetPath) {
        const Usd_ClipRefPtr clip(new Usd_Clip(
            /* clipSourceLayerStack = */ clipInfo.sourceLayerStack,
            /* clipSourcePrimPath   = */ clipInfo.sourcePrimPath,
            /* clipSourceLayerIndex = */ 
                clipInfo.indexOfLayerWhereAssetPathsFound,
            /* clipAssetPath        = */ *clipInfo.clipManifestAssetPath,
            /* clipPrimPath         = */ SdfPath(*clipInfo.clipPrimPath),
            /* clipStartTime        = */ Usd_ClipTimesEarliest,
            /* clipEndTime          = */ Usd_ClipTimesLatest,
            /* clipTimes            = */ Usd_Clip::TimeMappings()));
        clips->manifestClip = clip;
    }

    // Generate a mapping of startTime -> clip entry. This allows us to
    // quickly determine the (startTime, endTime) for a given clip.
    typedef std::map<double, Usd_ClipEntry> _TimeToClipMap;
    _TimeToClipMap startTimeToClip;

    for (const auto& startFrameAndClipIndex : *clipInfo.clipActive) {
        const double startFrame = startFrameAndClipIndex[0];
        const int clipIndex = (int)(startFrameAndClipIndex[1]);
        const SdfAssetPath& assetPath = (*clipInfo.clipAssetPaths)[clipIndex];

        Usd_ClipEntry entry;
        entry.startTime = startFrame;
        entry.clipAssetPath = assetPath;
        entry.clipPrimPath = SdfPath(*clipInfo.clipPrimPath);

        // Validation should have caused us to bail out if there were any
        // conflicting clip activations set.
        TF_VERIFY(startTimeToClip.insert(
                std::make_pair(entry.startTime, entry)).second);
    }

    // Generate the clip time mapping that applies to all clips.
    Usd_Clip::TimeMappings timeMapping;
    if (clipInfo.clipTimes) {
        for (const auto& clipTime : *clipInfo.clipTimes) {
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
            /* clipSourceLayerStack = */ clipInfo.sourceLayerStack,
            /* clipSourcePrimPath   = */ clipInfo.sourcePrimPath,
            /* clipSourceLayerIndex = */ 
                clipInfo.indexOfLayerWhereAssetPathsFound,
            /* clipAssetPath = */ clipEntry.clipAssetPath,
            /* clipPrimPath = */ clipEntry.clipPrimPath,
            /* clipStartTime = */ clipStartTime,
            /* clipEndTime = */ clipEndTime,
            /* clipTimes = */ timeMapping));

        clips->valueClips.push_back(clip);
    }

    clips->sourceLayerStack = clipInfo.sourceLayerStack;
    clips->sourcePrimPath = clipInfo.sourcePrimPath;
    clips->sourceLayerIndex = clipInfo.indexOfLayerWhereAssetPathsFound;    
}

static void
_AddClipsFromPrimIndex(
    const PcpPrimIndex& primIndex, 
    std::vector<Usd_ClipCache::Clips>* clips)
{
    std::vector<Usd_ResolvedClipInfo> clipInfo;
    if (!Usd_ResolveClipInfo(primIndex, &clipInfo)) {
        return;
    }

    for (const Usd_ResolvedClipInfo& entry : clipInfo) {
        Usd_ClipCache::Clips entryClips;
        _AddClipsFromClipInfo(primIndex.GetPath(), entry, &entryClips);
        if (!entryClips.valueClips.empty()) {
            clips->push_back(entryClips);
        }
    }
}

bool
Usd_ClipCache::PopulateClipsForPrim(
    const SdfPath& path, const PcpPrimIndex& primIndex)
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Usd", "Usd_ClipCache::PopulateClipsForPrim");

    std::vector<Clips> allClips;
    _AddClipsFromPrimIndex(primIndex, &allClips);

    const bool primHasClips = !allClips.empty();
    if (primHasClips) {
        tbb::mutex::scoped_lock lock(_mutex);

        const std::vector<Clips>& ancestralClips = 
            _GetClipsForPrim_NoLock(path.GetParentPath());
        allClips.insert(
            allClips.end(), ancestralClips.begin(), ancestralClips.end());

        TF_DEBUG(USD_CLIPS).Msg(
            "Populated clips for prim <%s>\n", 
            path.GetString().c_str());

        _table[path].swap(allClips);
    }

    return primHasClips;
}

SdfLayerHandleSet
Usd_ClipCache::GetUsedLayers() const
{
    SdfLayerHandleSet layers;

    tbb::mutex::scoped_lock lock(_mutex);

    for (_ClipTable::iterator::value_type const &clipsListIter : _table){
        for (Clips const &clipSet : clipsListIter.second){
            if (SdfLayerHandle layer = clipSet.manifestClip ?
                clipSet.manifestClip->GetLayerIfOpen() : SdfLayerHandle()){
                layers.insert(layer);
            }
            for (Usd_ClipRefPtr const &clip : clipSet.valueClips){
                if (SdfLayerHandle layer = clip->GetLayerIfOpen()){
                    layers.insert(layer);
                }
            }
        }
    }

    return layers;
}

const std::vector<Usd_ClipCache::Clips>&
Usd_ClipCache::GetClipsForPrim(const SdfPath& path) const
{
    TRACE_FUNCTION();
    tbb::mutex::scoped_lock lock(_mutex);
    return _GetClipsForPrim_NoLock(path);
}

const std::vector<Usd_ClipCache::Clips>&
Usd_ClipCache::_GetClipsForPrim_NoLock(const SdfPath& path) const
{
    for (SdfPath p = path; p != SdfPath::AbsoluteRootPath();
         p = p.GetParentPath()) {
        _ClipTable::const_iterator it = _table.find(p);
        if (it != _table.end()) {
            return it->second;
        }
    }

    static const std::vector<Clips> empty;
    return empty;
}

void 
Usd_ClipCache::InvalidateClipsForPrim(const SdfPath& path, Lifeboat* lifeboat)
{
    tbb::mutex::scoped_lock lock(_mutex);

    auto range = _table.FindSubtreeRange(path);
    for (auto entryIter = range.first; entryIter != range.second; ++entryIter) {
        const auto& entry = *entryIter;
        lifeboat->_clips.insert(
            lifeboat->_clips.end(), entry.second.begin(), entry.second.end());
    }

    _table.erase(path);
}

PXR_NAMESPACE_CLOSE_SCOPE

