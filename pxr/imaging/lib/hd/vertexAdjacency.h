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
#ifndef HD_VERTEX_ADJACENCY_H
#define HD_VERTEX_ADJACENCY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"

#include "pxr/base/vt/array.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class Hd_AdjacencyBuilderComputation> Hd_AdjacencyBuilderComputationSharedPtr;
typedef boost::weak_ptr<class Hd_AdjacencyBuilderComputation> Hd_AdjacencyBuilderComputationPtr;

class HdMeshTopology;

/// \class Hd_VertexAdjacency
///
/// Hd_VertexAdjacency encapsulates mesh adjacency information,
/// which is used for smooth normal computation.
///
/// The Adjacency table (built by the AdjacencyBuilder computation)
/// provides the index of the previous and next vertex for each face
/// that uses that vertex.
///
/// The table is split into two parts. The first part of the table
/// provides a offset to the prev/next data for the vertex as well as the
/// number of faces that use the vertex.  The second part of the table
/// provides the actual prev/next indices.
///
/// For example, The following prim has 4 vertices and 2 faces and uses
/// a CCW winding order:
///
///     3.---.2
///      |  /|
///      | / |
///      |/  |
///     0.---.1
///
/// Picking one vertex, 0, it is used by 2 faces, so it contains 2 previous/
/// next pairs: (2, 1) and (3, 2)
///
/// The full adjacency table for this prim would be:
///
///  0  1 |  2  3 |  4  5 |  6  7 || 8  9  10 11 | 12 13 | 14 15 16 17 | 18 19
///  8  2 | 12  1 | 14  2 | 18  1 || 2  1   3  2 |  0  2 |  1  0  0  3 |  2  0
///   Offset / Count pairs        ||            Prev / Next Pairs
///      per vertex               ||           Per Vertex, Per Face.
///
class Hd_VertexAdjacency final {
public:
    HD_API
    Hd_VertexAdjacency();

    HD_API
    ~Hd_VertexAdjacency();

    /// Updates the internal adjacency table using the supplied topology.
    /// Important! The adjacency table needs to be computed before smooth
    /// normals.
    HD_API
    bool BuildAdjacencyTable(HdMeshTopology const *topology);

    /// Returns a shared adjacency builder computation which will call
    /// BuildAdjacencyTable.  The shared computation is useful if multiple
    /// meshes share a topology and adjacency table, and only want to build the
    /// adjacency table once.
    HD_API
    HdBufferSourceSharedPtr GetSharedAdjacencyBuilderComputation(
        HdMeshTopology const *topology);

    /// Sets the buffer range used for adjacency table storage.
    void SetAdjacencyRange(HdBufferArrayRangeSharedPtr const &range) {
        _adjacencyRange = range;
    }

    /// Returns the buffer range used for adjacency table storage.
    HdBufferArrayRangeSharedPtr const &GetAdjacencyRange() const {
        return _adjacencyRange;
    }

    /// Returns the number of points in the adjacency table.
    int GetNumPoints() const {
        return _numPoints;
    }

    /// Returns the adjacency table.
    VtIntArray const &GetAdjacencyTable() const {
        return _adjacencyTable;
    }

private:
    int _numPoints;
    VtIntArray _adjacencyTable;

    // adjacency buffer range
    HdBufferArrayRangeSharedPtr _adjacencyRange;

    // weak ptr of adjacency builder used for dependent computations
    Hd_AdjacencyBuilderComputationPtr _sharedAdjacencyBuilder;
};

/// \class Hd_AdjacencyBuilderComputation
///
/// A null buffer source to compute the adjacency table.
/// Since this is a null buffer source, it won't actually produce buffer
/// output; but other computations can depend on this to ensure
/// BuildAdjacencyTable is called.
///
class Hd_AdjacencyBuilderComputation : public HdNullBufferSource {
public:
    HD_API
    Hd_AdjacencyBuilderComputation(Hd_VertexAdjacency *adjacency,
                                   HdMeshTopology const *topology);
    HD_API
    virtual bool Resolve() override;

protected:
    HD_API
    virtual bool _CheckValid() const override;

private:
    Hd_VertexAdjacency *_adjacency;
    HdMeshTopology const *_topology;
};

/// \class Hd_AdjacencyBufferSource
///
/// A buffer source that puts an already computed adjacency table into
/// a resource registry buffer. This computation should be dependent on an
/// Hd_AdjacencyBuilderComputation.
///
class Hd_AdjacencyBufferSource : public HdComputedBufferSource {
public:
    HD_API
    Hd_AdjacencyBufferSource(
        Hd_VertexAdjacency const *adjacency,
        HdBufferSourceSharedPtr const &adjacencyBuilder);

    // overrides
    HD_API
    virtual void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    HD_API
    virtual bool Resolve() override;

protected:
    HD_API
    virtual bool _CheckValid() const override;

private:
    Hd_VertexAdjacency const *_adjacency;
    HdBufferSourceSharedPtr const _adjacencyBuilder;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_VERTEX_ADJACENCY_H
