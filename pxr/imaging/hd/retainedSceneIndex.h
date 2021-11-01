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
#ifndef PXR_IMAGING_HD_RETAINED_SCENE_INDEX_H
#define PXR_IMAGING_HD_RETAINED_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/usd/sdf/pathTable.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdRetainedSceneIndex;
TF_DECLARE_REF_PTRS(HdRetainedSceneIndex);

///
/// \class HdRetainedSceneIndex
///
/// Concrete scene container which can be externally populated and dirtied.
///
class HdRetainedSceneIndex : public HdSceneIndexBase
{
public:

    /// Creates a new retained scene index.
    static HdRetainedSceneIndexRefPtr New() {
        return TfCreateRefPtr(new HdRetainedSceneIndex);
    }

    // ------------------------------------------------------------------------

    struct AddedPrimEntry
    {
        SdfPath primPath;
        TfToken primType;
        HdContainerDataSourceHandle dataSource;
    };

    using AddedPrimEntries = std::vector<AddedPrimEntry>;

    /// Add a prim to the retained scene index. Each added entry has a path,
    /// type, and datasource; the retained scene index assumes ownership of
    /// these and uses them to answer scene queries. This will also generate
    /// a PrimsAdded notification, if applicable.
    virtual void AddPrims(const AddedPrimEntries &entries);

    /// Removes a prim subtree from the retained scene index. This will also
    /// generate a PrimsRemoved notification, if applicable.
    virtual void RemovePrims(
        const HdSceneIndexObserver::RemovedPrimEntries &entries);

    /// Invalidates prim data in the retained scene index. This scene index
    /// doesn't have any internal invalidation mechanisms, but it generates
    /// a PrimsDirtied notification, if applicable. Subclasses can use it for
    /// invalidation of caches or retained data.
    virtual void DirtyPrims(
            const HdSceneIndexObserver::DirtiedPrimEntries &entries);

    // ------------------------------------------------------------------------
    // HdSceneIndexBase implementations.

    HdSceneIndexPrim GetPrim(const SdfPath & primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HD_API
    HdRetainedSceneIndex();

private:

    struct _PrimEntry
    {
        HdSceneIndexPrim prim;
    };

    using _PrimEntryTable = SdfPathTable<_PrimEntry>;
    _PrimEntryTable _entries;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_RETAINED_SCENE_INDEX_H
