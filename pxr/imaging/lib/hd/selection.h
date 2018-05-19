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

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;

typedef boost::shared_ptr<class HdSelection> HdSelectionSharedPtr;

/// \class HdSelection
///
/// HdSelection holds a collection of items which are rprims, instances of
/// rprim and subprimitives of rprim, such as elements (faces when dealing with 
/// meshes, individual curves when dealing with basis curves) and edges.
/// 
/// HdSelectionTracker takes HdSelection and generates a GPU buffer to be used 
/// for highlighting.
///
class HdSelection {
public:
    enum HighlightMode {
        HighlightModeSelect = 0,
        HighlightModeLocate,
        HighlightModeMask,

        HighlightModeCount
    };

    typedef TfHashMap<SdfPath, std::vector<VtIntArray>, SdfPath::Hash> InstanceMap;
    typedef TfHashMap<SdfPath, VtIntArray, SdfPath::Hash> ElementIndicesMap;
    typedef ElementIndicesMap EdgeIndicesMap;
    typedef ElementIndicesMap PointIndicesMap;

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

    HD_API
    SdfPathVector const&
    GetSelectedPrims(HighlightMode const& mode) const;

    HD_API
    InstanceMap const&
    GetSelectedInstances(HighlightMode const& mode) const;

    HD_API
    ElementIndicesMap const&
    GetSelectedElements(HighlightMode const& mode) const;

    HD_API
    EdgeIndicesMap const&
    GetSelectedEdges(HighlightMode const& mode) const;
    
    HD_API
    PointIndicesMap const&
    GetSelectedPoints(HighlightMode const& mode) const;

protected:
    struct _SelectedEntities {
        // The SdfPaths are expected to be resolved rprim paths,
        // root paths will not be expanded.
        // Duplicated entries are allowed.
        SdfPathVector prims;

        /// This maps from prototype path to a vector of instance indices which is
        /// also a vector (because of nested instancing).
        InstanceMap instances;

        // The selected elements, if any, for the selected objects. This maps
        // from object path to a vector of element indices.
        ElementIndicesMap elements;

        // The selected edges, if any, for the selected objects. This maps from 
        // object path to a vector of (authored) edge indices.
        EdgeIndicesMap edges;

        // The selected points, if any, for the selected objects. This maps from
        // object path to a vector of point (vertex) indices.
        PointIndicesMap points;
    };

    _SelectedEntities _selEntities[HighlightModeCount];
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_SELECTION_H
