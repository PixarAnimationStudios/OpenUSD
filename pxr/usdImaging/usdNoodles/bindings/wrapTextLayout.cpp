//
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the terms set forth in the LICENSE.txt file available
// at the root of this repository.
//

#include "render/TextLayout.h"
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

GfVec2d _NoodlesToGfVec2d(const Vec2d& v) {
  return GfVec2d(v[0], v[1]);
}

Vec2d _GfToNoodlesVec2d(const GfVec2d& gf) {
  return Vec2d(gf[0], gf[1]);
}

bool _HasAttr(const object& obj, const char* attrName) {
  return PyObject_HasAttrString(obj.ptr(), attrName) != 0;
}

// Convert Python dict of glyphs to C++ unordered_map
std::unordered_map<int, GlyphMetrics> _ConvertGlyphMap(const dict& pyGlyphMap) {
  std::unordered_map<int, GlyphMetrics> glyphMap;

  list keys = pyGlyphMap.keys();
  for (int i = 0; i < len(keys); ++i) {
    int unicode = extract<int>(keys[i]);
    object pyGlyph = pyGlyphMap[keys[i]];

    GlyphMetrics glyph;
    glyph.unicode = unicode;

    if (_HasAttr(pyGlyph, "advance")) {
      glyph.advance = extract<double>(pyGlyph.attr("advance"));
    }

    if (_HasAttr(pyGlyph, "planeBounds")) {
      object planeBounds = pyGlyph.attr("planeBounds");
      if (!planeBounds.is_none()) {
        GfVec2d minVal = extract<GfVec2d>(planeBounds.attr("GetMin")());
        GfVec2d maxVal = extract<GfVec2d>(planeBounds.attr("GetMax")());
        glyph.planeBounds = Range2d(Vec2d(minVal[0], minVal[1]), Vec2d(maxVal[0], maxVal[1]));
      }
    }

    if (_HasAttr(pyGlyph, "atlasBounds")) {
      object atlasBounds = pyGlyph.attr("atlasBounds");
      if (!atlasBounds.is_none()) {
        GfVec2d minVal = extract<GfVec2d>(atlasBounds.attr("GetMin")());
        GfVec2d maxVal = extract<GfVec2d>(atlasBounds.attr("GetMax")());
        glyph.atlasBounds = Range2d(Vec2d(minVal[0], minVal[1]), Vec2d(maxVal[0], maxVal[1]));
      }
    }

    glyphMap[unicode] = glyph;
  }

  return glyphMap;
}

std::vector<int> _ConvertIntVector(const list& pyValues) {
  std::vector<int> values;
  stl_input_iterator<int> begin(pyValues), end;
  values.assign(begin, end);
  return values;
}

double _CalculateTextWidth(const std::string& text, double fontSize, const dict& pyGlyphMap) {
  std::unordered_map<int, GlyphMetrics> glyphMap = _ConvertGlyphMap(pyGlyphMap);
  return TextLayout::CalculateTextWidth(text, fontSize, glyphMap);
}

// Returns tuple of (vertexData, cursorX, charCount)
tuple _GenerateTextVertices(
    const std::string& text,
    double cursorX,
    double cursorY,
    double depth,
    double scale,
    const dict& pyGlyphMap,
    double ascender,
    double nodeIndex) {
  std::unordered_map<int, GlyphMetrics> glyphMap = _ConvertGlyphMap(pyGlyphMap);

  std::vector<float> vertexData;
  TextVertexResult result = TextLayout::GenerateTextVertices(
      text, cursorX, cursorY, depth, scale, glyphMap, ascender, nodeIndex, vertexData);

  list pyVertexData;
  for (float v : vertexData) {
    pyVertexData.append(v);
  }

  return make_tuple(pyVertexData, result.cursorX, result.charCount);
}

tuple _GenerateTextVerticesAppend(
    const std::string& text,
    double cursorX,
    double cursorY,
    double depth,
    double scale,
    const dict& pyGlyphMap,
    double ascender,
    double nodeIndex,
    list& pyVertexData) {
  std::unordered_map<int, GlyphMetrics> glyphMap = _ConvertGlyphMap(pyGlyphMap);

  std::vector<float> vertexData;

  // Pre-populate with existing data
  vertexData.reserve(len(pyVertexData));
  for (int i = 0; i < len(pyVertexData); ++i) {
    vertexData.push_back(extract<float>(pyVertexData[i]));
  }

  TextVertexResult result = TextLayout::GenerateTextVertices(
      text, cursorX, cursorY, depth, scale, glyphMap, ascender, nodeIndex, vertexData);

  // Append new vertices to Python list
  size_t startIdx = len(pyVertexData);
  for (size_t i = startIdx; i < vertexData.size(); ++i) {
    pyVertexData.append(vertexData[i]);
  }

  return make_tuple(result.cursorX, result.charCount);
}

list _CalculateTextWidthBatch(const list& pyTexts, double fontSize, const dict& pyGlyphMap) {
  std::unordered_map<int, GlyphMetrics> glyphMap = _ConvertGlyphMap(pyGlyphMap);

  std::vector<std::string> texts;
  stl_input_iterator<std::string> textsBegin(pyTexts), textsEnd;
  texts.assign(textsBegin, textsEnd);

  std::vector<double> widths = TextLayout::CalculateTextWidthBatch(texts, fontSize, glyphMap);

  list pyWidths;
  for (double w : widths) {
    pyWidths.append(w);
  }

  return pyWidths;
}

list _CalculatePortPositions(
    const list& pyInputPins,
    const list& pyOutputPins,
    double fontSize,
    double portSpacing,
    const GfVec2d& nodePosition,
    const GfVec2d& nodeSize,
    const dict& pyGlyphMap,
    double marginH,
    double titleHeight,
    const list& pyInputRowSlots,
    const list& pyOutputRowSlots) {
  std::unordered_map<int, GlyphMetrics> glyphMap = _ConvertGlyphMap(pyGlyphMap);

  std::vector<std::string> inputPins;
  stl_input_iterator<std::string> inputBegin(pyInputPins), inputEnd;
  inputPins.assign(inputBegin, inputEnd);

  std::vector<std::string> outputPins;
  stl_input_iterator<std::string> outputBegin(pyOutputPins), outputEnd;
  outputPins.assign(outputBegin, outputEnd);

  std::vector<int> inputRowSlots = _ConvertIntVector(pyInputRowSlots);
  std::vector<int> outputRowSlots = _ConvertIntVector(pyOutputRowSlots);

  std::vector<Vec2d> positions = TextLayout::CalculatePortPositions(
      inputPins,
      outputPins,
      fontSize,
      portSpacing,
      _GfToNoodlesVec2d(nodePosition),
      _GfToNoodlesVec2d(nodeSize),
      glyphMap,
      marginH,
      titleHeight,
      inputRowSlots,
      outputRowSlots);

  list pyPositions;
  for (const Vec2d& pos : positions) {
    pyPositions.append(_NoodlesToGfVec2d(pos));
  }

  return pyPositions;
}

} // anonymous namespace

void wrapTextLayout() {
  // Note: GlyphMetrics is NOT exposed to Python since it's only used internally
  // The Python side passes glyph dicts that are converted in the wrapper functions

  class_<TextVertexResult>("TextVertexResult", "Result of text vertex generation.", init<>())
      .def(init<double, int>((arg("cursorX"), arg("charCount"))))
      .def_readwrite(
          "cursorX",
          &TextVertexResult::cursorX,
          "Final cursor X position after generating all vertices")
      .def_readwrite(
          "charCount", &TextVertexResult::charCount, "Number of characters successfully rendered");

  class_<TextLayout>(
      "TextLayout",
      "High-performance text layout calculator for node graph rendering.\n\n"
      "Provides optimized text width calculation and vertex generation for\n"
      "MSDF (Multi-channel Signed Distance Field) text rendering.\n",
      no_init)

      .def(
          "calculateTextWidth",
          &_CalculateTextWidth,
          (arg("text"), arg("fontSize"), arg("glyphMap")),
          "Calculate text width in pixels/units.\n\n"
          "Uses glyph advance values to compute total width.\n\n"
          "Args:\n"
          "    text (str): The text string to measure\n"
          "    fontSize (float): The font size (scale factor)\n"
          "    glyphMap (dict): Dict of unicode -> glyph objects with 'advance' attribute\n\n"
          "Returns:\n"
          "    float: Width of the text in the same units as fontSize")
      .staticmethod("calculateTextWidth")

      .def(
          "generateTextVertices",
          &_GenerateTextVertices,
          (arg("text"),
           arg("cursorX"),
           arg("cursorY"),
           arg("depth"),
           arg("scale"),
           arg("glyphMap"),
           arg("ascender"),
           arg("nodeIndex") = 0.0),
          "Generate text vertices for GPU rendering.\n\n"
          "Creates 6 vertices per character (2 triangles per quad).\n\n"
          "Vertex format: x, y, z, s, t, nodeIndex (6 floats per vertex)\n\n"
          "Args:\n"
          "    text (str): The text string to render\n"
          "    cursorX (float): Starting X position\n"
          "    cursorY (float): Starting Y position (baseline)\n"
          "    depth (float): Z-depth for rendering order\n"
          "    scale (float): Font scale (font size)\n"
          "    glyphMap (dict): Dict of unicode -> glyph objects\n"
          "    ascender (float): Font ascender value\n"
          "    nodeIndex (float): Node index for GPU transform lookup (default 0.0)\n\n"
          "Returns:\n"
          "    tuple: (vertexData, cursorX, charCount)")
      .staticmethod("generateTextVertices")

      .def(
          "generateTextVerticesAppend",
          &_GenerateTextVerticesAppend,
          (arg("text"),
           arg("cursorX"),
           arg("cursorY"),
           arg("depth"),
           arg("scale"),
           arg("glyphMap"),
           arg("ascender"),
           arg("nodeIndex"),
           arg("vertexData")),
          "Generate text vertices and append to existing list.\n\n"
          "Same as generateTextVertices but appends to an existing vertex data list.\n\n"
          "Args:\n"
          "    text (str): The text string to render\n"
          "    cursorX (float): Starting X position\n"
          "    cursorY (float): Starting Y position (baseline)\n"
          "    depth (float): Z-depth for rendering order\n"
          "    scale (float): Font scale (font size)\n"
          "    glyphMap (dict): Dict of unicode -> glyph objects\n"
          "    ascender (float): Font ascender value\n"
          "    nodeIndex (float): Node index for GPU transform lookup\n"
          "    vertexData (list): List to append vertex data to\n\n"
          "Returns:\n"
          "    tuple: (cursorX, charCount)")
      .staticmethod("generateTextVerticesAppend")

      .def(
          "calculateTextWidthBatch",
          &_CalculateTextWidthBatch,
          (arg("texts"), arg("fontSize"), arg("glyphMap")),
          "Batch calculate text widths for multiple strings.\n\n"
          "Args:\n"
          "    texts (list): List of text strings to measure\n"
          "    fontSize (float): The font size (scale factor)\n"
          "    glyphMap (dict): Dict of unicode -> glyph objects\n\n"
          "Returns:\n"
          "    list: List of widths in the same order as input texts")
      .staticmethod("calculateTextWidthBatch")

      .def(
          "calculatePortPositions",
          &_CalculatePortPositions,
          (arg("inputPins"),
           arg("outputPins"),
           arg("fontSize"),
           arg("portSpacing"),
           arg("nodePosition"),
           arg("nodeSize"),
           arg("glyphMap"),
           arg("marginH"),
           arg("titleHeight"),
           arg("inputRowSlots") = list(),
           arg("outputRowSlots") = list()),
          "Calculate port positions for a node (batch operation).\n\n"
          "Args:\n"
          "    inputPins (list): List of input pin names\n"
          "    outputPins (list): List of output pin names\n"
          "    fontSize (float): Font size for the port labels\n"
          "    portSpacing (float): Vertical spacing between ports\n"
          "    nodePosition (Gf.Vec2d): Node's top-left position\n"
          "    nodeSize (Gf.Vec2d): Node's width and height\n"
          "    glyphMap (dict): Dict of unicode -> glyph objects\n"
          "    marginH (float): Horizontal margin\n"
          "    titleHeight (float): Height of the title area\n"
          "    inputRowSlots (list[int], optional): Authored visible row slots for inputs\n"
          "    outputRowSlots (list[int], optional): Authored visible row slots for outputs\n\n"
          "Returns:\n"
          "    list: List of Gf.Vec2d port center positions")
      .staticmethod("calculatePortPositions");
}
