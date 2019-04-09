//
// Copyright 2016 Pixar
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
#ifndef HDX_SELECTION_TRACKER_H
#define HDX_SELECTION_TRACKER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/path.h"
#include <boost/smart_ptr.hpp>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdRenderIndex;
class TfToken;
class SdfPath;
class VtValue;

typedef boost::shared_ptr<class HdxSelectionTracker> HdxSelectionTrackerSharedPtr;
typedef boost::weak_ptr<class HdxSelectionTracker> HdxSelectionTrackerWeakPtr;


/// ----------------------------------------------------------------------------
/// Selection highlighting in Hydra:
///
/// Hydra Stream (*) supports selection highlighting of:
/// (a) a set of rprims, wherein each rprim is entirely highlighted
/// (b) a set of instances of an rprim, wherein each instance is highlighted
/// (c) a set of subprimitives of an rprim, wherein each subprim is highlighted.
/// Subprimitives support is limited to elements (faces of meshes, or
/// individual curves of basis curves), edges of meshes and points of meshes.
/// 
/// * While the goal is have an architecture that is extensible by rendering
/// backends, the current implementation is heavily influenced by the Stream(GL)
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
/// Conceptually, the implementation is split into:
/// (a) HdSelection : Client facing API that builds a collection of selected
/// items. This is agnostic of the rendering backend.
/// (b) HdxSelectionTracker: Base class that observes (a) and encodes it as
/// needed by (c). This may be specialized to be backend specific.
/// (c) HdxSelectionTask : A scene task that, currently, only syncs resources
/// related to selection highlighting. Currently, this is tied to Stream.
/// (d) HdxRenderSetupTask : A scene task that sets up the render pass shader
/// to use the selection highlighting mixin in the render pass(es) of
/// HdxRenderTask. This is relevant only to Stream.
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
class HdxSelectionTracker {
public:
    HDX_API
    HdxSelectionTracker();
    virtual ~HdxSelectionTracker() = default;

    /// Update dirty bits in the ChangeTracker and compute required primvars for
    /// later consumption.
    HDX_API
    virtual void Prepare(HdRenderIndex *index);

    /// Encodes the selection state (HdxSelection) as an integer array. This is
    /// uploaded to the GPU and decoded in the fragment shader to provide
    /// selection highlighting behavior. See HdxSelectionTask.
    /// Returns true if offsets has anything selected.
    HDX_API
    virtual bool GetSelectionOffsetBuffer(HdRenderIndex const *index,
                                          VtIntArray *offsets) const;

    HDX_API
    virtual VtVec4fArray GetSelectedPointColors() const;

    /// Returns a monotonically increasing version number, which increments
    /// whenever the result of GetBuffers has changed. Note that this number may
    /// overflow and become negative, thus clients should use a not-equal
    /// comparison.
    HDX_API
    int GetVersion() const;

    /// The collection of selected objects is expected to be created externally
    /// and set via SetSelection.
    HDX_API
    void SetSelection(HdSelectionSharedPtr const &selection) {
        _selection = selection;
        _IncrementVersion();
    }

    /// XXX: Rename to GetSelection
    HDX_API
    HdSelectionSharedPtr const &GetSelectionMap() const {
        return _selection;
    }

protected:
    /// Increments the internal selection state version, used for invalidation
    /// via GetVersion().
    HDX_API
    void _IncrementVersion();

    HDX_API
    virtual bool _GetSelectionOffsets(HdSelection::HighlightMode const& mode,
                                      HdRenderIndex const* index,
                                      size_t modeOffset,
                                      std::vector<int>* offsets) const;

private:
    int _version;
    HdSelectionSharedPtr _selection;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_SELECTION_TRACKER_H
