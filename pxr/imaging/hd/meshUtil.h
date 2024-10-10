//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_MESH_UTIL_H
#define PXR_IMAGING_HD_MESH_UTIL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdQuadInfo
/// A helper class for quadrangulation computation.

// v0           v2
// +-----e2----+
//  \    |    /
//   \ __c__ /
//   e0     e1
//     \   /
//      \ /
//       + v1
//
//
//  original points       additional center and edge points
// +------------ ... ----+--------------------------------+
// | v0 v1 v2         vn | e0 e1 e2 c0, e3 e4 e5 c1 ...   |
// +------------ ... ----+--------------------------------+
//                       ^
//                   pointsOffset
//                       <----- numAdditionalPoints  ---->

struct HdQuadInfo {
    HdQuadInfo() : pointsOffset(0), numAdditionalPoints(0), maxNumVert(0) { }

    /// Returns true if the mesh is all-quads.
    bool IsAllQuads() const { return numAdditionalPoints == 0; }

    int pointsOffset;
    int numAdditionalPoints;
    int maxNumVert;
    std::vector<int> numVerts;  // num vertices of non-quads
    std::vector<int> verts;     // vertex indices of non-quads
};

/// \class HdMeshUtil
/// A collection of utility algorithms for generating triangulation
/// and quadrangulation of an input topology.

class HdMeshUtil
{
public:
    HdMeshUtil(HdMeshTopology const* topology, SdfPath const& id)
        : _topology(topology), _id(id) {}
    virtual ~HdMeshUtil() {}

    // --------------------------------------------------------------------
    /// \name Triangulation
    ///
    /// Produces a mesh where each non-triangle face in the base mesh topology
    /// is fan-triangulated such that the resulting mesh consists entirely
    /// of triangles.
    ///
    /// In order to access per-face signals (face color, face selection etc)
    /// we need a mapping from primitiveID to authored face index domain.
    /// This is encoded in primitiveParams, and computed along with indices.
    /// See \ref PrimitiveParamEncoding.
    /// @{
    /*
                 +--------+-------+
                /| \      |\      |\
               / |  \  1  | \  2  | \
              /  |   \    |  \    |  \
             /   |    \   |   \   | 2 +
            / 0  |  1  \  | 2  \  |  /
           /     |      \ |     \ | /
          /      |       \|      \|/
         +-------+--------+-------+
    */

    /// Return a triangulation of the input topology.  indices and
    /// primitiveParams are output parameters.
    HD_API
    void ComputeTriangleIndices(VtVec3iArray *indices,
                                VtIntArray *primitiveParams,
                                VtIntArray *edgeIndices = nullptr) const;

    /// Return a triangulation of a face-varying primvar. source is
    /// a buffer of size numElements and type corresponding to dataType
    /// (e.g. HdTypeFloatVec3); the result is a VtArray<T> of the
    /// correct type written to the variable "triangulated".
    /// This function returns false if it can't resolve dataType.
    HD_API
    bool ComputeTriangulatedFaceVaryingPrimvar(void const* source,
                                               int numElements,
                                               HdType dataType,
                                               VtValue *triangulated) const;

    /// @}

    // --------------------------------------------------------------------
    /// \name Quadrangulation
    ///
    /// Produces a mesh where each non-quad face in the base mesh topology
    /// is quadrangulated such that the resulting mesh consists entirely
    /// of quads. Additionally, supports splitting each resulting quad
    /// face into a pair of triangles. This is different than simply
    /// triangulating the base mesh topology and can be useful for
    /// maintaining consistency with quad-based subdivision schemes.
    ///
    /// In order to access per-face signals (face color, face selection etc)
    /// we need a mapping from primitiveID to authored face index domain.
    /// This is encoded in primitiveParams, and computed along with indices.
    /// See \ref PrimitiveParamEncoding.
    /// @{

    /*
               +--------+-------+
              /|        |    |   \
             / |        |  2 | 2 /\
            /  |        |     \ /  \
           / 0 |    1   |------+  2 +
          /\  /|        |     / \  /
         /  \/ |        |  2 | 2 \/
        / 0 | 0|        |    |   /
       +-------+--------+-------+
    */

    /// Generate a quadInfo struct for the input topology.
    HD_API
    void ComputeQuadInfo(HdQuadInfo* quadInfo) const;

    /// Return quadrangulated indices of the input topology. indices and
    /// primitiveParams are output parameters.
    HD_API
    void ComputeQuadIndices(VtIntArray *indices,
                            VtIntArray *primitiveParams,
                            VtVec2iArray *edgeIndices = nullptr) const;

    /// Return triquad indices (triangulated after quadrangulation) of the
    /// input topology. indices and primitiveParams are output parameters.
    HD_API
    void ComputeTriQuadIndices(VtIntArray *indices,
                               VtIntArray *primitiveParams,
                               VtVec2iArray *edgeIndices = nullptr) const;

    /// Return a quadrangulation of a per-vertex primvar. source is
    /// a buffer of size numElements and type corresponding to dataType
    /// (e.g. HdTypeFloatVec3); the result is a VtArray<T> of the
    /// correct type written to the variable "quadrangulated".
    /// This function returns false if it can't resolve dataType.
    HD_API
    bool ComputeQuadrangulatedPrimvar(HdQuadInfo const* qi,
                                      void const* source,
                                      int numElements,
                                      HdType dataType,
                                      VtValue *quadrangulated) const;

    /// Return a quadrangulation of a face-varying primvar.
    /// source is a buffer of size numElements and type corresponding
    /// to dataType (e.g. HdTypeFloatVec3); the result is a VtArray<T> of the
    /// correct type written to the variable "quadrangulated".
    /// This function returns false if it can't resolve dataType.
    HD_API
    bool ComputeQuadrangulatedFaceVaryingPrimvar(void const* source,
                                                 int numElements,
                                                 HdType dataType,
                                                 VtValue *quadrangulated) const;

    /// @}

    /// Return a buffer filled with face vertex index pairs corresponding
    /// to the sequence in which edges are visited when iterating through
    /// the mesh topology. The edges of degenerate and hole faces are
    /// included so that this sequence will correspond with either base
    /// face triangulation or quadrangulation (which typically skips
    /// over hole faces) as well as for refined surfaces which take into
    /// account faces tagged as holes as well as other non-manifold faces.
    /// Optionally, records the first edge index for each face.
    /// Subsequent edge indices for each face are implicitly assigned
    /// sequentially following the first edge index.
    HD_API
    void EnumerateEdges(
        std::vector<GfVec2i> * edgeVerticesOut,
        std::vector<int> * firstEdgeIndexForFacesOut = nullptr) const;

    // --------------------------------------------------------------------
    /// \anchor PrimitiveParamEncoding
    /// \name Primitive Param bit encoding
    ///
    /// This encoding provides information about each sub-face resulting
    /// from the triangulation or quadrangulation of a base topology face.
    ///
    /// The encoded faceIndex is the index of the base topology face
    /// corresponding to a triangulated or quadrangulated sub-face.
    ///
    /// The encoded edge flag identifies where a sub-face occurs in the
    /// sequence of sub-faces produced for each base topology face.
    /// This edge flag can be used to determine which edges of a sub-face
    /// correspond to edges of a base topology face and which are internal
    /// edges that were introduced by triangulation or quadrangulation:
    /// - 0 unaffected triangle or quad base topology face
    /// - 1 first sub-face produced by triangulation or quadrangulation
    /// - 2 last sub-face produced by triangulation or quadrangulation
    /// - 3 intermediate sub-face produced by triangulation or quadrangulation
    /// @{

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

    /// }@

private:
    /// Return the number of quadrangulated quads.
    /// If degenerate face is found, sets invalidFaceFound as true.
    int _ComputeNumQuads(VtIntArray const &numVerts,
                         VtIntArray const &holeIndices,
                         bool *invalidFaceFound = nullptr) const;

    /// Return quad indices (optionally triangulated after quadrangulation).
    void _ComputeQuadIndices(
                            VtIntArray *indices,
                            VtIntArray *primitiveParams,
                            VtVec2iArray *edgeIndices,
                            bool triangulate = false) const;

    HdMeshTopology const* _topology;
    SdfPath const _id;
};

/// \class HdMeshEdgeIndexTable
///
/// Mesh edges are described as a pair of adjacent vertices encoded
/// as GfVec2i.
///
/// The encoding of mesh edge indices is derived from the enumeration
/// of face vertex index pairs provided by HdMeshUtil::EnumerateEdges().
///
/// This encoding is consistent across triangulation or quadrangulation
/// of the base mesh faces as well as for non-manifold faces on refined
/// subdivision surface meshes.
///
/// There can be multiple edge indices associated with each pair of
/// topological vertices in the mesh, e.g. one for each face incident
/// on the edge.
///
/// For example, here is a typical edge index assignment for a mesh
/// with 2 quad faces and 6 vertices:
///
///   faceVertexCounts: [4, 4]
///   faceVertexIndices: [0, 1, 4, 3, 1, 2, 5, 4]
///
///   edgeId:(edgeVertex[0], edgeVertex[1])
///
///         2:(3,4)          6:(4,5)
///   3----------------4----------------5
///   |                |                |
///   |     Face 0     |     Face 1     |
///   |                |                |
///   |3:(0,3)  1:(1,4)|7:(1,4)  5:(2,5)|
///   |                |                |
///   |                |                |
///   |                |                |
///   0----------------1----------------2
///         0:(0,1)          4:(1,2)
///
/// Notice that with this assignment, there are eight edge indices even
/// though the mesh has seven topological edges. The mesh edge between
/// vertex 1 and vertex 4 is associated with two edgeIds (1 and 7),
/// one for each incident face.
///
/// This kind of edge index assignment can be implemented efficiently
/// on the GPU since it falls out automatically from the primitive
/// drawing order and requires minimal additional GPU data.
///
///
class HdMeshEdgeIndexTable
{
public:
    HD_API
    explicit HdMeshEdgeIndexTable(HdMeshTopology const * topology);
    HD_API
    ~HdMeshEdgeIndexTable();

    HD_API
    bool GetVerticesForEdgeIndex(int edgeId, GfVec2i * edgeVerticesOut) const;

    HD_API
    bool GetVerticesForEdgeIndices(
        std::vector<int> const & edgeIndices,
        std::vector<GfVec2i> * edgeVerticesOut) const;

    HD_API
    bool GetEdgeIndices(GfVec2i const & edgeVertices,
                        std::vector<int> * edgeIndicesOut) const;

    /// Returns the edge indices for all faces in faceIndices.
    HD_API
    VtIntArray CollectFaceEdgeIndices(VtIntArray const &faceIndices) const;

private:
    struct _Edge{
        _Edge(GfVec2i const & verts_ = GfVec2i(-1), int index_ = -1)
            : verts(verts_)
            , index(index_)
        {
            // Simplify sorting and searching by keeping the vertices ordered.
            if (verts[0] > verts[1]) {
                std::swap(verts[0], verts[1]);
            }
        }
        GfVec2i verts;
        int index;

    };

    struct _CompareEdgeVertices {
        bool operator() (_Edge const &lhs, _Edge const & rhs) const {
            return (lhs.verts[0] < rhs.verts[0] ||
                    (lhs.verts[0] == rhs.verts[0] &&
                     lhs.verts[1] < rhs.verts[1]));
        }
    };

    struct _EdgeVerticesHash {
        // Use a custom hash so that edges (a,b) and (b,a) are equivalent
        inline size_t operator()(GfVec2i const& v) const {
            // Triangular numbers for 2-d hash.
            int theMin = v[0], theMax = v[1];
            if (theMin > theMax) {
                std::swap(theMin, theMax);
            }
            size_t x = theMin;
            size_t y = x + theMax;
            return x + (y * (y + 1)) / 2;
        }
    };

    HdMeshTopology const *_topology;
    std::vector<int> _firstEdgeIndexForFaces;

    std::vector<GfVec2i> _edgeVertices;
    std::vector<_Edge> _edgesByIndex;
};

/// \class HdMeshTriQuadBuilder
///
/// Helper class for emitting a buffer of quad indices, optionally
/// splitting each quad into two triangles.
///
class HdMeshTriQuadBuilder
{
public:
    static int const NumIndicesPerQuad = 4;
    static int const NumIndicesPerTriQuad = 6;

    HdMeshTriQuadBuilder(int * indicesBuffer, bool triangulate)
        : _outputPtr(indicesBuffer)
        , _triangulate(triangulate)
        { }

    void EmitQuadFace(GfVec4i const & quadIndices) {
        if (_triangulate) {
            *_outputPtr++ = quadIndices[0];
            *_outputPtr++ = quadIndices[1];
            *_outputPtr++ = quadIndices[2];
            *_outputPtr++ = quadIndices[2];
            *_outputPtr++ = quadIndices[3];
            *_outputPtr++ = quadIndices[0];
        } else {
            *_outputPtr++ = quadIndices[0];
            *_outputPtr++ = quadIndices[1];
            *_outputPtr++ = quadIndices[2];
            *_outputPtr++ = quadIndices[3];
        }
    }

private:
    int * _outputPtr;
    bool const _triangulate;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_MESH_UTIL_H
