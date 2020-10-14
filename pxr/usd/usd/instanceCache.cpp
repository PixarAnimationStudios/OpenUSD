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
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/debugCodes.h"

#include "pxr/usd/pcp/primIndex.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/trace/trace.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

using std::make_pair;
using std::pair;
using std::vector;

TF_DEFINE_ENV_SETTING(
    USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY, false,
    "Set to true to cause instances to be assigned to prototypes in a "
    "deterministic way, ensuring consistency across runs.  This incurs "
    "some additional overhead.");

Usd_InstanceCache::Usd_InstanceCache()
    : _lastPrototypeIndex(0)
{
}

bool
Usd_InstanceCache::RegisterInstancePrimIndex(
    const PcpPrimIndex& index,
    UsdStagePopulationMask const *mask,
    UsdStageLoadRules const &loadRules)
{
    TfAutoMallocTag tag("InstanceCache::RegisterIndex");

    if (!TF_VERIFY(index.IsInstanceable())) {
        return false;
    }

    // Make sure we compute the key for this index before we grab
    // the mutex to minimize the time we hold the lock.
    const Usd_InstanceKey key(index, mask, loadRules);

    // Check whether a prototype for this prim index already exists
    // or if this prim index is already being used as the source for
    // a prototype.
    _InstanceKeyToPrototypeMap::const_iterator keyToPrototypeIt = 
        _instanceKeyToPrototypeMap.find(key);
    const bool prototypeAlreadyExists = 
        (keyToPrototypeIt != _instanceKeyToPrototypeMap.end());

    {
        tbb::spin_mutex::scoped_lock lock(_mutex);

        _PrimIndexPaths& pendingIndexes = _pendingAddedPrimIndexes[key];
        pendingIndexes.push_back(index.GetPath());

        // A new prototype must be created for this instance if one doesn't
        // already exist and this instance is the first one registered for
        // this key.
        const bool needsNewPrototype = 
            (!prototypeAlreadyExists && pendingIndexes.size() == 1);
        if (needsNewPrototype) {
            return true;
        }
    }

    if (prototypeAlreadyExists) {
        _PrototypeToSourcePrimIndexMap::const_iterator prototypeToSourceIndexIt=
            _prototypeToSourcePrimIndexMap.find(keyToPrototypeIt->second);
        const bool existingPrototypeUsesIndexAsSource = 
            (prototypeToSourceIndexIt != _prototypeToSourcePrimIndexMap.end() &&
             prototypeToSourceIndexIt->second == index.GetPath());
        return existingPrototypeUsesIndexAsSource;
    }

    return false;
}

void
Usd_InstanceCache::UnregisterInstancePrimIndexesUnder(
    const SdfPath& primIndexPath)
{
    TfAutoMallocTag tag("InstanceCache::UnregisterIndex");

    for (_PrimIndexToPrototypeMap::const_iterator 
             it = _primIndexToPrototypeMap.lower_bound(primIndexPath),
             end = _primIndexToPrototypeMap.end();
         it != end && it->first.HasPrefix(primIndexPath); ++it) {

        const SdfPath& prototypePath = it->second;
        _PrototypeToInstanceKeyMap::const_iterator prototypeToKeyIt = 
            _prototypeToInstanceKeyMap.find(prototypePath);
        if (!TF_VERIFY(prototypeToKeyIt != _prototypeToInstanceKeyMap.end())) {
            continue;
        }

        const Usd_InstanceKey& key = prototypeToKeyIt->second;
        _PrimIndexPaths& pendingIndexes = _pendingRemovedPrimIndexes[key];
        pendingIndexes.push_back(it->first);
    }
}

void 
Usd_InstanceCache::ProcessChanges(Usd_InstanceChanges* changes)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("InstanceCache::ProcessChanges");


    // Remove unregistered prim indexes from the cache.
    std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>
        prototypeToOldSourceIndexPath;
    for (_InstanceKeyToPrimIndexesMap::value_type &v:
             _pendingRemovedPrimIndexes) {
        const Usd_InstanceKey& key = v.first;
        _PrimIndexPaths& primIndexes = v.second;

        // Ignore any unregistered prim index that was subsequently
        // re-registered.
        _InstanceKeyToPrimIndexesMap::const_iterator registeredIt = 
            _pendingAddedPrimIndexes.find(key);
        if (registeredIt != _pendingAddedPrimIndexes.end()) {
            _PrimIndexPaths registered = registeredIt->second;
            _PrimIndexPaths unregistered;
            unregistered.swap(primIndexes);

            std::sort(registered.begin(), registered.end());
            std::sort(unregistered.begin(), unregistered.end());
            std::set_difference(unregistered.begin(), unregistered.end(),
                                registered.begin(), registered.end(),
                                std::back_inserter(primIndexes));
        }

        _RemoveInstances(key, primIndexes, changes,
                         &prototypeToOldSourceIndexPath);
    }

    // Add newly-registered prim indexes to the cache.
    if (TfGetEnvSetting(USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY)) {
        // The order in which we process newly-registered prim indexes
        // determines the name of the prototype prims assigned to instances.
        // We need to iterate over the hash map in a fixed ordering to
        // ensure we have a consistent assignment of instances to prototypes.
        typedef std::map<SdfPath, Usd_InstanceKey> _PrimIndexPathToKey;
        std::map<SdfPath, Usd_InstanceKey> keysToProcess;
        for (_InstanceKeyToPrimIndexesMap::value_type& v:
                 _pendingAddedPrimIndexes) {
            const Usd_InstanceKey& key = v.first;
            const _PrimIndexPaths& primIndexes = v.second;
            if (TF_VERIFY(!primIndexes.empty())) {
                TF_VERIFY(
                    keysToProcess.emplace(primIndexes.front(), key).second);
            }
        }

        for (const _PrimIndexPathToKey::value_type& v: keysToProcess) {
            const Usd_InstanceKey& key = v.second;
            _PrimIndexPaths& primIndexes = _pendingAddedPrimIndexes[key];
            _CreateOrUpdatePrototypeForInstances(
                key, &primIndexes, changes, prototypeToOldSourceIndexPath);
        }
    }
    else {
        for(_InstanceKeyToPrimIndexesMap::value_type& v:
                _pendingAddedPrimIndexes) {
            _CreateOrUpdatePrototypeForInstances(
                v.first, &v.second, changes, prototypeToOldSourceIndexPath);
        }
    }

    // Now that we've processed all additions and removals, we can find and
    // drop any prototypes that have no instances associated with them.
    for (const auto& v : _pendingRemovedPrimIndexes) {
        _RemovePrototypeIfNoInstances(v.first, changes);
    }

    _pendingAddedPrimIndexes.clear();
    _pendingRemovedPrimIndexes.clear();
}

void
Usd_InstanceCache::_CreateOrUpdatePrototypeForInstances(
    const Usd_InstanceKey& key,
    _PrimIndexPaths* primIndexPaths,
    Usd_InstanceChanges* changes,
    std::unordered_map<SdfPath, SdfPath, SdfPath::Hash> const &
    prototypeToOldSourceIndexPath)
{
    pair<_InstanceKeyToPrototypeMap::iterator, bool> result = 
        _instanceKeyToPrototypeMap.insert(make_pair(key, SdfPath()));
    
    const bool createdNewPrototype = result.second;
    if (createdNewPrototype) {
        // If this is a new prototype prim, the first instanceable prim
        // index that was registered must be selected as the source
        // index because the consumer was told that index required
        // a new prototype via RegisterInstancePrimIndex.
        //
        // Note that this means the source prim index for a prototype may
        // change from run to run. This should be fine, because all
        // prim indexes with the same instancing key should have the 
        // same composed values.
        const SdfPath newPrototypePath = _GetNextPrototypePath(key);
        result.first->second = newPrototypePath;
        _prototypeToInstanceKeyMap[newPrototypePath] = key;

        const SdfPath sourcePrimIndexPath = primIndexPaths->front();
        _sourcePrimIndexToPrototypeMap[sourcePrimIndexPath] = newPrototypePath;
        _prototypeToSourcePrimIndexMap[newPrototypePath] = sourcePrimIndexPath;

        changes->newPrototypePrims.push_back(newPrototypePath);
        changes->newPrototypePrimIndexes.push_back(sourcePrimIndexPath);

        TF_DEBUG(USD_INSTANCING).Msg(
            "Instancing: Creating prototype <%s> with source prim index <%s> "
            "for instancing key: %s\n", 
            newPrototypePath.GetString().c_str(),
            sourcePrimIndexPath.GetString().c_str(),
            TfStringify(key).c_str());
    }
    else {
        // Otherwise, if a prototype prim for this instance already exists
        // but no source prim index has been assigned, do so here. This
        // is exactly what happens in _RemoveInstances when a new source
        // is assigned to a prototype; however, this handles the case where
        // the last instance of a prototype has been removed and a new instance
        // of the prototype has been added in the same round of changes.
        const SdfPath& prototypePath = result.first->second;
        const bool assignNewPrimIndexForPrototype = 
            (_prototypeToSourcePrimIndexMap.count(prototypePath) == 0);
        if (assignNewPrimIndexForPrototype) {
            const SdfPath sourcePrimIndexPath = primIndexPaths->front();
            _sourcePrimIndexToPrototypeMap[sourcePrimIndexPath] = prototypePath;
            _prototypeToSourcePrimIndexMap[prototypePath] = sourcePrimIndexPath;

            changes->changedPrototypePrims.push_back(prototypePath);
            changes->changedPrototypePrimIndexes.push_back(sourcePrimIndexPath);
            SdfPath const &oldSourcePath =
                prototypeToOldSourceIndexPath.find(prototypePath)->second;

            TF_DEBUG(USD_INSTANCING).Msg(
                "Instancing: Changing source <%s> -> <%s> for <%s>\n",
                oldSourcePath.GetText(), sourcePrimIndexPath.GetText(),
                prototypePath.GetText());
        }
    }

    // Assign the newly-registered prim indexes to their prototype.
    const SdfPath& prototypePath = result.first->second;
    for (const SdfPath& primIndexPath: *primIndexPaths) {
        TF_DEBUG(USD_INSTANCING).Msg(
            "Instancing: Added instance prim index <%s> for prototype "
            "<%s>\n", primIndexPath.GetText(), prototypePath.GetText());

        _primIndexToPrototypeMap[primIndexPath] = prototypePath;
    }

    _PrimIndexPaths& primIndexesForPrototype =
        _prototypeToPrimIndexesMap[prototypePath];
    std::sort(primIndexPaths->begin(), primIndexPaths->end());

    if (primIndexesForPrototype.empty()) {
        primIndexesForPrototype.swap(*primIndexPaths);
    }
    else {
        const size_t oldNumPrimIndexes = primIndexesForPrototype.size();
        primIndexesForPrototype.insert(
            primIndexesForPrototype.end(),
            primIndexPaths->begin(), primIndexPaths->end());

        _PrimIndexPaths::iterator newlyAddedIt = 
            primIndexesForPrototype.begin() + oldNumPrimIndexes;
        std::inplace_merge(primIndexesForPrototype.begin(), newlyAddedIt,
                           primIndexesForPrototype.end());

        primIndexesForPrototype.erase(
            std::unique(primIndexesForPrototype.begin(), 
                        primIndexesForPrototype.end()),
            primIndexesForPrototype.end());
    }
}

void
Usd_InstanceCache::_RemoveInstances(
    const Usd_InstanceKey& instanceKey,
    const _PrimIndexPaths& primIndexPaths,
    Usd_InstanceChanges* changes,
    std::unordered_map<SdfPath, SdfPath, SdfPath::Hash> *
    prototypeToOldSourceIndexPath)
{
    if (primIndexPaths.empty()) {
        // if all unregistered primIndexes are also in the registered set, then
        // vector of primIndexPaths to remove can be empty.
        return;
    }
    _InstanceKeyToPrototypeMap::iterator keyToPrototypeIt = 
        _instanceKeyToPrototypeMap.find(instanceKey);
    if (keyToPrototypeIt == _instanceKeyToPrototypeMap.end()) {
        return;
    }

    const SdfPath& prototypePath = keyToPrototypeIt->second;
    // This will be set to the prim index path that the prototype was formerly
    // using if we wind up removing it.  In this case, we'll need to select a
    // new prim index path for the prototype.
    SdfPath removedPrototypePrimIndexPath;

    // Remove the prim indexes from the prim index <-> prototype bidirectional
    // mapping.
    _PrimIndexPaths& primIndexesForPrototype =
        _prototypeToPrimIndexesMap[prototypePath];
    for (const SdfPath& path: primIndexPaths) {
        _PrimIndexPaths::iterator it = std::find(
            primIndexesForPrototype.begin(), primIndexesForPrototype.end(), 
            path);
        if (it != primIndexesForPrototype.end()) {
            TF_DEBUG(USD_INSTANCING).Msg(
                "Instancing: Removed instance prim index <%s> for prototype "
                "<%s>\n", path.GetText(), prototypePath.GetText());

            primIndexesForPrototype.erase(it);
            _primIndexToPrototypeMap.erase(path);
        }

        // This path is no longer instanced under this prototype, so record the
        // old source index path and the prim's index path. Note that we may
        // have removed the entry from _prototypeToSourcePrimIndexMap in an
        // earlier iteration of this loop; if we have, then we will have saved
        // the old path away in removedPrototypePrimIndexPath.
        const SdfPath* oldSourcePrimIndexPath = 
            TfMapLookupPtr(_prototypeToSourcePrimIndexMap, prototypePath);
        if (!oldSourcePrimIndexPath) {
            oldSourcePrimIndexPath = &removedPrototypePrimIndexPath;
        }

        if (_sourcePrimIndexToPrototypeMap.erase(path)) {
            TF_VERIFY(_prototypeToSourcePrimIndexMap.erase(prototypePath));
            removedPrototypePrimIndexPath = path;
        }
    }

    // If the source prim index for this prototype is no longer available
    // but we have other instance prim indexes we can use instead, select
    // one of those to serve as the new source. 
    //
    // Otherwise, do nothing; we defer removal of this prototype until the end
    // of instance change processing (see _RemovePrototypeIfNoInstances)
    // in case a new instance for this prototype was registered.
    if (!removedPrototypePrimIndexPath.IsEmpty()) {
        if (!primIndexesForPrototype.empty()) {
            const SdfPath& newSourceIndexPath = primIndexesForPrototype.front();
            
            TF_DEBUG(USD_INSTANCING).Msg(
                "Instancing: Changing source <%s> -> <%s> for <%s>\n",
                removedPrototypePrimIndexPath.GetText(),
                newSourceIndexPath.GetText(), prototypePath.GetText());
            
            _sourcePrimIndexToPrototypeMap[newSourceIndexPath] = prototypePath;
            _prototypeToSourcePrimIndexMap[prototypePath] = newSourceIndexPath;
            
            changes->changedPrototypePrims.push_back(prototypePath);
            changes->changedPrototypePrimIndexes.push_back(newSourceIndexPath);
            
        } else {
            // Fill a data structure with the removedPrototypePrimIndexPath
            // for the prototype so that we can fill in the right "before" path
            // in changedPrototypePrimIndexes in
            // _CreateOrUpdatePrototypeForInstances().
            (*prototypeToOldSourceIndexPath)[prototypePath] =
                removedPrototypePrimIndexPath;
        }
    }
}

void 
Usd_InstanceCache::_RemovePrototypeIfNoInstances(
    const Usd_InstanceKey& instanceKey,
    Usd_InstanceChanges* changes)
{
    auto keyToPrototypeIt = _instanceKeyToPrototypeMap.find(instanceKey);
    if (keyToPrototypeIt == _instanceKeyToPrototypeMap.end()) {
        return;
    }

    const SdfPath& prototypePath = keyToPrototypeIt->second;
    auto prototypeToPrimIndexesIt =
        _prototypeToPrimIndexesMap.find(prototypePath);
    if (!TF_VERIFY(
            prototypeToPrimIndexesIt != _prototypeToPrimIndexesMap.end())) {
        return;
    }

    const _PrimIndexPaths& primIndexesForPrototype =
        prototypeToPrimIndexesIt->second;
    if (primIndexesForPrototype.empty()) {
        // This prototype has no more instances associated with it, so it can
        // be released.
        TF_DEBUG(USD_INSTANCING).Msg(
            "Instancing: Removing prototype <%s>\n", prototypePath.GetText());

        // Do this first, since prototypePath will be a stale reference after
        // removing the map entries.
        changes->deadPrototypePrims.push_back(prototypePath);

        _prototypeToInstanceKeyMap.erase(keyToPrototypeIt->second);
        _instanceKeyToPrototypeMap.erase(keyToPrototypeIt);

        _prototypeToPrimIndexesMap.erase(prototypeToPrimIndexesIt);
    }
}

bool 
Usd_InstanceCache::IsPathInPrototype(const SdfPath& path)
{
    if (path.IsEmpty() || path == SdfPath::AbsoluteRootPath()) {
        return false;
    }
    if (!path.IsAbsolutePath()) {
        // We require an absolute path because there is no way for us
        // to walk to the root prim level from a relative path.
        TF_CODING_ERROR("IsPathInPrototype() requires an absolute path "
                        "but was given <%s>", path.GetText());
        return false;
    }

    SdfPath rootPath = path;
    while (!rootPath.IsRootPrimPath()) {
        rootPath = rootPath.GetParentPath();
    }

    return TfStringStartsWith(rootPath.GetName(), "__Prototype_");
}

bool 
Usd_InstanceCache::IsPrototypePath(const SdfPath& path) 
{
    return path.IsRootPrimPath() && 
        TfStringStartsWith(path.GetName(), "__Prototype_");
}

vector<SdfPath>
Usd_InstanceCache::GetInstancePrimIndexesForPrototype(
    const SdfPath& prototypePath) const
{
    _PrototypeToPrimIndexesMap::const_iterator it = 
        _prototypeToPrimIndexesMap.find(prototypePath);

    return (it == _prototypeToPrimIndexesMap.end()) ? 
        vector<SdfPath>() : it->second;
}

SdfPath 
Usd_InstanceCache::_GetNextPrototypePath(const Usd_InstanceKey& key)
{
    return SdfPath::AbsoluteRootPath().AppendChild(
        TfToken(TfStringPrintf("__Prototype_%zu", ++_lastPrototypeIndex)));
}

vector<SdfPath> 
Usd_InstanceCache::GetAllPrototypes() const
{
    vector<SdfPath> paths;
    paths.reserve(_instanceKeyToPrototypeMap.size());
    for (const _InstanceKeyToPrototypeMap::value_type& v:
             _instanceKeyToPrototypeMap) {
        paths.push_back(v.second);
    }
    return paths;
}

size_t 
Usd_InstanceCache::GetNumPrototypes() const
{
    return _prototypeToInstanceKeyMap.size();
}

SdfPath 
Usd_InstanceCache::GetPrototypeUsingPrimIndexPath(
    const SdfPath& primIndexPath) const
{
    _SourcePrimIndexToPrototypeMap::const_iterator it = 
        _sourcePrimIndexToPrototypeMap.find(primIndexPath);
    return it == _sourcePrimIndexToPrototypeMap.end() ? SdfPath() : it->second;
}

template <class PathMap>
static 
typename PathMap::const_iterator
_FindEntryForPathOrAncestor(const PathMap& map, SdfPath path)
{
    return SdfPathFindLongestPrefix(map, path);
}

template <class PathMap>
static 
typename PathMap::const_iterator
_FindEntryForAncestor(const PathMap& map, const SdfPath& path)
{
    if (path == SdfPath::AbsoluteRootPath()) {
        return map.end();
    }
    return SdfPathFindLongestStrictPrefix(map, path);
}

bool 
Usd_InstanceCache::PrototypeUsesPrimIndexPath(
    const SdfPath& primIndexPath) const
{
    return _PrototypeUsesPrimIndexPath(primIndexPath);
}

vector<SdfPath> 
Usd_InstanceCache::GetPrimsInPrototypesUsingPrimIndexPath(
    const SdfPath& primIndexPath) const
{
    vector<SdfPath> prototypePaths;
    _PrototypeUsesPrimIndexPath(primIndexPath, &prototypePaths);
    return prototypePaths;
}

vector<std::pair<SdfPath, SdfPath>> 
Usd_InstanceCache::GetPrototypesUsingPrimIndexPathOrDescendents(
        const SdfPath& primIndexPath) const
{
    vector<std::pair<SdfPath, SdfPath>> prototypeSourceIndexPairs;
    for (_SourcePrimIndexToPrototypeMap::const_iterator 
             it = _sourcePrimIndexToPrototypeMap.lower_bound(primIndexPath),
             end = _sourcePrimIndexToPrototypeMap.end();
         it != end && it->first.HasPrefix(primIndexPath); ++it) {

        const SdfPath& prototypePath = it->second;
        _PrototypeToSourcePrimIndexMap::const_iterator prototypeToSourceIt = 
            _prototypeToSourcePrimIndexMap.find(prototypePath);

        if (!TF_VERIFY(
                prototypeToSourceIt != _prototypeToSourcePrimIndexMap.end(),
                "prototypePath <%s> missing in prototypesToSourceIndexPath map",
                prototypePath.GetText())) {
            prototypeSourceIndexPairs.emplace_back(prototypePath, SdfPath());
            continue;
        }
        
        const SdfPath& sourceIndexPath = prototypeToSourceIt->second;
        prototypeSourceIndexPairs.emplace_back(prototypePath, sourceIndexPath);
    }
    
    return prototypeSourceIndexPairs;
}

bool
Usd_InstanceCache::_PrototypeUsesPrimIndexPath(
    const SdfPath& primIndexPath,
    vector<SdfPath>* prototypePaths) const
{
    // This function is trickier than you might expect because it has
    // to deal with nested instances. Consider this case:
    //
    // /World
    //   Set_1     [prototype: </__Prototype_1>]
    // /__Prototype_1 [index: </World/Set_1>]
    //   Prop_1    [prototype: </__Prototype_2>, index: </World/Set_1/Prop_1> ]
    //   Prop_2    [prototype: </__Prototype_2>, index: </World/Set_1/Prop_2> ]
    // /__Prototype_2 [index: </World/Set_1/Prop_1>]
    //   Scope     [index: </World/Set_1/Prop_1/Scope>]
    // 
    // Asking if the prim index /World/Set_1/Prop_1/Scope is used by a
    // prototype should return true, because it is used by /__Prototype_2/Scope.
    // But this function should return false for /World/Set_1/Prop_2/Scope.
    // The naive implementation that looks through 
    // _sourcePrimIndexToPrototypeMap would wind up returning true for both
    // of these.

    bool prototypeUsesPrimIndex = false;

    SdfPath curIndexPath = primIndexPath;
    while (curIndexPath != SdfPath::AbsoluteRootPath()) {
        // Find the instance prim index that is closest to the current prim
        // index path. If there isn't one, this prim index isn't a descendent
        // of an instance, which means it can't possibly be used by a prototype.
        _PrimIndexToPrototypeMap::const_iterator it = 
            _FindEntryForPathOrAncestor(_primIndexToPrototypeMap, curIndexPath);
        if (it == _primIndexToPrototypeMap.end()) {
            break;
        }

        // Figure out what prototype is associated with the prim index
        // we found, and see if the given prim index is a descendent of its
        // source prim index. If it is, then this prim index must be used
        // by a descendent of that prototype.
        _PrototypeToSourcePrimIndexMap::const_iterator prototypeToSourceIt =
            _prototypeToSourcePrimIndexMap.find(it->second);
        if (!TF_VERIFY(
                prototypeToSourceIt != _prototypeToSourcePrimIndexMap.end())) {
            break;
        }

        const SdfPath& prototypePath = prototypeToSourceIt->first;
        const SdfPath& sourcePrimIndexPath = prototypeToSourceIt->second;
        if (curIndexPath.HasPrefix(sourcePrimIndexPath)) {
            // If we don't need to collect all the prototype paths using this
            // prim index, we can bail out immediately.
            prototypeUsesPrimIndex = true;
            if (prototypePaths) {
                prototypePaths->push_back(primIndexPath.ReplacePrefix(
                    sourcePrimIndexPath, prototypePath));
            }
            else {
                break;
            }
        }

        // If we found an entry for an ancestor of curIndexPath in 
        // _primIndexToPrototypeMap, the index must be a descendant of an
        // instanceable prim index. These indexes can only ever be used by
        // a single prototype prim, so we can stop here. 
        // 
        // Otherwise, this index is an instanceable prim index. In the case of 
        // nested instancing, there may be another prototype prim using this
        // index,
        // so we have to keep looking.
        const bool indexIsDescendentOfInstance = (it->first != curIndexPath);
        if (indexIsDescendentOfInstance) {
            break;
        }

        curIndexPath = it->first.GetParentPath();
    }

    return prototypeUsesPrimIndex;
}

bool 
Usd_InstanceCache::IsPathDescendantToAnInstance(
    const SdfPath& usdPrimPath) const
{
    // If any ancestor of usdPrimPath is in _primIndexToPrototypeMap, it's
    // a descendent of an instance.
    return _FindEntryForAncestor(_primIndexToPrototypeMap, usdPrimPath) != 
        _primIndexToPrototypeMap.end();
}

SdfPath
Usd_InstanceCache::GetMostAncestralInstancePath(
    const SdfPath &usdPrimPath) const
{
    SdfPath path = usdPrimPath;
    SdfPath result;
    SdfPath const &absRoot = SdfPath::AbsoluteRootPath();
    while (path != absRoot) {
        auto it = _FindEntryForAncestor(_primIndexToPrototypeMap, path);
        if (it == _primIndexToPrototypeMap.end())
            break;
        result = it->first;
        path = it->first.GetParentPath();
    }
    return result;
}

SdfPath 
Usd_InstanceCache::GetPrototypeForInstanceablePrimIndexPath(
    const SdfPath& primIndexPath) const
{
    // Search the mapping from instance prim index to prototype prim
    // to find the associated prototype.
    _PrimIndexToPrototypeMap::const_iterator it = 
        _primIndexToPrototypeMap.find(primIndexPath);
    return (it == _primIndexToPrototypeMap.end() ? SdfPath() : it->second);
}

SdfPath
Usd_InstanceCache::GetPathInPrototypeForInstancePath(
    const SdfPath& primPath) const
{
    SdfPath primIndexPath;

    // Without instancing, the path of a prim on a stage will be the same
    // as the path for its prim index. However, this is not the case for
    // prims in prototypes (e.g., /__Prototype_1/Instance/Child). In this case,
    // we need to figure out what the source prim index path would be.
    if (IsPathInPrototype(primPath)) {
        // If primPath is prefixed by a prototype prim path, replace it
        // with that prototype's source index path to produce a prim index
        // path.
        _PrototypeToSourcePrimIndexMap::const_iterator it = 
            _prototypeToSourcePrimIndexMap.upper_bound(primPath);
        if (it != _prototypeToSourcePrimIndexMap.begin()) {
            --it;
            const SdfPath& prototypePath = it->first;
            const SdfPath& sourcePrimIndexPath = it->second;

            // Just try the prefix replacement instead of doing a separate
            // HasPrefix check. If it does nothing, we know primPath wasn't
            // a prim in a prototype that this cache knows about.
            const SdfPath p = 
                primPath.ReplacePrefix(prototypePath, sourcePrimIndexPath);
            if (p != primPath) {
                primIndexPath = p;
            }
        }
    }
    else {
        primIndexPath = primPath;
    }

    if (primIndexPath.IsEmpty())
        return primIndexPath;

    // This function is trickier than you might expect because it has
    // to deal with nested instances. Consider this case:
    //
    // /World
    //   Set_1     [prototype: </__Prototype_1>, index: </World/Set_1>]
    //   Set_2     [prototype: </__Prototype_1>, index: </World/Set_2>]
    // /__Prototype_1 [index: </World/Set_1>]
    //   Prop_1    [prototype: </__Prototype_2>, index: </World/Set_1/Prop_1> ]
    //   Prop_2    [prototype: </__Prototype_2>, index: </World/Set_1/Prop_2> ]
    // /__Prototype_2 [index: </World/Set_1/Prop_1>]
    //   Scope     [index: </World/Set_1/Prop_1/Scope>]
    // 
    // Asking for the prim in prototype for the prim index 
    // /World/Set_2/Prop_1/Scope should return /__Prototype_2/Scope, since
    // /World/Set_2 is an instance of /__Prototype_1, and /__Prototype_1/Prop_1
    // is an instance of /__Prototype_2.
    //
    // The naive implementation would look through _primIndexToPrototypeMap
    // and do a prefix replacement, but that gives /__Prototype_1/Prop_1/Scope. 
    // This is because the prim index /World/Set_2/Prop_1/Scope has never been 
    // computed in this example!

    SdfPath primInPrototypePath;
    SdfPath curPrimIndexPath = primIndexPath;
    while (!curPrimIndexPath.IsEmpty()) {
        // Find the instance prim index that is closest to the current
        // prim index path. If there isn't one, this prim index isn't a 
        // descendent of an instance.
        _PrimIndexToPrototypeMap::const_iterator it = 
            _FindEntryForAncestor(_primIndexToPrototypeMap, curPrimIndexPath);
        if (it == _primIndexToPrototypeMap.end()) {
            break;
        }

        // Find the source prim index corresponding to this prototype.
        // If curPrimIndexPath is already relative to this prim index,
        // we can do a prefix replacement to determine the final prototype
        // prim path.
        //
        // If curPrimIndexPath is *not* relative to this prim index,
        // do a prefix replacement to make it so, then loop and try again.
        // This helps us compute the correct prim in prototype in the case
        // above because we know the source prim index *must* have been
        // computed -- otherwise, it wouldn't be a prototype's source index.
        // The next time around we'll find a match for curPrimIndexPath 
        // in _primIndexToPrototypeMap that gets us closer to the nested
        // instance's prototype (if one exists).
        _PrototypeToSourcePrimIndexMap::const_iterator prototypeToSourceIt =
            _prototypeToSourcePrimIndexMap.find(it->second);
        if (!TF_VERIFY(
                prototypeToSourceIt != _prototypeToSourcePrimIndexMap.end())) {
            break;
        }
        
        const SdfPath& sourcePrimIndexPath = prototypeToSourceIt->second;
        if (it->first == sourcePrimIndexPath) {
            primInPrototypePath = 
                curPrimIndexPath.ReplacePrefix(it->first, it->second);
            break;
        }

        curPrimIndexPath = 
            curPrimIndexPath.ReplacePrefix(it->first, sourcePrimIndexPath);
    }

    return primInPrototypePath;
}

PXR_NAMESPACE_CLOSE_SCOPE

