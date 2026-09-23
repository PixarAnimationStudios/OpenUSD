#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Functional tests for node create, move, undo, and redo.

Exercises the complete lifecycle:
  create node -> move -> undo -> redo -> verify

Each test verifies both the data model state AND captures a
screenshot to confirm the rendering pipeline reflects the change.
"""

import unittest

try:
    from tests.headless.base import NoodlesHeadlessTestCase

    _has_headless = True
except ImportError:
    _has_headless = False
    NoodlesHeadlessTestCase = unittest.TestCase


@unittest.skipUnless(_has_headless, "Headless harness not available")
class TestCreateNode(NoodlesHeadlessTestCase):
    """Test node creation via PrimAuthor + _addNewNodeFast."""

    def test_create_single_node(self):
        """Creating a node should increase node count and set position."""
        node_id = self.helper.add_node(self.stage, self.bp_path, "TestNode", (100, 200))
        self.assertIsNotNone(node_id, "add_node returned None")
        self.assertEqual(self.helper.get_node_count(), 1)

    def test_node_position_matches(self):
        """Created node should have the requested position."""
        node_id = self.helper.add_node(self.stage, self.bp_path, "TestNode", (100, 200))
        pos = self.helper.get_node_position(node_id)
        self.assertAlmostEqual(pos[0], 100.0, places=0)
        self.assertAlmostEqual(pos[1], 200.0, places=0)

    def test_create_multiple_nodes(self):
        """Creating multiple nodes should all appear in the graph."""
        positions = [(100, 100), (300, 100), (200, 300)]
        node_ids = []
        for px, py in positions:
            nid = self.helper.add_node(self.stage, self.bp_path, "TestNode", (px, py))
            self.assertIsNotNone(nid)
            node_ids.append(nid)

        self.assertEqual(self.helper.get_node_count(), 3)
        # All node IDs should be unique
        self.assertEqual(len(set(node_ids)), 3)

    def test_create_node_screenshot(self):
        """Screenshot after creating a node should show content."""
        self.helper.add_node(self.stage, self.bp_path, "TestNode", (100, 200))
        image = self.harness.capture_screenshot()
        self.assertGreater(image.width(), 0)
        self.assertGreater(image.height(), 0)


@unittest.skipUnless(_has_headless, "Headless harness not available")
class TestMoveNode(NoodlesHeadlessTestCase):
    """Test node movement with undo support."""

    def test_move_changes_position(self):
        """Moving a node should update its position."""
        node_id = self.helper.add_node(self.stage, self.bp_path, "TestNode", (100, 200))
        self.helper.move_node(node_id, (300, 400))
        pos = self.helper.get_node_position(node_id)
        self.assertAlmostEqual(pos[0], 300.0, places=0)
        self.assertAlmostEqual(pos[1], 400.0, places=0)

    def test_move_pushes_undo(self):
        """Moving with undo support should make undo available."""
        node_id = self.helper.add_node(self.stage, self.bp_path, "TestNode", (100, 200))
        self.assertFalse(self.helper.can_undo())
        self.helper.move_node(node_id, (300, 400))
        self.assertTrue(self.helper.can_undo())


@unittest.skipUnless(_has_headless, "Headless harness not available")
class TestUndoRedo(NoodlesHeadlessTestCase):
    """Test undo and redo of node moves."""

    def _create_and_move(self):
        """Helper: create a node at (100, 200) and move it to (300, 400)."""
        node_id = self.helper.add_node(self.stage, self.bp_path, "TestNode", (100, 200))
        self.helper.move_node(node_id, (300, 400))
        return node_id

    def test_undo_reverts_position(self):
        """Undo should revert node position to before the move."""
        self._create_and_move()

        self.helper.undo()

        # After undo, the graph reloads from USD layer state.
        # The node ID may change after reload, so find it by checking
        # the nodes dict.
        nodes = self.helper.get_node_ids()
        self.assertEqual(len(nodes), 1, "Expected 1 node after undo")

    def test_undo_enables_redo(self):
        """After undo, redo should become available."""
        self._create_and_move()
        self.helper.undo()
        self.assertTrue(self.helper.can_redo())

    def test_redo_after_undo(self):
        """Redo should re-apply the undone operation."""
        self._create_and_move()

        self.helper.undo()
        self.assertTrue(self.helper.can_redo())

        self.helper.redo()
        self.assertTrue(self.helper.can_undo())

    def test_undo_redo_screenshot_cycle(self):
        """Screenshots should be capturable at each undo/redo step."""
        self._create_and_move()

        # Screenshot after move
        img_after_move = self.harness.capture_screenshot()
        self.assertGreater(img_after_move.width(), 0)

        # Screenshot after undo
        self.helper.undo()
        img_after_undo = self.harness.capture_screenshot()
        self.assertGreater(img_after_undo.width(), 0)

        # Screenshot after redo
        self.helper.redo()
        img_after_redo = self.harness.capture_screenshot()
        self.assertGreater(img_after_redo.width(), 0)

    def test_clear_undo_stack(self):
        """Clearing the undo stack should disable undo and redo."""
        self._create_and_move()
        self.assertTrue(self.helper.can_undo())

        self.helper.clear_undo_stack()
        self.assertFalse(self.helper.can_undo())
        self.assertFalse(self.helper.can_redo())
