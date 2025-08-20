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

    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;

    const bool hasSkelAnimDependencies = !_skelAnimPathToSkeletonPaths.empty();
    const bool hasInstancerDependencies = !_instancerPathToSkeletonPaths.empty();
    
    if (hasSkelAnimDependencies || hasInstancerDependencies) {
        TRACE_SCOPE("Second loop over prim added entries");

        SdfPathSet skelPathsNeedingRefresh;
        
        for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
            if (hasSkelAnimDependencies) {
                // Schedule resync for each skeleton whose animation
                // relationship points to prim added here.
                for (const SdfPath &primPath :
                         _Lookup(_skelAnimPathToSkeletonPaths, entry.primPath)) {
                    if (skelsJustAdded.count(primPath)) {
                        continue;
                    }
                    skelPathsNeedingRefresh.insert(primPath);
                }
            }
            if (hasInstancerDependencies) {
                // Schedule resync for each skeleton that is affected by an
                // instancer that gets added/resync'ed here.
                for (const SdfPath &primPath :
                         _Lookup(_instancerPathToSkeletonPaths, entry.primPath)) {
                    if (skelsJustAdded.count(primPath)) {
                        continue;
                    }
                    skelPathsNeedingRefresh.insert(primPath);
                }
            }
        }
        for (const SdfPath &skelPath : skelPathsNeedingRefresh) {
            if (isObserved) {
                newDirtiedEntries.push_back(
                    {skelPath, HdDataSourceLocatorSet::UniversalSet()});
            }
            _RefreshResolvedSkeletonDataSource(skelPath);
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
        const bool hasInstancerDependencies = !_instancerPathToSkeletonPaths.empty();
        
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

            static const HdDataSourceLocatorSet instancerLocators{
                UsdSkelImagingDataSourceXformResolver::GetInstancedByLocator(),
                UsdSkelImagingDataSourceXformResolver::GetXformLocator(),
                UsdSkelImagingDataSourceXformResolver::GetInstanceXformLocator()};
            
            if (hasInstancerDependencies &&
                    entry.dirtyLocators.Intersects(instancerLocators)) {
                for (const SdfPath &skelPath :
                         _Lookup(
                             _instancerPathToSkeletonPaths, entry.primPath)) {
                    _ProcessDirtyLocators(
                        skelPath,
                        HdPrimTypeTokens->instancer,
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

static
void
_PopulateFromDependencies(
    const std::map<SdfPath, SdfPathSet> &dependencies,
    const SdfPath &prefix,
    SdfPathSet * const primPaths)
{
    for (auto it = dependencies.lower_bound(prefix);
         it != dependencies.end() && it->first.HasPrefix(prefix);
         ++it) {
        primPaths->insert(it->second.begin(), it->second.end());
    }
}

void
UsdSkelImagingSkeletonResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    const bool isObserved = _IsObserved();

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

    HdSceneIndexObserver::DirtiedPrimEntries newDirtiedEntries;

    const bool hasSkelAnimationDependencies = !_skelAnimPathToSkeletonPaths.empty();
    const bool hasInstancerDependencies = !_instancerPathToSkeletonPaths.empty();

    if (hasSkelAnimationDependencies || hasInstancerDependencies) {
        TRACE_SCOPE("Second loop over prim removed entries");

        SdfPathSet skelPathsNeedingRefresh;

        // Then dependencies of the skeleton's.
        //
        // Note that the above loop already deleted the dependencies of a skeleton
        // that was just removed.
        //
        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            if (hasSkelAnimationDependencies) {
                _PopulateFromDependencies(
                    _skelAnimPathToSkeletonPaths, entry.primPath,
                    &skelPathsNeedingRefresh);
            }
            if (hasInstancerDependencies) {
                _PopulateFromDependencies(
                    _instancerPathToSkeletonPaths, entry.primPath,
                    &skelPathsNeedingRefresh);
            }
        }

        for (const SdfPath &skelPath : skelPathsNeedingRefresh) {
            _RefreshResolvedSkeletonDataSource(skelPath);
            if (isObserved) {
                newDirtiedEntries.push_back(
                    {skelPath, HdDataSourceLocatorSet::UniversalSet()});
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
    if (!animationSource.IsEmpty()) {
        // Note that we add the dependency even if there is no prim at
        // animationSource or the prim is not a skelAnimation.
        _skelAnimPathToSkeletonPaths[animationSource].insert(skeletonPath);
    }

    for (const SdfPath &instancerPath : resolvedSkeleton->GetInstancerPaths()) {
        _instancerPathToSkeletonPaths[instancerPath].insert(skeletonPath);
    }
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

static
void _Remove(const SdfPath &key, const SdfPath &value,
             std::map<SdfPath, SdfPathSet> * const map)
{
    const auto it = map->find(key);
    if (it == map->end()) {
        return;
    }
    it->second.erase(value);
    if (!it->second.empty()) {
        return;
    }
    map->erase(it);
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
    if (!animationSource.IsEmpty()) {
        _Remove(animationSource, skeletonPath, &_skelAnimPathToSkeletonPaths);
    }

    for (const SdfPath &instancerPath : resolvedSkeleton->GetInstancerPaths()) {
        _Remove(instancerPath, skeletonPath, &_instancerPathToSkeletonPaths);
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

    _RemoveDependenciesForResolvedSkeleton(skeletonPath, entry);
    
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

    _AddDependenciesForResolvedSkeleton(skeletonPath, entry);    
}

PXR_NAMESPACE_CLOSE_SCOPE
