//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file pxOsd/meshTopology.h
///

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/meshTopologyValidation.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/arch/hash.h"
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

PxOsdMeshTopology::PxOsdMeshTopology(TfToken const& scheme,
                                     TfToken const& orientation,
                                     VtIntArray const& faceVertexCounts,
                                     VtIntArray const& faceVertexIndices) :
    _scheme(scheme),
    _orientation(orientation),
    _faceVertexCounts(faceVertexCounts),
    _faceVertexIndices(faceVertexIndices),
    _holeIndices(),
    _subdivTags() { }

PxOsdMeshTopology::PxOsdMeshTopology(TfToken const& scheme,
                                     TfToken const& orientation,
                                     VtIntArray const& faceVertexCounts,
                                     VtIntArray const& faceVertexIndices,
                                     VtIntArray const& holeIndices) :
    _scheme(scheme),
    _orientation(orientation),
    _faceVertexCounts(faceVertexCounts),
    _faceVertexIndices(faceVertexIndices),
    _holeIndices(holeIndices),
    _subdivTags() { }

PxOsdMeshTopology::PxOsdMeshTopology(TfToken const& scheme,
                                     TfToken const& orientation,
                                     VtIntArray const& faceVertexCounts,
                                     VtIntArray const& faceVertexIndices,
                                     VtIntArray const& holeIndices,
                                     PxOsdSubdivTags const& subdivTags) :
    _scheme(scheme),
    _orientation(orientation),
    _faceVertexCounts(faceVertexCounts),
    _faceVertexIndices(faceVertexIndices),
    _holeIndices(holeIndices),
    _subdivTags(subdivTags) { }

PxOsdMeshTopology::PxOsdMeshTopology(TfToken const& scheme,
                                     TfToken const& orientation,
                                     VtIntArray const& faceVertexCounts,
                                     VtIntArray const& faceVertexIndices,
                                     PxOsdSubdivTags const& subdivTags) :
    _scheme(scheme),
    _orientation(orientation),
    _faceVertexCounts(faceVertexCounts),
    _faceVertexIndices(faceVertexIndices),
    _holeIndices(),
    _subdivTags(subdivTags)
{ }

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

PxOsdMeshTopologyValidation
PxOsdMeshTopology::Validate() const
{
    TRACE_FUNCTION();
    if (_validated.value){
        return PxOsdMeshTopologyValidation();
    }

    PxOsdMeshTopologyValidation validation(*this);
    _validated.value.store(bool(validation));
    return validation;
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

