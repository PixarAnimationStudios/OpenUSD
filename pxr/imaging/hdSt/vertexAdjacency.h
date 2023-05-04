//
// Copyright 2023 Pixar
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
#ifndef PXR_IMAGING_HD_ST_VERTEX_ADJACENCY_H
#define PXR_IMAGING_HD_ST_VERTEX_ADJACENCY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/vertexAdjacency.h"

#include "pxr/base/vt/array.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


using HdSt_VertexAdjacencyBuilderSharedPtr = 
    std::shared_ptr<class HdSt_VertexAdjacencyBuilder>;
using HdSt_VertexAdjacencyBuilderComputationSharedPtr = 
    std::shared_ptr<class HdSt_VertexAdjacencyBuilderComputation>;
using HdSt_VertexAdjacencyBuilderComputationPtr = 
    std::weak_ptr<class HdSt_VertexAdjacencyBuilderComputation>;

class HdMeshTopology;

/// \class HdSt_VertexAdjacencyBuilder
///
class HdSt_VertexAdjacencyBuilder final
{
public:
    HDST_API
    HdSt_VertexAdjacencyBuilder();

    HDST_API
    ~HdSt_VertexAdjacencyBuilder();

    Hd_VertexAdjacency const *GetVertexAdjacency() const {
        return &_vertexAdjacency;
    }

    /// Returns a shared adjacency builder computation which will call
    /// BuildAdjacencyTable.  The shared computation is useful if multiple
    /// meshes share a topology and adjacency table, and only want to build the
    /// adjacency table once.
    HDST_API
    HdBufferSourceSharedPtr GetSharedVertexAdjacencyBuilderComputation(
        HdMeshTopology const *topology);

    /// Sets the buffer range used for adjacency table storage.
    void SetVertexAdjacencyRange(HdBufferArrayRangeSharedPtr const &range) {
        _vertexAdjacencyRange = range;
    }

    /// Returns the buffer range used for adjacency table storage.
    HdBufferArrayRangeSharedPtr const &GetVertexAdjacencyRange() const {
        return _vertexAdjacencyRange;
    }

private:
    Hd_VertexAdjacency _vertexAdjacency;

    // adjacency buffer range
    HdBufferArrayRangeSharedPtr _vertexAdjacencyRange;

    // weak ptr of adjacency builder used for dependent computations
    HdSt_VertexAdjacencyBuilderComputationPtr _sharedVertexAdjacencyBuilder;
};

/// \class HdSt_VertexAdjacencyBuilderComputation
///
/// A null buffer source to compute the adjacency table.
/// Since this is a null buffer source, it won't actually produce buffer
/// output; but other computations can depend on this to ensure
/// BuildAdjacencyTable is called.
///
class HdSt_VertexAdjacencyBuilderComputation : public HdNullBufferSource
{
public:
    HDST_API
    HdSt_VertexAdjacencyBuilderComputation(Hd_VertexAdjacency *vertexAdjacency,
                                           HdMeshTopology const *topology);
    HDST_API
    bool Resolve() override;

protected:
    HDST_API
    bool _CheckValid() const override;

private:
    Hd_VertexAdjacency *_vertexAdjacency;
    HdMeshTopology const *_topology;
};

/// \class HdSt_VertexAdjacencyBufferSource
///
/// A buffer source that puts an already computed adjacency table into
/// a resource registry buffer. This computation should be dependent on an
/// HdSt_VertexAdjacencyBuilderComputation.
///
class HdSt_VertexAdjacencyBufferSource : public HdComputedBufferSource
{
public:
    HDST_API
    HdSt_VertexAdjacencyBufferSource(
        Hd_VertexAdjacency const *vertexAdjacency,
        HdBufferSourceSharedPtr const &vertexAdjacencyBuilder);

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

    HDST_API
    bool Resolve() override;

protected:
    HDST_API
    bool _CheckValid() const override;

private:
    Hd_VertexAdjacency const *_vertexAdjacency;
    HdBufferSourceSharedPtr const _vertexAdjacencyBuilder;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_VERTEX_ADJACENCY_H
