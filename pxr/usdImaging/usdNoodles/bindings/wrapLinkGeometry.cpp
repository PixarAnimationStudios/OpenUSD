//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "render/LinkGeometry.h"
#include "pxr/pxr.h"

#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

  #include "pxr/external/boost/python.hpp"
  #include "pxr/external/boost/python/stl_iterator.hpp"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_boost::python;
using namespace noodles;

namespace {

Vec2d _GfToNoodlesVec2d(const GfVec2d& gf) {
  return Vec2d(gf[0], gf[1]);
}

Range2d _GfToNoodlesRange2d(const GfRange2d& gf) {
  return Range2d(Vec2d(gf.GetMin()[0], gf.GetMin()[1]), Vec2d(gf.GetMax()[0], gf.GetMax()[1]));
}

GfRange2d _NoodlesToGfRange2d(const Range2d& r) {
  return GfRange2d(GfVec2d(r.GetMin()[0], r.GetMin()[1]), GfVec2d(r.GetMax()[0], r.GetMax()[1]));
}

std::vector<std::pair<Vec2d, Vec2d>> _ConvertLinkEndpoints(const list& pyList) {
  std::vector<std::pair<Vec2d, Vec2d>> result;

  stl_input_iterator<tuple> begin(pyList), end;
  for (auto it = begin; it != end; ++it) {
    GfVec2d gfStart = extract<GfVec2d>((*it)[0]);
    GfVec2d gfEndPt = extract<GfVec2d>((*it)[1]);
    result.emplace_back(_GfToNoodlesVec2d(gfStart), _GfToNoodlesVec2d(gfEndPt));
  }

  return result;
}

std::vector<std::pair<int, int>> _ConvertLodThresholds(const list& pyList) {
  std::vector<std::pair<int, int>> result;

  stl_input_iterator<tuple> begin(pyList), end;
  for (auto it = begin; it != end; ++it) {
    int threshold = extract<int>((*it)[0]);
    int level = extract<int>((*it)[1]);
    result.emplace_back(threshold, level);
  }

  return result;
}

int _FindLinkUnderCursor(
    const GfVec2d& cursorPos,
    const list& linkEndpoints,
    double worldTolerance) {
  return LinkGeometry::findLinkUnderCursor(
      _GfToNoodlesVec2d(cursorPos), _ConvertLinkEndpoints(linkEndpoints), worldTolerance);
}

bool _LinkIntersectsBounds(const GfVec2d& start, const GfVec2d& end, const GfRange2d& bounds) {
  return LinkGeometry::linkIntersectsBounds(
      _GfToNoodlesVec2d(start), _GfToNoodlesVec2d(end), _GfToNoodlesRange2d(bounds));
}

std::vector<int> _FindLinksInBounds(const list& linkEndpoints, const GfRange2d& bounds) {
  return LinkGeometry::findLinksInBounds(
      _ConvertLinkEndpoints(linkEndpoints), _GfToNoodlesRange2d(bounds));
}

int _CalculateLOD(double manhattanLength, double diffX, const list& lodThresholds) {
  return LinkGeometry::calculateLOD(manhattanLength, diffX, _ConvertLodThresholds(lodThresholds));
}

std::vector<float> _GenerateReferenceCurve(int numSamples) {
  return LinkGeometry::generateReferenceCurve(numSamples);
}

std::vector<GfVec2d> _SampleLinkCurve(
    const GfVec2d& start,
    const GfVec2d& end,
    int numSamples,
    bool verticalEndTangent) {
  std::vector<Vec2d> pts = LinkGeometry::sampleLinkCurve(
      _GfToNoodlesVec2d(start), _GfToNoodlesVec2d(end), numSamples, verticalEndTangent);
  std::vector<GfVec2d> result;
  result.reserve(pts.size());
  for (const Vec2d& p : pts) {
    result.emplace_back(p[0], p[1]);
  }
  return result;
}

GfRange2d _ComputeLinkCurveBounds(
    const GfVec2d& start,
    const GfVec2d& end,
    int numSamples,
    bool verticalEndTangent) {
  return _NoodlesToGfRange2d(
      LinkGeometry::computeLinkCurveBounds(
          _GfToNoodlesVec2d(start), _GfToNoodlesVec2d(end), numSamples, verticalEndTangent));
}

std::vector<GfVec2d> _SampleLinkCurveAdaptive(
    const GfVec2d& start,
    const GfVec2d& end,
    bool verticalEndTangent,
    double flatnessTolerance) {
  std::vector<Vec2d> pts = LinkGeometry::sampleLinkCurveAdaptive(
      _GfToNoodlesVec2d(start), _GfToNoodlesVec2d(end), verticalEndTangent, flatnessTolerance);
  std::vector<GfVec2d> result;
  result.reserve(pts.size());
  for (const Vec2d& p : pts) {
    result.emplace_back(p[0], p[1]);
  }
  return result;
}

} // anonymous namespace

void wrapLinkGeometry() {
  class_<LinkGeometry>("LinkGeometry", no_init)
      .def(
          "findLinkUnderCursor",
          &_FindLinkUnderCursor,
          (arg("cursorPos"), arg("linkEndpoints"), arg("worldTolerance")),
          "Find the link under cursor position.\n\n"
          "Args:\n"
          "    cursorPos (Gf.Vec2d): Cursor position in world space\n"
          "    linkEndpoints (list): List of (start, end) tuples for each link\n"
          "    worldTolerance (float): Maximum distance in world space for hit detection\n\n"
          "Returns:\n"
          "    int: Index of the link under cursor, or -1 if none found")
      .staticmethod("findLinkUnderCursor")

      .def(
          "linkIntersectsBounds",
          &_LinkIntersectsBounds,
          (arg("start"), arg("end"), arg("bounds")),
          "Check if a link intersects with rectangular bounds.\n\n"
          "Args:\n"
          "    start (Gf.Vec2d): Link start point\n"
          "    end (Gf.Vec2d): Link end point\n"
          "    bounds (Gf.Range2d): Rectangular bounds\n\n"
          "Returns:\n"
          "    bool: True if link intersects the bounds")
      .staticmethod("linkIntersectsBounds")

      .def(
          "findLinksInBounds",
          &_FindLinksInBounds,
          return_value_policy<TfPySequenceToList>(),
          (arg("linkEndpoints"), arg("bounds")),
          "Batch check multiple links against bounds.\n\n"
          "Args:\n"
          "    linkEndpoints (list): List of (start, end) tuples for each link\n"
          "    bounds (Gf.Range2d): Rectangular bounds\n\n"
          "Returns:\n"
          "    list: Indices of links that intersect the bounds")
      .staticmethod("findLinksInBounds")

      .def(
          "calculateLOD",
          &_CalculateLOD,
          (arg("manhattanLength"), arg("diffX"), arg("lodThresholds")),
          "Calculate LOD level based on Manhattan distance.\n\n"
          "Args:\n"
          "    manhattanLength (float): Manhattan distance between link endpoints\n"
          "    diffX (float): linkSampleRate / zoom\n"
          "    lodThresholds (list): List of (threshold, level) tuples\n\n"
          "Returns:\n"
          "    int: LOD level (number of samples)")
      .staticmethod("calculateLOD")

      .def(
          "generateReferenceCurve",
          &_GenerateReferenceCurve,
          return_value_policy<TfPySequenceToList>(),
          (arg("numSamples")),
          "Generate reference curve vertices for a given sample count.\n\n"
          "Args:\n"
          "    numSamples (int): Number of samples along the curve\n\n"
          "Returns:\n"
          "    list: Flat float array of vertices (7 floats per vertex, 2 vertices per sample)")
      .staticmethod("generateReferenceCurve")

      .def(
          "sampleLinkCurve",
          &_SampleLinkCurve,
          return_value_policy<TfPySequenceToList>(),
          (arg("start"), arg("end"), arg("numSamples"), arg("verticalEndTangent")),
          "Sample the shaped link curve into a polyline of world-space points.\n\n"
          "Reproduces the GPU vertex shader shaping so hit-testing matches what\n"
          "is drawn (forward<->backward blend, or the prim-target vertical cubic).\n\n"
          "Args:\n"
          "    start (Gf.Vec2d): Link start point in world space\n"
          "    end (Gf.Vec2d): Link end point in world space\n"
          "    numSamples (int): Number of samples (>= 2; fewer returns empty)\n"
          "    verticalEndTangent (bool): Use the prim-target vertical-entry cubic\n\n"
          "Returns:\n"
          "    list: numSamples Gf.Vec2d points along the rendered curve")
      .staticmethod("sampleLinkCurve")

      .def(
          "computeLinkCurveBounds",
          &_ComputeLinkCurveBounds,
          (arg("start"), arg("end"), arg("numSamples"), arg("verticalEndTangent")),
          "Conservative world-space bounds of the shaped link curve.\n\n"
          "Includes the backward S-bow that extends past the straight endpoint\n"
          "box, so a spatial broad-phase keyed on these bounds will not miss\n"
          "backward links. Pass the raw (un-inset) end to keep bounds zoom-\n"
          "independent.\n\n"
          "Args:\n"
          "    start (Gf.Vec2d): Link start point in world space\n"
          "    end (Gf.Vec2d): Link end point in world space\n"
          "    numSamples (int): Curve samples used to bound (>= 2)\n"
          "    verticalEndTangent (bool): Use the prim-target vertical-entry cubic\n\n"
          "Returns:\n"
          "    Gf.Range2d: Axis-aligned bounds containing the curve")
      .staticmethod("computeLinkCurveBounds")

      .def(
          "sampleLinkCurveAdaptive",
          &_SampleLinkCurveAdaptive,
          return_value_policy<TfPySequenceToList>(),
          (arg("start"), arg("end"), arg("verticalEndTangent"), arg("flatnessTolerance")),
          "Curvature-adaptive sampling of the shaped link curve.\n\n"
          "Subdivides where the curve bends until every chord is within\n"
          "flatnessTolerance world units of the curve; dense on the backward\n"
          "S-bow, sparse on near-straight links. Zoom-independent (cache it).\n\n"
          "Args:\n"
          "    start (Gf.Vec2d): Link start point in world space\n"
          "    end (Gf.Vec2d): Link end point in world space\n"
          "    verticalEndTangent (bool): Use the prim-target vertical-entry cubic\n"
          "    flatnessTolerance (float): Max chord deviation in world units\n\n"
          "Returns:\n"
          "    list: Gf.Vec2d points along the rendered curve (endpoints exact)")
      .staticmethod("sampleLinkCurveAdaptive");
}
