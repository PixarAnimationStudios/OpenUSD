//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_COORD_SYS_PRIM_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_COORD_SYS_PRIM_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiCoordSysPrimSceneIndex);

///
/// \class HdsiCoordSysPrimSceneIndex
///
/// If prim P has a coord sys binding FOO to another prim Q, the scene
/// index will add a coord sys prim Q.__coordSys:FOO under Q.
/// It will rewrite the coord sys binding on P to point to
/// Q.__coordSys:FOO and use Q's xform data source for Q.__coordSys:FOO.
///
/// Motivation: USD allows for a coord sys binding to target any xformable
/// prim. Some render delegates, however, only allow a coord sys binding
/// to point to a prim of type coord sys. This scene index is creating prim's
/// of that type for such render delegates.
///
/// For compatibility with the UsdImagingDelegate which is already adding a
/// coord sys prim under Q itself using a property path, we ignore coord sys
/// bindings to paths which are not prim paths.
///
/// An example:
///
/// Input to scene index:
///
/// /MyXform
///     dataSource:
///         xform: [ some xform ]
/// /MyPrim
///     dataSource:
///         coordSysBinding:
///             FOO: /MyXform
///
/// Will be transformed to:
///
/// /MyXform
///     dataSource:
///         xform: [ some xform ]
/// /MyXform.__coordSys:FOO
///     dataSource:
///         coordSys:
///             name: FOO
///         xform: [ as above ]
/// /MyPrim
///     dataSource:
///         coordSysBinding:
///             FOO: /MyXform.__coordSys:FOO
///
class HdsiCoordSysPrimSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    
    /// Creates a new coord sys prim scene index.
    ///
    static HdsiCoordSysPrimSceneIndexRefPtr New(
            HdSceneIndexBaseRefPtr const &inputScene)
    {
        return TfCreateRefPtr(
            new HdsiCoordSysPrimSceneIndex(inputScene));
    }

    HDSI_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HDSI_API
    HdsiCoordSysPrimSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene);

    // satisfying HdSingleInputFilteringSceneIndexBase
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    struct _Binding
    {
        TfToken name;
        SdfPath path;
    };
    using _Bindings = std::vector<_Binding>;
    using _PrimToBindings = std::map<SdfPath, _Bindings>;

    // Record coordSys bindings of prim at primPath. That is, add entries to the
    // below data structures if needed and increase ref-counts.
    // Optionally, return prims of type coord system that this scene index needs
    // to add.
    void _AddBindingsForPrim(const SdfPath &primPath,
                             SdfPathSet * addedCoordSysPrims = nullptr);
    // Remove coordSys bindings. That is, decrease ref-counts and remove entries
    // from below data structures if needed.
    // Optionally, return prims of type coord system that this scene index needs
    // to remove.
    void _RemoveBindings(const _Bindings &bindings,
                         SdfPathSet * removedCoordSysPrims);
    // Similar to above, but give the prim path explicitly to look-up bindings
    // in map.
    void _RemoveBindingsForPrim(const SdfPath &primPath,
                                SdfPathSet * removedCoordSysPrims);
    // Removes bindings for given prim and all its descendants stored in below
    // data structures.
    void _RemoveBindingsForSubtree(const SdfPath &primPath,
                                   SdfPathSet * removedCoordSysPrims);

    // If path is for coord sys prim added by this scene index, give the
    // prim source for it.
    HdContainerDataSourceHandle _GetCoordSysPrimSource(
        const SdfPath &primPath) const;

private:
    using _NameToRefCount =
        std::unordered_map<TfToken, size_t, TfToken::HashFunctor>;
    using _PrimToNameToRefCount =
        std::unordered_map<SdfPath, _NameToRefCount, SdfPath::Hash>;
    // Maps prim which is targeted by coord sys binding to name of binding to
    // count how many bindings are referencing that prim using that name.
    //
    // We delete an inner entry when there is no longer any coord sys binding
    // with that name targeting the prim.
    // We delete a prim when it is no longer targeted by any binding.
    //
    // This map is used to determine which coord sys prims we need to create
    // under the targeted prim.
    //
    //
    // In the above example, the content of the map will be:
    //
    // {
    //    /MyXform: {
    //                 FOO: 1
    //              }
    // }
    //
    _PrimToNameToRefCount _targetedPrimToNameToRefCount;

    // Maps prim to the coord sys bindings of that prim.
    //
    // Used to decrease ref counts when a prim gets deleted or modified.
    //
    // In the above example, the content of the map will be:
    //
    // { /MyPrim: [(FOO, /MyXform)] }
    //
    _PrimToBindings _primToBindings;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
