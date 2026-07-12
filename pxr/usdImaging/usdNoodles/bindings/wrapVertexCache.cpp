//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "render/NodeVertexCache.h"
#include "pxr/pxr.h"

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

void _Update(
    NodeVertexCache& cache,
    const std::string& nodeId,
    const list& pyVertices,
    const GfVec2d& position,
    const GfVec2d& size,
    bool selected) {
  std::vector<NodeVertex> vertices;
  stl_input_iterator<NodeVertex> begin(pyVertices), end;
  vertices.assign(begin, end);
  cache.update(nodeId, vertices, _GfToNoodlesVec2d(position), _GfToNoodlesVec2d(size), selected);
}

list _GetVertices(const NodeVertexCache& cache, const std::string& nodeId) {
  const auto& vertices = cache.getVertices(nodeId);
  list result;
  for (const auto& v : vertices) {
    result.append(v);
  }
  return result;
}

bool _IsDirty(
    const NodeVertexCache& cache,
    const std::string& nodeId,
    const GfVec2d& position,
    const GfVec2d& size) {
  return cache.isDirty(nodeId, _GfToNoodlesVec2d(position), _GfToNoodlesVec2d(size));
}

} // anonymous namespace

void wrapVertexCache() {
  class_<NodeVertexCache>(
      "NodeVertexCache", "Cache for node vertex data with dirty tracking.", init<>())
      .def(
          "isDirty",
          &_IsDirty,
          (arg("nodeId"), arg("position"), arg("size")),
          "Check if a node needs vertex regeneration.\n\n"
          "Args:\n"
          "    nodeId (str): Node ID\n"
          "    position (Gf.Vec2d): Current node position\n"
          "    size (Gf.Vec2d): Current node size\n\n"
          "Returns:\n"
          "    bool: True if vertices need regeneration")

      .def(
          "update",
          &_Update,
          (arg("nodeId"), arg("vertices"), arg("position"), arg("size"), arg("selected")),
          "Store generated vertices for a node.\n\n"
          "Args:\n"
          "    nodeId (str): Node ID\n"
          "    vertices (list): List of NodeVertex objects\n"
          "    position (Gf.Vec2d): Node position snapshot\n"
          "    size (Gf.Vec2d): Node size snapshot\n"
          "    selected (bool): Node selection state")

      .def(
          "getVertices",
          &_GetVertices,
          (arg("nodeId")),
          "Get cached vertices.\n\n"
          "Returns:\n"
          "    list: Cached NodeVertex objects (empty if not cached)")

      .def(
          "getGeneration",
          &NodeVertexCache::getGeneration,
          (arg("nodeId")),
          "Get per-node generation counter.")

      .def(
          "updateSelectedFlag",
          &NodeVertexCache::updateSelectedFlag,
          (arg("nodeId"), arg("selected"), arg("innerStrokeValue")),
          "Fast path: flip selected flag without full regeneration.\n\n"
          "Returns:\n"
          "    bool: True if the flag was actually changed")

      .def(
          "invalidateNode",
          &NodeVertexCache::invalidateNode,
          (arg("nodeId")),
          "Mark a specific node for regeneration.")

      .def("invalidateAll", &NodeVertexCache::invalidateAll, "Mark all nodes for regeneration.")

      .def("removeNode", &NodeVertexCache::removeNode, (arg("nodeId")), "Remove a node from cache.")

      .def("clear", &NodeVertexCache::clear, "Clear entire cache.")

      .def("__len__", &NodeVertexCache::size)
      .def("__contains__", &NodeVertexCache::contains);
}
