//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/rerootingContainerDataSource.h"

#include "pxr/base/trace/trace.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingRerootingSceneIndex::UsdImagingRerootingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputScene,
    PcpMapFunction const &mapFn)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
  , _mapFn(mapFn)
{
    // Populate _targetRootPathsTable with all mapped paths.
    for (auto sourceTargetPath: mapFn.GetSourceToTargetMap()) {
        _targetRootPathsTable[sourceTargetPath.second] = {};
    }
}

UsdImagingRerootingSceneIndex::~UsdImagingRerootingSceneIndex() = default;

HdSceneIndexPrim
UsdImagingRerootingSceneIndex::GetPrim(const SdfPath& primPath) const
{
    const SdfPath inputScenePath = _mapFn.MapTargetToSource(primPath);
    if (inputScenePath.IsEmpty()) {
        return { TfToken(), nullptr };
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(inputScenePath);
    if (prim.dataSource) {
        prim.dataSource = UsdImagingRerootingContainerDataSource::New(
            prim.dataSource, _mapFn);
    }
    return prim;
}

SdfPathVector
UsdImagingRerootingSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    SdfPathVector childPaths;

    const SdfPath inputScenePath = _mapFn.MapTargetToSource(primPath);
    if (!inputScenePath.IsEmpty()) {
        for (SdfPath const& childSourcePath:
             _GetInputSceneIndex()->GetChildPrimPaths(inputScenePath)) {
            // Return child paths in the rerooted namespace.
            childPaths.push_back(
                primPath.AppendChild(childSourcePath.GetNameToken()));
        }
    }

    // Append childPaths implied by mapping entries below primPath.
    auto range = _targetRootPathsTable.FindSubtreeRange(primPath);
    if (range.first != range.second) {
        // Step past primPath entry to its first child.
        range.first++;
        for (auto i = range.first; i != range.second; i = i.GetNextSubtree()) {
            if (TF_VERIFY(i->first.GetParentPath() == primPath)) {
                childPaths.push_back(i->first);
            }
        }
    }

    return childPaths;
}

void
UsdImagingRerootingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::AddedPrimEntries adjustedEntries;
    adjustedEntries.reserve(entries.size());

    for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
        const SdfPath entryPath = _mapFn.MapSourceToTarget(entry.primPath);
        if (!entryPath.IsEmpty()) {
            adjustedEntries.push_back( { entryPath, entry.primType } );
        }
    }

    _SendPrimsAdded(adjustedEntries);
}

void
UsdImagingRerootingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::RemovedPrimEntries adjustedEntries;
    adjustedEntries.reserve(entries.size());

    for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
        const SdfPath entryPath = _mapFn.MapSourceToTarget(entry.primPath);
        if (!entryPath.IsEmpty()) {
            adjustedEntries.push_back( { entryPath } );
        }
    }

    _SendPrimsRemoved(adjustedEntries);
}

void
UsdImagingRerootingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::DirtiedPrimEntries adjustedEntries;
    adjustedEntries.reserve(entries.size());

    for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
        const SdfPath entryPath = _mapFn.MapSourceToTarget(entry.primPath);
        if (!entryPath.IsEmpty()) {
            adjustedEntries.push_back( { entryPath, entry.dirtyLocators } );
        }
    }

    _SendPrimsDirtied(adjustedEntries);
}

PXR_NAMESPACE_CLOSE_SCOPE
