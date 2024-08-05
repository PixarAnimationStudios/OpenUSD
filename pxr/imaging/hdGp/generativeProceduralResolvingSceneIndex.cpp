//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "generativeProceduralResolvingSceneIndex.h"
#include "generativeProceduralPluginRegistry.h"

#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/systemMessages.h"

#include "pxr/base/tf/denseHashSet.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"

PXR_NAMESPACE_OPEN_SCOPE

/*static*/
HdGpGenerativeProcedural *
HdGpGenerativeProceduralResolvingSceneIndex::_ConstructProcedural(
    const TfToken &typeName, const SdfPath &proceduralPrimPath)
{
    return HdGpGenerativeProceduralPluginRegistry::GetInstance()
        .ConstructProcedural(typeName, proceduralPrimPath);
}

HdGpGenerativeProceduralResolvingSceneIndex::
        HdGpGenerativeProceduralResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _targetPrimTypeName(HdGpGenerativeProceduralTokens->generativeProcedural)
, _attemptAsync(false)
{
}

HdGpGenerativeProceduralResolvingSceneIndex::
        HdGpGenerativeProceduralResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const TfToken &targetPrimTypeName)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _targetPrimTypeName(targetPrimTypeName)
, _attemptAsync(false)
{
}

/* virtual */
HdSceneIndexPrim
HdGpGenerativeProceduralResolvingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{

    const auto it = _generatedPrims.find(primPath);
    if (it != _generatedPrims.end()) {
        if (_ProcEntry *procEntry = it->second.responsibleProc.load()) {
            
            // need to exclude prim-level deal itself from the returned value


            if (std::shared_ptr<HdGpGenerativeProcedural> proc =
                    procEntry->proc) {
                return proc->GetChildPrim(
                    _GetInputSceneIndex(), primPath);
            }
        }
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.primType == _targetPrimTypeName) {
        // TODO? confirm it's cooked?
        //_Notices notices; 
        //_UpdateProcedural(primPath, false, &notices);

        prim.primType = HdGpGenerativeProceduralTokens->resolvedGenerativeProcedural;
    }

    return prim;
}

// Adds unique elements from the cached child prim paths to a vector
/*static*/
void
HdGpGenerativeProceduralResolvingSceneIndex::_CombinePathArrays(
    const _DensePathSet &s, SdfPathVector *v)
{
    if (v->empty()) {
        v->insert(v->begin(), s.begin(), s.end());
        return;
    }
    _DensePathSet uniqueValues(v->begin(), v->end());
    for (const SdfPath &p : s) {
        if (uniqueValues.find(p) == uniqueValues.end()) {
            v->push_back(p);
        }
    }
}

/* virtual */
SdfPathVector
HdGpGenerativeProceduralResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    // Always incorporate the input's children even if we are beneath a
    // resolved procedural. This allows a procedural to mask the type or data
    // of an existing descendent (by returning it from Update) or to let it
    // go through unmodified.
    SdfPathVector inputResult =
        _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    // Check to see if the requested path already exists as a prim managed by
    // a procedural. Look up what the procedural added and potentially combine
    // with what might be present on the input scene.
    //
    // XXX: This doesn't cause a procedural to be run at an ancestor path --
    //      so we'd expect a notice-less traversal case to have already called
    //      GetChildPrimPaths with the parent procedural. The overhead of
    //      ensuring that happens for every scope outweighs the unlikely
    //      possibility of incorrect results for a speculative query without
    //      hitting any of the existing triggers.
    const auto it = _generatedPrims.find(primPath);
    if (it != _generatedPrims.end()) {
        if (_ProcEntry *procEntry = it->second.responsibleProc.load()) {
            std::unique_lock<std::mutex> cookLock(procEntry->cookMutex);
            const auto chIt = procEntry->childHierarchy.find(primPath);
            if (chIt != procEntry->childHierarchy.end()) {

                _CombinePathArrays(chIt->second, &inputResult);
                return inputResult;
            }
        }
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.primType == _targetPrimTypeName) {
        _Notices notices;

        // cook if necessary to find child prim paths. Do not forward notices
        // as use of this API implies a non-notice-driven traversal.
        if (_ProcEntry *procEntry =
                _UpdateProcedural(primPath, false, &notices)) {

            std::unique_lock<std::mutex> cookLock(procEntry->cookMutex);
            const auto hIt = procEntry->childHierarchy.find(primPath);
            if (hIt != procEntry->childHierarchy.end()) {
                _CombinePathArrays(hIt->second, &inputResult);
            }
        }
    }

    return inputResult;
}

/* virtual */
void
HdGpGenerativeProceduralResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // Added/removed/dirtied notices which result from cooking or recooking
    // a procedural.
    _Notices notices;

    TfDenseHashSet<SdfPath, TfHash> proceduralsToCook;

    bool entriesCopied = false;

    { // _dependencies and _procedural lock acquire
    // hold lock for longer but don't try to acquire it per iteration
    _MapLock procsLock(_proceduralsMutex);
    _MapLock depsLock(_dependenciesMutex);

    for (auto it = entries.begin(), e = entries.end(); it != e; ++it) {
        const HdSceneIndexObserver::AddedPrimEntry &entry = *it;
        if (entry.primPath.IsAbsoluteRootPath()) {
            continue;
        }

        if (entry.primType == _targetPrimTypeName) {
            if (!entriesCopied) {
                entriesCopied = true;
                notices.added.insert(notices.added.end(), entries.begin(), it);
            }
            notices.added.emplace_back(
                entry.primPath,
                HdGpGenerativeProceduralTokens->resolvedGenerativeProcedural);

            // force an update since an add of an existing prim is
            // considered a full invalidation as it may change type
            proceduralsToCook.insert(entry.primPath);

        } else {
            if (_procedurals.find(entry.primPath) != _procedurals.end()) {
                // This was a procedural that we previously cooked that is no
                // longer the target type.  We "cook" it primarily to make sure
                // it gets removed.
                proceduralsToCook.insert(entry.primPath);
            }
            if (entriesCopied) {
                notices.added.emplace_back(entry.primPath, entry.primType);
            }
        }

        // We've already skipped the case where entry.primPath is the absolute
        // root, so GetParentPath() makes sense here.
        const SdfPath entryPrimParentPath = entry.primPath.GetParentPath();
        // NOTE: potentially share code with primsremoved
        _DependencyMap::const_iterator dIt =
            _dependencies.find(entryPrimParentPath);
        if (dIt != _dependencies.end()) {
            for (const SdfPath &dependentPath : dIt->second) {
                // don't bother checking a procedural which already scheduled
                if (proceduralsToCook.find(dependentPath) !=
                       proceduralsToCook.end()) {
                    continue;
                }

                _ProcEntryMap::const_iterator procIt =
                    _procedurals.find(dependentPath);
                if (procIt == _procedurals.end()) {
                    continue;
                }

                const _ProcEntry &procEntry = procIt->second;
                const auto dslIt =
                    procEntry.dependencies.find(entryPrimParentPath);
                
                if (dslIt == procEntry.dependencies.end()) {
                    continue;
                }
                
                if (dslIt->second.Intersects(HdGpGenerativeProcedural::
                        GetChildNamesDependencyKey())) {
                    proceduralsToCook.insert(dependentPath);
                    // TODO consider providing this dependency set
                    // to send to _UpdateProcedural. Currently removals
                    // don't bother to track individual procedurals
                }
            }
        }
    }

    } // _dependencies and _procedural lock release

    if (!proceduralsToCook.empty()) {
        const size_t parallelThreshold = 2;

        if (proceduralsToCook.size() >= parallelThreshold) {
            using _CookEntry = std::pair<SdfPath, _Notices>;
            TfSmallVector<_CookEntry, 16> cookEntries;
            cookEntries.resize(proceduralsToCook.size());
            size_t i = 0;
            for (const SdfPath &p : proceduralsToCook) {
                cookEntries[i].first = p;
                ++i;
            }

            {
            TF_PY_ALLOW_THREADS_IN_SCOPE();
            WorkWithScopedParallelism([&]() {
            WorkParallelForEach(cookEntries.begin(), cookEntries.end(),
                    [this](const _CookEntry &e) {
                this->_UpdateProcedural(e.first, true, const_cast<
                    HdGpGenerativeProceduralResolvingSceneIndex::_Notices *>(
                        &e.second));
            });
            });
            }

            // combine all of the resulting notices following parallel cook
            for (const _CookEntry &e : cookEntries) {
                notices.added.insert(notices.added.end(),
                    e.second.added.begin(), e.second.added.end());
                notices.removed.insert(notices.removed.end(),
                    e.second.removed.begin(), e.second.removed.end());
                notices.dirtied.insert(notices.dirtied.end(),
                    e.second.dirtied.begin(), e.second.dirtied.end());
            }
        } else {
            for (const SdfPath &p : proceduralsToCook) {
                _UpdateProcedural(p, true, &notices);
            }
        }
    }

    if (!entriesCopied) {
        _SendPrimsAdded(entries);
    } else {
        _SendPrimsAdded(notices.added);
    }

    if (!notices.removed.empty()) {
        _SendPrimsRemoved(notices.removed);
    }

    if (!notices.dirtied.empty()) {
        _SendPrimsDirtied(notices.dirtied);
    }
}

/* virtual */
void
HdGpGenerativeProceduralResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    using _PathSetMap =
         TfDenseHashMap<SdfPath, TfDenseHashSet<SdfPath, TfHash>, TfHash>;

    // Pre-seed lookups to handle invalidating procedurals when the ancestor of
    // a dependency is removed.
    // NOTE: Doing this once per-vectorized batch is preferable to looping per
    //       entry element -- but only if meaningful batching is happening
    //       upstream.
    _PathSetMap dependencyAncestors;
    {
        _MapLock depsLock(_dependenciesMutex);
        for (const auto &pathEntryPair : _dependencies) {
            const SdfPath &path = pathEntryPair.first;
            for (const SdfPath &parentPath : path.GetAncestorsRange()) {
                dependencyAncestors[parentPath].insert(path);
            }
        }
    }

    // Pre-seed lookups to handle clearing our cache of previously cooked data
    // for when the ancestor of a procedural is removed.
    // NOTE: Doing this once per-vectorized batch is preferable to looping per
    //       entry element -- but only if meaningful batching is happening
    //       upstream.
    _PathSetMap procAncestors;
    {
        _MapLock procsLock(_proceduralsMutex);
        for (const auto &pathEntryPair : _procedurals) {
            const SdfPath &path = pathEntryPair.first;
            for (const SdfPath &parentPath : path.GetAncestorsRange()) {
                procAncestors[parentPath].insert(path);
            }
        }
    }

    // 1) if what's removed is a dependency, we need to dirty the dependents
    // 2) if what's removed in a procedural, we need to remove the cooked
    //    record of it as well as its dependency entry
    TfDenseHashSet<SdfPath, TfHash> removedDependencies;
    TfDenseHashSet<SdfPath, TfHash> invalidatedProcedurals;
    TfDenseHashSet<SdfPath, TfHash> removedProcedurals;

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        _PathSetMap::const_iterator it =
            dependencyAncestors.find(entry.primPath);
        if (it != dependencyAncestors.end()) {
            for (const SdfPath &dependencyPath : it->second) {
                _DependencyMap::const_iterator dIt =
                    _dependencies.find(dependencyPath);
                if (dIt != _dependencies.end()) {
                    removedDependencies.insert(dependencyPath);

                    for (const SdfPath &dependentPath : dIt->second) {
                        // don't invalidate procedurals which know are directly
                        // removed.
                        if (removedProcedurals.find(dependentPath) ==
                                removedProcedurals.end()) {
                            invalidatedProcedurals.insert(dependentPath);
                        }
                    }
                }
            }
        } else {
            // check if parent path is a dependency with childNames
            _DependencyMap::const_iterator dIt =
                _dependencies.find(entry.primPath.GetParentPath());
            if (dIt != _dependencies.end()) {
                for (const SdfPath &dependentPath : dIt->second) {

                    // don't bother checking a procedural slated for removal
                    if (removedProcedurals.find(dependentPath) !=
                           removedProcedurals.end()) {
                        continue;
                    }

                    _ProcEntryMap::const_iterator procIt =
                            _procedurals.find(dependentPath);
                    if (procIt == _procedurals.end()) {
                        continue;
                    }

                    const _ProcEntry &procEntry = procIt->second;
                    const auto dslIt = procEntry.dependencies.find(
                            entry.primPath.GetParentPath());
                    if (dslIt == procEntry.dependencies.end()) {
                        continue;
                    }

                    if (dslIt->second.Intersects(HdGpGenerativeProcedural::
                            GetChildNamesDependencyKey())) {
                        invalidatedProcedurals.insert(dependentPath);
                        // TODO consider providing this dependency set
                        // to send to _UpdateProcedural. Currently removals
                        // don't bother to track individual procedurals
                    }
                }
            }
        }

        it = procAncestors.find(entry.primPath);
        if (it != procAncestors.end()) {
            for (const SdfPath &procPath : it->second) {
                removedProcedurals.insert(procPath);
                // disregard any previously invalidated procedurals as removal
                // means we don't need to invalidate
                invalidatedProcedurals.erase(procPath);
            }
        }
    }

    if (!removedDependencies.empty()) {
        _MapLock depsLock(_dependenciesMutex);
        for (const SdfPath &dependencyPath : removedDependencies) {
            _dependencies.erase(dependencyPath);
        }
    }

    if (!removedProcedurals.empty()) {
        for (const SdfPath &removedProcPath : removedProcedurals) {
            _RemoveProcedural(removedProcPath);
        }
    }

    if (!invalidatedProcedurals.empty()) {
        _Notices notices;
        notices.removed = entries;

        // NOTE, we are not bothering to provide precise invalidation
        //       since the removal of a dependency is likely broad enough
        //       to indicate that all dependencies are dirty. If this is
        //       insufficient, we could collect info similarly to
        //       _PrimsDirtied

        const size_t parallelThreshold = 2;
        if (invalidatedProcedurals.size() >= parallelThreshold) {
            using _CookEntry = std::pair<SdfPath, _Notices>;
            TfSmallVector<_CookEntry, 16> cookEntries;
            cookEntries.resize(invalidatedProcedurals.size());
            size_t i = 0;
            for (const SdfPath &p : invalidatedProcedurals) {
                cookEntries[i].first = p;
                ++i;
            }

            {
            TF_PY_ALLOW_THREADS_IN_SCOPE();
            WorkWithScopedParallelism([&]() {
            WorkParallelForEach(cookEntries.begin(), cookEntries.end(),
                    [this](const _CookEntry &e) {
                this->_UpdateProcedural(e.first, true, const_cast<
                    HdGpGenerativeProceduralResolvingSceneIndex::_Notices *>(
                        &e.second));
            });
            });
            }

            // combine all of the resulting notices following parallel cook
            for (const _CookEntry &e : cookEntries) {
                notices.added.insert(notices.added.end(),
                    e.second.added.begin(), e.second.added.end());
                notices.removed.insert(notices.removed.end(),
                    e.second.removed.begin(), e.second.removed.end());
                notices.dirtied.insert(notices.dirtied.end(),
                    e.second.dirtied.begin(), e.second.dirtied.end());
            }

        } else {
            for (const SdfPath &invalidatedProcPath : invalidatedProcedurals) {
                // Procedurals here are cooked serially
                _UpdateProcedural(invalidatedProcPath, true, &notices);
            }
        }

        if (!notices.added.empty()) {
            _SendPrimsAdded(notices.added);
        }

        _SendPrimsRemoved(notices.removed);

        if (!notices.dirtied.empty()) {
            _SendPrimsDirtied(notices.dirtied);
        }

    } else {
        _SendPrimsRemoved(entries);
    }
}

/* virtual */
void
HdGpGenerativeProceduralResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    TfDenseHashMap<SdfPath, HdGpGenerativeProcedural::DependencyMap, TfHash>
        invalidatedProceduralDependencies;

    {
        // hold lock for longer but don't try to acquire it per iteration
        _MapLock procsLock(_proceduralsMutex);
        _MapLock depsLock(_dependenciesMutex);

        for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
            if (_procedurals.find(entry.primPath) != _procedurals.end()) {
                invalidatedProceduralDependencies[entry.primPath][
                            entry.primPath].insert(entry.dirtyLocators);
            }

            _DependencyMap::const_iterator it =
                _dependencies.find(entry.primPath);
            if (it == _dependencies.end()) {
                continue;
            }

            for (const SdfPath &dependentPath : it->second) {
                _ProcEntryMap::const_iterator procIt =
                        _procedurals.find(dependentPath);
                if (procIt == _procedurals.end()) {
                    continue;
                }

                const _ProcEntry &procEntry = procIt->second;
                const auto dslIt = procEntry.dependencies.find(entry.primPath);
                if (dslIt != procEntry.dependencies.end()) {
                    if (entry.dirtyLocators.Intersects(dslIt->second)) {
                        invalidatedProceduralDependencies[dependentPath][
                            entry.primPath].insert(entry.dirtyLocators);
                    }
                }
            }
        }
    }

    if (!invalidatedProceduralDependencies.empty()) {
        _Notices notices;
        notices.dirtied = entries;
        HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries = entries;


        const size_t parallelThreshold = 2;
        if (invalidatedProceduralDependencies.size() >= parallelThreshold) {
            struct _CookEntry
            {
                SdfPath path;
                _Notices notices;
                const HdGpGenerativeProcedural::DependencyMap *deps;
            };

            TfSmallVector<_CookEntry, 16> cookEntries;
            cookEntries.resize(invalidatedProceduralDependencies.size());
            size_t i = 0;
            for (const auto &pathDepsPair : invalidatedProceduralDependencies) {
                cookEntries[i].path = pathDepsPair.first;
                cookEntries[i].deps = &pathDepsPair.second;
                ++i;
            }

            {
            TF_PY_ALLOW_THREADS_IN_SCOPE();
            WorkWithScopedParallelism([&]() {
            WorkParallelForEach(cookEntries.begin(), cookEntries.end(),
                    [this](const _CookEntry &e) {
                this->_UpdateProcedural(e.path, true, const_cast<
                    HdGpGenerativeProceduralResolvingSceneIndex::_Notices *>(
                        &e.notices), e.deps);
            });
            });
            }

            // combine all of the resulting notices following parallel cook
            for (const _CookEntry &e : cookEntries) {
                notices.added.insert(notices.added.end(),
                    e.notices.added.begin(), e.notices.added.end());
                notices.removed.insert(notices.removed.end(),
                    e.notices.removed.begin(), e.notices.removed.end());
                notices.dirtied.insert(notices.dirtied.end(),
                    e.notices.dirtied.begin(), e.notices.dirtied.end());
            }

        } else {
            for (const auto &pathDepsPair : invalidatedProceduralDependencies) {
                _UpdateProcedural(pathDepsPair.first, true, &notices,
                        &pathDepsPair.second);
            }
        }

        if (!notices.added.empty()) {
            _SendPrimsAdded(notices.added);
        }
        if (!notices.removed.empty()) {
            _SendPrimsRemoved(notices.removed);
        }

        _SendPrimsDirtied(notices.dirtied);
    } else {
        _SendPrimsDirtied(entries);
    }
}

HdGpGenerativeProceduralResolvingSceneIndex::_ProcEntry *
HdGpGenerativeProceduralResolvingSceneIndex::_UpdateProceduralDependencies(
    const SdfPath& proceduralPrimPath, _Notices* outputNotices) const
{
    HdSceneIndexPrim procPrim =
        _GetInputSceneIndex()->GetPrim(proceduralPrimPath);

    if (procPrim.primType != _targetPrimTypeName) {
        _RemoveProcedural(proceduralPrimPath, outputNotices);
        return nullptr;
    }

    _ProcEntry *procEntryPtr = nullptr;
    {
        _MapLock procsLock(_proceduralsMutex);
        procEntryPtr = &_procedurals[proceduralPrimPath];
    }
    _ProcEntry &procEntry = *procEntryPtr;

    if (procEntry.state.load() >= _ProcEntry::StateDependenciesCooked) {
        return procEntryPtr;
    }

    TfToken procType;

    HdPrimvarsSchema primvars =
        HdPrimvarsSchema::GetFromParent(procPrim.dataSource);

    HdSampledDataSourceHandle procTypeDs = primvars.GetPrimvar(
        HdGpGenerativeProceduralTokens->proceduralType).GetPrimvarValue();

    if (procTypeDs) {
        VtValue v = procTypeDs->GetValue(0.0f);
        if (v.IsHolding<TfToken>()) {
            procType = v.UncheckedGet<TfToken>();
        }
    }

    std::shared_ptr<HdGpGenerativeProcedural> proc;

    if (!procEntry.proc || procType != procEntry.typeName) {
        proc.reset(
            _ConstructProcedural(procType, proceduralPrimPath));

        if (proc) {
            bool result = proc->AsyncBegin(_attemptAsync);
            if (_attemptAsync && result) {
                _activeSyncProcedurals[proceduralPrimPath] =
                    TfCreateWeakPtr(&procEntry);
            }
        }

    } else {

        // give the procedural a chance to become asychronous following an
        // update if we aren't already
        if (procEntry.proc && _attemptAsync &&
                _activeSyncProcedurals.find(proceduralPrimPath)
                    == _activeSyncProcedurals.end()) {
            if (procEntry.proc->AsyncBegin(true)) {
                _activeSyncProcedurals[proceduralPrimPath] =
                    TfCreateWeakPtr(&procEntry);
            }
        }

        proc = procEntry.proc;
    }

    HdGpGenerativeProcedural::DependencyMap newDependencies;

    if (proc) {
        newDependencies = proc->UpdateDependencies(_GetInputSceneIndex());
    }

    _ProcEntry::State current = _ProcEntry::StateUncooked;
    if (procEntry.state.compare_exchange_strong(
            current, _ProcEntry::StateDependenciesCooking)) {
        
        if (procEntry.proc != proc) {
            procEntry.proc = proc;
        }

        procEntry.typeName = procType;

        // compare old and new dependency maps
        _PathSet dependencesToRemove;
        for (const auto& pathLocatorsPair : procEntry.dependencies) {
            const SdfPath &dependencyPath = pathLocatorsPair.first;
            if (newDependencies.find(dependencyPath) == newDependencies.end()) {
                dependencesToRemove.insert(dependencyPath);
            }
        }

        if (!newDependencies.empty() || !dependencesToRemove.empty()) {
            _MapLock depsLock(_dependenciesMutex);

            for (const auto& pathLocatorsPair : newDependencies) {
                const SdfPath &dependencyPath = pathLocatorsPair.first;
                if (procEntry.dependencies.find(dependencyPath)
                        == procEntry.dependencies.end()) {
                    _dependencies[dependencyPath].insert(proceduralPrimPath);
                }
            }
            for (const SdfPath &dependencyPath : dependencesToRemove) {
                _DependencyMap::iterator it =
                    _dependencies.find(dependencyPath);
                if (it != _dependencies.end()) {
                    it->second.erase(proceduralPrimPath);
                    if (it->second.empty()) {
                        _dependencies.erase(it);
                    }
                }
            }
        }

        procEntry.dependencies = std::move(newDependencies);
        procEntry.state.store(_ProcEntry::StateDependenciesCooked);
    }

    return procEntryPtr;
}

HdGpGenerativeProceduralResolvingSceneIndex::_ProcEntry *
HdGpGenerativeProceduralResolvingSceneIndex::_UpdateProcedural(
    const SdfPath &proceduralPrimPath,
    bool forceUpdate,
    _Notices *outputNotices,
    const HdGpGenerativeProcedural::DependencyMap *dirtiedDependencies) const
{
    TRACE_FUNCTION();

    _ProcEntry *procEntryPtr;
    {
        _MapLock procsLock(_proceduralsMutex);
        procEntryPtr = &_procedurals[proceduralPrimPath];
    }
    _ProcEntry &procEntry = *procEntryPtr;

    if (forceUpdate) {
        procEntry.state.store(_ProcEntry::StateUncooked);
    }

    if (procEntry.state.load() < _ProcEntry::StateDependenciesCooked) {
        if (!_UpdateProceduralDependencies(proceduralPrimPath, outputNotices)) {
            return nullptr;
        }
    }

    if (procEntry.state.load() >= _ProcEntry::StateCooked) {
        return procEntryPtr;
    }

    if (!procEntry.proc) {
        return procEntryPtr;
    }

    // if a dirtied dependency map is provided, use that for more specificity,
    // otherwise send in full set of dependencies
    const HdGpGenerativeProcedural::DependencyMap &localDirtiedDependencies =
        dirtiedDependencies ? *dirtiedDependencies : procEntry.dependencies;

    // TODO, move this within the compare_exchange_strong so that only one
    //       side cooks or add pre-proc entry mutex
    HdGpGenerativeProcedural::ChildPrimTypeMap newChildTypes =
        procEntry.proc->Update(
            _GetInputSceneIndex(),
            procEntry.childTypes,
            localDirtiedDependencies,
            outputNotices ? &outputNotices->dirtied : nullptr);

    _ProcEntry::State current = _ProcEntry::StateDependenciesCooked;
    if (procEntry.state.compare_exchange_strong(
            current, _ProcEntry::StateCooking)) {

        std::unique_lock<std::mutex> cookLock(procEntry.cookMutex);

        _UpdateProceduralResult(
             &procEntry, proceduralPrimPath, newChildTypes, outputNotices);

         procEntry.state.store(_ProcEntry::StateCooked);

    } else {
        std::unique_lock<std::mutex> cookLock(procEntry.cookMutex);
    }

    return procEntryPtr;
}

void
HdGpGenerativeProceduralResolvingSceneIndex::_RemoveProcedural(
    const SdfPath &proceduralPrimPath,
    _Notices *outputNotices) const
{
    _MapLock procsLock(_proceduralsMutex);

    auto it = _procedurals.find(proceduralPrimPath);
    if (it == _procedurals.end()) {
        return;
    }

    const _ProcEntry &procEntry = it->second;

    // 0) Before we clear things out, record the children that we'll need to
    // notify that are being removed.
    if (outputNotices) {
        // Record the removal the children of the procedural.
        size_t procPathLen = proceduralPrimPath.GetPathElementCount();
        for (const auto& pathPathSetPair : procEntry.childHierarchy) {
            const SdfPath& childPrimPath = pathPathSetPair.first;
            const bool isImmediateChild
                = childPrimPath.GetPathElementCount() == procPathLen + 1;
            if (isImmediateChild) {
                outputNotices->removed.push_back(childPrimPath);
            }
        }
    }

    // 1) remove existing dependencies
    if (!procEntry.dependencies.empty()) {

        _MapLock depsLock(_dependenciesMutex);

        for (const auto& pathLocatorsPair : procEntry.dependencies) {
            _DependencyMap::iterator dIt =
                _dependencies.find(pathLocatorsPair.first);

            if (dIt != _dependencies.end()) {
                dIt->second.erase(proceduralPrimPath);
                if (dIt->second.empty()) {

                    _dependencies.erase(dIt);
                }
            }
        }
    }

    // 2) remove record of generated prims
    if (!procEntry.childTypes.empty()) {

        for (const auto& pathTypePair : procEntry.childTypes) {
            const auto gpIt = _generatedPrims.find(pathTypePair.first);
            if (gpIt != _generatedPrims.end()) {
                gpIt->second.responsibleProc.store(nullptr);
            }
        }

        // childHierarchy may contain intermediate prims not directly
        // specified
        for (const auto& pathPathSetPair : procEntry.childHierarchy) {
            const auto gpIt = _generatedPrims.find(pathPathSetPair.first);
            if (gpIt != _generatedPrims.end()) {
                gpIt->second.responsibleProc.store(nullptr);
            }
        }
    }

    // 3) remove procEntry itself
    {
        _procedurals.erase(it);
    }
}

// XXX Does thread-unsafe deletion.
// Removes deleted entries from _generatedPrims.
// This is private for now but intended for future use by a discussed formal
// method on HdSceneIndexBase itself.
void
HdGpGenerativeProceduralResolvingSceneIndex::_GarbageCollect()
{
    auto it = _generatedPrims.begin();
    while (it != _generatedPrims.end()) {

        if (!it->second.responsibleProc.load()) {
            auto curIt = it;
            ++it;
            _generatedPrims.unsafe_erase(curIt);
        } else {
            ++it;
        }
    }
}


void
HdGpGenerativeProceduralResolvingSceneIndex::_SystemMessage(
    const TfToken &messageType,
    const HdDataSourceBaseHandle &args)
{
    TRACE_FUNCTION();

    if (!_attemptAsync) {
        if (messageType == HdSystemMessageTokens->asyncAllow) {
            _attemptAsync = true;
        }
        return;
    } else {
        if (messageType != HdSystemMessageTokens->asyncPoll) {
            return;
        }
    }

    _Notices notices;
    HdGpGenerativeProcedural::ChildPrimTypeMap primTypes;
    TfSmallVector<SdfPath, 8> removedEntries;

    for (auto &pathEntryPair : _activeSyncProcedurals) {
        const SdfPath &proceduralPrimPath = pathEntryPair.first;
        _ProcEntryPtr &procEntryPtr = pathEntryPair.second;

        if (!procEntryPtr) {
            removedEntries.push_back(proceduralPrimPath);
            continue;
        }

        if (!procEntryPtr->proc) {
            continue;
        }

        HdGpGenerativeProcedural::AsyncState result = 
            procEntryPtr->proc->AsyncUpdate(procEntryPtr->childTypes,
                &primTypes, &notices.dirtied);

        if (result == HdGpGenerativeProcedural::FinishedWithNewChanges ||
                result == HdGpGenerativeProcedural::ContinuingWithNewChanges) {
            _UpdateProceduralResult(get_pointer(procEntryPtr),
                proceduralPrimPath, primTypes, &notices);
            primTypes.clear();
        }
        
        if (result == HdGpGenerativeProcedural::Finished ||
                result == HdGpGenerativeProcedural::FinishedWithNewChanges) {
            removedEntries.push_back(proceduralPrimPath);
        }
    }

    if (!removedEntries.empty()) {
        for (const SdfPath &removedPath : removedEntries) {
            _activeSyncProcedurals.unsafe_erase(removedPath);
        }
    }

    if (!notices.added.empty()) {
        _SendPrimsAdded(notices.added);
    }

    if (!notices.removed.empty()) {
        _SendPrimsRemoved(notices.removed);
    }

    if (!notices.dirtied.empty()) {
        _SendPrimsDirtied(notices.dirtied);
    }
}

void
HdGpGenerativeProceduralResolvingSceneIndex::_UpdateProceduralResult(
    _ProcEntry *procEntryPtr,
    const SdfPath &proceduralPrimPath,
    const HdGpGenerativeProcedural::ChildPrimTypeMap &newChildTypes,
    _Notices *outputNotices) const
{
    _ProcEntry &procEntry = *procEntryPtr;

    // stuff we need to signal
    TfDenseHashSet<SdfPath, TfHash> removedChildPrims;
    TfDenseHashSet<SdfPath, TfHash> generatedPrims;

    // if there are no previous cooks, we can directly add all
    // without comparison
    if (procEntry.childTypes.empty()) {
        for (const auto& pathTypePair : newChildTypes) {
            const SdfPath &childPrimPath = pathTypePair.first;
            outputNotices->added.emplace_back(
                childPrimPath, pathTypePair. second);

            if (childPrimPath.HasPrefix(proceduralPrimPath)) {
                for (const SdfPath &p :
                        childPrimPath.GetAncestorsRange()) {
                    if (p == proceduralPrimPath) {
                        break;
                    }
                    procEntry.childHierarchy[
                        p.GetParentPath()].insert(p);
                    generatedPrims.insert(p);
                }
            } else {
                // TODO, warning, error
            }
        }

        for (const auto &pathPathSetPair : procEntry.childHierarchy) {
            generatedPrims.insert(pathPathSetPair.first);
        }

    } else if (procEntry.childTypes != newChildTypes) {
        // gather hierarchy for inclusion
        _ProcEntry::_PathSetMap newChildHierarchy;

        // add new entries (or entries whose types have changed)
        for (const auto& pathTypePair : newChildTypes) {
            const SdfPath &childPrimPath = pathTypePair.first;

            if (childPrimPath.HasPrefix(proceduralPrimPath)) {
                for (const SdfPath &p :
                        childPrimPath.GetAncestorsRange()) {
                    if (p == proceduralPrimPath) {
                        break;
                    }
                    newChildHierarchy[p.GetParentPath()].insert(p);
                }
            } else {
                // TODO, warning, error?
            }

            auto it = procEntry.childTypes.find(childPrimPath);
            if (it != procEntry.childTypes.end() &&
                    pathTypePair.second == it->second) {
                // previously existed and type is the same, do nothing
            } else {
                // either didn't previously exist or type is different
                outputNotices->added.emplace_back(
                    childPrimPath, pathTypePair.second);
                generatedPrims.insert(pathTypePair.first);
            }
        }

        // remove entries not present in new cook
        for (const auto& pathTypePair : procEntry.childTypes) {
            if (newChildTypes.find(pathTypePair.first) ==
                    newChildTypes.end()) {
                if (newChildHierarchy.find(pathTypePair.first)
                        == newChildHierarchy.end()) {
                    outputNotices->removed.emplace_back(
                        pathTypePair.first);
                    removedChildPrims.insert(pathTypePair.first);
                }
            }
        }

        // Add/remove _generatedPrims entries for intermediate hierarchy
        // NOTE: Hierarchy can potentially be identical with two 
        // childType values of the same size. So always do comparsions
        // in that case.
        if (newChildTypes.size() != procEntry.childTypes.size() ||
                 newChildHierarchy != procEntry.childHierarchy) {
            for (const auto &pathPathSetPair : newChildHierarchy) {
                const SdfPath &parentPath = pathPathSetPair.first;
                if (parentPath == proceduralPrimPath) {
                    continue;
                }

                bool addAsIntermediate = false;

                if (procEntry.childHierarchy.find(parentPath) ==
                        procEntry.childHierarchy.end()) {
                    // if it's also not directly in our current
                    // childTypes, add it as a type-less prim
                    if (newChildTypes.find(parentPath) ==
                            newChildTypes.end()) {
                        addAsIntermediate = true;
                    } else if (procEntry.childTypes.find(parentPath) !=
                            procEntry.childTypes.end()) {
                        // -or- it WAS in our child types, it means that
                        // our type has to changed to an intermediate
                        addAsIntermediate = true;
                    }
                } else {
                    // it WAS in our child types and not in our current
                    // types, it means that our type has to changed to
                    // an intermediate
                    if (procEntry.childTypes.find(parentPath) !=
                                procEntry.childTypes.end()
                            && newChildTypes.find(parentPath) ==
                                newChildTypes.end()) {
                        addAsIntermediate = true;
                    }
                }

                if (addAsIntermediate) {
                    generatedPrims.insert(parentPath);
                    outputNotices->added.emplace_back(
                            parentPath, TfToken());
                }
            }

            for (const auto &pathPathSetPair :
                    procEntry.childHierarchy) {
                const SdfPath &parentPath = pathPathSetPair.first;
                if (parentPath == proceduralPrimPath) {
                    continue;
                }
                if (newChildHierarchy.find(parentPath) ==
                        newChildHierarchy.end()) {

                    // if it was an implicitly created intermediate
                    // prim, we need to remove it separately
                    if (newChildTypes.find(parentPath) ==
                            newChildTypes.end()) {
                        removedChildPrims.insert(parentPath);
                        outputNotices->removed.emplace_back(parentPath);
                    }
                }
            }

            procEntry.childHierarchy = std::move(newChildHierarchy);
        }
    }

    for (const SdfPath &generatedPrimPath : generatedPrims) {
        if (generatedPrimPath == proceduralPrimPath) {
            continue;
        }
        _generatedPrims[
            generatedPrimPath].responsibleProc.store(&procEntry);
    }

    for (const SdfPath &removedPrimPath : removedChildPrims) {
        auto gpIt = _generatedPrims.find(removedPrimPath);
        if (gpIt != _generatedPrims.end()) {
            gpIt->second.responsibleProc.store(nullptr);
        }
    }

    procEntry.childTypes = std::move(newChildTypes);
}


PXR_NAMESPACE_CLOSE_SCOPE
