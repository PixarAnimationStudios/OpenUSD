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
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/geomSubset.h"
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
    HD_API
    HdMeshTopology();
    HD_API
    HdMeshTopology(const HdMeshTopology &, int refineLevel=0);
    HD_API
    HdMeshTopology(const PxOsdMeshTopology &, int refineLevel=0);
    HD_API
    HdMeshTopology(const TfToken    &scheme,
                   const TfToken    &orientation,
                   const VtIntArray &faceVertexCounts,
                   const VtIntArray &faceVertexIndices,
                   int refineLevel = 0);
    HD_API
    HdMeshTopology(const TfToken &scheme,
                   const TfToken &orientation,
                   const VtIntArray &faceVertexCounts,
                   const VtIntArray &faceVertexIndices,
                   const VtIntArray &holeIndices,
                   int refineLevel = 0);
    HD_API
    virtual ~HdMeshTopology();

    HD_API
    HdMeshTopology &operator =(const HdMeshTopology &copy);

    /// Returns whether adaptive subdivision is enabled or not.
    HD_API
    static bool IsEnabledAdaptive();

    PxOsdMeshTopology const & GetPxOsdMeshTopology() const {
        return _topology;
    }

    /// Returns the num faces
    HD_API
    int GetNumFaces() const;

    /// Returns the num facevarying primvars
    HD_API
    int GetNumFaceVaryings() const;

    /// Returns the num points of the topology vert indices array
    HD_API
    int GetNumPoints() const;

    /// Returns the num points by looking vert indices array
    HD_API
    static int ComputeNumPoints(VtIntArray const &verts);

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
    HD_API
    virtual ID ComputeHash() const;

    /// Equality check between two mesh topologies.
    HD_API
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
    PxOsdSubdivTags const&GetSubdivTags() const {
        return _topology.GetSubdivTags();
    }

    /// @}

    ///
    /// \name Geometry subsets
    /// @{

    /// Sets geometry subsets
    HD_API
    void SetGeomSubsets(HdGeomSubsets const &geomSubsets) {
        _geomSubsets = geomSubsets;
    }

    /// Returns geometry subsets
    HD_API
    HdGeomSubsets const &GetGeomSubsets() const {
        return _geomSubsets;
    }

    /// @}

protected:
    PxOsdMeshTopology _topology;
    HdGeomSubsets _geomSubsets;
    int _refineLevel;
    int _numPoints;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_MESH_TOPOLOGY_H
