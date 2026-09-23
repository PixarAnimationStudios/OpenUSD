#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


from __future__ import annotations

import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock

try:
    from pxr import Gf
    from pxr.UsdNoodles.graphView import GraphView
    from pxr.Usdviewq.qt import QtCore

    _has_graph = True
except ImportError as e:
    print(f"marquee/hover graph imports failed: {e}")
    _has_graph = False


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class MarqueeSelectionTest(unittest.TestCase):
    """Exercise marquee selection logic through the public mouseMoveEvent."""

    def _make_node(self, node_id, selected=False):
        return SimpleNamespace(
            id=node_id,
            selected=selected,
            position=Gf.Vec2d(0.0, 0.0),
            size=Gf.Vec2d(10.0, 10.0),
        )

    @staticmethod
    def _fake_select_nodes(nodes_dict):
        def select(node_ids, _bounds):
            result = set()
            for nid in node_ids:
                if nid in nodes_dict:
                    nodes_dict[nid].selected = True
                    result.add(nid)
            return result

        return select

    def _make_event(self, x, y, buttons=None):
        if buttons is None:
            buttons = QtCore.Qt.LeftButton
        event = MagicMock()
        event.position.return_value.toPoint.return_value = QtCore.QPoint(x, y)
        event.buttons.return_value = buttons
        return event

    def _make_view(self, nodes, links=None, marquee_start=None, **overrides):
        if links is None:
            links = []
        if marquee_start is None:
            marquee_start = QtCore.QPointF(0.0, 0.0)
        view = SimpleNamespace(
            nodeCreationHotbox=SimpleNamespace(is_visible=False),
            minimap=SimpleNamespace(
                updateHoverState=MagicMock(),
                handleMouseMove=MagicMock(return_value=False),
            ),
            # mouseMoveEvent now gates the minimap path on this; production sets
            # it in _initCachedSettings (default True), which this unbound-call
            # fixture never runs.
            _cachedShowMinimap=True,
            _updateNodeUnderCursor=MagicMock(),
            _updateHoveredPort=MagicMock(return_value=False),
            _updatePropertyTooltip=MagicMock(),
            draggingNodes=False,
            _draggingLink=False,
            linkRenderer=SimpleNamespace(
                findLinkUnderCursor=MagicMock(return_value=-1),
            ),
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            links=links,
            _cachedLinkLineWidth=12.0,
            _linkUnderCursor=-1,
            lastMousePos=None,
            update=MagicMock(),
            spacePressed=False,
            ctrlPressedOnMouseDown=False,
            marqueeActive=True,
            nodes=nodes,
            _selectedNodes=set(),
            _selectedLinks=set(),
            _marqueeBaseSelectedNodes=set(),
            _marqueeBaseSelectedLinks=set(),
            _marqueePrevSelectedNodes=set(),
            _marqueePrevSelectedLinks=set(),
            marqueeStart=marquee_start,
            marqueeCurrent=marquee_start,
            _spatialIndex=SimpleNamespace(queryRegion=MagicMock()),
            _selectNodesInMarquee=MarqueeSelectionTest._fake_select_nodes(nodes),
            _selectLinksInMarquee=MagicMock(return_value=set()),
        )
        for key, value in overrides.items():
            setattr(view, key, value)
        view._updateMarqueeSelection = lambda: GraphView._updateMarqueeSelection(view)
        return view

    def test_marquee_grow_shrink_threshold_additive_sequence(self):
        """Drive grow, shrink-past-base, shrink-below-min, ctrl-additive
        against a mocked spatial index and assert _selectedNodes after
        each step."""
        node_a = self._make_node("/A")
        node_b = self._make_node("/B")
        node_c = self._make_node("/C")
        nodes = {"/A": node_a, "/B": node_b, "/C": node_c}

        view = self._make_view(nodes)

        # Step 1: Grow — marquee captures A and B
        view._spatialIndex.queryRegion.return_value = (["/A", "/B"], [])
        GraphView.mouseMoveEvent(view, self._make_event(50, 50))
        self.assertEqual(view._selectedNodes, {"/A", "/B"})
        self.assertTrue(node_a.selected)
        self.assertTrue(node_b.selected)
        self.assertFalse(node_c.selected)

        # Step 2: Shrink past base — B leaves the marquee
        view._spatialIndex.queryRegion.return_value = (["/A"], [])
        GraphView.mouseMoveEvent(view, self._make_event(20, 20))
        self.assertEqual(view._selectedNodes, {"/A"})
        self.assertTrue(node_a.selected)
        self.assertFalse(node_b.selected)

        # Step 3: Shrink below min threshold (dx=1, dy=1 < 2.0/zoom)
        GraphView.mouseMoveEvent(view, self._make_event(1, 1))
        self.assertEqual(view._selectedNodes, set())
        self.assertFalse(node_a.selected)
        self.assertEqual(view._marqueePrevSelectedNodes, set())

        # Step 4: Ctrl-additive — start with A as base selection
        node_a.selected = True
        view._marqueeBaseSelectedNodes = {"/A"}
        view._marqueePrevSelectedNodes = {"/A"}
        view._selectedNodes = {"/A"}
        view.marqueeStart = QtCore.QPointF(0.0, 0.0)
        view._spatialIndex.queryRegion.return_value = (["/C"], [])
        GraphView.mouseMoveEvent(view, self._make_event(80, 80))
        self.assertEqual(view._selectedNodes, {"/A", "/C"})
        self.assertTrue(node_c.selected)

    def test_marquee_inactive_is_noop(self):
        view = self._make_view({}, marqueeActive=False)

        GraphView.mouseMoveEvent(view, self._make_event(50, 50))

        view._spatialIndex.queryRegion.assert_not_called()

    def test_marquee_shrink_below_threshold_deselects_links(self):
        link_0 = SimpleNamespace(selected=True)
        link_1 = SimpleNamespace(selected=True)
        view = self._make_view(
            nodes={},
            links=[link_0, link_1],
            marquee_start=QtCore.QPointF(100.0, 100.0),
            _selectedLinks={0, 1},
            _marqueeBaseSelectedLinks={0},
            _marqueePrevSelectedLinks={0, 1},
        )

        # Event at (101, 101) → marqueeCurrent = (101, 101), dx=1 < 2.0/zoom
        GraphView.mouseMoveEvent(view, self._make_event(101, 101))

        self.assertFalse(link_1.selected)
        self.assertEqual(view._selectedLinks, {0})
        self.assertEqual(view._marqueePrevSelectedLinks, {0})


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class HoverGatingDuringDragTest(unittest.TestCase):
    """Verify mouseMoveEvent suppresses hover updates during drags."""

    def _make_event(self, buttons=QtCore.Qt.NoButton):
        event = MagicMock()
        event.position.return_value.toPoint.return_value = QtCore.QPoint(50, 50)
        event.buttons.return_value = buttons
        return event

    def _make_view(self, *, dragging_nodes=False, dragging_link=False):
        return SimpleNamespace(
            nodeCreationHotbox=SimpleNamespace(is_visible=False),
            minimap=SimpleNamespace(
                updateHoverState=MagicMock(),
                handleMouseMove=MagicMock(return_value=False),
            ),
            # mouseMoveEvent now gates the minimap path on this; production sets
            # it in _initCachedSettings (default True), which this unbound-call
            # fixture never runs.
            _cachedShowMinimap=True,
            _updateNodeUnderCursor=MagicMock(),
            _updateHoveredPort=MagicMock(return_value=False),
            _updatePropertyTooltip=MagicMock(),
            draggingNodes=dragging_nodes,
            _draggingLink=dragging_link,
            linkRenderer=SimpleNamespace(
                findLinkUnderCursor=MagicMock(return_value=-1),
            ),
            zoom=1.0,
            panX=0.0,
            panY=0.0,
            links=[],
            _cachedLinkLineWidth=12.0,
            _linkUnderCursor=-1,
            _updateLinkDrag=MagicMock(),
            lastMousePos=None,
            update=MagicMock(),
            spacePressed=False,
            ctrlPressedOnMouseDown=False,
            marqueeActive=False,
            # The hover path now passes the spatial index to the link picker.
            _spatialIndex=MagicMock(),
        )

    def test_hover_suppressed_during_node_drag(self):
        view = self._make_view(dragging_nodes=True)

        GraphView.mouseMoveEvent(view, self._make_event())

        view._updateHoveredPort.assert_not_called()
        view.linkRenderer.findLinkUnderCursor.assert_not_called()

    def test_port_hover_active_during_link_drag(self):
        view = self._make_view(dragging_link=True)

        GraphView.mouseMoveEvent(view, self._make_event())

        view._updateHoveredPort.assert_called_once()
        view.linkRenderer.findLinkUnderCursor.assert_not_called()

    def test_both_hovers_active_when_not_dragging(self):
        view = self._make_view()

        GraphView.mouseMoveEvent(view, self._make_event())

        view._updateNodeUnderCursor.assert_called_once()
        view._updateHoveredPort.assert_called_once()
        view._updatePropertyTooltip.assert_called_once()
        view.linkRenderer.findLinkUnderCursor.assert_called_once()

    def test_hover_suppressed_during_middle_button_pan(self):
        view = self._make_view()
        # The pan path reads lastMousePos.x()/.y(); give it a real point.
        view.lastMousePos = QtCore.QPoint(0, 0)

        GraphView.mouseMoveEvent(view, self._make_event(QtCore.Qt.MiddleButton))

        view._updateNodeUnderCursor.assert_not_called()
        view._updateHoveredPort.assert_not_called()
        view._updatePropertyTooltip.assert_not_called()
        view.linkRenderer.findLinkUnderCursor.assert_not_called()

    def test_hover_suppressed_during_space_left_pan(self):
        view = self._make_view()
        view.spacePressed = True
        view.lastMousePos = QtCore.QPoint(0, 0)

        GraphView.mouseMoveEvent(view, self._make_event(QtCore.Qt.LeftButton))

        view._updateNodeUnderCursor.assert_not_called()
        view._updateHoveredPort.assert_not_called()
        view._updatePropertyTooltip.assert_not_called()
        view.linkRenderer.findLinkUnderCursor.assert_not_called()


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class NodeDragLinkReindexTest(unittest.TestCase):
    """Dragging a node must refresh connected links' spatial-index bounds."""

    def test_moved_link_bounds_reinserted(self):
        moved_link = SimpleNamespace(
            data_sourceNodeId="A",
            data_targetNodeId="B",
            sourcePort="out",
            targetPort="in",
        )
        unrelated_link = SimpleNamespace(
            data_sourceNodeId="C",
            data_targetNodeId="D",
            sourcePort="out",
            targetPort="in",
        )
        view = SimpleNamespace(
            links=[moved_link, unrelated_link],
            nodes={"A": object(), "B": object(), "C": object(), "D": object()},
            _setupLinkEndpoints=MagicMock(),
            _computeLinkBounds=MagicMock(return_value="bounds0"),
            _spatialIndex=SimpleNamespace(insertLink=MagicMock()),
        )

        GraphView._updateLinksForMovedNodes(view, {"A"})

        # Only the link touching the moved node "A" is re-indexed (index 0); the
        # unrelated link is left untouched.
        view._setupLinkEndpoints.assert_called_once()
        view._spatialIndex.insertLink.assert_called_once_with(0, "bounds0")
