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

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/topology.h"

#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdMeshTopology> HdMeshTopologySharedPtr;

/// \class HdMeshTopology
///
/// Topology data for meshes.
///
/// HdMeshTopology holds the raw input topology data for a mesh and is capable
/// of computing derivative topological data (such as indices or subdivision
/// stencil tables and patch tables).
///
class HdMeshTopology : public HdTopology {
public:
    HdMeshTopology();
    HdMeshTopology(const HdMeshTopology &, int refineLevel=0);
    HdMeshTopology(const PxOsdMeshTopology &, int refineLevel=0);
    HdMeshTopology(const TfToken    &scheme,
                   const TfToken    &orientation,
                   const VtIntArray &faceVertexCounts,
                   const VtIntArray &faceVertexIndices,
                   int refineLevel = 0);
    HdMeshTopology(const TfToken &scheme,
                   const TfToken &orientation,
                   const VtIntArray &faceVertexCounts,
                   const VtIntArray &faceVertexIndices,
                   const VtIntArray &holeIndices,
                   int refineLevel = 0);
    virtual ~HdMeshTopology();

    HdMeshTopology &operator =(const HdMeshTopology &copy);


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


protected:
    PxOsdMeshTopology _topology;
    int _refineLevel;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_MESH_TOPOLOGY_H
