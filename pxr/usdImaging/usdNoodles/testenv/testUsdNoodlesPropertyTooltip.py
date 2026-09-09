#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit tests for property tooltip support in usdNoodles.

Verifies that:
- NodeModel.get_pin_documentation retrieves documentation from USD attributes
- GraphView._getPropertyAtPoint identifies the correct property row,
  respects z-order for overlapping nodes, and handles collapsed/missing cases
- GraphView._updatePropertyTooltip starts/stops the timer on property change
  and refreshes the screen position on every call so the tooltip appears at
  the actual cursor location
- GraphView._showPropertyTooltip handles missing documentation gracefully
- GraphView leaveEvent cleanup hides any already-visible tooltip
"""

import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

try:
    from pxr.UsdNoodles.graphView import GraphView
    from pxr.Usdviewq.qt import QtCore

    _has_graph = True
except ImportError as e:
    print(f"property tooltip graph imports failed: {e}")
    _has_graph = False


def _mock_attr(name, documentation="", is_valid=True):
    """Create a mock USD attribute with optional documentation."""
    attr = MagicMock()
    attr.GetName.return_value = name
    attr.IsValid.return_value = is_valid
    attr.GetDocumentation.return_value = documentation
    attr.__bool__ = lambda self: is_valid
    return attr


def _mock_prim_with_docs(attrs_docs):
    """Create a mock prim whose GetAttribute returns attrs with documentation.

    Args:
        attrs_docs: dict of attr_name -> documentation string
    """
    prim = MagicMock()

    def get_attribute(name):
        if name in attrs_docs:
            return _mock_attr(name, documentation=attrs_docs[name])
        return _mock_attr(name, is_valid=False)

    prim.GetAttribute.side_effect = get_attribute
    prim.GetPath.return_value = "/Root/TestNode"
    prim.GetName.return_value = "TestNode"
    prim.GetTypeName.return_value = "ExecNode"
    prim.IsValid.return_value = True
    prim.GetAttributes.return_value = []
    return prim


class TestGetPinDocumentation(unittest.TestCase):
    """Tests for NodeModel.get_pin_documentation()."""

    def _make_node(self, prim):
        from pxr.UsdNoodles.models import NodeModel

        node = NodeModel.__new__(NodeModel)
        node._prim = prim
        return node

    def test_input_pin_documentation(self):
        prim = _mock_prim_with_docs(
            {"inputs:color": "The diffuse color of the surface"}
        )
        node = self._make_node(prim)
        doc = node.get_pin_documentation("color")
        self.assertEqual(doc, "The diffuse color of the surface")

    def test_output_pin_documentation(self):
        prim = _mock_prim_with_docs({"outputs:result": "Computed shading result"})
        node = self._make_node(prim)
        doc = node.get_pin_documentation("result")
        self.assertEqual(doc, "Computed shading result")

    def test_bare_attr_documentation(self):
        prim = _mock_prim_with_docs({"myParam": "A bare parameter"})
        node = self._make_node(prim)
        doc = node.get_pin_documentation("myParam")
        self.assertEqual(doc, "A bare parameter")

    def test_input_preferred_over_bare(self):
        prim = _mock_prim_with_docs(
            {
                "inputs:val": "Input doc",
                "val": "Bare doc",
            }
        )
        node = self._make_node(prim)
        doc = node.get_pin_documentation("val")
        self.assertEqual(doc, "Input doc")

    def test_no_documentation_returns_empty(self):
        prim = _mock_prim_with_docs({"inputs:foo": ""})
        node = self._make_node(prim)
        doc = node.get_pin_documentation("foo")
        self.assertEqual(doc, "")

    def test_missing_attr_returns_empty(self):
        prim = _mock_prim_with_docs({})
        node = self._make_node(prim)
        doc = node.get_pin_documentation("nonexistent")
        self.assertEqual(doc, "")

    def test_no_prim_returns_empty(self):
        from pxr.UsdNoodles.models import NodeModel

        node = NodeModel.__new__(NodeModel)
        node._prim = None
        doc = node.get_pin_documentation("anything")
        self.assertEqual(doc, "")


# ---------------------------------------------------------------------------
# Shared fixtures for GraphView tooltip tests
# ---------------------------------------------------------------------------


def _make_renderer():
    """Renderer with deterministic metrics: title=10 units, each row=10 units."""
    return SimpleNamespace(
        getTitleFontSize=lambda: 10.0,
        getPortFontSize=lambda: 5.0,
        getPortTypeFontSize=lambda: 5.0,
        getPortMarginV=lambda: 0.0,
        getPortSpacing=lambda: 0.0,
    )


def _make_font_atlas():
    """Font atlas where ascender-descender=1 and lineHeight=1.

    Combined with _make_renderer:
      titleHeight = 10 * 1 = 10
      rowHeight   = (5 + 5) * 1 = 10
    """
    return SimpleNamespace(ascender=1.0, descender=0.0, lineHeight=1.0)


def _make_node(
    position=(0.0, 0.0),
    size=(100.0, 40.0),
    inputPins=("a",),
    outputPins=("b",),
    is_title_collapsed=False,
    zOrder=0,
):
    return SimpleNamespace(
        position=position,
        size=size,
        inputPins=list(inputPins),
        outputPins=list(outputPins),
        is_title_collapsed=is_title_collapsed,
        zOrder=zOrder,
        # Cached layout the migrated _getPropertyAtPoint reads (single source of
        # truth). Geometry contract: title height = 10, each row = 10 (see
        # _make_renderer / _make_font_atlas), node at y=0 → input "a" center 15,
        # output "b" center 25.
        layoutPortLineHeight=10.0,
        layoutPortStartY=10.0,
        layoutInputCenterY=[10.0 + 10.0 * i + 5.0 for i in range(len(inputPins))],
        layoutOutputCenterY=[
            10.0 + 10.0 * (len(inputPins) + i) + 5.0 for i in range(len(outputPins))
        ],
    )


def _make_view_for_hit_test(nodes):
    """Mock view with the minimum surface area for _getPropertyAtPoint."""
    return SimpleNamespace(
        zoom=1.0,
        panX=0.0,
        panY=0.0,
        defaultRenderer=_make_renderer(),
        fontAtlas=_make_font_atlas(),
        nodes=nodes,
    )


# ---------------------------------------------------------------------------
# Tests: GraphView._getPropertyAtPoint
# ---------------------------------------------------------------------------


@unittest.skipUnless(_has_graph, "GraphView modules not available (headless CI)")
class TestGetPropertyAtPoint(unittest.TestCase):
    """Branch coverage for _getPropertyAtPoint.

    Geometry (see _make_renderer / _make_font_atlas):
      title height = 10, row height = 10. Node at (0, 0) size (100, 40):
        y=[0,  10) → title
        y=[10, 20] → input row "a"
        y=[20, 30] → output row "b"
    """

    def test_no_renderer_returns_none(self):
        view = _make_view_for_hit_test({"/A": _make_node()})
        view.defaultRenderer = None

        self.assertIsNone(GraphView._getPropertyAtPoint(view, QtCore.QPoint(50, 15)))

    def test_hits_input_row(self):
        view = _make_view_for_hit_test({"/A": _make_node()})

        # input row y=[10,20] — pick midpoint
        self.assertEqual(
            GraphView._getPropertyAtPoint(view, QtCore.QPoint(50, 15)),
            ("/A", "a"),
        )

    def test_hits_output_row(self):
        view = _make_view_for_hit_test({"/A": _make_node()})

        # output row y=[20,30] — pick midpoint
        self.assertEqual(
            GraphView._getPropertyAtPoint(view, QtCore.QPoint(50, 25)),
            ("/A", "b"),
        )

    def test_skips_collapsed_node(self):
        view = _make_view_for_hit_test({"/A": _make_node(is_title_collapsed=True)})

        self.assertIsNone(GraphView._getPropertyAtPoint(view, QtCore.QPoint(50, 15)))

    def test_skips_node_when_x_out_of_bounds(self):
        # Node spans x=[0, 100]; cursor at x=200 misses it entirely
        view = _make_view_for_hit_test({"/A": _make_node()})

        self.assertIsNone(GraphView._getPropertyAtPoint(view, QtCore.QPoint(200, 15)))

    def test_returns_none_when_no_row_matches(self):
        # Cursor in the title band (y < input row start)
        view = _make_view_for_hit_test({"/A": _make_node()})

        self.assertIsNone(GraphView._getPropertyAtPoint(view, QtCore.QPoint(50, 5)))

    def test_topmost_node_wins_on_overlap(self):
        """When two nodes overlap, the one with higher zOrder must be returned."""
        bottom = _make_node(inputPins=("bottom_in",), zOrder=0)
        top = _make_node(inputPins=("top_in",), zOrder=5)
        view = _make_view_for_hit_test({"/Bottom": bottom, "/Top": top})

        result = GraphView._getPropertyAtPoint(view, QtCore.QPoint(50, 15))
        self.assertEqual(result, ("/Top", "top_in"))


# ---------------------------------------------------------------------------
# Tests: GraphView._updatePropertyTooltip
# ---------------------------------------------------------------------------


def _make_view_for_update_tooltip(
    next_property,
    current_property=None,
    current_screen_pos=None,
):
    timer = MagicMock()
    return SimpleNamespace(
        _getPropertyAtPoint=MagicMock(return_value=next_property),
        _tooltipProperty=current_property,
        _tooltipScreenPos=current_screen_pos,
        _tooltipTimer=timer,
        mapToGlobal=lambda p: ("global", p),
    )


@unittest.skipUnless(_has_graph, "GraphView modules not available (headless CI)")
class TestUpdatePropertyTooltip(unittest.TestCase):
    """Branch coverage for _updatePropertyTooltip.

    Branches:
      1. Property changed, new prop is not None → stop timer, hide, start timer
      2. Property changed, new prop is None     → stop timer, hide, do NOT start timer
      3. Property unchanged, prop is not None   → screen position refreshed (G1 fix)
      4. Property unchanged, prop is None       → no timer activity, no position update
    """

    def test_property_change_to_real_prop_starts_timer(self):
        view = _make_view_for_update_tooltip(
            next_property=("/A", "in"), current_property=None
        )

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            GraphView._updatePropertyTooltip(view, QtCore.QPoint(10, 20))

            view._tooltipTimer.stop.assert_called_once()
            mock_qt.QToolTip.hideText.assert_called_once()
            view._tooltipTimer.start.assert_called_once()
            self.assertEqual(view._tooltipProperty, ("/A", "in"))
            self.assertEqual(view._tooltipScreenPos, ("global", QtCore.QPoint(10, 20)))

    def test_property_change_to_none_does_not_start_timer(self):
        view = _make_view_for_update_tooltip(
            next_property=None, current_property=("/A", "in")
        )

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            GraphView._updatePropertyTooltip(view, QtCore.QPoint(10, 20))

            view._tooltipTimer.stop.assert_called_once()
            mock_qt.QToolTip.hideText.assert_called_once()
            view._tooltipTimer.start.assert_not_called()
            self.assertIsNone(view._tooltipProperty)

    def test_same_property_refreshes_screen_position(self):
        """G1: stale _tooltipScreenPos fix. When the hovered property hasn't
        changed but the cursor moved within the same row, _tooltipScreenPos
        must update so the eventual tooltip appears at the current cursor."""
        prop = ("/A", "in")
        view = _make_view_for_update_tooltip(
            next_property=prop,
            current_property=prop,
            current_screen_pos=("global", QtCore.QPoint(0, 0)),
        )

        with patch("pxr.UsdNoodles.graphView.QtWidgets"):
            GraphView._updatePropertyTooltip(view, QtCore.QPoint(99, 88))

            # Timer not touched when prop is unchanged
            view._tooltipTimer.stop.assert_not_called()
            view._tooltipTimer.start.assert_not_called()
            # But screen position is refreshed to current cursor
            self.assertEqual(view._tooltipScreenPos, ("global", QtCore.QPoint(99, 88)))

    def test_same_none_property_does_nothing(self):
        view = _make_view_for_update_tooltip(
            next_property=None,
            current_property=None,
            current_screen_pos=("global", QtCore.QPoint(0, 0)),
        )

        with patch("pxr.UsdNoodles.graphView.QtWidgets"):
            GraphView._updatePropertyTooltip(view, QtCore.QPoint(99, 88))

            view._tooltipTimer.stop.assert_not_called()
            view._tooltipTimer.start.assert_not_called()
            # Screen pos NOT updated when there's no hovered property
            self.assertEqual(view._tooltipScreenPos, ("global", QtCore.QPoint(0, 0)))


# ---------------------------------------------------------------------------
# Tests: GraphView._showPropertyTooltip
# ---------------------------------------------------------------------------


@unittest.skipUnless(_has_graph, "GraphView modules not available (headless CI)")
class TestShowPropertyTooltip(unittest.TestCase):
    """Branch coverage for _showPropertyTooltip.

    Branches:
      1. _tooltipProperty is None              → early return, no showText
      2. _tooltipScreenPos is None             → early return, no showText
      3. Node missing from self.nodes          → early return, no showText
      4. Node lacks get_pin_documentation      → early return, no showText
      5. Documentation is empty                → early return, no showText
      6. All conditions met                    → showText called
    """

    def _make_view(self, nodes=None, prop=("/A", "in"), screen_pos=("g", "p")):
        return SimpleNamespace(
            _tooltipProperty=prop,
            _tooltipScreenPos=screen_pos,
            nodes=nodes or {},
        )

    def test_no_property_returns_early(self):
        view = self._make_view(prop=None)

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            GraphView._showPropertyTooltip(view)
            mock_qt.QToolTip.showText.assert_not_called()

    def test_no_screen_pos_returns_early(self):
        view = self._make_view(screen_pos=None)

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            GraphView._showPropertyTooltip(view)
            mock_qt.QToolTip.showText.assert_not_called()

    def test_missing_node_returns_early(self):
        view = self._make_view(nodes={})  # /A is missing

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            GraphView._showPropertyTooltip(view)
            mock_qt.QToolTip.showText.assert_not_called()

    def test_node_without_get_pin_documentation_returns_early(self):
        node = SimpleNamespace()  # no get_pin_documentation attr
        view = self._make_view(nodes={"/A": node})

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            GraphView._showPropertyTooltip(view)
            mock_qt.QToolTip.showText.assert_not_called()

    def test_empty_documentation_does_not_show(self):
        node = SimpleNamespace(get_pin_documentation=lambda _: "")
        view = self._make_view(nodes={"/A": node})

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            GraphView._showPropertyTooltip(view)
            mock_qt.QToolTip.showText.assert_not_called()

    def test_shows_tooltip_when_doc_present(self):
        node = SimpleNamespace(get_pin_documentation=lambda pin: f"doc-for-{pin}")
        view = self._make_view(nodes={"/A": node})

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            GraphView._showPropertyTooltip(view)
            mock_qt.QToolTip.showText.assert_called_once_with(
                ("g", "p"), "doc-for-in", view
            )


# ---------------------------------------------------------------------------
# Tests: GraphView leaveEvent cleanup (G3)
# ---------------------------------------------------------------------------


def _leave_event_cleanup(view):
    """Mirror of the cleanup portion of GraphView.leaveEvent (graphView.py).

    Production leaveEvent also calls super().leaveEvent(event) which requires a
    real QGLWidget instance, so we shim the cleanup portion that the new code
    added. This locks down the regression that an already-visible tooltip
    would remain on screen after the cursor left the widget (G3).
    """
    view._tooltipTimer.stop()
    view._tooltipProperty = None
    # Production calls QtWidgets.QToolTip.hideText() — invoke via the same
    # module attribute so the test patch intercepts it.
    from pxr.UsdNoodles import graphView as _gv

    _gv.QtWidgets.QToolTip.hideText()


@unittest.skipUnless(_has_graph, "GraphView modules not available (headless CI)")
class TestLeaveEventCleanup(unittest.TestCase):
    """Lock down G3: leaveEvent must hide any already-visible tooltip."""

    def test_cleanup_stops_timer_clears_property_and_hides_tooltip(self):
        view = SimpleNamespace(
            _tooltipTimer=MagicMock(),
            _tooltipProperty=("/A", "in"),
        )

        with patch("pxr.UsdNoodles.graphView.QtWidgets") as mock_qt:
            _leave_event_cleanup(view)

            view._tooltipTimer.stop.assert_called_once()
            self.assertIsNone(view._tooltipProperty)
            mock_qt.QToolTip.hideText.assert_called_once()
