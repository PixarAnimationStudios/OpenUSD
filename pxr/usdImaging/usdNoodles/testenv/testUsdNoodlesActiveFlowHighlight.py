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
    from pxr.UsdNoodles.core import LinkData, LinkSelectionMode
    from pxr import Gf
    from pxr.UsdNoodles.graphView import GraphView

    _has_graph = True
except ImportError as e:
    print(f"active-flow highlight imports failed: {e}")
    _has_graph = False


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class LinkDataHighlightBindingTest(unittest.TestCase):
    """Verify that the C++ LinkData highlighted/color bindings work from Python."""

    def test_highlighted_defaults_to_false(self):
        link = LinkData()
        self.assertFalse(link.highlighted)

    def test_highlighted_set_to_true(self):
        link = LinkData()
        link.highlighted = True
        self.assertTrue(link.highlighted)

    def test_highlighted_toggle(self):
        link = LinkData()
        link.highlighted = True
        self.assertTrue(link.highlighted)
        link.highlighted = False
        self.assertFalse(link.highlighted)

    def test_highlight_color_defaults(self):
        link = LinkData()
        color = link.highlightColor
        self.assertEqual(len(color), 4)
        for c in color:
            self.assertAlmostEqual(c, 0.0, places=5)

    def test_highlight_color_set_and_read(self):
        link = LinkData()
        link.highlightColor = [0.31, 0.78, 0.47, 1.0]
        color = link.highlightColor
        self.assertAlmostEqual(color[0], 0.31, places=2)
        self.assertAlmostEqual(color[1], 0.78, places=2)
        self.assertAlmostEqual(color[2], 0.47, places=2)
        self.assertAlmostEqual(color[3], 1.0, places=2)

    def test_color_defaults(self):
        link = LinkData()
        color = link.color
        self.assertEqual(len(color), 4)
        for c in color:
            self.assertAlmostEqual(c, 0.0, places=5)

    def test_color_set_and_read(self):
        link = LinkData()
        link.color = [1.0, 0.5, 0.5, 1.0]
        color = link.color
        self.assertAlmostEqual(color[0], 1.0, places=2)
        self.assertAlmostEqual(color[1], 0.5, places=2)
        self.assertAlmostEqual(color[2], 0.5, places=2)
        self.assertAlmostEqual(color[3], 1.0, places=2)

    def test_highlighted_does_not_affect_selected(self):
        link = LinkData()
        link.highlighted = True
        self.assertFalse(link.selected)

    def test_selected_does_not_affect_highlighted(self):
        link = LinkData()
        link.selected = True
        self.assertFalse(link.highlighted)


def _make_link(source_id, source_port, target_id, target_port):
    """Create a SimpleNamespace link with data_ prefixed node IDs for GraphView."""
    return SimpleNamespace(
        sourceNodeId=source_id,
        sourcePort=source_port,
        targetNodeId=target_id,
        targetPort=target_port,
        data_sourceNodeId=source_id,
        data_targetNodeId=target_id,
        selected=False,
        highlighted=False,
        hovered=False,
        start=Gf.Vec2d(0.0, 0.0),
        end=Gf.Vec2d(10.0, 10.0),
        is_relationship_link=False,
    )


def _make_node(node_id, selected=False):
    """Create a SimpleNamespace node for GraphView."""
    return SimpleNamespace(
        id=node_id,
        selected=selected,
        position=Gf.Vec2d(0.0, 0.0),
        size=Gf.Vec2d(100.0, 50.0),
        inputLinks=[],
        outputLinks=[],
    )


def _make_view(nodes, links, mode=LinkSelectionMode.WITH_ALL_LINKS):
    """Create a minimal SimpleNamespace view for _updateLinkSelectionFromNodes."""
    return SimpleNamespace(
        nodes=nodes,
        links=links,
        _linkSelectionMode=mode,
        _selectedLinks=set(),
        _profiler=MagicMock(),
    )


@unittest.skipUnless(_has_graph, "GraphView modules not available")
class HighlightPropagationTest(unittest.TestCase):
    """Test that node selection propagates to link highlight state."""

    def test_selected_node_highlights_connected_links(self):
        """Select a node, verify connected links are highlighted."""
        node_a = _make_node("/A", selected=True)
        node_b = _make_node("/B", selected=False)
        link = _make_link("/A", "out", "/B", "in")

        view = _make_view(
            {"/A": node_a, "/B": node_b},
            [link],
            LinkSelectionMode.WITH_ALL_LINKS,
        )

        GraphView._updateLinkSelectionFromNodes(view)

        # WITH_ALL_LINKS highlights links connected to selected nodes
        self.assertTrue(link.highlighted)

    def test_deselected_node_clears_link_highlight(self):
        """Deselect a node, verify links lose their highlight."""
        node_a = _make_node("/A", selected=False)
        node_b = _make_node("/B", selected=False)
        link = _make_link("/A", "out", "/B", "in")
        link.highlighted = True  # simulate previously highlighted

        view = _make_view(
            {"/A": node_a, "/B": node_b},
            [link],
            LinkSelectionMode.WITH_ALL_LINKS,
        )

        GraphView._updateLinkSelectionFromNodes(view)

        self.assertFalse(link.highlighted)

    def test_nodes_only_mode_does_not_highlight_links(self):
        """With NODES_ONLY mode, selecting a node should NOT affect links."""
        node_a = _make_node("/A", selected=True)
        node_b = _make_node("/B", selected=False)
        link = _make_link("/A", "out", "/B", "in")

        view = _make_view(
            {"/A": node_a, "/B": node_b},
            [link],
            LinkSelectionMode.NODES_ONLY,
        )

        GraphView._updateLinkSelectionFromNodes(view)

        self.assertFalse(link.highlighted)

    def test_with_inputs_mode_highlights_only_input_links(self):
        """WITH_INPUTS: only links targeting the selected node are highlighted."""
        node_a = _make_node("/A", selected=True)
        node_b = _make_node("/B", selected=False)
        link_out = _make_link("/A", "out", "/B", "in")  # A is source
        link_in = _make_link("/B", "out", "/A", "in")  # A is target

        view = _make_view(
            {"/A": node_a, "/B": node_b},
            [link_out, link_in],
            LinkSelectionMode.WITH_INPUTS,
        )

        GraphView._updateLinkSelectionFromNodes(view)

        # WITH_INPUTS highlights links where the selected node is the target
        self.assertFalse(link_out.highlighted)
        self.assertTrue(link_in.highlighted)

    def test_with_outputs_mode_highlights_only_output_links(self):
        """WITH_OUTPUTS: only links sourced from the selected node are highlighted."""
        node_a = _make_node("/A", selected=True)
        node_b = _make_node("/B", selected=False)
        link_out = _make_link("/A", "out", "/B", "in")  # A is source
        link_in = _make_link("/B", "out", "/A", "in")  # A is target

        view = _make_view(
            {"/A": node_a, "/B": node_b},
            [link_out, link_in],
            LinkSelectionMode.WITH_OUTPUTS,
        )

        GraphView._updateLinkSelectionFromNodes(view)

        # WITH_OUTPUTS highlights links where the selected node is the source
        self.assertTrue(link_out.highlighted)
        self.assertFalse(link_in.highlighted)

    def test_no_links_no_crash(self):
        """Empty links list should not cause errors."""
        node_a = _make_node("/A", selected=True)

        view = _make_view({"/A": node_a}, [], LinkSelectionMode.WITH_ALL_LINKS)

        GraphView._updateLinkSelectionFromNodes(view)

    def test_no_selection_clears_all_highlights(self):
        """No selected nodes should clear all link highlights."""
        node_a = _make_node("/A", selected=False)
        node_b = _make_node("/B", selected=False)
        link = _make_link("/A", "out", "/B", "in")
        link.highlighted = True  # previously highlighted

        view = _make_view(
            {"/A": node_a, "/B": node_b},
            [link],
            LinkSelectionMode.WITH_ALL_LINKS,
        )

        GraphView._updateLinkSelectionFromNodes(view)

        self.assertFalse(link.highlighted)

    def test_multiple_links_only_connected_highlighted(self):
        """Only links connected to selected nodes should be highlighted."""
        node_a = _make_node("/A", selected=True)
        node_b = _make_node("/B", selected=False)
        node_c = _make_node("/C", selected=False)
        link_ab = _make_link("/A", "out", "/B", "in")
        link_bc = _make_link("/B", "out", "/C", "in")  # not connected to A

        view = _make_view(
            {"/A": node_a, "/B": node_b, "/C": node_c},
            [link_ab, link_bc],
            LinkSelectionMode.WITH_ALL_LINKS,
        )

        GraphView._updateLinkSelectionFromNodes(view)

        self.assertTrue(link_ab.highlighted)
        self.assertFalse(link_bc.highlighted)

    def test_link_selection_mode_property_setter_calls_update(self):
        """Setting linkSelectionMode via property should invoke update."""
        node_a = _make_node("/A", selected=True)
        node_b = _make_node("/B", selected=False)
        link = _make_link("/A", "out", "/B", "in")

        view = _make_view(
            {"/A": node_a, "/B": node_b},
            [link],
            LinkSelectionMode.NODES_ONLY,
        )
        view.update = MagicMock()
        view._updateLinkSelectionFromNodes = (
            lambda: GraphView._updateLinkSelectionFromNodes(view)
        )

        # Switch from NODES_ONLY to WITH_ALL_LINKS
        GraphView.linkSelectionMode.fset(view, LinkSelectionMode.WITH_ALL_LINKS)

        self.assertEqual(view._linkSelectionMode, LinkSelectionMode.WITH_ALL_LINKS)
        self.assertTrue(link.highlighted)


if __name__ == "__main__":
    unittest.main()
