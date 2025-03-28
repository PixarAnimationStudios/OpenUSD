//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SCENE_INDEX_OBSERVER_H
#define PXR_IMAGING_HD_SCENE_INDEX_OBSERVER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/declarePtrs.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/dataSourceLocator.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneIndexBase;

TF_DECLARE_WEAK_AND_REF_PTRS(HdSceneIndexBase);
TF_DECLARE_WEAK_PTRS(HdSceneIndexObserver);

/// \class HdSceneIndexObserver
///
/// Observer of scene data. From the time an observer is registered with
/// a scene index, the scene index will send it diffs as the scene changes.
///
class HdSceneIndexObserver : public TfWeakBase
{
public:

    HD_API
    virtual ~HdSceneIndexObserver();

    /// A notice indicating a prim of a given type was added to the scene.
    /// Note that \p primPath might already exist in the scene, in which case
    /// this acts as a resync or change-of-primtype notice.
    struct AddedPrimEntry
    {
        SdfPath primPath;
        TfToken primType;

        AddedPrimEntry() = default;
        AddedPrimEntry(const SdfPath &primPath, const TfToken &primType)
        : primPath(primPath)
        , primType(primType)
        {
        }
    };
    using AddedPrimEntries = TfSmallVector<AddedPrimEntry, 16>;

    //----------------------------------

    /// A notice indicating a prim subtree was removed from the scene.
    /// Note that all prims which are descendants of \p primPath should be
    /// removed as well.
    struct RemovedPrimEntry
    {
        SdfPath primPath;
        
        RemovedPrimEntry() = default;
        RemovedPrimEntry(const SdfPath &primPath)
        : primPath(primPath)
        {}
    };

    using RemovedPrimEntries = TfSmallVector<RemovedPrimEntry, 16>;

    //----------------------------------

    /// A notice indicating a prim was invalidated. \p dirtyLocators identifies
    /// a set of datasources for which data needs to be re-pulled. Locators
    /// are hierarchical: if \p primvars was invalidated, \p primvars/color
    /// was considered invalidated as well. This notice only affects the named
    /// prim; descendants of \p primPath are unaffected.
    struct DirtiedPrimEntry
    {
        SdfPath primPath;
        HdDataSourceLocatorSet dirtyLocators;

        DirtiedPrimEntry() = default;
        DirtiedPrimEntry(
            const SdfPath &primPath,
            const HdDataSourceLocatorSet &dirtyLocators)
        : primPath(primPath)
        , dirtyLocators(dirtyLocators)
        {}
    };

    using DirtiedPrimEntries = TfSmallVector<DirtiedPrimEntry, 16>;

    //----------------------------------

    /// A notice indicating a prim (and its descendents) was renamed or
    /// reparented.
    struct RenamedPrimEntry
    {
        SdfPath oldPrimPath;
        SdfPath newPrimPath;

        RenamedPrimEntry() = default;
        RenamedPrimEntry(
            const SdfPath &oldPrimPath,
            const SdfPath &newPrimPath)
        : oldPrimPath(oldPrimPath)
        , newPrimPath(newPrimPath)
        {}
    };

    using RenamedPrimEntries = TfSmallVector<RenamedPrimEntry, 16>;

    //-------------------------------------------------------------------------

    /// A notification indicating prims have been added to the scene. The
    /// set of scene prims compiled from added/removed notices should match
    /// the set from a traversal based on \p sender.GetChildPrimNames. Each
    /// prim has a path and type. It's possible for \p PrimsAdded to be called
    /// for prims that already exist; in that case, observers should be sure to
    /// update the prim type, in case it changed, and resync the prim. This
    /// function is not expected to be threadsafe.
    HD_API
    virtual void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) = 0;

    /// A notification indicating prims have been removed from the scene.
    /// Note that this message is considered hierarchical; if \p /Path is
    /// removed, \p /Path/child is considered removed as well. This function is
    /// not expected to be threadsafe.
    HD_API
    virtual void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) = 0;

    /// A notification indicating prim datasources have been invalidated.
    /// This message is not considered hierarchical on \p primPath; if \p
    /// /Path is dirtied, \p /Path/child is not necessarily dirtied. However
    /// datasource locators are considered hierarchical: if \p primvars is
    /// dirtied on a prim, \p primvars/color is considered dirtied as well.
    /// This function is not expected to be threadsafe.
    HD_API
    virtual void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) = 0;

    /// A notification indicating prims (and their descendants) have been
    /// renamed or reparented.
    /// This function is not expected to be threadsafe.
    HD_API
    virtual void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) = 0;

    /// A utility for converting prims renamed messages into equivalent removed
    /// and added notices.
    HD_API
    static void ConvertPrimsRenamedToRemovedAndAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RenamedPrimEntries &renamedEntries,
        HdSceneIndexObserver::RemovedPrimEntries *outputRemovedEntries,
        HdSceneIndexObserver::AddedPrimEntries *outputAddedEntries);

    /// A utility for converting prims renamed messages into equivalent removed
    /// and added notices at the observer level
    HD_API
    static void ConvertPrimsRenamedToRemovedAndAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RenamedPrimEntries &renamedEntries,
        HdSceneIndexObserver *observer);





};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SCENE_INDEX_OBSERVER_H
