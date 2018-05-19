//
// Copyright 2018 Pixar
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
#ifndef HD_SELECTION_H
#define HD_SELECTION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include <boost/smart_ptr.hpp>
#include <vector>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;

typedef boost::shared_ptr<class HdSelection> HdSelectionSharedPtr;

/// \class HdSelection
///
/// HdSelection holds a collection of selected items per selection mode.
/// The items may be rprims, instances of an rprim and subprimitives of an
/// rprim, such as elements (faces for meshes, individual curves for basis
/// curves), edges & points.
/// 
class HdSelection {
public:
    /// Selection modes allow differentiation in selection highlight behavior.
    enum HighlightMode {
        HighlightModeSelect = 0,
        HighlightModeLocate,
        HighlightModeMask,

        HighlightModeCount
    };

    HdSelection() = default;

    HD_API
    void AddRprim(HighlightMode const& mode,
                  SdfPath const &path);

    HD_API
    void AddInstance(HighlightMode const& mode,
                     SdfPath const &path,
                     VtIntArray const &instanceIndex=VtIntArray());

    HD_API
    void AddElements(HighlightMode const& mode,
                     SdfPath const &path,
                     VtIntArray const &elementIndices);
    
    HD_API
    void AddEdges(HighlightMode const& mode,
                  SdfPath const &path,
                  VtIntArray const &edgeIndices);
    
    HD_API
    void AddPoints(HighlightMode const& mode,
                   SdfPath const &path,
                   VtIntArray const &pointIndices);

    // XXX: Ideally, this should be per instance, if we want to support
    // selection of subprims (faces/edges/points) per instance of an rprim.
    // By making this per rprim, all selected instances of the rprim will share
    // the same subprim highlighting.
    struct PrimSelectionState {
        PrimSelectionState() : fullySelected(false) {}
        bool fullySelected;
        // We use a vector of VtIntArray, to avoid any copy of indices data.
        // This way, we support multiple  Add<Subprim> operations, without 
        // having to consolidate the indices each time.
        std::vector<VtIntArray> instanceIndices;
        std::vector<VtIntArray> elementIndices;
        std::vector<VtIntArray> edgeIndices;
        std::vector<VtIntArray> pointIndices;
    };

    HD_API
    PrimSelectionState const*
    GetPrimSelectionState(HighlightMode const &mode,
                          SdfPath const &path) const;

    HD_API
    SdfPathVector
    GetSelectedPrimPaths(HighlightMode const &mode) const;

protected:
    
    typedef std::unordered_map<SdfPath, PrimSelectionState, SdfPath::Hash>
        _PrimSelectionStateMap;
    // Keep track of selection per selection mode.
    _PrimSelectionStateMap _selMap[HighlightModeCount];
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_SELECTION_H
