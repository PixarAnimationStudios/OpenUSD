//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hd/dependencyForwardingSceneIndex.h"
#include "pxr/imaging/hd/dependenciesSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

//----------------------------------------------------------------------------

HdDependencyForwardingSceneIndex::HdDependencyForwardingSceneIndex(
    HdSceneIndexBaseRefPtr inputScene)
: HdSingleInputFilteringSceneIndexBase(inputScene)
{
}

HdSceneIndexPrim
HdDependencyForwardingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    if (_GetInputSceneIndex()) {

        if (_affectedPrimToDependsOnPathsMap.find(primPath) == 
                _affectedPrimToDependsOnPathsMap.end()) {
            _UpdateDependencies(primPath);
        }

        return _GetInputSceneIndex()->GetPrim(primPath);
    }

    return {TfToken(), nullptr};
}

SdfPathVector
HdDependencyForwardingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    // pass through without change
    if (_GetInputSceneIndex()) {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

    return {};
}

void
HdDependencyForwardingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _SendPrimsAdded(entries);
}

void
HdDependencyForwardingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{

    _VisitedNodeSet visited;
    HdSceneIndexObserver::DirtiedPrimEntries affectedEntries;

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        const SdfPath &primPath = entry.primPath;

        // Clear this prim's dependencies.
        _ClearDependencies(primPath);

        // If this prim is depended on, flag its map of affected paths/locators
        // for deletion. Also, send a dirty notice for each affected entry.
        // Note: The affected path/locator isn't notified explicitly of this 
        //       prim's removal. It needs to query the scene index and handle
        //       the absence of the prim to detect the removal.
        //
        _DependedOnPrimsAffectedPrimsMap::iterator it =
            _dependedOnPrimToDependentsMap.find(primPath);

        if (it != _dependedOnPrimToDependentsMap.end()) {
            _potentiallyDeletedDependedOnPaths.insert(primPath);

            for (auto &affectedPair : (*it).second) {
                affectedPair.second.flaggedForDeletion = true;

                const SdfPath &affectedPrimPath = affectedPair.first;

                for (const auto &keyEntryPair :
                        affectedPair.second.locatorsEntryMap) {
                    const _LocatorsEntry &entry = keyEntryPair.second;

                    _PrimDirtied(affectedPrimPath,
                                 entry.affectedDataSourceLocator,
                                 &visited, &affectedEntries);
                }
            }
        }
    }

    _SendPrimsRemoved(entries);

    if (!affectedEntries.empty()) {
        _SendPrimsDirtied(affectedEntries);
    }
}

void
HdDependencyForwardingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _VisitedNodeSet visited;
    HdSceneIndexObserver::DirtiedPrimEntries affectedEntries;

    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        for (const HdDataSourceLocator &sourceLocator : entry.dirtyLocators) {
            _PrimDirtied(entry.primPath, sourceLocator, &visited,
                &affectedEntries);
        }
    }

    if (affectedEntries.empty()) {
        _SendPrimsDirtied(entries);
    } else {
        affectedEntries.insert(affectedEntries.begin(),
            entries.begin(), entries.end());
        _SendPrimsDirtied(affectedEntries);
    }
}


void
HdDependencyForwardingSceneIndex::_PrimDirtied(
    const SdfPath &primPath,
    const HdDataSourceLocator &sourceLocator,
    _VisitedNodeSet *visited,
    HdSceneIndexObserver::DirtiedPrimEntries *moreDirtiedEntries)
{
    if (TF_VERIFY(visited)) {
        _VisitedNode node = {primPath, sourceLocator};

        if (visited->find(node) != visited->end()) {
            return;
        }

        visited->insert(node);
    }

    moreDirtiedEntries->emplace_back(primPath, sourceLocator);

    // check to see if dependencies are dirty and should be recomputed
    const HdDataSourceLocator &depsLoc =
            HdDependenciesSchema::GetDefaultLocator();
    if (sourceLocator.Intersects(depsLoc)) {
        if (TF_VERIFY(visited)) {
            _VisitedNode dependenciesNode = {primPath, depsLoc};
            if (visited->find(dependenciesNode) == visited->end()) {
                visited->insert(dependenciesNode);
                _ClearDependencies(primPath);
                _UpdateDependencies(primPath);
            }
        }
    }


    // check me in the reverse update table
    // now dirty any dependencies
    _DependedOnPrimsAffectedPrimsMap::const_iterator it =
        _dependedOnPrimToDependentsMap.find(primPath);
    if (it == _dependedOnPrimToDependentsMap.end()) {
        return;
    }

    for (const auto &affectedPair : (*it).second) {
        const SdfPath &affectedPrimPath = affectedPair.first;

        for (const auto &keyEntryPair : affectedPair.second.locatorsEntryMap) {
            const _LocatorsEntry &entry = keyEntryPair.second;

            if (entry.dependedOnDataSourceLocator.Intersects(sourceLocator)) {
                _PrimDirtied(affectedPrimPath, entry.affectedDataSourceLocator,
                        visited, moreDirtiedEntries);
            }
        }
    }
}

//----------------------------------------------------------------------------

// when called?
// 1) when our own __dependencies are dirtied
// 2) when someone asks for our prim

void 
HdDependencyForwardingSceneIndex::_ClearDependencies(const SdfPath &primPath)
{
    _AffectedPrimToDependsOnPathsEntryMap::const_iterator it =
        _affectedPrimToDependsOnPathsMap.find(primPath);
    if (it == _affectedPrimToDependsOnPathsMap.end()) {
        return;
    }

    _AffectedPrimToDependsOnPathsEntry &affectedPrimEntry = (*it).second;

    affectedPrimEntry.flaggedForDeletion = true;

    const _PathSet &dependsOnPaths = affectedPrimEntry.dependsOnPaths;

    // If we know we are clearing an already empty one, add it to the set
    // of potential deletions. If it's not empty, we'll be represented
    // by adding our dependedOn paths as removal of those clears the
    // affected prim paths which are made empty as result.
    if (dependsOnPaths.empty()) {
        _potentiallyDeletedAffectedPaths.insert(primPath);
    }

    // Flag entries within our depended-on prims and add those prims to the
    // set of paths which should be checked during RemoveDeletedEntries
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
    if (!_GetInputSceneIndex()) {
        return;
    }

    HdContainerDataSourceHandle primDataSource =
        _GetInputSceneIndex()->GetPrim(primPath).dataSource;

    if (!primDataSource) {
        return;
    }

    HdDependenciesSchema dependenciesSchema =
            HdDependenciesSchema::GetFromParent(primDataSource);

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
    SdfPathVector entriesToRemove;

    for (const SdfPath &dependedOnPrimPath :
            _potentiallyDeletedDependedOnPaths) {

        _DependedOnPrimsAffectedPrimsMap::iterator dependedOnPrimIt = 
            _dependedOnPrimToDependentsMap.find(dependedOnPrimPath);

        if (dependedOnPrimIt == _dependedOnPrimToDependentsMap.end()) {
            continue;
        }

        _AffectedPrimsDependencyMap &_affectedPrimsMap =
            (*dependedOnPrimIt).second;

        entriesToRemove.clear();

        for (auto &affectedPrimPair : _affectedPrimsMap) {
            const SdfPath &affectedPrimPath = affectedPrimPair.first;
            _AffectedPrimDependencyEntry &affectedPrimDependencyEntry =
                    affectedPrimPair.second;

            if (!affectedPrimDependencyEntry.flaggedForDeletion) {
                continue;
            }

            entriesToRemove.push_back(affectedPrimPath);


            // now remove dependedOn prim from affected prim entry
            // if that removal leaves it empty, then remove the whole thing

            _AffectedPrimToDependsOnPathsEntryMap::iterator affectedPrimIt = 
                _affectedPrimToDependsOnPathsMap.find(affectedPrimPath);

            if (affectedPrimIt == _affectedPrimToDependsOnPathsMap.end()) {
                continue;
            }

            _AffectedPrimToDependsOnPathsEntry &affectedPrimEntry =
                    (*affectedPrimIt).second;

            if (affectedPrimEntry.dependsOnPaths.find(dependedOnPrimPath)
                    == affectedPrimEntry.dependsOnPaths.end()) {
                continue;
            }

            if (affectedPrimEntry.dependsOnPaths.size() == 1) {
                // If I'm the only thing in there, remove the whole entry
                _affectedPrimToDependsOnPathsMap.unsafe_erase(
                        affectedPrimPath);

                if (removedAffectedPrimPaths) {
                    removedAffectedPrimPaths->push_back(affectedPrimPath);
                }

            
            } else {
                affectedPrimEntry.dependsOnPaths.unsafe_erase(
                        dependedOnPrimPath);
            }
        }

        if (entriesToRemove.size() == _affectedPrimsMap.size()) {
            // removing everything?, just erase the dependedOn prim entry
            _dependedOnPrimToDependentsMap.unsafe_erase(dependedOnPrimPath);

            if (removedDependedOnPrimPaths) {
                removedDependedOnPrimPaths->push_back(dependedOnPrimPath);
            }

        } else {
            for (const SdfPath &affectedPrimPath : entriesToRemove) {
                _affectedPrimsMap.unsafe_erase(affectedPrimPath);
            }
        }
    }


    for (const SdfPath &affectedPrimPath :
            _potentiallyDeletedAffectedPaths) {

        // anything in here which flagged for deletion (XXX should it need to be 
        // empty too?)

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
