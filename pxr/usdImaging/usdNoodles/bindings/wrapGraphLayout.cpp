//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "core/GraphLayout.h"
#include "core/NodeData.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/pxr.h"

  #include "pxr/external/boost/python.hpp"

#include <string>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_boost::python;
using namespace noodles;

namespace {

// Build a GraphModel from the Python NodeModel dict + LinkData list, run the
// Sugiyama layout, and return {nodeId: GfVec2d} of the new positions. The caller
// (graphView.py) applies these through the NodeModel position setter, so this
// binding stays decoupled from persistence (display-only vs USD-authoring).
//
// We return positions instead of mutating in place because the C++ GraphModel
// holds COPIES of each NodeData (NodeModel IS-A NodeData; extract<NodeData&>
// yields the embedded base, then emplace copies it) and GraphModel::nodes is
// never exposed back to Python -- mirrors _GraphModel_syncNodesFromModels in
// wrapNodeData.cpp. The copies carry the cached size + per-pin centers that the
// layout reads.
dict _layoutGraphPositions(const dict& pyNodes, const list& pyLinks, const LayoutParams& params) {
  GraphModel graph;

  std::unordered_map<std::string, NodeData> nodes;
  list nodeItems = pyNodes.items();
  ssize_t nodeCount = len(nodeItems);
  nodes.reserve(static_cast<size_t>(nodeCount));
  for (ssize_t i = 0; i < nodeCount; ++i) {
    object kv = nodeItems[i];
    extract<std::string> keyExtract(kv[0]);
    extract<NodeData&> nodeExtract(kv[1]);
    if (!keyExtract.check() || !nodeExtract.check()) {
      continue;
    }
    nodes.emplace(keyExtract(), nodeExtract());
  }
  graph.nodes = std::move(nodes);

  std::vector<LinkData> links;
  ssize_t linkCount = len(pyLinks);
  links.reserve(static_cast<size_t>(linkCount));
  for (ssize_t i = 0; i < linkCount; ++i) {
    extract<LinkData&> linkExtract(pyLinks[i]);
    if (!linkExtract.check()) {
      continue;
    }
    links.push_back(linkExtract());
  }
  graph.links = std::move(links);

  layoutGraph(graph, params);

  dict result;
  for (const auto& [id, node] : graph.nodes) {
    result[id] = GfVec2d(node.position[0], node.position[1]);
  }
  return result;
}

} // namespace

void wrapGraphLayout() {
  class_<LayoutParams>("LayoutParams", "Tunable parameters for graph auto-layout.", init<>())
      .def_readwrite("columnGap", &LayoutParams::columnGap)
      .def_readwrite("intraClusterGap", &LayoutParams::intraClusterGap)
      .def_readwrite("interClusterGap", &LayoutParams::interClusterGap)
      .def_readwrite("componentGap", &LayoutParams::componentGap)
      .def_readwrite("tightClusterMinSpacing", &LayoutParams::tightClusterMinSpacing)
      .def_readwrite("tightClusterGap", &LayoutParams::tightClusterGap)
      .def_readwrite("inputWeight", &LayoutParams::inputWeight)
      .def_readwrite("outputWeight", &LayoutParams::outputWeight)
      .def_readwrite("centerBias", &LayoutParams::centerBias)
      .def_readwrite("linearCenterBias", &LayoutParams::linearCenterBias)
      .def_readwrite("jitterFractionX", &LayoutParams::jitterFractionX)
      .def_readwrite("jitterFractionY", &LayoutParams::jitterFractionY);

  // Returns {nodeId: GfVec2d} for the laid-out positions; the caller applies them.
  // params is optional and defaults to the built-in LayoutParams constants.
  def("layoutGraphPositions",
      &_layoutGraphPositions,
      (arg("nodes"), arg("links"), arg("params") = LayoutParams()));
}
