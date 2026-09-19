//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "core/NodeVertex.h"
#include "render/GraphNodeRenderer.h"
#include "render/Profiler.h"
#include "render/VertexGenerator.h"
#include "pxr/pxr.h"

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

  #include "pxr/external/boost/python.hpp"
  #include "pxr/external/boost/python/stl_iterator.hpp"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_boost::python;
using namespace noodles;

namespace {

// only support usd24 and above

GfVec2d _NoodlesToGfVec2d(const Vec2d& v) {
  return GfVec2d(v[0], v[1]);
}

Vec2d _GfToNoodlesVec2d(const GfVec2d& gf) {
  return Vec2d(gf[0], gf[1]);
}

// Wrapper to return NodeVertex::pack() as Python bytes instead of
// std::vector<uint8_t> (which has no registered Boost.Python converter).
object _NodeVertex_pack(const NodeVertex& self) {
  auto data = self.pack();
  return object(
      handle<>(PyBytes_FromStringAndSize(
          reinterpret_cast<const char*>(data.data()), static_cast<Py_ssize_t>(data.size()))));
}

list _GenerateDefaultNodeVertices(
    const GfVec2d& nodePos,
    const GfVec2d& nodeSize,
    float depth,
    float titleHeight,
    int bgHigh,
    int bgLow,
    int bgAlpha,
    int bgHighShadow,
    int bgLowShadow,
    int selectedBgHigh,
    int selectedBgLow,
    int selectedBgHighShadow,
    int selectedBgLowShadow,
    int titleR,
    int titleG,
    int titleB,
    int titleRShadow,
    int titleGShadow,
    int titleBShadow,
    int selectedTitleR,
    int selectedTitleG,
    int selectedTitleB,
    int selectedTitleRShadow,
    int selectedTitleGShadow,
    int selectedTitleBShadow,
    float innerStroke,
    float selectedFlag) {
  DefaultNodeColors colors{
      bgHigh,
      bgLow,
      bgAlpha,
      bgHighShadow,
      bgLowShadow,
      selectedBgHigh,
      selectedBgLow,
      selectedBgHighShadow,
      selectedBgLowShadow,
      titleR,
      titleG,
      titleB,
      titleRShadow,
      titleGShadow,
      titleBShadow,
      selectedTitleR,
      selectedTitleG,
      selectedTitleB,
      selectedTitleRShadow,
      selectedTitleGShadow,
      selectedTitleBShadow};

  auto result = VertexGenerator::generateDefaultNodeVertices(
      _GfToNoodlesVec2d(nodePos),
      _GfToNoodlesVec2d(nodeSize),
      depth,
      titleHeight,
      colors,
      innerStroke,
      selectedFlag);

  list pyResult;
  for (const auto& v : result) {
    pyResult.append(v);
  }
  return pyResult;
}

list _GenerateGraffiNodeVertices(
    const GfVec2d& nodePos,
    const GfVec2d& nodeSize,
    float depth,
    float titleHeight,
    int bodyR,
    int bodyG,
    int bodyB,
    int bodyAlpha,
    int headerR,
    int headerG,
    int headerB,
    int headerAlpha,
    int selectedBodyR,
    int selectedBodyG,
    int selectedBodyB,
    int selectedHeaderR,
    int selectedHeaderG,
    int selectedHeaderB,
    const list& pyInputPorts,
    const list& pyOutputPorts,
    float portRadius,
    float bodyStroke,
    float selectedFlag) {
  std::vector<PortInfo> inputPorts;
  stl_input_iterator<PortInfo> iBegin(pyInputPorts), iEnd;
  inputPorts.assign(iBegin, iEnd);

  std::vector<PortInfo> outputPorts;
  stl_input_iterator<PortInfo> oBegin(pyOutputPorts), oEnd;
  outputPorts.assign(oBegin, oEnd);

  auto result = VertexGenerator::generateGraffiNodeVertices(
      _GfToNoodlesVec2d(nodePos),
      _GfToNoodlesVec2d(nodeSize),
      depth,
      titleHeight,
      GraffiNodeColors{
          bodyR,
          bodyG,
          bodyB,
          bodyAlpha,
          headerR,
          headerG,
          headerB,
          headerAlpha,
          selectedBodyR,
          selectedBodyG,
          selectedBodyB,
          selectedHeaderR,
          selectedHeaderG,
          selectedHeaderB},
      inputPorts,
      outputPorts,
      portRadius,
      bodyStroke,
      selectedFlag);

  list pyResult;
  for (const auto& v : result) {
    pyResult.append(v);
  }
  return pyResult;
}

list _GraphNodeRenderer_generateVertices(
    GraphNodeRenderer& self,
    NodeData& node,
    float depth,
    const FontAtlas& fontAtlas) {
  auto vertices = self.generateVertices(node, depth, fontAtlas);
  list result;
  for (const auto& v : vertices) {
    result.append(v);
  }
  return result;
}

list _GraphNodeRenderer_getStrokeColor(const GraphNodeRenderer& self, const NodeData& node) {
  auto sc = self.getStrokeColor(node);
  list result;
  for (int i = 0; i < 4; ++i) {
    result.append(sc[i]);
  }
  return result;
}

dict _PortHitResult_to_dict(const GraphNodeRenderer::PortHitResult& result) {
  dict d;
  d["portName"] = result.portName;
  d["isOutput"] = result.isOutput;
  d["found"] = result.found;
  return d;
}

dict _GraphNodeRenderer_getPortAtPoint(
    const GraphNodeRenderer& self,
    const NodeData& node,
    const GfVec2d& worldPos,
    const FontAtlas& fontAtlas,
    double hitRadiusMultiplier) {
  auto result =
      self.getPortAtPoint(node, _GfToNoodlesVec2d(worldPos), fontAtlas, hitRadiusMultiplier);
  return _PortHitResult_to_dict(result);
}

dict _GraphNodeRenderer_getGroupHeaderAtPoint(
    const GraphNodeRenderer& self,
    const NodeData& node,
    const GfVec2d& worldPos,
    const FontAtlas& fontAtlas) {
  auto result = self.getGroupHeaderAtPoint(node, _GfToNoodlesVec2d(worldPos), fontAtlas);
  return _PortHitResult_to_dict(result);
}

GfVec4f _HsvToRgb(float h, float s, float v, float a) {
  auto result = VertexGenerator::hsvToRgb(h, s, v, a);
  return GfVec4f(result[0], result[1], result[2], result[3]);
}

GfVec2d _PortInfo_getPosition(const PortInfo& p) {
  return _NoodlesToGfVec2d(p.position);
}
void _PortInfo_setPosition(PortInfo& p, const GfVec2d& v) {
  p.position = _GfToNoodlesVec2d(v);
}

GfVec2d _GraphNodeRenderer_getPortPosition(
    const GraphNodeRenderer& self,
    const NodeData& node,
    const std::string& portName,
    bool isOutput,
    const FontAtlas& fontAtlas) {
  return _NoodlesToGfVec2d(self.getPortPosition(node, portName, isOutput, fontAtlas));
}

GfVec2d _GraphNodeRenderer_resolvePortPosition(
    const GraphNodeRenderer& self,
    const NodeData& node,
    const std::string& portName,
    bool isOutput) {
  return _NoodlesToGfVec2d(self.resolvePortPosition(node, portName, isOutput));
}

} // anonymous namespace

void wrapRendering() {
// only support usd24 and above
  class_<NodeVertex>("NodeVertex", init<>())
      .def(
          init<
              float,
              float,
              float,
              float,
              float,
              float,
              float,
              uint8_t,
              uint8_t,
              uint8_t,
              uint8_t,
              float,
              uint8_t,
              uint8_t,
              uint8_t,
              uint8_t,
              float>())
      .def_readwrite("x", &NodeVertex::x)
      .def_readwrite("y", &NodeVertex::y)
      .def_readwrite("z", &NodeVertex::z)
      .def_readwrite("u", &NodeVertex::u)
      .def_readwrite("v", &NodeVertex::v)
      .def_readwrite("w", &NodeVertex::w)
      .def_readwrite("h", &NodeVertex::h)
      .def_readwrite("r", &NodeVertex::r)
      .def_readwrite("g", &NodeVertex::g)
      .def_readwrite("b", &NodeVertex::b)
      .def_readwrite("a", &NodeVertex::a)
      .def_readwrite("innerStroke", &NodeVertex::innerStroke)
      .def_readwrite("sr", &NodeVertex::sr)
      .def_readwrite("sg", &NodeVertex::sg)
      .def_readwrite("sb", &NodeVertex::sb)
      .def_readwrite("sa", &NodeVertex::sa)
      .def_readwrite("selected", &NodeVertex::selected)
      .def_readwrite("nodeIndex", &NodeVertex::nodeIndex)
      .def("pack", &_NodeVertex_pack)
      .def("size", &NodeVertex::size)
      .staticmethod("size");

  class_<VertexGenerator>("VertexGenerator")
      .def("generateDefaultNodeVertices", &_GenerateDefaultNodeVertices)
      .staticmethod("generateDefaultNodeVertices")
      .def("generateGraffiNodeVertices", &_GenerateGraffiNodeVertices)
      .staticmethod("generateGraffiNodeVertices")
      .def("hsvToRgb", &_HsvToRgb, (arg("h"), arg("s"), arg("v"), arg("a") = 1.0f))
      .staticmethod("hsvToRgb");

  class_<PortInfo>("PortInfo", init<>())
      .add_property("position", &_PortInfo_getPosition, &_PortInfo_setPosition)
      .def_readwrite("r", &PortInfo::r)
      .def_readwrite("g", &PortInfo::g)
      .def_readwrite("b", &PortInfo::b)
      .def_readwrite("a", &PortInfo::a);

  class_<GraphNodeRenderer, noncopyable>(
      "GraphNodeRenderer", "Base class for node renderers.", no_init)
      .def("getCornerRadius", &GraphNodeRenderer::getCornerRadius, (arg("node")))
      .def("getSelectedStrokeWidth", &GraphNodeRenderer::getSelectedStrokeWidth)
      .def("getUnselectedStrokeWidth", &GraphNodeRenderer::getUnselectedStrokeWidth)
      .def("getPortFontSize", &GraphNodeRenderer::getPortFontSize)
      .def("getPortMarginH", &GraphNodeRenderer::getPortMarginH)
      .def("getPortMarginV", &GraphNodeRenderer::getPortMarginV)
      .def("getPortSpacing", &GraphNodeRenderer::getPortSpacing)
      .def("getPortWidth", &GraphNodeRenderer::getPortWidth)
      .def("getTitleFontSize", &GraphNodeRenderer::getTitleFontSize)
      .def("getPortTypeFontSize", &GraphNodeRenderer::getPortTypeFontSize)
      .def("isGraffiStyle", &GraphNodeRenderer::isGraffiStyle)
      .def(
          "generateVertices",
          &_GraphNodeRenderer_generateVertices,
          (arg("node"), arg("depth"), arg("fontAtlas")))
      .def("getStrokeColor", &_GraphNodeRenderer_getStrokeColor, (arg("node")))
      .def(
          "getPortAtPoint",
          &_GraphNodeRenderer_getPortAtPoint,
          (arg("node"), arg("worldPos"), arg("fontAtlas"), arg("hitRadiusMultiplier") = 1.5))
      .def(
          "getGroupHeaderAtPoint",
          &_GraphNodeRenderer_getGroupHeaderAtPoint,
          (arg("node"), arg("worldPos"), arg("fontAtlas")))
      .def(
          "getPortPosition",
          &_GraphNodeRenderer_getPortPosition,
          (arg("node"), arg("portName"), arg("isOutput"), arg("fontAtlas")))
      .def(
          "resolvePortPosition",
          &_GraphNodeRenderer_resolvePortPosition,
          (arg("node"), arg("portName"), arg("isOutput")));

  class_<DefaultNodeRenderer, bases<GraphNodeRenderer>>(
      "DefaultNodeRenderer", "Default gradient-style node renderer.", init<>())
      .def(init<const RenderConfig&>());

  class_<GraffiNodeRenderer, bases<GraphNodeRenderer>>(
      "GraffiNodeRenderer", "Graffi flat dark-style node renderer.", init<>())
      .def(init<const RenderConfig&>());

  // Profiler
  class_<ProfileStats>("ProfileStats", "Profile timing statistics.", init<>())
      .def_readwrite("total", &ProfileStats::total)
      .def_readwrite("count", &ProfileStats::count)
      .def_readwrite("minVal", &ProfileStats::minVal)
      .def_readwrite("maxVal", &ProfileStats::maxVal)
      .def("average", &ProfileStats::average);

  class_<DummyContext>("DummyContext", "No-op profiling context.", init<>());

  class_<RenderProfiler>("RenderProfiler", "Render performance profiler.", init<>())
      .def(init<int>())
      .def("startFrame", &RenderProfiler::startFrame)
      .def("endFrame", &RenderProfiler::endFrame)
      .def("recordTime", &RenderProfiler::recordTime, (arg("name"), arg("elapsedMs")))
      .def("getStats", &RenderProfiler::getStats)
      .def("printStats", &RenderProfiler::printStats)
      .def("reset", &RenderProfiler::reset)
      .def_readwrite("enabled", &RenderProfiler::enabled);
}
