#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""Regression test: removing a node should create dangling links, not hide connections.

When a USD prim that is the OTHER end of a connection is removed (via removeSelectedNodes):
1. Relationships: the noodle should NOT render (only dangling circle)
2. Attributes: the port should be filled and double-clickable to restore the missing prim
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
    """Load the 3-node fixture (NodeA -> NodeB -> NodeC) and return the stage."""
    from pxr import Usd

    fixture_path = Path(__file__).parent / "fixtures" / "headless_test_3nodes.usda"
    stage = Usd.Stage.Open(str(fixture_path))
    harness.load_blueprint(stage, "/TestBP")
    return stage


_RELATIONSHIP_USDA = """#usda 1.0

def Blueprint "TestBP"
{
    def ExecNode "Constraint"
    {
        uniform token info:implementationSource = "id"
        token id = "TestConstraint"
        float2 ui:nodegraph:node:pos = (0.1, 0.1)
        prepend rel sources = </TestBP/Driver>
    }

    def ExecNode "Driver"
    {
        uniform token info:implementationSource = "id"
        token id = "TestDriver"
        float2 ui:nodegraph:node:pos = (0.3, 0.1)
    }
}
"""


def _build_relationship_stage():
    """Build an in-memory stage with a relationship connection."""
    from pxr import Sdf, Usd

    layer = Sdf.Layer.CreateAnonymous("relationship_test.usda")
    layer.ImportFromString(_RELATIONSHIP_USDA)
    stage = Usd.Stage.Open(layer)
    return stage


@unittest.skipUnless(_has_headless, "Headless harness not available")
class TestRemoveCreatesDanglingLinks(NoodlesHeadlessTestCase):
    """Removing a connected node must create dangling links, not hide them."""

    def _node_id(self, name):
        return f"/TestBP/{name}"

    def test_remove_middle_attribute_node_creates_dangling(self):
        """Removing NodeB should leave dangling links on NodeA and NodeC ports."""
        _load_3node_fixture(self.harness)
        gv = self.harness.graph_view

        node_b_id = self._node_id("NodeB")
        self.assertIn(node_b_id, gv.nodes, "NodeB should be loaded")

        # Remove NodeB (middle of the chain)
        self.helper.clear_selection()
        self.helper.select_node(node_b_id)
        gv.removeSelectedNodes()

        # NodeB should be gone
        self.assertNotIn(node_b_id, gv.nodes)

        # Links should still exist, marked as dangling
        dangling_links = [
            link for link in gv.links if getattr(link, "isDangling", False)
        ]
        self.assertGreater(
            len(dangling_links),
            0,
            "Removing NodeB should create dangling links (ports should stay filled)",
        )

    def test_find_dangling_link_at_port(self):
        """_findDanglingLinkAtPort should locate dangling links by port hit-test."""
        _load_3node_fixture(self.harness)
        gv = self.harness.graph_view

        node_b_id = self._node_id("NodeB")
        self.helper.clear_selection()
        self.helper.select_node(node_b_id)
        gv.removeSelectedNodes()

        # Get NodeA's output port position
        node_a_id = self._node_id("NodeA")
        node_a = gv.nodes[node_a_id]
        if node_a.outputPins:
            port_name = node_a.outputPins[0]
            port_pos = gv._getPortPosition(node_a, port_name, isOutput=True)
            if port_pos:
                link_idx = gv._findDanglingLinkAtPort(port_pos)
                self.assertGreaterEqual(
                    link_idx,
                    0,
                    "Should find a dangling link at NodeA output port (for double-click)",
                )

    def test_remove_relationship_target_creates_dangling(self):
        """Removing a relationship target should create dangling link (circle only, no noodle)."""
        stage = _build_relationship_stage()
        self.harness.load_blueprint(stage, "/TestBP")
        gv = self.harness.graph_view

        # Both nodes should be loaded
        constraint_id = "/TestBP/Constraint"
        driver_id = "/TestBP/Driver"
        self.assertIn(constraint_id, gv.nodes)
        self.assertIn(driver_id, gv.nodes)

        # Remove Driver (the target of the relationship)
        self.helper.clear_selection()
        self.helper.select_node(driver_id)
        gv.removeSelectedNodes()

        # Driver should be gone
        self.assertNotIn(driver_id, gv.nodes)

        # Links should still exist, marked as dangling
        dangling_links = [
            link for link in gv.links if getattr(link, "isDangling", False)
        ]
        self.assertGreater(
            len(dangling_links),
            0,
            "Removing Driver should create dangling relationship link",
        )
