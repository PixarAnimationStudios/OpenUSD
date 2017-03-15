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
#ifndef HD_MESH_UTIL_H
#define HD_MESH_UTIL_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/meshTopology.h"

#include "pxr/usd/sdf/path.h"
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
    void ComputeTriangleIndices(VtVec3iArray *indices,
                                VtIntArray *primitiveParams);

    /// Return a triangulation of a face-varying primvar. source is
    /// a buffer of size numElements and type corresponding to glDataType
    /// (e.g. GL_FLOAT_VEC3); the result is a VtArray<T> of the
    /// correct type written to the variable "triangulated".
    /// This function returns false if it can't resolve glDataType.
    bool ComputeTriangulatedFaceVaryingPrimvar(void const* source,
                                               int numElements,
                                               int glDataType,
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
    static int ComputeNumQuads(VtIntArray const &numVerts,
                               VtIntArray const &holeIndices,
                               bool *invalidFaceFound=NULL);


    /// Generate a quadInfo struct for the input topology.
    void ComputeQuadInfo(HdQuadInfo* quadInfo);

    /// Return quadrangulated indices of the input topology. indices and
    /// primitiveParams are output parameters.
    void ComputeQuadIndices(VtVec4iArray *indices,
                            VtVec2iArray *primitiveParams);

    /// Return a quadrangulation of a per-vertex primvar. source is
    /// a buffer of size numElements and type corresponding to glDataType
    /// (e.g. GL_FLOAT_VEC3); the result is a VtArray<T> of the
    /// correct type written to the variable "quadrangulated".
    /// This function returns false if it can't resolve glDataType.
    bool ComputeQuadrangulatedPrimvar(HdQuadInfo const* qi,
                                      void const* source,
                                      int numElements,
                                      int glDataType,
                                      VtValue *quadrangulated);

    /// Return a quadrangulation of a face-varying primvar.
    /// source is a buffer of size numElements and type corresponding
    /// to glDataType (e.g. GL_FLOAT_VEC3); the result is a VtArray<T> of the
    /// correct type written to the variable "quadrangulated".
    /// This function returns false if it can't resolve glDataType.
    bool ComputeQuadrangulatedFaceVaryingPrimvar(void const* source,
                                                 int numElements,
                                                 int glDataType,
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

private:
    HdMeshTopology const* _topology;
    SdfPath const _id;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_MESH_UTIL_H
