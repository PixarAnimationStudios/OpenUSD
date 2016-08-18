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
///
/// \file pxOsd/meshTopology.h
///

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/tracelite/trace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/debug.h"

PxOsdMeshTopology::PxOsdMeshTopology() :
    _scheme(PxOsdOpenSubdivTokens->bilinear),
    _orientation(PxOsdOpenSubdivTokens->rightHanded),
    _faceVertexCounts(),
    _faceVertexIndices(),
    _holeIndices() { }

PxOsdMeshTopology::PxOsdMeshTopology(PxOsdMeshTopology const & src) :
    _scheme(src._scheme),
    _orientation(src._orientation),
    _faceVertexCounts(src._faceVertexCounts),
    _faceVertexIndices(src._faceVertexIndices),
    _holeIndices(src._holeIndices) { }

PxOsdMeshTopology::PxOsdMeshTopology(TfToken scheme,
                                     TfToken orientation,
                                     VtIntArray faceVertexCounts,
                                     VtIntArray faceVertexIndices) :
    _scheme(scheme),
    _orientation(orientation),
    _faceVertexCounts(faceVertexCounts),
    _faceVertexIndices(faceVertexIndices) { }

PxOsdMeshTopology::PxOsdMeshTopology(TfToken scheme,
                                     TfToken orientation,
                                     VtIntArray faceVertexCounts,
                                     VtIntArray faceVertexIndices,
                                     VtIntArray holeIndices) :
    _scheme(scheme),
    _orientation(orientation),
    _faceVertexCounts(faceVertexCounts),
    _faceVertexIndices(faceVertexIndices) {

    SetHoleIndices(holeIndices);
}


PxOsdMeshTopology::~PxOsdMeshTopology() { }


PxOsdMeshTopology::ID
PxOsdMeshTopology::ComputeHash() const {

    TRACE_FUNCTION();

    uint32_t hash = static_cast<uint32_t>(_subdivTags.ComputeHash());
    hash = ArchHash((const char*)&_scheme, sizeof(TfToken), hash);
    hash = ArchHash((const char*)&_orientation, sizeof(TfToken), hash);
    hash = ArchHash((const char*)_faceVertexCounts.cdata(),
                    _faceVertexCounts.size() * sizeof(int), hash);
    hash = ArchHash((const char*)_faceVertexIndices.cdata(),
                    _faceVertexIndices.size() * sizeof(int), hash);
    hash = ArchHash((const char*)_holeIndices.cdata(),
                    _holeIndices.size() * sizeof(int), hash);
    // promote to size_t
    return (ID)hash;
}

bool
PxOsdMeshTopology::operator==(PxOsdMeshTopology const &other) const {

    TRACE_FUNCTION();

    return (_scheme == other._scheme and
            _orientation == other._orientation and
            _faceVertexCounts == other._faceVertexCounts and
            _faceVertexIndices == other._faceVertexIndices and
            _subdivTags == other._subdivTags and
            _holeIndices == other._holeIndices);
}

void
PxOsdMeshTopology::SetHoleIndices(VtIntArray const &holeIndices)
{
    if (TfDebug::IsEnabled(0)) {
        // make sure faceIndices is given in ascending order.
        int nFaceIndices = static_cast<int>(holeIndices.size());
        for (int i = 1; i < nFaceIndices; ++i) {
            if (holeIndices[i] <= holeIndices[i-1]) {
                // XXX: would be better to print the prim name.
                TF_WARN("hole face indices are not in ascending order.");
                return;
            }
        }
    }
    _holeIndices = holeIndices;
}

std::ostream&
operator << (std::ostream &out, PxOsdMeshTopology const &topo)
{
    out << "(" << topo.GetOrientation().GetString() << ", "
        << topo.GetScheme().GetString() << ", ("
        << topo.GetFaceVertexCounts() << "), ("
        << topo.GetFaceVertexIndices() << "), ("
        << topo.GetHoleIndices() << "))";
    return out;
}

bool operator!=(const PxOsdMeshTopology& lhs, const PxOsdMeshTopology& rhs)
{
    return not (lhs == rhs);
}
