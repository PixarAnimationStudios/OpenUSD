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
#ifndef PXR_IMAGING_HD_MERGING_SCENE_H
#define PXR_IMAGING_HD_MERGING_SCENE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMergingSceneIndex;
TF_DECLARE_REF_PTRS(HdMergingSceneIndex);

/// \class HdMergingSceneIndex
///
/// Merges multiple scenes together. For prims which exist in more than one
/// input scene, data sources are overlayed (down to the leaf) with the earlier
/// inserted scene having the stronger opinion.
/// NOTE: This is currently the only example of a multi-input scene index and
///       therefore that pattern hasn't yet been broken into a base class.
class HdMergingSceneIndex : public HdFilteringSceneIndexBase
{
public:

    static HdMergingSceneIndexRefPtr New() {
        return TfCreateRefPtr(new HdMergingSceneIndex);
    }

    /// Adds a scene with activeInputSceneRoot specifying the shallowest path
    /// at which prims should be considered. This is an optional optimization
    /// to avoid having to query multiple inputs when it's known in advance
    /// which might be relevant for a given prim.
    HD_API
    void AddInputScene(
        const HdSceneIndexBaseRefPtr &inputScene,
        const SdfPath &activeInputSceneRoot);

    HD_API
    void RemoveInputScene(const HdSceneIndexBaseRefPtr &sceneIndex);

    /// satisfying HdFilteringSceneIndex
    HD_API
    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const override;

    // satisfying HdSceneIndexBase
    HD_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HD_API
    HdMergingSceneIndex();

private:

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries);

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries);

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries);

    void _FillAddedEntriesRecursively(
        const HdSceneIndexBaseRefPtr &newSceneIndex,
        const SdfPath &primPath,
        HdSceneIndexObserver::AddedPrimEntries * const addedEntries);


    friend class _Observer;

    class _Observer : public HdSceneIndexObserver
    {
    public:
        _Observer(HdMergingSceneIndex *owner)
        : _owner(owner) {}

        void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override;

        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override;

        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override;
    private:
        HdMergingSceneIndex *_owner;
    };

    _Observer _observer;

    struct _InputEntry
    {
        HdSceneIndexBaseRefPtr sceneIndex;
        SdfPath sceneRoot;

        _InputEntry(const HdSceneIndexBaseRefPtr &sceneIndex,
                const SdfPath &sceneRoot)
        : sceneIndex(sceneIndex)
        , sceneRoot(sceneRoot)
        {
        }
    };

    using _InputEntries = std::vector<_InputEntry>;

    _InputEntries _inputs;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_MERGING_SCENE_H
