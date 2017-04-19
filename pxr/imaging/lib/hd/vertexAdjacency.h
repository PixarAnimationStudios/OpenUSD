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
#include "pxr/imaging/hd/glUtils.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/vt/array.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class Hd_VertexAdjacency> Hd_VertexAdjacencySharedPtr;
typedef boost::shared_ptr<class Hd_AdjacencyBuilderComputation> Hd_AdjacencyBuilderComputationSharedPtr;
typedef boost::weak_ptr<class Hd_AdjacencyBuilderComputation> Hd_AdjacencyBuilderComputationPtr;

class HdMeshTopology;

/// \class Hd_VertexAdjacency
///
/// Hd_VertexAdjacency encapsulates mesh adjacency information,
/// which is used for smooth normal computation.
///
/// Hd_VertexAdjacency provides 4 buffer computations. They are
/// adjacency building and compute smooth normals on CPU or GPU.
/// The dependencies between them will be internally created if necessary
/// (i.e. adjacency builder always runs before smooth normals).
///
/// HdMesh ---> HdMeshTopology
///        ---> Hd_VertexAdjacency ---> AdjacencyBuilder (for CPU/GPU smooth)
///                                ---> AdjacencyBuilderForGPU (for GPU smooth)
///                                ---> SmoothNormals (CPU smooth normals)
///                                ---> SmoothNormalsGPU (GPU smooth normals)
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

    /// Returns an array of the same size and type as the source points
    /// containing normal vectors computed by averaging the cross products
    /// of incident face edges.
    HD_API
    VtArray<GfVec3f> ComputeSmoothNormals(int numPoints,
                                          GfVec3f const * pointsPtr) const;
    HD_API
    VtArray<GfVec3d> ComputeSmoothNormals(int numPoints,
                                          GfVec3d const * pointsPtr) const;
    HD_API
    VtArray<HdVec4f_2_10_10_10_REV> ComputeSmoothNormalsPacked(int numPoints,
                                          GfVec3f const * pointsPtr) const;
    HD_API
    VtArray<HdVec4f_2_10_10_10_REV> ComputeSmoothNormalsPacked(int numPoints,
                                          GfVec3d const * pointsPtr) const;

    /// Returns the adjacency builder computation.
    /// This computaions generates adjacency table on CPU.
    HD_API
    HdBufferSourceSharedPtr GetAdjacencyBuilderComputation(
        HdMeshTopology const *topology);

    /// Returns the adjacency builder computation.
    /// This computaions generates adjacency table on GPU.
    HD_API
    HdBufferSourceSharedPtr GetAdjacencyBuilderForGPUComputation();

    ///
    /// \name Smooth normals
    /// @{

    /// Returns the smooth normal computation on CPU.
    /// This computation generates buffer source of computed normals
    /// to be transferred later. It requires adjacency table on CPU
    /// produced by AdjacencyBuilderComputation.
    HD_API
    HdBufferSourceSharedPtr GetSmoothNormalsComputation(
        HdBufferSourceSharedPtr const &points,
        TfToken const &dstName,
        bool packed=false);

    /// Returns the smooth normal computation on GPU.
    /// This computation requires adjacency table on GPU produced by
    /// AdjacencyBuilderForGPUComputation.
    HD_API
    HdComputationSharedPtr GetSmoothNormalsComputationGPU(
        TfToken const &srcName, TfToken const &dstName,
        GLenum srcDataType, GLenum dstDataType);

    /// @}

    /// Sets the adjacency range which locates the adjacency table on GPU.
    void SetAdjacencyRange(HdBufferArrayRangeSharedPtr const &range) {
        _adjacencyRange = range;
    }

    /// Returns the adjacency table range.
    HdBufferArrayRangeSharedPtr const &GetAdjacencyRange() const {
        return _adjacencyRange;
    }

    /// Returns the number of points in the adjacency table.
    int GetNumPoints() const {
        return _numPoints;
    }

    /// Returns the adjacency table.
    std::vector<int> const &GetAdjacencyTable() const {
        return _adjacencyTable;
    }

private:
    // only AdjacencyBuilder can generate _entry and _stride.
    friend class Hd_AdjacencyBuilderComputation;

    int _numPoints;
    std::vector<int> _adjacencyTable;

    // adjacency buffer range on GPU
    HdBufferArrayRangeSharedPtr _adjacencyRange;

    // weak ptr of cpu adjacency builder used for dependent computations
    Hd_AdjacencyBuilderComputationPtr _adjacencyBuilder;
};

/// \class Hd_AdjacencyBuilderComputation
///
/// Adjacency table computation CPU.
///
class Hd_AdjacencyBuilderComputation : public HdNullBufferSource {
public:
    HD_API
    Hd_AdjacencyBuilderComputation(Hd_VertexAdjacency *adjacency,
                                   HdMeshTopology const *topology);
    HD_API
    virtual bool Resolve();

protected:
    HD_API
    virtual bool _CheckValid() const;

private:
    Hd_VertexAdjacency *_adjacency;
    HdMeshTopology const *_topology;
};

/// \class Hd_AdjacencyBuilderForGPUComputation
///
/// Adjacency table computation GPU.
///
class Hd_AdjacencyBuilderForGPUComputation : public HdComputedBufferSource {
public:
    typedef boost::shared_ptr<class Hd_AdjacencyBuilderComputation>
        Hd_AdjacencyBuilderComputationSharedPtr;

    HD_API
    Hd_AdjacencyBuilderForGPUComputation(
        Hd_VertexAdjacency const *adjacency,
        Hd_AdjacencyBuilderComputationSharedPtr const &adjacencyBuilder);

    // overrides
    HD_API
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;
    HD_API
    virtual bool Resolve();

protected:
    HD_API
    virtual bool _CheckValid() const;

private:
    Hd_VertexAdjacency const *_adjacency;
    Hd_AdjacencyBuilderComputationSharedPtr const _adjacencyBuilder;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_VERTEX_ADJACENCY_H
