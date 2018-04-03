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

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/debug.h"

PXR_NAMESPACE_OPEN_SCOPE


PxOsdMeshTopology::PxOsdMeshTopology() :
    _scheme(PxOsdOpenSubdivTokens->bilinear),
    _orientation(PxOsdOpenSubdivTokens->rightHanded),
    _faceVertexCounts(),
    _faceVertexIndices(),
    _holeIndices(),
    _subdivTags() { }

PxOsdMeshTopology::PxOsdMeshTopology(PxOsdMeshTopology const & src) :
    _scheme(src._scheme),
    _orientation(src._orientation),
    _faceVertexCounts(src._faceVertexCounts),
    _faceVertexIndices(src._faceVertexIndices),
    _holeIndices(src._holeIndices),
    _subdivTags(src._subdivTags) { }

PxOsdMeshTopology::PxOsdMeshTopology(TfToken scheme,
                                     TfToken orientation,
                                     VtIntArray faceVertexCounts,
                                     VtIntArray faceVertexIndices) :
    _scheme(scheme),
    _orientation(orientation),
    _faceVertexCounts(faceVertexCounts),
    _faceVertexIndices(faceVertexIndices),
    _holeIndices(),
    _subdivTags() { }

PxOsdMeshTopology::PxOsdMeshTopology(TfToken scheme,
                                     TfToken orientation,
                                     VtIntArray faceVertexCounts,
                                     VtIntArray faceVertexIndices,
                                     VtIntArray holeIndices) :
    _scheme(scheme),
    _orientation(orientation),
    _faceVertexCounts(faceVertexCounts),
    _faceVertexIndices(faceVertexIndices),
    _holeIndices(),
    _subdivTags()
{
    SetHoleIndices(holeIndices);
}


PxOsdMeshTopology::~PxOsdMeshTopology() { }


PxOsdMeshTopology::ID
PxOsdMeshTopology::ComputeHash() const {

    TRACE_FUNCTION();

    ID hash = _subdivTags.ComputeHash();
    hash = ArchHash64((const char*)&_scheme, sizeof(TfToken), hash);
    hash = ArchHash64((const char*)&_orientation, sizeof(TfToken), hash);
    hash = ArchHash64((const char*)_faceVertexCounts.cdata(),
                      _faceVertexCounts.size() * sizeof(int), hash);
    hash = ArchHash64((const char*)_faceVertexIndices.cdata(),
                      _faceVertexIndices.size() * sizeof(int), hash);
    hash = ArchHash64((const char*)_holeIndices.cdata(),
                      _holeIndices.size() * sizeof(int), hash);
    return hash;
}

bool
PxOsdMeshTopology::operator==(PxOsdMeshTopology const &other) const {

    TRACE_FUNCTION();

    return (_scheme == other._scheme                        && 
            _orientation == other._orientation              && 
            _faceVertexCounts == other._faceVertexCounts    && 
            _faceVertexIndices == other._faceVertexIndices  && 
            _subdivTags == other._subdivTags                && 
            _holeIndices == other._holeIndices);
}

void
PxOsdMeshTopology::SetHoleIndices(VtIntArray const &holeIndices)
{
    if (TfDebug::IsEnabled(0)) {
        // make sure faceIndices is given in ascending order.
        const size_t nFaceIndices = holeIndices.size();
        for (size_t i = 1; i < nFaceIndices; ++i) {
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
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

