//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "core/Animator.h"
#include "core/Glyph.h"
#include "core/LinkSelectionMode.h"
#include "core/NodeData.h"
#include "core/NodeLayout.h"
#include "core/RenderConfig.h"
#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/pxr.h"

#include <atomic>

#include "pxr/base/gf/range2d.h"
#include "pxr/base/gf/vec2d.h"

  #include "pxr/external/boost/python.hpp"
  #include "pxr/external/boost/python/suite/indexing/map_indexing_suite.hpp"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_boost::python;
using namespace noodles;

namespace {

GfVec2d _NoodlesToGfVec2d(const Vec2d& v) {
  return GfVec2d(v[0], v[1]);
}

Vec2d _GfToNoodlesVec2d(const GfVec2d& gf) {
  return Vec2d(gf[0], gf[1]);
}

GfRange2d _NoodlesToGfRange2d(const Range2d& r) {
  return GfRange2d(GfVec2d(r.GetMin()[0], r.GetMin()[1]), GfVec2d(r.GetMax()[0], r.GetMax()[1]));
}

Range2d _GfToNoodlesRange2d(const GfRange2d& gf) {
  return Range2d(Vec2d(gf.GetMin()[0], gf.GetMin()[1]), Vec2d(gf.GetMax()[0], gf.GetMax()[1]));
}

// Counter advanced on every NodeData position assignment made through the
// binding, which is the path all Python node moves take (the `position` setter
// and setDisplayPosition both route here). GraphView reads it as an O(1) check
// for "did any node move since I last reconciled" (see _reconcileMovedNodes), so
// a pan or idle frame never scans node versions. It is process-global, so a move
// in one graph also bumps another graph's gate; that is harmless, since the other
// graph's per-node version watermarks then find nothing changed.
std::atomic<uint64_t>& _positionWriteGen() {
  static std::atomic<uint64_t> gen{0};
  return gen;
}
uint64_t _positionWriteGeneration() {
  return _positionWriteGen().load(std::memory_order_relaxed);
}

// NodeData property accessors
GfVec2d _NodeData_getPosition(const NodeData& n) {
  return _NoodlesToGfVec2d(n.position);
}
void _NodeData_setPosition(NodeData& n, const GfVec2d& v) {
  n.position = _GfToNoodlesVec2d(v);
  _positionWriteGen().fetch_add(1, std::memory_order_relaxed);
}
GfVec2d _NodeData_getSize(const NodeData& n) {
  return _NoodlesToGfVec2d(n.size);
}
void _NodeData_setSize(NodeData& n, const GfVec2d& v) {
  n.size = _GfToNoodlesVec2d(v);
}

// LinkData property accessors
GfVec2d _LinkData_getStart(const LinkData& l) {
  return _NoodlesToGfVec2d(l.start);
}
void _LinkData_setStart(LinkData& l, const GfVec2d& v) {
  l.start = _GfToNoodlesVec2d(v);
}
GfVec2d _LinkData_getEnd(const LinkData& l) {
  return _NoodlesToGfVec2d(l.end);
}
void _LinkData_setEnd(LinkData& l, const GfVec2d& v) {
  l.end = _GfToNoodlesVec2d(v);
}

list _LinkData_getColor(const LinkData& l) {
  list result;
  for (int i = 0; i < 4; ++i) {
    result.append(l.color[i]);
  }
  return result;
}
void _LinkData_setColor(LinkData& l, const list& c) {
  for (int i = 0; i < 4 && i < len(c); ++i) {
    l.color[i] = extract<float>(c[i]);
  }
}

list _LinkData_getHighlightColor(const LinkData& l) {
  list result;
  for (int i = 0; i < 4; ++i) {
    result.append(l.highlightColor[i]);
  }
  return result;
}
void _LinkData_setHighlightColor(LinkData& l, const list& c) {
  for (int i = 0; i < 4 && i < len(c); ++i) {
    l.highlightColor[i] = extract<float>(c[i]);
  }
}

// StickerData property accessors
GfVec2d _StickerData_getPosition(const StickerData& s) {
  return _NoodlesToGfVec2d(s.position);
}
void _StickerData_setPosition(StickerData& s, const GfVec2d& v) {
  s.position = _GfToNoodlesVec2d(v);
}
GfVec2d _StickerData_getSize(const StickerData& s) {
  return _NoodlesToGfVec2d(s.size);
}
void _StickerData_setSize(StickerData& s, const GfVec2d& v) {
  s.size = _GfToNoodlesVec2d(v);
}

// Glyph property accessors
GfRange2d _Glyph_getPlaneBounds(const Glyph& g) {
  return _NoodlesToGfRange2d(g.planeBounds);
}
void _Glyph_setPlaneBounds(Glyph& g, const GfRange2d& r) {
  g.planeBounds = _GfToNoodlesRange2d(r);
}
GfRange2d _Glyph_getAtlasBounds(const Glyph& g) {
  return _NoodlesToGfRange2d(g.atlasBounds);
}
void _Glyph_setAtlasBounds(Glyph& g, const GfRange2d& r) {
  g.atlasBounds = _GfToNoodlesRange2d(r);
}

// rowKinds property accessors (std::vector<int> has no automatic converter)
list _NodeData_getInputRowKinds(const NodeData& n) {
  list result;
  for (int k : n.inputRowKinds) {
    result.append(k);
  }
  return result;
}
void _NodeData_setInputRowKinds(NodeData& n, const list& kinds) {
  n.inputRowKinds.clear();
  for (int i = 0; i < len(kinds); ++i) {
    n.inputRowKinds.push_back(extract<int>(kinds[i]));
  }
}
// relationship-pin accessors (std::unordered_set<std::string> has no auto converter)
list _NodeData_getRelationshipInputPins(const NodeData& n) {
  list result;
  for (const auto& p : n.relationshipInputPins) {
    result.append(p);
  }
  return result;
}
void _NodeData_setRelationshipInputPins(NodeData& n, const list& pins) {
  n.relationshipInputPins.clear();
  for (int i = 0; i < len(pins); ++i) {
    n.relationshipInputPins.insert(extract<std::string>(pins[i]));
  }
}
list _NodeData_getRelationshipOutputPins(const NodeData& n) {
  list result;
  for (const auto& p : n.relationshipOutputPins) {
    result.append(p);
  }
  return result;
}
void _NodeData_setRelationshipOutputPins(NodeData& n, const list& pins) {
  n.relationshipOutputPins.clear();
  for (int i = 0; i < len(pins); ++i) {
    n.relationshipOutputPins.insert(extract<std::string>(pins[i]));
  }
}
list _NodeData_getOutputRowKinds(const NodeData& n) {
  list result;
  for (int k : n.outputRowKinds) {
    result.append(k);
  }
  return result;
}
void _NodeData_setOutputRowKinds(NodeData& n, const list& kinds) {
  n.outputRowKinds.clear();
  for (int i = 0; i < len(kinds); ++i) {
    n.outputRowKinds.push_back(extract<int>(kinds[i]));
  }
}
list _NodeData_getInputRowSlots(const NodeData& n) {
  list result;
  for (int slot : n.inputRowSlots) {
    result.append(slot);
  }
  return result;
}
void _NodeData_setInputRowSlots(NodeData& n, const list& slots) {
  n.inputRowSlots.clear();
  for (int i = 0; i < len(slots); ++i) {
    n.inputRowSlots.push_back(extract<int>(slots[i]));
  }
}
list _NodeData_getOutputRowSlots(const NodeData& n) {
  list result;
  for (int slot : n.outputRowSlots) {
    result.append(slot);
  }
  return result;
}
void _NodeData_setOutputRowSlots(NodeData& n, const list& slots) {
  n.outputRowSlots.clear();
  for (int i = 0; i < len(slots); ++i) {
    n.outputRowSlots.push_back(extract<int>(slots[i]));
  }
}
list _NodeData_getDisplayRowKinds(const NodeData& n) {
  list result;
  for (int kind : n.displayRowKinds) {
    result.append(kind);
  }
  return result;
}
void _NodeData_setDisplayRowKinds(NodeData& n, const list& kinds) {
  n.displayRowKinds.clear();
  for (int i = 0; i < len(kinds); ++i) {
    n.displayRowKinds.push_back(extract<int>(kinds[i]));
  }
}

// Read-only: per-pin resolved row-center Y (node-local), set by calculateNodeSize.
list _NodeData_getLayoutInputCenterY(const NodeData& n) {
  list result;
  for (double y : n.layoutInputCenterY) {
    result.append(y);
  }
  return result;
}
list _NodeData_getLayoutOutputCenterY(const NodeData& n) {
  list result;
  for (double y : n.layoutOutputCenterY) {
    result.append(y);
  }
  return result;
}

// --- Raw USD-derived content accessors (no auto converters for these types) ---
dict _NodeData_getFoldState(const NodeData& n) {
  dict result;
  for (const auto& kv : n.foldState) {
    result[kv.first] = kv.second;
  }
  return result;
}
void _NodeData_setFoldState(NodeData& n, const dict& d) {
  n.foldState.clear();
  list items = d.items();
  for (int i = 0; i < len(items); ++i) {
    object kv = items[i];
    n.foldState[extract<std::string>(kv[0])] = extract<bool>(kv[1]);
  }
}
list _NodeData_getOrderedPinEntries(const NodeData& n) {
  list result;
  for (const auto& e : n.orderedPinEntries) {
    result.append(make_tuple(e.first, e.second));
  }
  return result;
}
void _NodeData_setOrderedPinEntries(NodeData& n, const list& entries) {
  n.orderedPinEntries.clear();
  for (int i = 0; i < len(entries); ++i) {
    object e = entries[i];
    n.orderedPinEntries.emplace_back(extract<std::string>(e[0]), extract<std::string>(e[1]));
  }
}
list _NodeData_getInputDirectionGroupPins(const NodeData& n) {
  list result;
  for (const auto& p : n.inputDirectionGroupPins) {
    result.append(p);
  }
  return result;
}
void _NodeData_setInputDirectionGroupPins(NodeData& n, const list& pins) {
  n.inputDirectionGroupPins.clear();
  for (int i = 0; i < len(pins); ++i) {
    n.inputDirectionGroupPins.insert(extract<std::string>(pins[i]));
  }
}
list _NodeData_getOutputDirectionGroupPins(const NodeData& n) {
  list result;
  for (const auto& p : n.outputDirectionGroupPins) {
    result.append(p);
  }
  return result;
}
void _NodeData_setOutputDirectionGroupPins(NodeData& n, const list& pins) {
  n.outputDirectionGroupPins.clear();
  for (int i = 0; i < len(pins); ++i) {
    n.outputDirectionGroupPins.insert(extract<std::string>(pins[i]));
  }
}
list _NodeData_getDualPinNames(const NodeData& n) {
  list result;
  for (const auto& p : n.dualPinNames) {
    result.append(p);
  }
  return result;
}
void _NodeData_setDualPinNames(NodeData& n, const list& pins) {
  n.dualPinNames.clear();
  for (int i = 0; i < len(pins); ++i) {
    n.dualPinNames.insert(extract<std::string>(pins[i]));
  }
}
list _NodeData_getOutputDualPinNames(const NodeData& n) {
  list result;
  for (const auto& p : n.outputDualPinNames) {
    result.append(p);
  }
  return result;
}
void _NodeData_setOutputDualPinNames(NodeData& n, const list& pins) {
  n.outputDualPinNames.clear();
  for (int i = 0; i < len(pins); ++i) {
    n.outputDualPinNames.insert(extract<std::string>(pins[i]));
  }
}

// Read-only native-list views of the C++-produced display pins. NodeData's
// std::vector<string> fields (inputPins) have no C++->Python read converter
// (the _tf registry mismatch), so these manual-list accessors are how Python
// reads the display pins the C++ layout producer wrote.
list _NodeData_getDisplayInputPins(const NodeData& n) {
  list result;
  for (const auto& p : n.inputPins) {
    result.append(p);
  }
  return result;
}
list _NodeData_getDisplayOutputPins(const NodeData& n) {
  list result;
  for (const auto& p : n.outputPins) {
    result.append(p);
  }
  return result;
}

// Folded child->header maps as native dicts (no dict<->unordered_map converter).
dict _NodeData_getFoldedInputPinMap(const NodeData& n) {
  dict result;
  for (const auto& kv : n.foldedInputPinMap) {
    result[kv.first] = kv.second;
  }
  return result;
}
void _NodeData_setFoldedInputPinMap(NodeData& n, const dict& d) {
  n.foldedInputPinMap.clear();
  list items = d.items();
  for (int i = 0; i < len(items); ++i) {
    object kv = items[i];
    n.foldedInputPinMap[extract<std::string>(kv[0])] = extract<std::string>(kv[1]);
  }
}
dict _NodeData_getFoldedOutputPinMap(const NodeData& n) {
  dict result;
  for (const auto& kv : n.foldedOutputPinMap) {
    result[kv.first] = kv.second;
  }
  return result;
}
void _NodeData_setFoldedOutputPinMap(NodeData& n, const dict& d) {
  n.foldedOutputPinMap.clear();
  list items = d.items();
  for (int i = 0; i < len(items); ++i) {
    object kv = items[i];
    n.foldedOutputPinMap[extract<std::string>(kv[0])] = extract<std::string>(kv[1]);
  }
}

// RenderConfig port-color accessors (std::array<float,4> has no auto converter).
list _RenderConfig_getArray4(const std::array<float, 4>& a) {
  list result;
  for (float v : a) {
    result.append(v);
  }
  return result;
}
void _RenderConfig_setArray4(std::array<float, 4>& a, const list& c) {
  for (int i = 0; i < 4 && i < len(c); ++i) {
    a[i] = extract<float>(c[i]);
  }
}
list _RenderConfig_getPortConnectedFill(const RenderConfig& c) {
  return _RenderConfig_getArray4(c.portConnectedFillColor);
}
void _RenderConfig_setPortConnectedFill(RenderConfig& c, const list& v) {
  _RenderConfig_setArray4(c.portConnectedFillColor, v);
}
list _RenderConfig_getPortConnectedRing(const RenderConfig& c) {
  return _RenderConfig_getArray4(c.portConnectedRingColor);
}
void _RenderConfig_setPortConnectedRing(RenderConfig& c, const list& v) {
  _RenderConfig_setArray4(c.portConnectedRingColor, v);
}
list _RenderConfig_getPortDisconnectedFill(const RenderConfig& c) {
  return _RenderConfig_getArray4(c.portDisconnectedFillColor);
}
void _RenderConfig_setPortDisconnectedFill(RenderConfig& c, const list& v) {
  _RenderConfig_setArray4(c.portDisconnectedFillColor, v);
}
list _RenderConfig_getPortDisconnectedRing(const RenderConfig& c) {
  return _RenderConfig_getArray4(c.portDisconnectedRingColor);
}
void _RenderConfig_setPortDisconnectedRing(RenderConfig& c, const list& v) {
  _RenderConfig_setArray4(c.portDisconnectedRingColor, v);
}
list _RenderConfig_getPortHover(const RenderConfig& c) {
  return _RenderConfig_getArray4(c.portHoverColor);
}
void _RenderConfig_setPortHover(RenderConfig& c, const list& v) {
  _RenderConfig_setArray4(c.portHoverColor, v);
}

list _GetConnectedNodeIds(const GraphModel& model, const std::string& nodeId) {
  auto ids = model.getConnectedNodeIds(nodeId);
  list result;
  for (const auto& id : ids) {
    result.append(id);
  }
  return result;
}

// (port, isOutput) tuples; std::vector<std::pair<...>> has no auto converter.
list _SyntheticRelationshipPortsForNode(const GraphModel& model, const std::string& nodeId) {
  list result;
  for (const auto& [port, isOutput] : model.syntheticRelationshipPortsForNode(nodeId)) {
    result.append(make_tuple(port, isOutput));
  }
  return result;
}

// Populate the C++ GraphModel::nodes render snapshot from the Python NodeModel
// dict. NodeModel IS-A NodeData, so extract<NodeData&> yields the embedded
// struct. Build a local map then move-assign, so a malformed-entry skip can
// never leave nodes half-populated. The unordered_map type is never exposed to
// Python (no container converter registered — avoids the _tf double-registration
// hazard noted below); Python only hands in a dict.
void _GraphModel_syncNodesFromModels(GraphModel& self, const dict& pyNodes) {
  std::unordered_map<std::string, NodeData> nodes;
  list items = pyNodes.items();
  int n = len(items);
  nodes.reserve(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    object kv = items[i];
    extract<std::string> keyExtract(kv[0]);
    extract<NodeData&> nodeExtract(kv[1]);
    if (!keyExtract.check() || !nodeExtract.check()) {
      continue;
    }
    nodes.emplace(keyExtract(), nodeExtract());
  }
  self.nodes = std::move(nodes);
}

// Populate the C++ GraphModel::links render snapshot from the Python self.links
// list (the live interaction model). LinkData is bound directly (no Model
// subclass), so extract<LinkData&> yields the struct and copies every bound
// field, including is_relationship_link. Build a local vector then move-assign,
// so a malformed-entry skip can never leave links half-populated. Mirrors
// _GraphModel_syncNodesFromModels; called only on structural change.
void _GraphModel_syncLinksFromModels(GraphModel& self, const list& pyLinks) {
  std::vector<LinkData> links;
  ssize_t n = len(pyLinks);
  links.reserve(static_cast<size_t>(n));
  for (ssize_t i = 0; i < n; ++i) {
    extract<LinkData&> linkExtract(pyLinks[i]);
    if (!linkExtract.check()) {
      continue;
    }
    links.push_back(linkExtract());
  }
  self.links = std::move(links);
}

void _CalculateNodeSize(
    GraphModel& self,
    NodeData& node,
    object pyCalcTextWidth,
    const FontMetrics& fontMetrics,
    const RenderConfig& config) {
  TextWidthCallback cb = [&pyCalcTextWidth](const std::string& text, double fontSize) -> double {
    return extract<double>(pyCalcTextWidth(text, fontSize));
  };
  self.calculateNodeSize(node, cb, fontMetrics, &config);
}

void _LayoutNode(
    GraphModel& self,
    NodeData& node,
    object pyCalcTextWidth,
    const FontMetrics& fontMetrics,
    const RenderConfig& config) {
  TextWidthCallback cb = [&pyCalcTextWidth](const std::string& text, double fontSize) -> double {
    return extract<double>(pyCalcTextWidth(text, fontSize));
  };
  self.layoutNode(node, cb, fontMetrics, &config);
}

AnimatableProperty* _Animator_addProperty(Animator& self) {
  return self.addProperty();
}

} // anonymous namespace

void wrapNodeData() {
  // NOTE: We do NOT wrap std::vector<std::string> or std::vector<int> here.
  // USD's _tf module (wrapPyContainerConversions.cpp) already registers global
  // to_python/from_python converters for these types. Registering them again via
  // class_<> + vector_indexing_suite would trigger a boost::python assertion:
  //   assert(slot->m_to_python == 0)  // registry.cpp:217
  // The def_readwrite members on NodeData etc. work fine with the _tf converters.

  // String-to-string map (for pin types)
  class_<std::unordered_map<std::string, std::string>>("StringMap")
      .def(map_indexing_suite<std::unordered_map<std::string, std::string>>());

  class_<NodeData>("NodeData", "Data for a node in the graph.", init<>())
      .def_readwrite("id", &NodeData::id)
      .def_readwrite("name", &NodeData::name)
      .def_readwrite("type", &NodeData::type)
      .def_readwrite("schemaTypeName", &NodeData::schemaTypeName)
      .add_property("position", &_NodeData_getPosition, &_NodeData_setPosition)
      // Read-only move counter consumed by GraphView._reconcileMovedNodes (see
      // VersionedVec2d); assigning `position` advances it.
      .add_property("positionVersion", &NodeData::positionVersion)
      .add_property("size", &_NodeData_getSize, &_NodeData_setSize)
      .def_readwrite("inputPins", &NodeData::inputPins)
      .def_readwrite("outputPins", &NodeData::outputPins)
      .def_readwrite("inputPinTypes", &NodeData::inputPinTypes)
      .def_readwrite("outputPinTypes", &NodeData::outputPinTypes)
      .def_readwrite("inputLinks", &NodeData::inputLinks)
      .def_readwrite("outputLinks", &NodeData::outputLinks)
      .add_property("inputRowKinds", &_NodeData_getInputRowKinds, &_NodeData_setInputRowKinds)
      .add_property("outputRowKinds", &_NodeData_getOutputRowKinds, &_NodeData_setOutputRowKinds)
      .add_property("inputRowSlots", &_NodeData_getInputRowSlots, &_NodeData_setInputRowSlots)
      .add_property("outputRowSlots", &_NodeData_getOutputRowSlots, &_NodeData_setOutputRowSlots)
      .add_property("displayRowKinds", &_NodeData_getDisplayRowKinds, &_NodeData_setDisplayRowKinds)
      .add_property(
          "relationshipInputPins",
          &_NodeData_getRelationshipInputPins,
          &_NodeData_setRelationshipInputPins)
      .add_property(
          "relationshipOutputPins",
          &_NodeData_getRelationshipOutputPins,
          &_NodeData_setRelationshipOutputPins)
      .def_readwrite("originalInputPins", &NodeData::originalInputPins)
      .def_readwrite("originalOutputPins", &NodeData::originalOutputPins)
      .def_readwrite("originalInputPinTypes", &NodeData::originalInputPinTypes)
      .def_readwrite("originalOutputPinTypes", &NodeData::originalOutputPinTypes)
      .add_property("foldState", &_NodeData_getFoldState, &_NodeData_setFoldState)
      .add_property(
          "orderedPinEntries", &_NodeData_getOrderedPinEntries, &_NodeData_setOrderedPinEntries)
      .add_property(
          "inputDirectionGroupPins",
          &_NodeData_getInputDirectionGroupPins,
          &_NodeData_setInputDirectionGroupPins)
      .add_property(
          "outputDirectionGroupPins",
          &_NodeData_getOutputDirectionGroupPins,
          &_NodeData_setOutputDirectionGroupPins)
      .add_property("dualPinNames", &_NodeData_getDualPinNames, &_NodeData_setDualPinNames)
      .add_property(
          "outputDualPinNames", &_NodeData_getOutputDualPinNames, &_NodeData_setOutputDualPinNames)
      .add_property("displayInputPins", &_NodeData_getDisplayInputPins)
      .add_property("displayOutputPins", &_NodeData_getDisplayOutputPins)
      .add_property(
          "foldedInputPinMap", &_NodeData_getFoldedInputPinMap, &_NodeData_setFoldedInputPinMap)
      .add_property(
          "foldedOutputPinMap", &_NodeData_getFoldedOutputPinMap, &_NodeData_setFoldedOutputPinMap)
      .def_readwrite("selected", &NodeData::selected)
      .def_readwrite("titleCollapsed", &NodeData::titleCollapsed)
      .def_readwrite("uiStyle", &NodeData::uiStyle)
      .def_readwrite("titleIconPath", &NodeData::titleIconPath)
      .def_readwrite("textStartIndex", &NodeData::textStartIndex)
      .def_readwrite("textNumChars", &NodeData::textNumChars)
      .def_readwrite("zOrder", &NodeData::zOrder)
      .def_readwrite("displayColorR", &NodeData::displayColorR)
      .def_readwrite("displayColorG", &NodeData::displayColorG)
      .def_readwrite("displayColorB", &NodeData::displayColorB)
      .def_readonly("layoutTitleHeight", &NodeData::layoutTitleHeight)
      .def_readonly("layoutPortStartY", &NodeData::layoutPortStartY)
      .def_readonly("layoutPortLineHeight", &NodeData::layoutPortLineHeight)
      .def_readonly("layoutPortNameHeight", &NodeData::layoutPortNameHeight)
      .def_readonly("layoutPortTypeHeight", &NodeData::layoutPortTypeHeight)
      .def_readonly("layoutRowCount", &NodeData::layoutRowCount)
      .add_property("layoutInputCenterY", &_NodeData_getLayoutInputCenterY)
      .add_property("layoutOutputCenterY", &_NodeData_getLayoutOutputCenterY);

  class_<LinkData>("LinkData", "Data for a link between nodes.", init<>())
      .def_readwrite("sourceNodeId", &LinkData::sourceNodeId)
      .def_readwrite("sourcePort", &LinkData::sourcePort)
      .def_readwrite("targetNodeId", &LinkData::targetNodeId)
      .def_readwrite("targetPort", &LinkData::targetPort)
      .add_property("start", &_LinkData_getStart, &_LinkData_setStart)
      .add_property("end", &_LinkData_getEnd, &_LinkData_setEnd)
      .def_readwrite("selected", &LinkData::selected)
      .def_readwrite("hovered", &LinkData::hovered)
      .def_readwrite("isDangling", &LinkData::isDangling)
      .def_readwrite("danglingDirection", &LinkData::danglingDirection)
      .add_property("color", &_LinkData_getColor, &_LinkData_setColor)
      .def_readwrite("highlighted", &LinkData::highlighted)
      .add_property("highlightColor", &_LinkData_getHighlightColor, &_LinkData_setHighlightColor)
      .def_readwrite("hasColor", &LinkData::hasColor)
      .def_readwrite("is_relationship_link", &LinkData::isRelationship)
      .def_readwrite("targetPropertyName", &LinkData::targetPropertyName)
      .def_readonly("DANGLING_LINK_LENGTH", &LinkData::DANGLING_LINK_LENGTH);

  class_<StickerData>("StickerData", "Data for a group sticker.", init<>())
      .def_readwrite("id", &StickerData::id)
      .def_readwrite("label", &StickerData::label)
      .add_property("position", &_StickerData_getPosition, &_StickerData_setPosition)
      .add_property("size", &_StickerData_getSize, &_StickerData_setSize)
      .def_readwrite("r", &StickerData::r)
      .def_readwrite("g", &StickerData::g)
      .def_readwrite("b", &StickerData::b)
      .def_readwrite("a", &StickerData::a)
      .def_readwrite("selected", &StickerData::selected)
      .def_readwrite("nodeIds", &StickerData::nodeIds);

  class_<FontMetrics>("FontMetrics", "Font metrics for layout.", init<>())
      .def_readwrite("ascender", &FontMetrics::ascender)
      .def_readwrite("descender", &FontMetrics::descender)
      .def_readwrite("lineHeight", &FontMetrics::lineHeight);

  class_<Glyph>("Glyph", "Font glyph metrics.", init<>())
      .def_readwrite("unicode", &Glyph::unicode)
      .def_readwrite("advance", &Glyph::advance)
      .add_property("planeBounds", &_Glyph_getPlaneBounds, &_Glyph_setPlaneBounds)
      .add_property("atlasBounds", &_Glyph_getAtlasBounds, &_Glyph_setAtlasBounds);

  auto renderConfigClass =
      class_<RenderConfig>("RenderConfig", "Rendering configuration.", init<>())
          .def_readwrite("nodeTitleFontSize", &RenderConfig::nodeTitleFontSize)
          .def_readwrite("nodePinFontSize", &RenderConfig::nodePinFontSize)
          .def_readwrite("nodePinTypeFontSize", &RenderConfig::nodePinTypeFontSize)
          .def_readwrite("nodeMarginH", &RenderConfig::nodeMarginH)
          .def_readwrite("nodeMarginV", &RenderConfig::nodeMarginV)
          .def_readwrite("nodePortSpacing", &RenderConfig::nodePortSpacing)
          .def_readwrite("nodePortWidth", &RenderConfig::nodePortWidth)
          .def_readwrite("nodeCornerRadius", &RenderConfig::nodeCornerRadius)
          .def_readwrite("selectedNodeStrokeWidth", &RenderConfig::selectedNodeStrokeWidth)
          .def_readwrite("nodeBgHigh", &RenderConfig::nodeBgHigh)
          .def_readwrite("nodeBgLow", &RenderConfig::nodeBgLow)
          .def_readwrite("nodeBgAlpha", &RenderConfig::nodeBgAlpha)
          .def_readwrite("nodeShadowFactor", &RenderConfig::nodeShadowFactor)
          .def_readwrite("nodeTypeSaturation", &RenderConfig::nodeTypeSaturation)
          .def_readwrite("nodeTypeBrightness", &RenderConfig::nodeTypeBrightness)
          .def_readwrite("nodePortRadius", &RenderConfig::nodePortRadius)
          .def_readwrite("nodeFontSize", &RenderConfig::nodeFontSize)
          .def_readwrite("nodeRendererType", &RenderConfig::nodeRendererType)
          .def_readwrite("isGraffiStyle", &RenderConfig::isGraffiStyle)
          .def_readwrite("showPinTypeLabels", &RenderConfig::showPinTypeLabels)
          .def_readwrite("linkLineWidth", &RenderConfig::linkLineWidth)
          .def_readwrite("linkSampleRate", &RenderConfig::linkSampleRate)
          .def_readwrite("linkEdgeDimmingStart", &RenderConfig::linkEdgeDimmingStart)
          .def_readwrite("linkEdgeDimmingEnd", &RenderConfig::linkEdgeDimmingEnd)
          .def_readwrite("linkCutoffAlpha", &RenderConfig::linkCutoffAlpha)
          .def_readwrite("nodePortRingThickness", &RenderConfig::nodePortRingThickness)
          .add_property(
              "portConnectedFillColor",
              &_RenderConfig_getPortConnectedFill,
              &_RenderConfig_setPortConnectedFill)
          .add_property(
              "portConnectedRingColor",
              &_RenderConfig_getPortConnectedRing,
              &_RenderConfig_setPortConnectedRing)
          .add_property(
              "portDisconnectedFillColor",
              &_RenderConfig_getPortDisconnectedFill,
              &_RenderConfig_setPortDisconnectedFill)
          .add_property(
              "portDisconnectedRingColor",
              &_RenderConfig_getPortDisconnectedRing,
              &_RenderConfig_setPortDisconnectedRing)
          .add_property("portHoverColor", &_RenderConfig_getPortHover, &_RenderConfig_setPortHover)
          .def("get", &RenderConfig::get, (arg("key"), arg("defaultValue")))
          .def("getInt", &RenderConfig::getInt, (arg("key"), arg("defaultValue")))
          .def("getString", &RenderConfig::getString, (arg("key"), arg("defaultValue")));
  renderConfigClass.attr("kSchemaTypeFontRatio") = RenderConfig::kSchemaTypeFontRatio;

  enum_<LinkSelectionMode>("LinkSelectionMode")
      .value("NODES_ONLY", LinkSelectionMode::NODES_ONLY)
      .value("WITH_ALL_LINKS", LinkSelectionMode::WITH_ALL_LINKS)
      .value("WITH_INPUTS", LinkSelectionMode::WITH_INPUTS)
      .value("WITH_OUTPUTS", LinkSelectionMode::WITH_OUTPUTS);

  class_<AnimatableProperty>("AnimatableProperty", "Animatable property.", init<>())
      .def_readwrite("current", &AnimatableProperty::current)
      .def_readwrite("target", &AnimatableProperty::target)
      .def_readwrite("speed", &AnimatableProperty::speed);

  class_<Animator>("Animator", "Exponential ease-out animator.", init<>())
      .def("addProperty", &_Animator_addProperty, return_internal_reference<>())
      .def("update", &Animator::update, (arg("dt")))
      .def("isAnimating", &Animator::isAnimating);

  class_<GraphModel>("GraphModel", "Container for graph data.", init<>())
      .def("markLinksChanged", &GraphModel::markLinksChanged)
      .def("isLinksChanged", &GraphModel::isLinksChanged)
      .def("getLinkCount", &GraphModel::linkCount)
      .def("buildConnectionCache", &GraphModel::buildConnectionCache)
      .def("getConnectedNodeIds", &_GetConnectedNodeIds, (arg("nodeId")))
      .def(
          "isPortConnected",
          &GraphModel::isPortConnected,
          (arg("nodeId"), arg("port"), arg("isOutput")))
      .def(
          "isFoldedHeaderConnected",
          &GraphModel::isFoldedHeaderConnected,
          (arg("nodeId"), arg("headerPin"), arg("isOutput")))
      .def(
          "syntheticRelationshipPortsForNode", &_SyntheticRelationshipPortsForNode, (arg("nodeId")))
      .def(
          "calculateNodeSize",
          &_CalculateNodeSize,
          (arg("node"), arg("calculateTextWidth"), arg("fontMetrics"), arg("config")))
      .def(
          "layoutNode",
          &_LayoutNode,
          (arg("node"), arg("calculateTextWidth"), arg("fontMetrics"), arg("config")))
      .def("syncNodesFromModels", &_GraphModel_syncNodesFromModels, (arg("nodes")))
      .def("syncLinksFromModels", &_GraphModel_syncLinksFromModels, (arg("links")))
      .def("clear", &GraphModel::clear);

  // Free-function layout passes (text-metric agnostic): the Python data source
  // calls these to (re)derive the display pins + row slots in C++ synchronously
  // when pins/fold change, without needing a font atlas. The full layoutNode
  // (with sizing) runs at the render chokepoint.
  def("buildDisplayPins", &buildDisplayPins, (arg("node")));
  def("assignRowSlots", &assignRowSlots, (arg("node")));
  def("positionWriteGeneration", &_positionWriteGeneration);
}
