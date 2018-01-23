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
#ifndef HD_BASIS_CURVES_TOPOLOGY_H
#define HD_BASIS_CURVES_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/topology.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::shared_ptr<class HdBasisCurvesTopology> HdBasisCurvesTopologySharedPtr;


/// \class HdBasisCurvesTopology
///
/// Topology data for basisCurves.
///
/// HdBasisCurvesTopology holds the raw input topology data for basisCurves
///
/// The Type, Basis and Wrap mode combined describe the curve and it's
/// segments.
///
/// If Type == linear, the curve is a bunch of line segments and basis is
/// ignored.
///
/// The wrap mode defines how the curve segments are specified:
///
///   If Wrap == segmented, this is equivalent to GL_LINES and curve vertex
///      counts is 2 * number of segments (multiple entries in curve vertex
///      array is optional).
///
///   If Wrap == nonperiodic, this is equivalent to GL_LINE_STRIP and curve
///     counts is an array where each entry is the number of vertices in that
///     line segment.  The first and last vertex in the segment are not joined.
///
///   If Wrap == periodic, this is equivalent to GL_LINE_LOOP  and curve counts
///     is an array where each entry is the number of vertices in that line
///     segment. An additional line is place between the first and last vertex
///     in each segment.
///
///  If Type == cubic, the type of curve is specified by basis:
///     The Basis can be bezier, bspline or catmullRom.
///
///     Wrap can be either periodic or nonperiodic (segmented is unsupported).
///
///  For each type of line, the generated vertex indices can pass through an
///  optional index buffer to map the generated indices to actual indices in
///  the vertex buffer.
///
class HdBasisCurvesTopology : public HdTopology {
public:

    HD_API
    HdBasisCurvesTopology();
    HD_API
    HdBasisCurvesTopology(const HdBasisCurvesTopology &src);

    HD_API
    HdBasisCurvesTopology(const TfToken &curveType,
                          const TfToken &curveBasis,
                          const TfToken &curveWrap,
                          const VtIntArray &curveVertexCounts,
                          const VtIntArray &curveIndices);
    HD_API
    virtual ~HdBasisCurvesTopology();


    /// Returns segment vertex counts.
    VtIntArray const &GetCurveVertexCounts() const {
        return _curveVertexCounts;
    }

    /// Returns indicies.
    VtIntArray const &GetCurveIndices() const {
        return _curveIndices;
    }

    /// See class documentation for valid combination of values
    TfToken GetCurveType() const { return _curveType; }
    TfToken GetCurveBasis() const { return _curveBasis; }
    TfToken GetCurveWrap() const { return _curveWrap; }

    /// Does the topology use an index buffer
    bool HasIndices() const { return !_curveIndices.empty(); }

    /// Returns the hash value of this topology to be used for instancing.
    HD_API
    virtual ID ComputeHash() const;

    /// Equality check between two basisCurves topologies.
    HD_API
    bool operator==(HdBasisCurvesTopology const &other) const;
    HD_API
    bool operator!=(HdBasisCurvesTopology const &other) const;

    /// Figure out how many vertices / control points this topology references
    HD_API
    size_t CalculateNeededNumberOfControlPoints() const;

    /// Figure out how many control points with varying data this topology needs
    HD_API
    size_t CalculateNeededNumberOfVaryingControlPoints() const;

private:
    TfToken _curveType;
    TfToken _curveBasis;
    TfToken _curveWrap;
    VtIntArray _curveVertexCounts;
    VtIntArray _curveIndices;
};

HD_API
std::ostream& operator << (std::ostream &out, HdBasisCurvesTopology const &topo);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_BASIS_CURVES_TOPOLOGY_H
