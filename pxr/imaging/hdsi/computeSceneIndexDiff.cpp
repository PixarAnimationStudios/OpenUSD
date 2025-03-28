//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/computeSceneIndexDiff.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/dispatcher.h"

#include <tbb/concurrent_queue.h>

PXR_NAMESPACE_OPEN_SCOPE

using _RemovedPrimEntryQueue
    = tbb::concurrent_queue<HdSceneIndexObserver::RemovedPrimEntry>;
using _AddedPrimEntryQueue
    = tbb::concurrent_queue<HdSceneIndexObserver::AddedPrimEntry>;
using _DirtiedPrimEntryQueue
    = tbb::concurrent_queue<HdSceneIndexObserver::DirtiedPrimEntry>;

static void
_FillAddedChildEntriesInParallel(
    WorkDispatcher* dispatcher,
    const HdSceneIndexBaseRefPtr& sceneIndex,
    const SdfPath& path,
    _AddedPrimEntryQueue* queue)
{
    queue->emplace(path, sceneIndex->GetPrim(path).primType);
    for (const SdfPath& childPath : sceneIndex->GetChildPrimPaths(path)) {
        dispatcher->Run([=]() {
            _FillAddedChildEntriesInParallel(
                dispatcher, sceneIndex, childPath, queue);
        });
    }
}

void
HdsiComputeSceneIndexDiffRoot(
    const HdSceneIndexBaseRefPtr& siA,
    const HdSceneIndexBaseRefPtr& siB,
    HdSceneIndexObserver::RemovedPrimEntries* removedEntries,
    HdSceneIndexObserver::AddedPrimEntries* addedEntries,
    HdSceneIndexObserver::RenamedPrimEntries* renamedEntries,
    HdSceneIndexObserver::DirtiedPrimEntries* dirtiedEntries)
{
    TRACE_FUNCTION();
    if (siA) {
        removedEntries->emplace_back(SdfPath::AbsoluteRootPath());
    }

    if (siB) {
        WorkDispatcher dispatcher;
        _AddedPrimEntryQueue queue;
        _FillAddedChildEntriesInParallel(
            &dispatcher, siB, SdfPath::AbsoluteRootPath(), &queue);
        dispatcher.Wait();
        addedEntries->insert(
            addedEntries->end(), queue.unsafe_begin(), queue.unsafe_end());
    }
}

// Given sorted ranges A [first1, last1) and B [first2, last2),
// this will write all elements in A^B into `d_firstBoth`, A-B into `d_first1`,
// and B-A into `d_first2`.
template <typename InputIt1, typename InputIt2, typename OutputIt>
void
_SetIntersectionAndDifference(
    InputIt1 first1,
    InputIt1 last1,
    InputIt2 first2,
    InputIt2 last2,
    OutputIt d_firstBoth,
    OutputIt d_first1,
    OutputIt d_first2)
{
    while ((first1 != last1) && (first2 != last2)) {
        if (*first1 < *first2) {
            // element is in A only
            *d_first1++ = *first1++;
        }
        else if (*first2 < *first1) {
            // element is in B only
            *d_first2++ = *first2++;
        }
        else {
            // element is in both
            *d_firstBoth++ = *first1++;
            first2++;
        }
    }

    // We've run out of elements in at least one of the input ranges.
    // Copy whatever may be left into the appropriate output.
    std::copy(first1, last1, d_first1);
    std::copy(first2, last2, d_first2);
}

static std::vector<SdfPath>
_GetSortedChildPaths(const HdSceneIndexBaseRefPtr& si, const SdfPath& path)
{
    std::vector<SdfPath> ret = si->GetChildPrimPaths(path);
    // XXX(edluong): could provide API to get these already sorted..
    std::sort(ret.begin(), ret.end());
    return ret;
}

static void
_ComputeDeltaDiffHelper(
    WorkDispatcher* dispatcher,
    const HdSceneIndexBaseRefPtr& siA,
    const HdSceneIndexBaseRefPtr& siB,
    const SdfPath& commonPath,
    _RemovedPrimEntryQueue* removedEntries,
    _AddedPrimEntryQueue* addedEntries,
    _DirtiedPrimEntryQueue* dirtiedEntries)
{
    const HdSceneIndexPrim primA = siA->GetPrim(commonPath);
    const HdSceneIndexPrim primB = siB->GetPrim(commonPath);

    if (primA.primType == primB.primType) {
        if (primA.dataSource != primB.dataSource) {
            // append to dirtiedEntries
            dirtiedEntries->emplace(
                commonPath, HdDataSourceLocatorSet::UniversalSet());
        }
        else {
            // do nothing
        }
    }
    else {
        // mark as added.  downstream clients should know to resync this.
        addedEntries->emplace(commonPath, primB.primType);
    }

    const std::vector<SdfPath> aPaths = _GetSortedChildPaths(siA, commonPath);
    const std::vector<SdfPath> bPaths = _GetSortedChildPaths(siB, commonPath);

    // For a common path, we are more likely to also have common children so
    // this is optimized for that.
    std::vector<SdfPath> sharedChildren;
    sharedChildren.reserve(std::min(aPaths.size(), bPaths.size()));
    std::vector<SdfPath> aOnlyPaths;
    std::vector<SdfPath> bOnlyPaths;
    _SetIntersectionAndDifference(
        aPaths.begin(), aPaths.end(), bPaths.begin(), bPaths.end(),
        std::back_inserter(sharedChildren), std::back_inserter(aOnlyPaths),
        std::back_inserter(bOnlyPaths));

    // XXX It might be nice to support renaming at this level.  If the prim
    // (path123, dataSource123) is removed, and (path456, dataSource123) is
    // added, we could express that as a rename(path123, path456).

    // For elements only in A, we remove.
    for (const SdfPath& aPath : aOnlyPaths) {
        removedEntries->emplace(aPath);
    }

    // For elements that are common, we recurse.
    for (const SdfPath& commonChildPath : sharedChildren) {
        dispatcher->Run([=]() {
            _ComputeDeltaDiffHelper(
                dispatcher, siA, siB, commonChildPath, removedEntries,
                addedEntries, dirtiedEntries);
        });
    }

    // For elements only in B, we recursively add.
    for (const SdfPath& bPath : bOnlyPaths) {
        _FillAddedChildEntriesInParallel(dispatcher, siB, bPath, addedEntries);
    }
}

void
HdsiComputeSceneIndexDiffDelta(
    const HdSceneIndexBaseRefPtr& siA,
    const HdSceneIndexBaseRefPtr& siB,
    HdSceneIndexObserver::RemovedPrimEntries* removedEntries,
    HdSceneIndexObserver::AddedPrimEntries* addedEntries,
    HdSceneIndexObserver::RenamedPrimEntries* renamedEntries,
    HdSceneIndexObserver::DirtiedPrimEntries* dirtiedEntries)
{
    TRACE_FUNCTION();

    if (!(siA && siB)) {
        // If either is null, fall back to very coarse notifications.
        HdsiComputeSceneIndexDiffRoot(
            siA, siB, removedEntries, addedEntries, renamedEntries,
            dirtiedEntries);
        return;
    }

    // We have both input scenes so we can do a diff.
    _RemovedPrimEntryQueue removedEntriesQueue;
    _AddedPrimEntryQueue addedEntriesQueue;
    _DirtiedPrimEntryQueue dirtiedEntriesQueue;
    {
        WorkDispatcher dispatcher;
        _ComputeDeltaDiffHelper(
            &dispatcher, siA, siB, SdfPath::AbsoluteRootPath(),
            &removedEntriesQueue, &addedEntriesQueue, &dirtiedEntriesQueue);
        dispatcher.Wait();
    }

    removedEntries->insert(
        removedEntries->end(), removedEntriesQueue.unsafe_begin(),
        removedEntriesQueue.unsafe_end());
    addedEntries->insert(
        addedEntries->end(), addedEntriesQueue.unsafe_begin(),
        addedEntriesQueue.unsafe_end());
    dirtiedEntries->insert(
        dirtiedEntries->end(), dirtiedEntriesQueue.unsafe_begin(),
        dirtiedEntriesQueue.unsafe_end());
}

PXR_NAMESPACE_CLOSE_SCOPE
