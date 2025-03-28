//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/skeletonResolvingSceneIndex.h"

#include "pxr/usdImaging/usdSkelImaging/bindingSchema.h"
#include "pxr/usdImaging/usdSkelImaging/dataSourceResolvedSkeletonPrim.h"
#include "pxr/usdImaging/usdSkelImaging/resolvedSkeletonSchema.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

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

UsdSkelImagingSkeletonResolvingSceneIndexRefPtr
UsdSkelImagingSkeletonResolvingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new UsdSkelImagingSkeletonResolvingSceneIndex(
            inputSceneIndex));
}

UsdSkelImagingSkeletonResolvingSceneIndex::
UsdSkelImagingSkeletonResolvingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
    TRACE_FUNCTION();

    for (const SdfPath &path : HdSceneIndexPrimView(inputSceneIndex)) {
        _AddResolvedSkeleton(path);
    }
}

HdSceneIndexPrim
UsdSkelImagingSkeletonResolvingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.primType != UsdSkelImagingPrimTypeTokens->skeleton) {
        return prim;
    }

    if (!prim.dataSource) {
        return prim;
    }

    auto it = _pathToResolvedSkeleton.find(primPath);
    if (it == _pathToResolvedSkeleton.end()) {
        return prim;
    }

    return {
        HdPrimTypeTokens->mesh,
        HdOverlayContainerDataSource::New(it->second, prim.dataSource) };
}

SdfPathVector
UsdSkelImagingSkeletonResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdSkelImagingSkeletonResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

    bool hasSkeletons = !_pathToResolvedSkeleton.empty();

    SdfPathSet skelsJustAdded;

    // Indices into entries which need to be changed from skeleton to mesh.
    std::vector<size_t> entriesIndices;

    {
        TRACE_SCOPE("First loop over prim added entries");

        const size_t n = entries.size();

        for (size_t i = 0; i < n; ++i) {
            const HdSceneIndexObserver::AddedPrimEntry &entry = entries[i];
            if (hasSkeletons) {
                // There might have already been a skeleton here already.
                _RemoveResolvedSkeleton(entry.primPath);
            }

            if (entry.primType != UsdSkelImagingPrimTypeTokens->skeleton) {
                continue;
            }

            if (!_AddResolvedSkeleton(entry.primPath)) {
                continue;
            }

            entriesIndices.push_back(i);
            hasSkeletons = true;
            skelsJustAdded.insert(entry.primPath);
        }
    }

    // Resync each skeleton if its animation relationship points to
    // a prim added here.
    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;

    if (!_skelAnimPathToSkeletonPaths.empty()) {
        TRACE_SCOPE("Second loop over prim added entries");

        for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
            for (const SdfPath &primPath :
                     _Lookup(_skelAnimPathToSkeletonPaths, entry.primPath)) {
                if (skelsJustAdded.count(primPath)) {
                    continue;
                }
                // E.g. skelAnimation prim targeted by skeleton was activated.
                //
                // Note that the dependencies of the skeleton do not change,
                // and we are iterating through _skelAnimPathToSkeletonPaths
                // so it is not safe to call _RemoveResolvedSkeleton/
                // _AddResolvedSkeleton.
                //
                _RefreshResolvedSkeletonDataSource(primPath);
                if (isObserved) {
                    newDirtiedEntries.push_back(
                        {primPath, HdDataSourceLocatorSet::UniversalSet()});
                }
            }
        }
    }

    if (!isObserved) {
        return;
    }

    if (entriesIndices.empty()) {
        _SendPrimsAdded(entries);
    } else {
        HdSceneIndexObserver::AddedPrimEntries newEntries(entries);
        for (const size_t index : entriesIndices) {
            newEntries[index].primType = HdPrimTypeTokens->mesh;
        }
        _SendPrimsAdded(newEntries);
    }
    if (!newDirtiedEntries.empty()) {
        _SendPrimsDirtied(newDirtiedEntries);
    }
}

void
UsdSkelImagingSkeletonResolvingSceneIndex::_ProcessDirtyLocators(
    const SdfPath &skelPath,
    const TfToken &dirtiedPrimType,
    const HdDataSourceLocatorSet &dirtyLocators,
    HdSceneIndexObserver::DirtiedPrimEntries * const entries)
{
    TRACE_FUNCTION();

    auto it = _pathToResolvedSkeleton.find(skelPath);
    if (it == _pathToResolvedSkeleton.end()) {
        return;
    }

    if (!it->second->ProcessDirtyLocators(
            dirtiedPrimType, dirtyLocators, entries)) {
        return;
    }

    // Need resync - including dependencies.
    _RemoveResolvedSkeleton(skelPath);
    _AddResolvedSkeleton(skelPath);
    if (entries) {
        entries->push_back({skelPath, HdDataSourceLocatorSet::UniversalSet()});
    }
}

void
UsdSkelImagingSkeletonResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (_pathToResolvedSkeleton.empty()) {
        _SendPrimsDirtied(entries);
        return;
    }

    const bool isObserved = _IsObserved();

    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries * const newDirtiedEntriesPtr =
        isObserved ? &newDirtiedEntries : nullptr;

    {
        TRACE_SCOPE("Looping over dirtied entries");

        const bool hasAnimDependencies = !_skelAnimPathToSkeletonPaths.empty();
        
        for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
            if (entry.dirtyLocators.Intersects(
                    UsdSkelImagingDataSourceResolvedSkeletonPrim::
                    GetDependendendOnDataSourceLocators())) {
                _ProcessDirtyLocators(
                    entry.primPath,
                    UsdSkelImagingPrimTypeTokens->skeleton,
                    entry.dirtyLocators,
                    newDirtiedEntriesPtr);
            }

            if (hasAnimDependencies && entry.dirtyLocators.Intersects(
                    UsdSkelImagingAnimationSchema::GetDefaultLocator())) {
                for (const SdfPath &skelPath :
                         _Lookup(
                             _skelAnimPathToSkeletonPaths, entry.primPath)) {
                    _ProcessDirtyLocators(
                        skelPath,
                        UsdSkelImagingPrimTypeTokens->skelAnimation,
                        entry.dirtyLocators,
                        newDirtiedEntriesPtr);
                }
            }
        }
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

void
UsdSkelImagingSkeletonResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (_pathToResolvedSkeleton.empty()) {
        _SendPrimsRemoved(entries);
        return;
    }

    // First process the skeleton's.
    //
    {
        TRACE_SCOPE("First loop over prim removed entries");

        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            // Deleting by prefix - can't use existing _RemoveResolvedSkeleton.
            auto it = _pathToResolvedSkeleton.lower_bound(entry.primPath);
            while (it != _pathToResolvedSkeleton.end() &&
                   it->first.HasPrefix(entry.primPath)) {
                _RemoveDependenciesForResolvedSkeleton(it->first, it->second);
                it = _pathToResolvedSkeleton.erase(it);
            }
        }
    }

    if (_skelAnimPathToSkeletonPaths.empty()) {
        _SendPrimsRemoved(entries);
        return;
    }

    const bool isObserved = _IsObserved();

    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;

    {
        TRACE_SCOPE("Second loop over prim removed entries");

        // Then dependencies of the skeleton's.
        //
        // Note that the above loop already deleted the dependencies of a skeleton
        // that was just removed.
        //
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            for (auto it = _skelAnimPathToSkeletonPaths.lower_bound(
                                                            entry.primPath);
                 it != _skelAnimPathToSkeletonPaths.end() &&
                     it->first.HasPrefix(entry.primPath);
                 ++it) {
                for (const SdfPath &primPath : it->second) {
                    // E.g. skelAnimation was deactivated.
                    _RefreshResolvedSkeletonDataSource(primPath);
                    if (isObserved) {
                        newDirtiedEntries.push_back(
                            {primPath, HdDataSourceLocatorSet::UniversalSet()});
                    }
                }
            }
        }
    }

    if (!isObserved) {
        return;
    }

    _SendPrimsRemoved(entries);
    if (!newDirtiedEntries.empty()) {
        _SendPrimsDirtied(newDirtiedEntries);
    }
}

bool
UsdSkelImagingSkeletonResolvingSceneIndex::_AddResolvedSkeleton(
    const SdfPath &path)
{
    TRACE_FUNCTION();

    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
    if (!prim.dataSource) {
        return false;
    }
    if (prim.primType != UsdSkelImagingPrimTypeTokens->skeleton) {
        return false;
    }

    _DsHandle ds =
        UsdSkelImagingDataSourceResolvedSkeletonPrim::New(
            _GetInputSceneIndex(), path, prim.dataSource);

    _AddDependenciesForResolvedSkeleton(path, ds);

    _pathToResolvedSkeleton[path] = std::move(ds);

    return true;
}

void
UsdSkelImagingSkeletonResolvingSceneIndex::_AddDependenciesForResolvedSkeleton(
    const SdfPath &skeletonPath,
    _DsHandle const &resolvedSkeleton)
{
    TRACE_FUNCTION();

    const SdfPath animationSource = resolvedSkeleton->GetAnimationSource();
    if (animationSource.IsEmpty()) {
        return;
    }

    // Note that we add the dependency even if there is no prim at
    // animationSource or the prim is not a skelAnimation.
    _skelAnimPathToSkeletonPaths[animationSource].insert(skeletonPath);
}

bool
UsdSkelImagingSkeletonResolvingSceneIndex::_RemoveResolvedSkeleton(
    const SdfPath &path)
{
    TRACE_FUNCTION();

    const auto it = _pathToResolvedSkeleton.find(path);
    if (it == _pathToResolvedSkeleton.end()) {
        return false;
    }
    _RemoveDependenciesForResolvedSkeleton(path, it->second);
    _pathToResolvedSkeleton.erase(it);
    return true;
}

void
UsdSkelImagingSkeletonResolvingSceneIndex::
_RemoveDependenciesForResolvedSkeleton(
    const SdfPath &skeletonPath,
    _DsHandle const &resolvedSkeleton)
{
    TRACE_FUNCTION();

    if (!resolvedSkeleton) {
        return;
    }

    const SdfPath animationSource = resolvedSkeleton->GetAnimationSource();
    if (animationSource.IsEmpty()) {
        return;
    }

    auto it = _skelAnimPathToSkeletonPaths.find(animationSource);
    if (it == _skelAnimPathToSkeletonPaths.end()) {
        return;
    }
    it->second.erase(skeletonPath);
    if (it->second.empty()) {
        _skelAnimPathToSkeletonPaths.erase(it);
    }
}

void
UsdSkelImagingSkeletonResolvingSceneIndex::
_RefreshResolvedSkeletonDataSource(
    const SdfPath &skeletonPath)
{
    TRACE_FUNCTION();

    _DsHandle &entry = _pathToResolvedSkeleton[skeletonPath];

    if (!entry) {
        TF_CODING_ERROR(
            "Expected data source for resolved skeleton at %s.",
            skeletonPath.GetText());
    }

    const HdSceneIndexPrim prim =
        _GetInputSceneIndex()->GetPrim(skeletonPath);
    if (prim.primType != UsdSkelImagingPrimTypeTokens->skeleton) {
        TF_CODING_ERROR(
            "Expected skeleton prim at %s.",
            skeletonPath.GetText());
        _pathToResolvedSkeleton.erase(skeletonPath);
        return;
    }
    if (!prim.dataSource) {
        TF_CODING_ERROR(
            "Expected data source for prim at %s.",
            skeletonPath.GetText());
        _pathToResolvedSkeleton.erase(skeletonPath);
        return;
    }

    {
        TRACE_SCOPE("Creating data source")

            entry =
                UsdSkelImagingDataSourceResolvedSkeletonPrim::New(
                    _GetInputSceneIndex(), skeletonPath, prim.dataSource);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
