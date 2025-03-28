//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_VERTEX_ADJACENCY_H
#define PXR_IMAGING_HD_VERTEX_ADJACENCY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/base/vt/array.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


using Hd_VertexAdjacencySharedPtr = 
    std::shared_ptr<class Hd_VertexAdjacency>;

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
class Hd_VertexAdjacency final
{
public:
    HD_API
    Hd_VertexAdjacency();

    HD_API
    ~Hd_VertexAdjacency();

    /// Updates the internal adjacency table using the supplied topology.
    /// Important! The adjacency table needs to be computed before smooth
    /// normals.
    HD_API
    void BuildAdjacencyTable(HdMeshTopology const *topology);

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
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_VERTEX_ADJACENCY_H
