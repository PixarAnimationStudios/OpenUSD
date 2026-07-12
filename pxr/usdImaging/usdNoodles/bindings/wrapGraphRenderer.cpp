//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "render/GraphRenderer.h"
#include "pxr/pxr.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

  #include "pxr/external/boost/python.hpp"
  #include "pxr/external/boost/python/noncopyable.hpp"

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_boost::python;
using namespace noodles;

namespace {

void _SetInnerStrokeColor(RenderParams& self, const list& pyColor) {
  for (int i = 0; i < 4; ++i) {
    self.innerStrokeColor[i] = extract<float>(pyColor[i]);
  }
}

list _GetInnerStrokeColor(const RenderParams& self) {
  list result;
  for (int i = 0; i < 4; ++i) {
    result.append(self.innerStrokeColor[i]);
  }
  return result;
}

} // anonymous namespace

void wrapGraphRenderer() {
  class_<RenderParams>("RenderParams", "Parameters for a single render frame.", init<>())
      .def_readwrite("zoom", &RenderParams::zoom)
      .def_readwrite("panX", &RenderParams::panX)
      .def_readwrite("panY", &RenderParams::panY)
      .def_readwrite("viewportWidth", &RenderParams::viewportWidth)
      .def_readwrite("viewportHeight", &RenderParams::viewportHeight)
      .def_readwrite("cornerRadius", &RenderParams::cornerRadius)
      .def_readwrite("linkDimming", &RenderParams::linkDimming)
      .def_readwrite("drawNodes", &RenderParams::drawNodes)
      .def_readwrite("drawLinks", &RenderParams::drawLinks)
      .def_readwrite("drawText", &RenderParams::drawText)
      .def_readwrite("drawStickers", &RenderParams::drawStickers)
      .def_readwrite("textChanged", &RenderParams::textChanged)
      .def_readwrite("nodeStrokeColor", &RenderParams::nodeStrokeColor)
      .def_readwrite("nodeGeneration", &RenderParams::nodeGeneration)
      .def_readwrite("nodeSelectionChanged", &RenderParams::nodeSelectionChanged)
      .def_readwrite("renderConfig", &RenderParams::renderConfig)
      .add_property("innerStrokeColor", &_GetInnerStrokeColor, &_SetInnerStrokeColor);

  class_<GraphRenderer, noncopyable>(
      "GraphRenderer", "Unified entry point for graph rendering.", init<>())
      .def(
          "initialize",
          &GraphRenderer::initialize,
          (arg("assetsPath"), arg("fontAtlasPath"), arg("fontMetricsPath")))
      .def("render", &GraphRenderer::render, (arg("graph"), arg("params")))
      .def("invalidateNode", &GraphRenderer::invalidateNode, (arg("nodeId")))
      .def("invalidateAll", &GraphRenderer::invalidateAll)
      .def("cleanup", &GraphRenderer::cleanup)
      .def("shaderLibrary", &GraphRenderer::shaderLibrary, return_internal_reference<>())
      .def("fontAtlas", &GraphRenderer::fontAtlas, return_internal_reference<>())
      .def("nodeManager", &GraphRenderer::nodeManager, return_internal_reference<>())
      .def("linkManager", &GraphRenderer::linkManager, return_internal_reference<>())
      .def("textManager", &GraphRenderer::textManager, return_internal_reference<>())
      .def("stickerRenderer", &GraphRenderer::stickerRenderer, return_internal_reference<>());
}
