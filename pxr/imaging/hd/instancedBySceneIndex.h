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
#ifndef PXR_IMAGING_HD_INSTANCED_BY_SCENE_INDEX_H
#define PXR_IMAGING_HD_INSTANCED_BY_SCENE_INDEX_H

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

// This scene index gathers "prototypes" declarations from instancer prims,
// and uses them to define a synthetic attribute "instancedBy/paths", answering
// the question "Which instancers list me as a prototype?".
//
// To do this, the scene index inverts the "instancerTopology/prototypes"
// relationship of all instancers. That is, "instancedBy/paths" of a prim
// returns the paths of all instancers that have one of the
// "instancerTopology/prototypes" pointing to that prim.
//
// Note that if an instancer points to a prototype prim, we expect that
// instances all namespace descendants of the prototype prim (except for those
// subtrees that are targeted by a nested instancer). This behavior is not
// implemented here but by the flattening scene index. To determine the
// "instancedBy/paths" of a prim, the flattening scene index traverses the
// namespace ancestors of that prim starting with the prim itself until a
// non-empty list of "instancedBy/paths" (or the pseudo-root) is hit.
//
// Note that having more than one path in instancedBy/paths means that there
// are several (sibling) instancers instancing the same prim, not that the
// instancers are (necessarily) nested. In other words, to find all nested
// instancers, a client has to recurse the instancers that instancedBy/paths
// points to.
//
TF_DECLARE_REF_PTRS(HdInstancedBySceneIndex);

class HdInstancedBySceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    HD_API
    static HdInstancedBySceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene) {
        return TfCreateRefPtr(new HdInstancedBySceneIndex(inputScene));
    }

    HD_API
    ~HdInstancedBySceneIndex() override;

    HD_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    // Data shared between this scene index and the data sources it produces.
    class InstancerMapping;
    using InstancerMappingSharedPtr = std::shared_ptr<InstancerMapping>;

protected:
    HdInstancedBySceneIndex(const HdSceneIndexBaseRefPtr &inputScene);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

private:
    // Given a prim path, extracts prototype paths from instancer topology
    // schema from input scene index.
    VtArray<SdfPath> _GetPrototypes(
        const SdfPath &instancer) const;

    // Sends prims dirtied messages with data source locator instancedBy
    // for all given prim paths to scene index observers.
    void _SendInstancedByDirtied(const SdfPathSet &paths);
    
    InstancerMappingSharedPtr const _instancerMapping;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_INSTANCED_BY_SCENE_INDEX_H
