//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/ostreamMethods.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

// ------------------------------------------------------------

class Usd_ClipCache::Lifeboat::Data
{
public:
    struct ManifestKey
    {
        SdfPath primPath;
        std::string clipSetName;
        SdfPath clipPrimPath;
        VtArray<SdfAssetPath> clipAssetPaths;
        
        bool operator==(const ManifestKey& rhs) const
        {
            return std::tie(
                primPath, clipSetName, clipPrimPath, clipAssetPaths) ==
                   std::tie(
                rhs.primPath, rhs.clipSetName, rhs.clipPrimPath, 
                rhs.clipAssetPaths);
        }
        
        struct Hash
        {
            inline size_t operator()(const ManifestKey& key) const
            {
                return TfHash::Combine(
                    key.primPath,
                    key.clipSetName,
                    key.clipPrimPath,
                    key.clipAssetPaths
                );
            }
        };
    };

    std::vector<Usd_ClipSetRefPtr> clips;
    std::unordered_map<ManifestKey, std::string, ManifestKey::Hash>
        generatedManifests;
};

Usd_ClipCache::Lifeboat::Lifeboat(Usd_ClipCache& cache)
    : _cache(cache)
    , _data(new Data)
{
    TF_AXIOM(!_cache._lifeboat);
    _cache._lifeboat = this;
}

Usd_ClipCache::Lifeboat::~Lifeboat()
{
    // Release the invalidated clip sets, then stop tracking the layers that
    // only they used.
    _data.reset();

    _cache._lifeboat = nullptr;
    _cache._RemoveUnusedClipLayers();
}

// ------------------------------------------------------------

Usd_ClipCache::Usd_ClipCache(RegisterLayerFn registerLayerFn)
    : _concurrentPopulationContext(nullptr)
    , _lifeboat(nullptr)
    , _registerLayerFn(std::move(registerLayerFn))
{
}

Usd_ClipCache::~Usd_ClipCache()
{
    for (auto& entry : _clipLayers) {
        TfNotice::Revoke(entry.second);
    }
}

void
Usd_ClipCache::_AddClipLayer(const SdfLayerHandle &layer)
{
    if (!_registerLayerFn) {
        return;
    }

    std::lock_guard<std::mutex> lock(_clipLayersMutex);
    TfNotice::Key &key = _clipLayers[layer];
    if (!key) {
        key = _registerLayerFn(layer);
    }
}

void
Usd_ClipCache::_AdoptClipSet(const Usd_ClipSetRefPtr &clipSet)
{
    // Clips that opened their layers before this point, either because the
    // layer was already open when the clip was constructed or because it is a
    // generated manifest, will not report them to this cache, so record them
    // here.
    const auto adopt = [this](Usd_Clip &clip) {
        clip._cache = this;
        if (SdfLayerHandle layer = clip.GetLayerIfOpen()) {
            _AddClipLayer(layer);
        }
    };

    adopt(*clipSet->manifestClip);
    for (const Usd_ClipRefPtr& clip : clipSet->valueClips) {
        adopt(*clip);
    }
}

void
Usd_ClipCache::_RemoveUnusedClipLayers()
{
    {
        std::lock_guard<std::mutex> lock(_clipLayersMutex);
        if (_clipLayers.empty()) {
            return;
        }
    }

    const SdfLayerHandleSet usedLayers = GetUsedLayers();

    std::vector<TfNotice::Key> keysToRevoke;
    {
        std::lock_guard<std::mutex> lock(_clipLayersMutex);
        for (auto it = _clipLayers.begin(); it != _clipLayers.end(); ) {
            if (!it->first || !usedLayers.count(it->first)) {
                keysToRevoke.push_back(it->second);
                it = _clipLayers.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    for (TfNotice::Key& key : keysToRevoke) {
        TfNotice::Revoke(key);
    }
}

const SdfChangeList::Entry &
Usd_ClipCache::GetClipValuesChangedEntry()
{
    // Intentional leak to avoid static destructor registration & call.
    static const SdfChangeList::Entry *entry = []() {
        SdfChangeList::Entry *entry = new SdfChangeList::Entry;
        entry->flags.didChangeAttributeTimeSamples = true;
        entry->infoChanged.emplace_back(
            SdfFieldKeys->Spline, std::make_pair(VtValue(), VtValue()));
        return entry;
    }();
    return *entry;
}

void
Usd_ClipCache::ComputeChangedSites(
    const SdfLayerHandle &layer,
    const SdfChangeList &changeList,
    ChangedSites *sites) const
{
    {
        std::lock_guard<std::mutex> lock(_clipLayersMutex);
        if (!_clipLayers.count(layer)) {
            return;
        }
    }

    // Find the clip sets that use layer. Collect unique clip sets first, as
    // Reload does, since ancestral clip sets appear in several entries (see
    // PopulateClipsForPrim).
    std::unordered_set<Usd_ClipSetRefPtr> clipSets;
    for (auto it = _table.begin(), end = _table.end(); it != end; ++it) {
        clipSets.insert(it->second.begin(), it->second.end());
    }

    for (const Usd_ClipSetRefPtr& clipSet : clipSets) {
        // Value resolution consults a clip set's manifest before any of its
        // value clips, so if the manifest has not been opened no attribute
        // value has been resolved from this clip set and there is nothing to
        // report. The clip set's value clip layers may still be open, since a
        // clip takes its layer if it is already open when the clip is
        // constructed. Checking this also avoids opening the manifest here.
        const SdfLayerHandle manifest =
            clipSet->manifestClip->GetLayerIfOpen();
        if (!manifest) {
            continue;
        }

        const bool isManifest = manifest == layer;
        const bool isValueClip = std::any_of(
            clipSet->valueClips.begin(), clipSet->valueClips.end(),
            [&layer](const Usd_ClipRefPtr& clip) {
                return clip->GetLayerIfOpen() == layer;
            });
        if (isManifest || isValueClip) {
            _AddChangedSites(*clipSet, manifest, isManifest, isValueClip,
                             changeList, sites);
        }
    }
}

void
Usd_ClipCache::_AddChangedSites(
    const Usd_ClipSet &clipSet,
    const SdfLayerHandle &manifest,
    bool isManifest,
    bool isValueClip,
    const SdfChangeList &changeList,
    ChangedSites *sites)
{
    const SdfPath &clipPrimPath = clipSet.clipPrimPath;
    const auto addSite = [&clipSet, &clipPrimPath, sites](
        const SdfPath &pathInClip) {
        sites->push_back({
            clipSet.sourceLayerStack,
            pathInClip.ReplacePrefix(clipPrimPath, clipSet.sourcePrimPath)});
    };

    // Edits to a manifest's attribute can change whether and how the attribute
    // takes values from clips. Edits to a value clip's attribute only matter if
    // the manifest declares it.
    const auto addAttributeSite = [&](const SdfPath &pathInClip) {
        if (pathInClip.HasPrefix(clipPrimPath) &&
            (isManifest ||
             clipSet.ContainsValueForAttribute(pathInClip.ReplacePrefix(
                 clipPrimPath, clipSet.sourcePrimPath)))) {
            addSite(pathInClip);
        }
    };

    for (const auto& pathAndEntry : changeList.GetEntryList()) {
        const SdfPath &path = pathAndEntry.first;
        const SdfChangeList::Entry &entry = pathAndEntry.second;

        if (path == SdfPath::AbsoluteRootPath()) {
            if (!entry.flags.didReplaceContent) {
                continue;
            }

            // The layer's content was replaced without itemizing changes, as
            // when a layer that streams its data is reloaded.
            if (isManifest) {
                // The attributes the manifest declares may have changed
                // arbitrarily. Since clips cannot introduce attributes, the
                // affected attributes are among those on the clip prim and its
                // descendants.
                addSite(clipPrimPath);
            }
            else if (isValueClip && manifest->HasSpec(clipPrimPath)) {
                // Only attributes declared in the manifest take values from
                // clips, so report those.
                manifest->Traverse(clipPrimPath,
                    [&addSite](const SdfPath &p) {
                        if (p.IsPrimPropertyPath()) {
                            addSite(p);
                        }
                    });
            }
            continue;
        }

        if (path.IsPrimPropertyPath()) {
            addAttributeSite(path);
            if (entry.flags.didRename) {
                addAttributeSite(entry.oldPath);
            }
        }
        else if (path.IsPrimOrPrimVariantSelectionPath() &&
                 (entry.flags.didAddInertPrim ||
                  entry.flags.didAddNonInertPrim ||
                  entry.flags.didRemoveInertPrim ||
                  entry.flags.didRemoveNonInertPrim ||
                  entry.flags.didRename)) {
            // Adding, removing or renaming a prim spec in a clip or manifest
            // can change the values of attributes beneath it without itemizing
            // them.
            if (path.HasPrefix(clipPrimPath)) {
                addSite(path);
            }
            if (entry.flags.didRename &&
                entry.oldPath.HasPrefix(clipPrimPath)) {
                addSite(entry.oldPath);
            }
        }
    }
}

static Usd_ClipSetRefPtr
_CreateClipSetFromDefinition(
    const SdfPath& usdPrimPath,
    const std::string& clipSetName,
    const Usd_ClipSetDefinition& clipSetDef)
{
    std::string status;
    Usd_ClipSetRefPtr clipSet = Usd_ClipSet::New(
        clipSetName, clipSetDef, &status);

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

void
Usd_ClipCache::_ComputeClipsFromPrimIndex(
    const SdfPath& usdPrimPath,
    const PcpPrimIndex& primIndex,
    std::vector<Usd_ClipSetRefPtr>* clips)
{
    std::vector<Usd_ClipSetDefinition> clipSetDefs;
    std::vector<std::string> clipSetNames;
    Usd_ComputeClipSetDefinitionsForPrimIndex(
        primIndex, &clipSetDefs, &clipSetNames);

    clips->reserve(clipSetDefs.size());
    for (size_t i = 0; i < clipSetDefs.size(); ++i) {
        Usd_ClipSetDefinition& clipSetDef = clipSetDefs[i];
        const std::string& clipSetName = clipSetNames[i];

        // If no clip manifest was explicitly specified and we have an
        // active lifeboat (i.e., we're in the middle of change processing)
        // see if we can reuse a generated manifest from before.
        bool reusingGeneratedManifest = false;
        if (!clipSetDef.clipManifestAssetPath && _lifeboat) {
            Lifeboat::Data::ManifestKey key;
            key.primPath = usdPrimPath;
            key.clipSetName = clipSetName;
            if (clipSetDef.clipPrimPath) {
                key.clipPrimPath = SdfPath(*clipSetDef.clipPrimPath);
            }
            if (clipSetDef.clipAssetPaths) {
                key.clipAssetPaths = *clipSetDef.clipAssetPaths;
            }

            const std::string* manifestIdentifier = TfMapLookupPtr(
                _lifeboat->_data->generatedManifests, key);
            if (manifestIdentifier) {
                clipSetDef.clipManifestAssetPath = 
                    SdfAssetPath(*manifestIdentifier);
                reusingGeneratedManifest = true;
            }
        }

        Usd_ClipSetRefPtr clipSet = _CreateClipSetFromDefinition(
            usdPrimPath, clipSetName, clipSetDef);
        if (clipSet && !clipSet->valueClips.empty()) {
            // If reusing a previously-generated manifest from the lifeboat, 
            // pull on it here to ensure the manifest takes ownership of it.
            if (reusingGeneratedManifest) {
                TF_UNUSED(clipSet->manifestClip->GetLayer());
            }
            _AdoptClipSet(clipSet);
            clips->push_back(clipSet);
        }
    }
}

bool
Usd_ClipCache::PopulateClipsForPrim(
    const SdfPath& path, const PcpPrimIndex& primIndex)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("Usd", "Usd_ClipCache::PopulateClipsForPrim");

    std::vector<Usd_ClipSetRefPtr> allClips;
    _ComputeClipsFromPrimIndex(path, primIndex, &allClips);

    const bool primHasClips = !allClips.empty();
    if (primHasClips) {
        TRACE_SCOPE("Usd_ClipCache::PopulateClipsForPrim (primHasClips)");
        std::unique_lock<std::mutex> lock = (_concurrentPopulationContext) ?
            std::unique_lock<std::mutex>(_concurrentPopulationContext->_mutex) :
            std::unique_lock<std::mutex>();

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
    std::unique_lock<std::mutex> lock = (_concurrentPopulationContext) ?
            std::unique_lock<std::mutex>(_concurrentPopulationContext->_mutex) :
            std::unique_lock<std::mutex>();
            
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

void
Usd_ClipCache::Reload()
{
    SdfChangeBlock changeBlock;

    // Collect all unique clip sets to iterate over to avoid duplicated work
    // due to ancestral clip entries (see PopulateClipsForPrim)
    std::unordered_set<Usd_ClipSetRefPtr> clipSets;
    for (auto it = _table.begin(), end = _table.end(); it != end; ++it) {
        clipSets.insert(it->second.begin(), it->second.end());
    }

    // Iterate through all clip sets and call SdfLayer::Reload for any
    // associated layers that are opened.
    std::unordered_set<SdfLayerHandle, TfHash> reloadedClipLayers;
    for (const Usd_ClipSetRefPtr& clipSet : clipSets) {
        // Reload all clip layers.
        for (const Usd_ClipRefPtr& clip : clipSet->valueClips) {
            const SdfLayerHandle clipLayer = clip->GetLayerIfOpen();
            if (!clipLayer) {
                continue;
            }

            // It's possible (but unlikely?) that the same clip layer is used
            // in multiple clip sets. We need to keep track of all the layers
            // that were reloaded to handle that case.
            const bool notPreviouslyReloaded =
                reloadedClipLayers.insert(clipLayer).second;
            if (notPreviouslyReloaded) {
                clipLayer->Reload();
            }
        }

        // Reload the manifest if it was supplied by the user, otherwise
        // regenerate it.
        //
        // It's tempting to try to only regenerate the manifest if any
        // of the clip layers were actually reloaded. However, this doesn't
        // handle the case where someone modifies a clip layer, saves it,
        // then tries to reload the stage. In that case, the clip layer
        // wouldn't have anything to reload, so we wouldn't know that we
        // need to regenerate the manifest.
        SdfLayerHandle manifestLayer = clipSet->manifestClip->GetLayerIfOpen();
        if (manifestLayer) {
            if (Usd_IsAutoGeneratedClipManifest(manifestLayer)) {
                SdfLayerRefPtr newManifest = Usd_GenerateClipManifest(
                    clipSet->valueClips, clipSet->clipPrimPath);
                manifestLayer->TransferContent(newManifest);
            }
            else {
                manifestLayer->Reload();
            }
        }
    }
}

const std::vector<Usd_ClipSetRefPtr>&
Usd_ClipCache::GetClipsForPrim(const SdfPath& path) const
{
    TRACE_FUNCTION();
    std::unique_lock<std::mutex> lock = (_concurrentPopulationContext) ?
        std::unique_lock<std::mutex>(_concurrentPopulationContext->_mutex) :
        std::unique_lock<std::mutex>();
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
Usd_ClipCache::InvalidateClipsForPrim(const SdfPath& invalidatePath)
{
    // We expect a Lifeboat to always be active when invoking this function
    // for simplicity.
    TF_AXIOM(_lifeboat);

    // We do not have to take the lock here -- this function must be invoked
    // exclusive to any other member function.

    auto range = _table.FindSubtreeRange(invalidatePath);
    for (auto entryIter = range.first; entryIter != range.second; ++entryIter) {
        const SdfPath& path = entryIter->first;
        const std::vector<Usd_ClipSetRefPtr>& clipSets = entryIter->second;

        // Keep all clip sets alive. In particular, this keeps any layers
        // that have been opened and are owned by these clip sets alive until
        // the lifeboat is dropped.
        _lifeboat->_data->clips.insert(
            _lifeboat->_data->clips.end(), clipSets.begin(), clipSets.end());

        // Keep track of all generated manifests so that they can be reused
        // during population until the lifeboat s dropped.
        for (const Usd_ClipSetRefPtr& clipSet : clipSets) {
            const SdfLayerHandle manifestLayer = 
                clipSet->manifestClip->GetLayerIfOpen();
            if (!manifestLayer 
                || !Usd_IsAutoGeneratedClipManifest(manifestLayer)) {
                continue;
            }

            Lifeboat::Data::ManifestKey key;
            key.primPath = path;
            key.clipSetName = clipSet->name;
            key.clipPrimPath = clipSet->clipPrimPath;
                
            key.clipAssetPaths.reserve(clipSet->valueClips.size());
            for (const Usd_ClipRefPtr& clip : clipSet->valueClips) {
                key.clipAssetPaths.push_back(clip->assetPath);
            }

            _lifeboat->_data->generatedManifests[std::move(key)] = 
                manifestLayer->GetIdentifier();
        }
    }

    _table.erase(invalidatePath);
}

PXR_NAMESPACE_CLOSE_SCOPE

