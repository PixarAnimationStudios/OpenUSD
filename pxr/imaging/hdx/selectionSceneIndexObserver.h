//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_SELECTION_SCENE_INDEX_OBSERVER_H
#define PXR_IMAGING_HDX_SELECTION_SCENE_INDEX_OBSERVER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxSelectionSceneIndexObserver
///
/// Queries each prim of the given scene index for the HdSelectionsSchema to
/// compute a HdSelection.
///
class HdxSelectionSceneIndexObserver : public HdSceneIndexObserver
{
public:
    HDX_API
    HdxSelectionSceneIndexObserver();

    /// Set which scene index to query for selection.
    HDX_API
    void SetSceneIndex(HdSceneIndexBaseRefPtr const &sceneIndex);

    /// Increased every time the selection in the scene index gets dirtied
    /// (or a difference scene index is set).
    HDX_API
    int GetVersion() const;

    /// Get the result of querying the scene index for the selection as
    /// HdSelection.
    HDX_API
    HdSelectionSharedPtr GetSelection();

    HDX_API
    void PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries) override;
    HDX_API
    void PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries) override;
    HDX_API
    void PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries) override;
    
    HDX_API
    void PrimsRenamed(
        const HdSceneIndexBase &sender,
        const RenamedPrimEntries &entries) override;

private:
    HdSelectionSharedPtr _ComputeSelection();

    HdSceneIndexBaseRefPtr _sceneIndex;

    int _version;
    HdSelectionSharedPtr _selection;
    SdfPathSet _dirtiedPrims;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
