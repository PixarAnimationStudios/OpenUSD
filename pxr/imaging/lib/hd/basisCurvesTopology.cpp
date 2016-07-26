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
#include "pxr/imaging/hd/basisCurvesTopology.h"
#include "pxr/imaging/hd/basisCurvesComputations.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"


HdBasisCurvesTopology::HdBasisCurvesTopology()
    : _curveType(HdTokens->linear)
    , _curveBasis(HdTokens->bezier)
    , _curveWrap(HdTokens->nonperiodic)
    , _curveVertexCounts()
    , _curveIndices()
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->basisCurvesTopology);
}

HdBasisCurvesTopology::HdBasisCurvesTopology(const HdBasisCurvesTopology& src)
    : _curveType(src._curveType)
    , _curveBasis(src._curveBasis)
    , _curveWrap(src._curveWrap)
    , _curveVertexCounts(src._curveVertexCounts)
    , _curveIndices(src._curveIndices)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->basisCurvesTopology);
}

HdBasisCurvesTopology::HdBasisCurvesTopology(TfToken curveType,
                                             TfToken curveBasis,
                                             TfToken curveWrap,
                                             const VtIntArray &curveVertexCounts,
                                             const VtIntArray &curveIndices)
    : _curveType(curveType)
    , _curveBasis(curveBasis)
    , _curveWrap(curveWrap)
    , _curveVertexCounts(curveVertexCounts)
    , _curveIndices(curveIndices)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->basisCurvesTopology);
}

HdBasisCurvesTopology::~HdBasisCurvesTopology()
{
    HD_PERF_COUNTER_DECR(HdPerfTokens->basisCurvesTopology);
}

bool
HdBasisCurvesTopology::operator==(HdBasisCurvesTopology const &other) const
{
    HD_TRACE_FUNCTION();

    // no need to compare _adajency and _quadInfo
    return (_curveType == other._curveType and
            _curveBasis == other._curveBasis and
            _curveWrap == other._curveWrap and
            _curveVertexCounts == other._curveVertexCounts and
            _curveIndices == other._curveIndices);
}

bool
HdBasisCurvesTopology::operator!=(HdBasisCurvesTopology const &other) const
{
    return !(*this == other);
}

HdTopology::ID
HdBasisCurvesTopology::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    uint32_t hash = 0;
    hash = ArchHash((const char*)&_curveBasis, sizeof(TfToken), hash);
    hash = ArchHash((const char*)&_curveType, sizeof(TfToken), hash);
    hash = ArchHash((const char*)&_curveWrap, sizeof(TfToken), hash);
    hash = ArchHash((const char*)_curveVertexCounts.cdata(),
                    _curveVertexCounts.size() * sizeof(int), hash);
    hash = ArchHash((const char*)_curveIndices.cdata(),
                    _curveIndices.size() * sizeof(int), hash);
    // promote to size_t
    return (ID)hash;
}

std::ostream&
operator << (std::ostream &out, HdBasisCurvesTopology const &topo)
{
    out << "(" << topo.GetCurveBasis().GetString() << ", " <<
        topo.GetCurveType().GetString() << ", " <<
        topo.GetCurveWrap().GetString() << ", (" <<
        topo.GetCurveVertexCounts() << "), (" <<
        topo.GetCurveIndices() << "))";
    return out;
}

HdBufferSourceSharedPtr
HdBasisCurvesTopology::GetIndexBuilderComputation(bool supportSmoothCurves)
{
    return HdBufferSourceSharedPtr(
        new Hd_BasisCurvesIndexBuilderComputation(this, supportSmoothCurves));
}

size_t
HdBasisCurvesTopology::CalculateNeededNumberOfControlPoints() const
{
    size_t numVerts= 0;

    // Make absolutely sure the iterator is constant 
    // (so we don't detach the array while multi-threaded)
    for (VtIntArray::const_iterator itCounts = _curveVertexCounts.cbegin();
            itCounts != _curveVertexCounts.cend(); ++itCounts) {
        numVerts += *itCounts;
    }

    return numVerts;
}

size_t
HdBasisCurvesTopology::CalculateNeededNumberOfVaryingControlPoints() const
{
    size_t numVerts= 0;
    int numSegs = 0, vStep = 0;
    bool wrap = GetCurveWrap() == HdTokens->periodic;
    
    if(GetCurveBasis() == HdTokens->bezier) {
        vStep = 3;
    }
    else {
        vStep = 1;
    }

    // Make absolutely sure the iterator is constant 
    // (so we don't detach the array while multi-threaded)
    for (VtIntArray::const_iterator itCounts = _curveVertexCounts.cbegin();
            itCounts != _curveVertexCounts.cend(); ++itCounts) {
        
        // Handling for the case of potentially incorrect vertex counts 
        if (*itCounts < 1) {
            continue;
        }

        // The number of verts is different if we have periodic vs non-periodic
        // curves, check basisCurvesComputations.cpp line 207 for a diagram.
        if (wrap) {
            numSegs = *itCounts / vStep;
        } else {
            numSegs = ((*itCounts - 4) / vStep) + 1;
        }
        numVerts += numSegs + 1;
    }

    return numVerts;
}
