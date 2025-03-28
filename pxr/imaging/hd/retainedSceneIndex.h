//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    HD_API
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
    HD_API
    virtual void AddPrims(const AddedPrimEntries &entries);

    /// Removes a prim subtree from the retained scene index. This will also
    /// generate a PrimsRemoved notification, if applicable.
    HD_API
    virtual void RemovePrims(
        const HdSceneIndexObserver::RemovedPrimEntries &entries);

    /// Invalidates prim data in the retained scene index. This scene index
    /// doesn't have any internal invalidation mechanisms, but it generates
    /// a PrimsDirtied notification, if applicable. Subclasses can use it for
    /// invalidation of caches or retained data.
    HD_API
    virtual void DirtyPrims(
            const HdSceneIndexObserver::DirtiedPrimEntries &entries);

    // ------------------------------------------------------------------------
    // HdSceneIndexBase implementations.

    HD_API
    HdSceneIndexPrim GetPrim(const SdfPath & primPath) const override;

    HD_API
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
