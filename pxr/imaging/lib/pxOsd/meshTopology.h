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
#ifndef PXOSD_MESH_TOPOLOGY_H
#define PXOSD_MESH_TOPOLOGY_H

/// \file pxOsd/meshTopology.h

#include "pxr/pxr.h"
#include "pxr/imaging/pxOsd/api.h"
#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class PxOsdMeshTopology> PxOsdMeshTopologySharedPtr;

/// \class PxOsdMeshTopology
///
/// Topology data for meshes.
///
class PxOsdMeshTopology {

public:

    typedef uint64_t ID;

    PXOSD_API
    PxOsdMeshTopology();

    PXOSD_API
    ~PxOsdMeshTopology();

    PXOSD_API
    PxOsdMeshTopology(const PxOsdMeshTopology &);

    PXOSD_API
    PxOsdMeshTopology(
        TfToken scheme,
        TfToken orientation,
        VtIntArray faceVertexCounts,
        VtIntArray faceVertexIndices);

    PXOSD_API
    PxOsdMeshTopology(
        TfToken scheme,
        TfToken orientation,
        VtIntArray faceVertexCounts,
        VtIntArray faceVertexIndices,
        VtIntArray holeIndices);

public:

    /// Returns the subdivision scheme
    TfToken const GetScheme() const {
        return _scheme;
    }

    /// Sets the subdivision scheme to be used during refinement.
    /// Valid values include: catmullClark, loop, bilinear.
    ///
    /// Note that the token "catmark" is also supported for backward
    /// compatibility, but has been deprecated.
    void SetScheme(TfToken const& scheme) {
        _scheme = scheme;
    }

    /// Returns face vertex counts.
    VtIntArray const &GetFaceVertexCounts() const {
        return _faceVertexCounts;
    }

    /// Returns face vertex indics.
    VtIntArray const &GetFaceVertexIndices() const {
        return _faceVertexIndices;
    }

    /// Returns orientation.
    TfToken const &GetOrientation() const {
        return _orientation;
    }


    ///
    /// \name Holes
    /// @{

    /// Sets hole face indices for the control mesh.
    PXOSD_API
    void SetHoleIndices(VtIntArray const &holeFaceIndices) {
        _holeIndices = holeFaceIndices;
    }

    /// Returns the hole face indices.
    VtIntArray const &GetHoleIndices() const {
        return _holeIndices;
    }

    /// @}

    ///
    /// \name Tags
    /// @{

    /// Sets subdivision tags
    void SetSubdivTags(PxOsdSubdivTags const &subdivTags) {
        _subdivTags = subdivTags;
    }

    /// Returns subdivision tags
    PxOsdSubdivTags const & GetSubdivTags() const {
        return _subdivTags;
    }

    /// Returns subdivision tags (non-const)
    PxOsdSubdivTags & GetSubdivTags() {
        return _subdivTags;
    }

    /// @}

public:

    /// Returns the hash value of this topology to be used for instancing.
    PXOSD_API
    ID ComputeHash() const;

    /// Equality check between two mesh topologies.
    PXOSD_API
    bool operator==(PxOsdMeshTopology const &other) const;

private:

    // note: if you're going to add more members, make sure
    // ComputeHash will be updated too.

    TfToken _scheme,
            _orientation;

    VtIntArray _faceVertexCounts;
    VtIntArray _faceVertexIndices;
    VtIntArray _holeIndices;

    PxOsdSubdivTags _subdivTags;
};

PXOSD_API
std::ostream& operator << (std::ostream &out, PxOsdMeshTopology const &);
PXOSD_API
bool operator!=(const PxOsdMeshTopology& lhs, const PxOsdMeshTopology& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXOSD_MESH_TOPOLOGY_H
