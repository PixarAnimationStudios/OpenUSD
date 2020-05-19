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

#include "pxr/usd/usd/clipSetDefinition.h"
#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

Usd_ClipCache::
ConcurrentPopulationContext::ConcurrentPopulationContext(Usd_ClipCache &cache)
    : _cache(cache)
{
    TF_AXIOM(!_cache._concurrentPopulationContext);
    _cache._concurrentPopulationContext = this;
}

Usd_ClipCache::ConcurrentPopulationContext::~ConcurrentPopulationContext()
{
    _cache._concurrentPopulationContext = nullptr;
}

Usd_ClipCache::Usd_ClipCache()
    : _concurrentPopulationContext(nullptr)
{
}

Usd_ClipCache::~Usd_ClipCache()
{
}

static Usd_ClipSetRefPtr
_CreateClipSetFromDefinition(
    const SdfPath& usdPrimPath,
    const Usd_ClipSetDefinition& clipSetDef)
{
    std::string status;
    Usd_ClipSetRefPtr clipSet = Usd_ClipSet::New(clipSetDef, &status);

    if (!status.empty()) {
        if (!clipSet) {
            TF_WARN(
                "Invalid clips specified for prim <%s>: %s",
                usdPrimPath.GetString().c_str(),
                status.c_str());
        }
        else {
            TF_DEBUG(USD_CLIPS).Msg(
                "%s (on prim <%s>)\n", 
                status.c_str(), usdPrimPath.GetText());
        }
    }

    return clipSet;
}

static void
_AddClipsFromPrimIndex(
    const SdfPath& usdPrimPath,
    const PcpPrimIndex& primIndex, 
    std::vector<Usd_ClipSetRefPtr>* clips)
{
    std::vector<Usd_ClipSetDefinition> clipSetDefs;
    Usd_ComputeClipSetDefinitionsForPrimIndex(primIndex, &clipSetDefs);

    clips->reserve(clipSetDefs.size());
    for (const Usd_ClipSetDefinition& clipSetDef : clipSetDefs) {
        Usd_ClipSetRefPtr clipSet = 
            _CreateClipSetFromDefinition(usdPrimPath, clipSetDef);
        if (clipSet && !clipSet->valueClips.empty()) {
            clips->push_back(clipSet);
        }
    }
}

bool
Usd_ClipCache::PopulateClipsForPrim(
    const SdfPath& path, const PcpPrimIndex& primIndex)
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Usd", "Usd_ClipCache::PopulateClipsForPrim");

    std::vector<Usd_ClipSetRefPtr> allClips;
    _AddClipsFromPrimIndex(path, primIndex, &allClips);

    const bool primHasClips = !allClips.empty();
    if (primHasClips) {
        tbb::mutex::scoped_lock lock;
        if (_concurrentPopulationContext) {
            lock.acquire(_concurrentPopulationContext->_mutex);
        }

        // Find nearest ancestor with clips specified.
        const std::vector<Usd_ClipSetRefPtr>* ancestralClips = nullptr;
        SdfPath ancestralClipsPath(path.GetParentPath());
        for (; !ancestralClipsPath.IsAbsoluteRootPath() && !ancestralClips; 
             ancestralClipsPath = ancestralClipsPath.GetParentPath()) {
            ancestralClips = TfMapLookupPtr(_table, ancestralClipsPath);
        }
        
        if (ancestralClips) {
            // SdfPathTable will create entries for all ancestor paths when
            // inserting a new path. So if there were clips on prim /A and
            // we're inserting clips on prim /A/B/C, we need to make sure
            // we copy the ancestral clips from /A down to /A/B as well.
            for (SdfPath p = path.GetParentPath(); p != ancestralClipsPath;
                 p = p.GetParentPath()) {
                _table[p] = *ancestralClips;
            }

            // Append ancestral clips since they are weaker than clips
            // authored on this prim.
            allClips.insert(
                allClips.end(), ancestralClips->begin(), ancestralClips->end());
        }

        _table[path] = std::move(allClips);

        TF_DEBUG(USD_CLIPS).Msg(
            "Populated clips for prim <%s>\n", 
            path.GetString().c_str());
    }

    return primHasClips;
}

SdfLayerHandleSet
Usd_ClipCache::GetUsedLayers() const
{
    tbb::mutex::scoped_lock lock;
    if (_concurrentPopulationContext) {
        lock.acquire(_concurrentPopulationContext->_mutex);
    }
    SdfLayerHandleSet layers;
    for (_ClipTable::iterator::value_type const &clipsListIter : _table){
        for (Usd_ClipSetRefPtr const &clipSet : clipsListIter.second){
            if (SdfLayerHandle layer = clipSet->manifestClip ?
                clipSet->manifestClip->GetLayerIfOpen() : SdfLayerHandle()) {
                layers.insert(layer);
            }
            for (Usd_ClipRefPtr const &clip : clipSet->valueClips){
                if (SdfLayerHandle layer = clip->GetLayerIfOpen()){
                    layers.insert(layer);
                }
            }
        }
    }

    return layers;
}

const std::vector<Usd_ClipSetRefPtr>&
Usd_ClipCache::GetClipsForPrim(const SdfPath& path) const
{
    TRACE_FUNCTION();
    tbb::mutex::scoped_lock lock;
    if (_concurrentPopulationContext) {
        lock.acquire(_concurrentPopulationContext->_mutex);
    }
    return _GetClipsForPrim_NoLock(path);
}

const std::vector<Usd_ClipSetRefPtr>&
Usd_ClipCache::_GetClipsForPrim_NoLock(const SdfPath& path) const
{
    for (SdfPath p = path; p != SdfPath::AbsoluteRootPath();
         p = p.GetParentPath()) {
        _ClipTable::const_iterator it = _table.find(p);
        if (it != _table.end()) {
            return it->second;
        }
    }

    static const std::vector<Usd_ClipSetRefPtr> empty;
    return empty;
}

void 
Usd_ClipCache::InvalidateClipsForPrim(const SdfPath& path, Lifeboat* lifeboat)
{
    // We do not have to take the lock here -- this function must be invoked
    // exclusive to any other member function.
    auto range = _table.FindSubtreeRange(path);
    for (auto entryIter = range.first; entryIter != range.second; ++entryIter) {
        const auto& entry = *entryIter;
        lifeboat->_clips.insert(
            lifeboat->_clips.end(), entry.second.begin(), entry.second.end());
    }

    _table.erase(path);
}

PXR_NAMESPACE_CLOSE_SCOPE

