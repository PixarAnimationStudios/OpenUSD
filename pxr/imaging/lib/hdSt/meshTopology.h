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
#ifndef HDST_MESH_TOPOLOGY_H
#define HDST_MESH_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/types.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdStResourceRegistry;
class HdSt_Subdivision;
struct HdQuadInfo;
class SdfPath;

typedef boost::weak_ptr<class HdBufferSource> HdBufferSourceWeakPtr;
typedef boost::weak_ptr<class HdSt_AdjacencyBuilderComputation> HdSt_AdjacencyBuilderComputationPtr;
typedef boost::weak_ptr<class HdSt_QuadInfoBuilderComputation> HdSt_QuadInfoBuilderComputationPtr;
typedef boost::shared_ptr<class HdBufferSource> HdBufferSourceSharedPtr;
typedef boost::shared_ptr<class HdBufferArrayRange> HdBufferArrayRangeSharedPtr;
typedef boost::shared_ptr<class HdComputation> HdComputationSharedPtr;
typedef boost::shared_ptr<class HdSt_AdjacencyBuilderComputation> HdSt_AdjacencyBuilderComputationSharedPtr;
typedef boost::shared_ptr<class HdSt_QuadInfoBuilderComputation> HdSt_QuadInfoBuilderComputationSharedPtr;
typedef boost::shared_ptr<class HdSt_MeshTopology> HdSt_MeshTopologySharedPtr;

/// \class HdSt_MeshTopology
///
/// Hydra Stream implementation for mesh topology.
///
class HdSt_MeshTopology final : public HdMeshTopology {
public:
    /// Specifies how subdivision mesh topology is refined.
    enum RefineMode {
        RefineModeUniform = 0,
        RefineModePatches
    };

    static HdSt_MeshTopologySharedPtr New(
        const HdMeshTopology &src,
        int refineLevel,
        RefineMode refineMode = RefineModeUniform);

    virtual ~HdSt_MeshTopology();

    /// Equality check between two mesh topologies.
    bool operator==(HdSt_MeshTopology const &other) const;

    /// \name Triangulation
    /// @{

    /// Returns the triangle indices (for drawing) buffer source computation.
    HdBufferSourceSharedPtr GetTriangleIndexBuilderComputation(
        SdfPath const &id);

    /// Returns the CPU face-varying triangulate computation
    HdBufferSourceSharedPtr GetTriangulateFaceVaryingComputation(
        HdBufferSourceSharedPtr const &source,
        SdfPath const &id);

    /// @}

    ///
    /// \name Quadrangulation
    /// @{

    /// Returns the quadinfo computation for the use of primvar
    /// quadrangulation.
    /// If gpu is true, the quadrangulate table will be transferred to GPU
    /// via the resource registry.
    HdSt_QuadInfoBuilderComputationSharedPtr GetQuadInfoBuilderComputation(
        bool gpu, SdfPath const &id,
        HdStResourceRegistry *resourceRegistry = nullptr);

    /// Returns the quad indices (for drawing) buffer source computation.
    HdBufferSourceSharedPtr GetQuadIndexBuilderComputation(SdfPath const &id);

    /// Returns the CPU quadrangulated buffer source.
    HdBufferSourceSharedPtr GetQuadrangulateComputation(
        HdBufferSourceSharedPtr const &source, SdfPath const &id);

    /// Returns the GPU quadrangulate computation.
    HdComputationSharedPtr GetQuadrangulateComputationGPU(
        TfToken const &name, HdType dataType, SdfPath const &id);

    /// Returns the CPU face-varying quadrangulate computation
    HdBufferSourceSharedPtr GetQuadrangulateFaceVaryingComputation(
        HdBufferSourceSharedPtr const &source, SdfPath const &id);

    /// Returns the quadrangulation table range on GPU
    HdBufferArrayRangeSharedPtr const &GetQuadrangulateTableRange() const {
        return _quadrangulateTableRange;
    }

    /// Clears the quadrangulation table range
    void ClearQuadrangulateTableRange() {
        _quadrangulateTableRange.reset();
    }

    /// Sets the quadrangulation struct. HdMeshTopology takes an
    /// ownership of quadInfo (caller shouldn't free)
    void SetQuadInfo(HdQuadInfo const *quadInfo);

    /// Returns the quadrangulation struct.
    HdQuadInfo const *GetQuadInfo() const {
        return _quadInfo;
    }

    /// @}

    ///
    /// \name Points
    /// @{

    /// Returns the point indices buffer source computation.
    HdBufferSourceSharedPtr GetPointsIndexBuilderComputation();

    /// @}

    ///
    /// \name Subdivision
    /// @{


    /// Returns the subdivision struct.
    HdSt_Subdivision const *GetSubdivision() const {
        return _subdivision;
    }

    /// Returns the subdivision struct (non-const).
    HdSt_Subdivision *GetSubdivision() {
        return _subdivision;
    }

    /// Returns true if the subdivision on this mesh produces
    /// triangles (otherwise quads)
    bool RefinesToTriangles() const;

    /// Returns true if the subdivision on this mesh produces patches
    bool RefinesToBSplinePatches() const;

    /// Returns the subdivision topology computation. It computes
    /// far mesh and produces refined quad-indices buffer.
    HdBufferSourceSharedPtr GetOsdTopologyComputation(SdfPath const &debugId);

    /// Returns the refined indices builder computation.
    /// this just returns index and primitive buffer, and should be preceded by
    /// topology computation.
    HdBufferSourceSharedPtr GetOsdIndexBuilderComputation();

    /// Returns the subdivision primvar refine computation on CPU.
    HdBufferSourceSharedPtr GetOsdRefineComputation(
        HdBufferSourceSharedPtr const &source, bool varying);

    /// Returns the subdivision primvar refine computation on GPU.
    HdComputationSharedPtr GetOsdRefineComputationGPU(
        TfToken const &name, HdType dataType);

    /// @}

private:
    // quadrangulation info on CPU
    HdQuadInfo const *_quadInfo;

    // quadrangulation info on GPU
    HdBufferArrayRangeSharedPtr _quadrangulateTableRange;

    HdSt_QuadInfoBuilderComputationPtr _quadInfoBuilder;

    // OpenSubdiv
    RefineMode _refineMode;
    HdSt_Subdivision *_subdivision;
    HdBufferSourceWeakPtr _osdTopologyBuilder;

    // Must be created through factory
    explicit HdSt_MeshTopology(
        const HdMeshTopology &src,
        int refineLevel,
        RefineMode refineMode);

    // No default construction or copying.
    HdSt_MeshTopology()                                      = delete;
    HdSt_MeshTopology(const HdSt_MeshTopology &)             = delete;
    HdSt_MeshTopology &operator =(const HdSt_MeshTopology &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_MESH_TOPOLOGY_H
