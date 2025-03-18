//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/pointsResolvingSceneIndex.h"

#include "pxr/usdImaging/usdSkelImaging/dataSourceResolvedPointsBasedPrim.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceResolvedExtComputationPrim.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/blendShapeSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceUtils.h"
#include "pxr/usdImaging/usdSkelImaging/inbetweenShapeSchema.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

static
bool
_IsPointBasedPrim(const TfToken &primType)
{
    return
        primType == HdPrimTypeTokens->mesh ||
        primType == HdPrimTypeTokens->basisCurves ||
        primType == HdPrimTypeTokens->points;
}

static
const SdfPathSet &
_Lookup(const std::map<SdfPath, SdfPathSet> &map, const SdfPath &key)
{
    const auto it = map.find(key);
    if (it == map.end()) {
        static const SdfPathSet empty;
        return empty;
    }
    return it->second;
}

UsdSkelImagingPointsResolvingSceneIndexRefPtr
UsdSkelImagingPointsResolvingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new UsdSkelImagingPointsResolvingSceneIndex(
            inputSceneIndex));
}

UsdSkelImagingPointsResolvingSceneIndex::
UsdSkelImagingPointsResolvingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
    TRACE_FUNCTION();

    for (const SdfPath &path : HdSceneIndexPrimView(inputSceneIndex)) {
        _AddResolvedPrim(path);
    }
}

HdSceneIndexPrim
UsdSkelImagingPointsResolvingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    ///////////////////////////////////////////////////////////////////////////
    // Handle points-based prims - adding resolved data source as overlay.
    if (_IsPointBasedPrim(prim.primType)) {
        TRACE_SCOPE("Processing points based prim");

        if (!prim.dataSource) {
            return prim;
        }

        auto it = _pathToResolvedPrim.find(primPath);
        if (it == _pathToResolvedPrim.end()) {
            return prim;
        }
        prim.dataSource =
            HdOverlayContainerDataSource::New(
                it->second, prim.dataSource);
        return prim;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Return data source for ext computations.
    if (!prim.primType.IsEmpty()) {
        // We do not expect a prim in the input scene for the ext
        // computations we are adding.
        return prim;
    }

    if (prim.dataSource) {
        // As above.
        return prim;
    }

    if (primPath.IsAbsoluteRootPath()) {
        // Our ext computation is a child of the skinned prim.
        return prim;
    }

    {
        TRACE_SCOPE("Processing potential ext computation");

        // Use that our ext computation is a child of the skinned prim.
        const TfToken &computationName = primPath.GetNameToken();
        const SdfPath &resolvedPrimPath = primPath.GetParentPath();

        const auto it = _pathToResolvedPrim.find(resolvedPrimPath);
        if (it == _pathToResolvedPrim.end()) {
            return prim;
        }

        if (!it->second->HasExtComputations()) {
            return prim;
        }

        if (HdContainerDataSourceHandle ds =
                UsdSkelImagingDataSourceResolvedExtComputationPrim(
                    it->second, computationName)) {
            return { HdPrimTypeTokens->extComputation, std::move(ds) };
        }
    }

    return prim;
}

SdfPathVector
UsdSkelImagingPointsResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    SdfPathVector result = _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    // Add ext computations if necessary.
    const auto it = _pathToResolvedPrim.find(primPath);
    if (it == _pathToResolvedPrim.end()) {
        return result;
    }
    if (!it->second->HasExtComputations()) {
        return result;
    }

    for (const TfToken &name :
             UsdSkelImagingExtComputationNameTokens->allTokens) {
        result.push_back(primPath.AppendChild(name));
    }

    return result;
}

void
UsdSkelImagingPointsResolvingSceneIndex::
_ProcessPrimsNeedingRefreshAndSendNotices(
    const std::map<SdfPath, bool> &primsNeedingRefreshToHasAddedEntry,
    HdSceneIndexObserver::AddedPrimEntries * const addedEntries,
    HdSceneIndexObserver::RemovedPrimEntries * const removedEntries,
    HdSceneIndexObserver::DirtiedPrimEntries * const dirtiedEntries)
{
    for (const auto &[primPath, hasAddedEntry] :
             primsNeedingRefreshToHasAddedEntry) {
        bool hadExtComputations = false;
        const bool removed = _RemoveResolvedPrim(primPath, &hadExtComputations);
        bool hasExtComputations = false;
        const bool added = _AddResolvedPrim(primPath, &hasExtComputations);

        if (dirtiedEntries) {
            if (!hasAddedEntry && (removed || added)) {
                dirtiedEntries->push_back(
                    { primPath, HdDataSourceLocatorSet::UniversalSet() });
            }
        }
        if (removedEntries) {
            if (hadExtComputations && !hasExtComputations) {
                for (const TfToken &name :
                         UsdSkelImagingExtComputationNameTokens->allTokens) {
                    removedEntries->push_back(
                        { primPath.AppendChild(name) });
                }
            }
        }
        if (addedEntries) {
            if (hasExtComputations && !hadExtComputations) {
                for (const TfToken &name :
                         UsdSkelImagingExtComputationNameTokens->allTokens) {
                    addedEntries->push_back(
                        { primPath.AppendChild(name),
                          HdPrimTypeTokens->extComputation });
                }
            }
        }
    }
}

void
UsdSkelImagingPointsResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    std::map<SdfPath, bool> primsNeedingRefreshToHasAddedEntry;

    {
        TRACE_SCOPE("Loop over added prim entries");

        const bool hasResolvedPrims = !_pathToResolvedPrim.empty();
        const bool hasSkelDependencies = !_skelPathToPrimPaths.empty();
        const bool hasBlendDependencies = !_blendShapePathToPrimPaths.empty();

        for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
            if (// Need to refresh if this is a new mesh, ...
                _IsPointBasedPrim(entry.primType) ||
                (hasResolvedPrims &&
                 // Need to refresh if this is a resync and this used to be
                 // a mesh affected by a skeleton and now is a different prim.
                 _pathToResolvedPrim.find(entry.primPath) !=
                         _pathToResolvedPrim.end())) {
                primsNeedingRefreshToHasAddedEntry[entry.primPath] = true;
            }

            if (hasSkelDependencies) {
                for (const SdfPath &primPath :
                         _Lookup(_skelPathToPrimPaths, entry.primPath)) {
                    primsNeedingRefreshToHasAddedEntry.emplace(primPath, false);
                }
            }

            if (hasBlendDependencies) {
                for (const SdfPath &primPath :
                         _Lookup(_blendShapePathToPrimPaths, entry.primPath)) {
                    primsNeedingRefreshToHasAddedEntry.emplace(primPath, false);
                }
            }
        }
    }

    if (primsNeedingRefreshToHasAddedEntry.empty()) {
        _SendPrimsAdded(entries);
        return;
    }

    const bool isObserved = _IsObserved();

    HdSceneIndexObserver::AddedPrimEntries newAddedEntries;
    HdSceneIndexObserver::RemovedPrimEntries newRemovedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;

    _ProcessPrimsNeedingRefreshAndSendNotices(
        primsNeedingRefreshToHasAddedEntry,
        isObserved ? &newAddedEntries : nullptr,
        isObserved ? &newRemovedEntries : nullptr,
        isObserved ? &newDirtiedEntries : nullptr);

    if (!isObserved) {
        return;
    }

    _SendPrimsRemoved(newRemovedEntries);

    if (newAddedEntries.empty()) {
        _SendPrimsAdded(entries);
    } else {
        TRACE_SCOPE("Creating and sending new added prim entries");
        newAddedEntries.insert(
            newAddedEntries.begin(), entries.begin(), entries.end());
        _SendPrimsAdded(newAddedEntries);
    }

    _SendPrimsDirtied(newDirtiedEntries);
}

bool
UsdSkelImagingPointsResolvingSceneIndex::_ProcessDirtyLocators(
    const SdfPath &primPath,
    const TfToken &dirtiedPrimType,
    const HdDataSourceLocatorSet &dirtyLocators,
    HdSceneIndexObserver::DirtiedPrimEntries * const entries)
{
    TRACE_FUNCTION();

    auto it = _pathToResolvedPrim.find(primPath);
    if (it == _pathToResolvedPrim.end()) {
        return false;
    }

    return
        it->second->ProcessDirtyLocators(
            dirtiedPrimType, dirtyLocators, entries);
}

void
UsdSkelImagingPointsResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;

    HdSceneIndexObserver::DirtiedPrimEntries * const newDirtiedEntriesPtr =
        isObserved ? &newDirtiedEntries : nullptr;

    std::map<SdfPath, bool> primsNeedingRefreshToHasAddedEntry;

    {
        TRACE_SCOPE("Looping over dirtied prim entries");

        const bool hasResolvedPrims = !_pathToResolvedPrim.empty();
        const bool hasSkelDependencies = !_skelPathToPrimPaths.empty();
        const bool hasBlendDependencies = !_blendShapePathToPrimPaths.empty();

        for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
            // Note that the dirty entry doesn't give us any type indication.
            //
            // We avoid look-ups in the maps or calls to the input scene by
            // checking for the existence of certain locators in the dirty
            // locators. Assuming that it is cheaper to query the dirty
            // locators first before doing other look-ups to see whether the
            // prim path is relevant.

            // Early bail: if we did not have any prim having a bound skeleton
            // and we do not change whether this prim is bound to a skeleton.
            if (!hasResolvedPrims &&
                !entry.dirtyLocators.Intersects(
                    UsdSkelImagingBindingSchema::GetSkeletonLocator())) {
                continue;
            }

            // This prim could be a mesh, ... affected by a skeleton.
            // Check whether any locators mean we need to do further dirtying
            // or refreshing the resolved data source.
            if (entry.dirtyLocators.Intersects(
                    UsdSkelImagingDataSourceResolvedPointsBasedPrim::
                    GetDependendendOnDataSourceLocators())) {
                if (_ProcessDirtyLocators(
                        entry.primPath,
                        /* primType = */ TfToken(),
                        entry.dirtyLocators,
                        newDirtiedEntriesPtr)) {
                    primsNeedingRefreshToHasAddedEntry.insert(
                        {entry.primPath, false});
                }
            }

            // This prim could be a skeleton affecting a mesh, ...
            if (hasSkelDependencies &&
                    entry.dirtyLocators.Intersects(
                        UsdSkelImagingResolvedSkeletonSchema::
                            GetDefaultLocator())) {
                for (const SdfPath &primPath :
                         _Lookup(_skelPathToPrimPaths, entry.primPath)) {
                    if (_ProcessDirtyLocators(
                            primPath,
                            UsdSkelImagingPrimTypeTokens->skeleton,
                            entry.dirtyLocators,
                            newDirtiedEntriesPtr)) {
                        primsNeedingRefreshToHasAddedEntry.insert(
                            {primPath, false});
                    }
                }
            }

            // This prim could be a blend shape affecting a mesh, ...
            if (hasBlendDependencies &&
                    entry.dirtyLocators.Intersects(
                        UsdSkelImagingBlendShapeSchema::GetDefaultLocator())) {
                for (const SdfPath &primPath :
                         _Lookup(_blendShapePathToPrimPaths, entry.primPath)) {
                    if (_ProcessDirtyLocators(
                            primPath,
                            UsdSkelImagingPrimTypeTokens->skelBlendShape,
                            entry.dirtyLocators,
                            newDirtiedEntriesPtr)) {
                        primsNeedingRefreshToHasAddedEntry.insert(
                            {primPath, false});
                    }
                }
            }
        }
    }

    if (!primsNeedingRefreshToHasAddedEntry.empty()) {
        HdSceneIndexObserver::AddedPrimEntries newAddedEntries;
        HdSceneIndexObserver::RemovedPrimEntries newRemovedEntries;

        _ProcessPrimsNeedingRefreshAndSendNotices(
            primsNeedingRefreshToHasAddedEntry,
            isObserved ? &newAddedEntries : nullptr,
            isObserved ? &newRemovedEntries : nullptr,
            newDirtiedEntriesPtr);

        if (!isObserved) {
            return;
        }

        _SendPrimsRemoved(newRemovedEntries);
        _SendPrimsAdded(newAddedEntries);
    }

    if (!isObserved) {
        return;
    }

    if (newDirtiedEntries.empty()) {
        _SendPrimsDirtied(entries);
    } else {
        {
            TRACE_SCOPE("Merging dirtied entries");
            newDirtiedEntries.insert(
                newDirtiedEntries.begin(), entries.begin(), entries.end());
        }
        _SendPrimsDirtied(newDirtiedEntries);
    }
}

static
void
_PopulateFromDependencies(
    const std::map<SdfPath, SdfPathSet> &dependencies,
    const SdfPath &prefix,
    std::map<SdfPath, bool> * const primsNeedingRefreshToHasAddedEntry)
{
    for (auto it = dependencies.lower_bound(prefix);
         it != dependencies.end() && it->first.HasPrefix(prefix);
         ++it) {
        for (const SdfPath &primPath : it->second) {
            primsNeedingRefreshToHasAddedEntry->insert({primPath, false});
        }
    }
}

void
UsdSkelImagingPointsResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // First removed resolved prim data sources.
    if (!_pathToResolvedPrim.empty()) {
        TRACE_SCOPE("First loop over removed prim entries");

        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            auto it = _pathToResolvedPrim.lower_bound(entry.primPath);
            while (it != _pathToResolvedPrim.end() &&
                   it->first.HasPrefix(entry.primPath)) {
                _RemoveDependenciesForResolvedPrim(it->first, it->second);
                it = _pathToResolvedPrim.erase(it);
            }
        }
    }

    const bool hasSkelDependencies = !_skelPathToPrimPaths.empty();
    const bool hasBlendDependencies = !_blendShapePathToPrimPaths.empty();

    if (!hasSkelDependencies && !hasBlendDependencies) {
        _SendPrimsRemoved(entries);
        return;
    }

    std::map<SdfPath, bool> primsNeedingRefreshToHasAddedEntry;

    // Then check for the dependencies.
    {
        TRACE_SCOPE("Second loop over removed prim entries");

        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            if (hasSkelDependencies) {
                _PopulateFromDependencies(
                    _skelPathToPrimPaths, entry.primPath,
                    &primsNeedingRefreshToHasAddedEntry);
            }
            if (hasBlendDependencies) {
                _PopulateFromDependencies(
                    _blendShapePathToPrimPaths, entry.primPath,
                    &primsNeedingRefreshToHasAddedEntry);
            }
        }
    }

    if (primsNeedingRefreshToHasAddedEntry.empty()) {
        _SendPrimsRemoved(entries);
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries newAddedEntries;
    HdSceneIndexObserver::RemovedPrimEntries newRemovedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;

    const bool isObserved = _IsObserved();

    _ProcessPrimsNeedingRefreshAndSendNotices(
        primsNeedingRefreshToHasAddedEntry,
        isObserved ? &newAddedEntries : nullptr,
        isObserved ? &newRemovedEntries : nullptr,
        isObserved ? &newDirtiedEntries : nullptr);

    if (!isObserved) {
        return;
    }

    if (newRemovedEntries.empty()) {
        _SendPrimsRemoved(entries);
    } else {
        {
            TRACE_SCOPE("Merging removed entries");
            newRemovedEntries.insert(
                newRemovedEntries.begin(), entries.begin(), entries.end());
        }
        _SendPrimsRemoved(newRemovedEntries);
    }

    _SendPrimsAdded(newAddedEntries);
    _SendPrimsDirtied(newDirtiedEntries);
}

bool
UsdSkelImagingPointsResolvingSceneIndex::_AddResolvedPrim(
    const SdfPath &path,
    bool * const hasExtComputations)
{
    TRACE_FUNCTION();

    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
    if (!_IsPointBasedPrim(prim.primType)) {
        return false;
    }

    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle ds =
        UsdSkelImagingDataSourceResolvedPointsBasedPrim::New(
            _GetInputSceneIndex(), path, prim.dataSource);
    if (!ds) {
        return false;
    }

    if (hasExtComputations) {
        *hasExtComputations = bool(ds->HasExtComputations());
    }

    _AddDependenciesForResolvedPrim(path, ds);
    _pathToResolvedPrim[path] = std::move(ds);

    return true;
}


void
UsdSkelImagingPointsResolvingSceneIndex::_AddDependenciesForResolvedPrim(
    const SdfPath &primPath,
    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle const &resolvedPrim)
{
    TRACE_FUNCTION();

    const SdfPath &skelPath = resolvedPrim->GetSkeletonPath();
    if (!skelPath.IsEmpty()) {
        _skelPathToPrimPaths[skelPath].insert(primPath);
    }
    for (const SdfPath &path : resolvedPrim->GetBlendShapeTargetPaths()) {
        _blendShapePathToPrimPaths[path].insert(primPath);
    }
}

bool
UsdSkelImagingPointsResolvingSceneIndex::_RemoveResolvedPrim(
    const SdfPath &primPath,
    bool * const hasExtComputations)
{
    TRACE_FUNCTION();

    auto it = _pathToResolvedPrim.find(primPath);
    if (it == _pathToResolvedPrim.end()) {
        return false;
    }

    if (hasExtComputations) {
        *hasExtComputations = it->second->HasExtComputations();
    }

    _RemoveDependenciesForResolvedPrim(primPath, it->second);
    _pathToResolvedPrim.erase(it);
    return true;
}

void
UsdSkelImagingPointsResolvingSceneIndex::_RemoveDependenciesForResolvedPrim(
    const SdfPath &primPath,
    UsdSkelImagingDataSourceResolvedPointsBasedPrimHandle const &resolvedPrim)
{
    TRACE_FUNCTION();

    if (!resolvedPrim) {
        return;
    }

    const SdfPath skelPath = resolvedPrim->GetSkeletonPath();
    if (!skelPath.IsEmpty()) {
        const auto it = _skelPathToPrimPaths.find(skelPath);
        if (it != _skelPathToPrimPaths.end()) {
            it->second.erase(primPath);
            if (it->second.empty()) {
                _skelPathToPrimPaths.erase(it);
            }
        }
    }

    for (const SdfPath &path : resolvedPrim->GetBlendShapeTargetPaths()) {
        const auto it = _blendShapePathToPrimPaths.find(path);
        if (it != _blendShapePathToPrimPaths.end()) {
            it->second.erase(primPath);
            if (it->second.empty()) {
                _blendShapePathToPrimPaths.erase(it);
            }
        }
    }
}

void
UsdSkelImagingPointsResolvingSceneIndex::_RefreshResolvedPrimDataSource(
    const SdfPath &primPath,
    bool * const hasExtComputations)
{
    TRACE_FUNCTION();

    _DsHandle &entry = _pathToResolvedPrim[primPath];

    if (!entry) {
        TF_CODING_ERROR(
            "Expected data source for resolved points based prim at %s.",
            primPath.GetText());
    }

    const HdSceneIndexPrim prim =
        _GetInputSceneIndex()->GetPrim(primPath);
    if (!_IsPointBasedPrim(prim.primType)) {
        TF_CODING_ERROR(
            "Expected points based prim at %s.",
            primPath.GetText());
        _pathToResolvedPrim.erase(primPath);
        return;
    }

    {
        TRACE_SCOPE("Creating data source");

        entry =
            UsdSkelImagingDataSourceResolvedPointsBasedPrim::New(
                _GetInputSceneIndex(), primPath, prim.dataSource);
    }

    if (!entry) {
        TF_CODING_ERROR(
            "Expected resolved points based prim at %s.",
            primPath.GetText());
        _pathToResolvedPrim.erase(primPath);
        return;
    }

    if (hasExtComputations) {
        *hasExtComputations = entry->HasExtComputations();
    }
}

void
UsdSkelImagingPointsResolvingSceneIndex::_RefreshResolvedPrimDataSources(
    const SdfPathSet &primPaths,
    HdSceneIndexObserver::DirtiedPrimEntries * const entries,
    SdfPathSet * const addedResolvedPrimsWithComputations,
    SdfPathSet * const removedResolvedPrimsWithComputations)
{
    TRACE_FUNCTION();

    for (const SdfPath &primPath : primPaths) {
        bool hasExtComputations = false;
        _RefreshResolvedPrimDataSource(primPath, &hasExtComputations);
        if (entries) {
            entries->push_back(
                { primPath,
                  HdDataSourceLocatorSet::UniversalSet()});
        }
        if (hasExtComputations) {
            if (addedResolvedPrimsWithComputations) {
                addedResolvedPrimsWithComputations->insert(primPath);
            }
            if (removedResolvedPrimsWithComputations) {
                removedResolvedPrimsWithComputations->erase(primPath);
            }
        } else {
            if (addedResolvedPrimsWithComputations) {
                addedResolvedPrimsWithComputations->erase(primPath);
            }
            if (removedResolvedPrimsWithComputations) {
                removedResolvedPrimsWithComputations->insert(primPath);
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
