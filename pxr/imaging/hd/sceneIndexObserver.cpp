//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSceneIndexObserver::~HdSceneIndexObserver() = default;

/*static*/
void
HdSceneIndexObserver::ConvertPrimsRenamedToRemovedAndAdded(
    const HdSceneIndexBase &inputScene,
    const HdSceneIndexObserver::RenamedPrimEntries &renamedEntries,
    HdSceneIndexObserver::RemovedPrimEntries *outputRemovedEntries,
    HdSceneIndexObserver::AddedPrimEntries *outputAddedEntries)
{

    if (ARCH_UNLIKELY(!outputRemovedEntries)) {
        TF_CODING_ERROR("no outputRemovedEntries provided");
    }

    if (ARCH_UNLIKELY(!outputAddedEntries)) {
        TF_CODING_ERROR("no outputAddedEntries provided");
    }

    std::vector<SdfPath> workQueue;

    SdfPathVector childPaths;

    for (const HdSceneIndexObserver::RenamedPrimEntry &renamedEntry
            : renamedEntries) {
        if (renamedEntry.oldPrimPath != renamedEntry.newPrimPath) {
            // remove existing
            outputRemovedEntries->emplace_back(renamedEntry.oldPrimPath);

            workQueue.push_back(renamedEntry.newPrimPath);

            // add back based on traversal on input from new path
            while (!workQueue.empty()) {
                SdfPath path = workQueue.back();
                workQueue.pop_back();

                HdSceneIndexPrim p = inputScene.GetPrim(path);
                outputAddedEntries->emplace_back(path, p.primType);

                childPaths = inputScene.GetChildPrimPaths(path);
                // insert backwards as we are pulling off the end.
                workQueue.insert(workQueue.end(),
                    childPaths.rbegin(), childPaths.rend());
            }
        }
    }
}


/*static*/
void HdSceneIndexObserver::ConvertPrimsRenamedToRemovedAndAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RenamedPrimEntries &renamedEntries,
    HdSceneIndexObserver *observer
)
{
    RemovedPrimEntries removed;
    AddedPrimEntries added;
    ConvertPrimsRenamedToRemovedAndAdded(
        sender, renamedEntries, &removed, &added);

    observer->PrimsRemoved(sender, removed);
    observer->PrimsAdded(sender, added);
}


PXR_NAMESPACE_CLOSE_SCOPE
