//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDUI_SCENE_OBSERVER_H
#define PXR_IMAGING_HDUI_SCENE_OBSERVER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/sceneIndexObserver.h"

#include <QObject>

PXR_NAMESPACE_OPEN_SCOPE

/// Observer object for reporting change notifications from Hydra scenes.
///
/// As the observed Hydra scene changes, it notifies this object which forwards
/// those changes as Qt signals, or queues them for later processing.
class HduiSceneObserver :
    public QObject, public HdSceneIndexObserver
{
    Q_OBJECT;

public:
    HduiSceneObserver();
    ~HduiSceneObserver() override;

    HduiSceneObserver(const HduiSceneObserver&) = delete;
    HduiSceneObserver& operator=(const HduiSceneObserver&) = delete;

    /// Start reporting change notifications for the Hydra scene produced by
    /// \p sceneIndex. The previous observed scene, if any, is unsubscribed.
    ///
    /// Batching mode is unaffected. Any batched changes from the previous
    /// scene are discarded.
    void Subscribe(const HdSceneIndexBasePtr& sceneIndex);

    /// Stop reporting change notifications.
    ///
    /// Batching mode is unaffected. Any batched changes are discarded.
    void Unsubscribe();

    /// Turn batching mode on or off. In batching mode, observed changes are
    /// coalesced and queued until either batching is disabled or
    /// \c FlushBatchedUpdates() is called.
    ///
    /// While batching is enabled, the data source locator sets from
    /// PrimsDirtied notices are combined together for each dirtied prim.  No
    /// other notice coalescing is done.
    ///
    /// Batching mode is disabled by default.
    void SetBatchingEnabled(bool enabled);

    /// Report queued change notifications, if any. Prim adds and removes are
    /// reported first, followed by dirties. Does not disable batching mode.
    void FlushBatchedUpdates();

private:
    // Override HdSceneIndexObserver
    void PrimsAdded(
        const HdSceneIndexBase&,
        const AddedPrimEntries&) override;

    void PrimsRemoved(
        const HdSceneIndexBase&,
        const RemovedPrimEntries&) override;

    void PrimsDirtied(
        const HdSceneIndexBase&,
        const DirtiedPrimEntries&) override;

    void PrimsRenamed(
        const HdSceneIndexBase&,
        const RenamedPrimEntries&) override;

Q_SIGNALS:
    /// Sent when prims in the observed scene are added, removed, or moved
    /// (including renames).
    ///
    /// For moves, the original and new paths are included in \p removedPaths
    /// and \p addedPaths, respectively.
    void PrimsAddedOrRemoved(
        const SdfPathSet& addedPaths,
        const SdfPathSet& removedPaths);

    /// Sent when prims in the observed scene are dirtied, indicating one or
    /// more of their data sources have changed. Details described by \p entries.
    void PrimsMarkedDirty(const DirtiedPrimEntries& entries);

    /// Sent when a change to the observed scene is queued while in batching
    /// mode.
    void ChangeBatched();

private:
    void _BatchAddedPrim(const SdfPath&);
    void _BatchRemovedPrim(const SdfPath&);
    void _BatchDirtiedPrim(const SdfPath&, const HdDataSourceLocatorSet&);

    void _ClearBatchedChanges();

private:
    HdSceneIndexBasePtr _index;

    bool _batching;
    SdfPathSet _batchedAddedPrims;
    SdfPathSet _batchedRemovedPrims;
    std::map<SdfPath, HdDataSourceLocatorSet> _batchedDirtiedPrims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
