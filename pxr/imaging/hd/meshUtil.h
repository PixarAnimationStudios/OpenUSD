//
// Copyright 2017 Pixar
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
#ifndef PXR_IMAGING_HD_MESH_UTIL_H
#define PXR_IMAGING_HD_MESH_UTIL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include <unordered_map>
#include <utility> // std::{min,max}, std::pair

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
    // Triangulation

    // In order to access per-face signals (face color, face selection etc)
    // we need a mapping from primitiveID to authored face index domain.
    // This is stored in primitiveParams, and computed along with indices.
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
                                VtVec3iArray *trianglesEdgeIndices = nullptr);

    /// Return a triangulation of a face-varying primvar. source is
    /// a buffer of size numElements and type corresponding to dataType
    /// (e.g. HdTypeFloatVec3); the result is a VtArray<T> of the
    /// correct type written to the variable "triangulated".
    /// This function returns false if it can't resolve dataType.
    HD_API
    bool ComputeTriangulatedFaceVaryingPrimvar(void const* source,
                                               int numElements,
                                               HdType dataType,
                                               VtValue *triangulated);

    // --------------------------------------------------------------------
    // Quadrangulation

    // In order to access per-face signals (face color, face selection etc)
    // we need a mapping from primitiveID to authored face index domain.
    // This is stored in primitiveParams, and computed along with indices.
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

    /// Return the number of quadrangulated quads.
    /// If degenerated face is found, sets invalidFaceFound as true.
    HD_API
    static int ComputeNumQuads(VtIntArray const &numVerts,
                               VtIntArray const &holeIndices,
                               bool *invalidFaceFound=NULL);


    /// Generate a quadInfo struct for the input topology.
    HD_API
    void ComputeQuadInfo(HdQuadInfo* quadInfo);

    /// Return quadrangulated indices of the input topology. indices and
    /// primitiveParams are output parameters.
    HD_API
    void ComputeQuadIndices(VtVec4iArray *indices,
                            VtVec2iArray *primitiveParams,
                            VtVec4iArray *quadsEdgeIndices = nullptr);

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
                                      VtValue *quadrangulated);

    /// Return a quadrangulation of a face-varying primvar.
    /// source is a buffer of size numElements and type corresponding
    /// to dataType (e.g. HdTypeFloatVec3); the result is a VtArray<T> of the
    /// correct type written to the variable "quadrangulated".
    /// This function returns false if it can't resolve dataType.
    HD_API
    bool ComputeQuadrangulatedFaceVaryingPrimvar(void const* source,
                                                 int numElements,
                                                 HdType dataType,
                                                 VtValue *quadrangulated);

    // --------------------------------------------------------------------
    // Primitive param bit encoding

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

    // --------------------------------------------------------------------
    // Authored edge id computation
    struct EdgeHash {
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

    struct EdgeEquality {
        inline bool operator() (GfVec2i const& v1, GfVec2i const& v2) const {
            // The bitwise operators here give a small speedup in the generated
            // code since we avoid the conditional jumps required by
            // short-circuiting logical ops.
            return
                ((v1[0] == v2[0]) & (v1[1] == v2[1])) |
                ((v1[0] == v2[1]) & (v1[1] == v2[0]));
        }
    };

    using EdgeMap = std::unordered_map<GfVec2i, int, EdgeHash, EdgeEquality>;
    using ReverseEdgeMap = std::unordered_map<int, GfVec2i>;
    
    // Enumerates all the edges of the authored mesh topology, and returns a map
    // of (vertex indices pair, edge id).
    // If skipHoles is true, unshared edges of hole faces aren't enumerated.
    HD_API
    static EdgeMap ComputeAuthoredEdgeMap(HdMeshTopology const* topology,
                                          bool skipHoles = false);

    // Given the map from (vertex indices pair, edge id) computed by
    // ComputeAuthoredEdgeMap, returns the reverse map (edge id, vertex indices
    // pair).
    HD_API
    static ReverseEdgeMap ComputeReverseEdgeMap(const EdgeMap &edgeMap);

    // Translates an authored edge id to its vertex indices
    // Returns a pair, with first indicating success of the look up, and
    // second being the vertex indices for the edge.
    HD_API
    static std::pair<bool, GfVec2i>
    GetVertexIndicesForEdge(const ReverseEdgeMap &rEdgeMap, int authoredEdgeId);

    // Translates an edge to its authored edge id
    // Returns a pair, with first indicating success of the look up, and
    // second being the authored edge id
    HD_API
    static std::pair<bool, int>
    GetAuthoredEdgeID(HdMeshTopology const* topology,
                      GfVec2i edge);
private:
    HdMeshTopology const* _topology;
    SdfPath const _id;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_MESH_UTIL_H
