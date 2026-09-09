//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "spatial/SpatialIndex.h"
#include "pxr/pxr.h"

#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

  #include "pxr/external/boost/python.hpp"
  #include "pxr/external/boost/python/noncopyable.hpp"
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

tuple _QueryPoint(const SpatialIndex& index, const GfVec2d& point) {
  std::vector<std::string> nodeIds;
  std::vector<int> linkIndices;
  index.QueryPoint(_GfToNoodlesVec2d(point), nodeIds, linkIndices);

  list pyNodeIds;
  for (const auto& id : nodeIds) {
    pyNodeIds.append(id);
  }

  list pyLinkIndices;
  for (int idx : linkIndices) {
    pyLinkIndices.append(idx);
  }

  return make_tuple(pyNodeIds, pyLinkIndices);
}

tuple _QueryRegion(const SpatialIndex& index, const GfRange2d& bounds) {
  std::vector<std::string> nodeIds;
  std::vector<int> linkIndices;
  index.QueryRegion(_GfToNoodlesRange2d(bounds), nodeIds, linkIndices);

  list pyNodeIds;
  for (const auto& id : nodeIds) {
    pyNodeIds.append(id);
  }

  list pyLinkIndices;
  for (int idx : linkIndices) {
    pyLinkIndices.append(idx);
  }

  return make_tuple(pyNodeIds, pyLinkIndices);
}

void _BulkInsert(
    SpatialIndex& index,
    const list& pyNodeIds,
    const list& pyNodeBounds,
    const list& pyLinkBounds) {
  std::vector<std::string> nodeIds;
  std::vector<Range2d> nodeBounds;
  std::vector<Range2d> linkBounds;

  stl_input_iterator<std::string> nodeIdBegin(pyNodeIds), nodeIdEnd;
  nodeIds.assign(nodeIdBegin, nodeIdEnd);

  stl_input_iterator<GfRange2d> nodeBoundsBegin(pyNodeBounds), nodeBoundsEnd;
  for (auto it = nodeBoundsBegin; it != nodeBoundsEnd; ++it) {
    nodeBounds.push_back(_GfToNoodlesRange2d(*it));
  }

  stl_input_iterator<GfRange2d> linkBoundsBegin(pyLinkBounds), linkBoundsEnd;
  for (auto it = linkBoundsBegin; it != linkBoundsEnd; ++it) {
    linkBounds.push_back(_GfToNoodlesRange2d(*it));
  }

  index.BulkInsert(nodeIds, nodeBounds, linkBounds);
}

list _GetCells(const SpatialIndex& index) {
  std::vector<Range2d> cells = index.GetCells();

  list pyCells;
  for (const auto& cell : cells) {
    pyCells.append(_NoodlesToGfRange2d(cell));
  }

  return pyCells;
}

SpatialIndex* _MakeSpatialIndexDefault() {
  return new SpatialIndex();
}

SpatialIndex* _MakeSpatialIndexWithParams(int maxDepth, int maxItems, double minCellSize) {
  return new SpatialIndex(Range2d(), maxDepth, maxItems, minCellSize);
}

SpatialIndex*
_MakeSpatialIndexFull(const GfRange2d& bounds, int maxDepth, int maxItems, double minCellSize) {
  return new SpatialIndex(_GfToNoodlesRange2d(bounds), maxDepth, maxItems, minCellSize);
}

void _InsertNode(SpatialIndex& index, const std::string& nodeId, const GfRange2d& bounds) {
  index.InsertNode(nodeId, _GfToNoodlesRange2d(bounds));
}

void _InsertLink(SpatialIndex& index, int linkIndex, const GfRange2d& bounds) {
  index.InsertLink(linkIndex, _GfToNoodlesRange2d(bounds));
}

void _UpdateNode(SpatialIndex& index, const std::string& nodeId, const GfRange2d& bounds) {
  index.UpdateNode(nodeId, _GfToNoodlesRange2d(bounds));
}

} // anonymous namespace

void wrapSpatial() {
  enum_<SpatialItemType>("SpatialItemType")
      .value("Node", SpatialItemType::Node)
      .value("Link", SpatialItemType::Link);

  class_<SpatialIndex, noncopyable>(
      "SpatialIndex", "Quadtree for managing nodes and links in a node graph.\n\n", no_init)
      .def("__init__", make_constructor(&_MakeSpatialIndexDefault))
      .def(
          "__init__",
          make_constructor(
              &_MakeSpatialIndexWithParams,
              default_call_policies(),
              (arg("maxDepth") = 8, arg("maxItems") = 10, arg("minCellSize") = 10.0)),
          "Args:\n"
          "    maxDepth (int): Maximum quadtree depth (default: 8)\n"
          "    maxItems (int): Maximum items per leaf before subdivision (default: 10)\n"
          "    minCellSize (float): Minimum cell size for subdivision (default: 10.0)")
      .def(
          "__init__",
          make_constructor(
              &_MakeSpatialIndexFull,
              default_call_policies(),
              (arg("bounds"),
               arg("maxDepth") = 8,
               arg("maxItems") = 10,
               arg("minCellSize") = 10.0)),
          "Args:\n"
          "    bounds (Gf.Range2d): Initial bounds\n"
          "    maxDepth (int): Maximum quadtree depth (default: 8)\n"
          "    maxItems (int): Maximum items per leaf before subdivision (default: 10)\n"
          "    minCellSize (float): Minimum cell size for subdivision (default: 10.0)")

      .def(
          "insertNode",
          &_InsertNode,
          (arg("nodeId"), arg("bounds")),
          "Insert a node.\n\n"
          "Args:\n"
          "    nodeId (str): String ID of the node\n"
          "    bounds (Gf.Range2d): Pre-computed bounding box for the node")

      .def(
          "insertLink",
          &_InsertLink,
          (arg("linkIndex"), arg("bounds")),
          "Insert a link.\n\n"
          "Args:\n"
          "    linkIndex (int): Integer index of the link\n"
          "    bounds (Gf.Range2d): Pre-computed bounding box for the link")

      .def(
          "removeNode",
          &SpatialIndex::RemoveNode,
          (arg("nodeId")),
          "Remove a node.\n\n"
          "Args:\n"
          "    nodeId (str): String ID of the node\n\n"
          "Returns:\n"
          "    bool: True if node was found and removed")

      .def(
          "removeLink",
          &SpatialIndex::RemoveLink,
          (arg("linkIndex")),
          "Remove a link.\n\n"
          "Args:\n"
          "    linkIndex (int): Integer index of the link\n\n"
          "Returns:\n"
          "    bool: True if link was found and removed")

      .def(
          "updateNode",
          &_UpdateNode,
          (arg("nodeId"), arg("bounds")),
          "Update a node's position.\n\n"
          "Equivalent to remove + insert.\n\n"
          "Args:\n"
          "    nodeId (str): String ID of the node\n"
          "    bounds (Gf.Range2d): New bounding box for the node")

      .def(
          "queryPoint",
          &_QueryPoint,
          (arg("point")),
          "Args:\n"
          "    point (Gf.Vec2d): Point to query\n\n"
          "Returns:\n"
          "    tuple: (nodeIds[], linkIndices[]) where nodeIds is a list of\n"
          "           string node IDs and linkIndices is a list of integer\n"
          "           link indices")

      .def(
          "queryRegion",
          &_QueryRegion,
          (arg("bounds")),
          "Args:\n"
          "    bounds (Gf.Range2d): Region to query\n\n"
          "Returns:\n"
          "    tuple: (nodeIds[], linkIndices[]) where nodeIds is a list of\n"
          "           string node IDs and linkIndices is a list of integer\n"
          "           link indices")

      .def(
          "bulkInsert",
          &_BulkInsert,
          (arg("nodeIds"), arg("nodeBounds"), arg("linkBounds")),
          "Args:\n"
          "    nodeIds (list): List of node ID strings\n"
          "    nodeBounds (list): List of Gf.Range2d node bounding boxes\n"
          "    linkBounds (list): List of Gf.Range2d link bounding boxes")

      .def("clear", &SpatialIndex::Clear, "Clear all items from the spatial index.")

      .def(
          "getCells",
          &_GetCells,
          "Returns:\n"
          "    list: List of Gf.Range2d cells")

      .def(
          "getNodeCount",
          &SpatialIndex::GetNodeCount,
          "Returns:\n"
          "    int: Number of nodes")

      .def(
          "getLinkCount",
          &SpatialIndex::GetLinkCount,
          "Returns:\n"
          "    int: Number of links");
}
