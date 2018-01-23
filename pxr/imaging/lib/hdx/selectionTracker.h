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
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/path.h"
#include <boost/smart_ptr.hpp>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdRenderIndex;
class TfToken;
class SdfPath;
class VtValue;

typedef std::shared_ptr<class HdxSelection> HdxSelectionSharedPtr;
typedef std::shared_ptr<class HdxSelectionTracker> HdxSelectionTrackerSharedPtr;
typedef std::weak_ptr<class HdxSelectionTracker> HdxSelectionTrackerWeakPtr;


enum HdxSelectionHighlightMode {
    HdxSelectionHighlightModeSelect = 0,
    HdxSelectionHighlightModeLocate,
    HdxSelectionHighlightModeMask,

    HdxSelectionHighlightModeCount
};

/// \class HdxSelection
///
/// HdxSelection holds a collection of items which are rprims, instances of
/// rprim, sub elements of rprim (such as faces, verts). HdxSelectionTracker
/// takes HdxSelection and generates GPU buffer to be used for highlighting.
///
class HdxSelection {
public:
    typedef TfHashMap<SdfPath, std::vector<VtIntArray>, SdfPath::Hash> InstanceMap;
    typedef TfHashMap<SdfPath, VtIntArray, SdfPath::Hash> ElementMap;

    HdxSelection() = default;

    HDX_API
    void AddRprim(HdxSelectionHighlightMode const& mode,
                  SdfPath const &path);

    HDX_API
    void AddInstance(HdxSelectionHighlightMode const& mode,
                     SdfPath const &path,
                     VtIntArray const &instanceIndex=VtIntArray());

    HDX_API
    void AddElements(HdxSelectionHighlightMode const& mode,
                     SdfPath const &path,
                     VtIntArray const &elementIndices);

    SdfPathVector const&
    GetSelectedPrims(HdxSelectionHighlightMode const& mode) const;

    InstanceMap const&
    GetSelectedInstances(HdxSelectionHighlightMode const& mode) const;

    ElementMap const&
    GetSelectedElements(HdxSelectionHighlightMode const& mode) const;

protected:
    struct _SelectedEntities {
        // The SdfPaths are expected to be resolved rprim paths,
        // root paths will not be expanded.
        // Duplicated entries are allowed.
        SdfPathVector prims;

        /// This maps from prototype path to a vector of instance indices which is
        /// also a vector (because of nested instancing).
        InstanceMap instances;

        // The selected elements (faces, points, edges) , if any, for the selected
        // objects. This maps from object path to a vector of element indices.
        ElementMap elements;
    };

    _SelectedEntities _selEntities[HdxSelectionHighlightModeCount];
};

/// \class HdxSelectionTracker
///
/// HdxSelectionTracker is a base class for observing selection state and
/// providing selection highlighting details to interested clients.
///
class HdxSelectionTracker {
public:
    HDX_API
    HdxSelectionTracker();
    virtual ~HdxSelectionTracker() = default;

    /// Update dirty bits in the ChangeTracker and compute required primvars for
    /// later consumption.
    HDX_API
    virtual void Sync(HdRenderIndex* index);

    /// Populates an array of offsets required for selection highlighting for
    /// the given highlight mode.
    /// Returns true if offsets has anything selected.
    HDX_API
    virtual bool GetSelectionOffsetBuffer(HdRenderIndex const* index,
                                          VtIntArray* offsets) const;

    /// Returns a monotonically increasing version number, which increments
    /// whenever the result of GetBuffers has changed. Note that this number may
    /// overflow and become negative, thus clients should use a not-equal
    /// comparison.
    HDX_API
    int GetVersion() const;

    void SetSelection(HdxSelectionSharedPtr const &selection) {
        _selection = selection;
        _IncrementVersion();
    }

    HdxSelectionSharedPtr const &GetSelectionMap() const {
        return _selection;
    }

protected:
    /// Increments the internal selection state version, used for invalidation
    /// via GetVersion().
    HDX_API
    void _IncrementVersion();

    HDX_API
    virtual bool _GetSelectionOffsets(HdxSelectionHighlightMode const& mode,
                                      HdRenderIndex const* index,
                                      std::vector<int>* offsets) const;

private:
    int _version;
    HdxSelectionSharedPtr _selection;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_SELECTION_TRACKER_H
