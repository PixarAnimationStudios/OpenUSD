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
#ifndef HD_MESH_TOPOLOGY_H
#define HD_MESH_TOPOLOGY_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/topology.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

typedef boost::shared_ptr<class HdBufferSource> HdBufferSourceSharedPtr;
typedef boost::shared_ptr<class Hd_AdjacencyBuilderComputation> Hd_AdjacencyBuilderComputationSharedPtr;
typedef boost::shared_ptr<class Hd_QuadInfoBuilderComputation> Hd_QuadInfoBuilderComputationSharedPtr;
typedef boost::weak_ptr<class Hd_AdjacencyBuilderComputation> Hd_AdjacencyBuilderComputationPtr;
typedef boost::weak_ptr<class Hd_QuadInfoBuilderComputation> Hd_QuadInfoBuilderComputationPtr;
class HdResourceRegistry;
class Hd_Subdivision;
class Hd_QuadInfo;
class Hd_VertexAdjacency;
typedef boost::shared_ptr<class HdMeshTopology> HdMeshTopologySharedPtr;

/// \class HdMeshTopology
///
/// Topology data for meshes.
///
/// HtMeshTopology holds the raw input topology data for a mesh and is capable
/// of computing derivative topological data (such as indices or subdivision
/// stencil tables and patch tables).
///
class HdMeshTopology : public HdTopology {
public:

    HdMeshTopology();
    HdMeshTopology(const HdMeshTopology &, int refineLevel=0);
    HdMeshTopology(const PxOsdMeshTopology &, int refineLevel=0);
    HdMeshTopology(
        TfToken scheme,
        TfToken orientation,
        VtIntArray faceVertexCounts,
        VtIntArray faceVertexIndices,
        int refineLevel=0);
    HdMeshTopology(
        TfToken scheme,
        TfToken orientation,
        VtIntArray faceVertexCounts,
        VtIntArray faceVertexIndices,
        VtIntArray holeIndices,
        int refineLevel=0);
    virtual ~HdMeshTopology();

#if defined(HD_SUPPORT_OPENSUBDIV2)
    /// Returns whether OpenSubdiv 3.0 to be used.
    static bool IsEnabledOpenSubdiv3();
#endif

    /// Returns whether adaptive subdivision is enabled or not.
    static bool IsEnabledAdaptive();

    PxOsdMeshTopology const & GetPxOsdMeshTopology() const {
        return _topology;
    }

    /// Returns the num faces
    int GetNumFaces() const;

    /// Returns the num facevarying primvars
    int GetNumFaceVaryings() const;

    /// Returns the num points by looking vert indices array
    int ComputeNumPoints() const;

    /// Returns the num points by looking vert indices array
    static int ComputeNumPoints(VtIntArray const &verts);

    /// Returns the number of quadrangulated quads.
    /// If degenerated face is found, sets invalidFaceFound as true.
    static int ComputeNumQuads(VtIntArray const &numVerts,
                               VtIntArray const &holeIndices,
                               bool *invalidFaceFound=NULL);

    /// Returns the subdivision scheme
    TfToken const GetScheme() const {
        return _topology.GetScheme();
    }

    /// Returns the refinement level
    int GetRefineLevel() const {
        return _refineLevel;
    }

    /// Returns face vertex counts.
    VtIntArray const &GetFaceVertexCounts() const {
        return _topology.GetFaceVertexCounts();
    }

    /// Returns face vertex indics.
    VtIntArray const &GetFaceVertexIndices() const {
        return _topology.GetFaceVertexIndices();
    }

    /// Returns orientation.
    TfToken const &GetOrientation() const {
        return _topology.GetOrientation();
    }

    /// Returns the hash value of this topology to be used for instancing.
    virtual ID ComputeHash() const;

    /// Equality check between two mesh topologies.
    bool operator==(HdMeshTopology const &other) const;

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
    Hd_QuadInfoBuilderComputationSharedPtr GetQuadInfoBuilderComputation(
        bool gpu, SdfPath const &id,
        HdResourceRegistry *resourceRegistry=NULL);

    /// Returns the quad indices (for drawing) buffer source computation.
    HdBufferSourceSharedPtr GetQuadIndexBuilderComputation(SdfPath const &id);

    /// Returns the CPU quadrangulated buffer source.
    HdBufferSourceSharedPtr GetQuadrangulateComputation(
        HdBufferSourceSharedPtr const &source, SdfPath const &id);

    /// Returns the GPU quadrangulate computation.
    HdComputationSharedPtr GetQuadrangulateComputationGPU(
        TfToken const &name, GLenum dataType, SdfPath const &id);

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
    void SetQuadInfo(Hd_QuadInfo const *quadInfo);

    /// Returns the quadrangulation struct.
    Hd_QuadInfo const *GetQuadInfo() const {
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
    /// \name Hole
    /// @{

    /// Sets hole face indices. faceIndices needs to be sorted in
    /// ascending order.
    void SetHoleIndices(VtIntArray const &holeIndices) {
        _topology.SetHoleIndices(holeIndices);
    }

    /// Returns the hole face indices.
    VtIntArray const &GetHoleIndices() const {
        return _topology.GetHoleIndices();
    }

    /// @}

    ///
    /// \name Subdivision
    /// @{


    /// Sets subdivision tags.
    void SetSubdivTags(PxOsdSubdivTags const &subdivTags) {
        _topology.SetSubdivTags(subdivTags);
    }

    /// Returns subdivision tags
    PxOsdSubdivTags &GetSubdivTags() {
        return _topology.GetSubdivTags();
    }

    /// Returns the subdivision struct.
    Hd_Subdivision const *GetSubdivision() const {
        return _subdivision;
    }

    /// Returns the subdivision struct (non-const).
    Hd_Subdivision *GetSubdivision() {
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
        TfToken const &name, GLenum dataType, int numComponents);

    /// @}

    // Per-primitive coarse-face-param encoding/decoding functions
    static int EncodeCoarseFaceParam(int faceIndex, int edgeFlag) {
        return ((faceIndex << 2) | (edgeFlag & 3));
    }
    static int DecodeFaceIndexFromCoarseFaceParam(int coarseFaceParam) {
        return (coarseFaceParam >> 2);
    }
    static int DecodeEdgeFlagFromCoarseFaceParam(int coarseFaceParam) {
        return (coarseFaceParam & 3);
    }

private:
    // Computes smooth normals using vertex adjacency.
    template <typename Vec3Type>
    VtArray<Vec3Type>
    _ComputeSmoothNormals(int numPoints, Vec3Type const * pointsPtr);

private:

    PxOsdMeshTopology _topology;
    int _refineLevel;

    // quadrangulation info on CPU
    Hd_QuadInfo const *_quadInfo;

    // quadrangulation info on GPU
    HdBufferArrayRangeSharedPtr _quadrangulateTableRange;

    Hd_QuadInfoBuilderComputationPtr _quadInfoBuilder;

    // OpenSubdiv
    Hd_Subdivision *_subdivision;
    HdBufferSourceWeakPtr _osdTopologyBuilder;
};

#endif // HD_MESH_TOPOLOGY_H
