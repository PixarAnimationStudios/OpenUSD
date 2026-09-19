#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit tests for pin-drag interaction behavior in the node graph view.

Covers three related GraphView features:

  REQ 3 - Pin visibility gate (graphView.py:5011, top of _addPortVertices):
          a non-relationship pin glyph is only drawn when the pin carries a
          connection or is the endpoint currently being dragged. Relationship
          glyphs always render as authoring handles.

  REQ 4 - Row-edge drag start (graphView.py:5464): pressing the outer tenth of
          a row on the left or right side starts a new connection from that row
          even when no pin glyph is visible. Left band -> input side, right band
          -> output side. A press in the middle of the row, above the first row
          (the title), or below the last row falls through to node drag.

  REQ 5 - Arrowhead hit-test (graphView.py:8030): the arrowhead at a
          relationship link's target end is grabbable so it can be dragged to
          another pin (reconnect) or onto empty space (disconnect).

GraphView cannot be imported in headless CI (module-level GL imports), so the
production logic is mirrored here in shims tested against lightweight fakes -
the same approach as testUsdNoodlesDisconnectDrag and
testUsdNoodlesGraphViewShortcut. The shims are byte-faithful copies of the
production methods; the cited line numbers keep them aligned.
"""

import collections
import unittest


# ---------------------------------------------------------------------------
# Lightweight fakes (plain objects, NOT MagicMock).
#
# MagicMock is unsafe here: getattr(mock, "_title_collapsed", False) returns a
# truthy Mock rather than the default, which would silently short-circuit the
# row-edge logic. Plain objects give honest attribute defaults.
# ---------------------------------------------------------------------------


class _Renderer:
    def __init__(self, port_width=8.0):
        self._port_width = port_width

    def getPortWidth(self):
        return self._port_width


class _Node:
    def __init__(
        self,
        node_id="/N",
        position=(0.0, 0.0),
        size=(100.0, 200.0),
        inputPins=None,
        outputPins=None,
        input_slots=None,
        output_slots=None,
        input_kinds=None,
        output_kinds=None,
        dual=None,
        title_collapsed=False,
        renderer=None,
    ):
        self.id = node_id
        self.position = position
        self.size = size
        self.inputPins = list(inputPins or [])
        self.outputPins = list(outputPins or [])
        self._input_row_slots = (
            list(input_slots)
            if input_slots is not None
            else list(range(len(self.inputPins)))
        )
        self._output_row_slots = (
            list(output_slots)
            if output_slots is not None
            else list(range(len(self.outputPins)))
        )
        self._input_row_kinds = (
            list(input_kinds) if input_kinds is not None else [0] * len(self.inputPins)
        )
        self._output_row_kinds = (
            list(output_kinds)
            if output_kinds is not None
            else [0] * len(self.outputPins)
        )
        self._dual_pin_names = set(dual or set())
        self._title_collapsed = title_collapsed
        self.renderer = renderer


# namedtuple (immutable) keeps FLAKE8 B903 quiet for this pure data holder.
_Link = collections.namedtuple(
    "_Link",
    ["end", "is_relationship_link", "isDangling"],
    defaults=(False, False),
)


class _View:
    def __init__(
        self,
        port_start_y=30.0,
        port_line_height=20.0,
        port_positions=None,
        renderer=None,
        zoom=1.0,
        links=None,
    ):
        self._port_start_y = port_start_y
        self._port_line_height = port_line_height
        self._port_positions = dict(port_positions or {})
        self.defaultRenderer = renderer if renderer is not None else _Renderer()
        self.zoom = zoom
        self.links = list(links or [])
        # Drag state consumed by the REQ 3 visibility gate.
        self._draggingLink = False
        self._dragLinkSourceNode = None
        self._dragLinkSourcePort = None
        self._dragLinkSourceIsOutput = None

    # Unchanged production helpers, stubbed deterministically.
    def _getRowHitMetrics(self, node, renderer):
        return self._port_start_y, self._port_line_height

    def _getPortPosition(self, node, pinName, isOutput):
        return self._port_positions.get((pinName, isOutput))


# ===========================================================================
# Shims (byte-faithful mirrors of the production methods)
# ===========================================================================


def _should_draw_pin(view, node, pin_name, is_output, is_connected, is_rel_pin):
    """Mirror of the visibility gate at the top of _addPortVertices
    (graphView.py:5011). Returns True when the glyph should be drawn."""
    if not is_rel_pin:
        is_drag_source = (
            view._draggingLink
            and view._dragLinkSourceNode == getattr(node, "id", None)
            and view._dragLinkSourcePort == pin_name
            and view._dragLinkSourceIsOutput == is_output
        )
        if not (is_connected or is_drag_source):
            return False
    return True


def _row_edge_pin_candidates(node, is_output):
    """Mirror of GraphView._rowEdgePinCandidates (graphView.py:5403)."""
    candidates = []
    if is_output:
        out_pins = getattr(node, "outputPins", [])
        out_slots = getattr(node, "_output_row_slots", [])
        out_kinds = getattr(node, "_output_row_kinds", [])
        for i, pin in enumerate(out_pins):
            if i < len(out_kinds) and out_kinds[i] == 2:
                continue
            candidates.append((pin, out_slots[i] if i < len(out_slots) else i))
        out_set = set(out_pins)
        dual = getattr(node, "_dual_pin_names", set())
        in_pins = getattr(node, "inputPins", [])
        in_slots = getattr(node, "_input_row_slots", [])
        in_kinds = getattr(node, "_input_row_kinds", [])
        for i, pin in enumerate(in_pins):
            if pin not in dual or pin in out_set:
                continue
            if i < len(in_kinds) and in_kinds[i] == 2:
                continue
            candidates.append((pin, in_slots[i] if i < len(in_slots) else i))
    else:
        in_pins = getattr(node, "inputPins", [])
        in_slots = getattr(node, "_input_row_slots", [])
        in_kinds = getattr(node, "_input_row_kinds", [])
        for i, pin in enumerate(in_pins):
            if i < len(in_kinds) and in_kinds[i] == 2:
                continue
            candidates.append((pin, in_slots[i] if i < len(in_slots) else i))
    return candidates


def _pin_at_row_edge(view, node, world_pos, is_output):
    """Mirror of GraphView._pinAtRowEdge (graphView.py:5440)."""
    renderer = node.renderer if node.renderer else view.defaultRenderer
    if not renderer:
        return None
    port_start_y, port_line_height = view._getRowHitMetrics(node, renderer)
    best = None
    best_distance = None
    for pin_name, row_slot in _row_edge_pin_candidates(node, is_output):
        row_y = port_start_y + row_slot * port_line_height
        if world_pos[1] < row_y or world_pos[1] > row_y + port_line_height:
            continue
        port_pos = view._getPortPosition(node, pin_name, is_output)
        row_center_y = (
            float(port_pos[1])
            if port_pos is not None
            else row_y + port_line_height * 0.5
        )
        distance = abs(float(world_pos[1]) - row_center_y)
        if best_distance is None or distance < best_distance:
            best_distance = distance
            best = pin_name
    return best


def _find_row_edge_drag_start(view, node, world_pos):
    """Mirror of GraphView._findRowEdgeDragStart (graphView.py:5464)."""
    if getattr(node, "_title_collapsed", False):
        return None
    renderer = node.renderer if node.renderer else view.defaultRenderer
    if not renderer:
        return None
    nx = float(node.position[0])
    nw = float(node.size[0])
    if world_pos[0] < nx or world_pos[0] > nx + nw:
        return None
    edge_band = nw * 0.1
    if world_pos[0] >= nx + nw - edge_band:
        is_output = True
    elif world_pos[0] <= nx + edge_band:
        is_output = False
    else:
        return None
    pin_name = _pin_at_row_edge(view, node, world_pos, is_output)
    if pin_name is None:
        return None
    return (pin_name, is_output)


def _find_arrowhead_under_cursor(view, world_pos):
    """Mirror of GraphView._findArrowheadUnderCursor (graphView.py:8030)."""
    arrow_length = max(10.0 / max(view.zoom, 1e-6), 2.0)
    arrow_width = max(8.0 / max(view.zoom, 1e-6), 2.0)
    hit_radius = max(arrow_length, arrow_width)
    hit_radius_sq = hit_radius * hit_radius
    best = -1
    best_distance = None
    for i, link in enumerate(view.links):
        if not bool(getattr(link, "is_relationship_link", False)):
            continue
        if getattr(link, "isDangling", False):
            continue
        dx = world_pos[0] - float(link.end[0])
        dy = world_pos[1] - float(link.end[1])
        distance = dx * dx + dy * dy
        if distance <= hit_radius_sq and (
            best_distance is None or distance < best_distance
        ):
            best_distance = distance
            best = i
    return best


# ===========================================================================
# REQ 3 - Pin visibility gate
# ===========================================================================


class TestPinVisibilityGate(unittest.TestCase):
    """A pin glyph is drawn only when connected, drag-source, or relationship."""

    def _view(self):
        return _View()

    def test_connected_pin_is_drawn(self):
        node = _Node(node_id="/A")
        self.assertTrue(_should_draw_pin(self._view(), node, "in", False, True, False))

    def test_unconnected_idle_pin_is_hidden(self):
        node = _Node(node_id="/A")
        self.assertFalse(
            _should_draw_pin(self._view(), node, "in", False, False, False)
        )

    def test_relationship_pin_always_drawn_even_when_idle(self):
        node = _Node(node_id="/A")
        self.assertTrue(_should_draw_pin(self._view(), node, "rel", True, False, True))

    def test_drag_source_pin_is_drawn_while_unconnected(self):
        view = self._view()
        node = _Node(node_id="/A")
        view._draggingLink = True
        view._dragLinkSourceNode = "/A"
        view._dragLinkSourcePort = "in"
        view._dragLinkSourceIsOutput = False
        self.assertTrue(_should_draw_pin(view, node, "in", False, False, False))

    def test_drag_other_side_of_same_pin_stays_hidden(self):
        """Dragging a dual row's input side must not reveal its output glyph."""
        view = self._view()
        node = _Node(node_id="/A")
        view._draggingLink = True
        view._dragLinkSourceNode = "/A"
        view._dragLinkSourcePort = "dual"
        view._dragLinkSourceIsOutput = False
        # The output-side render of the same pin should NOT match.
        self.assertFalse(_should_draw_pin(view, node, "dual", True, False, False))
        # The input-side render does match.
        self.assertTrue(_should_draw_pin(view, node, "dual", False, False, False))

    def test_drag_different_port_stays_hidden(self):
        view = self._view()
        node = _Node(node_id="/A")
        view._draggingLink = True
        view._dragLinkSourceNode = "/A"
        view._dragLinkSourcePort = "other"
        view._dragLinkSourceIsOutput = False
        self.assertFalse(_should_draw_pin(view, node, "in", False, False, False))

    def test_drag_from_different_node_stays_hidden(self):
        view = self._view()
        node = _Node(node_id="/A")
        view._draggingLink = True
        view._dragLinkSourceNode = "/OTHER"
        view._dragLinkSourcePort = "in"
        view._dragLinkSourceIsOutput = False
        self.assertFalse(_should_draw_pin(view, node, "in", False, False, False))


# ===========================================================================
# REQ 4 - Row-edge drag start
# ===========================================================================


class TestRowEdgePinCandidates(unittest.TestCase):
    """Candidate pins per side, including dual mirrors and header skips."""

    def test_input_side_lists_input_pins(self):
        node = _Node(inputPins=["a", "b"], outputPins=["x"])
        self.assertEqual(_row_edge_pin_candidates(node, False), [("a", 0), ("b", 1)])

    def test_output_side_lists_output_pins(self):
        node = _Node(inputPins=["a"], outputPins=["x", "y"])
        self.assertEqual(_row_edge_pin_candidates(node, True), [("x", 0), ("y", 1)])

    def test_unfolded_header_kind2_is_skipped(self):
        # "grp" is an unfolded header (kind 2); only the child is a candidate.
        node = _Node(
            outputPins=["grp", "grp:child"],
            output_slots=[0, 1],
            output_kinds=[2, 3],
        )
        self.assertEqual(_row_edge_pin_candidates(node, True), [("grp:child", 1)])

    def test_lazy_dual_pin_mirrors_to_output_side(self):
        # Lazy path: dual pin lives only in inputPins, mirrored to output.
        node = _Node(inputPins=["dual"], outputPins=[], dual={"dual"})
        self.assertEqual(_row_edge_pin_candidates(node, True), [("dual", 0)])

    def test_descriptor_dual_not_double_counted(self):
        # Descriptor path: pin already in both lists; dual set empty.
        node = _Node(inputPins=["rig1:space"], outputPins=["rig1:space"])
        self.assertEqual(_row_edge_pin_candidates(node, True), [("rig1:space", 0)])

    def test_non_dual_input_not_mirrored_to_output(self):
        node = _Node(inputPins=["a"], outputPins=["x"], dual=set())
        self.assertEqual(_row_edge_pin_candidates(node, True), [("x", 0)])


class TestPinAtRowEdge(unittest.TestCase):
    """Resolve a press to the pin whose row band contains its Y."""

    def _view(self):
        return _View(
            port_start_y=30.0,
            port_line_height=20.0,
            port_positions={
                ("a", False): (0.0, 40.0),
                ("b", False): (0.0, 60.0),
                ("x", True): (100.0, 40.0),
            },
        )

    def _node(self):
        return _Node(inputPins=["a", "b"], outputPins=["x"])

    def test_first_input_row(self):
        self.assertEqual(
            _pin_at_row_edge(self._view(), self._node(), (5.0, 40.0), False), "a"
        )

    def test_second_input_row(self):
        self.assertEqual(
            _pin_at_row_edge(self._view(), self._node(), (5.0, 60.0), False), "b"
        )

    def test_output_row(self):
        self.assertEqual(
            _pin_at_row_edge(self._view(), self._node(), (95.0, 40.0), True), "x"
        )

    def test_y_outside_any_row_returns_none(self):
        self.assertIsNone(
            _pin_at_row_edge(self._view(), self._node(), (5.0, 150.0), False)
        )

    def test_missing_port_position_falls_back_to_band_center(self):
        # No port_positions -> row_center falls back to row band center; the
        # press still resolves to the row whose band contains it.
        view = _View(port_start_y=30.0, port_line_height=20.0, port_positions={})
        node = _Node(inputPins=["a", "b"])
        self.assertEqual(_pin_at_row_edge(view, node, (5.0, 55.0), False), "b")

    def test_no_renderer_returns_none(self):
        view = _View()
        view.defaultRenderer = None
        self.assertIsNone(_pin_at_row_edge(view, self._node(), (5.0, 40.0), False))


class TestFindRowEdgeDragStart(unittest.TestCase):
    """End-to-end row-edge drag start: press -> (pin, is_output) or None."""

    def _view(self):
        return _View(
            port_start_y=30.0,
            port_line_height=20.0,
            port_positions={
                ("a", False): (0.0, 40.0),
                ("b", False): (0.0, 60.0),
                ("x", True): (100.0, 40.0),
            },
        )

    def _node(self):
        # position (0,0), size (100,200); 10% edge band = 10px on each side.
        return _Node(
            node_id="/A",
            position=(0.0, 0.0),
            size=(100.0, 200.0),
            inputPins=["a", "b"],
            outputPins=["x"],
        )

    def test_left_edge_starts_input_drag(self):
        self.assertEqual(
            _find_row_edge_drag_start(self._view(), self._node(), (5.0, 40.0)),
            ("a", False),
        )

    def test_right_edge_starts_output_drag(self):
        self.assertEqual(
            _find_row_edge_drag_start(self._view(), self._node(), (95.0, 40.0)),
            ("x", True),
        )

    def test_middle_falls_through_to_node_drag(self):
        # The middle 80% of the row is a node-body drag handle, not a
        # connect-zone, so a press there returns None and the node moves.
        self.assertIsNone(
            _find_row_edge_drag_start(self._view(), self._node(), (50.0, 40.0))
        )

    def test_edge_band_is_outer_tenth(self):
        # Band = 10% of the 100px-wide node = 10px on each side, boundaries
        # inclusive. Just inside the band connects; one pixel past it falls
        # through to a node-body drag.
        view, node = self._view(), self._node()
        self.assertEqual(
            _find_row_edge_drag_start(view, node, (10.0, 40.0)), ("a", False)
        )
        self.assertIsNone(_find_row_edge_drag_start(view, node, (11.0, 40.0)))
        self.assertEqual(
            _find_row_edge_drag_start(view, node, (90.0, 40.0)), ("x", True)
        )
        self.assertIsNone(_find_row_edge_drag_start(view, node, (89.0, 40.0)))

    def test_press_left_of_node_returns_none(self):
        self.assertIsNone(
            _find_row_edge_drag_start(self._view(), self._node(), (-5.0, 40.0))
        )

    def test_press_right_of_node_returns_none(self):
        self.assertIsNone(
            _find_row_edge_drag_start(self._view(), self._node(), (105.0, 40.0))
        )

    def test_press_below_all_rows_returns_none(self):
        # Y is below every row band -> no pin on either side -> None, so the
        # press falls through to node selection/drag.
        self.assertIsNone(
            _find_row_edge_drag_start(self._view(), self._node(), (5.0, 150.0))
        )

    def test_title_collapsed_node_returns_none(self):
        node = self._node()
        node._title_collapsed = True
        self.assertIsNone(_find_row_edge_drag_start(self._view(), node, (5.0, 40.0)))

    def test_no_renderer_returns_none(self):
        view = self._view()
        view.defaultRenderer = None
        self.assertIsNone(_find_row_edge_drag_start(view, self._node(), (5.0, 40.0)))

    def test_dual_row_both_edges_resolve(self):
        view = _View(
            port_start_y=30.0,
            port_line_height=20.0,
            port_positions={
                ("dual", False): (0.0, 40.0),
                ("dual", True): (100.0, 40.0),
            },
        )
        node = _Node(
            node_id="/A",
            position=(0.0, 0.0),
            size=(100.0, 200.0),
            inputPins=["dual"],
            outputPins=[],
            dual={"dual"},
        )
        self.assertEqual(
            _find_row_edge_drag_start(view, node, (5.0, 40.0)), ("dual", False)
        )
        self.assertEqual(
            _find_row_edge_drag_start(view, node, (95.0, 40.0)), ("dual", True)
        )


# ===========================================================================
# REQ 5 - Arrowhead hit-test
# ===========================================================================


class TestFindArrowheadUnderCursor(unittest.TestCase):
    """Grab the arrowhead at a relationship link's target end."""

    def test_hit_within_radius(self):
        view = _View(
            zoom=1.0,
            links=[_Link((100.0, 50.0), is_relationship_link=True)],
        )
        # radius = max(10, 8) = 10; (105,50) is 5 away -> hit.
        self.assertEqual(_find_arrowhead_under_cursor(view, (105.0, 50.0)), 0)

    def test_miss_outside_radius(self):
        view = _View(
            zoom=1.0,
            links=[_Link((100.0, 50.0), is_relationship_link=True)],
        )
        # (115,50) is 15 away > radius 10 -> miss.
        self.assertEqual(_find_arrowhead_under_cursor(view, (115.0, 50.0)), -1)

    def test_non_relationship_link_ignored(self):
        view = _View(
            zoom=1.0,
            links=[_Link((100.0, 50.0), is_relationship_link=False)],
        )
        self.assertEqual(_find_arrowhead_under_cursor(view, (100.0, 50.0)), -1)

    def test_dangling_relationship_link_ignored(self):
        view = _View(
            zoom=1.0,
            links=[_Link((100.0, 50.0), is_relationship_link=True, isDangling=True)],
        )
        self.assertEqual(_find_arrowhead_under_cursor(view, (100.0, 50.0)), -1)

    def test_nearest_arrowhead_wins(self):
        view = _View(
            zoom=1.0,
            links=[
                _Link((100.0, 50.0), is_relationship_link=True),
                _Link((103.0, 50.0), is_relationship_link=True),
            ],
        )
        # Cursor (104,50): link0 dist 4, link1 dist 1 -> link1.
        self.assertEqual(_find_arrowhead_under_cursor(view, (104.0, 50.0)), 1)

    def test_empty_links_returns_minus_one(self):
        self.assertEqual(_find_arrowhead_under_cursor(_View(), (0.0, 0.0)), -1)

    def test_radius_scales_with_zoom(self):
        # zoom 2.0 -> arrow_length max(5,2)=5, radius 5, radius^2 25.
        view = _View(
            zoom=2.0,
            links=[_Link((100.0, 50.0), is_relationship_link=True)],
        )
        self.assertEqual(_find_arrowhead_under_cursor(view, (104.0, 50.0)), 0)
        self.assertEqual(_find_arrowhead_under_cursor(view, (106.0, 50.0)), -1)


# ===========================================================================
# Bug 2 regression - _getRowHitMetrics schema-type offset
#
# After REQ 1 every USD-prim node carries a schemaTypeName, so the renderer
# draws a schema-type subtitle that reserves an extra line below the title
# (C++ GraphNodeRenderer::getSchemaTypeHeight, mirrored in getPortPosition).
# _getRowHitMetrics computes the row-edge-drag / row-hover hit bands and must
# add the same offset; otherwise the bands sit above the real rows and a
# freshly added (unconnected, glyph-less) node cannot start a connection from
# its side. The shim below mirrors the production method (graphView.py:5224);
# schema_type_font_ratio stands in for RenderConfig.kSchemaTypeFontRatio.
# ===========================================================================


class _MetricsRenderer:
    """Renderer fake exposing the font-size getters _getRowHitMetrics reads."""

    def __init__(
        self,
        port_margin_v=4.0,
        title_font_size=14.0,
        port_font_size=10.0,
        port_type_font_size=8.0,
        port_spacing=2.0,
        port_width=8.0,
    ):
        self._port_margin_v = port_margin_v
        self._title_font_size = title_font_size
        self._port_font_size = port_font_size
        self._port_type_font_size = port_type_font_size
        self._port_spacing = port_spacing
        self._port_width = port_width

    def getPortMarginV(self):
        return self._port_margin_v

    def getTitleFontSize(self):
        return self._title_font_size

    def getPortFontSize(self):
        return self._port_font_size

    def getPortTypeFontSize(self):
        return self._port_type_font_size

    def getPortSpacing(self):
        return self._port_spacing

    def getPortWidth(self):
        return self._port_width


class _FontAtlas:
    """Font-atlas fake with the three metrics _getRowHitMetrics reads."""

    __slots__ = ("ascender", "descender", "lineHeight")

    def __init__(self, ascender=0.8, descender=-0.2, line_height=1.2):
        self.ascender = ascender
        self.descender = descender
        self.lineHeight = line_height


class _MetricsNode:
    """Node fake carrying the geometry/schema fields _getRowHitMetrics reads."""

    # __slots__ silences FLAKE8 B903 while still allowing the getattr-default
    # test to `del` schemaTypeName (slots support attribute deletion).
    __slots__ = ("position", "size", "schemaTypeName")

    def __init__(self, position=(0.0, 0.0), size=(100.0, 200.0), schema_type_name=""):
        self.position = position
        self.size = size
        self.schemaTypeName = schema_type_name


def _row_hit_metrics(node, renderer, font_atlas, schema_type_font_ratio):
    """Mirror of GraphView._getRowHitMetrics (graphView.py:5224).

    font_atlas is the production self.fontAtlas (None falls to the size-based
    fallback). schema_type_font_ratio stands in for the production module
    constant RenderConfig.kSchemaTypeFontRatio.
    """
    node_margin_v = renderer.getPortMarginV()
    if font_atlas:
        title_height = (
            renderer.getTitleFontSize() * (font_atlas.ascender - font_atlas.descender)
            + node_margin_v * 2.0
        )
        if getattr(node, "schemaTypeName", ""):
            title_height += (
                renderer.getTitleFontSize()
                * schema_type_font_ratio
                * font_atlas.lineHeight
            )
        port_name_height = renderer.getPortFontSize() * font_atlas.lineHeight
        port_type_height = renderer.getPortTypeFontSize() * font_atlas.lineHeight
        port_line_height = (
            port_name_height + port_type_height + renderer.getPortSpacing()
        )
    else:
        title_height = min(float(node.size[1]), 32.0)
        port_line_height = max(renderer.getPortWidth() * 2.0, 16.0)
    port_start_y = node.position[1] + title_height + node_margin_v
    return port_start_y, port_line_height


class TestRowHitMetricsSchemaTypeOffset(unittest.TestCase):
    """_getRowHitMetrics reserves the schema-type subtitle line (Bug 2)."""

    _RATIO = 0.6  # stand-in for RenderConfig.kSchemaTypeFontRatio

    def _expected_schema_offset(self, renderer, font_atlas):
        return renderer.getTitleFontSize() * self._RATIO * font_atlas.lineHeight

    def test_schema_type_pushes_first_row_down(self):
        renderer = _MetricsRenderer()
        atlas = _FontAtlas()
        typed_start, _ = _row_hit_metrics(
            _MetricsNode(schema_type_name="IrFkController"),
            renderer,
            atlas,
            self._RATIO,
        )
        bare_start, _ = _row_hit_metrics(
            _MetricsNode(schema_type_name=""), renderer, atlas, self._RATIO
        )
        self.assertGreater(typed_start, bare_start)
        self.assertAlmostEqual(
            typed_start - bare_start,
            self._expected_schema_offset(renderer, atlas),
        )

    def test_missing_schema_attr_matches_empty(self):
        renderer = _MetricsRenderer()
        atlas = _FontAtlas()
        # A node lacking schemaTypeName exercises the getattr default branch.
        no_attr = _MetricsNode(schema_type_name="")
        del no_attr.schemaTypeName
        empty_start, _ = _row_hit_metrics(
            _MetricsNode(schema_type_name=""), renderer, atlas, self._RATIO
        )
        no_attr_start, _ = _row_hit_metrics(no_attr, renderer, atlas, self._RATIO)
        self.assertAlmostEqual(no_attr_start, empty_start)

    def test_port_line_height_unaffected_by_schema_type(self):
        renderer = _MetricsRenderer()
        atlas = _FontAtlas()
        _, typed_h = _row_hit_metrics(
            _MetricsNode(schema_type_name="Mesh"), renderer, atlas, self._RATIO
        )
        _, bare_h = _row_hit_metrics(
            _MetricsNode(schema_type_name=""), renderer, atlas, self._RATIO
        )
        self.assertAlmostEqual(typed_h, bare_h)

    def test_typed_start_matches_full_formula(self):
        renderer = _MetricsRenderer()
        atlas = _FontAtlas()
        node = _MetricsNode(position=(5.0, 7.0), schema_type_name="Xform")
        start, line_h = _row_hit_metrics(node, renderer, atlas, self._RATIO)
        margin = renderer.getPortMarginV()
        title = (
            renderer.getTitleFontSize() * (atlas.ascender - atlas.descender)
            + margin * 2.0
            + self._expected_schema_offset(renderer, atlas)
        )
        self.assertAlmostEqual(start, node.position[1] + title + margin)
        self.assertAlmostEqual(
            line_h,
            renderer.getPortFontSize() * atlas.lineHeight
            + renderer.getPortTypeFontSize() * atlas.lineHeight
            + renderer.getPortSpacing(),
        )

    def test_fallback_without_font_atlas(self):
        renderer = _MetricsRenderer()
        node = _MetricsNode(position=(0.0, 0.0), size=(100.0, 20.0))
        start, line_h = _row_hit_metrics(node, renderer, None, self._RATIO)
        # title_height = min(size[1], 32.0) = 20.0; plus the vertical margin.
        self.assertAlmostEqual(start, 0.0 + 20.0 + renderer.getPortMarginV())
        self.assertAlmostEqual(line_h, max(renderer.getPortWidth() * 2.0, 16.0))
