#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Tests that deleting a node also removes its USD-side incident connections.

Regression test for the dangling-link bug where deleting a node left its
authored USD connections on surviving prims, causing short stub lines to
render at the surviving node's ports after deletion.
"""

import unittest
from pathlib import Path

try:
    from tests.headless.base import NoodlesHeadlessTestCase

    _has_headless = True
except ImportError:
    _has_headless = False
    NoodlesHeadlessTestCase = unittest.TestCase


def _load_3node_fixture(harness):
    """Load the 3-node fixture (NodeA -> NodeB -> NodeC) and return the BP path."""
    from pxr import Usd

    fixture_path = Path(__file__).parent / "fixtures" / "headless_test_3nodes.usda"
    stage = Usd.Stage.Open(str(fixture_path))
    harness.load_blueprint(stage, "/TestBP")
    return stage


_SUBLAYER_ROOT_USDA = """#usda 1.0

def Blueprint "TestBP"
{
    def ExecNode "NodeA"
    {
        uniform token info:implementationSource = "id"
        token id = "TestNodeA"
        float2 ui:nodegraph:node:pos = (0.1, 0.1)
        token outputs:result = "Float_t"
    }

    def ExecNode "NodeB"
    {
        uniform token info:implementationSource = "id"
        token id = "TestNodeB"
        float2 ui:nodegraph:node:pos = (0.3, 0.1)
        float inputs:x = 0
        token outputs:result = "Float_t"
    }
}
"""


_SUBLAYER_OVERLAY_USDA = """#usda 1.0

over "TestBP"
{
    over "NodeB"
    {
        prepend token inputs:x.connect = </TestBP/NodeA.outputs:result>
    }
}
"""


def _build_sublayer_stage():
    """Build an in-memory stage where the connection is authored on a sublayer.

    Returns (stage, root_layer, overlay_layer). Root defines NodeA and NodeB
    without connections; overlay sublayer adds the inputs:x connection on
    NodeB pointing to NodeA.outputs:result. The overlay layer is the
    expected authoring layer for the NodeA->NodeB connection.
    """
    from pxr import Sdf, Usd

    root = Sdf.Layer.CreateAnonymous("sublayer_test_main.usda")
    root.ImportFromString(_SUBLAYER_ROOT_USDA)
    overlay = Sdf.Layer.CreateAnonymous("sublayer_test_overlay.usda")
    overlay.ImportFromString(_SUBLAYER_OVERLAY_USDA)
    root.subLayerPaths.append(overlay.identifier)
    stage = Usd.Stage.Open(root)
    return stage, root, overlay


@unittest.skipUnless(_has_headless, "Headless harness not available")
class TestDeleteRemovesIncidentLinks(NoodlesHeadlessTestCase):
    """Deleting a node must remove the USD connections that referenced it."""

    def _node_id(self, name):
        return f"/TestBP/{name}"

    def test_delete_middle_node_clears_dangling_link(self):
        """Deleting NodeB should leave no link referencing /TestBP/NodeB."""
        stage = _load_3node_fixture(self.harness)
        gv = self.harness.graph_view

        node_b_id = self._node_id("NodeB")
        self.assertIn(node_b_id, gv.nodes, "NodeB should be loaded")

        self.helper.clear_selection()
        self.helper.select_node(node_b_id)

        gv.deleteSelectedNodes()

        self.assertNotIn(node_b_id, gv.nodes)
        for link in gv.links:
            self.assertNotEqual(link.sourceNodeId, node_b_id)
            self.assertNotEqual(link.targetNodeId, node_b_id)
            self.assertFalse(
                getattr(link, "isDangling", False),
                f"Stale dangling link survived: "
                f"{link.sourceNodeId}.{link.sourcePort} -> "
                f"{link.targetNodeId}.{link.targetPort}",
            )

        node_c = stage.GetPrimAtPath("/TestBP/NodeC")
        self.assertTrue(node_c, "NodeC prim must exist in fixture")
        x_attr = node_c.GetAttribute("inputs:x")
        self.assertTrue(
            x_attr and x_attr.IsValid(),
            "NodeC must have a valid inputs:x attribute",
        )
        for conn in x_attr.GetConnections():
            self.assertNotEqual(
                conn.GetPrimPath().pathString,
                "/TestBP/NodeB",
                "NodeC.inputs:x still has a USD connection to deleted NodeB",
            )

    def test_delete_via_menu_callback_clears_dangling_link(self):
        """The menu path (deleteSelectedNodes) must also clear incident links."""
        _load_3node_fixture(self.harness)
        gv = self.harness.graph_view

        node_b_id = self._node_id("NodeB")
        self.helper.clear_selection()
        self.helper.select_node(node_b_id)

        gv.deleteSelectedNodes()

        for link in gv.links:
            self.assertNotEqual(link.sourceNodeId, node_b_id)
            self.assertNotEqual(link.targetNodeId, node_b_id)
            self.assertFalse(getattr(link, "isDangling", False))

    def test_undo_restores_node_and_connections(self):
        """Undoing a node deletion must restore both the node and its links."""
        _load_3node_fixture(self.harness)
        gv = self.harness.graph_view

        node_b_id = self._node_id("NodeB")
        link_count_before = len(gv.links)

        self.helper.clear_selection()
        self.helper.select_node(node_b_id)
        gv.deleteSelectedNodes()

        self.assertNotIn(node_b_id, gv.nodes)

        self.helper.undo()

        self.assertIn(node_b_id, gv.nodes)
        self.assertEqual(len(gv.links), link_count_before)

    def test_authoring_layer_is_sublayer_and_delete_undo_redo_target_it(self):
        """Multi-sublayer scenario: connection authored on an overlay sublayer.

        Verifies _findConnectionAuthoringLayer returns the overlay identifier
        (not None, not root). Then deletes NodeB and asserts the overlay's
        attribute spec no longer authors the connection. Undo must restore
        the connection ON THE OVERLAY (not on root/session). Redo must
        remove it again from the overlay.
        """
        from pxr import Sdf

        stage, root, overlay = _build_sublayer_stage()
        self.harness.load_blueprint(stage, "/TestBP")
        gv = self.harness.graph_view

        node_a = gv.nodes["/TestBP/NodeA"]
        node_b = gv.nodes["/TestBP/NodeB"]
        a_prim = node_a.getUsdPrim()
        b_prim = node_b.getUsdPrim()

        expected_conn = Sdf.Path("/TestBP/NodeA.outputs:result")

        overlay_spec_before = overlay.GetAttributeAtPath("/TestBP/NodeB.inputs:x")
        self.assertIsNotNone(
            overlay_spec_before,
            "fixture invariant: overlay must author NodeB.inputs:x",
        )
        self.assertIn(
            expected_conn,
            list(overlay_spec_before.connectionPathList.prependedItems),
            "fixture invariant: overlay must prepend NodeA->NodeB connection",
        )

        layer_id = gv._findConnectionAuthoringLayer(b_prim, "x", a_prim, "result")
        self.assertIsNotNone(
            layer_id,
            "_findConnectionAuthoringLayer must locate the overlay layer",
        )
        self.assertNotEqual(
            layer_id,
            root.identifier,
            "Connection is on overlay, not root; identifier must not be root",
        )
        self.assertEqual(
            layer_id,
            overlay.identifier,
            f"Expected overlay identifier {overlay.identifier!r}; got {layer_id!r}",
        )

        self.helper.clear_selection()
        self.helper.select_node("/TestBP/NodeB")
        gv.deleteSelectedNodes()

        self.assertNotIn("/TestBP/NodeB", gv.nodes)

        def _overlay_authors_connection():
            spec = overlay.GetAttributeAtPath("/TestBP/NodeB.inputs:x")
            if not spec:
                return False
            items = (
                list(spec.connectionPathList.explicitItems)
                + list(spec.connectionPathList.prependedItems)
                + list(spec.connectionPathList.appendedItems)
                + list(spec.connectionPathList.addedItems)
            )
            return expected_conn in items

        self.assertFalse(
            _overlay_authors_connection(),
            "After delete, overlay must no longer author the connection",
        )
        root_spec = root.GetAttributeAtPath("/TestBP/NodeB.inputs:x")
        if root_spec:
            root_conns = (
                list(root_spec.connectionPathList.explicitItems)
                + list(root_spec.connectionPathList.prependedItems)
                + list(root_spec.connectionPathList.appendedItems)
                + list(root_spec.connectionPathList.addedItems)
            )
            self.assertNotIn(
                expected_conn,
                root_conns,
                "Delete must not have authored the connection on the root layer",
            )

        self.helper.undo()
        self.assertIn("/TestBP/NodeB", gv.nodes)
        self.assertTrue(
            _overlay_authors_connection(),
            "Undo must restore the connection on the overlay layer",
        )

        self.helper.redo()
        self.assertNotIn("/TestBP/NodeB", gv.nodes)
        self.assertFalse(
            _overlay_authors_connection(),
            "Redo must again remove the connection from the overlay layer",
        )

    def test_redo_reapplies_delete_after_undo(self):
        """Redo (Ctrl+Y) after undo must reapply the delete: node gone, links cleared.

        Mirrors test_undo_restores_node_and_connections so the symmetric
        _applyDeleteSelected path used by redo gets coverage parallel to
        _applyRestoreDeleted used by undo.
        """
        stage = _load_3node_fixture(self.harness)
        gv = self.harness.graph_view

        node_b_id = self._node_id("NodeB")
        link_count_before = len(gv.links)

        self.helper.clear_selection()
        self.helper.select_node(node_b_id)
        gv.deleteSelectedNodes()
        link_count_after_delete = len(gv.links)
        self.assertNotIn(node_b_id, gv.nodes)
        self.assertLess(link_count_after_delete, link_count_before)

        self.helper.undo()
        self.assertIn(node_b_id, gv.nodes)
        self.assertEqual(len(gv.links), link_count_before)

        self.helper.redo()

        self.assertNotIn(node_b_id, gv.nodes)
        self.assertEqual(len(gv.links), link_count_after_delete)
        for link in gv.links:
            self.assertNotEqual(link.sourceNodeId, node_b_id)
            self.assertNotEqual(link.targetNodeId, node_b_id)

        node_c = stage.GetPrimAtPath("/TestBP/NodeC")
        x_attr = node_c.GetAttribute("inputs:x")
        for conn in x_attr.GetConnections():
            self.assertNotEqual(
                conn.GetPrimPath().pathString,
                "/TestBP/NodeB",
                "Redo failed to remove NodeC.inputs:x -> NodeB connection",
            )
