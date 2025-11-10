//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/dashDotLinesTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/arch/hash.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static size_t
_ComputeNumPoints(
    VtIntArray const &curveVertexCounts,
    VtIntArray const &indices,
    bool needAdjInfo)
{
    // Make absolutely sure the iterator is constant
    // (so we don't detach the array while multi-threaded)
    if (indices.empty()) {
        if (needAdjInfo)
        {
            // Calculate the count of line segments.
            size_t countOfLineSegmemt = 0;
            for (auto count : curveVertexCounts)
            {
                countOfLineSegmemt += (count - 1);
            }
            // For each line segment, we need four vertices: each line segment will
            // be converted to a quad.
            return countOfLineSegmemt * 4;
        }
        else
            return std::accumulate(
                curveVertexCounts.cbegin(), curveVertexCounts.cend(), size_t {0} );
    } else {
        return 1 + *std::max_element(indices.cbegin(), indices.cend());
    }
}

}; // anon namespace

HdDashDotLinesTopology::HdDashDotLinesTopology()
  : HdTopology()
  , _shapeDetail()
  , _screenSpacePattern()
  , _curveVertexCounts()
  , _curveIndices()
  , _invisiblePoints()
  , _invisibleCurves()
  , _numPoints()
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->dashDotLinesTopology);
}

HdDashDotLinesTopology::HdDashDotLinesTopology(const HdDashDotLinesTopology& src)
  : HdTopology(src)
  , _shapeDetail(src._shapeDetail)
  , _screenSpacePattern(src._screenSpacePattern)
  , _curveVertexCounts(src._curveVertexCounts)
  , _curveIndices(src._curveIndices)
  , _invisiblePoints(src._invisiblePoints)
  , _invisibleCurves(src._invisibleCurves)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->dashDotLinesTopology);
    _numPoints = _ComputeNumPoints(_curveVertexCounts, _curveIndices, NeedAdjacentPoints());
}

HdDashDotLinesTopology::HdDashDotLinesTopology(const TfToken& shapeDetail,
                                               const bool& screenSpacePattern,
                                               const VtIntArray &curveVertexCounts,
                                               const VtIntArray &curveIndices)
  : HdTopology()
  , _shapeDetail(shapeDetail)
  , _screenSpacePattern(screenSpacePattern)
  , _curveVertexCounts(curveVertexCounts)
  , _curveIndices(curveIndices)
  , _invisiblePoints()
  , _invisibleCurves()
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->dashDotLinesTopology);
    _numPoints = _ComputeNumPoints(_curveVertexCounts, _curveIndices, NeedAdjacentPoints());
}

HdDashDotLinesTopology::~HdDashDotLinesTopology()
{
    HD_PERF_COUNTER_DECR(HdPerfTokens->dashDotLinesTopology);
}

bool
HdDashDotLinesTopology::operator==(HdDashDotLinesTopology const &other) const
{
    HD_TRACE_FUNCTION();

    // no need to compare _adajency and _quadInfo
    return (_shapeDetail == other._shapeDetail &&
            _screenSpacePattern == other._screenSpacePattern &&
            _curveVertexCounts == other._curveVertexCounts  &&
            _curveIndices == other._curveIndices            &&
            _invisiblePoints == other._invisiblePoints      &&
            _invisibleCurves == other._invisibleCurves);
}

bool
HdDashDotLinesTopology::operator!=(HdDashDotLinesTopology const &other) const
{
    return !(*this == other);
}

HdTopology::ID
HdDashDotLinesTopology::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    HdTopology::ID hash = 0;
    hash = ArchHash64((const char*)&_shapeDetail, sizeof(TfToken), hash);
    hash = ArchHash64((const char*)&_screenSpacePattern, sizeof(bool), hash);
    hash = ArchHash64((const char*)_curveVertexCounts.cdata(),
                      _curveVertexCounts.size() * sizeof(int), hash);
    hash = ArchHash64((const char*)_curveIndices.cdata(),
                      _curveIndices.size() * sizeof(int), hash);

    // Note: We don't hash topological visibility, because it is treated as a
    // per-prim opinion, and hence, shouldn't break topology sharing.
    return hash;
}

std::ostream&
operator << (std::ostream &out, HdDashDotLinesTopology const &topo)
{
    out << "(" << topo.GetShapeDetail().GetString() << ", (" <<
        topo.GetScreenSpacePattern() << "), (" <<
        topo.GetCurveVertexCounts() << "), (" <<
        topo.GetCurveIndices() << "), (" <<
        topo.GetInvisiblePoints() << "), (" <<
        topo.GetInvisibleCurves() << "))";
    return out;
}

size_t
HdDashDotLinesTopology::CalculateNeededNumberOfControlPoints() const
{
    // This is computed on construction and accounts for authored indices.
    return _numPoints;
}

size_t
HdDashDotLinesTopology::CalculateNeededNumberOfVaryingControlPoints() const
{
    return CalculateNeededNumberOfControlPoints();
}

bool 
HdDashDotLinesTopology::NeedAdjacentPoints() const
{
    return _shapeDetail == HdTokens->allDetails;
}

PXR_NAMESPACE_CLOSE_SCOPE

