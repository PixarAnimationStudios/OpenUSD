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

#include "pxr/base/vt/array.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

typedef boost::shared_ptr<class Hd_VertexAdjacency> Hd_VertexAdjacencySharedPtr;
typedef boost::weak_ptr<class Hd_AdjacencyBuilderComputation> Hd_AdjacencyBuilderComputationPtr;

class HdMeshTopology;

/// \class Hd_VertexAdjacency
///
/// Hd_VertexAdjacency encapsulates mesh adjacency information,
/// which is used for smooth normal computation.
///
/// Hd_VertexAdjacency provides 4 buffer computations. They are
/// adajancey building and compute smooth normals on CPU or GPU.
/// The dependencies between them will be internally created if necessary
/// (i.e. adjacency builder always runs before smooth normals).
///
/// HdMesh ---> HdMeshTopology
///        ---> Hd_VertexAdjacency ---> AdjacencyBuilder (for CPU/GPU smooth)
///                                ---> AdjacencyBuilderForGPU (for GPU smooth)
///                                ---> SmoothNormals (CPU smooth normals)
///                                ---> SmoothNormalsGPU (GPU smooth normals)
///
class Hd_VertexAdjacency {
public:
    Hd_VertexAdjacency();

    /// Returns an array of the same size and type as the source points
    /// containing normal vectors computed by averaging the cross products
    /// of incident face edges.
    VtArray<GfVec3f> ComputeSmoothNormals(int numPoints,
                                          GfVec3f const * pointsPtr) const;
    VtArray<GfVec3d> ComputeSmoothNormals(int numPoints,
                                          GfVec3d const * pointsPtr) const;

    /// Returns the adjacency builder computation.
    /// This computaions generates adjacency table on CPU.
    HdBufferSourceSharedPtr GetAdjacencyBuilderComputation(
        HdMeshTopology const *topology);

    /// Returns the adjacency builder computation.
    /// This computaions generates adjacency table on GPU.
    HdBufferSourceSharedPtr GetAdjacencyBuilderForGPUComputation();

    ///
    /// \name Smooth normals
    /// @{

    /// Returns the smooth normal computation on CPU.
    /// This computation generates buffer source of computed normals
    /// to be transferred later. It requires adjacency table on CPU
    /// produced by AdjacencyBuilderComputation.
    HdBufferSourceSharedPtr GetSmoothNormalsComputation(
        HdBufferSourceSharedPtr const &points,
        TfToken const &dstName);

    /// Returns the smooth normal computation on GPU.
    /// This computation requires adjacency table on GPU produced by
    /// AdjacencyBuilderForGPUComputation.
    HdComputationSharedPtr GetSmoothNormalsComputationGPU(
        TfToken const &srcName, TfToken const &dstName, GLenum dstDataType);

    /// @}

    /// Sets the adjacency range which locates the adjacency table on GPU.
    void SetAdjacencyRange(HdBufferArrayRangeSharedPtr const &range) {
        _adjacencyRange = range;
    }

    /// Returns the adjacency table range.
    HdBufferArrayRangeSharedPtr const &GetAdjacencyRange() const {
        return _adjacencyRange;
    }

    /// Returns the stride of adjacency entry table.
    int GetStride() const {
        return _stride;
    }

    /// Returns the adjacency entry table.
    std::vector<int> const &GetEntry() const {
        return _entry;
    }

private:
    // only AdjacencyBuilder can generate _entry and _stride.
    friend class Hd_AdjacencyBuilderComputation;

    int _stride;
    std::vector<int> _entry;

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
    Hd_AdjacencyBuilderComputation(Hd_VertexAdjacency *adjacency,
                                   HdMeshTopology const *topology);
    virtual bool Resolve();

protected:
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

    Hd_AdjacencyBuilderForGPUComputation(
        Hd_VertexAdjacency const *adjacency,
        Hd_AdjacencyBuilderComputationSharedPtr const &adjacencyBuilder);

    // overrides
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;
    virtual bool Resolve();

protected:
    virtual bool _CheckValid() const;

private:
    Hd_VertexAdjacency const *_adjacency;
    Hd_AdjacencyBuilderComputationSharedPtr const _adjacencyBuilder;
};

#endif  // HD_VERTEX_ADJACENCY_H
