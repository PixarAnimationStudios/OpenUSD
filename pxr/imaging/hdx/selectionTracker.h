//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_SELECTION_TRACKER_H
#define PXR_IMAGING_HDX_SELECTION_TRACKER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/base/vt/array.h"
#include <vector>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class HdRenderIndex;

using HdxSelectionTrackerSharedPtr =
    std::shared_ptr<class HdxSelectionTracker>;

/// ----------------------------------------------------------------------------
/// Selection highlighting in Hydra:
///
/// Hydra Storm (*) supports selection highlighting of:
/// (a) a set of rprims, wherein each rprim is entirely highlighted
/// (b) a set of instances of an rprim, wherein each instance is highlighted
/// (c) a set of subprimitives of an rprim, wherein each subprim is highlighted.
/// Subprimitives support is limited to elements (faces of meshes, or
/// individual curves of basis curves), edges of meshes/curves,
///  and points of meshes.
/// 
/// * While the goal is have an architecture that is extensible by rendering
/// backends, the current implementation is heavily influenced by the Storm
/// backend.
/// 
/// Background:
/// The current selection implementation is, in a sense, global in nature. 
/// If there are no selected objects, we do not bind any selection-related 
/// resources, nor does the shader execute any selection-related operations.
/// 
/// If there are one or more selected objects, we *don't* choose to have them
/// in a separate 'selection' collection.
/// Instead, we stick by AZDO principles and avoid command buffer changes as
/// a result of selection updates (which would involve removal of draw items
/// corresponding to the selected objects from each render pass' command buffer
/// and building the selection pass' command buffer).
/// We build an integer buffer encoding of the selected items, for use in the
/// fragment shader, that allows us to perform a small number of lookups to
/// quickly tell us if a fragment needs to be highlighted.
///
/// For scene indices, the HdxSelectionTracker uses the HdSelectionsSchema
/// (instaniated with GetFromParent of a prim data source) for a prim to
/// determine the prim's selection status. However, to support scene delegates,
/// we do support settings the selection directly with SetSelection.
/// If both are used, the union of the selections is taken.
///
/// Conceptually, the implementation is split into:
/// (a) HdSelection (for clients using scene delegates) : Client facing API
/// that builds a collection of selected items. This is agnostic of the
/// rendering backend.
/// HdSelectionsSchema (for clients using scene indices) : Scene indices
/// can populate this schema for each selected prim.
/// (b) HdxSelectionTracker: Base class that observes (a) and encodes it as
/// needed by (c). This may be specialized to be backend specific.
/// (c) HdxSelectionTask : A scene task that, currently, only syncs resources
/// related to selection highlighting. Currently, this is tied to Storm.
/// (d) HdxRenderSetupTask : A scene task that sets up the render pass shader
/// to use the selection highlighting mixin in the render pass(es) of
/// HdxRenderTask. This is relevant only to Storm.
///
/// ----------------------------------------------------------------------------


/// \class HdxSelectionTracker
///
/// HdxSelectionTracker takes HdSelection and generates a GPU buffer to be used 
/// \class HdxSelectionTracker
///
/// HdxSelectionTracker is a base class for observing selection state and
/// providing selection highlighting details to interested clients.
/// 
/// Applications may use HdxSelectionTracker as-is, or extend it as needed.
/// 
/// HdxSelectionTask takes HdxSelectionTracker as a task parameter, and uploads
/// the selection buffer encoding to the GPU.
///
class HdxSelectionTracker
{
public:
    HDX_API
    HdxSelectionTracker();

    HDX_API
    virtual ~HdxSelectionTracker();

    /// Optional override to update the selection (either compute HdSelection and
    /// call SetSelection or update a scene index with selection information using
    /// the HdSelectionsSchema) during HdxSelectionTask::Prepare.
    HDX_API
    virtual void UpdateSelection(HdRenderIndex *index);

    /// Encodes the selection state (HdxSelection) as an integer array. This is
    /// uploaded to the GPU and decoded in the fragment shader to provide
    /// selection highlighting behavior. See HdxSelectionTask.
    /// Returns true if offsets has anything selected.
    /// \p enableSelectionHighlight will populate selection buffer for any
    /// active selection highlighting if true.
    /// \p enableLocateHighlight will populate selection buffer for any active 
    /// locate (rollover) highlighting if true.
    HDX_API
    virtual bool GetSelectionOffsetBuffer(const HdRenderIndex *index,
                                          bool enableSelectionHighlight,
                                          bool enableLocateHighlight,
                                          VtIntArray *offsets) const;

    HDX_API
    virtual VtVec4fArray GetSelectedPointColors(const HdRenderIndex *index);

    /// Returns a monotonically increasing version number, which increments
    /// whenever the result of GetBuffers has changed. Note that this number may
    /// overflow and become negative, thus clients should use a not-equal
    /// comparison.
    HDX_API
    int GetVersion() const;

    /// Set the collection of selected objects. The ultimate selection (used for
    /// selection highlighting) will be the union of the collection set here and
    /// the one computed by querying the scene indices (using the
    /// HdxSelectionSceneIndexObserver).
    HDX_API
    void SetSelection(HdSelectionSharedPtr const &selection);

    /// Returns selection set with SetSelection.
    ///
    /// XXX: Rename to GetSelection
    HDX_API
    HdSelectionSharedPtr const &GetSelectionMap() const;

protected:
    /// Increments the internal selection state version, used for invalidation
    /// via GetVersion().
    HDX_API
    void _IncrementVersion();

private:
    bool _GetSelectionOffsets(HdSelectionSharedPtr const &selection,
                              HdSelection::HighlightMode mode,
                              const HdRenderIndex *index,
                              size_t modeOffset,
                              std::vector<int>* offsets) const;

    // A helper class to obtain the union of the selection computed
    // by querying the scene indices (with the HdxSelectionSceneIndexObserver)
    // and the selection set with SetSelection.
    class _MergedSelection;
    std::unique_ptr<_MergedSelection> _mergedSelection;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_SELECTION_TRACKER_H
