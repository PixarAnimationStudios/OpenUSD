#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit coverage for the graph auto-layout feature.

Two surfaces:

1. The Python binding ``noodles.layoutGraphPositions`` (and ``LayoutParams``) —
   tested directly against ``_noodles.NodeData`` / ``LinkData`` with no GL / no
   USD, locking in the data-edges-only relationship seam at the Python boundary.
2. The ``GraphView`` auto-layout action (``_autoLayoutNodes`` /
   ``_applyNodePositions`` / ``_authorNodePosition`` and the "L" key) — tested as
   unbound methods on ``SimpleNamespace`` fakes with USD/notice/update mocked,
   mirroring testUsdNoodlesGraphViewRelationships.py.
"""

from __future__ import annotations

import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

try:
    from pxr.UsdNoodles import _usdNoodles as _noodles
    from pxr import Gf

    _has_core = True
except ImportError as e:  # pragma: no cover - import guard
    print(f"noodles.core import failed: {e}")
    _has_core = False

try:
    from pxr.UsdNoodles.graphView import GraphView
    from pxr.UsdNoodles.models import NodeModel
    from pxr.Usdviewq.qt import QtCore

    _has_graph = True
except ImportError as e:  # pragma: no cover - import guard
    print(f"GraphView import failed: {e}")
    _has_graph = False


def _make_node(node_id, width=200.0, height=100.0):
    node = _noodles.NodeData()
    node.id = node_id
    node.name = node_id
    node.size = Gf.Vec2d(width, height)
    return node


def _make_link(source, target, is_relationship=False):
    link = _noodles.LinkData()
    link.sourceNodeId = source
    link.sourcePort = "out"
    link.targetNodeId = target
    link.targetPort = "in"
    link.is_relationship_link = is_relationship
    return link


def _no_jitter_params():
    params = _noodles.LayoutParams()
    params.jitterFractionX = 0.0
    params.jitterFractionY = 0.0
    return params


@unittest.skipUnless(_has_core, "noodles.core not available")
class TestLayoutGraphPositionsBinding(unittest.TestCase):
    """Python-level contract for the binding the GraphView action calls."""

    def test_returns_a_position_per_node(self):
        nodes = {"A": _make_node("A"), "B": _make_node("B")}
        result = _noodles.layoutGraphPositions(
            nodes, [_make_link("A", "B")], _no_jitter_params()
        )
        self.assertEqual(set(result.keys()), {"A", "B"})

    def test_data_flow_chain_ranks_left_to_right(self):
        nodes = {n: _make_node(n) for n in ("A", "B", "C")}
        links = [_make_link("A", "B"), _make_link("B", "C")]
        result = _noodles.layoutGraphPositions(nodes, links, _no_jitter_params())
        self.assertLess(result["A"][0], result["B"][0])
        self.assertLess(result["B"][0], result["C"][0])

    def test_relationship_edge_does_not_change_positions(self):
        # The relationship seam: a relationship edge that would merge components
        # or create a back-edge if it were ranked must move no node.
        data_only = _noodles.layoutGraphPositions(
            {n: _make_node(n) for n in ("A", "B", "C", "D")},
            [_make_link("A", "B"), _make_link("C", "D")],
            _no_jitter_params(),
        )
        with_rel = _noodles.layoutGraphPositions(
            {n: _make_node(n) for n in ("A", "B", "C", "D")},
            [
                _make_link("A", "B"),
                _make_link("C", "D"),
                _make_link("B", "C", is_relationship=True),
                _make_link("D", "A", is_relationship=True),
            ],
            _no_jitter_params(),
        )
        for node_id in ("A", "B", "C", "D"):
            self.assertEqual(
                (data_only[node_id][0], data_only[node_id][1]),
                (with_rel[node_id][0], with_rel[node_id][1]),
            )

    def test_relationship_only_nodes_share_a_column(self):
        # Joined only by a relationship link, the two nodes are not data
        # connected, so they stack vertically (same column) instead of chaining.
        result = _noodles.layoutGraphPositions(
            {"A": _make_node("A"), "B": _make_node("B")},
            [_make_link("A", "B", is_relationship=True)],
            _no_jitter_params(),
        )
        self.assertEqual(result["A"][0], result["B"][0])
        self.assertNotEqual(result["A"][1], result["B"][1])


def _fake_node(position):
    return SimpleNamespace(
        position=position,
        _writePositionToUsdRaw=MagicMock(),
        setDisplayPosition=MagicMock(),
    )


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestAutoLayoutAction(unittest.TestCase):
    """The GraphView action layer, with USD / notices / repaint mocked."""

    def test_author_node_position_writes_usd_and_updates_cache(self):
        node = _fake_node(Gf.Vec2d(0.0, 0.0))
        view = SimpleNamespace(nodes={"A": node})
        GraphView._authorNodePosition(view, "A", (10.0, 20.0))
        node._writePositionToUsdRaw.assert_called_once()
        node.setDisplayPosition.assert_called_once()
        written = node._writePositionToUsdRaw.call_args[0][0]
        self.assertEqual((written[0], written[1]), (10.0, 20.0))

    def test_author_node_position_missing_node_is_noop(self):
        view = SimpleNamespace(nodes={})
        GraphView._authorNodePosition(view, "missing", (1.0, 2.0))  # must not raise

    def test_write_position_skips_instance_proxy(self):
        # USD forbids authoring to an instance proxy; the writer must skip it
        # instead of raising, so bulk authoring (auto-layout, save) survives an
        # instanced scene. The display position is updated separately by the
        # caller, so the node still moves on screen.
        prim = SimpleNamespace(
            IsValid=lambda: True,
            GetPath=lambda: SimpleNamespace(name="foo"),
            IsInstanceProxy=lambda: True,
            GetAttribute=MagicMock(),
            CreateAttribute=MagicMock(),
        )
        node = SimpleNamespace(_prim=prim)
        NodeModel._writePositionToUsdRaw(node, Gf.Vec2d(1.0, 2.0))  # must not raise
        prim.GetAttribute.assert_not_called()
        prim.CreateAttribute.assert_not_called()

    def test_reset_render_caches_after_bulk_move(self):
        # The shared helper invalidates every baked vertex cache + drag frame and
        # flags text/links so a bulk move re-bakes all drawables.
        view = SimpleNamespace(
            _clearRenderCache=MagicMock(),
            textRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
            _cppIconRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
            _nodeRenderManager=SimpleNamespace(resetNodeQuadCaches=MagicMock()),
            _nodeTransformFrame=SimpleNamespace(reset=MagicMock()),
            textChanged=False,
            linksChanged=False,
        )
        GraphView._resetRenderCachesAfterBulkMove(view)
        view._clearRenderCache.assert_called_once()
        view.textRenderer.resetPositionCaches.assert_called_once()
        view._cppIconRenderer.resetPositionCaches.assert_called_once()
        view._nodeRenderManager.resetNodeQuadCaches.assert_called_once()
        view._nodeTransformFrame.reset.assert_called_once()
        self.assertTrue(view.textChanged)
        self.assertTrue(view.linksChanged)

    def test_reset_render_caches_skips_link_rebuild_when_requested(self):
        # A pure move (auto-layout) skips the full link rebuild: drawables/text
        # re-bake, but linksChanged stays False (endpoints refresh incrementally).
        view = SimpleNamespace(
            _clearRenderCache=MagicMock(),
            textRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
            _cppIconRenderer=SimpleNamespace(resetPositionCaches=MagicMock()),
            _nodeRenderManager=SimpleNamespace(resetNodeQuadCaches=MagicMock()),
            _nodeTransformFrame=SimpleNamespace(reset=MagicMock()),
            textChanged=False,
            linksChanged=False,
        )
        GraphView._resetRenderCachesAfterBulkMove(view, rebuild_links=False)
        view._clearRenderCache.assert_called_once()
        self.assertTrue(view.textChanged)
        self.assertFalse(view.linksChanged)

    def test_apply_node_positions_batches_and_refreshes(self):
        notice = SimpleNamespace(setEnabled=MagicMock())
        view = SimpleNamespace(
            nodes={"A": _fake_node(Gf.Vec2d(0.0, 0.0))},
            _noticeHandler=notice,
            _authorNodePosition=MagicMock(),
            _resetRenderCachesAfterBulkMove=MagicMock(),
            update=MagicMock(),
        )
        GraphView._applyNodePositions(view, {"A": (5.0, 6.0)})
        view._authorNodePosition.assert_called_once_with("A", (5.0, 6.0))
        # Notice handler is disabled during authoring and re-enabled after.
        self.assertEqual(
            [c.args[0] for c in notice.setEnabled.call_args_list], [False, True]
        )
        # Drawables re-bake here; the spatial index / link endpoints / port
        # highlights are NOT touched by this method -- authoring bumped each
        # node's self-versioning position and the single _reconcileMovedNodes
        # pass applies those before the next paint / hit-test.
        view._resetRenderCachesAfterBulkMove.assert_called_once_with(
            rebuild_links=False
        )
        view.update.assert_called_once()

    def test_auto_layout_applies_positions_and_pushes_single_undo(self):
        view = SimpleNamespace(
            nodes={
                "A": _fake_node(Gf.Vec2d(1.0, 1.0)),
                "B": _fake_node(Gf.Vec2d(2.0, 2.0)),
            },
            links=[],
            _applyNodePositions=MagicMock(),
            frameAll=MagicMock(),
        )
        positions = {"A": Gf.Vec2d(100.0, 0.0), "B": Gf.Vec2d(300.0, 0.0)}
        with (
            patch(
                "pxr.UsdNoodles.graphView.layoutGraphPositions", return_value=positions
            ),
            patch("pxr.UsdNoodles.graphView._push_undo_command") as push,
        ):
            GraphView._autoLayoutNodes(view)
        view._applyNodePositions.assert_called_once()
        applied = view._applyNodePositions.call_args[0][0]
        self.assertEqual(set(applied.keys()), {"A", "B"})
        self.assertEqual(applied["A"], (100.0, 0.0))
        push.assert_called_once()
        self.assertEqual(push.call_args[0][0], "Auto Layout")
        view.frameAll.assert_called_once()

    def test_auto_layout_empty_graph_is_noop(self):
        view = SimpleNamespace(nodes={})
        with patch("pxr.UsdNoodles.graphView.layoutGraphPositions") as layout:
            GraphView._autoLayoutNodes(view)
        layout.assert_not_called()

    def test_auto_layout_no_positions_does_not_apply_or_undo(self):
        view = SimpleNamespace(
            nodes={"A": _fake_node(Gf.Vec2d(0.0, 0.0))},
            links=[],
            _applyNodePositions=MagicMock(),
            frameAll=MagicMock(),
        )
        with (
            patch("pxr.UsdNoodles.graphView.layoutGraphPositions", return_value={}),
            patch("pxr.UsdNoodles.graphView._push_undo_command") as push,
        ):
            GraphView._autoLayoutNodes(view)
        view._applyNodePositions.assert_not_called()
        push.assert_not_called()
        view.frameAll.assert_not_called()

    def test_l_key_triggers_auto_layout(self):
        view = SimpleNamespace(
            nodeCreationHotbox=SimpleNamespace(is_showing=False),
            _autoLayoutNodes=MagicMock(),
        )
        event = SimpleNamespace(
            key=MagicMock(return_value=QtCore.Qt.Key_L),
            modifiers=MagicMock(return_value=QtCore.Qt.NoModifier),
            accept=MagicMock(),
        )
        GraphView.keyPressEvent(view, event)
        view._autoLayoutNodes.assert_called_once()
        event.accept.assert_called_once()


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestLinkSnapshotSyncGating(unittest.TestCase):
    """The per-frame link snapshot sync is gated so steady-state redraws skip the
    O(links) marshal, and every in-place link-field mutation flags a re-sync."""

    def test_sync_skipped_when_nothing_changed(self):
        # Steady-state redraw: hover unchanged and no flagged mutation -> the
        # expensive syncLinksFromModels marshal is skipped entirely.
        graph = SimpleNamespace(syncLinksFromModels=MagicMock())
        view = SimpleNamespace(
            links=[SimpleNamespace(hovered=False)],
            _linkUnderCursor=-1,
            _lastSyncedLinkHover=-1,
            _linksNeedSync=False,
        )
        GraphView._syncLinksIfNeeded(view, graph)
        graph.syncLinksFromModels.assert_not_called()

    def test_sync_runs_and_applies_hover_when_hover_changes(self):
        graph = SimpleNamespace(syncLinksFromModels=MagicMock())
        links = [SimpleNamespace(hovered=False), SimpleNamespace(hovered=False)]
        view = SimpleNamespace(
            links=links,
            _linkUnderCursor=1,
            _lastSyncedLinkHover=-1,
            _linksNeedSync=False,
        )
        GraphView._syncLinksIfNeeded(view, graph)
        graph.syncLinksFromModels.assert_called_once_with(links)
        self.assertFalse(links[0].hovered)
        self.assertTrue(links[1].hovered)  # the hovered index
        self.assertEqual(view._lastSyncedLinkHover, 1)

    def test_sync_runs_when_flagged_dirty_even_if_hover_unchanged(self):
        graph = SimpleNamespace(syncLinksFromModels=MagicMock())
        links = [SimpleNamespace(hovered=False)]
        view = SimpleNamespace(
            links=links,
            _linkUnderCursor=-1,
            _lastSyncedLinkHover=-1,
            _linksNeedSync=True,
        )
        GraphView._syncLinksIfNeeded(view, graph)
        graph.syncLinksFromModels.assert_called_once_with(links)
        self.assertFalse(view._linksNeedSync)  # flag cleared after sync

    def test_clear_link_selection_flags_resync(self):
        link = SimpleNamespace(selected=True)
        view = SimpleNamespace(links=[link], _selectedLinks={0}, _linksNeedSync=False)
        GraphView.clearLinkSelection(view)
        self.assertFalse(link.selected)
        self.assertTrue(view._linksNeedSync)

    def test_select_link_flags_resync(self):
        link = SimpleNamespace(selected=False)
        view = SimpleNamespace(
            links=[link],
            _selectedLinks=set(),
            _linksNeedSync=False,
            _syncLinkSelectionToPrimtree=MagicMock(),
            update=MagicMock(),
        )
        GraphView._selectLink(view, 0)
        self.assertTrue(link.selected)
        self.assertTrue(view._linksNeedSync)

    def test_toggle_link_selection_flags_resync(self):
        link = SimpleNamespace(selected=False)
        view = SimpleNamespace(
            links=[link],
            _selectedLinks=set(),
            _linksNeedSync=False,
            _syncLinkSelectionToPrimtree=MagicMock(),
            update=MagicMock(),
        )
        GraphView._toggleLinkSelection(view, 0)
        self.assertTrue(link.selected)
        self.assertTrue(view._linksNeedSync)

    def test_update_links_for_moved_nodes_flags_resync(self):
        # A pure endpoint move (drag / auto-layout) flags a re-sync so paintLinks
        # mirrors the moved endpoints without a full rebuild.
        view = SimpleNamespace(links=[], _linksNeedSync=False)
        GraphView._updateLinksForMovedNodes(view, {"A"})
        self.assertTrue(view._linksNeedSync)

    def test_update_node_spatial_bounds_reindexes_moved_node(self):
        # Hit-testing reads the spatial index; a moved node's bounds must be
        # updated there from its (position, position+size) box.
        node = SimpleNamespace(
            position=Gf.Vec2d(100.0, 200.0), size=Gf.Vec2d(30.0, 40.0)
        )
        spatial = SimpleNamespace(updateNode=MagicMock())
        view = SimpleNamespace(nodes={"A": node}, _spatialIndex=spatial)
        GraphView._updateNodeSpatialBounds(view, {"A"})
        spatial.updateNode.assert_called_once()
        called_id, bounds = spatial.updateNode.call_args[0]
        self.assertEqual(called_id, "A")
        self.assertEqual((bounds.GetMin()[0], bounds.GetMin()[1]), (100.0, 200.0))
        self.assertEqual((bounds.GetMax()[0], bounds.GetMax()[1]), (130.0, 240.0))

    def test_update_node_spatial_bounds_skips_missing_node(self):
        spatial = SimpleNamespace(updateNode=MagicMock())
        view = SimpleNamespace(nodes={}, _spatialIndex=spatial)
        GraphView._updateNodeSpatialBounds(view, {"gone"})  # must not raise
        spatial.updateNode.assert_not_called()


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestReconcileMovedNodes(unittest.TestCase):
    """The single position-change consumer: version-driven, no caller flags, no
    flag to clear. This is the safety-by-design core -- moving a node and
    forgetting a derived update is structurally impossible."""

    def _view(self, nodes):
        return SimpleNamespace(
            nodes=nodes,
            draggingNodes=False,
            _reconcilePositionGen=0,
            _reconciledPositionVersions={},
            _applyMovedNodeDerivedState=MagicMock(),
        )

    def test_skips_in_o1_when_generation_unchanged(self):
        # Nothing moved since the last reconcile -> no per-node scan at all.
        view = self._view({"A": SimpleNamespace(positionVersion=1)})
        view._reconcilePositionGen = 7
        with patch("pxr.UsdNoodles.graphView.positionWriteGeneration", return_value=7):
            GraphView._reconcileMovedNodes(view)
        view._applyMovedNodeDerivedState.assert_not_called()

    def test_skips_during_drag(self):
        # A drag refreshes derived state inline; the reconcile stays out of the way.
        view = self._view({"A": SimpleNamespace(positionVersion=1)})
        view.draggingNodes = True
        with patch("pxr.UsdNoodles.graphView.positionWriteGeneration", return_value=99):
            GraphView._reconcileMovedNodes(view)
        view._applyMovedNodeDerivedState.assert_not_called()

    def test_reconciles_only_version_changed_nodes(self):
        view = self._view(
            {
                "A": SimpleNamespace(positionVersion=3),
                "B": SimpleNamespace(positionVersion=0),
            }
        )
        # A advanced past its watermark; B is already current.
        view._reconciledPositionVersions = {"A": 1, "B": 0}
        with patch("pxr.UsdNoodles.graphView.positionWriteGeneration", return_value=5):
            GraphView._reconcileMovedNodes(view)
        # Routes the changed set through the single derived-state definition.
        view._applyMovedNodeDerivedState.assert_called_once_with({"A"})
        # Watermark advanced so the next frame won't reprocess A.
        self.assertEqual(view._reconciledPositionVersions["A"], 3)
        self.assertEqual(view._reconcilePositionGen, 5)

    def test_records_new_node_first_seen(self):
        # A node never reconciled before (no watermark) is treated as changed.
        view = self._view({"A": SimpleNamespace(positionVersion=1)})
        with patch("pxr.UsdNoodles.graphView.positionWriteGeneration", return_value=2):
            GraphView._reconcileMovedNodes(view)
        view._applyMovedNodeDerivedState.assert_called_once_with({"A"})

    def test_advances_generation_when_no_node_changed(self):
        # The global gen moved (some write happened) but every node is already at
        # its watermark -> no derived updates, and gen is recorded so the next
        # frame short-circuits instead of rescanning.
        view = self._view({"A": SimpleNamespace(positionVersion=2)})
        view._reconciledPositionVersions = {"A": 2}
        with patch("pxr.UsdNoodles.graphView.positionWriteGeneration", return_value=5):
            GraphView._reconcileMovedNodes(view)
        # Delegates with an empty set (the helper no-ops); the point is that the
        # generation is recorded so the next frame short-circuits.
        view._applyMovedNodeDerivedState.assert_called_once_with(set())
        self.assertEqual(view._reconcilePositionGen, 5)


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestApplyMovedNodeDerivedState(unittest.TestCase):
    """The single definition of position-derived state, shared by the per-frame
    reconcile and the live drag: spatial bounds + link endpoints + port
    highlights. Adding a consumer here covers every mover at once."""

    def _view(self):
        return SimpleNamespace(
            _updateNodeSpatialBounds=MagicMock(),
            _updateLinksForMovedNodes=MagicMock(),
            _portHighlightsDirty=False,
        )

    def test_refreshes_all_position_derived_state(self):
        view = self._view()
        GraphView._applyMovedNodeDerivedState(view, {"A", "B"})
        view._updateNodeSpatialBounds.assert_called_once_with({"A", "B"})
        view._updateLinksForMovedNodes.assert_called_once_with({"A", "B"})
        self.assertTrue(view._portHighlightsDirty)

    def test_empty_set_is_noop(self):
        view = self._view()
        GraphView._applyMovedNodeDerivedState(view, set())
        view._updateNodeSpatialBounds.assert_not_called()
        view._updateLinksForMovedNodes.assert_not_called()
        self.assertFalse(view._portHighlightsDirty)


@unittest.skipUnless(_has_graph, "GraphView not available")
class TestAutoLayoutTopCenterContract(unittest.TestCase):
    """A whole-prim relationship link still anchors at the moved node's
    top-center after layout authors a new position (the contract the layout is
    designed to preserve)."""

    def test_relationship_endpoint_tracks_top_center_after_move(self):
        node = SimpleNamespace(
            position=Gf.Vec2d(10.0, 12.0),
            size=Gf.Vec2d(30.0, 40.0),
        )
        # Whole-prim relationship target: empty pin AND empty property name.
        before = GraphView._getNodeTopCenterPosition(node)
        self.assertEqual((before[0], before[1]), (25.0, 12.0))  # 10 + 30*0.5, 12

        # Simulate the layout authoring a new position.
        node.position = Gf.Vec2d(100.0, 200.0)
        after = GraphView._getNodeTopCenterPosition(node)
        self.assertEqual((after[0], after[1]), (115.0, 200.0))  # 100 + 30*0.5, 200
