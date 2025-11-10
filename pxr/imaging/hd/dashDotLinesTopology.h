//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DASH_DOT_LINES_TOPOLOGY_H
#define PXR_IMAGING_HD_DASH_DOT_LINES_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/topology.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdDashDotLinesTopology
///
/// Topology data for dashDotLines.
///
/// HdDashDotLinesTopology holds the raw input topology data for dashDotLines
///
///  The generated vertex indices can pass through an
///  optional index buffer to map the generated indices to actual indices in
///  the vertex buffer.
///
class HdDashDotLinesTopology : public HdTopology {
public:
    HD_API
    HdDashDotLinesTopology();
    HD_API
    HdDashDotLinesTopology(const HdDashDotLinesTopology&src);

    HD_API
    HdDashDotLinesTopology(const TfToken &shapeDetail,
                           const bool &screenSpacePattern,
                           const VtIntArray &curveVertexCounts,
                           const VtIntArray &curveIndices);
    HD_API
    virtual ~HdDashDotLinesTopology();

    ///
    /// \name Topological invisibility
    /// @{
    ///
    HD_API
    void SetInvisiblePoints(VtIntArray const &invisiblePoints) {
        _invisiblePoints = invisiblePoints;
    }

    HD_API
    VtIntArray const & GetInvisiblePoints() const {
        return _invisiblePoints;
    }

    HD_API
    void SetInvisibleCurves(VtIntArray const &invisibleCurves) {
        _invisibleCurves = invisibleCurves;
    }

    HD_API
    VtIntArray const & GetInvisibleCurves() const {
        return _invisibleCurves;
    }
    /// @}

    /// Returns segment vertex counts.
    HD_API
    VtIntArray const &GetCurveVertexCounts() const {
        return _curveVertexCounts;
    }

    /// Returns indices.
    HD_API
    VtIntArray const &GetCurveIndices() const {
        return _curveIndices;
    }

    /// Returns the number of lines
    HD_API
    size_t GetNumCurves() const {
        return _curveVertexCounts.size();
    }

    /// Returns the number of points implied by vertex counts and indices
    HD_API
    size_t GetNumPoints() const {
        return _numPoints;
    }

    /// See class documentation for valid combination of values
    HD_API
    TfToken GetShapeDetail() const { return _shapeDetail; }

    /// If the line pattern is screen spaced.
    HD_API
    bool GetScreenSpacePattern() const { return _screenSpacePattern; }

    /// Does the topology use an index buffer
    HD_API
    bool HasIndices() const { return !_curveIndices.empty(); }

    /// Returns the hash value of this topology to be used for instancing.
    HD_API
    virtual ID ComputeHash() const;

    /// Equality check between two dashDotLines topologies.
    HD_API
    bool operator==(HdDashDotLinesTopology const &other) const;
    HD_API
    bool operator!=(HdDashDotLinesTopology const &other) const;

    /// Figure out how many vertices / control points this topology references
    HD_API
    size_t CalculateNeededNumberOfControlPoints() const;

    /// Figure out how many control points with varying data this topology needs
    HD_API
    size_t CalculateNeededNumberOfVaryingControlPoints() const;

    /// Whether the rendering of this lines requires information of adjacent points.
    /// If the lines style is DashDot or ScreenSpaceDashDot, this API will return true.
    HD_API
    bool NeedAdjacentPoints() const;

private:
    TfToken _shapeDetail;
    bool _screenSpacePattern;
    VtIntArray _curveVertexCounts;
    VtIntArray _curveIndices;
    VtIntArray _invisiblePoints;
    VtIntArray _invisibleCurves;
    size_t _numPoints;
};

HD_API
std::ostream& operator << (std::ostream &out, HdDashDotLinesTopology const &topo);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_DASH_DOT_LINES_TOPOLOGY_H
