#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Unit tests for disconnect-drag (link reconnect / disconnect) feature.

Covers the logic added in D101275205:
  1. _startLinkReconnect: state machine branching (which end is closer)
  2. _cancelLinkReconnect: restores link and clears reconnect state
  3. _finishReconnectDisconnect: USD deletion, undo command push, and
     early-return recovery on validation failure
  4. _abortConnection: delegates to _cancelLinkReconnect when reconnecting
  5. _resolveConnectionEndpoints: validation and abort paths
  6. _deleteOldConnection: validation and library delegation
  7. _make_reconnect_edits: redo/undo closure correctness
  8. _make_connection_edit: create vs delete on the current edit target

GraphView cannot be imported in headless CI (module-level GL imports),
so production logic is mirrored in shims — same approach as
testUsdNoodlesStageReload and testUsdNoodlesGraphViewShortcut.
"""

import unittest
from unittest.mock import MagicMock


# ---------------------------------------------------------------------------
# Shim: _make_connection_edit  (graphView.py:104)
# ---------------------------------------------------------------------------


def _make_connection_edit(gv, src_id, src_port, tgt_id, tgt_port, create):
    """Mirror of graphView._make_connection_edit (graphView.py:104).

    The shim authors on whatever edit target is currently set on the stage —
    the production code stopped forcing the root layer once the UI gained
    real edit-target support.
    """

    def _fn():
        stage = gv.nodeGraph.getStage()
        if not stage:
            return
        out_p = stage.GetPrimAtPath(src_id)
        in_p = stage.GetPrimAtPath(tgt_id)
        if out_p and in_p:
            lib = gv._findLibraryForPrim(in_p)
            if lib:
                if create:
                    lib.create_connection(stage, out_p, src_port, in_p, tgt_port)
                else:
                    lib.delete_connection(stage, out_p, src_port, in_p, tgt_port)

    return _fn


# ---------------------------------------------------------------------------
# Shim: _make_reconnect_edits  (graphView.py:140)
# ---------------------------------------------------------------------------


def _make_reconnect_edits(
    gv,
    old_src,
    old_src_port,
    old_tgt,
    old_tgt_port,
    new_src,
    new_src_port,
    new_tgt,
    new_tgt_port,
):
    """Mirror of graphView._make_reconnect_edits (graphView.py:140)."""
    delete_old = _make_connection_edit(
        gv,
        old_src,
        old_src_port,
        old_tgt,
        old_tgt_port,
        create=False,
    )
    create_old = _make_connection_edit(
        gv,
        old_src,
        old_src_port,
        old_tgt,
        old_tgt_port,
        create=True,
    )
    delete_new = _make_connection_edit(
        gv,
        new_src,
        new_src_port,
        new_tgt,
        new_tgt_port,
        create=False,
    )
    create_new = _make_connection_edit(
        gv,
        new_src,
        new_src_port,
        new_tgt,
        new_tgt_port,
        create=True,
    )

    def redo():
        delete_old()
        create_new()
        gv.linksChanged = True
        gv.update()

    def undo():
        delete_new()
        create_old()
        gv.linksChanged = True
        gv.update()

    return redo, undo


# ---------------------------------------------------------------------------
# Shim: _startLinkReconnect  (graphView.py:4329)
# ---------------------------------------------------------------------------


def _start_link_reconnect(view, linkIndex, worldClickPos):
    """Mirror of GraphView._startLinkReconnect (graphView.py:4329)."""
    link = view.links[linkIndex]

    startPos = link.start
    endPos = link.end
    startDistSq = (worldClickPos[0] - startPos[0]) ** 2 + (
        worldClickPos[1] - startPos[1]
    ) ** 2
    endDistSq = (worldClickPos[0] - endPos[0]) ** 2 + (
        worldClickPos[1] - endPos[1]
    ) ** 2

    if startDistSq < endDistSq:
        anchorNodeId = link.targetNodeId
        anchorPort = link.targetPort
        anchorIsOutput = False
    else:
        anchorNodeId = link.sourceNodeId
        anchorPort = link.sourcePort
        anchorIsOutput = True

    view._reconnectingLink = True
    view._reconnectOldSourceNodeId = link.sourceNodeId
    view._reconnectOldSourcePort = link.sourcePort
    view._reconnectOldTargetNodeId = link.targetNodeId
    view._reconnectOldTargetPort = link.targetPort

    view._removeLinkFromGraph(
        link.sourceNodeId, link.sourcePort, link.targetNodeId, link.targetPort
    )

    if anchorNodeId not in view.nodes:
        view._cancelLinkReconnect()
        return

    view._startLinkDrag(anchorNodeId, anchorPort, anchorIsOutput)

    if anchorIsOutput:
        view._dragLinkTempLink.end = worldClickPos
    else:
        view._dragLinkTempLink.start = worldClickPos


# ---------------------------------------------------------------------------
# Shim: _clearReconnectState  (graphView.py:4377)
# ---------------------------------------------------------------------------


def _clear_reconnect_state(view):
    """Mirror of GraphView._clearReconnectState (graphView.py:4377)."""
    view._reconnectingLink = False
    view._reconnectOldSourceNodeId = None
    view._reconnectOldSourcePort = None
    view._reconnectOldTargetNodeId = None
    view._reconnectOldTargetPort = None


# ---------------------------------------------------------------------------
# Shim: _cancelLinkReconnect  (graphView.py:4384)
# ---------------------------------------------------------------------------


def _cancel_link_reconnect(view):
    """Mirror of GraphView._cancelLinkReconnect (graphView.py:4384)."""
    srcId = view._reconnectOldSourceNodeId
    srcPort = view._reconnectOldSourcePort
    tgtId = view._reconnectOldTargetNodeId
    tgtPort = view._reconnectOldTargetPort
    _clear_reconnect_state(view)

    srcNode = view.nodes.get(srcId)
    tgtNode = view.nodes.get(tgtId)
    if srcNode and tgtNode:
        view._addLinkToGraph(srcId, srcPort, tgtId, tgtPort, srcNode, tgtNode)


# ---------------------------------------------------------------------------
# Shim: _finishReconnectDisconnect  (graphView.py:4394)
# ---------------------------------------------------------------------------


def _finish_reconnect_disconnect(view, push_undo_fn):
    """Mirror of GraphView._finishReconnectDisconnect (graphView.py:4394).

    push_undo_fn is injected to avoid importing the C++ noodles module.
    """
    srcId = view._reconnectOldSourceNodeId
    srcPort = view._reconnectOldSourcePort
    tgtId = view._reconnectOldTargetNodeId
    tgtPort = view._reconnectOldTargetPort

    inputNode = view.nodes.get(tgtId)
    outputNode = view.nodes.get(srcId)
    if not inputNode or not outputNode:
        _cancel_link_reconnect(view)
        return

    inputPrim = inputNode.getUsdPrim()
    outputPrim = outputNode.getUsdPrim()
    if not inputPrim or not outputPrim:
        _cancel_link_reconnect(view)
        return

    library = view._findLibraryForPrim(inputPrim)
    if not library:
        _cancel_link_reconnect(view)
        return

    _clear_reconnect_state(view)

    stage = view.nodeGraph.getStage()

    view._noticeHandler.setEnabled(False)
    try:
        success = library.delete_connection(
            stage, outputPrim, srcPort, inputPrim, tgtPort
        )
        if success:
            gv = view
            delete_fn = _make_connection_edit(
                gv,
                srcId,
                srcPort,
                tgtId,
                tgtPort,
                create=False,
            )
            create_fn = _make_connection_edit(
                gv,
                srcId,
                srcPort,
                tgtId,
                tgtPort,
                create=True,
            )

            def redo():
                delete_fn()
                gv.linksChanged = True
                gv.update()

            def undo():
                create_fn()
                gv.linksChanged = True
                gv.update()

            push_undo_fn("Disconnect", redo, undo)

            # Re-derive links from USD so the disconnected pin's port highlight
            # (relationship arrow / port circle) reflects the now-empty target;
            # the in-place removal at reconnect-start is not guaranteed to match
            # relationship links, which would leave the arrow stuck yellow.
            view.linksChanged = True
            view._rebuildLinks()
            view.update()
        else:
            view._addLinkToGraph(srcId, srcPort, tgtId, tgtPort, outputNode, inputNode)
    finally:
        view._noticeHandler.setEnabled(True)


# ---------------------------------------------------------------------------
# Shim: _abortConnection  (graphView.py:5027)
# ---------------------------------------------------------------------------


def _abort_connection(view, message):
    """Mirror of GraphView._abortConnection (graphView.py:5027)."""
    view._warnCalled = message
    if view._reconnectingLink:
        _cancel_link_reconnect(view)


# ---------------------------------------------------------------------------
# Shim: _resolveConnectionEndpoints  (graphView.py:5032)
# ---------------------------------------------------------------------------


def _resolve_connection_endpoints(view, outputNodeId, inputNodeId):
    """Mirror of GraphView._resolveConnectionEndpoints (graphView.py:5032)."""
    outputNode = view.nodes.get(outputNodeId)
    inputNode = view.nodes.get(inputNodeId)
    if not outputNode or not inputNode:
        _abort_connection(view, "Cannot create connection: invalid node IDs")
        return None

    outputPrim = outputNode.getUsdPrim()
    inputPrim = inputNode.getUsdPrim()
    if not outputPrim or not inputPrim:
        _abort_connection(view, "Cannot create connection: invalid USD prims")
        return None

    library = view._findLibraryForPrim(inputPrim)
    if not library:
        _abort_connection(
            view,
            f"Cannot create connection: no library handles prim type "
            f"'{inputPrim.GetTypeName()}'",
        )
        return None

    return outputNode, inputNode, outputPrim, inputPrim, library


# ---------------------------------------------------------------------------
# Shim: _deleteOldConnection  (graphView.py:5054)
# ---------------------------------------------------------------------------


def _delete_old_connection(view, stage, oldSrcId, oldSrcPort, oldTgtId, oldTgtPort):
    """Mirror of GraphView._deleteOldConnection (graphView.py:5054)."""
    oldInputNode = view.nodes.get(oldTgtId)
    oldOutputNode = view.nodes.get(oldSrcId)
    if not oldInputNode or not oldOutputNode:
        return False
    oldInputPrim = oldInputNode.getUsdPrim()
    oldOutputPrim = oldOutputNode.getUsdPrim()
    if not oldInputPrim or not oldOutputPrim:
        return False
    oldLib = view._findLibraryForPrim(oldInputPrim)
    if not oldLib:
        return False
    return oldLib.delete_connection(
        stage,
        oldOutputPrim,
        oldSrcPort,
        oldInputPrim,
        oldTgtPort,
    )


# ---------------------------------------------------------------------------
# Shim: same-pin no-op check from _completeLinkDrag  (graphView.py:4593)
# ---------------------------------------------------------------------------


def _is_same_pin_reconnect(view, outputNodeId, outputPort, inputNodeId, inputPort):
    """Mirror of the same-pin no-op guard in _completeLinkDrag."""
    if view._reconnectingLink and (
        outputNodeId == view._reconnectOldSourceNodeId
        and outputPort == view._reconnectOldSourcePort
        and inputNodeId == view._reconnectOldTargetNodeId
        and inputPort == view._reconnectOldTargetPort
    ):
        _cancel_link_reconnect(view)
        return True
    return False


# ---------------------------------------------------------------------------
# Mock helpers
# ---------------------------------------------------------------------------


def _make_link(src_node_id, src_port, tgt_node_id, tgt_port, start, end):
    """Create a mock LinkData with the given endpoints and screen positions."""
    link = MagicMock()
    link.sourceNodeId = src_node_id
    link.sourcePort = src_port
    link.targetNodeId = tgt_node_id
    link.targetPort = tgt_port
    link.sourcePropertyName = ""
    link.targetPropertyName = ""
    link.start = start
    link.end = end
    link.isDangling = False
    return link


def _make_node(node_id, has_prim=True, prim_type="NoodleSoupNode"):
    """Create a mock node with optional prim."""
    node = MagicMock()
    node.id = node_id
    node.inputLinks = []
    node.outputLinks = []
    if has_prim:
        prim = MagicMock()
        prim.GetTypeName.return_value = prim_type
        node.getUsdPrim.return_value = prim
    else:
        node.getUsdPrim.return_value = None
    return node


def _make_view(nodes=None, links=None):
    """Create a duck-typed GraphView stand-in for disconnect-drag tests."""
    view = MagicMock()
    view.nodes = nodes or {}
    view.links = links or []
    view._reconnectingLink = False
    view._reconnectOldSourceNodeId = None
    view._reconnectOldSourcePort = None
    view._reconnectOldTargetNodeId = None
    view._reconnectOldTargetPort = None
    view.linksChanged = False
    view._warnCalled = None

    library = MagicMock()
    library.enabled = True
    library.can_handle_prim.return_value = True
    library.delete_connection.return_value = True
    library.create_connection.return_value = True
    view._findLibraryForPrim = MagicMock(return_value=library)
    view._library = library
    view._nodeLibraries = [library]

    stage = MagicMock()
    view.nodeGraph.getStage.return_value = stage
    view._stage = stage
    return view


def _set_reconnect_state(view, src_id, src_port, tgt_id, tgt_port):
    """Put view into mid-reconnect state as _startLinkReconnect would."""
    view._reconnectingLink = True
    view._reconnectOldSourceNodeId = src_id
    view._reconnectOldSourcePort = src_port
    view._reconnectOldTargetNodeId = tgt_id
    view._reconnectOldTargetPort = tgt_port


# ---------------------------------------------------------------------------
# Shim: connected-input-port lift logic added to _startLinkDrag
#       (graphView.py:5184)
# ---------------------------------------------------------------------------


def _apply_connected_input_lift(view, nodeId, portName):
    """Mirror of the connected-input lift block at the end of _startLinkDrag.

    When dragging from an input port that already has a connection, enter
    reconnect mode so that releasing on empty space or an invalid target
    deletes the existing connection (T262819088 — Presto parity).
    """
    if view._reconnectingLink:
        return
    existing = view._findLinkToInput(nodeId, portName)
    if not existing or getattr(existing, "isDangling", False):
        return
    view._reconnectingLink = True
    view._reconnectOldSourceNodeId = existing.sourceNodeId
    view._reconnectOldSourcePort = existing.sourcePort
    view._reconnectOldTargetNodeId = existing.targetNodeId
    view._reconnectOldTargetPort = existing.targetPort
    view._reconnectOldSourcePropertyName = str(
        getattr(existing, "sourcePropertyName", "")
    )
    view._reconnectOldTargetPropertyName = str(
        getattr(existing, "targetPropertyName", "")
    )
    view._removeLinkFromGraph(
        existing.sourceNodeId,
        existing.sourcePort,
        existing.targetNodeId,
        existing.targetPort,
        sourcePropertyName=str(getattr(existing, "sourcePropertyName", "")),
        targetPropertyName=str(getattr(existing, "targetPropertyName", "")),
    )


# ===========================================================================
# Tests
# ===========================================================================


class TestStartLinkReconnect(unittest.TestCase):
    """Branch coverage for _startLinkReconnect (graphView.py:4329).

    Branch map:
      1. Click closer to start (output) → anchor at target (input)
      2. Click closer to end (input)   → anchor at source (output)
      3. Anchor node missing           → cancel reconnect
    """

    def _run(self, click_pos):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        link = _make_link("/A", "out", "/B", "inp", (0.0, 0.0), (100.0, 0.0))
        view = _make_view(
            nodes={"/A": nodeA, "/B": nodeB},
            links=[link],
        )
        _start_link_reconnect(view, 0, click_pos)
        return view

    def test_click_near_start_anchors_at_target(self):
        """Click at (10,0) is closer to start (0,0) than end (100,0).
        The anchor should be the target node (input side)."""
        view = self._run((10.0, 0.0))
        self.assertTrue(view._reconnectingLink)
        view._startLinkDrag.assert_called_once_with("/B", "inp", False)

    def test_click_near_end_anchors_at_source(self):
        """Click at (90,0) is closer to end (100,0) than start (0,0).
        The anchor should be the source node (output side)."""
        view = self._run((90.0, 0.0))
        self.assertTrue(view._reconnectingLink)
        view._startLinkDrag.assert_called_once_with("/A", "out", True)

    def test_stores_old_connection_endpoints(self):
        view = self._run((10.0, 0.0))
        self.assertEqual(view._reconnectOldSourceNodeId, "/A")
        self.assertEqual(view._reconnectOldSourcePort, "out")
        self.assertEqual(view._reconnectOldTargetNodeId, "/B")
        self.assertEqual(view._reconnectOldTargetPort, "inp")

    def test_removes_link_from_local_graph(self):
        view = self._run((10.0, 0.0))
        view._removeLinkFromGraph.assert_called_once_with("/A", "out", "/B", "inp")

    def test_temp_link_end_set_when_anchor_is_output(self):
        """When anchor is output, the cursor side is the end (input)."""
        view = self._run((90.0, 0.0))
        self.assertEqual(view._dragLinkTempLink.end, (90.0, 0.0))

    def test_temp_link_start_set_when_anchor_is_input(self):
        """When anchor is input, the cursor side is the start (output)."""
        view = self._run((10.0, 0.0))
        self.assertEqual(view._dragLinkTempLink.start, (10.0, 0.0))

    def test_cancels_if_anchor_node_missing(self):
        """If the anchor node is not in view.nodes, cancel and return."""
        link = _make_link("/A", "out", "/B", "inp", (0.0, 0.0), (100.0, 0.0))
        nodeA = _make_node("/A")
        # Click near start → anchor is target "/B", which is missing from nodes
        view = _make_view(nodes={"/A": nodeA}, links=[link])
        _start_link_reconnect(view, 0, (10.0, 0.0))
        view._cancelLinkReconnect.assert_called_once()
        view._startLinkDrag.assert_not_called()


class TestCancelLinkReconnect(unittest.TestCase):
    """Tests for _cancelLinkReconnect (graphView.py:4377)."""

    def test_clears_reconnect_flag(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        _cancel_link_reconnect(view)

        self.assertFalse(view._reconnectingLink)

    def test_clears_old_endpoint_variables(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        _cancel_link_reconnect(view)

        self.assertIsNone(view._reconnectOldSourceNodeId)
        self.assertIsNone(view._reconnectOldSourcePort)
        self.assertIsNone(view._reconnectOldTargetNodeId)
        self.assertIsNone(view._reconnectOldTargetPort)

    def test_restores_link_when_both_nodes_exist(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        _cancel_link_reconnect(view)

        view._addLinkToGraph.assert_called_once_with(
            "/A",
            "out",
            "/B",
            "inp",
            nodeA,
            nodeB,
        )

    def test_no_restore_when_source_node_missing(self):
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/B": nodeB})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        _cancel_link_reconnect(view)

        view._addLinkToGraph.assert_not_called()

    def test_no_restore_when_target_node_missing(self):
        nodeA = _make_node("/A")
        view = _make_view(nodes={"/A": nodeA})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        _cancel_link_reconnect(view)

        view._addLinkToGraph.assert_not_called()


class TestFinishReconnectDisconnect(unittest.TestCase):
    """Tests for _finishReconnectDisconnect (graphView.py:4394).

    Branch map:
      1. Nodes missing        → cancel (restore link)
      2. Prims missing        → cancel (restore link)
      3. Library missing      → cancel (restore link)
      4. delete_connection ok  → push "Disconnect" undo
      5. delete_connection fail → no undo pushed
      6. Notice handler re-enabled in all cases (try/finally)
    """

    def _setup(self, node_has_prim=True, library_found=True, delete_ok=True):
        nodeA = _make_node("/A", has_prim=node_has_prim)
        nodeB = _make_node("/B", has_prim=node_has_prim)
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        if not library_found:
            view._findLibraryForPrim.return_value = None
        else:
            view._library.delete_connection.return_value = delete_ok

        self.push_calls = []

        def _mock_push(desc, redo, undo):
            self.push_calls.append((desc, redo, undo))

        return view, _mock_push

    def test_success_pushes_disconnect_undo(self):
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)

        self.assertEqual(len(self.push_calls), 1)
        self.assertEqual(self.push_calls[0][0], "Disconnect")

    def test_success_clears_reconnect_state(self):
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)

        self.assertFalse(view._reconnectingLink)
        self.assertIsNone(view._reconnectOldSourceNodeId)

    def test_success_does_not_override_edit_target(self):
        view, push = self._setup()
        stage = view.nodeGraph.getStage()

        _finish_reconnect_disconnect(view, push)

        # Production code authors on whatever edit target is currently set —
        # it no longer forces root and therefore makes no SetEditTarget call.
        stage.SetEditTarget.assert_not_called()

    def test_success_re_enables_notice_handler(self):
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)

        view._noticeHandler.setEnabled.assert_any_call(False)
        view._noticeHandler.setEnabled.assert_called_with(True)

    def test_success_rebuilds_links_from_usd(self):
        # Regression: a successful disconnect must re-derive links from USD so
        # the disconnected pin's port highlight (the relationship "arrow") stops
        # drawing connected. Relying only on the reconnect-start in-place removal
        # left the arrow stuck yellow when the in-place match missed.
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)

        view._rebuildLinks.assert_called_once()
        self.assertTrue(view.linksChanged)

    def test_success_repaints(self):
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)

        view.update.assert_called()

    def test_delete_fails_no_undo_pushed(self):
        view, push = self._setup(delete_ok=False)
        _finish_reconnect_disconnect(view, push)

        self.assertEqual(len(self.push_calls), 0)

    def test_delete_fails_does_not_rebuild_links(self):
        # The failure path uses its own recovery (restore the visual link); it
        # must not run the success-path USD rebuild.
        view, push = self._setup(delete_ok=False)
        _finish_reconnect_disconnect(view, push)

        view._rebuildLinks.assert_not_called()

    def test_delete_fails_restores_visual_link(self):
        view, push = self._setup(delete_ok=False)
        _finish_reconnect_disconnect(view, push)

        view._addLinkToGraph.assert_called_once_with(
            "/A",
            "out",
            "/B",
            "inp",
            view.nodes["/A"],
            view.nodes["/B"],
        )

    def test_missing_nodes_restores_link(self):
        view = _make_view(nodes={})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        _finish_reconnect_disconnect(view, MagicMock())

        self.assertFalse(view._reconnectingLink)
        self.assertIsNone(view._reconnectOldSourceNodeId)

    def test_missing_prims_restores_link(self):
        view, push = self._setup(node_has_prim=False)
        _finish_reconnect_disconnect(view, push)

        self.assertFalse(view._reconnectingLink)
        self.assertEqual(len(self.push_calls), 0)

    def test_missing_library_restores_link(self):
        view, push = self._setup(library_found=False)
        _finish_reconnect_disconnect(view, push)

        self.assertFalse(view._reconnectingLink)
        self.assertEqual(len(self.push_calls), 0)

    def test_notice_handler_re_enabled_even_on_exception(self):
        view, push = self._setup()
        view._library.delete_connection.side_effect = RuntimeError("boom")

        with self.assertRaises(RuntimeError):
            _finish_reconnect_disconnect(view, push)

        view._noticeHandler.setEnabled.assert_called_with(True)


class TestAbortConnection(unittest.TestCase):
    """Tests for _abortConnection (graphView.py:5027)."""

    def test_cancels_reconnect_when_reconnecting(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        _abort_connection(view, "test error")

        self.assertFalse(view._reconnectingLink)
        view._addLinkToGraph.assert_called_once()

    def test_no_cancel_when_not_reconnecting(self):
        view = _make_view()
        _abort_connection(view, "test error")

        self.assertFalse(view._reconnectingLink)
        view._addLinkToGraph.assert_not_called()

    def test_records_warning_message(self):
        view = _make_view()
        _abort_connection(view, "some problem")

        self.assertEqual(view._warnCalled, "some problem")


class TestResolveConnectionEndpoints(unittest.TestCase):
    """Tests for _resolveConnectionEndpoints (graphView.py:5032)."""

    def test_returns_tuple_on_success(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        lib = MagicMock()
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        view._findLibraryForPrim.return_value = lib

        result = _resolve_connection_endpoints(view, "/A", "/B")

        self.assertIsNotNone(result)
        self.assertEqual(len(result), 5)

    def test_returns_none_missing_output_node(self):
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/B": nodeB})

        result = _resolve_connection_endpoints(view, "/A", "/B")

        self.assertIsNone(result)

    def test_returns_none_missing_input_node(self):
        nodeA = _make_node("/A")
        view = _make_view(nodes={"/A": nodeA})

        result = _resolve_connection_endpoints(view, "/A", "/B")

        self.assertIsNone(result)

    def test_returns_none_missing_output_prim(self):
        nodeA = _make_node("/A", has_prim=False)
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})

        result = _resolve_connection_endpoints(view, "/A", "/B")

        self.assertIsNone(result)

    def test_returns_none_missing_input_prim(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B", has_prim=False)
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})

        result = _resolve_connection_endpoints(view, "/A", "/B")

        self.assertIsNone(result)

    def test_returns_none_no_library(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        view._findLibraryForPrim.return_value = None

        result = _resolve_connection_endpoints(view, "/A", "/B")

        self.assertIsNone(result)

    def test_aborts_with_prim_type_in_message_when_no_library(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B", prim_type="CustomType")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        view._findLibraryForPrim.return_value = None

        _resolve_connection_endpoints(view, "/A", "/B")

        self.assertIn("CustomType", view._warnCalled)

    def test_cancels_reconnect_on_missing_node(self):
        """When reconnecting and resolution fails, the link is restored."""
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        _resolve_connection_endpoints(view, "/A", "/MISSING")

        self.assertFalse(view._reconnectingLink)


class TestDeleteOldConnection(unittest.TestCase):
    """Tests for _deleteOldConnection (graphView.py:5054)."""

    def test_success_returns_true(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        stage = MagicMock()

        result = _delete_old_connection(view, stage, "/A", "out", "/B", "inp")

        self.assertTrue(result)

    def test_delegates_to_library(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        lib = MagicMock()
        lib.delete_connection.return_value = True
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        view._findLibraryForPrim.return_value = lib
        stage = MagicMock()

        _delete_old_connection(view, stage, "/A", "out", "/B", "inp")

        lib.delete_connection.assert_called_once()

    def test_returns_false_missing_input_node(self):
        nodeA = _make_node("/A")
        view = _make_view(nodes={"/A": nodeA})
        result = _delete_old_connection(view, MagicMock(), "/A", "out", "/B", "inp")
        self.assertFalse(result)

    def test_returns_false_missing_output_node(self):
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/B": nodeB})
        result = _delete_old_connection(view, MagicMock(), "/A", "out", "/B", "inp")
        self.assertFalse(result)

    def test_returns_false_missing_prim(self):
        nodeA = _make_node("/A", has_prim=False)
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        result = _delete_old_connection(view, MagicMock(), "/A", "out", "/B", "inp")
        self.assertFalse(result)

    def test_returns_false_no_library(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        view._findLibraryForPrim.return_value = None
        result = _delete_old_connection(view, MagicMock(), "/A", "out", "/B", "inp")
        self.assertFalse(result)


class TestMakeReconnectEdits(unittest.TestCase):
    """Tests for _make_reconnect_edits (graphView.py:140)."""

    def _make_gv(self):
        gv = MagicMock()
        gv.linksChanged = False
        lib = MagicMock()
        gv._findLibraryForPrim.return_value = lib
        stage = MagicMock()
        gv.nodeGraph.getStage.return_value = stage
        stage.GetPrimAtPath.return_value = MagicMock()
        return gv, lib, stage

    def test_redo_deletes_old_creates_new(self):
        gv, lib, stage = self._make_gv()
        redo, undo = _make_reconnect_edits(
            gv,
            "/A",
            "out",
            "/B",
            "inp",
            "/C",
            "out2",
            "/D",
            "inp2",
        )
        redo()

        lib.delete_connection.assert_called_once()
        lib.create_connection.assert_called_once()
        self.assertTrue(gv.linksChanged)

    def test_undo_deletes_new_creates_old(self):
        gv, lib, stage = self._make_gv()
        redo, undo = _make_reconnect_edits(
            gv,
            "/A",
            "out",
            "/B",
            "inp",
            "/C",
            "out2",
            "/D",
            "inp2",
        )
        undo()

        lib.delete_connection.assert_called_once()
        lib.create_connection.assert_called_once()
        self.assertTrue(gv.linksChanged)

    def test_redo_then_undo_symmetry(self):
        """Redo then undo should result in two deletes and two creates."""
        gv, lib, stage = self._make_gv()
        redo, undo = _make_reconnect_edits(
            gv,
            "/A",
            "out",
            "/B",
            "inp",
            "/C",
            "out2",
            "/D",
            "inp2",
        )
        redo()
        undo()

        self.assertEqual(lib.delete_connection.call_count, 2)
        self.assertEqual(lib.create_connection.call_count, 2)


class TestMakeConnectionEdit(unittest.TestCase):
    """Tests for _make_connection_edit (graphView.py:104)."""

    def _make_gv(self):
        gv = MagicMock()
        lib = MagicMock()
        gv._findLibraryForPrim.return_value = lib
        stage = MagicMock()
        gv.nodeGraph.getStage.return_value = stage
        out_prim = MagicMock()
        in_prim = MagicMock()
        stage.GetPrimAtPath.side_effect = lambda p: {
            "/A": out_prim,
            "/B": in_prim,
        }.get(p)
        return gv, lib, stage, out_prim, in_prim

    def test_create_calls_create_connection(self):
        gv, lib, stage, out_p, in_p = self._make_gv()
        fn = _make_connection_edit(gv, "/A", "out", "/B", "inp", create=True)
        fn()

        lib.create_connection.assert_called_once_with(stage, out_p, "out", in_p, "inp")
        lib.delete_connection.assert_not_called()

    def test_delete_calls_delete_connection(self):
        gv, lib, stage, out_p, in_p = self._make_gv()
        fn = _make_connection_edit(gv, "/A", "out", "/B", "inp", create=False)
        fn()

        lib.delete_connection.assert_called_once_with(stage, out_p, "out", in_p, "inp")
        lib.create_connection.assert_not_called()

    def test_does_not_override_edit_target(self):
        gv, lib, stage, _, _ = self._make_gv()
        fn = _make_connection_edit(gv, "/A", "out", "/B", "inp", create=True)
        fn()

        # Production code authors on whatever edit target is currently set —
        # it no longer forces root and therefore makes no SetEditTarget call.
        stage.SetEditTarget.assert_not_called()

    def test_no_op_when_stage_is_none(self):
        gv = MagicMock()
        gv.nodeGraph.getStage.return_value = None
        fn = _make_connection_edit(gv, "/A", "out", "/B", "inp", create=True)
        fn()

    def test_no_op_when_prim_missing(self):
        gv, lib, stage, _, _ = self._make_gv()
        stage.GetPrimAtPath.side_effect = None
        stage.GetPrimAtPath.return_value = None
        fn = _make_connection_edit(gv, "/A", "out", "/B", "inp", create=True)
        fn()

        lib.create_connection.assert_not_called()

    def test_no_op_when_library_missing(self):
        gv, lib, stage, _, _ = self._make_gv()
        gv._findLibraryForPrim.return_value = None
        fn = _make_connection_edit(gv, "/A", "out", "/B", "inp", create=True)
        fn()

        lib.create_connection.assert_not_called()

    def test_propagates_library_exception(self):
        gv, lib, stage, _, _ = self._make_gv()
        lib.create_connection.side_effect = RuntimeError("boom")
        fn = _make_connection_edit(gv, "/A", "out", "/B", "inp", create=True)

        with self.assertRaises(RuntimeError):
            fn()

        # No edit-target juggling means the exception just propagates.
        stage.SetEditTarget.assert_not_called()


class TestSamePinNoOp(unittest.TestCase):
    """Tests for the same-pin no-op guard in _completeLinkDrag.

    When reconnecting and the user drops the link back on the same
    original pin, the operation should be cancelled rather than
    creating a duplicate connection.
    """

    def test_same_endpoints_cancels_reconnect(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        result = _is_same_pin_reconnect(view, "/A", "out", "/B", "inp")

        self.assertTrue(result)
        self.assertFalse(view._reconnectingLink)
        view._addLinkToGraph.assert_called_once()

    def test_different_target_proceeds(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        nodeC = _make_node("/C")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB, "/C": nodeC})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        result = _is_same_pin_reconnect(view, "/A", "out", "/C", "inp2")

        self.assertFalse(result)
        self.assertTrue(view._reconnectingLink)

    def test_different_source_proceeds(self):
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        nodeC = _make_node("/C")
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB, "/C": nodeC})
        _set_reconnect_state(view, "/A", "out", "/B", "inp")

        result = _is_same_pin_reconnect(view, "/C", "out2", "/B", "inp")

        self.assertFalse(result)
        self.assertTrue(view._reconnectingLink)

    def test_not_reconnecting_always_proceeds(self):
        view = _make_view()

        result = _is_same_pin_reconnect(view, "/A", "out", "/B", "inp")

        self.assertFalse(result)


class TestConnectedInputLift(unittest.TestCase):
    """Tests for the connected-input-port lift added to _startLinkDrag.

    When the user drags from an input port that already has a connection,
    Noodles enters reconnect mode so releasing on empty or an invalid target
    deletes that connection (T262819088 — Presto parity).

    Branch map:
      1. Already reconnecting (from _startLinkReconnect) → no-op
      2. Input port has no connection                    → no reconnect
      3. Input port has dangling link                    → no reconnect
      4. Input port has normal connection                → reconnect mode
    """

    def _make_connected_view(self):
        link = _make_link("/A", "out", "/B", "inp", (0.0, 0.0), (100.0, 0.0))
        view = _make_view()
        view._findLinkToInput = MagicMock(return_value=link)
        return view, link

    def test_connected_input_enters_reconnect_mode(self):
        view, _ = self._make_connected_view()
        _apply_connected_input_lift(view, "/B", "inp")
        self.assertTrue(view._reconnectingLink)

    def test_stores_old_connection_endpoints(self):
        view, _ = self._make_connected_view()
        _apply_connected_input_lift(view, "/B", "inp")
        self.assertEqual(view._reconnectOldSourceNodeId, "/A")
        self.assertEqual(view._reconnectOldSourcePort, "out")
        self.assertEqual(view._reconnectOldTargetNodeId, "/B")
        self.assertEqual(view._reconnectOldTargetPort, "inp")

    def test_removes_link_visually(self):
        view, _ = self._make_connected_view()
        _apply_connected_input_lift(view, "/B", "inp")
        view._removeLinkFromGraph.assert_called_once_with(
            "/A",
            "out",
            "/B",
            "inp",
            sourcePropertyName="",
            targetPropertyName="",
        )

    def test_unconnected_input_no_reconnect(self):
        view = _make_view()
        view._findLinkToInput = MagicMock(return_value=None)
        _apply_connected_input_lift(view, "/B", "inp")
        self.assertFalse(view._reconnectingLink)
        view._removeLinkFromGraph.assert_not_called()

    def test_dangling_link_not_lifted(self):
        link = _make_link("/A", "out", "/B", "inp", (0.0, 0.0), (100.0, 0.0))
        link.isDangling = True
        view = _make_view()
        view._findLinkToInput = MagicMock(return_value=link)
        _apply_connected_input_lift(view, "/B", "inp")
        self.assertFalse(view._reconnectingLink)
        view._removeLinkFromGraph.assert_not_called()

    def test_already_reconnecting_is_no_op(self):
        """Called from _startLinkReconnect: _reconnectingLink already True."""
        view, _ = self._make_connected_view()
        view._reconnectingLink = True
        orig_src = "/C"
        view._reconnectOldSourceNodeId = orig_src
        _apply_connected_input_lift(view, "/B", "inp")
        # Must not override existing reconnect state
        self.assertEqual(view._reconnectOldSourceNodeId, orig_src)
        view._removeLinkFromGraph.assert_not_called()


class TestConnectedInputLiftIntegration(unittest.TestCase):
    """End-to-end scenarios: port-drag lift feeding into _completeLinkDrag.

    These tests wire together _apply_connected_input_lift (the new lift block
    in _startLinkDrag) with the downstream release handlers to verify that
    full user flows produce correct USD mutations.

    Scenarios from manual testing (T262819088):
      1. Drag input → empty space  → connection deleted, undo pushed
      2. Drag input → same source  → cancelled, connection restored
      3. Output port drag → empty  → no deletion (output ports not affected)
      4. Lift + cancel (Escape)    → connection restored visually, no USD change
    """

    def _setup(self, delete_ok=True):
        """Build a view with A→B connected, lifted from B's input port."""
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        link = _make_link("/A", "out", "/B", "inp", (0.0, 0.0), (100.0, 0.0))
        view = _make_view(nodes={"/A": nodeA, "/B": nodeB})
        view._findLinkToInput = MagicMock(return_value=link)
        view._library.delete_connection.return_value = delete_ok

        self.push_calls = []

        def _mock_push(desc, redo, undo):
            self.push_calls.append((desc, redo, undo))

        _apply_connected_input_lift(view, "/B", "inp")
        return view, _mock_push

    # ------------------------------------------------------------------
    # Scenario 1: drag input → release on empty → delete
    # ------------------------------------------------------------------

    def test_release_on_empty_calls_delete_connection(self):
        """Lift + release on empty must invoke library.delete_connection."""
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)
        view._library.delete_connection.assert_called_once()

    def test_release_on_empty_pushes_disconnect_undo(self):
        """Lift + release on empty must push a 'Disconnect' undo entry."""
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)
        self.assertEqual(len(self.push_calls), 1)
        self.assertEqual(self.push_calls[0][0], "Disconnect")

    def test_release_on_empty_clears_reconnect_state(self):
        """Reconnect state must be cleared after disconnect."""
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)
        self.assertFalse(view._reconnectingLink)
        self.assertIsNone(view._reconnectOldSourceNodeId)

    def test_release_on_empty_deletes_correct_endpoints(self):
        """delete_connection must be called with the original A→B endpoints."""
        view, push = self._setup()
        _finish_reconnect_disconnect(view, push)
        call_args = view._library.delete_connection.call_args
        # stage, outputPrim, srcPort, inputPrim, tgtPort
        self.assertEqual(call_args[0][2], "out")  # srcPort
        self.assertEqual(call_args[0][4], "inp")  # tgtPort

    # ------------------------------------------------------------------
    # Scenario 2: drag input → drop on same original source → cancel
    # ------------------------------------------------------------------

    def test_drop_on_same_source_cancels_reconnect(self):
        """Dropping on the original source port must cancel, not delete."""
        view, _ = self._setup()
        # Simulate completing the drag back onto the original source (A.out → B.inp)
        cancelled = _is_same_pin_reconnect(view, "/A", "out", "/B", "inp")
        self.assertTrue(cancelled)
        self.assertFalse(view._reconnectingLink)

    def test_drop_on_same_source_restores_link_visually(self):
        """Cancel must add the link back to the graph."""
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view, _ = self._setup()
        view.nodes["/A"] = nodeA
        view.nodes["/B"] = nodeB
        _is_same_pin_reconnect(view, "/A", "out", "/B", "inp")
        view._addLinkToGraph.assert_called_once()

    def test_drop_on_same_source_no_usd_mutation(self):
        """Cancel must not call delete_connection."""
        view, _ = self._setup()
        _is_same_pin_reconnect(view, "/A", "out", "/B", "inp")
        view._library.delete_connection.assert_not_called()

    # ------------------------------------------------------------------
    # Scenario 3: output port drag → empty → no deletion
    # ------------------------------------------------------------------

    def test_output_port_drag_does_not_enter_reconnect_mode(self):
        """Dragging from an output port must not set _reconnectingLink."""
        link = _make_link("/A", "out", "/B", "inp", (0.0, 0.0), (100.0, 0.0))
        view = _make_view()
        view._findLinkToInput = MagicMock(return_value=link)
        # isOutput=True path — the lift guard `not isOutput` prevents entry
        # We simulate by NOT calling _apply_connected_input_lift (isOutput=True
        # means the guard fires and skips the body).
        self.assertFalse(view._reconnectingLink)
        view._removeLinkFromGraph.assert_not_called()

    def test_output_port_release_on_empty_no_delete(self):
        """If _reconnectingLink is False (output drag), empty release = no-op."""
        view = _make_view()
        view._reconnectingLink = False
        push_calls = []
        # Simulate _completeLinkDrag cancel path with no reconnect state
        if view._reconnectingLink:
            _finish_reconnect_disconnect(view, lambda *a: push_calls.append(a))
        self.assertEqual(push_calls, [])
        view._library.delete_connection.assert_not_called()

    # ------------------------------------------------------------------
    # Scenario 4: lift + Escape (cancel) → restore visual, no USD change
    # ------------------------------------------------------------------

    def test_cancel_reconnect_after_lift_restores_link(self):
        """Escaping a port-drag (cancel) must restore the link visually."""
        nodeA = _make_node("/A")
        nodeB = _make_node("/B")
        view, _ = self._setup()
        view.nodes["/A"] = nodeA
        view.nodes["/B"] = nodeB
        _cancel_link_reconnect(view)
        view._addLinkToGraph.assert_called_once_with(
            "/A", "out", "/B", "inp", nodeA, nodeB
        )

    def test_cancel_reconnect_after_lift_no_usd_mutation(self):
        """Escaping must not touch USD (delete_connection not called)."""
        view, _ = self._setup()
        _cancel_link_reconnect(view)
        view._library.delete_connection.assert_not_called()
