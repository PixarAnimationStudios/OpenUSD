#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Tests for T272552142: text labels offset after remove + re-add a prim.

Root cause: addNodesFromPrimTreeSelection called _clearRenderCache() but not
textRenderer.resetPositionCaches(), so the text renderer kept stale node base
positions from before removal. Re-adding the same prim (same node ID) caused
text labels to render at the old cached position instead of the new one.

Fix: call textRenderer.resetPositionCaches() after _clearRenderCache() whenever
addedCount > 0, matching every other load path in GraphView.
"""

import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock

try:
    from pxr.UsdNoodles.graphView import GraphView

    _has_pxr = True
except ImportError:
    _has_pxr = False


def _make_view(nodes=None, added_count_from_add_prim=1):
    """Build a minimal SimpleNamespace that satisfies addNodesFromPrimTreeSelection."""
    nodes = nodes if nodes is not None else {}

    mock_prim = MagicMock()
    mock_prim.GetPath.return_value = MagicMock(
        pathString="/Node", __str__=lambda s: "/Node"
    )
    mock_prim.GetTypeName.return_value = "Xform"

    dataModel = SimpleNamespace(
        selection=SimpleNamespace(getPrims=MagicMock(return_value=[mock_prim]))
    )
    api = SimpleNamespace(
        dataModel=dataModel,
        stage=MagicMock(),
    )

    view = SimpleNamespace(
        _usdviewApi=api,
        nodes=nodes,
        _selectedNodes=set(),
        _lastPrimTreeSelection=[],
        linksChanged=False,
        textChanged=False,
        _nextZOrder=0,
        _clearRenderCache=MagicMock(),
        textRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
        _cppIconRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
        _nodeRenderManager=SimpleNamespace(resetNodeQuadCaches=MagicMock()),
        _nodeTransformFrame=SimpleNamespace(reset=MagicMock()),
        # addNodesFromPrimTreeSelection grid-places the freshly added nodes; mock
        # it (returns the placed list) so the unbound call doesn't AttributeError
        # before the cache-reset assertions below.
        _gridPlaceNodes=MagicMock(return_value=[]),
        nodeGraph=SimpleNamespace(syncSelectionToPrimTree=False),
        _showPopupMessage=MagicMock(),
        _frameNodeBounds=MagicMock(),
        update=MagicMock(),
    )

    def _fake_add_prim(prim, stage, added_node_ids):
        if added_count_from_add_prim > 0:
            node_id = str(prim.GetPath())
            view.nodes[node_id] = SimpleNamespace(
                id=node_id,
                position=MagicMock(),
                size=MagicMock(),
                selected=False,
            )
            added_node_ids.append(node_id)
            return True
        return False

    view._addPrimAsNode = _fake_add_prim
    return view


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestAddNodePositionCacheReset(unittest.TestCase):
    """Verify resetPositionCaches is called when nodes are added."""

    def test_reset_position_caches_called_after_add(self):
        """Adding a node must reset text position cache to prevent label offset."""
        view = _make_view()
        GraphView.addNodesFromPrimTreeSelection(view)
        view.textRenderer.resetPositionCaches.assert_called_once()

    def test_clear_render_cache_and_reset_called_together(self):
        """Both _clearRenderCache and resetPositionCaches must fire on add."""
        view = _make_view()
        GraphView.addNodesFromPrimTreeSelection(view)
        view._clearRenderCache.assert_called_once()
        view.textRenderer.resetPositionCaches.assert_called_once()

    def test_reset_not_called_when_no_nodes_added(self):
        """If prim is already in graph, no cache reset needed (no new positions)."""
        existing_node = SimpleNamespace(
            id="/Node", position=MagicMock(), size=MagicMock(), selected=False
        )
        view = _make_view(nodes={"/Node": existing_node}, added_count_from_add_prim=0)
        GraphView.addNodesFromPrimTreeSelection(view)
        view.textRenderer.resetPositionCaches.assert_not_called()

    def test_remove_then_readd_calls_reset(self):
        """Remove a node then re-add it: cache must be reset on re-add."""
        view = _make_view()

        # First add
        GraphView.addNodesFromPrimTreeSelection(view)
        self.assertEqual(view.textRenderer.resetPositionCaches.call_count, 1)

        # Simulate remove (like removeSelectedNodes)
        view.nodes.clear()

        # Re-add same prim
        GraphView.addNodesFromPrimTreeSelection(view)
        self.assertEqual(view.textRenderer.resetPositionCaches.call_count, 2)

    def test_node_transform_frame_reset_called_on_add(self):
        """Adding a node must reset the shared NodeTransformFrame.

        The shared frame holds slot assignments, base positions, and accumulated
        move offsets. Without reset, a re-added prim with the same node id would
        inherit the previous owner's slot/offset and render at the wrong position
        (the same root-cause shape as the text-label offset bug above, but for
        the shared transform texture). The mock was wired in the diff that
        introduced NodeTransformFrame; this test pins the actual call.
        """
        view = _make_view()
        GraphView.addNodesFromPrimTreeSelection(view)
        view._nodeTransformFrame.reset.assert_called_once()

    def test_node_transform_frame_reset_not_called_when_no_nodes_added(self):
        """If no node was added (prim already in graph), the shared frame is
        left alone — resetting it would discard the offsets of every other
        moved node in the scene."""
        existing_node = SimpleNamespace(
            id="/Node", position=MagicMock(), size=MagicMock(), selected=False
        )
        view = _make_view(nodes={"/Node": existing_node}, added_count_from_add_prim=0)
        GraphView.addNodesFromPrimTreeSelection(view)
        view._nodeTransformFrame.reset.assert_not_called()


def _make_prim(*, valid=True, has_attr=True, has_value=True, path="/Node"):
    """Build a mock USD prim with a controllable ui:nodegraph:node:pos attr."""
    prim = MagicMock()
    prim.IsValid.return_value = valid
    prim.GetPath.return_value = MagicMock(__str__=lambda s: path)
    if has_attr:
        attr = MagicMock()
        attr.HasValue.return_value = has_value
        prim.GetAttribute.return_value = attr
    else:
        prim.GetAttribute.return_value = None
    return prim


class _RecordingNode:
    """Minimal node that records reads and writes to ``position``.

    In production the setter authors ui:nodegraph:node:pos, while reading the
    getter syncs the C++ NodeData.position that the noodle renderer consumes
    for link endpoints. The respect-authored path must read (to sync) without
    writing (which would overwrite the authored value)."""

    def __init__(self, prim, position_writes):
        self._prim = prim
        self._position_writes = position_writes
        self.id = "/Node"
        self.size = (100.0, 50.0)
        self.zOrder = 0
        self._pos = None
        self.position_reads = 0

    def getUsdPrim(self):
        return self._prim

    @property
    def position(self):
        self.position_reads += 1
        return self._pos

    @position.setter
    def position(self, value):
        self._position_writes.append(value)
        self._pos = value


def _make_position_view(node_has_authored_position, nodes=None):
    """A fake GraphView for exercising _positionNodeInViewport directly."""
    return SimpleNamespace(
        _nodeHasAuthoredPosition=MagicMock(return_value=node_has_authored_position),
        nodes=nodes if nodes is not None else {},
        width=MagicMock(return_value=800),
        height=MagicMock(return_value=600),
        zoom=1.0,
        panX=0.0,
        panY=0.0,
    )


@unittest.skipUnless(_has_pxr, "pxr.UsdNoodles not available")
class TestRespectAuthoredNodePosition(unittest.TestCase):
    """Adding a node whose prim already has an authored ui:nodegraph:node:pos
    must place it at that position rather than authoring a new one."""

    def test_has_authored_position_true(self):
        """A prim with an authored, valued position attr is detected."""
        node = SimpleNamespace(getUsdPrim=MagicMock(return_value=_make_prim()))
        self.assertTrue(GraphView._nodeHasAuthoredPosition(SimpleNamespace(), node))
        node.getUsdPrim.return_value.GetAttribute.assert_called_once_with(
            "ui:nodegraph:node:pos"
        )

    def test_has_authored_position_false_when_attr_has_no_value(self):
        """An attr that exists but holds no value is not an authored position."""
        prim = _make_prim(has_value=False)
        node = SimpleNamespace(getUsdPrim=MagicMock(return_value=prim))
        self.assertFalse(GraphView._nodeHasAuthoredPosition(SimpleNamespace(), node))

    def test_has_authored_position_false_when_attr_missing(self):
        """A prim with no position attr at all has no authored position."""
        prim = _make_prim(has_attr=False)
        node = SimpleNamespace(getUsdPrim=MagicMock(return_value=prim))
        self.assertFalse(GraphView._nodeHasAuthoredPosition(SimpleNamespace(), node))

    def test_has_authored_position_false_when_no_prim(self):
        """A non-USD-backed node (no prim) has no authored position."""
        node = SimpleNamespace(getUsdPrim=MagicMock(return_value=None))
        self.assertFalse(GraphView._nodeHasAuthoredPosition(SimpleNamespace(), node))

    def test_has_authored_position_false_when_prim_invalid(self):
        """An invalid prim short-circuits before touching attributes."""
        prim = _make_prim(valid=False)
        node = SimpleNamespace(getUsdPrim=MagicMock(return_value=prim))
        self.assertFalse(GraphView._nodeHasAuthoredPosition(SimpleNamespace(), node))
        prim.GetAttribute.assert_not_called()

    def test_position_in_viewport_respects_authored_position(self):
        """An authored position is kept, but read once so the cached/C++
        position syncs (the noodle renderer reads the C++ position to place
        link endpoints)."""
        writes = []
        node = _RecordingNode(_make_prim(), writes)
        view = _make_position_view(node_has_authored_position=True)
        GraphView._positionNodeInViewport(view, node)
        view._nodeHasAuthoredPosition.assert_called_once_with(node)
        # Early return: viewport geometry is never consulted (no auto-placement).
        view.width.assert_not_called()
        # The authored position is read (to sync the C++ side) but never
        # rewritten, so the authored ui:nodegraph:node:pos survives.
        self.assertGreaterEqual(node.position_reads, 1)
        self.assertEqual(writes, [])

    def test_position_in_viewport_autoplaces_without_authored_position(self):
        """With no authored position, the node is centered in the viewport."""
        node = SimpleNamespace(size=(100.0, 50.0))
        view = _make_position_view(node_has_authored_position=False)
        GraphView._positionNodeInViewport(view, node)
        # center (400, 300) minus half node size (50, 25)
        self.assertEqual(node.position[0], 350.0)
        self.assertEqual(node.position[1], 275.0)

    def test_position_in_viewport_nudges_to_avoid_overlap(self):
        """Auto-placement still nudges right to avoid overlapping a node."""
        existing = SimpleNamespace(position=(350.0, 275.0), size=(100.0, 50.0))
        node = SimpleNamespace(size=(100.0, 50.0))
        view = _make_position_view(
            node_has_authored_position=False, nodes={"/Existing": existing}
        )
        GraphView._positionNodeInViewport(view, node)
        # Overlaps at the center → shifted to ex + ew + padding = 350 + 100 + 30.
        self.assertEqual(node.position[0], 480.0)

    def test_grid_place_respects_authored_position(self):
        """_gridPlaceNodes leaves a node with an authored (non-origin) position
        untouched — placement was moved out of _addPrimAsNode, and the grid only
        auto-places nodes still sitting at the USD default (0, 0)."""
        node = SimpleNamespace(
            id="/Node",
            position=(123.0, 456.0),
            size=(100.0, 50.0),
            setDisplayPosition=MagicMock(),
        )
        view = SimpleNamespace(
            nodes={"/Node": node},
            _AUTO_GRID_COLUMNS=GraphView._AUTO_GRID_COLUMNS,
            _AUTO_GRID_GAP=GraphView._AUTO_GRID_GAP,
        )

        placed = GraphView._gridPlaceNodes(view, [node])

        # Authored node is not placeable, so nothing is placed or moved and the
        # authored ui:nodegraph:node:pos survives.
        self.assertEqual(placed, [])
        node.setDisplayPosition.assert_not_called()

    def test_grid_place_autoplaces_node_without_authored_position(self):
        """_gridPlaceNodes auto-places a node at the origin (the default for a
        prim with no authored ui:nodegraph:node:pos), display-only so it never
        dirties the stage."""
        node = SimpleNamespace(
            id="/Node",
            position=(0.0, 0.0),
            size=(100.0, 50.0),
            setDisplayPosition=MagicMock(),
        )
        view = SimpleNamespace(
            nodes={"/Node": node},
            _AUTO_GRID_COLUMNS=GraphView._AUTO_GRID_COLUMNS,
            _AUTO_GRID_GAP=GraphView._AUTO_GRID_GAP,
        )

        placed = GraphView._gridPlaceNodes(view, [node])

        # The origin node is placed (display-only) at the world-origin anchor,
        # since nothing else is already positioned.
        self.assertEqual(placed, [node])
        node.setDisplayPosition.assert_called_once()
        arg = node.setDisplayPosition.call_args[0][0]
        self.assertEqual((arg[0], arg[1]), (0.0, 0.0))
