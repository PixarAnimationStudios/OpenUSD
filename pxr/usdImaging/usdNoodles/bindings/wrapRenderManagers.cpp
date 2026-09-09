//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "render/FontAtlas.h"
#include "render/IconRenderManager.h"
#include "render/LinkRenderManager.h"
#include "render/NodeRenderManager.h"
#include "render/NodeTransformFrame.h"
#include "render/ShaderLibrary.h"
#include "render/StickerRenderer.h"
#include "render/TextRenderManager.h"
#include "pxr/pxr.h"

#include <array>

  #include "pxr/external/boost/python.hpp"
  #include "pxr/external/boost/python/noncopyable.hpp"
  #include "pxr/external/boost/python/stl_iterator.hpp"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_boost::python;
using namespace noodles;

namespace {

void _ShaderLibrary_initialize(ShaderLibrary& self, const std::string& path) {
  self.initialize(path);
}

void _FontAtlas_initialize(
    FontAtlas& self,
    const std::string& atlasPath,
    const std::string& jsonPath) {
  self.initialize(atlasPath, jsonPath);
}

dict _FontAtlas_glyphs(const FontAtlas& self) {
  dict result;
  for (const auto& pair : self.glyphs()) {
    const Glyph& g = pair.second;
    object pyGlyph = object(g);
    result[pair.first] = pyGlyph;
  }
  return result;
}

unsigned int _FontAtlas_textureId(const FontAtlas& self) {
  return static_cast<unsigned int>(self.textureId());
}

void _NodeRenderManager_initialize(NodeRenderManager& self, ShaderLibrary& shaders) {
  self.initialize(&shaders);
}

void _NodeRenderManager_renderNodes(
    NodeRenderManager& self,
    const list& pyVertices,
    uint32_t strokeColor,
    uint32_t generation,
    bool selectionChanged,
    const list& pyProjection,
    float cornerRadius,
    const list& pyStrokeColor) {
  if (len(pyProjection) < 16 || len(pyStrokeColor) < 4) {
    return;
  }

  std::vector<NodeVertex> vertices;
  stl_input_iterator<NodeVertex> begin(pyVertices), end;
  vertices.assign(begin, end);

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 4> sc{};
  for (int i = 0; i < 4; ++i) {
    sc[i] = extract<float>(pyStrokeColor[i]);
  }

  self.renderNodes(
      vertices, strokeColor, generation, selectionChanged, proj.data(), cornerRadius, sc);
}

// Live entry point for the C++-owned node-quad path: generate and draw node
// background quads from the authoritative GraphModel::nodes snapshot (populated
// by GraphModel::syncNodesFromModels on change). renderNodesFromGraph self-gates
// regeneration on contentChanged / needsNodeRebuild(); moves are applied via the
// transform texture. graph is the live NodeGraph upcast to its GraphModel base.
void _NodeRenderManager_renderNodesFromGraph(
    NodeRenderManager& self,
    GraphModel& graph,
    const RenderConfig& config,
    FontAtlas& fontAtlas,
    const list& pyProjection,
    float cornerRadius,
    const list& pyInnerStrokeColor,
    bool contentChanged) {
  if (len(pyProjection) < 16 || len(pyInnerStrokeColor) < 4) {
    return;
  }

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 4> innerStroke{};
  for (int i = 0; i < 4; ++i) {
    innerStroke[i] = extract<float>(pyInnerStrokeColor[i]);
  }

  self.renderNodesFromGraph(
      graph, config, fontAtlas, proj.data(), cornerRadius, innerStroke, contentChanged);
}

// Live entry point for the C++-owned port-highlight path: build + draw the port
// circles and relationship triangles from the authoritative GraphModel snapshot.
// Hover / drag interaction state is passed as scalars (constructed into a
// PortHighlightState here) so no struct needs binding. graph is the live
// NodeGraph upcast to its GraphModel base.
void _NodeRenderManager_renderPortHighlightsFromGraph(
    NodeRenderManager& self,
    GraphModel& graph,
    const RenderConfig& config,
    const list& pyProjection,
    float cornerRadius,
    bool hasHover,
    const std::string& hoveredNodeId,
    const std::string& hoveredPort,
    bool hoveredIsOutput,
    bool draggingLink,
    const std::string& dragSourceNodeId,
    const std::string& dragSourcePort,
    bool dragSourceIsOutput,
    bool geometryChanged) {
  if (len(pyProjection) < 16) {
    return;
  }
  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }
  PortHighlightState state;
  state.hasHover = hasHover;
  state.hoveredNodeId = hoveredNodeId;
  state.hoveredPort = hoveredPort;
  state.hoveredIsOutput = hoveredIsOutput;
  state.draggingLink = draggingLink;
  state.dragSourceNodeId = dragSourceNodeId;
  state.dragSourcePort = dragSourcePort;
  state.dragSourceIsOutput = dragSourceIsOutput;
  self.renderPortHighlightsFromGraph(
      graph, config, state, proj.data(), cornerRadius, geometryChanged);
}

// Port hit-test entry point: find the closest port under a world-space cursor in
// the authoritative GraphModel snapshot, returned as a Python dict so no struct
// needs binding. Mirrors the old getPortAtPoint dict shape (found / portName /
// isOutput) plus the nodeId, since this single call covers every node's ports
// (the Python hover loop no longer iterates candidate nodes itself).
dict _NodeRenderManager_portAtPoint(
    NodeRenderManager& self,
    GraphModel& graph,
    double worldX,
    double worldY,
    double hitRadius) {
  NodeRenderManager::PortHit hit =
      self.portAtPointFromGraph(graph, Vec2d(worldX, worldY), hitRadius);
  dict result;
  result["found"] = hit.found;
  result["nodeId"] = hit.nodeId;
  result["portName"] = hit.portName;
  result["isOutput"] = hit.isOutput;
  return result;
}

void _NodeRenderManager_renderPortHighlight(
    NodeRenderManager& self,
    float portX,
    float portY,
    float portW,
    float portH,
    float depth,
    const list& pyProjection,
    float cornerRadius,
    const list& pyHighlightColor) {
  if (len(pyProjection) < 16 || len(pyHighlightColor) < 4) {
    return;
  }

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 4> color{};
  for (int i = 0; i < 4; ++i) {
    color[i] = extract<float>(pyHighlightColor[i]);
  }

  self.renderPortHighlight(portX, portY, portW, portH, depth, proj.data(), cornerRadius, color);
}

void _NodeRenderManager_renderEndpointCircle(
    NodeRenderManager& self,
    float cx,
    float cy,
    float radius,
    float depth,
    const list& pyProjection,
    const list& pyFillColor,
    const list& pyStrokeColor,
    float strokeWidth) {
  if (len(pyProjection) < 16 || len(pyFillColor) < 4 || len(pyStrokeColor) < 4) {
    return;
  }

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 4> fill{};
  std::array<float, 4> stroke{};
  for (int i = 0; i < 4; ++i) {
    fill[i] = extract<float>(pyFillColor[i]);
    stroke[i] = extract<float>(pyStrokeColor[i]);
  }

  self.renderEndpointCircle(cx, cy, radius, depth, proj.data(), fill, stroke, strokeWidth);
}

void _NodeRenderManager_renderEndpointTriangle(
    NodeRenderManager& self,
    float x0,
    float y0,
    float x1,
    float y1,
    float x2,
    float y2,
    float depth,
    const list& pyProjection,
    const list& pyColor) {
  if (len(pyProjection) < 16 || len(pyColor) < 4) {
    return;
  }

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 4> color{};
  for (int i = 0; i < 4; ++i) {
    color[i] = extract<float>(pyColor[i]);
  }

  self.renderEndpointTriangle(x0, y0, x1, y1, x2, y2, depth, proj.data(), color);
}

void _LinkRenderManager_initialize(LinkRenderManager& self, ShaderLibrary& shaders) {
  self.initialize(&shaders);
}

void _LinkRenderManager_renderLinks(
    LinkRenderManager& self,
    const list& pyLinks,
    const list& pyProjection,
    const RenderConfig& config,
    float zoom,
    float panX,
    float panY,
    float viewportWidth,
    float viewportHeight,
    float dimming,
    bool drawSelected,
    const list& pyBaseColor,
    const list& pySelectedColor,
    const list& pyHoveredColor,
    const list& pyHighlightedColor,
    int cacheKey) {
  if (len(pyProjection) < 16) {
    return;
  }

  std::vector<LinkData> links;
  stl_input_iterator<LinkData> begin(pyLinks), end;
  links.assign(begin, end);

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 3> baseColor{0.45f, 0.46f, 0.47f}; // default gray
  if (len(pyBaseColor) >= 3) {
    baseColor[0] = extract<float>(pyBaseColor[0]);
    baseColor[1] = extract<float>(pyBaseColor[1]);
    baseColor[2] = extract<float>(pyBaseColor[2]);
  }

  std::array<float, 3> selectedColor{1.0f, 1.0f, 0.3f}; // default yellow
  if (len(pySelectedColor) >= 3) {
    selectedColor[0] = extract<float>(pySelectedColor[0]);
    selectedColor[1] = extract<float>(pySelectedColor[1]);
    selectedColor[2] = extract<float>(pySelectedColor[2]);
  }

  std::array<float, 3> hoveredColor{1.0f, 1.0f, 0.0f}; // default yellow
  if (len(pyHoveredColor) >= 3) {
    hoveredColor[0] = extract<float>(pyHoveredColor[0]);
    hoveredColor[1] = extract<float>(pyHoveredColor[1]);
    hoveredColor[2] = extract<float>(pyHoveredColor[2]);
  }

  std::array<float, 3> highlightedColor{0.31f, 0.78f, 0.47f}; // Presto green
  if (len(pyHighlightedColor) >= 3) {
    highlightedColor[0] = extract<float>(pyHighlightedColor[0]);
    highlightedColor[1] = extract<float>(pyHighlightedColor[1]);
    highlightedColor[2] = extract<float>(pyHighlightedColor[2]);
  }

  self.renderLinks(
      links,
      proj.data(),
      config,
      zoom,
      panX,
      panY,
      viewportWidth,
      viewportHeight,
      dimming,
      drawSelected,
      baseColor.data(),
      selectedColor.data(),
      hoveredColor.data(),
      highlightedColor.data(),
      cacheKey);
}

// Render links from the held GraphModel.links snapshot (no Python list
// marshalling — graph is the live NodeGraph upcast to its GraphModel base).
void _LinkRenderManager_renderLinksFromGraph(
    LinkRenderManager& self,
    GraphModel& graph,
    const list& pyProjection,
    const RenderConfig& config,
    float zoom,
    float panX,
    float panY,
    float viewportWidth,
    float viewportHeight,
    float dimming,
    bool drawSelected,
    bool relationshipOnly,
    const list& pyBaseColor,
    const list& pySelectedColor,
    const list& pyHoveredColor,
    const list& pyHighlightedColor,
    int cacheKey) {
  if (len(pyProjection) < 16) {
    return;
  }

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 3> baseColor{0.45f, 0.46f, 0.47f}; // default gray
  if (len(pyBaseColor) >= 3) {
    baseColor[0] = extract<float>(pyBaseColor[0]);
    baseColor[1] = extract<float>(pyBaseColor[1]);
    baseColor[2] = extract<float>(pyBaseColor[2]);
  }

  std::array<float, 3> selectedColor{1.0f, 1.0f, 0.3f}; // default yellow
  if (len(pySelectedColor) >= 3) {
    selectedColor[0] = extract<float>(pySelectedColor[0]);
    selectedColor[1] = extract<float>(pySelectedColor[1]);
    selectedColor[2] = extract<float>(pySelectedColor[2]);
  }

  std::array<float, 3> hoveredColor{1.0f, 1.0f, 0.0f}; // default yellow
  if (len(pyHoveredColor) >= 3) {
    hoveredColor[0] = extract<float>(pyHoveredColor[0]);
    hoveredColor[1] = extract<float>(pyHoveredColor[1]);
    hoveredColor[2] = extract<float>(pyHoveredColor[2]);
  }

  std::array<float, 3> highlightedColor{0.31f, 0.78f, 0.47f}; // Presto green
  if (len(pyHighlightedColor) >= 3) {
    highlightedColor[0] = extract<float>(pyHighlightedColor[0]);
    highlightedColor[1] = extract<float>(pyHighlightedColor[1]);
    highlightedColor[2] = extract<float>(pyHighlightedColor[2]);
  }

  self.renderLinksFromGraph(
      graph,
      proj.data(),
      config,
      zoom,
      panX,
      panY,
      viewportWidth,
      viewportHeight,
      dimming,
      drawSelected,
      relationshipOnly,
      baseColor.data(),
      selectedColor.data(),
      hoveredColor.data(),
      highlightedColor.data(),
      cacheKey);
}

void _TextRenderManager_initialize(
    TextRenderManager& self,
    ShaderLibrary& shaders,
    FontAtlas& fontAtlas) {
  self.initialize(&shaders, &fontAtlas);
}

double _TextRenderManager_calculateTextWidth(
    const TextRenderManager& self,
    const std::string& text,
    double fontSize) {
  return self.calculateTextWidth(text, fontSize);
}

tuple _TextRenderManager_generateTextVertices(
    const TextRenderManager& self,
    const std::string& text,
    double cursorX,
    double cursorY,
    float depth,
    double scale,
    list& pyVertexData,
    float nodeIndex) {
  std::vector<float> vertexData;
  for (int i = 0; i < len(pyVertexData); ++i) {
    vertexData.push_back(extract<float>(pyVertexData[i]));
  }

  auto [finalCursorX, charCount] =
      self.generateTextVertices(text, cursorX, cursorY, depth, scale, vertexData, nodeIndex);

  // Update pyVertexData in place
  while (len(pyVertexData) > 0) {
    pyVertexData.pop();
  }
  for (float val : vertexData) {
    pyVertexData.append(val);
  }

  return make_tuple(finalCursorX, charCount);
}

void _TextRenderManager_renderText(
    TextRenderManager& self,
    const list& pyVertexData,
    const list& pyProjection,
    const list& pyColor) {
  if (len(pyProjection) < 16 || len(pyColor) < 4) {
    return;
  }

  std::vector<float> vertexData;
  for (int i = 0; i < len(pyVertexData); ++i) {
    vertexData.push_back(extract<float>(pyVertexData[i]));
  }

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 4> color{};
  for (int i = 0; i < 4; ++i) {
    color[i] = extract<float>(pyColor[i]);
  }

  self.renderText(vertexData, proj.data(), color);
}

void _TextRenderManager_renderNodeText(
    TextRenderManager& self,
    const list& pyVertexData,
    const list& pyProjection,
    const list& pyColor,
    bool needsFullUpdate) {
  if (len(pyProjection) < 16 || len(pyColor) < 4) {
    return;
  }

  std::vector<float> vertexData;
  for (int i = 0; i < len(pyVertexData); ++i) {
    vertexData.push_back(extract<float>(pyVertexData[i]));
  }

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 4> color{};
  for (int i = 0; i < 4; ++i) {
    color[i] = extract<float>(pyColor[i]);
  }

  self.renderNodeText(vertexData, proj.data(), color, needsFullUpdate);
}

// Live entry point for the C++-owned text path: render from the authoritative
// NodeData snapshot held in GraphModel::nodes (populated by
// GraphModel::syncNodesFromModels on change). No per-frame marshalling here;
// renderNodeTextFull self-gates the actual re-layout on textChanged /
// nodeTextNeedsRebuild_. graph is the live NodeGraph upcast to its GraphModel base.
bool _TextRenderManager_renderNodeTextFromGraph(
    TextRenderManager& self,
    GraphModel& graph,
    const list& pyProjection,
    float zoom,
    bool textChanged,
    const RenderConfig& config,
    double panX,
    double panY,
    double viewportWidth,
    double viewportHeight,
    bool cullOffscreen) {
  if (len(pyProjection) < 16) {
    return false;
  }
  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }
  return self.renderNodeTextFull(
      graph.nodes,
      proj.data(),
      zoom,
      textChanged,
      config,
      panX,
      panY,
      viewportWidth,
      viewportHeight,
      cullOffscreen);
}

void _IconRenderManager_initialize(
    IconRenderManager& self,
    ShaderLibrary& shaders,
    const std::string& assetsPath) {
  self.initialize(&shaders, assetsPath);
}

// Live entry point for the C++-owned icon path: draw the title + row icons from
// the authoritative NodeData snapshot held in GraphModel::nodes. No per-frame
// marshalling here; renderIconsFromGraph self-gates the rebuild on contentChanged
// / markIconsDirty. graph is the live NodeGraph upcast to its GraphModel base.
// Nothing in Python calls this yet (the cutover is the next diff, behind a flag).
bool _IconRenderManager_renderIconsFromGraph(
    IconRenderManager& self,
    GraphModel& graph,
    const list& pyProjection,
    float zoom,
    bool contentChanged,
    const RenderConfig& config,
    double panX,
    double panY,
    double viewportWidth,
    double viewportHeight,
    bool cullOffscreen) {
  if (len(pyProjection) < 16) {
    return false;
  }
  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }
  return self.renderIconsFromGraph(
      graph.nodes,
      proj.data(),
      zoom,
      contentChanged,
      config,
      panX,
      panY,
      viewportWidth,
      viewportHeight,
      cullOffscreen);
}

void _StickerRenderer_initialize(StickerRenderer& self, ShaderLibrary& shaders) {
  self.initialize(&shaders);
}

void _StickerRenderer_renderStickers(
    StickerRenderer& self,
    const list& pyVertices,
    const list& pyProjection,
    float cornerRadius,
    const list& pyStrokeColor) {
  if (len(pyProjection) < 16 || len(pyStrokeColor) < 4) {
    return;
  }

  std::vector<NodeVertex> vertices;
  stl_input_iterator<NodeVertex> begin(pyVertices), end;
  vertices.assign(begin, end);

  std::array<float, 16> proj{};
  for (int i = 0; i < 16; ++i) {
    proj[i] = extract<float>(pyProjection[i]);
  }

  std::array<float, 4> sc{};
  for (int i = 0; i < 4; ++i) {
    sc[i] = extract<float>(pyStrokeColor[i]);
  }

  self.renderStickers(vertices, proj.data(), cornerRadius, sc);
}

} // anonymous namespace

void wrapRenderManagers() {
  class_<GLSLProgram, noncopyable>("GLSLProgram", no_init)
      .def("use", &GLSLProgram::use)
      .def("release", &GLSLProgram::release)
      .def("getUniformLocation", &GLSLProgram::getUniformLocation, (arg("name")));

  // Shared node-local coordinate frame (base positions + move offsets + the
  // transform texture) used by both the text and node-quad render paths.
  class_<NodeTransformFrame, noncopyable>(
      "NodeTransformFrame",
      "Shared per-node move offsets + transform texture for the render path.",
      init<>())
      .def("translate", &NodeTransformFrame::translate, (arg("nodeId"), arg("dx"), arg("dy")))
      .def("reset", &NodeTransformFrame::reset)
      .def("cleanup", &NodeTransformFrame::cleanup);

  class_<ShaderLibrary, noncopyable>("ShaderLibrary", "Manages GLSL shader programs.", init<>())
      .def("initialize", &_ShaderLibrary_initialize, (arg("assetsPath")))
      .def(
          "get",
          &ShaderLibrary::get,
          (arg("name")),
          return_value_policy<reference_existing_object>())
      .def("cleanup", &ShaderLibrary::cleanup);

  class_<FontAtlas, noncopyable>("FontAtlas", "MSDF font atlas for text rendering.", init<>())
      .def("initialize", &_FontAtlas_initialize, (arg("atlasPath"), arg("jsonPath")))
      .def("bindTexture", &FontAtlas::bindTexture, (arg("textureUnit") = 0))
      .def("unbindTexture", &FontAtlas::unbindTexture)
      .def("cleanup", &FontAtlas::cleanup)
      .add_property("pxRange", &FontAtlas::pxRange)
      .add_property("lineHeight", &FontAtlas::lineHeight)
      .add_property("ascender", &FontAtlas::ascender)
      .add_property("descender", &FontAtlas::descender)
      .add_property("texture", &_FontAtlas_textureId)
      .def("getGlyphs", &_FontAtlas_glyphs);

  class_<NodeRenderManager, noncopyable>(
      "NodeRenderManager", "Manages node VBO batching and rendering.", init<>())
      .def("initialize", &_NodeRenderManager_initialize, (arg("shaders")))
      .def("setTransformFrame", &NodeRenderManager::setTransformFrame, (arg("frame")))
      .def(
          "renderNodes",
          &_NodeRenderManager_renderNodes,
          (arg("vertices"),
           arg("strokeColor"),
           arg("generation"),
           arg("selectionChanged"),
           arg("projection"),
           arg("cornerRadius"),
           arg("strokeColorVec")))
      .def(
          "renderPortHighlight",
          &_NodeRenderManager_renderPortHighlight,
          (arg("portX"),
           arg("portY"),
           arg("portW"),
           arg("portH"),
           arg("depth"),
           arg("projection"),
           arg("cornerRadius"),
           arg("highlightColor")))
      .def(
          "renderEndpointCircle",
          &_NodeRenderManager_renderEndpointCircle,
          (arg("cx"),
           arg("cy"),
           arg("radius"),
           arg("depth"),
           arg("projection"),
           arg("fillColor"),
           arg("strokeColor"),
           arg("strokeWidth")))
      .def(
          "renderEndpointTriangle",
          &_NodeRenderManager_renderEndpointTriangle,
          (arg("x0"),
           arg("y0"),
           arg("x1"),
           arg("y1"),
           arg("x2"),
           arg("y2"),
           arg("depth"),
           arg("projection"),
           arg("color")))
      .def(
          "renderNodesFromGraph",
          &_NodeRenderManager_renderNodesFromGraph,
          (arg("graph"),
           arg("config"),
           arg("fontAtlas"),
           arg("projection"),
           arg("cornerRadius"),
           arg("innerStrokeColor"),
           arg("contentChanged")))
      .def(
          "renderPortHighlightsFromGraph",
          &_NodeRenderManager_renderPortHighlightsFromGraph,
          (arg("graph"),
           arg("config"),
           arg("projection"),
           arg("cornerRadius"),
           arg("hasHover"),
           arg("hoveredNodeId"),
           arg("hoveredPort"),
           arg("hoveredIsOutput"),
           arg("draggingLink"),
           arg("dragSourceNodeId"),
           arg("dragSourcePort"),
           arg("dragSourceIsOutput"),
           arg("geometryChanged")))
      .def(
          "portAtPoint",
          &_NodeRenderManager_portAtPoint,
          (arg("graph"), arg("worldX"), arg("worldY"), arg("hitRadius")))
      .def("resetNodeQuadCaches", &NodeRenderManager::resetNodeQuadCaches)
      .def(
          "markNodeQuadDirty", &NodeRenderManager::markNodeQuadDirty, (arg("needsRebuild") = false))
      .def("needsNodeRebuild", &NodeRenderManager::needsNodeRebuild)
      .def("invalidateAll", &NodeRenderManager::invalidateAll)
      .def("invalidateNode", &NodeRenderManager::invalidateNode, (arg("nodeId")))
      .def("clearCache", &NodeRenderManager::clearCache)
      .def("cleanup", &NodeRenderManager::cleanup);

  class_<LinkRenderManager, noncopyable>(
      "LinkRenderManager", "Manages link instanced rendering.", init<>())
      .def("initialize", &_LinkRenderManager_initialize, (arg("shaders")))
      .def(
          "renderLinks",
          &_LinkRenderManager_renderLinks,
          (arg("links"),
           arg("projection"),
           arg("config"),
           arg("zoom"),
           arg("panX"),
           arg("panY"),
           arg("viewportWidth"),
           arg("viewportHeight"),
           arg("dimming"),
           arg("drawSelected"),
           arg("baseColor") = list(),
           arg("selectedColor") = list(),
           arg("hoveredColor") = list(),
           arg("highlightedColor") = list(),
           arg("cacheKey") = 0))
      .def(
          "renderLinksFromGraph",
          &_LinkRenderManager_renderLinksFromGraph,
          (arg("graph"),
           arg("projection"),
           arg("config"),
           arg("zoom"),
           arg("panX"),
           arg("panY"),
           arg("viewportWidth"),
           arg("viewportHeight"),
           arg("dimming"),
           arg("drawSelected"),
           arg("relationshipOnly"),
           arg("baseColor") = list(),
           arg("selectedColor") = list(),
           arg("hoveredColor") = list(),
           arg("highlightedColor") = list(),
           arg("cacheKey") = 0))
      .def("cleanup", &LinkRenderManager::cleanup)
      .def("getCacheHits", &LinkRenderManager::getCacheHits)
      .def("getCacheMisses", &LinkRenderManager::getCacheMisses);

  class_<TextRenderManager, noncopyable>(
      "TextRenderManager", "Manages MSDF text rendering.", init<>())
      .def("initialize", &_TextRenderManager_initialize, (arg("shaders"), arg("fontAtlas")))
      .def("setTransformFrame", &TextRenderManager::setTransformFrame, (arg("frame")))
      .def(
          "calculateTextWidth",
          &_TextRenderManager_calculateTextWidth,
          (arg("text"), arg("fontSize")))
      .def(
          "generateTextVertices",
          &_TextRenderManager_generateTextVertices,
          (arg("text"),
           arg("cursorX"),
           arg("cursorY"),
           arg("depth"),
           arg("scale"),
           arg("vertexData"),
           arg("nodeIndex") = 0.0f))
      .def(
          "renderText",
          &_TextRenderManager_renderText,
          (arg("vertexData"), arg("projection"), arg("color")))
      .def(
          "renderNodeText",
          &_TextRenderManager_renderNodeText,
          (arg("vertexData"), arg("projection"), arg("color"), arg("needsFullUpdate")))
      .def(
          "renderNodeTextFromGraph",
          &_TextRenderManager_renderNodeTextFromGraph,
          (arg("graph"),
           arg("projection"),
           arg("zoom"),
           arg("textChanged"),
           arg("config"),
           arg("panX") = 0.0,
           arg("panY") = 0.0,
           arg("viewportWidth") = 0.0,
           arg("viewportHeight") = 0.0,
           arg("cullOffscreen") = false))
      .def("needsTextRebuild", &TextRenderManager::needsTextRebuild)
      .def("resetPositionCaches", &TextRenderManager::resetPositionCaches)
      .def(
          "patchNodeTextDepth",
          &TextRenderManager::patchNodeTextDepth,
          (arg("nodeId"), arg("zOrder")))
      .def(
          "markNodeTextDirty", &TextRenderManager::markNodeTextDirty, (arg("needsRebuild") = false))
      .def("cleanup", &TextRenderManager::cleanup);

  class_<IconRenderManager, noncopyable>(
      "IconRenderManager", "Manages node title + row icon rendering.", init<>())
      .def("initialize", &_IconRenderManager_initialize, (arg("shaders"), arg("assetsPath")))
      .def("setTransformFrame", &IconRenderManager::setTransformFrame, (arg("frame")))
      .def(
          "renderIconsFromGraph",
          &_IconRenderManager_renderIconsFromGraph,
          (arg("graph"),
           arg("projection"),
           arg("zoom"),
           arg("contentChanged"),
           arg("config"),
           arg("panX") = 0.0,
           arg("panY") = 0.0,
           arg("viewportWidth") = 0.0,
           arg("viewportHeight") = 0.0,
           arg("cullOffscreen") = false))
      .def("markIconsDirty", &IconRenderManager::markIconsDirty)
      .def("needsIconRebuild", &IconRenderManager::needsIconRebuild)
      .def("resetPositionCaches", &IconRenderManager::resetPositionCaches)
      .def("cleanup", &IconRenderManager::cleanup);

  class_<StickerRenderer, noncopyable>(
      "StickerRenderer", "Renders group sticker backgrounds.", init<>())
      .def("initialize", &_StickerRenderer_initialize, (arg("shaders")))
      .def(
          "renderStickers",
          &_StickerRenderer_renderStickers,
          (arg("vertices"), arg("projection"), arg("cornerRadius"), arg("strokeColor")))
      .def("markDirty", &StickerRenderer::markDirty)
      .def("cleanup", &StickerRenderer::cleanup);
}
