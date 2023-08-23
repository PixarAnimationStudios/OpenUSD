//
// Copyright 2021 Pixar
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
