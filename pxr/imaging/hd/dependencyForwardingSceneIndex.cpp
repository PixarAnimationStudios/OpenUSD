//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/dependencyForwardingSceneIndex.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

//----------------------------------------------------------------------------

HdDependencyForwardingSceneIndex::HdDependencyForwardingSceneIndex(
    HdSceneIndexBaseRefPtr inputScene)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _manualGarbageCollect(false)
{
}

void
HdDependencyForwardingSceneIndex::SetManualGarbageCollect(
    bool manualGarbageCollect) {
    _manualGarbageCollect = manualGarbageCollect;
}

HdSceneIndexPrim
HdDependencyForwardingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Check incoming prim for dependencies.
    if (_affectedPrimToDependsOnPathsMap.find(primPath) == 
        _affectedPrimToDependsOnPathsMap.end()) {
        _UpdateDependencies(primPath, prim);
    }

    // HdDependencyForwardingSceneIndex implements invalidation for
    // incoming dependencies, so downstream observers need not bother.
    // Block the dependencies from further processing.
    if (_affectedPrimToDependsOnPathsMap.find(primPath) != 
        _affectedPrimToDependsOnPathsMap.end()) {
        static const auto blockDeps =
            HdRetainedContainerDataSource::New(
                HdDependenciesSchema::GetSchemaToken(),
                HdBlockDataSource::New());
        prim.dataSource =
            HdCreateOverlayContainerDataSource(
                blockDeps, prim.dataSource);
    }

    return prim;
}

SdfPathVector
HdDependencyForwardingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    // pass through without change
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdDependencyForwardingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _VisitedNodeSet visited;
    _AdditionalDirtiedVector additionalDirtied;
    _PathSet rebuildDependencies;

    WorkParallelForN(entries.size(),
    [&](size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
        const HdSceneIndexObserver::AddedPrimEntry &entry = entries[i];

        // If this prim shows up in the dependency map, make sure we clear
        // cached dependency data.

        // Clear out the affected prim map.
        _AffectedPrimToDependsOnPathsEntryMap::iterator ait =
            _affectedPrimToDependsOnPathsMap.find(entry.primPath);
        if (ait != _affectedPrimToDependsOnPathsMap.end()) {
            _ClearDependencies(entry.primPath);
        }

        // Send out relevant invalidations.
        _DependedOnPrimsAffectedPrimsMap::iterator dit =
            _dependedOnPrimToDependentsMap.find(entry.primPath);
        if (dit != _dependedOnPrimToDependentsMap.end()) {
            for (auto &affectedPair : (*dit).second) {
                const SdfPath &affectedPrimPath = affectedPair.first;

                // Filter out self-dependencies.
                if (affectedPrimPath == entry.primPath) {
                    continue;
                }

                // Dirty affected prims.
                HdDataSourceLocatorSet affectedLocators;
                for (const auto &keyEntryPair :
                        affectedPair.second.locatorsEntryMap) {
                    const _LocatorsEntry &entry = keyEntryPair.second;
                    affectedLocators.insert(entry.affectedDataSourceLocator);
                }

                if (!affectedLocators.IsEmpty()) {
                    additionalDirtied.emplace_back(affectedPrimPath,
                            affectedLocators);
                    _PrimDirtied(affectedPrimPath, affectedLocators,
                            &visited, &additionalDirtied, &rebuildDependencies);
                }
            }
        }
    }});

    WorkParallelForTBBRange(rebuildDependencies.range(),
    [&](const _PathSet::range_type& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        SdfPath const& rebuildPath = *it;
        _ClearDependencies(rebuildPath);
        _UpdateDependencies(rebuildPath);
    }});

    if (!_manualGarbageCollect) {
        RemoveDeletedEntries(nullptr, nullptr);
    }

    _SendPrimsAdded(entries);

    if (!additionalDirtied.empty()) {
        HdSceneIndexObserver::DirtiedPrimEntries flattened(
            additionalDirtied.begin(), additionalDirtied.end());
        _SendPrimsDirtied(flattened);
    }
}

void
HdDependencyForwardingSceneIndex::_ResetDependencies()
{
    // deleting these maps can be surprisingly expensive!
    TRACE_FUNCTION();

    if (!_manualGarbageCollect) {
        WorkSwapDestroyAsync(_dependedOnPrimToDependentsMap);
        WorkSwapDestroyAsync(_affectedPrimToDependsOnPathsMap);
        _potentiallyDeletedDependedOnPaths.clear();
        _potentiallyDeletedAffectedPaths.clear();
    } else {
        for (auto &pair : _dependedOnPrimToDependentsMap) {
            _potentiallyDeletedDependedOnPaths.insert(pair.first);
            for (auto &locPair : pair.second) {
                locPair.second.flaggedForDeletion = true;
            }
        }
        for (auto &pair : _affectedPrimToDependsOnPathsMap) {
            _potentiallyDeletedAffectedPaths.insert(pair.first);
            pair.second.flaggedForDeletion = true;
        }
    }
}

void
HdDependencyForwardingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // Reset deps and early out if the entry vector starts with "/".
    // (Note: we don't search the whole vector for "/", since that's additional
    // work for our current call patterns; and we have another early out below.
    // ... we could search in the future if warranted though.)
    if (entries.size() > 0 && entries[0].primPath.IsAbsoluteRootPath()) {
        _ResetDependencies();
        _SendPrimsRemoved(entries);
        return;
    }

    _VisitedNodeSet visited;
    _AdditionalDirtiedVector additionalDirtied;
    _PathSet rebuildDependencies;

    // For performance, if the PrimsRemoved vector is really large we add the
    // entries to a hash set; we can check a prim by iterating up-hierarchy and
    // checking against the hash set.  This tells us whether a prim has been
    // removed in O(max scene depth) time.
    //
    // If the vector is small, checking against PrimsRemoved entries is still
    // faster, as this is O(notice size).
    //
    // We're mainly trying to avoid accidentally quadratic behavior as the size
    // of the notice approaches the size of the scene, so we use the hash set
    // if the notice vector is bigger than our size threshold, chosen because
    // it's small for a for loop but large for a scene namespace depth.
    static constexpr size_t HashsetThreshold = 30;

    std::function<bool(SdfPath const&)> hasBeenRemoved;
    TfHashSet<SdfPath, SdfPath::Hash> removedEntriesHashSet;

    if (entries.size() > HashsetThreshold) {
        // Create a hashset of entries
        removedEntriesHashSet.reserve(entries.size());
        for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
            removedEntriesHashSet.insert(entry.primPath);
        }

        // Function to quickly check if a prim (including any parent prim) has
        // been removed.
        // (When a parent prim is removed, all child prims are also removed).
        hasBeenRemoved = [&removedEntriesHashSet](const SdfPath& primToCheck)
        {
            // Check if the prim has been removed, or if a parent of the prim
            for (SdfPath path = primToCheck; !path.IsEmpty();
                 path = path.GetParentPath())
            {
                if (removedEntriesHashSet.find(path) !=
                    removedEntriesHashSet.end())
                {
                    return true;
                }
                if (path.IsAbsoluteRootPath())
                {
                    break;
                }
            }
            return false;
        };
    } else {
        hasBeenRemoved = [&entries](const SdfPath& primToCheck)
        {
            for (const auto& entry : entries) {
                if (primToCheck.HasPrefix(entry.primPath)) {
                    return true;
                }
            }
            return false;
        };
    }

    // Because of the recursive nature of remove notices, we need to iterate the
    // whole dependency map.
    //
    // Iterate _affectedPrimToDependsOnPathsMap looking for:
    // - Affected prims that are descendants of primPath.
    // Clear their dependency info out.
    WorkParallelForTBBRange(_affectedPrimToDependsOnPathsMap.range(),
    [&](const _AffectedPrimToDependsOnPathsEntryMap::range_type& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        _AffectedPrimToDependsOnPathsEntryMap::value_type& pair = *it;
        if (hasBeenRemoved(pair.first)) {
            pair.second.flaggedForDeletion = true;
            _potentiallyDeletedAffectedPaths.insert(pair.first);
        }
    }});

    // If we deleted every affected prim, there's no point looping over
    // dependedOn prims; we can reset deps and stop here.
    if (_potentiallyDeletedAffectedPaths.size() ==
            _affectedPrimToDependsOnPathsMap.size()) {
        _ResetDependencies();
        _SendPrimsRemoved(entries);
        return;
    }

    // Iterate _dependedOnPrimToDependentsMap looking for:
    // - Depended on prims with affected entries that are descendants of
    //   primPath.
    // Clear their dependency info out.
    // - Depended on prims that are descendants of primPath with affected
    //   entries that are *not* descendants of primPath.
    // Send out relevant dirty notices.
    WorkParallelForTBBRange(_dependedOnPrimToDependentsMap.range(),
    [&](const _DependedOnPrimsAffectedPrimsMap::range_type& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        _DependedOnPrimsAffectedPrimsMap::value_type& depPair = *it;
        WorkParallelForTBBRange(depPair.second.range(),
        [&](const _AffectedPrimsDependencyMap::range_type& range) {

        // Note: these are scoped to the parallel range (rather than the whole
        // depPair) so that they stay thread-local; depPair.first is constant
        // across the inner loop, so caching within the range is still a win.
        bool dependedOnChecked = false;
        bool dependedOnRemoved = false;

        for (auto it = range.begin(); it != range.end(); ++it) {
            _AffectedPrimsDependencyMap::value_type& affPair = *it;

            if (hasBeenRemoved(affPair.first))
            {
                affPair.second.flaggedForDeletion = true;
                _potentiallyDeletedDependedOnPaths.insert(depPair.first);
            }

            // We keep this condition separate, in case this has already been flagged for deletion
            // from elsewhere.
            if (affPair.second.flaggedForDeletion) {
                continue;
            }

            // Only check ONCE per "depPair.first" if the "depended on" was removed. This is either
            // true or false for all items in the inner loop.
            // Note: This could be moved out of the inner loop for the sake of simplicity, but it
            // is more optimal to do it here, after the early out, in case this code is not even
            // reached.
            if (!dependedOnChecked)
            {
                dependedOnRemoved = hasBeenRemoved(depPair.first);
                dependedOnChecked = true;
            }

            // Otherwise, if the affected prim remains but the depended on
            // prim was deleted, send dirty notices.
            if (dependedOnRemoved) {
                HdDataSourceLocatorSet affectedLocators;
                for (const auto &keyEntryPair :
                        affPair.second.locatorsEntryMap) {
                    const _LocatorsEntry &entry = keyEntryPair.second;
                    affectedLocators.insert(
                        entry.affectedDataSourceLocator);
                }

                if (!affectedLocators.IsEmpty()) {
                    additionalDirtied.emplace_back(affPair.first,
                        affectedLocators);
                    _PrimDirtied(affPair.first, affectedLocators,
                        &visited, &additionalDirtied, &rebuildDependencies);
                }
            }
        }});
    }});

    // Dependency rebuild done after _PrimDirtied for consistent semantics,
    // and so that we don't invalidate iterators everywhere...
    WorkParallelForTBBRange(rebuildDependencies.range(),
    [&](const _PathSet::range_type& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        SdfPath const& rebuildPath = *it;
        _ClearDependencies(rebuildPath);
        _UpdateDependencies(rebuildPath);
    }});

    if (!_manualGarbageCollect) {
        RemoveDeletedEntries(nullptr, nullptr);
    }

    _SendPrimsRemoved(entries);

    if (!additionalDirtied.empty()) {
        HdSceneIndexObserver::DirtiedPrimEntries flattened(
            additionalDirtied.begin(), additionalDirtied.end());
        _SendPrimsDirtied(flattened);
    }
}

void
HdDependencyForwardingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _VisitedNodeSet visited;
    _AdditionalDirtiedVector additionalEntries;
    _PathSet rebuildDependencies;

    static const HdDataSourceLocator &depsLoc =
            HdDependenciesSchema::GetDefaultLocator();

    WorkParallelForN(entries.size(),
    [&](size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
        const HdSceneIndexObserver::DirtiedPrimEntry &entry = entries[i];
        // Check if dependencies were directly invalidated.
        if (entry.dirtyLocators.Intersects(depsLoc)) {
            _VisitedNode visitedNode = {entry.primPath, depsLoc};
            if (visited.find(visitedNode) == visited.end()) {
                visited.insert(visitedNode);
                rebuildDependencies.insert(entry.primPath);
            }
        }
        _PrimDirtied(entry.primPath, entry.dirtyLocators,
            &visited, &additionalEntries, &rebuildDependencies);
    }});

    // Dependency rebuild done after _PrimDirtied for consistent semantics,
    // and so that we don't invalidate iterators everywhere...
    WorkParallelForTBBRange(rebuildDependencies.range(),
    [&](const _PathSet::range_type& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        SdfPath const& rebuildPath = *it;
        _ClearDependencies(rebuildPath);
        _UpdateDependencies(rebuildPath);
    }});

    if (!_manualGarbageCollect) {
        RemoveDeletedEntries(nullptr, nullptr);
    }

    if (additionalEntries.empty()) {
        _SendPrimsDirtied(entries);
    } else {
        HdSceneIndexObserver::DirtiedPrimEntries flattened(
            entries.begin(), entries.end());
        flattened.insert(flattened.end(), additionalEntries.begin(),
            additionalEntries.end());
        _SendPrimsDirtied(flattened);
    }
}

void
HdDependencyForwardingSceneIndex::_PrimDirtied(
    const SdfPath &primPath,
    const HdDataSourceLocatorSet &sourceLocatorSet,
    _VisitedNodeSet *visited,
    _AdditionalDirtiedVector *moreDirtiedEntries,
    _PathSet *rebuildDependencies)
{
    if (!TF_VERIFY(visited) || !TF_VERIFY(rebuildDependencies)) {
        return;
    }

    static const HdDataSourceLocator &depsLoc =
            HdDependenciesSchema::GetDefaultLocator();

    // check me in the reverse update table
    _DependedOnPrimsAffectedPrimsMap::iterator it =
        _dependedOnPrimToDependentsMap.find(primPath);
    if (it == _dependedOnPrimToDependentsMap.end()) {
        return;
    }

    WorkParallelForTBBRange(it->second.range(),
    [&](const _AffectedPrimsDependencyMap::range_type& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        const _AffectedPrimsDependencyMap::value_type& affectedPair = *it;
        const SdfPath &affectedPrimPath = affectedPair.first;
        HdDataSourceLocatorSet affectedLocators;

        // now dirty any dependencies
        for (const auto &keyEntryPair : affectedPair.second.locatorsEntryMap) {
            const _LocatorsEntry &entry = keyEntryPair.second;

            if (!sourceLocatorSet.Intersects(
                    entry.dependedOnDataSourceLocator)) {
                continue;
            }

            // Don't recurse if we've seen this invalidation before...
            _VisitedNode visitedNode =
                {affectedPrimPath, entry.affectedDataSourceLocator};
            if (visited->find(visitedNode) == visited->end()) {
                visited->insert(visitedNode);
                affectedLocators.insert(entry.affectedDataSourceLocator);
                if (entry.affectedDataSourceLocator == depsLoc) {
                    rebuildDependencies->insert(affectedPrimPath);
                }
                else if (entry.affectedDataSourceLocator.Intersects(depsLoc)) {
                    _VisitedNode depsNode = { affectedPrimPath, depsLoc };
                    if (visited->find(depsNode) == visited->end()) {
                        visited->insert(depsNode);
                        rebuildDependencies->insert(affectedPrimPath);
                    }
                }
            }
        }

        // Add the dependent invalidations & recurse
        if (!affectedLocators.IsEmpty()) {
            moreDirtiedEntries->emplace_back(affectedPrimPath,
                affectedLocators);

            _PrimDirtied(affectedPrimPath, affectedLocators,
                visited, moreDirtiedEntries, rebuildDependencies);
        }
    }});
}

//----------------------------------------------------------------------------

// when called?
// 1) when our own __dependencies are dirtied
// 2) when someone asks for our prim

void 
HdDependencyForwardingSceneIndex::_ClearDependencies(const SdfPath &primPath)
{
    _AffectedPrimToDependsOnPathsEntryMap::iterator it =
        _affectedPrimToDependsOnPathsMap.find(primPath);
    if (it == _affectedPrimToDependsOnPathsMap.end()) {
        return;
    }

    // Mark the affected entry for garbage collection.
    _AffectedPrimToDependsOnPathsEntry &affectedPrimEntry = (*it).second;
    affectedPrimEntry.flaggedForDeletion = true;
    _potentiallyDeletedAffectedPaths.insert(primPath);

    // Flag entries within our depended-on prims and add those prims to the
    // set of paths which should be checked during RemoveDeletedEntries
    const _PathSet &dependsOnPaths = affectedPrimEntry.dependsOnPaths;
    for (const SdfPath &dependedOnPrimPath : dependsOnPaths) {
        _DependedOnPrimsAffectedPrimsMap::iterator dependedOnPrimIt =
                _dependedOnPrimToDependentsMap.find(dependedOnPrimPath);

        if (dependedOnPrimIt == _dependedOnPrimToDependentsMap.end()) {
            continue;
        }

        _AffectedPrimsDependencyMap &_affectedPrimsMap =
            (*dependedOnPrimIt).second;


        _AffectedPrimsDependencyMap::iterator thisAffectedPrimEntryIt =
            _affectedPrimsMap.find(primPath);
        if (thisAffectedPrimEntryIt == _affectedPrimsMap.end()) {
            continue;
        }

        (*thisAffectedPrimEntryIt).second.flaggedForDeletion = true;
        _potentiallyDeletedDependedOnPaths.insert(dependedOnPrimPath);
    }
}

//----------------------------------------------------------------------------

void
HdDependencyForwardingSceneIndex::_UpdateDependencies(
    const SdfPath &primPath) const
{
    _UpdateDependencies(primPath, _GetInputSceneIndex()->GetPrim(primPath));
}

void
HdDependencyForwardingSceneIndex::_UpdateDependencies(
    const SdfPath &primPath,
    HdSceneIndexPrim const& prim) const
{
    if (!prim.dataSource) {
        return;
    }
    HdDependenciesSchema dependenciesSchema =
            HdDependenciesSchema::GetFromParent(prim.dataSource);

    // NOTE: This early exit prevents addition of an entry within
    //       _affectedPrimToDependsOnPathsMap if there isn't one already.
    //       The trade-off is repeatedly doing this check vs adding an entry
    //       for every prim which doesn't have dependencies.
    if (!dependenciesSchema.IsDefined()) {
        return;
    }

    // presence (even if empty) indicates we've been checked
    // NOTE: we only add to this set. We'll remove entries (and the map itself)
    //       as part of single-threaded clearing.
    
    _AffectedPrimToDependsOnPathsEntry &dependsOnPathsEntry = 
        _affectedPrimToDependsOnPathsMap[primPath];

    dependsOnPathsEntry.flaggedForDeletion = false;

    _PathSet &dependsOnPaths = dependsOnPathsEntry.dependsOnPaths;

    for (HdDependenciesSchema::EntryPair &entryPair :
            dependenciesSchema.GetEntries()) {

        TfToken &entryName = entryPair.first;
        HdDependencySchema &depSchema = entryPair.second;

        if (!depSchema.IsDefined()) {
           continue;
        }

        SdfPath dependedOnPrimPath;
        if (HdPathDataSourceHandle dependedOnPrimPathDataSource =
                depSchema.GetDependedOnPrimPath()) {
            dependedOnPrimPath =
                dependedOnPrimPathDataSource->GetTypedValue(0.0f);
        }

        HdDataSourceLocator dependedOnDataSourceLocator;
        HdDataSourceLocator affectedSourceLocator;

        if (HdLocatorDataSourceHandle lds =
                depSchema.GetDependedOnDataSourceLocator()) {
            dependedOnDataSourceLocator = lds->GetTypedValue(0.0f);
        }

        if (HdLocatorDataSourceHandle lds =
                depSchema.GetAffectedDataSourceLocator()) {
            affectedSourceLocator = lds->GetTypedValue(0.0f);
        }

        // self dependency
        if (dependedOnPrimPath.IsEmpty()) {
            dependedOnPrimPath = primPath;
        }

        dependsOnPaths.insert(dependedOnPrimPath);

        _AffectedPrimsDependencyMap &reverseDependencies =
            _dependedOnPrimToDependentsMap[dependedOnPrimPath];

        _AffectedPrimDependencyEntry &reverseDependenciesEntry =
            reverseDependencies[primPath];

        _LocatorsEntry &entry =
                reverseDependenciesEntry.locatorsEntryMap[entryName];
        entry.dependedOnDataSourceLocator = dependedOnDataSourceLocator;
        entry.affectedDataSourceLocator = affectedSourceLocator;

        reverseDependenciesEntry.flaggedForDeletion = false;
    }

}

// ---------------------------------------------------------------------------

void
HdDependencyForwardingSceneIndex::RemoveDeletedEntries(
    SdfPathVector *removedAffectedPrimPaths,
    SdfPathVector *removedDependedOnPrimPaths)
{
    for (const SdfPath &dependedOnPrimPath :
            _potentiallyDeletedDependedOnPaths) {

        _DependedOnPrimsAffectedPrimsMap::iterator dependedOnPrimIt = 
            _dependedOnPrimToDependentsMap.find(dependedOnPrimPath);

        if (dependedOnPrimIt == _dependedOnPrimToDependentsMap.end()) {
            continue;
        }

        _AffectedPrimsDependencyMap &affectedPrimsMap =
            (*dependedOnPrimIt).second;

        // Note: some prims have pretty wild fan-out, so we iterate over
        // whichever set of potentially affected prims is smaller...
        if (affectedPrimsMap.size() <=
                _potentiallyDeletedAffectedPaths.size()) {
            for (auto it = affectedPrimsMap.begin();
                 it != affectedPrimsMap.end(); ) {
                auto &affectedPrimPair = *it;
                if (affectedPrimPair.second.flaggedForDeletion) {
                    it = affectedPrimsMap.unsafe_erase(it);
                } else {
                    ++it;
                }
            }
        } else {
            for (auto &path : _potentiallyDeletedAffectedPaths) {
                auto it = affectedPrimsMap.find(path);
                if (it != affectedPrimsMap.end()) {
                    auto &affectedPrimPair = *it;
                    if (affectedPrimPair.second.flaggedForDeletion) {
                        affectedPrimsMap.unsafe_erase(it);
                    }
                }
            }
        }

        // If we've erased all of the affected entries, we need to erase
        // the dependedOn entry as well.
        if (affectedPrimsMap.empty()) {
            if (removedDependedOnPrimPaths) {
                removedDependedOnPrimPaths->push_back(dependedOnPrimPath);
            }

            _dependedOnPrimToDependentsMap.unsafe_erase(dependedOnPrimIt);
        }
    }

    for (const SdfPath &affectedPrimPath :
            _potentiallyDeletedAffectedPaths) {

        _AffectedPrimToDependsOnPathsEntryMap::iterator it =
            _affectedPrimToDependsOnPathsMap.find(affectedPrimPath);

        if (it == _affectedPrimToDependsOnPathsMap.end()) {
            continue;
        }

        if ((*it).second.flaggedForDeletion) {
            if (removedAffectedPrimPaths) {
                removedAffectedPrimPaths->push_back(affectedPrimPath);
            }
            _affectedPrimToDependsOnPathsMap.unsafe_erase(it);
        }
    }

    _potentiallyDeletedDependedOnPaths.clear();
    _potentiallyDeletedAffectedPaths.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
